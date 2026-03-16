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
#include <mutex>
#include <condition_variable>
#include <chrono>

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
        bool active = true;
        scope_exit_t(F f) : func(std::move(f)) {}
        ~scope_exit_t() { if (active) func(); }
        void reset() { active = false; }
        void release() { active = false; }
    };

    template<typename F>
    scope_exit_t<F> scope_exit(F&& f) { return scope_exit_t<F>(std::forward<F>(f)); }

    // --- function_deleter: for use with std::unique_ptr ---
    template<typename FuncType, FuncType fn>
    struct function_deleter
    {
        template<typename T>
        void operator()(T* p) const noexcept { fn(p); }
    };

    // --- slim_event: lightweight event using mutex + condvar ---
    class slim_event
    {
    public:
        slim_event(bool initialState = false) : _signaled(initialState) {}
        void SetEvent() { { std::lock_guard lk(_mtx); _signaled = true; } _cv.notify_all(); }
        void ResetEvent() { std::lock_guard lk(_mtx); _signaled = false; }
        bool wait(uint32_t timeoutMs = UINT32_MAX)
        {
            std::unique_lock lk(_mtx);
            if (timeoutMs == UINT32_MAX) { _cv.wait(lk, [&]{ return _signaled; }); return true; }
            return _cv.wait_for(lk, std::chrono::milliseconds(timeoutMs), [&]{ return _signaled; });
        }
    private:
        std::mutex _mtx;
        std::condition_variable _cv;
        bool _signaled;
    };

    class slim_event_manual_reset : public slim_event
    {
    public:
        slim_event_manual_reset(bool initialState = false) : slim_event(initialState) {}
    };

    class slim_event_auto_reset : public slim_event
    {
    public:
        slim_event_auto_reset(bool initialState = false) : slim_event(initialState) {}
    };

    // --- unique_struct: RAII wrapper for C structs with a close function ---
    template<typename T, typename CloseFunc, CloseFunc closeFn>
    struct unique_struct : public T
    {
        unique_struct() : T{} {}
        ~unique_struct() { closeFn(static_cast<T*>(this)); }
        unique_struct(const unique_struct&) = delete;
        unique_struct& operator=(const unique_struct&) = delete;
        unique_struct(unique_struct&& o) noexcept : T(o) { static_cast<T&>(o) = T{}; }
        unique_struct& operator=(unique_struct&& o) noexcept
        {
            if (this != &o) { closeFn(static_cast<T*>(this)); static_cast<T&>(*this) = static_cast<T&>(o); static_cast<T&>(o) = T{}; }
            return *this;
        }
    };

    // --- unique_virtualalloc_ptr (use mmap/munmap on Linux) ---
    struct virtualalloc_deleter
    {
        size_t _size = 0;
        void operator()(void* p) const noexcept
        {
            if (p) { munmap(p, _size); }
        }
    };

    template<typename T>
    struct unique_virtualalloc_ptr_t
    {
        using pointer = T*;

        unique_virtualalloc_ptr_t() noexcept = default;
        explicit unique_virtualalloc_ptr_t(T* p) noexcept : _ptr(p) {}
        unique_virtualalloc_ptr_t(unique_virtualalloc_ptr_t&& o) noexcept : _ptr(o._ptr) { o._ptr = nullptr; }
        unique_virtualalloc_ptr_t& operator=(unique_virtualalloc_ptr_t&& o) noexcept { reset(); _ptr = o._ptr; o._ptr = nullptr; return *this; }
        ~unique_virtualalloc_ptr_t() { reset(); }
        unique_virtualalloc_ptr_t(const unique_virtualalloc_ptr_t&) = delete;
        unique_virtualalloc_ptr_t& operator=(const unique_virtualalloc_ptr_t&) = delete;

        T* get() const noexcept { return _ptr; }
        explicit operator bool() const noexcept { return _ptr != nullptr; }
        void reset(T* p = nullptr) noexcept
        {
            if (_ptr)
            {
                // We can't know size for munmap, so we used mmap with MAP_ANONYMOUS
                // and rely on the kernel to track the mapping. Use a large sentinel.
                free(_ptr);
            }
            _ptr = p;
        }
    private:
        T* _ptr = nullptr;
    };

    template<typename T>
    using unique_virtualalloc_ptr = unique_virtualalloc_ptr_t<T>;

    // --- hide_name ---
    struct hide_name {};

    // --- srwlock ---
    struct srwlock
    {
        std::mutex _mtx;

        struct lock_guard_t
        {
            std::mutex& _m;
            lock_guard_t(std::mutex& m) : _m(m) { _m.lock(); }
            ~lock_guard_t() { _m.unlock(); }
        };

        lock_guard_t lock_exclusive() { return lock_guard_t(_mtx); }
        lock_guard_t lock_shared() { return lock_guard_t(_mtx); }
        void lock() { _mtx.lock(); }
        void unlock() { _mtx.unlock(); }
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
#define LOG_IF_FAILED(hr) [&]() { HRESULT _hr = (hr); return _hr; }()
#define LOG_HR(hr) [&]() { HRESULT _hr = (hr); return _hr; }()
#define LOG_HR_IF(hr, cond) do { (void)(hr); (void)(cond); } while(0)
#define LOG_CAUGHT_EXCEPTION() do {} while(0)
#define LOG_IF_WIN32_BOOL_FALSE(expr) do { (void)(expr); } while(0)
#define THROW_HR(hr) throw wil::ResultException(hr)
#define THROW_LAST_ERROR() throw wil::ResultException(E_FAIL)
#define THROW_IF_WIN32_BOOL_FALSE(expr) do { if (!(expr)) { throw wil::ResultException(E_FAIL); } } while(0)
#define THROW_IF_NULL_ALLOC(expr) do { if ((expr) == nullptr) { throw std::bad_alloc(); } } while(0)
#define THROW_WIN32(err) throw wil::ResultException(HRESULT_FROM_WIN32(err))
#define THROW_WIN32_IF_MSG(err, cond, ...) do { if (cond) { throw wil::ResultException(HRESULT_FROM_WIN32(err)); } } while(0)
#define THROW_IF_NTSTATUS_FAILED(status) do { if (!NT_SUCCESS(status)) { throw wil::ResultException((HRESULT)(status)); } } while(0)
#define THROW_ERRNO_IF(cond, val) do { if (cond) { throw wil::ResultException(E_FAIL); } } while(0)
#define THROW_HR_IF(hr, cond) do { if (cond) { throw wil::ResultException(hr); } } while(0)
#define THROW_LAST_ERROR_IF(cond) do { if (cond) { throw wil::ResultException(E_FAIL); } } while(0)
#define THROW_LAST_ERROR_IF_NULL(ptr) [&]() { auto _p = (ptr); if (_p == nullptr) { throw wil::ResultException(E_FAIL); } return _p; }()
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

// --- wistd: WIL's std-compatible namespace (just alias to std) ---
namespace wistd = std;

// WI_ flag functions are defined in windows_compat.h

#endif // __linux__
