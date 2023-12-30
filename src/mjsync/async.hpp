// async.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_ASYNC_HPP_
#define _MJSYNC_ASYNC_HPP_
#include <mjmem/object_allocator.hpp>
#include <mjsync/thread.hpp>
#include <tuple>
#include <type_traits>

namespace mjx {
    template <class _Sched, class = void>
    inline constexpr bool _Is_scheduler = false;

    template <class _Sched>
    inline constexpr bool _Is_scheduler<_Sched,
        ::std::void_t<decltype(::std::declval<_Sched&>().schedule_task(
            thread::callable{}, static_cast<void*>(nullptr), task_priority{}))>> = true;

    template <class _Sched>
    using _Enable_if_is_scheduler = ::std::enable_if_t<_Is_scheduler<_Sched>, int>;

    template <class _Fn, class... _Types>
    class _Task_invoker { // invokes any callable as a thread task
    public:
        using _Tuple = ::std::tuple<::std::decay_t<_Fn>, ::std::decay_t<_Types>...>;

        struct _Delete_guard {
            void* _Ptr;

            ~_Delete_guard() noexcept {
                ::mjx::delete_object(static_cast<_Tuple*>(_Ptr));
            }
        };

        static constexpr _Tuple* _Pack_callable(_Fn&& _Func, _Types&&... _Args) {
            return ::mjx::create_object<_Tuple>(::std::forward<_Fn>(_Func), ::std::forward<_Types>(_Args)...);
        }

        static constexpr void _Invoker(void* const _Data) noexcept {
            _Delete_guard _Guard{_Data}; // automatically deletes _Data
            _Tuple& _Packed = *static_cast<_Tuple*>(_Data);
            const _Fn _Func = ::std::forward<_Fn>(::std::get<_Fn>(_Packed));
            try {
                (void) _Func(::std::forward<_Types>(::std::get<_Types>(_Packed))...); // may throw
            } catch (...) {
                // don't care about the thrown exception
            }
        }
    };

    template <class _Sched, class _Fn, class... _Types, _Enable_if_is_scheduler<_Sched> = 0>
    constexpr bool async(_Sched& _Scheduler, const task_priority _Priority, _Fn&& _Func, _Types&&... _Args) {
        using _Invoker_t        = _Task_invoker<_Fn, _Types...>;
        using _Tuple_t          = typename _Invoker_t::_Tuple;
        const auto _Invoker     = &_Invoker_t::_Invoker;
        _Tuple_t* const _Packed = _Invoker_t::_Pack_callable(
            ::std::forward<_Fn>(_Func), ::std::forward<_Types>(_Args)...);
        return _Packed ? _Scheduler.schedule_task(_Invoker, _Packed, _Priority) : false;
    }

    template <class _Sched, class _Fn, class... _Types, _Enable_if_is_scheduler<_Sched> = 0>
    constexpr bool async(_Sched& _Scheduler, _Fn&& _Func, _Types&&... _Args) {
        return ::mjx::async(
            _Scheduler, task_priority::normal, ::std::forward<_Fn>(_Func), ::std::forward<_Types>(_Args)...);
    }
} // namespace mjx

#endif // _MJSYNC_ASYNC_HPP_