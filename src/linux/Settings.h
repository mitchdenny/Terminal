// Linux port: Simple settings loader for Windows Terminal on Linux
// Reads from ~/.config/windows-terminal/settings.json

#pragma once

#include <string>
#include <array>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>

namespace Linux
{
    struct TerminalSettings
    {
        std::string fontFamily = "Monospace";
        float fontSize = 11.0f;
        std::string shellPath;
        std::string colorSchemeName = "Campbell";

        // Campbell color scheme (Windows Terminal default)
        // 16 ANSI colors (0-15)
        std::array<uint32_t, 16> ansiColors = {{
            0x0C0C0C, // 0: Black
            0xC50F1F, // 1: Red
            0x13A10E, // 2: Green
            0xC19C00, // 3: Yellow
            0x0037DA, // 4: Blue
            0x881798, // 5: Magenta
            0x3A96DD, // 6: Cyan
            0xCCCCCC, // 7: White
            0x767676, // 8: Bright Black
            0xE74856, // 9: Bright Red
            0x16C60C, // 10: Bright Green
            0xF9F1A5, // 11: Bright Yellow
            0x3B78FF, // 12: Bright Blue
            0xB4009E, // 13: Bright Magenta
            0x61D6D6, // 14: Bright Cyan
            0xF2F2F2  // 15: Bright White
        }};

        uint32_t foreground = 0xCCCCCC;
        uint32_t background = 0x0C0C0C;
        uint32_t cursorColor = 0xFFFFFF;

        // Convert RGB (0xRRGGBB) to COLORREF (0x00BBGGRR)
        static uint32_t RgbToColorref(uint32_t rgb)
        {
            uint32_t r = (rgb >> 16) & 0xFF;
            uint32_t g = (rgb >> 8) & 0xFF;
            uint32_t b = rgb & 0xFF;
            return (b << 16) | (g << 8) | r;
        }

        // Get the detected shell path
        std::string GetShellPath() const
        {
            if (!shellPath.empty())
            {
                return shellPath;
            }
            // Try $SHELL environment variable
            const char* shell = std::getenv("SHELL");
            if (shell && shell[0])
            {
                return shell;
            }
            return "/bin/bash";
        }

        // Load settings from file
        bool LoadFromFile(const std::string& path)
        {
            std::ifstream file(path);
            if (!file.is_open())
            {
                return false;
            }

            std::stringstream ss;
            ss << file.rdbuf();
            std::string json = ss.str();

            // Simple JSON key-value extraction (not a full parser)
            auto getString = [&](const std::string& key) -> std::string {
                std::string pattern = "\"" + key + "\"";
                auto pos = json.find(pattern);
                if (pos == std::string::npos) { return ""; }
                pos = json.find(':', pos);
                if (pos == std::string::npos) { return ""; }
                pos = json.find('"', pos);
                if (pos == std::string::npos) { return ""; }
                auto end = json.find('"', pos + 1);
                if (end == std::string::npos) { return ""; }
                return json.substr(pos + 1, end - pos - 1);
            };

            auto getNumber = [&](const std::string& key) -> float {
                std::string pattern = "\"" + key + "\"";
                auto pos = json.find(pattern);
                if (pos == std::string::npos) { return -1.0f; }
                pos = json.find(':', pos);
                if (pos == std::string::npos) { return -1.0f; }
                pos++;
                while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) { pos++; }
                return std::stof(json.substr(pos));
            };

            auto getColor = [&](const std::string& key) -> uint32_t {
                std::string val = getString(key);
                if (val.empty()) { return 0xFFFFFFFF; }
                if (val[0] == '#') { val = val.substr(1); }
                return std::stoul(val, nullptr, 16);
            };

            // Read settings
            std::string ff = getString("fontFamily");
            if (!ff.empty()) { fontFamily = ff; }

            float fs = getNumber("fontSize");
            if (fs > 0) { fontSize = fs; }

            std::string sp = getString("shell");
            if (!sp.empty()) { shellPath = sp; }

            std::string cs = getString("colorScheme");
            if (!cs.empty()) { colorSchemeName = cs; }

            // Color overrides
            uint32_t fg = getColor("foreground");
            if (fg != 0xFFFFFFFF) { foreground = fg; }

            uint32_t bg = getColor("background");
            if (bg != 0xFFFFFFFF) { background = bg; }

            uint32_t cc = getColor("cursorColor");
            if (cc != 0xFFFFFFFF) { cursorColor = cc; }

            // ANSI color overrides
            const char* colorKeys[] = {
                "black", "red", "green", "yellow",
                "blue", "purple", "cyan", "white",
                "brightBlack", "brightRed", "brightGreen", "brightYellow",
                "brightBlue", "brightPurple", "brightCyan", "brightWhite"
            };
            for (int i = 0; i < 16; i++)
            {
                uint32_t c = getColor(colorKeys[i]);
                if (c != 0xFFFFFFFF) { ansiColors[i] = c; }
            }

            return true;
        }

        // Load from default path
        bool LoadDefaults()
        {
            const char* home = std::getenv("HOME");
            if (!home) { return false; }

            std::string path = std::string(home) + "/.config/windows-terminal/settings.json";
            return LoadFromFile(path);
        }

        // Get Pango font description string
        std::string GetPangoFontString() const
        {
            return fontFamily + " " + std::to_string(static_cast<int>(fontSize));
        }
    };
}
