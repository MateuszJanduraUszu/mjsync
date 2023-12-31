// task.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_TASK_HPP_
#define _MJSYNC_TASK_HPP_
#include <cstdint>
#include <mjsync/api.hpp>

namespace mjx {
    enum class task_priority : unsigned char {
        none,
        idle,
        below_normal,
        normal,
        above_normal,
        real_time
    };

    enum class task_state : unsigned char {
        none,
        canceled,
        enqueued,
        running,
        interrupted,
        done
    };

    class thread;

    class _MJSYNC_API task { // oversees task lifecycle and execution
    public:
        using id = uint64_t;

        static constexpr id invalid_id = 0;

        task() noexcept;
        task(task&& _Other) noexcept;
        ~task() noexcept;

        task& operator=(task&& _Other) noexcept;

        task(const task&)            = delete;
        task& operator=(const task&) = delete;

        // checks if the task is registered
        bool is_registered() const noexcept;

        // returns the task ID
        id get_id() const noexcept;

        // returns the task state
        task_state state() const noexcept;

        // returns the task priority
        task_priority priority() const noexcept;

        enum class cancellation_result : unsigned char {
            success,
            already_canceled,
            task_not_registered
        };

        // cancels the task
        cancellation_result cancel() noexcept;

        // waits until the task is done
        void wait_until_done() noexcept;

    private:
        friend thread;

        task(const id _Id, thread* const _Thread) noexcept;

        id _Myid; // unique task ID within thread task queue
        thread* _Mythrd; // thread that will execute this task
    };
} // namespace mjx

#endif // _MJSYNC_TASK_HPP_