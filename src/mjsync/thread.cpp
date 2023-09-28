// thread.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjsync/details/thread.hpp>
#include <mjsync/thread.hpp>
#include <type_traits>

namespace mjsync {
    thread::thread() noexcept : _Myimpl(::std::make_unique<details::_Thread_impl>()) {}

    thread::thread(thread&& _Other) noexcept : _Myimpl(_Other._Myimpl.release()) {}

    thread::thread(const task _Task, void* const _Data) noexcept
        : _Myimpl(::std::make_unique<details::_Thread_impl>(
            details::_Thread_task{_Task, _Data, task_priority::normal})) {}

    thread::~thread() noexcept {
        terminate(); // wait for the current task to finish, discard the rest
    }

    thread& thread::operator=(thread&& _Other) noexcept {
        if (this != ::std::addressof(_Other)) {
            _Myimpl.reset(_Other._Myimpl.release());
        }
        
        return *this;
    }

    bool thread::joinable() const noexcept {
        return _Myimpl ? _Myimpl->_Get_state() != thread_state::terminated : false;
    }

    thread::id thread::get_id() const noexcept {
        return _Myimpl ? _Myimpl->_Id : 0;
    }

    thread::native_handle_type thread::native_handle() const noexcept {
        return _Myimpl ? _Myimpl->_Handle : nullptr;
    }

    thread_state thread::state() const noexcept {
        return _Myimpl ? _Myimpl->_Get_state() : thread_state::terminated;
    }

    size_t thread::pending_tasks() const noexcept {
        return _Myimpl ? _Myimpl->_Cache._Queue._Size() : 0;
    }

    void thread::cancel_all_pending_tasks() noexcept {
        if (_Myimpl) {
            _Myimpl->_Cache._Queue._Clear();
        }
    }

    bool thread::schedule_task(const task _Task, void* const _Data, const task_priority _Priority) noexcept {
        if (!_Myimpl) {
            return false;
        }

        const thread_state _State = _Myimpl->_Get_state();
        if (_State == thread_state::terminated) {
            return false;
        }

        if (!_Myimpl->_Cache._Queue._Enqueue(details::_Thread_task{_Task, _Data, _Priority})) {
            return false;
        }

        // resume the thread if it is waiting
        return _State == thread_state::waiting ? resume() : true;
    }

    bool thread::terminate(const bool _Wait) noexcept {
        if (!_Myimpl) {
            return false;
        }

        // Note: To ensure the proper termination of the thread, we suspend it first and then
        //       set its state to "terminated". Suspending the thread is crucial for immediate
        //       and accurate termination, as it prevents the thread from picking up another task,
        //       which could otherwise delay termination.
        if (_Myimpl->_Get_state() != thread_state::waiting) {
            if (!_Myimpl->_Suspend()) {
                return false;
            }
        }

        _Myimpl->_Set_state(thread_state::terminated); // force the thread to terminate itself
        if (!_Myimpl->_Resume()) {
            return false;
        }
        
        if (_Wait) { // wait for termination completion
            _Myimpl->_Wait_until_terminated();
        }

        _Myimpl->_Tidy();
        return true;
    }

    bool thread::suspend() noexcept {
        if (!_Myimpl || _Myimpl->_Get_state() != thread_state::working) {
            return false;
        }

        if (!_Myimpl->_Suspend()) { // failed to suspend the thread, break
            return false;
        }

        _Myimpl->_Set_state(thread_state::waiting); // update thread's state
        return true;
    }

    bool thread::resume() noexcept {
        if (!_Myimpl || _Myimpl->_Get_state() != thread_state::waiting) {
            return false;
        }

        // Note: Unlike suspend(), where the thread can be suspended directly, we must first change
        //       the thread's state before resuming it. If the thread's routine observes that
        //       it is in a waiting state, it may decide to suspend itself again, effectively
        //       remaining suspended, even if we attempt to resume it manually.
        _Myimpl->_Set_state(thread_state::working);
        if (_Myimpl->_Resume()) {
            return true;
        } else { // failed to resume the thread, restore an old state
            _Myimpl->_Set_state(thread_state::waiting);
            return false;
        }
    }

    size_t hardware_concurrency() noexcept {
        static const size_t _Count = details::_Hardware_concurrency();
        return _Count;
    }
} // namespace mjsync