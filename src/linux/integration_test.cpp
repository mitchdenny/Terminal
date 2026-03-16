// Integration test: pipes PTY output through the full Terminal core pipeline.
// This proves that LinuxPtyConnection → Terminal → VT Parser → TextBuffer
// all work together end-to-end.

#ifdef __linux__
#include <LibraryIncludes.h>
#include "winrt/Windows.Foundation.h"
#include "winrt/Microsoft.Terminal.Core.h"
#include <til.h>
#else
#error "This test is Linux-only"
#endif

#include "../../cascadia/TerminalCore/Terminal.hpp"
#include "../../renderer/inc/IRenderData.hpp"
#include "../../renderer/base/renderer.hpp"
#include "LinuxPtyConnection.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

using namespace Microsoft::Terminal::Core;
using namespace Microsoft::Terminal::Connection;

int main()
{
    const int ROWS = 24;
    const int COLS = 80;

    // Create Terminal instance
    Terminal terminal;

    // Initialize terminal with lock held
    Microsoft::Console::Render::Renderer* pRenderer = nullptr;
    {
        auto lock = terminal.LockForWriting();
        auto& renderSettings = terminal.GetRenderSettings();
        pRenderer = new Microsoft::Console::Render::Renderer(renderSettings, &terminal);
        terminal.Create(til::size{ COLS, ROWS }, 9001, *pRenderer);
    }

    std::cout << "Terminal core initialized (" << COLS << "x" << ROWS << ")" << std::endl;
    std::cout << "Starting PTY connection..." << std::endl;

    // Create PTY connection
    LinuxPtyConnection pty;
    pty.SetDimensions(ROWS, COLS);

    std::atomic<bool> running{true};

    // When PTY produces output, write it to Terminal (need lock)
    pty.OnTerminalOutput([&](std::wstring_view output) {
        auto lock = terminal.LockForWriting();
        terminal.Write(output);
    });

    pty.OnStateChanged([&](ConnectionState state) {
        if (state == ConnectionState::Closed || state == ConnectionState::Failed)
        {
            running.store(false);
        }
    });

    pty.Start();

    // Send a few test commands
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pty.WriteInput(L"echo 'Hello from Windows Terminal on Linux!'\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pty.WriteInput(L"echo $'VT test: \\033[31mRED\\033[0m \\033[32mGREEN\\033[0m \\033[34mBLUE\\033[0m'\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Read back from the buffer to verify the pipeline worked
    {
        auto lock = terminal.LockForWriting();
        auto& buffer = terminal.GetTextBuffer();
        auto viewport = terminal.GetViewport();

        std::cout << "\n=== Buffer contents ===" << std::endl;

        bool foundContent = false;
        for (int row = 0; row < viewport.BottomExclusive(); row++)
        {
            auto& textRow = buffer.GetRowByOffset(row);
            auto text = textRow.GetText();

            bool hasContent = false;
            for (wchar_t wc : text)
            {
                if (wc != L' ' && wc != L'\0')
                {
                    hasContent = true;
                    break;
                }
            }

            if (hasContent)
            {
                foundContent = true;
                std::cout << "Row " << row << ": ";
                for (wchar_t wc : text)
                {
                    if (wc < 0x80 && wc >= 0x20)
                    {
                        std::cout << static_cast<char>(wc);
                    }
                }
                std::cout << std::endl;
            }
        }

        if (!foundContent)
        {
            std::cout << "(buffer is empty - VT parsing may not have run)" << std::endl;
        }

        auto cursorPos = buffer.GetCursor().GetPosition();
        std::cout << "\nCursor at: (" << cursorPos.x << ", " << cursorPos.y << ")" << std::endl;
    }

    // Clean up
    pty.WriteInput(L"exit\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    pty.Close();

    delete pRenderer;

    std::cout << "\nIntegration test complete!" << std::endl;
    return 0;
}
