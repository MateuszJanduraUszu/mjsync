// thread_pool.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_THREAD_POOL_HPP_
#define _MJSYNC_THREAD_POOL_HPP_
#include <cstddef>
#include <memory>
#include <mjsync/api.hpp>
#include <mjsync/thread.hpp>

namespace mjsync {
    namespace details {
        class _Thread_list;
    } // namespace details

    class _MJSYNC_API thread_pool {
    public:
        thread_pool() noexcept;
        thread_pool(thread_pool&& _Other) noexcept;
        ~thread_pool() noexcept;

        thread_pool(const thread_pool&)            = delete;
        thread_pool& operator=(const thread_pool&) = delete;

        explicit thread_pool(const size_t _Count) noexcept;

        thread_pool& operator=(thread_pool&& _Other) noexcept;

        // checks if the thread-pool is still open
        bool is_open() const noexcept;

        // checks if the thread-pool is waiting
        bool is_waiting() const noexcept;

        // checks if the thread-pool is working
        bool is_working() const noexcept;

        // returns the number of threads in the thread-pool
        size_t thread_count() const noexcept;

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
        bool increase_threads(const size_t _Count) noexcept;

        // removes existing threads from the thread-pool
        bool decrease_threads(const size_t _Count) noexcept;

        // changes the number of threads in the thread-pool
        bool set_thread_count(const size_t _New_count) noexcept;

        // schedules a new task
        bool schedule_task(const thread::task _Task, void* const _Data,
            const task_priority _Priority = task_priority::normal) noexcept;

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

        _Internal_state _Mystate;
#pragma warning(suppress : 4251) // C4251: _Thread_list needs to have dll-interface
        ::std::unique_ptr<details::_Thread_list> _Mylist;
    };
} // namespace mjsync

#endif // _MJSYNC_THREAD_POOL_HPP_