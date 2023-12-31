// thread_pool.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_THREAD_POOL_HPP_
#define _MJSYNC_THREAD_POOL_HPP_
#include <cstddef>
#include <mjmem/smart_pointer.hpp>
#include <mjsync/api.hpp>
#include <mjsync/task.hpp>
#include <mjsync/thread.hpp>

namespace mjx {
    namespace mjsync_impl {
        class _Thread_list;
    } // namespace mjsync_impl

    class _MJSYNC_API thread_pool {
    public:
        thread_pool() noexcept;
        thread_pool(thread_pool&& _Other) noexcept;
        ~thread_pool() noexcept;

        thread_pool(const thread_pool&)            = delete;
        thread_pool& operator=(const thread_pool&) = delete;

        explicit thread_pool(const size_t _Count);

        thread_pool& operator=(thread_pool&& _Other) noexcept;

        // checks if the thread-pool is still open
        bool is_open() const noexcept;

        // checks if the thread-pool is waiting
        bool is_waiting() const noexcept;

        // checks if the thread-pool is working
        bool is_working() const noexcept;

        // closes the thread-pool
        void close() noexcept;

        // checks if the thread is in the thread-pool
        bool is_thread_in_pool(const thread::id _Id) const noexcept;

        struct statistics {
            size_t waiting_threads = 0;
            size_t working_threads = 0;
            size_t pending_tasks   = 0;
        };

        // collects the thread-pool's statistics
        statistics collect_statistics() const noexcept;

        // cancels all pending tasks
        void cancel_all_pending_tasks() noexcept;

        // adds additional threads to the thread-pool
        void increase_thread_count(const size_t _Count);

        // removes existing threads from the thread-pool
        void decrease_thread_count(const size_t _Count) noexcept;

        // returns or changes the number of threads in the thread-pool
        size_t thread_count() const noexcept;
        void thread_count(const size_t _New_count);

        // schedules a new task
        task schedule_task(const thread::callable _Callable, void* const _Arg,
            const task_priority _Priority = task_priority::normal);

        // suspends all threads
        bool suspend() noexcept;

        // resumes all threads
        bool resume() noexcept;

    private:
        enum _Internal_state : unsigned char {
            _Closed,
            _Waiting,
            _Working
        };

        // selects the best thread for task scheduling
        thread* _Select_ideal_thread() noexcept;

#pragma warning(suppress : 4251) // C4251: _Thread_list needs to have dll-interface
        unique_smart_ptr<mjsync_impl::_Thread_list> _Mylist;
        _Internal_state _Mystate;
    };
} // namespace mjx

#endif // _MJSYNC_THREAD_POOL_HPP_