// object_allocator.hpp

// Copyright (c) Mateusz Jandura. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef _MJMEM_OBJECT_ALLOCATOR_HPP_
#define _MJMEM_OBJECT_ALLOCATOR_HPP_
#include <mjmem/allocator.hpp>
#include <new>
#include <type_traits>

namespace mjx {
    template <class _Ty>
    class object_allocator { // type-specific wrapper around the global allocator
    public:
        static_assert(!::std::is_const_v<_Ty>, "T cannot be const");
        static_assert(!::std::is_reference_v<_Ty>, "T cannot be a reference");
        static_assert(!::std::is_function_v<_Ty>, "T cannot be a function object");
        static_assert(!::std::is_volatile_v<_Ty>, "T cannot be volatile");

        using value_type      = _Ty;
        using size_type       = allocator::size_type;
        using difference_type = allocator::difference_type;
        using pointer         = _Ty*;
        using const_pointer   = const _Ty*;
        using reference       = _Ty&;
        using const_reference = const _Ty&;

        template <class _Other>
        struct rebind {
            using other = object_allocator<_Other>;
        };

        object_allocator() noexcept                        = default;
        object_allocator(const object_allocator&) noexcept = default;
        object_allocator(object_allocator&&) noexcept      = default;
        ~object_allocator() noexcept                       = default;
    
        template <class _Other>
        object_allocator(const object_allocator<_Other>&) noexcept {}

        object_allocator& operator=(const object_allocator&) noexcept = default;
        object_allocator& operator=(object_allocator&&) noexcept      = default;

        template <class _Other>
        object_allocator& operator=(const object_allocator<_Other>&) noexcept {
            return *this;
        }

        pointer allocate(const size_type _Count) {
            return static_cast<pointer>(::mjx::get_allocator().allocate(_Count * sizeof(_Ty)));
        }

        pointer allocate_aligned(const size_type _Count, const size_type _Align) {
            return static_cast<pointer>(::mjx::get_allocator().allocate_aligned(_Count * sizeof(_Ty), _Align));
        }

        void deallocate(pointer _Ptr, const size_type _Count) noexcept {
            ::mjx::get_allocator().deallocate(_Ptr, _Count * sizeof(_Ty));
        }

        size_type max_size() const noexcept {
            return ::mjx::get_allocator().max_size() / sizeof(_Ty);
        }

        bool is_equal(const object_allocator&) const noexcept {
            return true; // always equal
        }
    };

    template <class _Ty>
    inline bool operator==(const object_allocator<_Ty>&, const object_allocator<_Ty>&) noexcept {
        return true;
    }

    template <class _Ty, class... _Types>
    inline _Ty* create_object(_Types&&... _Args) {
        object_allocator<_Ty> _Al;
        return ::new (static_cast<void*>(_Al.allocate(1))) _Ty(::std::forward<_Types>(_Args)...);
    }

    template <class _Ty>
    inline void delete_object(_Ty* const _Obj) noexcept(::std::is_nothrow_destructible_v<_Ty>) {
        if (_Obj) {
            if constexpr (!::std::is_trivially_destructible_v<_Ty>) {
                _Obj->~_Ty(); // non-trivial destructor, call it
            }

            object_allocator<_Ty> _Al;
            _Al.deallocate(_Obj, 1);
        }
    }

    template <class _Ty>
    inline _Ty* allocate_object_array(const size_t _Count) {
        object_allocator<_Ty> _Al;
        return _Al.allocate(_Count);
    }

    template <class _Ty>
    inline void delete_object_array(
        _Ty* const _Objects, const size_t _Count) noexcept(::std::is_nothrow_destructible_v<_Ty>) {
        if (_Objects && _Count > 0) {
            if constexpr (!::std::is_trivially_destructible_v<_Ty>) {
                for (size_t _Idx = 0; _Idx < _Count; ++_Idx) {
                    _Objects[_Idx].~_Ty(); // non-trivial destructor, call it
                }
            }

            object_allocator<_Ty> _Al;
            _Al.deallocate(_Objects, _Count);
        }
    }

    template <class _Alloc>
    concept compatible_allocator = ::std::is_base_of_v<allocator, _Alloc>;

    template <class _Ty, compatible_allocator _Alloc, class... _Types>
    inline _Ty* create_object_using_allocator(_Alloc& _Al, _Types&&... _Args) {
        return ::new (static_cast<void*>(_Al.allocate(sizeof(_Ty)))) _Ty(::std::forward<_Types>(_Args)...);
    }

    template <class _Ty, compatible_allocator _Alloc>
    inline void delete_object_using_allocator(
        _Ty* const _Obj, _Alloc& _Al) noexcept(::std::is_nothrow_destructible_v<_Ty>) {
        if (_Obj) {
            if constexpr (!::std::is_trivially_destructible_v<_Ty>) {
                _Obj->~_Ty(); // non-trivial destructor, call it
            }

            _Al.deallocate(_Obj, sizeof(_Ty));
        }
    }

    template <class _Ty, compatible_allocator _Alloc>
    inline _Ty* allocate_object_array_using_allocator(const size_t _Count, _Alloc& _Al) {
        return static_cast<_Ty*>(_Al.allocate(_Count * sizeof(_Ty)));
    }

    template <class _Ty, compatible_allocator _Alloc>
    inline void delete_object_array_using_allocator(
        _Ty* const _Objects, const size_t _Count, _Alloc& _Al) noexcept(::std::is_nothrow_destructible_v<_Ty>) {
        if (_Objects && _Count > 0) {
            if constexpr (!::std::is_trivially_destructible_v<_Ty>) {
                for (size_t _Idx = 0; _Idx < _Count; ++_Idx) {
                    _Objects[_Idx].~_Ty(); // non-trivial destructor, call it
                }
            }

            _Al.deallocate(_Objects, _Count * sizeof(_Ty));
        }
    }
} // namespace mjx

#endif // _MJMEM_OBJECT_ALLOCATOR_HPP_