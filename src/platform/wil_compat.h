// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
// Linux compatibility shim for Windows Implementation Library (WIL).

#pragma once

#ifdef __linux__

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <cstdio>
#include <cstdarg>
#include <type_traits>

#include "windows_compat.h"

// --- Exception types ---
namespace wil
{
    struct ResultException : std::runtime_error
    {
        HRESULT hr;
        explicit ResultException(HRESULT code) : std::runtime_error("HRESULT error"), hr(code) {}
    };

    // --- RAII handle wrappers ---
    struct handle_closer
    {
        void operator()(HANDLE) const noexcept { /* no-op on Linux, real handles use fd_closer */ }
    };

    using unique_handle = std::unique_ptr<void, handle_closer>;

    struct fd_closer
    {
        void operator()(int* fd) const noexcept;
    };

    // --- unique_event ---
    struct unique_event
    {
        void create(unsigned /*type*/ = 0) {}
        void SetEvent() {}
        void ResetEvent() {}
        HANDLE get() const { return nullptr; }
        explicit operator bool() const { return true; }
    };

    // --- unique_hfile (file handle) ---
    struct unique_hfile
    {
        int fd = -1;
        unique_hfile() = default;
        explicit unique_hfile(int f) : fd(f) {}
        unique_hfile(unique_hfile&& o) noexcept : fd(o.fd) { o.fd = -1; }
        unique_hfile& operator=(unique_hfile&& o) noexcept { if (this != &o) { reset(); fd = o.fd; o.fd = -1; } return *this; }
        ~unique_hfile() { reset(); }
        void reset(int f = -1);
        int get() const { return fd; }
        int release() { int f = fd; fd = -1; return f; }
        explicit operator bool() const { return fd >= 0; }
    };

    // --- unique_process_information ---
    struct unique_process_information
    {
        HANDLE hProcess = nullptr;
        HANDLE hThread = nullptr;
        DWORD dwProcessId = 0;
        DWORD dwThreadId = 0;
    };

    // --- str_printf ---
    template<typename T = std::wstring>
    T str_printf(const wchar_t* fmt, ...)
    {
        // Stub: return empty string
        return T{};
    }

    // --- zwstring_view ---
    using zwstring_view = std::wstring_view;
    using zstring_view = std::string_view;

    // --- ResultFromCaughtException ---
    inline HRESULT ResultFromCaughtException() noexcept
    {
        try { throw; }
        catch (const ResultException& e) { return e.hr; }
        catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
        catch (...) { return E_FAIL; }
    }

    // --- scope_exit ---
    template<typename F>
    struct scope_exit_t
    {
        F func;
        scope_exit_t(F f) : func(std::move(f)) {}
        ~scope_exit_t() { func(); }
    };

    template<typename F>
    scope_exit_t<F> scope_exit(F&& f) { return scope_exit_t<F>(std::forward<F>(f)); }

    // --- hide_name ---
    struct hide_name {};

    // --- srwlock ---
    struct srwlock
    {
        void lock() {}
        void unlock() {}
        void lock_shared() {}
        void unlock_shared() {}
    };

    // --- com_ptr stub ---
    template<typename T>
    struct com_ptr
    {
        T* ptr = nullptr;
        com_ptr() = default;
        explicit com_ptr(T* p) : ptr(p) {}
        T* get() const { return ptr; }
        T** addressof() { return &ptr; }
        T* operator->() const { return ptr; }
        T& operator*() const { return *ptr; }
        explicit operator bool() const { return ptr != nullptr; }
        void reset() { ptr = nullptr; }
    };

    // --- unique_any template ---
    template<typename T, typename CloseFunc, CloseFunc closeFunc>
    struct unique_any
    {
        T value{};
        unique_any() = default;
        explicit unique_any(T v) : value(v) {}
        unique_any(unique_any&& o) noexcept : value(o.value) { o.value = T{}; }
        unique_any& operator=(unique_any&& o) noexcept { if (this != &o) { reset(); value = o.value; o.value = T{}; } return *this; }
        ~unique_any() { reset(); }
        void reset() { if (value != T{}) { closeFunc(value); value = T{}; } }
        T get() const { return value; }
        explicit operator bool() const { return value != T{}; }
    };

} // namespace wil

// --- WIL-style error handling macros ---
#define THROW_IF_FAILED(hr) do { HRESULT _hr = (hr); if (FAILED(_hr)) { throw wil::ResultException(_hr); } } while(0)
#define RETURN_IF_FAILED(hr) do { HRESULT _hr = (hr); if (FAILED(_hr)) { return _hr; } } while(0)
#define RETURN_HR(hr) return (hr)
#define RETURN_HR_IF(hr, cond) do { if (cond) { return (hr); } } while(0)
#define RETURN_HR_IF_NULL(hr, ptr) do { if ((ptr) == nullptr) { return (hr); } } while(0)
#define LOG_IF_FAILED(hr) do { (void)(hr); } while(0)
#define LOG_HR(hr) do { (void)(hr); } while(0)
#define LOG_HR_IF(hr, cond) do { (void)(hr); (void)(cond); } while(0)
#define LOG_CAUGHT_EXCEPTION() do {} while(0)
#define LOG_IF_WIN32_BOOL_FALSE(expr) do { (void)(expr); } while(0)
#define THROW_HR(hr) throw wil::ResultException(hr)
#define THROW_LAST_ERROR() throw wil::ResultException(E_FAIL)
#define THROW_IF_WIN32_BOOL_FALSE(expr) do { if (!(expr)) { throw wil::ResultException(E_FAIL); } } while(0)
#define THROW_IF_NULL_ALLOC(expr) do { if ((expr) == nullptr) { throw std::bad_alloc(); } } while(0)
#define THROW_WIN32(err) throw wil::ResultException(HRESULT_FROM_WIN32(err))
#define THROW_IF_NTSTATUS_FAILED(status) do { if (!NT_SUCCESS(status)) { throw wil::ResultException((HRESULT)(status)); } } while(0)
#define THROW_ERRNO_IF(cond, val) do { if (cond) { throw wil::ResultException(E_FAIL); } } while(0)
#define THROW_HR_IF(hr, cond) do { if (cond) { throw wil::ResultException(hr); } } while(0)
#define THROW_LAST_ERROR_IF(cond) do { if (cond) { throw wil::ResultException(E_FAIL); } } while(0)
#define THROW_LAST_ERROR_IF_NULL(ptr) do { if ((ptr) == nullptr) { throw wil::ResultException(E_FAIL); } } while(0)
#define THROW_HR_IF_NULL(hr, ptr) do { if ((ptr) == nullptr) { throw wil::ResultException(hr); } } while(0)
#define RETURN_IF_WIN32_BOOL_FALSE(expr) do { if (!(expr)) { return E_FAIL; } } while(0)
#define RETURN_LAST_ERROR() return E_FAIL
#define RETURN_NTSTATUS(status) return (HRESULT)(status)

#define CATCH_LOG() catch (...) {}
#define CATCH_RETURN() catch (...) { return E_FAIL; }

// --- FAIL_FAST macros ---
#define FAIL_FAST_IF(cond) do { if (cond) { std::abort(); } } while(0)
#define FAIL_FAST_IF_NULL(ptr) do { if ((ptr) == nullptr) { std::abort(); } } while(0)
#define FAIL_FAST() std::abort()
#define FAIL_FAST_HR(hr) std::abort()

// --- CATCH_FAIL_FAST ---
#define CATCH_FAIL_FAST() catch (...) { std::abort(); }

// --- SUCCEEDED_LOG, SUCCEEDED_WIN32_LOG ---
#define SUCCEEDED_LOG(hr) SUCCEEDED(hr)
#define SUCCEEDED_WIN32_LOG(hr) SUCCEEDED(hr)

// --- TraceLogging stubs ---
#define TraceLoggingRegister(...)
#define TraceLoggingUnregister(...)
#define TraceLoggingWrite(...)
#define TraceLoggingDescription(...)
#define TraceLoggingKeyword(...)
#define TelemetryPrivacyDataTag(...)
#define TRACELOGGING_DEFINE_PROVIDER(...)
#define PDT_ProductAndServiceUsage 0
#define MICROSOFT_KEYWORD_MEASURES 0

// --- CppCoreCheck warnings stub ---
namespace gsl
{
    // gsl is provided by ms-gsl, just ensure narrowing_error is available
}

// --- WI_AreAllFlagsClear etc ---
template<typename T>
constexpr bool WI_AreAllFlagsClear(T val, T flags) noexcept
{
    return (static_cast<std::underlying_type_t<T>>(val) & static_cast<std::underlying_type_t<T>>(flags)) == 0;
}

inline constexpr bool WI_AreAllFlagsClear(DWORD val, DWORD flags) noexcept { return (val & flags) == 0; }

template<typename T, typename U, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr bool WI_IsFlagClear(T val, U flag) noexcept
{
    using UT = std::underlying_type_t<T>;
    return (static_cast<UT>(val) & static_cast<UT>(flag)) == 0;
}

template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr bool WI_AreAllFlagsClear(T val, T flags) noexcept
{
    using UT = std::underlying_type_t<T>;
    return (static_cast<UT>(val) & static_cast<UT>(flags)) == 0;
}

#endif // __linux__
