// Quick test for the Linux PTY connection.
// Launches a shell, sends a command, prints output.

#include "LinuxPtyConnection.h"
#include <iostream>
#include <chrono>
#include <thread>

int main()
{
    using namespace Microsoft::Terminal::Connection;

    LinuxPtyConnection pty;
    pty.SetDimensions(24, 80);

    // Print output from the shell
    pty.OnTerminalOutput([](std::wstring_view output) {
        for (wchar_t wc : output)
        {
            if (wc < 0x80)
            {
                std::cout << static_cast<char>(wc);
            }
            else
            {
                // Just print a placeholder for non-ASCII
                std::cout << '?';
            }
        }
        std::cout.flush();
    });

    pty.OnStateChanged([](ConnectionState state) {
        const char* names[] = { "NotConnected", "Connecting", "Connected", "Closing", "Closed", "Failed" };
        std::cerr << "[State: " << names[static_cast<int>(state)] << "]" << std::endl;
    });

    std::cout << "Starting PTY..." << std::endl;
    pty.Start();

    // Give the shell time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send a command
    std::wstring cmd = L"echo 'Hello from Windows Terminal on Linux!'\n";
    pty.WriteInput(cmd);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send exit
    pty.WriteInput(L"exit\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    pty.Close();
    std::cout << std::endl << "PTY test complete." << std::endl;
    return 0;
}
