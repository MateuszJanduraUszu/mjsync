// sync_flag.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_SYNC_FLAG_HPP_
#define _MJSYNC_SYNC_FLAG_HPP_
#include <atomic>
#include <mjsync/api.hpp>

namespace mjx {
    class _MJSYNC_API sync_flag { // atomic flag for thread synchronization
    public:
        sync_flag(const bool _Initial_value = false) noexcept;
        ~sync_flag() noexcept;

        sync_flag(const sync_flag&)            = delete;
        sync_flag& operator=(const sync_flag&) = delete;

        // checks if the flag is set
        bool is_set() const noexcept;
        bool is_set(const ::std::memory_order _Order) const noexcept;

        // clears the flag
        void clear() noexcept;
        void clear(const ::std::memory_order _Order) noexcept;

        // sets the flag
        void set() noexcept;
        void set(const ::std::memory_order _Order) noexcept;

    private:
#pragma warning(suppress : 4251) // C4251: std::atomic needs to have dll-interface
        ::std::atomic<bool> _Myval;
    };
} // namespace mjx

#endif // _MJSYNC_SYNC_FLAG_HPP_