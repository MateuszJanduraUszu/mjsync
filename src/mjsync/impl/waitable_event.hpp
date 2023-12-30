// waitable_event.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_IMPL_WAITABLE_EVENT_HPP_
#define _MJSYNC_IMPL_WAITABLE_EVENT_HPP_
#include <mjsync/impl/tinywin.hpp>

namespace mjx {
    namespace mjsync_impl {
        inline void* _Create_anonymous_waitable_event() noexcept {
            return ::CreateEventW(nullptr, true, false, nullptr);
        }

        inline void* _Create_or_open_named_waitable_event(const wchar_t* const _Name) noexcept {
            void* _Handle = ::CreateEventW(nullptr, true, false, _Name);
            if (_Handle) { // created a new named event
                return _Handle;
            } else if (::GetLastError() == ERROR_ALREADY_EXISTS) { // already exists, try to open it
                return ::OpenEventW(EVENT_MODIFY_STATE, true, _Name);
            } else { // neither exists nor was created
                return nullptr;
            }
        }
    } // namespace mjsync_impl
} // namespace mjx

#endif // _MJSYNC_IMPL_WAITABLE_EVENT_HPP_