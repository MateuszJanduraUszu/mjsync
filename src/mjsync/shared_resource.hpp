// shared_resource.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_SHARED_RESOURCE_HPP_
#define _MJSYNC_SHARED_RESOURCE_HPP_
#include <mjsync/srwlock.hpp>
#include <type_traits>

namespace mjx {
    template <class _Ty>
    class shared_resource { // manages access to a shared resource
    public:
        using value_type      = _Ty;
        using reference       = _Ty&;
        using const_reference = const _Ty&;

        constexpr shared_resource() noexcept(::std::is_nothrow_default_constructible_v<_Ty>)
            : _Myval(), _Mylock() {}

        template <class... _Types>
        constexpr explicit shared_resource(
            _Types&&... _Args) noexcept(::std::is_nothrow_constructible_v<_Ty, _Types...>)
            : _Myval(::std::forward<_Types>(_Args)...), _Mylock() {}

        constexpr ~shared_resource() noexcept {}

        shared_resource(const shared_resource&)            = delete;
        shared_resource& operator=(const shared_resource&) = delete;

        constexpr reference get() noexcept {
            lock_guard _Guard(_Mylock);
            return _Myval;
        }

        constexpr const_reference get() const noexcept {
            shared_lock_guard _Guard(_Mylock);
            return _Myval;
        }

        template <class _Visitor>
        constexpr decltype(auto) visit(_Visitor&& _Vis) noexcept(noexcept(_Vis(::std::declval<_Ty&>()))) {
            lock_guard _Guard(_Mylock);
            return ::std::forward<_Visitor>(_Vis)(_Myval);
        }

        template <class _Visitor>
        constexpr decltype(auto) visit(_Visitor&& _Vis) const
            noexcept(noexcept(_Vis(::std::declval<const _Ty&>()))) {
            shared_lock_guard _Guard(_Mylock);
            return ::std::forward<_Visitor>(_Vis)(_Myval);
        }

    private:
        _Ty _Myval;
        mutable shared_lock _Mylock;
    };
} // namespace mjx

#endif // _MJSYNC_SHARED_RESOURCE_HPP_