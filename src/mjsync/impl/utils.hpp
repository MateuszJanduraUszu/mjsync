// utils.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_IMPL_UTILS_HPP_
#define _MJSYNC_IMPL_UTILS_HPP_
#include <crtdbg.h>
#include <cstdlib>
#include <new>

// generic assert macro, useful in debug mode
#define _INTERNAL_ASSERT(_Cond, _Msg)                                   \
    if (!(_Cond)) {                                                     \
        ::_CrtDbgReport(_CRT_ERROR, __FILE__, __LINE__, nullptr, _Msg); \
    }

namespace mjx {
    namespace mjsync_impl {
        [[noreturn]] inline void _Unreachable() noexcept {
            __assume(false);
#ifdef _DEBUG
            ::abort();
#endif // _DEBUG
        }

        inline void _Narrow_to_widen(const char* _First, const char* const _Last, wchar_t* _Dest) noexcept {
            for (; _First != _Last; ++_First, ++_Dest) {
                *_Dest = static_cast<wchar_t>(*_First); // no encoding conversion is performed
            }

            *_Dest = L'\0'; // end with null-terminator
        }
    } // namespace mjsync_impl
} // namespace mjx

#endif // _MJSYNC_IMPL_UTILS_HPP_