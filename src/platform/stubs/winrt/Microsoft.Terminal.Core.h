// Linux stub for winrt/Microsoft.Terminal.Core.h
#pragma once

#include "winrt/Windows.Foundation.h"

namespace winrt::Microsoft::Terminal::Core
{
    enum class CursorStyle : int32_t
    {
        Vintage = 0, Bar = 1, Underscore = 2,
        FilledBox = 3, EmptyBox = 4, DoubleUnderscore = 5,
    };

    enum class AdjustTextMode : int32_t
    {
        Never = 0, Indexed = 1, Always = 2, Automatic = 3,
    };

    enum class MatchMode : int32_t { None = 0, All = 1 };

    struct Color
    {
        uint8_t R, G, B, A;
    };

    // ICoreScheme — base for color scheme data
    struct ICoreScheme
    {
        virtual ~ICoreScheme() = default;
        virtual uint32_t DefaultForeground() const { return 0x00CCCCCC; }
        virtual uint32_t DefaultBackground() const { return 0x00000000; }
        virtual uint32_t CursorColor() const { return 0x00FFFFFF; }
        virtual uint32_t SelectionBackground() const { return 0x00FFFFFF; }
        virtual void GetColorTable(winrt::com_array<Color>& out) const
        {
            out.resize(16);
            // Default xterm colors
            uint32_t defaults[] = {
                0x0C0C0C, 0x0037DA, 0x13A10E, 0x3A96DD,
                0xC50F1F, 0x881798, 0xC19C00, 0xCCCCCC,
                0x767676, 0x3B78FF, 0x16C60C, 0x61D6D6,
                0xE74856, 0xB4009E, 0xF9F1A5, 0xF2F2F2,
            };
            for (int i = 0; i < 16; i++)
            {
                auto c = defaults[i];
                out[i] = Color{ uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c), 0xFF };
            }
        }
    };

    // ICoreAppearance — inherits scheme, adds appearance settings
    struct ICoreAppearance : ICoreScheme
    {
        CursorStyle CursorShape() const { return CursorStyle::Bar; }
        uint32_t CursorHeight() const { return 25; }
        bool IntenseIsBold() const { return true; }
        bool IntenseIsBright() const { return true; }
        AdjustTextMode AdjustIndistinguishableColors() const { return AdjustTextMode::Never; }
    };

    // ICoreSettings — inherits appearance, adds settings
    struct ICoreSettings : ICoreAppearance
    {
        int32_t HistorySize() const { return 9001; }
        int32_t InitialRows() const { return 30; }
        int32_t InitialCols() const { return 120; }
        bool SnapOnInput() const { return true; }
        bool AltGrAliasing() const { return true; }
        winrt::hstring WordDelimiters() const { return L" ./\\()\"'-:,.;<>~!@#$%^&*|+=[]{}~?"; }
        bool TrimBlockSelection() const { return false; }
        bool DetectURLs() const { return true; }
        bool ForceVTInput() const { return false; }
        bool AutoMarkPrompts() const { return false; }
        bool RainbowSuggestions() const { return false; }
        bool AllowKittyKeyboardMode() const { return false; }
        bool ForceDisableKittyKeyboardProtocol() const { return true; }
        bool ForceDisableWin32InputMode() const { return true; }
        bool AllowVtChecksumReport() const { return false; }
        bool AllowVtClipboardWrite() const { return false; }
        winrt::Windows::Foundation::IReference<uint32_t> TabColor() const { return {}; }
        winrt::Windows::Foundation::IReference<uint32_t> StartingTabColor() const { return {}; }
        winrt::hstring StartingTitle() const { return L""; }
        bool SuppressApplicationTitle() const { return false; }
        winrt::hstring AnswerbackMessage() const { return L""; }
    };
}
