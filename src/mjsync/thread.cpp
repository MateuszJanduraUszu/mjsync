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
        return state() != thread_state::terminated;
    }

    thread::id thread::get_id() const noexcept {
        return _Myimpl ? _Myimpl->_Id : 0;
    }

    thread::native_handle_type thread::native_handle() const noexcept {
        return _Myimpl ? _Myimpl->_Handle : nullptr;
    }

    thread_state thread::state() const noexcept {
        return _Myimpl ? _Myimpl->_Cache._State.load(::std::memory_order_relaxed) : thread_state::terminated;
    }

    size_t thread::pending_tasks() const noexcept {
        return _Myimpl ? _Myimpl->_Cache._Queue._Size() : 0;
    }

    bool thread::set_name(const char* const _Name) noexcept {
        if (state() == thread_state::terminated) {
            return false;
        }

        void* const _Handle = _Myimpl->_Handle;
        if (mjsync_impl::_Set_thread_name_preferred(_Handle, _Name)) { // preferred solution succeeded
            return true;
        }

        // preferred solution failed, use fallback solution
        return mjsync_impl::_Set_thread_name_fallback(_Handle, _Name);
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
        if (_State == thread_state::terminated) { // scheduling inactive, breaj
            return task{};
        }

        const task::id _New_id = _Myimpl->_Cache._Counter._Next_id();
        _Myimpl->_Cache._Queue._Enqueue(mjsync_impl::_Queued_task(_New_id, _Callable, _Arg, _Priority));
        task _New_task(_New_id, this);
        if (_State == thread_state::waiting && _Resume) { // resume the thread
            _Myimpl->_Set_state(thread_state::working);
            _Myimpl->_Cache._State_event.notify();
        }

        return _New_task;
    }

    bool thread::suspend() noexcept {
        if (!_Myimpl || _Myimpl->_Get_state() != thread_state::working) { // wrong state, break
            return false;
        }

        // set the state to 'waiting', the thread will suspend itself
        _Myimpl->_Set_state(thread_state::waiting);
        return true;
    }

    bool thread::resume() noexcept {
        if (!_Myimpl || _Myimpl->_Get_state() != thread_state::waiting) { // wrong state, break
            return false;
        }

        // set the state to 'working' and notify the thread
        _Myimpl->_Set_state(thread_state::working);
        _Myimpl->_Cache._State_event.notify();
        return true;
    }
    
    bool thread::terminate() noexcept {
        if (!_Myimpl) {
            return false;
        }

        if (_Myimpl->_Exchange_state(thread_state::terminated) == thread_state::waiting) {
            // the thread is waiting, notify it
            _Myimpl->_Cache._State_event.notify();
        }
        
        _Myimpl->_Cache._Termination_event.wait_and_reset(); // wait until terminated
        _Myimpl.reset();
        return true;
    }

    size_t hardware_concurrency() noexcept {
        static const size_t _Count = mjsync_impl::_Hardware_concurrency();
        return _Count;
    }
} // namespace mjx