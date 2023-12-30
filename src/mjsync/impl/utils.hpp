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
    } // namespace mjsync_impl
} // namespace mjx

#endif // _MJSYNC_IMPL_UTILS_HPP_