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
#ifdef _Analysis_assume_lock_held_
        _Analysis_assume_lock_held_(reinterpret_cast<SRWLOCK>(_Myimpl)); // avoids C26110 warning
#endif // _Analysis_assume_lock_held_
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