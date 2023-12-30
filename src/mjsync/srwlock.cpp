// srwlock.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjsync/impl/tinywin.hpp>
#include <mjsync/srwlock.hpp>

namespace mjx {
    shared_lock::shared_lock() noexcept : _Myimpl{0} {}

    shared_lock::~shared_lock() noexcept {}

    void shared_lock::lock() noexcept {
        ::AcquireSRWLockExclusive(reinterpret_cast<SRWLOCK*>(&_Myimpl));
    }

    void shared_lock::lock_shared() noexcept {
        ::AcquireSRWLockShared(reinterpret_cast<SRWLOCK*>(&_Myimpl));
    }

    void shared_lock::unlock() noexcept {
#pragma warning(suppress : 26110) // C26110: lock must be held
        ::ReleaseSRWLockExclusive(reinterpret_cast<SRWLOCK*>(&_Myimpl));
    }

    void shared_lock::unlock_shared() noexcept {
        ::ReleaseSRWLockShared(reinterpret_cast<SRWLOCK*>(&_Myimpl));
    }

    lock_guard::lock_guard(shared_lock& _Lock) noexcept : _Mylock(_Lock) {
        _Mylock.lock();
    }

    lock_guard::~lock_guard() noexcept {
        _Mylock.unlock();
    }

    shared_lock_guard::shared_lock_guard(shared_lock& _Lock) noexcept : _Mylock(_Lock) {
        _Mylock.lock_shared();
    }

    shared_lock_guard::~shared_lock_guard() noexcept {
        _Mylock.unlock_shared();
    }
} // namespace mjx