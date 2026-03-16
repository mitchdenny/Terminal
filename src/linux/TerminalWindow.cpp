// Linux port: GTK4 terminal window implementation
// Renders Windows Terminal core output using Cairo+Pango

#include "TerminalWindow.h"

#include <cstring>
#include <iostream>
#include <codecvt>
#include <locale>

using namespace Microsoft::Terminal::Core;
using namespace Microsoft::Terminal::Connection;

namespace Linux
{

// ---- Construction / Destruction ----

TerminalWindow::TerminalWindow()
{
    _settings.LoadDefaults();
    _fontDesc = pango_font_description_from_string(_settings.GetPangoFontString().c_str());
}

TerminalWindow::~TerminalWindow()
{
    Close();
    if (_fontDesc)
    {
        pango_font_description_free(_fontDesc);
    }
    delete _renderer;
}

void TerminalWindow::Close()
{
    _running.store(false);
    _pty.Close();
}

// ---- GTK Application Setup ----

void TerminalWindow::Initialize(GtkApplication* app)
{
    g_signal_connect(app, "activate", G_CALLBACK(OnActivate), this);
}

void TerminalWindow::OnActivate(GtkApplication* app, gpointer userData)
{
    auto* self = static_cast<TerminalWindow*>(userData);
    self->SetupWindow(app);
}

void TerminalWindow::SetupWindow(GtkApplication* app)
{
    // Create main window
    _window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(_window), _windowTitle.c_str());
    gtk_window_set_default_size(GTK_WINDOW(_window), 900, 600);

    // Create drawing area for the terminal
    _drawingArea = gtk_drawing_area_new();
    gtk_widget_set_hexpand(_drawingArea, TRUE);
    gtk_widget_set_vexpand(_drawingArea, TRUE);
    gtk_widget_set_focusable(_drawingArea, TRUE);
    gtk_widget_set_can_focus(_drawingArea, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(_drawingArea), OnDraw, this, nullptr);

    // Keyboard controller
    auto* keyController = gtk_event_controller_key_new();
    g_signal_connect(keyController, "key-pressed", G_CALLBACK(OnKeyPressed), this);
    g_signal_connect(keyController, "key-released", G_CALLBACK(OnKeyReleased), this);
    gtk_widget_add_controller(_drawingArea, keyController);

    // Scroll controller
    auto* scrollController = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
    g_signal_connect(scrollController, "scroll", G_CALLBACK(OnScrolled), this);
    gtk_widget_add_controller(_drawingArea, scrollController);

    // Resize signal
    g_signal_connect(_drawingArea, "resize", G_CALLBACK(OnResize), this);

    // Add drawing area to window
    gtk_window_set_child(GTK_WINDOW(_window), _drawingArea);

    // Compute font metrics using a temporary Cairo surface
    {
        auto* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        auto* cr = cairo_create(surface);
        auto* layout = pango_cairo_create_layout(cr);
        pango_layout_set_font_description(layout, _fontDesc);
        pango_layout_set_text(layout, "M", 1);

        PangoRectangle ink, logical;
        pango_layout_get_pixel_extents(layout, &ink, &logical);

        _cellWidth = logical.width;
        _cellHeight = logical.height;
        auto* iter = pango_layout_get_iter(layout);
        _fontBaseline = pango_layout_iter_get_baseline(iter) / PANGO_SCALE;
        pango_layout_iter_free(iter);

        g_object_unref(layout);
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
    }

    if (_cellWidth <= 0) { _cellWidth = 8; }
    if (_cellHeight <= 0) { _cellHeight = 16; }

    // Compute initial terminal dimensions from window size
    _cols = 900 / _cellWidth;
    _rows = 600 / _cellHeight;
    if (_cols < 10) { _cols = 80; }
    if (_rows < 5) { _rows = 24; }

    // Initialize Terminal core
    {
        auto lock = _terminal.LockForWriting();
        auto& renderSettings = _terminal.GetRenderSettings();
        _renderer = new Microsoft::Console::Render::Renderer(renderSettings, &_terminal);
        _terminal.Create(til::size{ static_cast<til::CoordType>(_cols), static_cast<til::CoordType>(_rows) }, 9001, *_renderer);

        // Apply color scheme from settings
        for (int i = 0; i < 16; i++)
        {
            renderSettings.SetColorTableEntry(i, TerminalSettings::RgbToColorref(_settings.ansiColors[i]));
        }
        renderSettings.SetColorAlias(ColorAlias::DefaultForeground, 256, TerminalSettings::RgbToColorref(_settings.foreground));
        renderSettings.SetColorAlias(ColorAlias::DefaultBackground, 257, TerminalSettings::RgbToColorref(_settings.background));
    }

    // Set up PTY connection
    _pty.SetDimensions(_rows, _cols);

    _pty.OnTerminalOutput([this](std::wstring_view output) {
        auto lock = _terminal.LockForWriting();
        _terminal.Write(output);
        QueueRedraw();
    });

    _pty.OnStateChanged([this](ConnectionState state) {
        if (state == ConnectionState::Closed || state == ConnectionState::Failed)
        {
            _running.store(false);
            // Close the window from the GTK main thread
            g_idle_add([](gpointer data) -> gboolean {
                auto* win = static_cast<GtkWidget*>(data);
                gtk_window_close(GTK_WINDOW(win));
                return G_SOURCE_REMOVE;
            }, _window);
        }
    });

    _pty.Start();

    // Wire up title change callback
    {
        auto lock = _terminal.LockForWriting();
        _terminal.SetTitleChangedCallback([this](std::wstring_view title) {
            // Convert to UTF-8 and update window title on main thread
            std::string utf8;
            for (wchar_t wc : title)
            {
                uint32_t cp = static_cast<uint32_t>(wc);
                if (cp < 0x80)
                {
                    utf8 += static_cast<char>(cp);
                }
                else if (cp < 0x800)
                {
                    utf8 += static_cast<char>(0xC0 | (cp >> 6));
                    utf8 += static_cast<char>(0x80 | (cp & 0x3F));
                }
                else if (cp < 0x10000)
                {
                    if (cp >= 0xD800 && cp <= 0xDFFF) { continue; }
                    utf8 += static_cast<char>(0xE0 | (cp >> 12));
                    utf8 += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                    utf8 += static_cast<char>(0x80 | (cp & 0x3F));
                }
                else if (cp <= 0x10FFFF)
                {
                    utf8 += static_cast<char>(0xF0 | (cp >> 18));
                    utf8 += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                    utf8 += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                    utf8 += static_cast<char>(0x80 | (cp & 0x3F));
                }
            }
            auto* titleCopy = g_strdup(utf8.c_str());
            g_idle_add([](gpointer data) -> gboolean {
                auto* pair = static_cast<std::pair<GtkWidget*, char*>*>(data);
                if (pair->first)
                {
                    gtk_window_set_title(GTK_WINDOW(pair->first), pair->second);
                }
                g_free(pair->second);
                delete pair;
                return G_SOURCE_REMOVE;
            }, new std::pair<GtkWidget*, char*>(_window, titleCopy));
        });
    }

    // Set up a tick callback for cursor blinking
    gtk_widget_add_tick_callback(_drawingArea, OnTickCallback, this, nullptr);

    // Show window and grab focus
    gtk_window_present(GTK_WINDOW(_window));
    gtk_widget_grab_focus(_drawingArea);
}

// ---- Redraw Management ----

void TerminalWindow::QueueRedraw()
{
    _needsRedraw.store(true);
    // Must queue from GTK main thread
    g_idle_add([](gpointer data) -> gboolean {
        auto* self = static_cast<TerminalWindow*>(data);
        if (self->_drawingArea)
        {
            gtk_widget_queue_draw(self->_drawingArea);
        }
        return G_SOURCE_REMOVE;
    }, this);
}

gboolean TerminalWindow::OnTickCallback(GtkWidget* widget, GdkFrameClock* clock, gpointer userData)
{
    auto* self = static_cast<TerminalWindow*>(userData);
    if (!self->_running.load())
    {
        return G_SOURCE_REMOVE;
    }
    // Blink cursor every ~500ms
    static gint64 lastBlink = 0;
    gint64 now = g_get_monotonic_time();
    if (now - lastBlink > 500000) // 500ms
    {
        lastBlink = now;
        gtk_widget_queue_draw(widget);
    }
    return G_SOURCE_CONTINUE;
}

// ---- Drawing ----

void TerminalWindow::OnDraw(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer userData)
{
    auto* self = static_cast<TerminalWindow*>(userData);
    self->DrawTerminal(cr, width, height);
}

static void colorrefToRgb(COLORREF c, double& r, double& g, double& b)
{
    r = static_cast<double>(c & 0xFF) / 255.0;
    g = static_cast<double>((c >> 8) & 0xFF) / 255.0;
    b = static_cast<double>((c >> 16) & 0xFF) / 255.0;
}

void TerminalWindow::DrawTerminal(cairo_t* cr, int widthPx, int heightPx)
{
    auto lock = _terminal.LockForWriting();
    auto& buffer = _terminal.GetTextBuffer();
    auto& renderSettings = _terminal.GetRenderSettings();
    auto viewport = _terminal.GetViewport();

    // Fill background with default background color
    TextAttribute defaultAttr;
    auto [defFg, defBg] = renderSettings.GetAttributeColors(defaultAttr);
    double bgR, bgG, bgB;
    colorrefToRgb(defBg, bgR, bgG, bgB);
    cairo_set_source_rgb(cr, bgR, bgG, bgB);
    cairo_paint(cr);

    // Create Pango layout for text rendering
    auto* layout = pango_cairo_create_layout(cr);
    pango_layout_set_font_description(layout, _fontDesc);

    // Get cursor position
    auto cursorPos = buffer.GetCursor().GetPosition();
    bool cursorVisible = buffer.GetCursor().IsVisible();

    // Static blink toggle
    static bool blinkPhase = true;
    static gint64 lastToggle = 0;
    gint64 now = g_get_monotonic_time();
    if (now - lastToggle > 500000)
    {
        blinkPhase = !blinkPhase;
        lastToggle = now;
    }

    // Render each row
    for (til::CoordType row = viewport.Top(); row < viewport.BottomExclusive(); ++row)
    {
        int screenRow = row - viewport.Top();
        auto& textRow = buffer.GetRowByOffset(row);
        int y = screenRow * _cellHeight;

        if (y >= heightPx) { break; }

        // Render each cell in the row
        til::CoordType col = 0;
        while (col < viewport.Width())
        {
            int x = col * _cellWidth;
            if (x >= widthPx) { break; }

            auto attr = textRow.GetAttrByColumn(col);
            auto [fg, bg] = renderSettings.GetAttributeColors(attr);

            // Find run of cells with same attributes
            til::CoordType runEnd = col + 1;
            while (runEnd < viewport.Width())
            {
                auto nextAttr = textRow.GetAttrByColumn(runEnd);
                auto [nfg, nbg] = renderSettings.GetAttributeColors(nextAttr);
                if (nfg != fg || nbg != bg || nextAttr.IsIntense() != attr.IsIntense() ||
                    nextAttr.IsItalic() != attr.IsItalic() || nextAttr.IsUnderlined() != attr.IsUnderlined())
                {
                    break;
                }
                ++runEnd;
            }

            til::CoordType runLen = runEnd - col;

            // Draw background for run
            if (bg != defBg)
            {
                double r, g, b;
                colorrefToRgb(bg, r, g, b);
                cairo_set_source_rgb(cr, r, g, b);
                cairo_rectangle(cr, x, y, runLen * _cellWidth, _cellHeight);
                cairo_fill(cr);
            }

            // Build text string for the run
            std::string utf8Run;
            utf8Run.reserve(runLen * 4);
            for (til::CoordType c = col; c < runEnd; ++c)
            {
                auto glyph = textRow.GlyphAt(c);
                if (glyph.empty() || glyph[0] == L'\0' || glyph[0] == L' ')
                {
                    utf8Run += ' ';
                }
                else
                {
                    // Convert wchar_t (32-bit on Linux) to UTF-8
                    for (wchar_t wc : glyph)
                    {
                        if (wc == 0) { break; }
                        uint32_t cp = static_cast<uint32_t>(wc);
                        if (cp < 0x80)
                        {
                            utf8Run += static_cast<char>(cp);
                        }
                        else if (cp < 0x800)
                        {
                            utf8Run += static_cast<char>(0xC0 | (cp >> 6));
                            utf8Run += static_cast<char>(0x80 | (cp & 0x3F));
                        }
                        else if (cp < 0x10000)
                        {
                            // Skip surrogates and invalid codepoints
                            if (cp >= 0xD800 && cp <= 0xDFFF) { utf8Run += ' '; continue; }
                            utf8Run += static_cast<char>(0xE0 | (cp >> 12));
                            utf8Run += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                            utf8Run += static_cast<char>(0x80 | (cp & 0x3F));
                        }
                        else if (cp <= 0x10FFFF)
                        {
                            utf8Run += static_cast<char>(0xF0 | (cp >> 18));
                            utf8Run += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                            utf8Run += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                            utf8Run += static_cast<char>(0x80 | (cp & 0x3F));
                        }
                        else
                        {
                            // Invalid codepoint, replace with space
                            utf8Run += ' ';
                        }
                    }
                }
            }

            // Set font style
            pango_font_description_set_weight(_fontDesc,
                attr.IsIntense() ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
            pango_font_description_set_style(_fontDesc,
                attr.IsItalic() ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
            pango_layout_set_font_description(layout, _fontDesc);

            // Draw text
            double fgR, fgG, fgB;
            colorrefToRgb(fg, fgR, fgG, fgB);
            cairo_set_source_rgb(cr, fgR, fgG, fgB);
            cairo_move_to(cr, x, y);
            pango_layout_set_text(layout, utf8Run.c_str(), utf8Run.size());
            pango_cairo_show_layout(cr, layout);

            // Draw underline
            if (attr.IsUnderlined())
            {
                cairo_set_line_width(cr, 1.0);
                cairo_move_to(cr, x, y + _cellHeight - 1.5);
                cairo_line_to(cr, x + runLen * _cellWidth, y + _cellHeight - 1.5);
                cairo_stroke(cr);
            }

            // Draw strikethrough
            if (attr.IsCrossedOut())
            {
                cairo_set_line_width(cr, 1.0);
                cairo_move_to(cr, x, y + _cellHeight / 2);
                cairo_line_to(cr, x + runLen * _cellWidth, y + _cellHeight / 2);
                cairo_stroke(cr);
            }

            col = runEnd;
        }
    }

    // Draw cursor
    if (cursorVisible && blinkPhase)
    {
        int cursorScreenRow = cursorPos.y - viewport.Top();
        if (cursorScreenRow >= 0 && cursorScreenRow < _rows)
        {
            int cx = cursorPos.x * _cellWidth;
            int cy = cursorScreenRow * _cellHeight;

            // Use foreground color for cursor
            double r, g, b;
            colorrefToRgb(defFg, r, g, b);
            cairo_set_source_rgba(cr, r, g, b, 0.7);
            cairo_rectangle(cr, cx, cy, _cellWidth, _cellHeight);
            cairo_fill(cr);
        }
    }

    g_object_unref(layout);

    // Reset font style after drawing
    pango_font_description_set_weight(_fontDesc, PANGO_WEIGHT_NORMAL);
    pango_font_description_set_style(_fontDesc, PANGO_STYLE_NORMAL);
}

// ---- Keyboard Input ----

gboolean TerminalWindow::OnKeyPressed(GtkEventControllerKey* controller, guint keyval,
                                       guint keycode, GdkModifierType state, gpointer userData)
{
    auto* self = static_cast<TerminalWindow*>(userData);
    self->HandleKeyPress(keyval, keycode, state);
    return TRUE;
}

void TerminalWindow::OnKeyReleased(GtkEventControllerKey* controller, guint keyval,
                                    guint keycode, GdkModifierType state, gpointer userData)
{
    // No action needed on key release for now
}

void TerminalWindow::HandleKeyPress(guint keyval, guint keycode, GdkModifierType state)
{
    // Convert GDK keyval to a character or VT sequence to send to PTY
    bool ctrl = (state & GDK_CONTROL_MASK) != 0;
    bool shift = (state & GDK_SHIFT_MASK) != 0;
    bool alt = (state & GDK_ALT_MASK) != 0;

    // Handle clipboard shortcuts
    if (ctrl && shift)
    {
        if (keyval == GDK_KEY_C || keyval == GDK_KEY_c)
        {
            // Copy: get selected text from terminal buffer
            auto lock = _terminal.LockForWriting();
            auto text = _terminal.RetrieveSelectedTextFromBuffer(false).plainText;
            if (!text.empty())
            {
                // Convert wstring to UTF-8
                std::string utf8;
                for (wchar_t wc : text)
                {
                    uint32_t cp = static_cast<uint32_t>(wc);
                    if (cp < 0x80) { utf8 += static_cast<char>(cp); }
                    else if (cp < 0x800) { utf8 += static_cast<char>(0xC0 | (cp >> 6)); utf8 += static_cast<char>(0x80 | (cp & 0x3F)); }
                    else if (cp < 0x10000) { utf8 += static_cast<char>(0xE0 | (cp >> 12)); utf8 += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); utf8 += static_cast<char>(0x80 | (cp & 0x3F)); }
                    else if (cp <= 0x10FFFF) { utf8 += static_cast<char>(0xF0 | (cp >> 18)); utf8 += static_cast<char>(0x80 | ((cp >> 12) & 0x3F)); utf8 += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); utf8 += static_cast<char>(0x80 | (cp & 0x3F)); }
                }
                auto* clipboard = gdk_display_get_clipboard(gtk_widget_get_display(_drawingArea));
                gdk_clipboard_set_text(clipboard, utf8.c_str());
            }
            return;
        }
        if (keyval == GDK_KEY_V || keyval == GDK_KEY_v)
        {
            // Paste from clipboard
            auto* clipboard = gdk_display_get_clipboard(gtk_widget_get_display(_drawingArea));
            gdk_clipboard_read_text_async(clipboard, nullptr,
                [](GObject* source, GAsyncResult* result, gpointer userData) {
                    auto* tw = static_cast<TerminalWindow*>(userData);
                    char* text = gdk_clipboard_read_text_finish(GDK_CLIPBOARD(source), result, nullptr);
                    if (text)
                    {
                        // Convert UTF-8 to wstring and send to PTY
                        std::wstring wide;
                        const char* p = text;
                        while (*p)
                        {
                            uint32_t cp = 0;
                            unsigned char c = static_cast<unsigned char>(*p);
                            if (c < 0x80) { cp = c; p++; }
                            else if (c < 0xE0) { cp = (c & 0x1F) << 6; p++; if (*p) { cp |= (*p & 0x3F); p++; } }
                            else if (c < 0xF0) { cp = (c & 0x0F) << 12; p++; if (*p) { cp |= (*p & 0x3F) << 6; p++; } if (*p) { cp |= (*p & 0x3F); p++; } }
                            else { cp = (c & 0x07) << 18; p++; if (*p) { cp |= (*p & 0x3F) << 12; p++; } if (*p) { cp |= (*p & 0x3F) << 6; p++; } if (*p) { cp |= (*p & 0x3F); p++; } }
                            wide += static_cast<wchar_t>(cp);
                        }
                        tw->_pty.WriteInput(wide);
                        g_free(text);
                    }
                }, this);
            return;
        }
    }

    std::wstring toSend;

    // Handle special keys
    switch (keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
        toSend = L"\r";
        break;
    case GDK_KEY_BackSpace:
        toSend = ctrl ? L"\x1F" : L"\x7F";
        break;
    case GDK_KEY_Tab:
        toSend = shift ? L"\033[Z" : L"\t";
        break;
    case GDK_KEY_Escape:
        toSend = L"\033";
        break;
    case GDK_KEY_Up:
        toSend = alt ? L"\033\033[A" : L"\033[A";
        break;
    case GDK_KEY_Down:
        toSend = alt ? L"\033\033[B" : L"\033[B";
        break;
    case GDK_KEY_Right:
        toSend = alt ? L"\033\033[C" : L"\033[C";
        break;
    case GDK_KEY_Left:
        toSend = alt ? L"\033\033[D" : L"\033[D";
        break;
    case GDK_KEY_Home:
        toSend = L"\033[H";
        break;
    case GDK_KEY_End:
        toSend = L"\033[F";
        break;
    case GDK_KEY_Insert:
        toSend = L"\033[2~";
        break;
    case GDK_KEY_Delete:
        toSend = L"\033[3~";
        break;
    case GDK_KEY_Page_Up:
        if (shift)
        {
            auto lock = _terminal.LockForWriting();
            _terminal.UserScrollViewport(-10);
            QueueRedraw();
            return;
        }
        toSend = L"\033[5~";
        break;
    case GDK_KEY_Page_Down:
        if (shift)
        {
            auto lock = _terminal.LockForWriting();
            _terminal.UserScrollViewport(10);
            QueueRedraw();
            return;
        }
        toSend = L"\033[6~";
        break;
    case GDK_KEY_F1:  toSend = L"\033OP"; break;
    case GDK_KEY_F2:  toSend = L"\033OQ"; break;
    case GDK_KEY_F3:  toSend = L"\033OR"; break;
    case GDK_KEY_F4:  toSend = L"\033OS"; break;
    case GDK_KEY_F5:  toSend = L"\033[15~"; break;
    case GDK_KEY_F6:  toSend = L"\033[17~"; break;
    case GDK_KEY_F7:  toSend = L"\033[18~"; break;
    case GDK_KEY_F8:  toSend = L"\033[19~"; break;
    case GDK_KEY_F9:  toSend = L"\033[20~"; break;
    case GDK_KEY_F10: toSend = L"\033[21~"; break;
    case GDK_KEY_F11: toSend = L"\033[23~"; break;
    case GDK_KEY_F12: toSend = L"\033[24~"; break;

    default:
    {
        // Printable characters
        gunichar uc = gdk_keyval_to_unicode(keyval);
        if (uc == 0)
        {
            return; // Non-printable, non-special key
        }

        if (ctrl && uc >= 'a' && uc <= 'z')
        {
            // Ctrl+a through Ctrl+z → 0x01-0x1A
            wchar_t ctrlChar = static_cast<wchar_t>(uc - 'a' + 1);
            toSend = std::wstring(1, ctrlChar);
        }
        else if (ctrl && uc >= 'A' && uc <= 'Z')
        {
            wchar_t ctrlChar = static_cast<wchar_t>(uc - 'A' + 1);
            toSend = std::wstring(1, ctrlChar);
        }
        else if (ctrl && uc == '@')
        {
            toSend = std::wstring(1, L'\0');
        }
        else if (ctrl && uc == '[')
        {
            toSend = L"\033";
        }
        else if (ctrl && uc == '\\')
        {
            toSend = std::wstring(1, L'\x1C');
        }
        else if (ctrl && uc == ']')
        {
            toSend = std::wstring(1, L'\x1D');
        }
        else if (ctrl && uc == '^')
        {
            toSend = std::wstring(1, L'\x1E');
        }
        else if (ctrl && uc == '_')
        {
            toSend = std::wstring(1, L'\x1F');
        }
        else
        {
            if (alt)
            {
                toSend = L"\033";
            }
            toSend += static_cast<wchar_t>(uc);
        }
        break;
    }
    }

    if (!toSend.empty())
    {
        _pty.WriteInput(toSend);
    }
}

// ---- Scroll ----

gboolean TerminalWindow::OnScrolled(GtkEventControllerScroll* controller, double dx, double dy, gpointer userData)
{
    auto* self = static_cast<TerminalWindow*>(userData);
    self->HandleScroll(dx, dy);
    return TRUE;
}

void TerminalWindow::HandleScroll(double dx, double dy)
{
    auto lock = _terminal.LockForWriting();
    int lines = static_cast<int>(dy * 3);
    if (lines != 0)
    {
        _terminal.UserScrollViewport(lines);
        QueueRedraw();
    }
}

// ---- Resize ----

void TerminalWindow::OnResize(GtkDrawingArea* area, int width, int height, gpointer userData)
{
    auto* self = static_cast<TerminalWindow*>(userData);
    self->HandleResize(width, height);
}

void TerminalWindow::HandleResize(int widthPx, int heightPx)
{
    if (_cellWidth <= 0 || _cellHeight <= 0) { return; }

    int newCols = widthPx / _cellWidth;
    int newRows = heightPx / _cellHeight;

    if (newCols < 2) { newCols = 2; }
    if (newRows < 2) { newRows = 2; }

    if (newCols != _cols || newRows != _rows)
    {
        _cols = newCols;
        _rows = newRows;

        // Resize terminal buffer
        {
            auto lock = _terminal.LockForWriting();
            std::ignore = _terminal.UserResize(til::size{ static_cast<til::CoordType>(_cols), static_cast<til::CoordType>(_rows) });
        }

        // Resize PTY
        _pty.Resize(_rows, _cols);
    }
}

// ---- Title Update ----

void TerminalWindow::UpdateTitle(const std::wstring& title)
{
    if (_window)
    {
        // Convert wstring to UTF-8
        std::string utf8;
        for (wchar_t wc : title)
        {
            if (wc < 0x80)
            {
                utf8 += static_cast<char>(wc);
            }
            else if (wc < 0x800)
            {
                utf8 += static_cast<char>(0xC0 | (wc >> 6));
                utf8 += static_cast<char>(0x80 | (wc & 0x3F));
            }
            else
            {
                utf8 += static_cast<char>(0xE0 | (wc >> 12));
                utf8 += static_cast<char>(0x80 | ((wc >> 6) & 0x3F));
                utf8 += static_cast<char>(0x80 | (wc & 0x3F));
            }
        }
        gtk_window_set_title(GTK_WINDOW(_window), utf8.c_str());
    }
}

} // namespace Linux
