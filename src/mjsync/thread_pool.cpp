// thread_pool.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjsync/details/thread_pool.hpp>
#include <mjsync/details/utils.hpp>
#include <mjsync/thread_pool.hpp>

namespace mjsync {
    thread_pool::thread_pool() noexcept : _Mystate(_Closed), _Mylist(nullptr) {}

    thread_pool::thread_pool(thread_pool&& _Other) noexcept
        : _Mystate(_Other._Mystate), _Mylist(_Other._Mylist.release()) {
        _Other._Mystate = _Closed;
    }

    thread_pool::thread_pool(const size_t _Count) noexcept : _Mystate(_Count > 0 ? _Working : _Closed),
        _Mylist(_Count > 0 ? ::std::make_unique<details::_Thread_list>(_Count) : nullptr) {
        if (_Mystate != _Closed && !_Mylist) { // failed to create the thread list, mark failure
            _Mystate = _Closed;
        }
    }

    thread_pool::~thread_pool() noexcept {
        close();
    }

    thread_pool& thread_pool::operator=(thread_pool&& _Other) noexcept {
        if (this != ::std::addressof(_Other)) {
            _Mylist.reset(_Other._Mylist.release());
            _Mystate        = _Other._Mystate;
            _Other._Mystate = _Closed;
        }

        return *this;
    }

    thread* thread_pool::_Select_ideal_thread() noexcept {
        switch (_Mystate) {
        case _Waiting: // all threads are waiting, choose the one with the fewest pending tasks
            return _Mylist->_Select_thread_with_fewest_pending_tasks();
        case _Working:
        {
            thread* const _Thread = _Mylist->_Select_any_waiting_thread();
            if (_Thread) { // some thread is waiting for a task, select it
                return _Thread;
            } else { // no thread is waiting, choose the thread with the fewest pending tasks
                return _Mylist->_Select_thread_with_fewest_pending_tasks();
            }
        }
        default:
            details::_Unreachable();
            return nullptr;
        }
    }
    
    bool thread_pool::is_open() const noexcept {
        return _Mystate != _Closed;
    }

    bool thread_pool::is_waiting() const noexcept {
        return _Mystate == _Waiting;
    }

    bool thread_pool::is_working() const noexcept {
        return _Mystate == _Working;
    }

    size_t thread_pool::thread_count() const noexcept {
        return _Mylist ? _Mylist->_Size() : 0;
    }

    void thread_pool::close() noexcept {
        _Mystate = _Closed;
        _Mylist.reset();
    }

    bool thread_pool::is_thread_in_pool(const thread::id _Id) const noexcept {
        return _Mylist ? _Mylist->_Is_thread_present(_Id) : false;
    }

    thread_pool::statistics thread_pool::collect_statistics() const noexcept {
        if (_Mystate == _Closed) {
            return statistics{};
        }

        statistics _Result;
        _Mylist->_For_each_thread(
            [&_Result](thread& _Thread) noexcept {
                _Result.pending_tasks += _Thread.pending_tasks();
                if (_Thread.state() == thread_state::waiting) {
                    ++_Result.waiting_threads;
                } else {
                    ++_Result.working_threads;
                }
            }
        );
        return _Result;
    }

    void thread_pool::cancel_all_pending_tasks() noexcept {
        if (_Mystate != _Closed) { // must not be closed
            _Mylist->_For_each_thread(
                [](thread& _Thread) noexcept {
                    _Thread.cancel_all_pending_tasks();
                }
            );
        }
    }

    bool thread_pool::increase_threads(const size_t _Count) noexcept {
        return _Mystate != _Closed ? _Mylist->_Grow(_Count) : false;
    }

    bool thread_pool::decrease_threads(const size_t _Count) noexcept {
        if (_Mystate == _Closed) {
            return false;
        }

        const size_t _Size = _Mylist->_Size();
        if (_Count > _Size) { // not enough threads
            return false;
        } else if (_Count == _Size) { // remove all threads and close the thread-pool
            close();
            return true;
        } else {
            return _Mylist->_Reduce(_Count);
        }
    }

    bool thread_pool::set_thread_count(const size_t _New_count) noexcept {
        if (_Mystate == _Closed) {
            return false;
        }

        if (_New_count == 0) { // close the thread-pool
            close();
            return true;
        }

        const size_t _Size = _Mylist->_Size();
        if (_New_count > _Size) { // increase the number of threads
            return _Mylist->_Grow(_New_count - _Size);
        } else { // decrease the number of threads
            return _Mylist->_Reduce(_Size - _New_count);
        }
    }

    bool thread_pool::schedule_task(
        const thread::task _Task, void* const _Data, const task_priority _Priority) noexcept {
        if (_Mystate == _Closed) { // scheduling inactive
            return false;
        }

        thread* const _Thread = _Select_ideal_thread();
        return _Thread ? _Thread->schedule_task(_Task, _Data, _Priority) : false;
    }

    bool thread_pool::suspend() noexcept {
        if (_Mystate != _Working) { // must be working
            return false;
        }

        _Mystate      = _Waiting;
        bool _Success = true;
        _Mylist->_For_each_thread( // suspend as many threads, as possible
            [&_Success](thread& _Thread) noexcept {
                if (!_Thread.suspend()) {
                    _Success = false;
                }
            }
        );
        return _Success;
    }

    bool thread_pool::resume() noexcept {
        if (_Mystate != _Waiting) { // must be waiting
            return false;
        }

        _Mystate      = _Working;
        bool _Success = true;
        _Mylist->_For_each_thread( // resume as many threads, as possible
            [&_Success](thread& _Thread) noexcept {
                if (!_Thread.resume()) {
                    _Success = false;
                }
            }
        );
        return _Success;
    }
} // namespace mjsync