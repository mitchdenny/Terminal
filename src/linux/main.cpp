// Linux port: GTK4 application entry point for Windows Terminal
// Launches a native Linux terminal window powered by the Windows Terminal core

#include "TerminalWindow.h"
#include <iostream>

int main(int argc, char* argv[])
{
    auto* app = gtk_application_new("com.microsoft.terminal.linux", G_APPLICATION_DEFAULT_FLAGS);

    Linux::TerminalWindow terminalWindow;
    terminalWindow.Initialize(app);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    return status;
}
