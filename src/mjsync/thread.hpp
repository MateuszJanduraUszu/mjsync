// thread.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_THREAD_HPP_
#define _MJSYNC_THREAD_HPP_
#include <mjmem/smart_pointer.hpp>
#include <mjsync/api.hpp>
#include <mjsync/task.hpp>

namespace mjx {
    namespace mjsync_impl {
        class _Thread_impl;
    } // namespace mjsync_impl

    enum class thread_state : unsigned char {
        terminated,
        waiting,
        working
    };

    class _MJSYNC_API thread {
    public:
        using native_handle_type = void*;
        using id                 = unsigned int;
        using callable           = void(*)(void*);

        thread();
        thread(thread&& _Other) noexcept;
        ~thread() noexcept;

        thread(const thread&)            = delete;
        thread& operator=(const thread&) = delete;

        thread(const callable _Callable, void* const _Arg);

        thread& operator=(thread&& _Other) noexcept;

        // checks if the thread is joinable
        bool joinable() const noexcept;

        // returns the thread's ID
        id get_id() const noexcept;

        // returns the thread's handle
        native_handle_type native_handle() const noexcept;

        // returns the thread's state
        thread_state state() const noexcept;

        // returns the number of pending tasks
        size_t pending_tasks() const noexcept;

        // cancels all pending tasks
        void cancel_all_pending_tasks() noexcept;

        // schedules a new task
        task schedule_task(const callable _Callable, void* const _Arg,
            const task_priority _Priority = task_priority::normal, const bool _Resume = true);

        // terminates the thread (optinally waits)
        bool terminate(const bool _Wait = true) noexcept;

        // suspends the thread
        bool suspend() noexcept;

        // resumes the thread
        bool resume() noexcept;

    private:
        friend task;

#pragma warning(suppress : 4251) // C4251: _Thread_impl needs to have dll-interface
        unique_smart_ptr<mjsync_impl::_Thread_impl> _Myimpl;
    };

    _MJSYNC_API size_t hardware_concurrency() noexcept;
} // namespace mjx

#endif // _MJSYNC_THREAD_HPP_