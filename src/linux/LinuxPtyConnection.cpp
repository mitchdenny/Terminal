// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
// Linux PTY connection implementation.

#include "LinuxPtyConnection.h"

#include <pty.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <cstring>
#include <cstdlib>
#include <codecvt>
#include <locale>
#include <array>
#include <stdexcept>

namespace Microsoft::Terminal::Connection
{
    LinuxPtyConnection::LinuxPtyConnection()
    {
        // Default shell from $SHELL or fall back to /bin/bash
        const char* shell = getenv("SHELL");
        _commandline = shell ? shell : "/bin/bash";
    }

    LinuxPtyConnection::~LinuxPtyConnection()
    {
        Close();
    }

    void LinuxPtyConnection::SetCommandline(std::string commandline)
    {
        _commandline = std::move(commandline);
    }

    void LinuxPtyConnection::SetStartingDirectory(std::string directory)
    {
        _startingDirectory = std::move(directory);
    }

    void LinuxPtyConnection::SetDimensions(uint32_t rows, uint32_t columns)
    {
        _rows = rows;
        _columns = columns;
    }

    void LinuxPtyConnection::SetEnvironment(std::vector<std::string> env)
    {
        _environment = std::move(env);
    }

    void LinuxPtyConnection::_transitionToState(ConnectionState state)
    {
        _state.store(state);
        if (_stateHandler)
        {
            _stateHandler(state);
        }
    }

    void LinuxPtyConnection::Start()
    {
        _transitionToState(ConnectionState::Connecting);

        // Set up PTY dimensions
        struct winsize ws{};
        ws.ws_row = static_cast<unsigned short>(_rows);
        ws.ws_col = static_cast<unsigned short>(_columns);

        // Create PTY
        int slaveFd = -1;
        if (openpty(&_masterFd, &slaveFd, nullptr, nullptr, &ws) == -1)
        {
            _transitionToState(ConnectionState::Failed);
            throw std::runtime_error("Failed to create PTY: " + std::string(strerror(errno)));
        }

        // Fork child process
        _childPid = fork();
        if (_childPid == -1)
        {
            close(_masterFd);
            close(slaveFd);
            _masterFd = -1;
            _transitionToState(ConnectionState::Failed);
            throw std::runtime_error("Failed to fork: " + std::string(strerror(errno)));
        }

        if (_childPid == 0)
        {
            // --- Child process ---
            close(_masterFd);

            // Create new session and set controlling terminal
            setsid();
            ioctl(slaveFd, TIOCSCTTY, 0);

            // Redirect stdin/stdout/stderr to slave PTY
            dup2(slaveFd, STDIN_FILENO);
            dup2(slaveFd, STDOUT_FILENO);
            dup2(slaveFd, STDERR_FILENO);
            if (slaveFd > STDERR_FILENO)
            {
                close(slaveFd);
            }

            // Change directory if specified
            if (!_startingDirectory.empty())
            {
                chdir(_startingDirectory.c_str());
            }

            // Set environment variables
            setenv("TERM", "xterm-256color", 1);
            setenv("COLORTERM", "truecolor", 1);
            setenv("TERM_PROGRAM", "WindowsTerminal", 1);
            setenv("TERM_PROGRAM_VERSION", "linux-port", 1);
            for (const auto& envVar : _environment)
            {
                auto eq = envVar.find('=');
                if (eq != std::string::npos)
                {
                    std::string key = envVar.substr(0, eq);
                    std::string val = envVar.substr(eq + 1);
                    setenv(key.c_str(), val.c_str(), 1);
                }
            }

            // Execute shell
            // Parse commandline into argv
            // For simplicity, use shell -c for complex commands, or exec directly
            if (_commandline.find(' ') != std::string::npos)
            {
                // Complex command — use shell -c
                const char* shell = getenv("SHELL");
                if (!shell) shell = "/bin/bash";
                execlp(shell, shell, "-c", _commandline.c_str(), nullptr);
            }
            else
            {
                // Simple command — exec directly as login shell
                std::string shellName = "-" + _commandline.substr(_commandline.rfind('/') + 1);
                execlp(_commandline.c_str(), shellName.c_str(), nullptr);
            }

            // If exec failed
            _exit(127);
        }

        // --- Parent process ---
        close(slaveFd);

        // Start output reading thread
        _outputThreadHandle = std::thread(&LinuxPtyConnection::_outputThread, this);

        _transitionToState(ConnectionState::Connected);
    }

    void LinuxPtyConnection::WriteInput(std::wstring_view data)
    {
        if (_state.load() != ConnectionState::Connected)
        {
            return;
        }

        // Convert wchar_t (UTF-32 on Linux) to UTF-8 for the PTY
        std::string utf8;
        utf8.reserve(data.size() * 4);
        for (wchar_t wc : data)
        {
            if (wc < 0x80)
            {
                utf8.push_back(static_cast<char>(wc));
            }
            else if (wc < 0x800)
            {
                utf8.push_back(static_cast<char>(0xC0 | (wc >> 6)));
                utf8.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
            }
            else if (wc < 0x10000)
            {
                utf8.push_back(static_cast<char>(0xE0 | (wc >> 12)));
                utf8.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
                utf8.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
            }
            else
            {
                utf8.push_back(static_cast<char>(0xF0 | (wc >> 18)));
                utf8.push_back(static_cast<char>(0x80 | ((wc >> 12) & 0x3F)));
                utf8.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
                utf8.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
            }
        }

        std::lock_guard lock(_writeLock);
        ssize_t written = 0;
        while (written < static_cast<ssize_t>(utf8.size()))
        {
            ssize_t result = write(_masterFd, utf8.data() + written, utf8.size() - written);
            if (result < 0)
            {
                if (errno == EINTR) continue;
                break;
            }
            written += result;
        }
    }

    void LinuxPtyConnection::Resize(uint32_t rows, uint32_t columns)
    {
        _rows = rows;
        _columns = columns;

        if (_masterFd >= 0)
        {
            struct winsize ws{};
            ws.ws_row = static_cast<unsigned short>(rows);
            ws.ws_col = static_cast<unsigned short>(columns);
            ioctl(_masterFd, TIOCSWINSZ, &ws);
        }
    }

    void LinuxPtyConnection::Close()
    {
        auto expected = ConnectionState::Connected;
        if (!_state.compare_exchange_strong(expected, ConnectionState::Closing))
        {
            return;
        }

        // Close master fd which causes the output thread to exit
        if (_masterFd >= 0)
        {
            close(_masterFd);
            _masterFd = -1;
        }

        // Wait for output thread (but don't try to join from the thread itself)
        if (_outputThreadHandle.joinable() &&
            _outputThreadHandle.get_id() != std::this_thread::get_id())
        {
            _outputThreadHandle.join();
        }
        else if (_outputThreadHandle.joinable())
        {
            _outputThreadHandle.detach();
        }

        // Wait for child process
        if (_childPid > 0)
        {
            int status;
            // Give child a chance to exit gracefully
            kill(_childPid, SIGHUP);
            if (waitpid(_childPid, &status, WNOHANG) == 0)
            {
                // Still running, wait a bit then force kill
                usleep(100000); // 100ms
                kill(_childPid, SIGKILL);
                waitpid(_childPid, &status, 0);
            }
            _childPid = -1;
        }

        _transitionToState(ConnectionState::Closed);
    }

    void LinuxPtyConnection::_outputThread()
    {
        std::array<char, 128 * 1024> buffer{};
        std::wstring wstr;

        // UTF-8 decoder state
        uint32_t codepoint = 0;
        int bytesRemaining = 0;

        while (_state.load() == ConnectionState::Connected ||
               _state.load() == ConnectionState::Connecting)
        {
            ssize_t bytesRead = read(_masterFd, buffer.data(), buffer.size());

            if (bytesRead <= 0)
            {
                if (bytesRead == -1 && errno == EINTR)
                {
                    continue;
                }
                break; // EOF or error — child exited
            }

            // Convert UTF-8 to wchar_t (UTF-32 on Linux)
            wstr.clear();
            wstr.reserve(static_cast<size_t>(bytesRead));

            for (ssize_t i = 0; i < bytesRead; ++i)
            {
                uint8_t byte = static_cast<uint8_t>(buffer[i]);

                if (bytesRemaining > 0)
                {
                    if ((byte & 0xC0) == 0x80)
                    {
                        codepoint = (codepoint << 6) | (byte & 0x3F);
                        if (--bytesRemaining == 0)
                        {
                            wstr.push_back(static_cast<wchar_t>(codepoint));
                        }
                    }
                    else
                    {
                        // Invalid continuation — emit replacement and restart
                        wstr.push_back(L'\uFFFD');
                        bytesRemaining = 0;
                        --i; // Re-process this byte
                    }
                }
                else if (byte < 0x80)
                {
                    wstr.push_back(static_cast<wchar_t>(byte));
                }
                else if ((byte & 0xE0) == 0xC0)
                {
                    codepoint = byte & 0x1F;
                    bytesRemaining = 1;
                }
                else if ((byte & 0xF0) == 0xE0)
                {
                    codepoint = byte & 0x0F;
                    bytesRemaining = 2;
                }
                else if ((byte & 0xF8) == 0xF0)
                {
                    codepoint = byte & 0x07;
                    bytesRemaining = 3;
                }
                else
                {
                    wstr.push_back(L'\uFFFD');
                }
            }

            // Fire output event
            if (!wstr.empty() && _outputHandler)
            {
                _outputHandler(wstr);
            }
        }

        // Child exited
        if (_state.load() != ConnectionState::Closing)
        {
            Close();
        }
    }
}
