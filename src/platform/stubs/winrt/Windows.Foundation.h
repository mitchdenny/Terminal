// Linux stub for winrt/Windows.Foundation.h
#pragma once

#define WINRT_Windows_Foundation_H

#include <string>
#include <cstdint>
#include <functional>
#include <vector>
#include <mutex>

namespace winrt
{
    // hstring — basic wide string wrapper
    struct hstring
    {
        std::wstring _str;
        hstring() = default;
        hstring(const wchar_t* s) : _str(s ? s : L"") {}
        hstring(const wchar_t* s, uint32_t len) : _str(s, len) {}
        hstring(const std::wstring& s) : _str(s) {}
        hstring(std::wstring_view sv) : _str(sv) {}

        const wchar_t* c_str() const noexcept { return _str.c_str(); }
        const wchar_t* data() const noexcept { return _str.data(); }
        uint32_t size() const noexcept { return static_cast<uint32_t>(_str.size()); }
        bool empty() const noexcept { return _str.empty(); }
        explicit operator bool() const noexcept { return !_str.empty(); }
        operator std::wstring_view() const noexcept { return _str; }
        auto begin() const noexcept { return _str.begin(); }
        auto end() const noexcept { return _str.end(); }
        bool operator==(const hstring& o) const { return _str == o._str; }
        bool operator!=(const hstring& o) const { return _str != o._str; }
        bool operator<(const hstring& o) const { return _str < o._str; }
    };

    // guid
    struct guid
    {
        uint32_t Data1;
        uint16_t Data2;
        uint16_t Data3;
        uint8_t Data4[8];
        bool operator==(const guid&) const = default;
    };

    // event_token
    struct event_token
    {
        int64_t value = 0;
    };

    // event<T> — simple event system
    template<typename Delegate>
    struct event
    {
        event_token add(const Delegate& handler)
        {
            std::lock_guard lk(_mtx);
            auto token = event_token{_next++};
            _handlers.push_back({token.value, handler});
            return token;
        }
        void remove(event_token token)
        {
            std::lock_guard lk(_mtx);
            std::erase_if(_handlers, [&](auto& p) { return p.first == token.value; });
        }
        template<typename... Args>
        void operator()(Args&&... args) const
        {
            std::lock_guard lk(_mtx);
            for (auto& [_, handler] : _handlers)
            {
                handler(std::forward<Args>(args)...);
            }
        }
    private:
        mutable std::mutex _mtx;
        int64_t _next = 1;
        std::vector<std::pair<int64_t, Delegate>> _handlers;
    };

    // com_array
    template<typename T>
    struct com_array : public std::vector<T>
    {
        using std::vector<T>::vector;
    };

    // IReference
    template<typename T>
    struct Windows_Foundation_IReference
    {
        T _val{};
        bool _has = false;
        Windows_Foundation_IReference() = default;
        Windows_Foundation_IReference(T v) : _val(v), _has(true) {}
        Windows_Foundation_IReference(std::nullptr_t) : _has(false) {}
        explicit operator bool() const { return _has; }
        T Value() const { return _val; }
        bool operator==(std::nullptr_t) const { return !_has; }
        bool operator!=(std::nullptr_t) const { return _has; }
    };
}

namespace winrt::Windows::Foundation
{
    struct IInspectable {};

    template<typename T>
    using IReference = winrt::Windows_Foundation_IReference<T>;

    template<typename TSender, typename TArgs>
    using TypedEventHandler = std::function<void(TSender, TArgs)>;
}

namespace winrt::Windows::UI::Xaml::Data
{
    using PropertyChangedEventHandler = std::function<void()>;
}
