// waitable_event.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_WAITABLE_EVENT_HPP_
#define _MJSYNC_WAITABLE_EVENT_HPP_
#include <cstdint>
#include <mjsync/api.hpp>

namespace mjx {
    struct uninitialized_event_t { // tag for waitable_event's constructor
        uninitialized_event_t() noexcept = default;
    };

    inline constexpr uninitialized_event_t uninitialized_event{};

    class _MJSYNC_API waitable_event { // waiting-based event
    public:
        using native_handle_type = void*;

        static constexpr uint32_t infinite_timeout = 0xFFFF'FFFF;

        waitable_event() noexcept;
        waitable_event(waitable_event&& _Other) noexcept;
        ~waitable_event() noexcept;

        explicit waitable_event(const wchar_t* const _Name) noexcept;
        explicit waitable_event(uninitialized_event_t) noexcept;

        waitable_event& operator=(waitable_event&& _Other) noexcept;

        waitable_event(const waitable_event&)            = delete;
        waitable_event& operator=(const waitable_event&) = delete;

        // checks if the event is valid
        bool valid() const noexcept;

        // returns the event handle
        const native_handle_type native_handle() const noexcept;

        // waits for the event
        void wait(const uint32_t _Timeout = infinite_timeout) noexcept;
        
        // waits for the event, then resets it
        void wait_and_reset(const uint32_t _Timeout = infinite_timeout) noexcept;

        // notifies a thread that waits for the event
        void notify() noexcept;

        // resets the event
        void reset() noexcept;

    private:
        native_handle_type _Myhandle;
    };
} // namespace mjx

#endif // _MJSYNC_WAITABLE_EVENT_HPP_