// thread.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_THREAD_HPP_
#define _MJSYNC_THREAD_HPP_
#include <memory>
#include <mjsync/api.hpp>

namespace mjsync {
    namespace details {
        class _Thread_impl;
    } // namespace details

    enum class thread_state : unsigned char {
        terminated,
        waiting,
        working
    };

    enum class task_priority : unsigned char {
        idle,
        below_normal,
        normal,
        above_normal,
        real_time
    };

    class _MJSYNC_API thread {
    public:
        using native_handle_type = void*;
        using id                 = unsigned int;
        using task               = void(*)(void*);

        thread() noexcept;
        thread(thread&& _Other) noexcept;
        ~thread() noexcept;

        thread(const thread&)            = delete;
        thread& operator=(const thread&) = delete;

        explicit thread(const task _Task, void* const _Data) noexcept;

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
        bool schedule_task(const task _Task, void* const _Data,
            const task_priority _Priority = task_priority::normal) noexcept;

        // terminates the thread (optinally waits)
        bool terminate(const bool _Wait = true) noexcept;

        // suspends the thread
        bool suspend() noexcept;

        // resumes the thread
        bool resume() noexcept;

    private:
#pragma warning(suppress : 4251) // C4251: _Thread_impl needs to have dll-interface
        ::std::unique_ptr<details::_Thread_impl> _Myimpl;
    };

    _MJSYNC_API size_t hardware_concurrency() noexcept;
} // namespace mjsync

#endif // _MJSYNC_THREAD_HPP_