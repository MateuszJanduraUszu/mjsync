// task.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_IMPL_TASK_HPP_
#define _MJSYNC_IMPL_TASK_HPP_
#include <mjsync/impl/thread.hpp>
#include <mjsync/task.hpp>

namespace mjx {
    namespace mjsync_impl {
        class _Task_proxy { // proxy class for task-thread linkage
        public:
            _Task_proxy(const task::id _Id, _Thread_impl* const _Thread) noexcept
                : _Mytask(_Thread ? _Thread->_Find_task(_Id) : nullptr) {}

            ~_Task_proxy() noexcept {}

            _Task_proxy(const _Task_proxy&)            = delete;
            _Task_proxy& operator=(const _Task_proxy&) = delete;

            task_state _Get_state() const noexcept {
                return _Mytask ? _Mytask->_State.load(::std::memory_order_relaxed) : task_state::none;
            }

            task_priority _Get_priority() const noexcept {
                return _Mytask ? _Mytask->_Priority : task_priority::none;
            }

            task::cancellation_result _Cancel() noexcept {
                using _Result_t = task::cancellation_result;
                if (!_Mytask) {
                    return _Result_t::task_not_registered;
                }

                if (_Mytask->_State.load(::std::memory_order_relaxed) == task_state::canceled) {
                    return _Result_t::already_canceled;
                }

                _Mytask->_State.store(task_state::canceled, ::std::memory_order_relaxed);
                return _Result_t::success;
            }

            void _Wait_until_done() noexcept {
                if (_Mytask) {
                    switch (_Mytask->_State.load(::std::memory_order_relaxed)) {
                    case task_state::enqueued:
                    case task_state::running:
                        // worth waiting, do it
                        _Mytask->_Completion_event.wait_and_reset();
                        break;
                    default:
                        // avoid infinite wait, don't wait
                        break;
                    }
                }
            }

        private:
            _Queued_task* _Mytask;
        };
    } // namespace mjsync_impl
} // namespace mjx

#endif // _MJSYNC_IMPL_TASK_HPP_