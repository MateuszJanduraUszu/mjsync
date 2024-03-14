// thread.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_IMPL_THREAD_HPP_
#define _MJSYNC_IMPL_THREAD_HPP_
#include <atomic>
#include <cstdlib>
#include <mjmem/object_allocator.hpp>
#include <mjsync/impl/tinywin.hpp>
#include <mjsync/impl/utils.hpp>
#include <mjsync/srwlock.hpp>
#include <mjsync/thread.hpp>
#include <mjsync/waitable_event.hpp>
#include <type_traits>

namespace mjx {
    namespace mjsync_impl {
        class _Queued_task {
        public:
            task::id _Id;
            ::std::atomic<task_state> _State;
            waitable_event _Completion_event;
            task_priority _Priority;
            thread::callable _Callable;
            void* _Arg;

            _Queued_task() noexcept : _Id(0), _State(task_state::canceled), _Completion_event(),
                _Priority(task_priority::idle), _Callable(nullptr), _Arg(nullptr) {}
        
            _Queued_task(_Queued_task&& _Other) noexcept {
                _Steal_data(_Other);
            }

            _Queued_task(const task::id _Id, const thread::callable _Callable,
                void* const _Arg, const task_priority _Priority) noexcept
                : _Id(_Id), _State(task_state::enqueued), _Completion_event(), _Priority(_Priority),
                _Callable(_Callable), _Arg(_Arg) {}

            ~_Queued_task() noexcept {}

            _Queued_task& operator=(_Queued_task&& _Other) noexcept {
                if (this != &_Other) {
                    _Steal_data(_Other);
                }

                return *this;
            }

            _Queued_task(const _Queued_task&)            = delete;
            _Queued_task& operator=(const _Queued_task&) = delete;

            bool _Should_execute() const noexcept {
                return _State.load(::std::memory_order_acquire) == task_state::enqueued;
            }

            void _Execute() noexcept {
                _Set_state(task_state::running);
                try {
                    _Callable(_Arg);
                    _Set_state(task_state::done);
                } catch (...) {
                    _Set_state(task_state::interrupted);
                }

                _Completion_event.notify(); // notify regardless of the task result
            }

        private:
            void _Set_state(const task_state _New_state) noexcept {
                _State.store(_New_state, ::std::memory_order_release);
            }

            void _Steal_data(_Queued_task& _Other) noexcept {
                _Id               = _Other._Id;
                _State            = _Other._State.exchange(task_state::canceled, ::std::memory_order_relaxed);
                _Completion_event = ::std::move(_Other._Completion_event);
                _Priority         = _Other._Priority;
                _Callable         = _Other._Callable;
                _Arg              = _Other._Arg;
                _Other._Id        = task::invalid_id;
                _Other._Priority  = task_priority::idle;
                _Other._Callable  = nullptr;
                _Other._Arg       = nullptr;
            }
        };

        class _Task_queue { // singly-linked priority queue
        public:
            _Task_queue() noexcept : _Myhead(nullptr), _Mytail(nullptr), _Mysize(0), _Mylock() {}

            ~_Task_queue() noexcept {
                _Clear();
            }

            _Task_queue(const _Task_queue&)            = delete;
            _Task_queue& operator=(const _Task_queue&) = delete;

            bool _Empty() const noexcept {
                shared_lock_guard _Guard(_Mylock);
                return _Mysize == 0;
            }

            size_t _Size() const noexcept {
                shared_lock_guard _Guard(_Mylock);
                return _Mysize;
            }

            void _Clear() noexcept {
                lock_guard _Guard(_Mylock);
                if (_Myhead) {
                    for (_Queue_node* _Node = _Myhead, *_Next; _Node != nullptr; _Node = _Next) {
                        _Next = _Node->_Next;
                        ::mjx::delete_object(_Node);
                    }
                }
            }

            _Queued_task* _Find(const task::id _Id) noexcept {
                if (!_Myhead) { // empty queue, break
                    return nullptr;
                }

                for (_Queue_node* _Node = _Myhead; _Node != nullptr; _Node = _Node->_Next) {
                    if (_Node->_Task._Id == _Id) { // task found
                        return &_Node->_Task;
                    }
                }

                return nullptr; // not found
            }

            void _Enqueue(_Queued_task&& _Task) {
                _Queue_node* const _New_node = ::mjx::create_object<_Queue_node>(::std::move(_Task));
                lock_guard _Guard(_Mylock);
                if (!_Myhead) { // insert the first node
                    _Myhead = _New_node;
                    _Mytail = _New_node;
                    _Mysize = 1;
                } else { // insert the next node
                    if (_Task._Priority == task_priority::idle) { // always at the end of the queue
                        _Mytail->_Next = _New_node;
                        _Mytail        = _New_node;
                        ++_Mysize;
                        return;
                    }

                    if (_Myhead->_Task._Priority < _Task._Priority) { // insert before the head
                        _New_node->_Next = _Myhead;
                        _Myhead          = _New_node;
                    } else { // insert after the head
                        _Queue_node* _Node = _Myhead->_Next; // skip head, already checked
                        _Queue_node* _Prev = _Myhead;
                        while (_Node && _Node->_Task._Priority >= _Task._Priority) {
                            _Prev = _Node;
                            _Node = _Node->_Next;
                        }

                        _Prev->_Next     = _New_node;
                        _New_node->_Next = _Node;
                        if (!_Node) { // insert after the tail
                            _Mytail = _New_node;
                        }
                    }

                    ++_Mysize;
                }
            }

            _Queued_task _Steal() noexcept {
                lock_guard _Guard(_Mylock);
                if (!_Myhead) { // nothing to steal, break
                    return _Queued_task{};
                }

                _Queue_node* _Head = _Myhead;
                _Queued_task _Task = static_cast<_Queued_task&&>(_Head->_Task);
                _Myhead            = _Head->_Next;
                ::mjx::delete_object(_Head);
                --_Mysize;
                return _Task;
            }

        private:
            struct _Queue_node {
                _Queue_node* _Next = nullptr;
                _Queued_task _Task;

                explicit _Queue_node(_Queued_task&& _Task) noexcept
                    : _Next(nullptr), _Task(::std::move(_Task)) {}
            };

            void _Reset() noexcept {
                _Myhead = nullptr;
                _Mytail = nullptr;
                _Mysize = 0;
            }

            _Queue_node* _Myhead;
            _Queue_node* _Mytail;
            size_t _Mysize;
            mutable shared_lock _Mylock;
        };

        class _Task_counter { // unique task ID generator
        public:
            _Task_counter() noexcept : _Myval(1) {}

            ~_Task_counter() noexcept {}

            _Task_counter(const _Task_counter&)            = delete;
            _Task_counter& operator=(const _Task_counter&) = delete;

            task::id _Next_id() noexcept {
                const task::id _Id = _Myval++;
                return _Id != task::invalid_id ? _Id : _Myval++; // skip zero, invalid ID
            }

        private:
            ::std::atomic<task::id> _Myval;
        };

        class _Thread_cache { // thread's internal cache
        public:
            ::std::atomic<thread_state> _State;
            waitable_event _State_event; // event used for synchronization when state changes
            waitable_event _Termination_event; // event used for synchronization at termination
            _Task_queue _Queue;
            _Task_counter _Counter;

            explicit _Thread_cache(const thread_state _Initial_state) noexcept
                : _State(_Initial_state), _State_event(), _Termination_event(), _Queue(), _Counter() {}

            _Thread_cache()                                = delete;
            _Thread_cache(const _Thread_cache&)            = delete;
            _Thread_cache& operator=(const _Thread_cache&) = delete;
        };

        class _Thread_impl {
        public:
            void* _Handle;
            thread::id _Id;
            _Thread_cache _Cache;

            _Thread_impl() noexcept : _Handle(nullptr), _Id(0), _Cache(thread_state::waiting) {
                _Attach();
            }

            ~_Thread_impl() noexcept {
                ::CloseHandle(_Handle);
            }

            _Thread_impl(const _Thread_impl&)            = delete;
            _Thread_impl& operator=(const _Thread_impl&) = delete;

            thread_state _Get_state() const noexcept {
                return _Cache._State.load(::std::memory_order_acquire);
            }

            void _Set_state(const thread_state _New_state) noexcept {
                _Cache._State.store(_New_state, ::std::memory_order_release);
            }

            thread_state _Exchange_state(const thread_state _New_state) noexcept {
                return _Cache._State.exchange(_New_state, ::std::memory_order_acq_rel);
            }

            _Queued_task* _Find_task(const task::id _Id) noexcept {
                return _Cache._Queue._Find(_Id);
            }

        private:
            static unsigned long __stdcall _Thread_routine(void* const _Data) noexcept {
                _Thread_cache* const _Cache = static_cast<_Thread_cache*>(_Data);
                bool _Terminate             = false; // indicates whether termination has been requested
                bool _Was_idle              = false; // indicates whether the thread was in an idle state
                while (!_Terminate) {
                    switch (_Cache->_State.load(::std::memory_order_acquire)) {
                    case thread_state::terminated: // end execution
                        _Terminate = true;
                        break;
                    case thread_state::waiting: // wait for any signal
                        _Cache->_State_event.wait_and_reset();
                        break;
                    case thread_state::working: // perform another task
                        if (!_Cache->_Queue._Empty()) {
                            _Queued_task _Task = static_cast<_Queued_task&&>(_Cache->_Queue._Steal());
                            if (_Task._Should_execute()) {
                                _Task._Execute();
                            }

                            if (_Was_idle) { // got some task, reset the flag
                                _Was_idle = false;
                            }
                        } else { // no more tasks
                            // Note: The use of atomic operations here necessitates caution in changing the
                            //       state, especially the first time when the thread has no tasks.
                            //       Modifying the state in this critical section may result in a race condition,
                            //       where a termination request is overwritten by the state change to 'waiting'.
                            //       To prevent this, we use the _Was_idle flag to toggle the state only if the
                            //       thread was previously in an idle state, avoiding a potential race condition.
                            if (_Was_idle) {
                                _Cache->_State.store(thread_state::waiting, ::std::memory_order_release);
                            }

                            _Was_idle = !_Was_idle; // toggle flag state
                        }

                        break;
                    default:
                        mjsync_impl::_Unreachable();
                        break;
                    }
                }
                
                // Note: The termination request always originates from thread::terminate(), which subsequently
                //       waits until the thread is fully terminated. It is crucial to notify the termination
                //       process to prevent an indefinite wait.
                _Cache->_Termination_event.notify();
                return 0;
            }

            bool _Attach() noexcept {
                _Handle = ::CreateThread(nullptr, 0, &_Thread_impl::_Thread_routine,
                    &_Cache, 0, reinterpret_cast<unsigned long*>(&_Id));
                if (_Handle) {
                    return true;
                } else { // failed to attach a new thread
                    _Set_state(thread_state::terminated); // mark failure
                    return false;
                }
            }
        };

        inline bool _Set_thread_name_preferred(void* const _Handle, const char* const _Name) noexcept {
            // set the thread name by using SetThreadDescription()
            using _Fn_t        = long(__stdcall*)(void*, const wchar_t*);
            static _Fn_t _Func = []() noexcept -> _Fn_t { // load it once
                const HMODULE _Module = ::GetModuleHandleW(L"Kernel32");
                if (!_Module) [[unlikely]] { // module not loaded, break
                    return nullptr;
                }

                // Note: The SetThreadDescription() function was introducated in Windows 10, version 1607.
                //       While it's possible to check the Windows version to determine availibility,
                //       a more robust approach is to attempt to load the function dynamically from
                //       the Kernel32.dll library. If the function is present, its signature will be available,
                //       allowing successful loading. Otherwise, GetProcAddress() will return a null-pointer.
                return reinterpret_cast<_Fn_t>(::GetProcAddress(_Module, "SetThreadDescription"));
            }();
            if (!_Func) { // function not loaded, break
                return false;
            }

            try {
                // call within a try-catch block to handle potential allocation failure gracefully
                const size_t _Size = ::strlen(_Name);
                auto _Widen        = ::mjx::make_unique_smart_array<wchar_t>(_Size + 1);
                mjsync_impl::_Narrow_to_widen(_Name, _Name + _Size, _Widen.get());
                return SUCCEEDED(_Func(_Handle, _Widen.get()));
            } catch (...) {
                return false;
            }
        }

        struct alignas(8) _Thread_name_info {
            unsigned long _Type;
            const char* _Name;
            unsigned long _Thread_id;
            unsigned long _Flags;
        };

        inline bool _Set_thread_name_fallback(void* const _Handle, const char* const _Name) noexcept {
            // set the thread name by throwing an exception
            _Thread_name_info _Info;
            _Info._Type      = 0x1000; // must be 0x1000
            _Info._Name      = _Name;
            _Info._Thread_id = ::GetThreadId(_Handle);
            _Info._Flags     = 0; // must be zero
            __try {
                // communicate with debugger by throwing a specially-configured exception
                constexpr unsigned long _Code = 0x406D'1388;
                ::RaiseException(_Code, 0, sizeof(_Thread_name_info) / sizeof(ULONG_PTR),
                    reinterpret_cast<const ULONG_PTR*>(&_Info));
                return false;
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                return true; // expected behavior, treat it as a success
            }
        }

        inline size_t _Hardware_concurrency() noexcept {
            SYSTEM_INFO _Info = {0};
            ::GetSystemInfo(&_Info);
#ifdef _M_X64
            return static_cast<size_t>(_Info.dwNumberOfProcessors);
#else // ^^^ _M_X64 ^^^ / vvv _M_IX86 vvv
            return _Info.dwNumberOfProcessors;
#endif // _M_X64
        }
    } // namespace mjsync_impl
} // namespace mjx

#endif // _MJSYNC_IMPL_THREAD_HPP_