// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
// Linux PTY connection - replaces ConptyConnection for Linux.

#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

namespace Microsoft::Terminal::Connection
{
    enum class ConnectionState
    {
        NotConnected = 0,
        Connecting,
        Connected,
        Closing,
        Closed,
        Failed
    };

    // Callback types matching the Windows Terminal interface pattern
    using TerminalOutputHandler = std::function<void(std::wstring_view)>;
    using StateChangedHandler = std::function<void(ConnectionState)>;

    class LinuxPtyConnection
    {
    public:
        LinuxPtyConnection();
        ~LinuxPtyConnection();

        // Non-copyable
        LinuxPtyConnection(const LinuxPtyConnection&) = delete;
        LinuxPtyConnection& operator=(const LinuxPtyConnection&) = delete;

        // Configuration (call before Start)
        void SetCommandline(std::string commandline);
        void SetStartingDirectory(std::string directory);
        void SetDimensions(uint32_t rows, uint32_t columns);
        void SetEnvironment(std::vector<std::string> env);

        // ITerminalConnection interface
        void Start();
        void WriteInput(std::wstring_view data);
        void Resize(uint32_t rows, uint32_t columns);
        void Close();

        ConnectionState State() const noexcept { return _state.load(); }

        // Event registration
        void OnTerminalOutput(TerminalOutputHandler handler) { _outputHandler = std::move(handler); }
        void OnStateChanged(StateChangedHandler handler) { _stateHandler = std::move(handler); }

    private:
        void _outputThread();
        void _transitionToState(ConnectionState state);
        void _launchChild();

        // PTY state
        int _masterFd = -1;
        pid_t _childPid = -1;

        // Configuration
        std::string _commandline;
        std::string _startingDirectory;
        uint32_t _rows = 30;
        uint32_t _columns = 120;
        std::vector<std::string> _environment;

        // Threading
        std::thread _outputThreadHandle;
        std::mutex _writeLock;
        std::atomic<ConnectionState> _state{ConnectionState::NotConnected};

        // Event handlers
        TerminalOutputHandler _outputHandler;
        StateChangedHandler _stateHandler;
    };
}
