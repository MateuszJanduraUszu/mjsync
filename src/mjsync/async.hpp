// async.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_ASYNC_HPP_
#define _MJSYNC_ASYNC_HPP_
#include <concepts>
#include <mjmem/smart_pointer.hpp>
#include <mjsync/task.hpp>
#include <mjsync/thread.hpp>
#include <tuple>
#include <type_traits>

namespace mjx {
    template <class _Sched>
    concept task_scheduler = requires(_Sched& _Scheduler) {
        { _Scheduler.schedule_task(thread::callable{}, static_cast<void*>(nullptr), task_priority{}) }
            -> ::std::same_as<task>;
    };

    template <class _Fn, class... _Types>
    class _Task_invoker { // invokes any callable as a thread task
    public:
        template <class _Tuple, size_t... _Indices>
        static constexpr void _Invoker(void* _Arg) {
            // obtain _Fn and _Types... from _Arg, then invoke it
            const unique_smart_ptr<_Tuple> _Unique(static_cast<_Tuple*>(_Arg));
            _Tuple& _Vals = *_Unique;
            ::std::invoke(::std::move(::std::get<_Indices>(_Vals))...);
        }

        template <class _Tuple, size_t... _Indices>
        static constexpr auto _Get_invoker(::std::index_sequence<_Indices...>) noexcept {
            return &_Invoker<_Tuple, _Indices...>;
        }
    };

    template <task_scheduler _Sched, class _Fn, class... _Types>
    constexpr task async(_Sched& _Scheduler, const task_priority _Priority, _Fn&& _Func, _Types&&... _Args) {
        using _Invoker_t        = _Task_invoker<_Fn, _Types...>;
        using _Tuple_t          = ::std::tuple<::std::decay_t<_Fn>, ::std::decay_t<_Types>...>;
        auto _Vals              = ::mjx::make_unique_smart_ptr<_Tuple_t>(
            ::std::forward<_Fn>(_Func), ::std::forward<_Types>(_Args)...);
        constexpr auto _Invoker = _Invoker_t::_Get_invoker<_Tuple_t>(
            ::std::make_index_sequence<1 + sizeof...(_Types)>{});
        task _Task              = _Scheduler.schedule_task(_Invoker, _Vals.get(), _Priority);
        if (_Task.is_registered()) {
            _Vals.release(); // release ownership of _Vals, _Invoker() will destroy it
        }

        return _Task;
    }

    template <task_scheduler _Sched, class _Fn, class... _Types>
    constexpr task async(_Sched& _Scheduler, _Fn&& _Func, _Types&&... _Args) {
        return ::mjx::async(
            _Scheduler, task_priority::normal, ::std::forward<_Fn>(_Func), ::std::forward<_Types>(_Args)...);
    }
} // namespace mjx

#endif // _MJSYNC_ASYNC_HPP_