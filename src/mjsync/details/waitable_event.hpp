// waitable_event.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_DETAILS_WAITABLE_EVENT_HPP_
#define _MJSYNC_DETAILS_WAITABLE_EVENT_HPP_
#include <mjsync/details/tinywin.hpp>

namespace mjsync {
    namespace details {
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
    } // namespace details
} // namespace mjsync

#endif // _MJSYNC_DETAILS_WAITABLE_EVENT_HPP_