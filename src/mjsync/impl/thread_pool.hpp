// thread_pool.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJSYNC_IMPL_THREAD_POOL_HPP_
#define _MJSYNC_IMPL_THREAD_POOL_HPP_
#include <cstddef>
#include <mjmem/object_allocator.hpp>
#include <mjsync/impl/utils.hpp>
#include <mjsync/thread.hpp>
#include <type_traits>

namespace mjx {
    namespace mjsync_impl {
        class _Thread_list { // singly-linked thread list
        public:
            _Thread_list() noexcept : _Myhead(nullptr), _Mytail(nullptr), _Mysize(0) {}

            explicit _Thread_list(const size_t _Size) : _Myhead(nullptr), _Mytail(nullptr), _Mysize(0) {
                _Grow(_Size);
            }

            ~_Thread_list() noexcept {
                _Clear();
            }

            size_t _Size() const noexcept {
                return _Mysize;
            }

            void _Clear() noexcept {
                if (_Myhead) {
                    for (_List_node* _Node = _Myhead, *_Next; _Node != nullptr; _Node = _Next) {
                        _Next = _Node->_Next;
                        ::mjx::delete_object(_Node);
                    }

                    _Myhead = nullptr;
                    _Mytail = nullptr;
                    _Mysize = 0;
                }
            }

            void _Grow(size_t _Count) {
                if (_Count == 0) { // no growth, do nothing
                    return;
                }

                if (_Mysize == 0) { // allocate the first node
                    _Myhead = ::mjx::create_object<_List_node>();
                    _Mytail = _Myhead;
                    _Mysize = 1;
                    --_Count; // one already allocated
                }

                while (_Count-- > 0) {
                    _Mytail->_Next = ::mjx::create_object<_List_node>();
                    _Mytail        = _Mytail->_Next;
                    ++_Mysize;
                }
            }

            void _Reduce(size_t _Count) noexcept {
                _Reduce_waiting_threads(_Count);
                if (_Count > 0) { // reduced not enough threads, reduce few more
                    _Mysize -= _Count; // substract once
                    while (_Count-- > 0) {
                        _List_node* const _Old_head = _Myhead;
                        _Myhead                     = _Old_head->_Next;
                        ::mjx::delete_object(_Old_head);
                    }
                }
            }

            bool _Is_thread_present(const thread::id _Id) const noexcept {
                if (_Mysize == 0) {
                    return false;
                }

                for (_List_node* _Node = _Myhead; _Node != nullptr; _Node = _Node->_Next) {
                    if (_Node->_Thread.get_id() == _Id) {
                        return true;
                    }
                }

                return false;
            }

            thread* _Select_any_waiting_thread() noexcept {
                if (_Mysize == 0) {
                    return nullptr;
                }

                for (_List_node* _Node = _Myhead; _Node != nullptr; _Node = _Node->_Next) {
                    if (_Node->_Thread.state() == thread_state::waiting) {
                        return ::std::addressof(_Node->_Thread);
                    }
                }

                return nullptr;
            }

            thread* _Select_thread_with_fewest_pending_tasks() noexcept {
                switch (_Mysize) {
                case 0: // no threads available, don't select any
                    return nullptr;
                case 1: // only one thread is available, select it
                    return ::std::addressof(_Myhead->_Thread);
                default: // search for the best thread
                    break;
                }

                thread* _Result = ::std::addressof(_Myhead->_Thread); // first thread by default
                size_t _Count   = _Result->pending_tasks();
                for (_List_node* _Node = _Myhead->_Next; _Node != nullptr; _Node = _Node->_Next) {
                    const size_t _Pending_tasks = _Node->_Thread.pending_tasks();
                    if (_Pending_tasks < _Count) {
                        _Result = ::std::addressof(_Node->_Thread);
                        _Count  = _Pending_tasks;
                    }
                }

                return _Result;
            }

            template <class _Fn, class... _Types>
            void _For_each_thread(_Fn&& _Func, _Types&&... _Args) noexcept(
                noexcept(_Func(::std::declval<thread&>(), ::std::forward<_Types>(_Args)...))) {
                for (_List_node* _Node = _Myhead; _Node != nullptr; _Node = _Node->_Next) {
                    // calls _Func(thread&, ...)
                    (void) _Func(_Node->_Thread, ::std::forward<_Types>(_Args)...);
                }
            }

        private:
            struct _List_node {
                _List_node* _Next = nullptr;
                thread _Thread;
            };

            void _Reduce_waiting_threads(size_t& _Count) noexcept {
                // Note: We start from the head to ensure that we delete all matching nodes.
                //       The second step skips the head, as it must store the previous node.
                //       Neglecting to track the previous node could break the entire connection
                //       between existing nodes. This method is used exclusively within the _Reduce() method,
                //       so there's no need to verify the availability of nodes after this step,
                //       as _Reduce() already performs this check.
                while (_Myhead && _Myhead->_Thread.state() == thread_state::waiting && _Count > 0) {
                    _List_node* _Old_head = _Myhead;
                    _Myhead               = _Old_head->_Next;
                    ::mjx::delete_object(_Old_head);
                    --_Count;
                    --_Mysize;
                }

                _List_node* _Current = _Myhead; // skips head
                while (_Current && _Current->_Next && _Count > 0) {
                    if (_Current->_Next->_Thread.state() == thread_state::waiting) {
                        _List_node* _Temp = _Current->_Next;
                        _Current->_Next   = _Current->_Next->_Next; // unlink node
                        ::mjx::delete_object(_Temp);
                        --_Count;
                        --_Mysize;
                    } else {
                        _Current = _Current->_Next;
                    }
                }
            }

            _List_node* _Myhead;
            _List_node* _Mytail;
            size_t _Mysize;
        };
    } // namespace mjsync_impl
} // namespace mjx

#endif // _MJSYNC_IMPL_THREAD_POOL_HPP_