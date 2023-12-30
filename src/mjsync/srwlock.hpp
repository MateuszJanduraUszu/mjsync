// srwlock.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_SRWLOCK_HPP_
#define _MJSYNC_SRWLOCK_HPP_
#include <mjsync/api.hpp>

namespace mjx {
    class _MJSYNC_API shared_lock { // slim reader/writer lock
    public:
        shared_lock() noexcept;
        ~shared_lock() noexcept;

        shared_lock(const shared_lock&)            = delete;
        shared_lock& operator=(const shared_lock&) = delete;

        // acquires exclusive lock
        void lock() noexcept;

        // acquires shared lock
        void lock_shared() noexcept;

        // releases exclusive lock
        void unlock() noexcept;

        // releases shared lock
        void unlock_shared() noexcept;

    private:
        struct _Impl { // copy of SRWLOCK structure
            void* _Ptr;
        };
        
        _Impl _Myimpl;
    };

    class _MJSYNC_API lock_guard { // automatically acquires and releases exclusive lock
    public:
        explicit lock_guard(shared_lock& _Lock) noexcept;
        ~lock_guard() noexcept;

    private:
        shared_lock& _Mylock;
    };

    class _MJSYNC_API shared_lock_guard { // automatically acquires and releases shared lock
    public:
        explicit shared_lock_guard(shared_lock& _Lock) noexcept;
        ~shared_lock_guard() noexcept;
        
    private:
        shared_lock& _Mylock;
    };
} // namespace mjx

#endif // _MJSYNC_SRWLOCK_HPP_