// waitable_event.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjsync/details/waitable_event.hpp>
#include <mjsync/waitable_event.hpp>
#include <type_traits>

namespace mjsync {
    waitable_event::waitable_event() noexcept : _Myhandle(details::_Create_anonymous_waitable_event()) {}

    waitable_event::waitable_event(waitable_event&& _Other) noexcept : _Myhandle(_Other._Myhandle) {
        _Other._Myhandle = nullptr;
    }

    waitable_event::waitable_event(const wchar_t* const _Name) noexcept
        : _Myhandle(details::_Create_or_open_named_waitable_event(_Name)) {}

    waitable_event::waitable_event(uninitialized_event_t) noexcept : _Myhandle(nullptr) {}

    waitable_event::~waitable_event() noexcept {
        if (_Myhandle) {
            ::CloseHandle(_Myhandle);
            _Myhandle = nullptr;
        }
    }

    waitable_event& waitable_event::operator=(waitable_event&& _Other) noexcept {
        if (this != ::std::addressof(_Other)) {
            _Myhandle        = _Other._Myhandle;
            _Other._Myhandle = nullptr;
        }

        return *this;
    }

    bool waitable_event::valid() const noexcept {
        return _Myhandle != nullptr;
    }

    const waitable_event::native_handle_type waitable_event::native_handle() const noexcept {
        return _Myhandle;
    }

    void waitable_event::wait(const uint32_t _Timeout) noexcept {
        if (_Myhandle) {
            ::WaitForSingleObject(_Myhandle, _Timeout);
        }
    }

    void waitable_event::wait_and_reset(const uint32_t _Timeout) noexcept {
        if (_Myhandle) {
            if (::WaitForSingleObject(_Myhandle, _Timeout) == WAIT_OBJECT_0) {
                ::ResetEvent(_Myhandle);
            }
        }
    }

    void waitable_event::notify() noexcept {
        if (_Myhandle) {
            ::SetEvent(_Myhandle);
        }
    }

    void waitable_event::reset() noexcept {
        if (_Myhandle) {
            ::ResetEvent(_Myhandle);
        }
    }
} // namespace mjsync