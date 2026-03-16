// Linux port: GTK4 terminal window that renders the Windows Terminal core
// Uses Cairo+Pango for text rendering, reads TextBuffer directly

#pragma once

#ifdef __linux__
#include <LibraryIncludes.h>
#include "winrt/Windows.Foundation.h"
#include "winrt/Microsoft.Terminal.Core.h"
#include <til.h>
#endif

#include "../../cascadia/TerminalCore/Terminal.hpp"
#include "../../cascadia/TerminalCore/ControlKeyStates.hpp"
#include "../../renderer/inc/IRenderData.hpp"
#include "../../renderer/base/renderer.hpp"
#include "LinuxPtyConnection.h"
#include "Settings.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <pango/pangocairo.h>

#include <string>
#include <mutex>
#include <atomic>
#include <functional>

namespace Linux
{
    class TerminalWindow
    {
    public:
        TerminalWindow();
        ~TerminalWindow();

        void Initialize(GtkApplication* app);
        void Close();

    private:
        // GTK signal handlers (static C callbacks forward to instance)
        static void OnActivate(GtkApplication* app, gpointer userData);
        static void OnDraw(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer userData);
        static gboolean OnKeyPressed(GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType state, gpointer userData);
        static void OnKeyReleased(GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType state, gpointer userData);
        static gboolean OnScrolled(GtkEventControllerScroll* controller, double dx, double dy, gpointer userData);
        static void OnResize(GtkDrawingArea* area, int width, int height, gpointer userData);
        static gboolean OnTickCallback(GtkWidget* widget, GdkFrameClock* clock, gpointer userData);

        // Mouse handlers
        static void OnMousePressed(GtkGestureClick* gesture, int nPress, double x, double y, gpointer userData);
        static void OnMouseReleased(GtkGestureClick* gesture, int nPress, double x, double y, gpointer userData);
        static void OnMouseMoved(GtkEventControllerMotion* controller, double x, double y, gpointer userData);

        // Instance methods
        void SetupWindow(GtkApplication* app);
        void DrawTerminal(cairo_t* cr, int widthPx, int heightPx);
        void HandleKeyPress(guint keyval, guint keycode, GdkModifierType state);
        void HandleMousePress(int button, int nPress, double x, double y, GdkModifierType mods);
        void HandleMouseRelease(int button, double x, double y, GdkModifierType mods);
        void HandleMouseMove(double x, double y, GdkModifierType mods);
        void HandleScroll(double dx, double dy, GdkModifierType mods);
        void HandleResize(int widthPx, int heightPx);
        til::point PixelToCell(double x, double y) const;
        Microsoft::Terminal::Core::ControlKeyStates GdkModsToControlKeys(GdkModifierType mods) const;
        void QueueRedraw();
        void UpdateTitle(const std::wstring& title);

        // Key translation
        WORD GdkKeyvalToVK(guint keyval) const;
        wchar_t GdkKeyvalToChar(guint keyval) const;

        // Terminal core
        Microsoft::Terminal::Core::Terminal _terminal;
        Microsoft::Console::Render::Renderer* _renderer = nullptr;
        Microsoft::Terminal::Connection::LinuxPtyConnection _pty;

        // GTK widgets
        GtkWidget* _window = nullptr;
        GtkWidget* _drawingArea = nullptr;
        GtkWidget* _headerBar = nullptr;
        GtkWidget* _notebook = nullptr;

        // Font metrics (computed once on font load)
        PangoFontDescription* _fontDesc = nullptr;
        int _cellWidth = 0;
        int _cellHeight = 0;
        int _fontBaseline = 0;

        // Terminal dimensions in cells
        int _rows = 24;
        int _cols = 80;

        // State
        std::atomic<bool> _needsRedraw{false};
        std::atomic<bool> _running{true};
        std::string _windowTitle = "Windows Terminal (Linux)";
        bool _mouseLeftDown = false;
        bool _mouseMiddleDown = false;
        bool _mouseRightDown = false;

        // Settings
        TerminalSettings _settings;
    };
}
