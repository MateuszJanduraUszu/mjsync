// thread_pool.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjsync/impl/thread_pool.hpp>
#include <mjsync/impl/utils.hpp>
#include <mjsync/thread_pool.hpp>

namespace mjx {
    thread_pool::thread_pool() noexcept : _Mylist(nullptr), _Mystate(_Closed) {}

    thread_pool::thread_pool(thread_pool&& _Other) noexcept
        : _Mylist(_Other._Mylist.release()), _Mystate(_Other._Mystate) {
        _Other._Mystate = _Closed;
    }

    thread_pool::thread_pool(const size_t _Count) : _Mylist(nullptr), _Mystate(_Closed) {
        if (_Count > 0) { // requested non-empty pool, re-initialize
            _Mylist.reset(::mjx::create_object<mjsync_impl::_Thread_list>(_Count));
            _Mystate = _Working;
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
        if (_Mystate == _Waiting) { // all threads are waiting, choose the one with the fewest pending tasks
            return _Mylist->_Select_thread_with_fewest_pending_tasks();
        } else {
            thread* const _Thread = _Mylist->_Select_any_waiting_thread();
            if (_Thread) { // waiting thread found, select it
                return _Thread;
            } else { // no thread is waiting, choose the thread with the fewest pending tasks
                return _Mylist->_Select_thread_with_fewest_pending_tasks();
            }
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

    void thread_pool::close() noexcept {
        if (_Mystate != _Closed) {
            _Mystate = _Closed;
            _Mylist.reset();
        }
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

    void thread_pool::increase_thread_count(const size_t _Count) {
        if (_Mystate != _Closed) {
            _Mylist->_Grow(_Count);
        }
    }

    void thread_pool::decrease_thread_count(const size_t _Count) noexcept {
        if (_Mystate != _Closed) {
            if (_Count >= _Mylist->_Size()) { // remove all threads
                _Mylist->_Clear();
                _Mystate = _Closed;
            } else { // remove some threads
                _Mylist->_Reduce(_Count);
            }
        }
    }

    size_t thread_pool::thread_count() const noexcept {
        return _Mylist ? _Mylist->_Size() : 0;
    }

    void thread_pool::thread_count(const size_t _New_count) {
        if (_Mystate != _Closed) {
            const size_t _Count = _Mylist->_Size();
            if (_New_count > _Count) { // increase the number of threads
                increase_thread_count(_New_count - _Count);
            } else if (_New_count < _Count) { // decrease the number of threads
                decrease_thread_count(_Count - _New_count);
            }
        }
    }

    task thread_pool::schedule_task(
        const thread::callable _Callable, void* const _Arg, const task_priority _Priority) {
        if (_Mystate == _Closed) { // scheduling inactive
            return task{};
        }

        thread* const _Thread = _Select_ideal_thread();
        return _Thread ? _Thread->schedule_task(_Callable, _Arg, _Priority) : task{};
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
} // namespace mjx