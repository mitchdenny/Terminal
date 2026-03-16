// Stub implementations for functions excluded from the Linux build.

#ifdef __linux__
#include <LibraryIncludes.h>
#include <til.h>
#endif

#include <string>
#include <string_view>

struct UText;
struct URegularExpression;
class TextBuffer;

namespace Microsoft::Console::ICU
{
    // UTextAdapter functions — stubbed since ICU isn't linked yet
    void* UTextFromTextBuffer(const TextBuffer&, int, int) noexcept
    {
        return nullptr;
    }

    til::point_span BufferRangeFromMatch(UText*, URegularExpression*)
    {
        return {};
    }
}

namespace Microsoft::Console::Utils
{
    std::wstring_view TrimPaste(std::wstring_view textView) noexcept
    {
        auto end = textView.find_last_not_of(L" \t\r\n");
        if (end != std::wstring_view::npos)
        {
            return textView.substr(0, end + 1);
        }
        return textView;
    }

    std::vector<std::wstring_view> SplitString(const std::wstring_view str, const wchar_t delimiter) noexcept
    {
        std::vector<std::wstring_view> result;
        size_t start = 0;
        size_t pos;
        while ((pos = str.find(delimiter, start)) != std::wstring_view::npos)
        {
            result.push_back(str.substr(start, pos - start));
            start = pos + 1;
        }
        result.push_back(str.substr(start));
        return result;
    }

    std::tuple<int, int, int> ColorToHLS(const til::color) noexcept { return {0, 0, 0}; }
    til::color ColorFromHLS(const int, const int, const int) noexcept { return {}; }
    std::tuple<int, int, int> ColorToRGB100(const til::color) noexcept { return {0, 0, 0}; }
    til::color ColorFromRGB100(const int, const int, const int) noexcept { return {}; }

    bool StringToUint(const std::wstring_view str, unsigned int& result) noexcept
    {
        result = 0;
        for (wchar_t c : str)
        {
            if (c < L'0' || c > L'9') return false;
            result = result * 10 + (c - L'0');
        }
        return true;
    }

    std::optional<til::color> ColorFromXTermColor(const std::wstring_view) noexcept
    {
        return std::nullopt;
    }

    std::wstring ColorToHexString(const til::color c)
    {
        wchar_t buf[8];
        swprintf(buf, sizeof(buf)/sizeof(buf[0]), L"#%02x%02x%02x", c.r, c.g, c.b);
        return buf;
    }

    size_t FindActionableControlCharacter(const wchar_t* str, size_t len) noexcept
    {
        for (size_t i = 0; i < len; i++)
        {
            wchar_t c = str[i];
            if (c < 0x20 || c == 0x7F) return i;
        }
        return len;
    }
}
