// thread.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjmem/object_allocator.hpp>
#include <mjsync/impl/thread.hpp>
#include <mjsync/thread.hpp>
#include <type_traits>

namespace mjx {
    thread::thread() : _Myimpl(::mjx::create_object<mjsync_impl::_Thread_impl>()) {}

    thread::thread(thread&& _Other) noexcept : _Myimpl{_Other._Myimpl.release()} {}

    thread::thread(const callable _Callable, void* const _Arg)
        : _Myimpl(::mjx::create_object<mjsync_impl::_Thread_impl>()) {
        schedule_task(_Callable, _Arg); // schedule an immediate task
    }

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

    task thread::schedule_task(
        const callable _Callable, void* const _Arg, const task_priority _Priority, const bool _Resume) {
        if (!_Myimpl) {
            return task{};
        }

        const thread_state _State = _Myimpl->_Get_state();
        if (_State == thread_state::terminated) {
            return task{};
        }

        const task::id _New_id = _Myimpl->_Cache._Counter._Next_id();
        _Myimpl->_Cache._Queue._Enqueue(mjsync_impl::_Queued_task(
            _New_id, _Callable, _Arg, _Priority));
        task _New_task(_New_id, this);
        if (_State == thread_state::waiting && _Resume) { // resume the thread
            resume();
        }

        return _New_task;
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
        
        if (_Wait) { // wait until terminated
            _Myimpl->_Wait_until_terminated();
        }

        _Myimpl.reset();
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
        static const size_t _Count = mjsync_impl::_Hardware_concurrency();
        return _Count;
    }

    thread::id current_thread_id() noexcept {
        // retrieve the thread identifier of the calling thread
        return ::GetCurrentThreadId();
    }

    void yield_current_thread() noexcept {
        // yield execution to another thread that is ready to run on the current processor
        ::SwitchToThread();
    }

    void sleep_for(const uintmax_t _Duration) noexcept {
        // suspend the execution of the current thread until the time-out interval elapses
        ::Sleep(static_cast<unsigned long>(_Duration));
    }
} // namespace mjx