// utils.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_DETAILS_UTILS_HPP_
#define _MJSYNC_DETAILS_UTILS_HPP_
#include <new>

namespace mjsync {
    namespace details {
        template <class _Ty>
        constexpr void* _Allocate_memory_for_object() noexcept {
            return ::operator new(sizeof(_Ty), ::std::nothrow);
        }

        template <class _Ty>
        constexpr void _Destroy_object(_Ty* const _Obj) noexcept {
            if (_Obj) {
                _Obj->~_Ty();
                ::operator delete(static_cast<void*>(_Obj), sizeof(_Ty));
            }
        }
    } // namespace details
} // namespace mjsync

#endif // _MJSYNC_DETAILS_UTILS_HPP_