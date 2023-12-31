// task.cpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <mjsync/impl/task.hpp>
#include <mjsync/task.hpp>
#include <type_traits>

namespace mjx {
    task::task() noexcept : _Myid(invalid_id), _Mythrd(nullptr) {}

    task::task(task&& _Other) noexcept : _Myid(_Other._Myid), _Mythrd(_Other._Mythrd) {
        _Other._Myid   = invalid_id;
        _Other._Mythrd = nullptr;
    }

    task::task(const id _Id, thread* const _Thread) noexcept : _Myid(_Id), _Mythrd(_Thread) {}

    task::~task() noexcept {}

    task& task::operator=(task&& _Other) noexcept {
        if (this != ::std::addressof(_Other)) {
            _Myid          = _Other._Myid;
            _Mythrd        = _Other._Mythrd;
            _Other._Myid   = invalid_id;
            _Other._Mythrd = nullptr;
        }

        return *this;
    }

    bool task::is_registered() const noexcept {
        return _Myid != invalid_id && _Mythrd != nullptr;
    }

    task::id task::get_id() const noexcept {
        return _Myid;
    }

    task_state task::state() const noexcept {
        if (!is_registered()) { // task not registered, break
            return task_state::none;
        }

        mjsync_impl::_Task_proxy _Proxy(_Myid, _Mythrd->_Myimpl.get());
        return _Proxy._Get_state();
    }

    task_priority task::priority() const noexcept {
        if (!is_registered()) { // task not registered, break
            return task_priority::none;
        }

        mjsync_impl::_Task_proxy _Proxy(_Myid, _Mythrd->_Myimpl.get());
        return _Proxy._Get_priority();
    }

    task::cancellation_result task::cancel() noexcept {
        if (!is_registered()) { // task not registered, break
            return cancellation_result::task_not_registered;
        }

        mjsync_impl::_Task_proxy _Proxy(_Myid, _Mythrd->_Myimpl.get());
        return _Proxy._Cancel();
    }

    void task::wait_until_done() noexcept {
        if (is_registered()) { // task registered, wait if possible
            mjsync_impl::_Task_proxy _Proxy(_Myid, _Mythrd->_Myimpl.get());
            _Proxy._Wait_until_done();
        }
    }
} // namespace mjx