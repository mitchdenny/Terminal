// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
// Linux compatibility layer for Windows types and macros.

#pragma once

#ifdef __linux__

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <climits>
#include <cerrno>
#include <stdexcept>
#include <string>

// --- Basic Windows types ---
using BYTE = uint8_t;
using WORD = uint16_t;
using DWORD = uint32_t;
using UINT = unsigned int;
using INT = int;
using LONG = int32_t;
using ULONG = uint32_t;
using USHORT = uint16_t;
using SHORT = int16_t;
using BOOL = int;
using BOOLEAN = uint8_t;
using CHAR = char;
using WCHAR = wchar_t;
using FLOAT = float;
using HRESULT = int32_t;
using NTSTATUS = int32_t;
using COLORREF = uint32_t;

using UINT8 = uint8_t;
using UINT16 = uint16_t;
using UINT32 = uint32_t;
using UINT64 = uint64_t;
using INT8 = int8_t;
using INT16 = int16_t;
using INT32 = int32_t;
using INT64 = int64_t;
using UCHAR = unsigned char;

using LPWSTR = WCHAR*;
using LPCWSTR = const WCHAR*;
using PCWSTR = const WCHAR*;
using PWSTR = WCHAR*;
using LPSTR = char*;
using LPCSTR = const char*;

using HANDLE = void*;
using HWND = void*;
using HICON = void*;
using HMODULE = void*;
using HINSTANCE = void*;
using HPCON = void*;
using HDC = void*;
using HFONT = void*;
using HBRUSH = void*;
using HRGN = void*;

using SIZE_T = size_t;
using SSIZE_T = ssize_t;
using WPARAM = uintptr_t;
using LPARAM = intptr_t;
using LRESULT = intptr_t;

// --- INVALID_HANDLE_VALUE ---
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)

// --- HRESULT helpers ---
#define S_OK ((HRESULT)0)
#define S_FALSE ((HRESULT)1)
#define E_FAIL ((HRESULT)0x80004005)
#define E_INVALIDARG ((HRESULT)0x80070057)
#define E_OUTOFMEMORY ((HRESULT)0x8007000E)
#define E_NOTIMPL ((HRESULT)0x80004001)
#define E_UNEXPECTED ((HRESULT)0x8000FFFF)
#define E_ABORT ((HRESULT)0x80004004)
#define E_ACCESSDENIED ((HRESULT)0x80070005)
#define E_POINTER ((HRESULT)0x80004003)
#define E_NOINTERFACE ((HRESULT)0x80004002)

#define HRESULT_FROM_WIN32(x) ((HRESULT)(x) <= 0 ? (HRESULT)(x) : (HRESULT)(((x) & 0x0000FFFF) | 0x80070000))
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)

#define RETURN_WIN32(x) return HRESULT_FROM_WIN32(x)

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

#define ERROR_UNHANDLED_EXCEPTION 574

// --- COORD and SMALL_RECT (console types) ---
#define _WINCONTYPES_ 1

struct COORD
{
    SHORT X;
    SHORT Y;
};

struct SMALL_RECT
{
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
};

struct CHAR_INFO
{
    union
    {
        WCHAR UnicodeChar;
        CHAR AsciiChar;
    } Char;
    WORD Attributes;
};

// --- Console attribute constants ---
#define FOREGROUND_BLUE 0x0001
#define FOREGROUND_GREEN 0x0002
#define FOREGROUND_RED 0x0004
#define FOREGROUND_INTENSITY 0x0008
#define BACKGROUND_BLUE 0x0010
#define BACKGROUND_GREEN 0x0020
#define BACKGROUND_RED 0x0040
#define BACKGROUND_INTENSITY 0x0080

#define COMMON_LVB_LEADING_BYTE 0x0100
#define COMMON_LVB_TRAILING_BYTE 0x0200
#define COMMON_LVB_SBCSDBCS (COMMON_LVB_LEADING_BYTE | COMMON_LVB_TRAILING_BYTE)
#define COMMON_LVB_GRID_HORIZONTAL 0x0400
#define COMMON_LVB_GRID_LVERTICAL 0x0800
#define COMMON_LVB_GRID_RVERTICAL 0x1000
#define COMMON_LVB_REVERSE_VIDEO 0x4000
#define COMMON_LVB_UNDERSCORE 0x8000

// --- Virtual key codes ---
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_CLEAR 0x0C
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_PAUSE 0x13
#define VK_CAPITAL 0x14
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_SELECT 0x29
#define VK_PRINT 0x2A
#define VK_EXECUTE 0x2B
#define VK_SNAPSHOT 0x2C
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_HELP 0x2F
#define VK_LWIN 0x5B
#define VK_RWIN 0x5C
#define VK_APPS 0x5D
#define VK_SLEEP 0x5F
#define VK_NUMPAD0 0x60
#define VK_NUMPAD1 0x61
#define VK_NUMPAD2 0x62
#define VK_NUMPAD3 0x63
#define VK_NUMPAD4 0x64
#define VK_NUMPAD5 0x65
#define VK_NUMPAD6 0x66
#define VK_NUMPAD7 0x67
#define VK_NUMPAD8 0x68
#define VK_NUMPAD9 0x69
#define VK_MULTIPLY 0x6A
#define VK_ADD 0x6B
#define VK_SEPARATOR 0x6C
#define VK_SUBTRACT 0x6D
#define VK_DECIMAL 0x6E
#define VK_DIVIDE 0x6F
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_F13 0x7C
#define VK_F14 0x7D
#define VK_F15 0x7E
#define VK_F16 0x7F
#define VK_F17 0x80
#define VK_F18 0x81
#define VK_F19 0x82
#define VK_F20 0x83
#define VK_F21 0x84
#define VK_F22 0x85
#define VK_F23 0x86
#define VK_F24 0x87
#define VK_NUMLOCK 0x90
#define VK_SCROLL 0x91
#define VK_LSHIFT 0xA0
#define VK_RSHIFT 0xA1
#define VK_LCONTROL 0xA2
#define VK_RCONTROL 0xA3
#define VK_LMENU 0xA4
#define VK_RMENU 0xA5
#define VK_OEM_1 0xBA
#define VK_OEM_PLUS 0xBB
#define VK_OEM_COMMA 0xBC
#define VK_OEM_MINUS 0xBD
#define VK_OEM_PERIOD 0xBE
#define VK_OEM_2 0xBF
#define VK_OEM_3 0xC0
#define VK_OEM_4 0xDB
#define VK_OEM_5 0xDC
#define VK_OEM_6 0xDD
#define VK_OEM_7 0xDE
#define VK_OEM_8 0xDF

// --- Key event flags ---
#define LEFT_CTRL_PRESSED 0x0008
#define RIGHT_CTRL_PRESSED 0x0004
#define LEFT_ALT_PRESSED 0x0002
#define RIGHT_ALT_PRESSED 0x0001
#define SHIFT_PRESSED 0x0010
#define NUMLOCK_ON 0x0020
#define SCROLLLOCK_ON 0x0040
#define CAPSLOCK_ON 0x0080
#define ENHANCED_KEY 0x0100
#define NLS_DBCSCHAR 0x00010000
#define NLS_ALPHANUMERIC 0x0
#define NLS_KATAKANA 0x00020000
#define NLS_HIRAGANA 0x00040000
#define NLS_ROMAN 0x00400000
#define NLS_IME_CONVERSION 0x00800000
#define ALTNUMPAD_BIT 0x04000000
#define NLS_IME_DISABLE 0x20000000

// --- INPUT_RECORD types ---
#define KEY_EVENT 0x0001
#define MOUSE_EVENT 0x0002
#define WINDOW_BUFFER_SIZE_EVENT 0x0004
#define MENU_EVENT 0x0008
#define FOCUS_EVENT 0x0010

struct KEY_EVENT_RECORD
{
    BOOL bKeyDown;
    WORD wRepeatCount;
    WORD wVirtualKeyCode;
    WORD wVirtualScanCode;
    union
    {
        WCHAR UnicodeChar;
        CHAR AsciiChar;
    } uChar;
    DWORD dwControlKeyState;
};

struct MOUSE_EVENT_RECORD
{
    COORD dwMousePosition;
    DWORD dwButtonState;
    DWORD dwControlKeyState;
    DWORD dwEventFlags;
};

struct WINDOW_BUFFER_SIZE_RECORD
{
    COORD dwSize;
};

struct MENU_EVENT_RECORD
{
    UINT dwCommandId;
};

struct FOCUS_EVENT_RECORD
{
    BOOL bSetFocus;
};

struct INPUT_RECORD
{
    WORD EventType;
    union
    {
        KEY_EVENT_RECORD KeyEvent;
        MOUSE_EVENT_RECORD MouseEvent;
        WINDOW_BUFFER_SIZE_RECORD WindowBufferSizeEvent;
        MENU_EVENT_RECORD MenuEvent;
        FOCUS_EVENT_RECORD FocusEvent;
    } Event;
};

// --- Mouse button constants ---
#define FROM_LEFT_1ST_BUTTON_PRESSED 0x0001
#define RIGHTMOST_BUTTON_PRESSED 0x0002
#define FROM_LEFT_2ND_BUTTON_PRESSED 0x0004
#define FROM_LEFT_3RD_BUTTON_PRESSED 0x0008
#define FROM_LEFT_4TH_BUTTON_PRESSED 0x0010

#define MOUSE_MOVED 0x0001
#define DOUBLE_CLICK 0x0002
#define MOUSE_WHEELED 0x0004
#define MOUSE_HWHEELED 0x0008

// --- Console mode flags ---
#define ENABLE_PROCESSED_INPUT 0x0001
#define ENABLE_LINE_INPUT 0x0002
#define ENABLE_ECHO_INPUT 0x0004
#define ENABLE_WINDOW_INPUT 0x0008
#define ENABLE_MOUSE_INPUT 0x0010
#define ENABLE_INSERT_MODE 0x0020
#define ENABLE_QUICK_EDIT_MODE 0x0040
#define ENABLE_EXTENDED_FLAGS 0x0080
#define ENABLE_AUTO_POSITION 0x0100
#define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200

#define ENABLE_PROCESSED_OUTPUT 0x0001
#define ENABLE_WRAP_AT_EOL_OUTPUT 0x0002
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#define DISABLE_NEWLINE_AUTO_RETURN 0x0008
#define ENABLE_LVB_GRID_WORLDWIDE 0x0010

// --- CONSOLE_SCREEN_BUFFER_INFOEX ---
struct CONSOLE_SCREEN_BUFFER_INFOEX
{
    ULONG cbSize;
    COORD dwSize;
    COORD dwCursorPosition;
    WORD wAttributes;
    SMALL_RECT srWindow;
    COORD dwMaximumWindowSize;
    WORD wPopupAttributes;
    BOOL bFullscreenSupported;
    COLORREF ColorTable[16];
};

struct CONSOLE_CURSOR_INFO
{
    DWORD dwSize;
    BOOL bVisible;
};

// --- GUID ---
#ifndef GUID_DEFINED
#define GUID_DEFINED
struct GUID
{
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];

    bool operator==(const GUID& rhs) const noexcept
    {
        return memcmp(this, &rhs, sizeof(GUID)) == 0;
    }
    bool operator!=(const GUID& rhs) const noexcept
    {
        return !(*this == rhs);
    }
    bool operator<(const GUID& rhs) const noexcept
    {
        return memcmp(this, &rhs, sizeof(GUID)) < 0;
    }
};
#endif

// --- TRUE/FALSE ---
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// --- MAX_PATH ---
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// --- __declspec and other MSVC attributes ---
#define __declspec(x)
#define __stdcall
#define __cdecl
#define WINAPI
#define CALLBACK
#define APIENTRY
#define DECLSPEC_SELECTANY __attribute__((weak))

// --- MSVC-specific keywords ---
#define __forceinline __attribute__((always_inline)) inline
#define _TIL_INLINEPREFIX inline
#define __assume(x) do { if (!(x)) __builtin_unreachable(); } while(0)
#define sealed final

// --- MSVC pragmas (no-ops on GCC/Clang) ---
// float_control pragmas are MSVC-specific
#define TIL_FAST_MATH_BEGIN _Pragma("GCC optimize(\"fast-math\")")
#define TIL_FAST_MATH_END

// --- SAL annotations (no-ops on Linux) ---
#ifndef _In_
#define _In_
#endif
#ifndef _Out_
#define _Out_
#endif
#ifndef _Inout_
#define _Inout_
#endif
#ifndef _In_opt_
#define _In_opt_
#endif
#ifndef _Out_opt_
#define _Out_opt_
#endif
#ifndef _Inout_opt_
#define _Inout_opt_
#endif
#ifndef _In_reads_
#define _In_reads_(x)
#endif
#ifndef _Out_writes_
#define _Out_writes_(x)
#endif
#ifndef _In_reads_bytes_
#define _In_reads_bytes_(x)
#endif
#ifndef _Out_writes_bytes_
#define _Out_writes_bytes_(x)
#endif
#ifndef _In_z_
#define _In_z_
#endif
#ifndef _Ret_maybenull_
#define _Ret_maybenull_
#endif
#ifndef _Success_
#define _Success_(x)
#endif
#ifndef _Return_type_success_
#define _Return_type_success_(x)
#endif
#ifndef _Must_inspect_result_
#define _Must_inspect_result_
#endif
#ifndef _Check_return_
#define _Check_return_
#endif
#ifndef _Analysis_assume_
#define _Analysis_assume_(x)
#endif
#ifndef _Acquires_lock_
#define _Acquires_lock_(x)
#endif
#ifndef _Releases_lock_
#define _Releases_lock_(x)
#endif
#ifndef _Post_invalid_
#define _Post_invalid_
#endif
#ifndef _Pre_satisfies_
#define _Pre_satisfies_(x)
#endif
#ifndef _Post_satisfies_
#define _Post_satisfies_(x)
#endif
#ifndef _When_
#define _When_(x, y)
#endif
#ifndef _Outptr_result_nullonfailure_
#define _Outptr_result_nullonfailure_
#endif
#ifndef _Outptr_result_maybenull_
#define _Outptr_result_maybenull_
#endif
#ifndef _COM_Outptr_
#define _COM_Outptr_
#endif
#ifndef _Outptr_
#define _Outptr_
#endif
#ifndef _Field_size_
#define _Field_size_(x)
#endif

// --- DEFINE_ENUM_FLAG_OPERATORS ---
#ifndef DEFINE_ENUM_FLAG_OPERATORS
#define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) \
    inline constexpr ENUMTYPE operator|(ENUMTYPE a, ENUMTYPE b) noexcept { return static_cast<ENUMTYPE>(static_cast<std::underlying_type_t<ENUMTYPE>>(a) | static_cast<std::underlying_type_t<ENUMTYPE>>(b)); } \
    inline constexpr ENUMTYPE operator&(ENUMTYPE a, ENUMTYPE b) noexcept { return static_cast<ENUMTYPE>(static_cast<std::underlying_type_t<ENUMTYPE>>(a) & static_cast<std::underlying_type_t<ENUMTYPE>>(b)); } \
    inline constexpr ENUMTYPE operator~(ENUMTYPE a) noexcept { return static_cast<ENUMTYPE>(~static_cast<std::underlying_type_t<ENUMTYPE>>(a)); } \
    inline constexpr ENUMTYPE operator^(ENUMTYPE a, ENUMTYPE b) noexcept { return static_cast<ENUMTYPE>(static_cast<std::underlying_type_t<ENUMTYPE>>(a) ^ static_cast<std::underlying_type_t<ENUMTYPE>>(b)); } \
    inline constexpr ENUMTYPE& operator|=(ENUMTYPE& a, ENUMTYPE b) noexcept { a = a | b; return a; } \
    inline constexpr ENUMTYPE& operator&=(ENUMTYPE& a, ENUMTYPE b) noexcept { a = a & b; return a; } \
    inline constexpr ENUMTYPE& operator^=(ENUMTYPE& a, ENUMTYPE b) noexcept { a = a ^ b; return a; }
#endif

// --- __ImageBase / DLL related ---
#define EXTERN_C extern "C"

// --- InterlockedIncrement / Decrement ---
inline LONG InterlockedIncrement(volatile LONG* p) { return __sync_add_and_fetch(p, 1); }
inline LONG InterlockedDecrement(volatile LONG* p) { return __sync_sub_and_fetch(p, 1); }

// --- LOWORD / HIWORD ---
#define LOWORD(l) ((WORD)(((uintptr_t)(l)) & 0xffff))
#define HIWORD(l) ((WORD)((((uintptr_t)(l)) >> 16) & 0xffff))
#define LOBYTE(w) ((BYTE)(((WORD)(w)) & 0xff))
#define HIBYTE(w) ((BYTE)((((WORD)(w)) >> 8) & 0xff))
#define MAKEWORD(a, b) ((WORD)(((BYTE)(((DWORD)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD)(b)) & 0xff))) << 8))
#define MAKELONG(a, b) ((LONG)(((WORD)(((DWORD)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD)(b)) & 0xffff))) << 16))

// --- RGB macro ---
#define RGB(r, g, b) ((COLORREF)(((BYTE)(r)) | ((WORD)((BYTE)(g))) << 8 | ((DWORD)(BYTE)(b)) << 16))
#define GetRValue(rgb) (LOBYTE(rgb))
#define GetGValue(rgb) (LOBYTE(((WORD)(rgb)) >> 8))
#define GetBValue(rgb) (LOBYTE((rgb) >> 16))

// --- min/max ---
#ifndef NOMINMAX
#define NOMINMAX
#endif

// --- TERMINAL_STARTUP_INFO ---
struct TERMINAL_STARTUP_INFO
{
    LPCWSTR pszTitle;
    LPCWSTR pszIconPath;
    LONG iconIndex;
    DWORD dwFlags;
    WORD wShowWindow;
};

// --- intsafe.h replacement ---
#define ENABLE_INTSAFE_SIGNED_FUNCTIONS
inline HRESULT UIntAdd(UINT a, UINT b, UINT* result) { *result = a + b; return (*result < a) ? E_FAIL : S_OK; }
inline HRESULT UIntSub(UINT a, UINT b, UINT* result) { *result = a - b; return (a < b) ? E_FAIL : S_OK; }
inline HRESULT UIntMult(UINT a, UINT b, UINT* result) { uint64_t r = (uint64_t)a * b; *result = (UINT)r; return (r > UINT_MAX) ? E_FAIL : S_OK; }
inline HRESULT IntAdd(INT a, INT b, INT* result) { int64_t r = (int64_t)a + b; *result = (INT)r; return (r > INT_MAX || r < INT_MIN) ? E_FAIL : S_OK; }
inline HRESULT SizeTAdd(size_t a, size_t b, size_t* result) { *result = a + b; return (*result < a) ? E_FAIL : S_OK; }
inline HRESULT SizeTMult(size_t a, size_t b, size_t* result) { *result = a * b; return (b != 0 && *result / b != a) ? E_FAIL : S_OK; }
inline HRESULT ShortAdd(SHORT a, SHORT b, SHORT* result) { int r = a + b; *result = (SHORT)r; return (r > SHRT_MAX || r < SHRT_MIN) ? E_FAIL : S_OK; }
inline HRESULT ShortSub(SHORT a, SHORT b, SHORT* result) { int r = a - b; *result = (SHORT)r; return (r > SHRT_MAX || r < SHRT_MIN) ? E_FAIL : S_OK; }
inline HRESULT ShortMult(SHORT a, SHORT b, SHORT* result) { int r = a * b; *result = (SHORT)r; return (r > SHRT_MAX || r < SHRT_MIN) ? E_FAIL : S_OK; }
inline HRESULT SizeTToInt(size_t val, INT* result) { *result = (INT)val; return (val > (size_t)INT_MAX) ? E_FAIL : S_OK; }
inline HRESULT IntToSizeT(INT val, size_t* result) { *result = (size_t)val; return (val < 0) ? E_FAIL : S_OK; }
inline HRESULT SizeTToUInt(size_t val, UINT* result) { *result = (UINT)val; return (val > UINT_MAX) ? E_FAIL : S_OK; }
inline HRESULT SizeTToUShort(size_t val, USHORT* result) { *result = (USHORT)val; return (val > USHRT_MAX) ? E_FAIL : S_OK; }
inline HRESULT SizeTToShort(size_t val, SHORT* result) { *result = (SHORT)val; return (val > (size_t)SHRT_MAX) ? E_FAIL : S_OK; }
inline HRESULT UIntToSizeT(UINT val, size_t* result) { *result = (size_t)val; return S_OK; }
inline HRESULT UIntToUShort(UINT val, USHORT* result) { *result = (USHORT)val; return (val > USHRT_MAX) ? E_FAIL : S_OK; }

// --- D2D1_COLOR_F stub (used in renderer interfaces) ---
namespace D2D1
{
    struct ColorF
    {
        float r, g, b, a;
    };
}

struct D2D1_COLOR_F
{
    float r, g, b, a;
};

// --- WI_IsFlagSet / WI_SetFlag / etc. (from WIL) ---

// For enum types
template<typename T, typename U, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr bool WI_IsFlagSet(T val, U flag) noexcept
{
    using UT = std::underlying_type_t<T>;
    return (static_cast<UT>(val) & static_cast<UT>(flag)) == static_cast<UT>(flag);
}

template<typename T, typename U, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr bool WI_IsAnyFlagSet(T val, U flag) noexcept
{
    using UT = std::underlying_type_t<T>;
    return (static_cast<UT>(val) & static_cast<UT>(flag)) != 0;
}

template<typename T, typename U, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr void WI_SetFlag(T& val, U flag) noexcept
{
    using UT = std::underlying_type_t<T>;
    val = static_cast<T>(static_cast<UT>(val) | static_cast<UT>(flag));
}

template<typename T, typename U, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr void WI_ClearFlag(T& val, U flag) noexcept
{
    using UT = std::underlying_type_t<T>;
    val = static_cast<T>(static_cast<UT>(val) & ~static_cast<UT>(flag));
}

template<typename T, typename U, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr void WI_ToggleFlag(T& val, U flag) noexcept
{
    using UT = std::underlying_type_t<T>;
    val = static_cast<T>(static_cast<UT>(val) ^ static_cast<UT>(flag));
}

template<typename T, typename U, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr void WI_UpdateFlag(T& val, U flag, bool set) noexcept
{
    if (set) { WI_SetFlag(val, flag); } else { WI_ClearFlag(val, flag); }
}

// Overloads for plain integer types
inline constexpr bool WI_IsFlagSet(DWORD val, DWORD flag) noexcept { return (val & flag) == flag; }
inline constexpr bool WI_IsAnyFlagSet(DWORD val, DWORD flag) noexcept { return (val & flag) != 0; }
inline constexpr void WI_SetFlag(DWORD& val, DWORD flag) noexcept { val |= flag; }
inline constexpr void WI_ClearFlag(DWORD& val, DWORD flag) noexcept { val &= ~flag; }
inline constexpr void WI_UpdateFlag(DWORD& val, DWORD flag, bool set) noexcept { if (set) val |= flag; else val &= ~flag; }
inline constexpr bool WI_IsFlagClear(DWORD val, DWORD flag) noexcept { return (val & flag) == 0; }

// Additional overloads to handle type mismatches (int passed for DWORD, etc.)
inline constexpr bool WI_IsFlagSet(unsigned int val, int flag) noexcept { return (val & (unsigned int)flag) == (unsigned int)flag; }
inline constexpr bool WI_IsAnyFlagSet(unsigned int val, int flag) noexcept { return (val & (unsigned int)flag) != 0; }
inline constexpr bool WI_IsFlagSet(int val, int flag) noexcept { return (val & flag) == flag; }
inline constexpr bool WI_IsAnyFlagSet(int val, int flag) noexcept { return (val & flag) != 0; }
inline constexpr bool WI_IsFlagClear(int val, int flag) noexcept { return (val & flag) == 0; }
inline constexpr bool WI_IsFlagSet(short val, short flag) noexcept { return (val & flag) == flag; }
inline constexpr bool WI_IsAnyFlagSet(short val, short flag) noexcept { return (val & flag) != 0; }
inline constexpr bool WI_IsFlagClear(short val, short flag) noexcept { return (val & flag) == 0; }
inline constexpr bool WI_IsFlagSet(WORD val, WORD flag) noexcept { return (val & flag) == flag; }
inline constexpr void WI_SetFlag(WORD& val, WORD flag) noexcept { val |= flag; }
inline constexpr void WI_ClearFlag(WORD& val, WORD flag) noexcept { val &= ~flag; }

// WI_SetFlagIf
template<typename T, typename U>
inline constexpr void WI_SetFlagIf(T& val, U flag, bool condition) noexcept
{
    if (condition) { val |= static_cast<T>(flag); }
}

// --- Stub for OutputDebugStringW ---
inline void OutputDebugStringW(const WCHAR*) {}
inline void OutputDebugStringA(const char*) {}

// --- GetDoubleClickTime ---
inline UINT GetDoubleClickTime() { return 500; }

// --- MapVirtualKey ---
#define MAPVK_VK_TO_VSC 0
#define MAPVK_VSC_TO_VK 1
#define MAPVK_VK_TO_CHAR 2
#define MAPVK_VSC_TO_VK_EX 3
inline UINT MapVirtualKeyW(UINT code, UINT mapType) { return code; }

// --- WaitOnAddress / WakeByAddressAll (sync primitives) ---
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

inline BOOL WaitOnAddress(volatile void* addr, void* compare, SIZE_T size, DWORD timeout)
{
    // Simple futex-based implementation
    syscall(SYS_futex, addr, FUTEX_WAIT, *static_cast<int*>(compare), nullptr, nullptr, 0);
    return TRUE;
}

inline void WakeByAddressAll(void* addr)
{
    syscall(SYS_futex, addr, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0);
}

inline void WakeByAddressSingle(void* addr)
{
    syscall(SYS_futex, addr, FUTEX_WAKE, 1, nullptr, nullptr, 0);
}

// --- GetLastError / SetLastError stubs ---
inline DWORD GetLastError() { return (DWORD)errno; }
inline void SetLastError(DWORD) {}

// --- E_NOT_VALID_STATE ---
#define E_NOT_VALID_STATE ((HRESULT)0x8007139F)

// --- Win32 error codes ---
#define ERROR_INVALID_DATA 13
#define ERROR_INVALID_PARAMETER 87
#define ERROR_SUCCESS 0
#define ERROR_MORE_DATA 234
#define ERROR_PIPE_NOT_CONNECTED 233
#define ERROR_BROKEN_PIPE 109

// --- OVERLAPPED ---
struct OVERLAPPED
{
    ULONG Internal;
    ULONG InternalHigh;
    union {
        struct { DWORD Offset; DWORD OffsetHigh; };
        void* Pointer;
    };
    HANDLE hEvent;
};

// --- INFINITE ---
#define INFINITE 0xFFFFFFFF

// --- _Null_terminated_ SAL annotation ---
#ifndef _Null_terminated_
#define _Null_terminated_
#endif

// --- String conversion functions ---
#define CP_UTF8 65001
#define CP_ACP 0

#define LOCALE_NAME_USER_DEFAULT nullptr
#define LINGUISTIC_IGNORECASE 0x00000010
#define CSTR_LESS_THAN 1
#define CSTR_EQUAL 2
#define CSTR_GREATER_THAN 3
#define NORM_IGNORECASE 0x00000001
#define FIND_FROMSTART 0x00400000

// MB_ERR_INVALID_CHARS flag
#define MB_ERR_INVALID_CHARS 0x00000008

#include <codecvt>
#include <locale>

inline int MultiByteToWideChar(UINT codePage, DWORD flags, const char* src, int srcLen,
                                WCHAR* dst, int dstLen)
{
    if (srcLen < 0)
    {
        srcLen = static_cast<int>(strlen(src)) + 1;
    }

    std::mbstate_t state{};
    const char* srcEnd = src + srcLen;
    // Count needed chars
    if (dstLen == 0)
    {
        int count = 0;
        const char* p = src;
        while (p < srcEnd)
        {
            wchar_t wc;
            size_t rc = mbrtowc(&wc, p, srcEnd - p, &state);
            if (rc == 0) { count++; break; }
            if (rc == (size_t)-1 || rc == (size_t)-2) { count++; p++; }
            else { count++; p += rc; }
        }
        return count;
    }

    int count = 0;
    const char* p = src;
    while (p < srcEnd && count < dstLen)
    {
        wchar_t wc;
        size_t rc = mbrtowc(&wc, p, srcEnd - p, &state);
        if (rc == 0) { dst[count++] = L'\0'; break; }
        if (rc == (size_t)-1 || rc == (size_t)-2) { dst[count++] = L'?'; p++; }
        else { dst[count++] = wc; p += rc; }
    }
    return count;
}

inline int WideCharToMultiByte(UINT codePage, DWORD flags, const WCHAR* src, int srcLen,
                                char* dst, int dstLen, const char*, BOOL*)
{
    if (srcLen < 0)
    {
        const WCHAR* p = src;
        while (*p) p++;
        srcLen = static_cast<int>(p - src) + 1;
    }

    std::mbstate_t state{};
    // Count needed bytes
    if (dstLen == 0)
    {
        int count = 0;
        for (int i = 0; i < srcLen; i++)
        {
            char mb[MB_LEN_MAX];
            size_t rc = wcrtomb(mb, src[i], &state);
            if (rc == (size_t)-1) count++;
            else count += static_cast<int>(rc);
        }
        return count;
    }

    int count = 0;
    for (int i = 0; i < srcLen && count < dstLen; i++)
    {
        char mb[MB_LEN_MAX];
        size_t rc = wcrtomb(mb, src[i], &state);
        if (rc == (size_t)-1) { if (count < dstLen) dst[count++] = '?'; }
        else
        {
            for (size_t j = 0; j < rc && count < dstLen; j++)
            {
                dst[count++] = mb[j];
            }
        }
    }
    return count;
}

inline int CompareStringOrdinal(const WCHAR* s1, int len1, const WCHAR* s2, int len2, BOOL ignoreCase)
{
    // Simple ordinal comparison
    if (len1 < 0)
    {
        const WCHAR* p = s1;
        while (*p) p++;
        len1 = static_cast<int>(p - s1);
    }
    if (len2 < 0)
    {
        const WCHAR* p = s2;
        while (*p) p++;
        len2 = static_cast<int>(p - s2);
    }
    int minLen = std::min(len1, len2);
    for (int i = 0; i < minLen; i++)
    {
        WCHAR c1 = s1[i];
        WCHAR c2 = s2[i];
        if (ignoreCase)
        {
            c1 = (c1 >= 'A' && c1 <= 'Z') ? c1 + 32 : c1;
            c2 = (c2 >= 'A' && c2 <= 'Z') ? c2 + 32 : c2;
        }
        if (c1 < c2) return CSTR_LESS_THAN;
        if (c1 > c2) return CSTR_GREATER_THAN;
    }
    if (len1 < len2) return CSTR_LESS_THAN;
    if (len1 > len2) return CSTR_GREATER_THAN;
    return CSTR_EQUAL;
}

inline int CompareStringEx(const WCHAR*, DWORD flags, const WCHAR* s1, int len1,
                           const WCHAR* s2, int len2, void*, void*, LPARAM)
{
    return CompareStringOrdinal(s1, len1, s2, len2, (flags & NORM_IGNORECASE) != 0);
}

inline int FindNLSStringEx(const WCHAR*, DWORD, const WCHAR* src, int srcLen,
                           const WCHAR* value, int valueLen, int* found,
                           void*, void*, LPARAM)
{
    if (srcLen < 0)
    {
        const WCHAR* p = src;
        while (*p) p++;
        srcLen = static_cast<int>(p - src);
    }
    if (valueLen < 0)
    {
        const WCHAR* p = value;
        while (*p) p++;
        valueLen = static_cast<int>(p - value);
    }
    for (int i = 0; i <= srcLen - valueLen; i++)
    {
        bool match = true;
        for (int j = 0; j < valueLen; j++)
        {
            WCHAR c1 = src[i + j];
            WCHAR c2 = value[j];
            c1 = (c1 >= 'A' && c1 <= 'Z') ? c1 + 32 : c1;
            c2 = (c2 >= 'A' && c2 <= 'Z') ? c2 + 32 : c2;
            if (c1 != c2) { match = false; break; }
        }
        if (match)
        {
            if (found) *found = valueLen;
            return i;
        }
    }
    return -1;
}

// --- _ITERATOR_DEBUG_LEVEL ---
#ifndef _ITERATOR_DEBUG_LEVEL
#define _ITERATOR_DEBUG_LEVEL 0
#endif

#endif // __linux__
