// sync_flag.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjsync/sync_flag.hpp>

namespace mjx {
    sync_flag::sync_flag(const bool _Initial_value) noexcept : _Myval(_Initial_value) {}

    sync_flag::~sync_flag() noexcept {}

    bool sync_flag::is_set() const noexcept {
        return _Myval.load(::std::memory_order_relaxed);
    }

    bool sync_flag::is_set(const ::std::memory_order _Order) const noexcept {
        return _Myval.load(_Order);
    }

    void sync_flag::clear() noexcept {
        _Myval.store(false, ::std::memory_order_relaxed);
    }

    void sync_flag::clear(const ::std::memory_order _Order) noexcept {
        _Myval.store(false, _Order);
    }

    void sync_flag::set() noexcept {
        _Myval.store(true, ::std::memory_order_relaxed);
    }

    void sync_flag::set(const ::std::memory_order _Order) noexcept {
        _Myval.store(true, _Order);
    }
} // namespace mjx