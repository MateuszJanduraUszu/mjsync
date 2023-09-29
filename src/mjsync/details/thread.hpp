// thread.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_DETAILS_THREAD_HPP_
#define _MJSYNC_DETAILS_THREAD_HPP_
#include <atomic>
#include <cstdlib>
#include <mjsync/details/tinywin.hpp>
#include <mjsync/details/utils.hpp>
#include <mjsync/srwlock.hpp>
#include <mjsync/thread.hpp>

namespace mjsync {
    namespace details {
        inline void _Terminate_this_thread() noexcept {
            ::ExitThread(0);
        }
        
        inline void _Suspend_this_thread() noexcept {
            ::SuspendThread(::GetCurrentThread());
        }

        struct _Thread_task {
            thread::task _Func;
            void* _Data;
            task_priority _Priority;
        };

        class _Thread_task_queue { // single-linked non-throwing priority queue
        public:
            _Thread_task_queue() noexcept : _Mystorage(), _Mylock() {}

            ~_Thread_task_queue() noexcept {
                _Clear();
            }

            _Thread_task_queue(const _Thread_task_queue&)            = delete;
            _Thread_task_queue& operator=(const _Thread_task_queue&) = delete;

            bool _Empty() const noexcept {
                shared_lock_guard _Guard(_Mylock);
                return _Mystorage._Size == 0;
            }

            const size_t _Size() const noexcept {
                shared_lock_guard _Guard(_Mylock);
                return _Mystorage._Size;
            }

            void _Clear() noexcept {
                lock_guard _Guard(_Mylock);
                if (_Mystorage._Size > 0) {
                    for (_Queue_node* _Node = _Mystorage._Head, *_Next; _Node != nullptr; _Node = _Next) {
                        _Next = _Node->_Next;
                        _Destroy_object(_Node);
                    }

                    _Mystorage._Reset();
                }
            }

            bool _Enqueue(_Thread_task&& _Task) noexcept {
                void* const _Ptr = _Allocate_memory_for_object<_Queue_node>();
                if (!_Ptr) { // allocation failed, break
                    return false;
                }

                lock_guard _Guard(_Mylock);
                if (_Mystorage._Size == 0) { // allocate the first node, don't care about priority
                    _Mystorage._Head = ::new (_Ptr) _Queue_node(::std::move(_Task));
                    _Mystorage._Tail = _Mystorage._Head;
                    ++_Mystorage._Size;
                } else { // allocate another node, compare priority
                    if (_Task._Priority == task_priority::idle) { // always at the end of the queue
                        _Mystorage._Tail->_Next = ::new (_Ptr) _Queue_node(::std::move(_Task));
                        _Mystorage._Tail        = _Mystorage._Tail->_Next;
                        ++_Mystorage._Size;
                        return true;
                    }

                    if (_Mystorage._Head->_Task._Priority < _Task._Priority) { // insert before head
                        _Queue_node* const _Old_head = _Mystorage._Head;
                        _Mystorage._Head             = ::new (_Ptr) _Queue_node(::std::move(_Task));
                        _Mystorage._Head->_Next      = _Old_head;
                        ++_Mystorage._Size;
                    } else { // insert after head
                        _Queue_node* _Current = _Mystorage._Head->_Next; // skip head, already checked
                        _Queue_node* _Prev    = _Mystorage._Head;
                        while (_Current && _Current->_Task._Priority >= _Task._Priority) {
                            _Prev    = _Current;
                            _Current = _Current->_Next;
                        }

                        _Queue_node* const _New_node = ::new (_Ptr) _Queue_node(::std::move(_Task));
                        _Prev->_Next                 = _New_node;
                        _New_node->_Next             = _Current;
                        if (!_Current) { // insert at the end
                            _Mystorage._Tail = _New_node;
                        }

                        ++_Mystorage._Size;
                    }
                }

                return true;
            }

            _Thread_task _Steal() noexcept {
                lock_guard _Guard(_Mylock);
                if (_Mystorage._Size == 0) { // nothing to pop, do nothing
                    return _Thread_task{};
                }

                _Queue_node* _Head       = _Mystorage._Head;
                const _Thread_task _Task = static_cast<_Thread_task&&>(_Mystorage._Head->_Task);
                _Mystorage._Head         = _Head->_Next;
                _Destroy_object(_Head);
                --_Mystorage._Size;
                return _Task;
            }

        private:
            struct _Queue_node {
                _Queue_node* _Next = nullptr;
                _Thread_task _Task = _Thread_task{};

                explicit _Queue_node(_Thread_task&& _Task) noexcept
                    : _Next(nullptr), _Task(::std::move(_Task)) {}
            };

            struct _Queue_storage {
                _Queue_node* _Head = nullptr;
                _Queue_node* _Tail = nullptr;
                size_t _Size       = 0;

                void _Reset() noexcept {
                    _Head = nullptr;
                    _Tail = nullptr;
                    _Size = 0;
                }
            };

            _Queue_storage _Mystorage;
            mutable shared_lock _Mylock;
        };

        class _Thread_cache { // thread's internal cache
        public:
            ::std::atomic<thread_state> _State;
            _Thread_task_queue _Queue;

            explicit _Thread_cache(const thread_state _Initial_state) noexcept
                : _State(_Initial_state), _Queue() {}
        };

        class _Thread_impl {
        public:
            void* _Handle;
            uint32_t _Id;
            _Thread_cache _Cache;

            _Thread_impl() noexcept : _Handle(nullptr), _Id(0), _Cache(thread_state::waiting) {
                _Attach();
            }

            explicit _Thread_impl(_Thread_task&& _Task) noexcept
                : _Handle(nullptr), _Id(0), _Cache(thread_state::working) {
                _Cache._Queue._Enqueue(::std::move(_Task)); // schedule an immediate task
                if (!_Attach()) {
                    _Cache._Queue._Clear();
                }
            }

            thread_state _Get_state() const noexcept {
                return _Cache._State.load(::std::memory_order_relaxed);
            }

            void _Set_state(const thread_state _New_state) noexcept {
                _Cache._State.store(_New_state, ::std::memory_order_relaxed);
            }

            void _Wait_until_terminated() noexcept {
                ::WaitForSingleObject(_Handle, 0xFFFF'FFFF); // infinite timeout
            }

            void _Tidy() noexcept {
                _Cache._Queue._Clear(); // clear task queue
                ::CloseHandle(_Handle);
                _Handle = nullptr;
                _Id     = 0;
            }

            bool _Suspend() noexcept {
                return ::SuspendThread(_Handle) != 0xFFFF'FFFF;
            }

            bool _Resume() noexcept {
                return ::ResumeThread(_Handle) != 0xFFFF'FFFF;
            }

        private:
            static unsigned long __stdcall _Thread_routine(void* const _Data) noexcept {
                _Thread_cache* const _Cache = static_cast<_Thread_cache*>(_Data);
                for (;;) {
                    switch (_Cache->_State.load(::std::memory_order_relaxed)) {
                    case thread_state::terminated: // end execution
                        _Terminate_this_thread();
                        break;
                    case thread_state::waiting: // wait for any signal
                        _Suspend_this_thread();
                        break;
                    case thread_state::working: // perform another task
                        if (!_Cache->_Queue._Empty()) {
                            const _Thread_task _Task = static_cast<_Thread_task&&>(_Cache->_Queue._Steal());
                            (*_Task._Func)(_Task._Data);
                        } else { // no more tasks, wait for any
                            _Cache->_State.store(thread_state::waiting, ::std::memory_order_relaxed);
                        }

                        break;
                    default:
                        __assume(false); // unreachable
#ifdef _DEBUG
                        ::std::abort();
#endif // _DEBUG
                        break;
                    }
                }
                
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

        inline size_t _Hardware_concurrency() noexcept {
            SYSTEM_INFO _Info = {0};
            ::GetSystemInfo(&_Info);
#ifdef _M_X64
            return static_cast<size_t>(_Info.dwNumberOfProcessors);
#else // ^^^ _M_X64 ^^^ / vvv _M_IX86 vvv
            return _Info.dwNumberOfProcessors;
#endif // _M_X64
        }
    } // namespace details
} // namespace mjsync

#endif // _MJSYNC_DETAILS_THREAD_HPP_