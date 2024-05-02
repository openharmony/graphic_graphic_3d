/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef API_BASE_CONTAINERS_VECTOR_H
#define API_BASE_CONTAINERS_VECTOR_H

/*
IMPLEMENTATION NOTES:
Iterators are not compliant with c++17. They are just minimal effort implementations to make everything work.
Not all std::vector methods have been implemented yet.

Not handling alignment properly. (ie. expecting that malloc alignment is enough, and object granularity is enough. ie.
sizeof(value_type) is enough to align to next object) Exceptions are not handled (In engine exceptions are disabled
anyway) use of operator& (which could cause issues.. see std::address_of and operator& overloading)

some noexcepts are missing, and also constexprs...
allocators are partially done.
   - can't specify allocator in all constructors
   - allocator propagation rules are in a flux.
     (there are cases where we want the allocator to be copied/moved along with data,
      and then there are cases where we DONT want it)
*/
// Allow support for initializer lists
#define BASE_VECTOR_HAS_INITIALIZE_LIST

#include <cassert>
#include <cstddef> // size_t, ptrdiff_t
#include <cstdint> // uint8_t

#ifdef STANDALONE_BASE_VECTOR
// STANDALONE has no dependencies to other engine parts.
// Use the compiler/system defined initializer_list and new headers. (which do pollute the namespaces alot)
#define USE_NEW_AND_INITIALIZE_LIST_HEADERS

#ifdef BASE_VECTOR_HAS_INITIALIZE_LIST
#ifdef USE_NEW_AND_INITIALIZE_LIST_HEADERS
// Due to a mistake in c++ standards (in my opinion), we need to include these headers to get placement new and
// initializer_list support.
#include <initializer_list> // std::initializer list
#include <new>              // placement new
#else
// Fully declaring the placement new and std::intializer_list ourselves works, and pulls no extra headers.
void* operator new(size_t size, void* ptr) noexcept
{
    return ptr;
}
void* operator new[](size_t size, void* ptr) noexcept
{
    return ptr;
}
namespace std {
template<class E>
class initializer_list {
public:
    using value_type = E;
    using reference = const E&;
    using const_reference = const E&;
    using size_type = size_t;
    using iterator = const E*;
    using const_iterator = const E*;
    constexpr initializer_list() noexcept : _First(nullptr), _Last(nullptr) {}

    constexpr initializer_list(const E* _First_arg, const E* _Last_arg) noexcept : _First(_First_arg), _Last(_Last_arg)
    {}

    // number of elements
    constexpr size_t size() const noexcept
    {
        return (static_cast<size_t>(_Last - _First));
    }

    // first element
    constexpr const E* begin() const noexcept
    {
        return _First;
    }

    // one past the last element
    constexpr const E* end() const noexcept
    {
        return _Last;
    }

private:
    const E *_First, *_Last;
};
// initializer list range access
template<class E>
constexpr const E* begin(initializer_list<E> il) noexcept
{
    return (il.begin());
}

template<class E>
constexpr const E* end(initializer_list<E> il) noexcept
{
    return (il.end());
}
} // namespace std
#endif
#endif
#endif

#include <base/containers/allocator.h>
#include <base/containers/iterator.h>
#include <base/containers/type_traits.h>
#include <base/namespace.h>
#include <base/util/log.h>

BASE_BEGIN_NAMESPACE()
namespace {
// C++ Standard library exception guarantees have been relaxed (due to not allowing exceptions)
// conditionally partially implemented (one case only)
// SO NO OPERATION SHOULD EVER THROW AN EXCEPTION!
// If EXCEPTION_GUARANTEE == 1 use copy constructor instead of move if move is not noexcept..
static constexpr bool EXCEPTION_GUARANTEE = false;

// Enable/Disable memory poisoning. (could be useful for debug purposes)
static constexpr bool BASE_CONTAINERS_ENABLE_POISON = false;
static constexpr const uint8_t POISON = 0xCD;
} // namespace

template<typename T>
class vector {
public:
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    using size_type = allocator::size_type;
    using const_reference = const value_type&;
    using const_pointer = const value_type*;

    using iterator = iterator<vector<T>>;
    using const_iterator = const_iterator<vector<T>>;
    using reverse_iterator = reverse_iterator<iterator>;
    using const_reverse_iterator = BASE_NS::reverse_iterator<const_iterator>;

    void setAllocator(allocator& alloc)
    {
        // Check if the allocators are different.
        if (&alloc != &allocator_.get()) {
            // check if vector has data allocated.
            if ((size_ == 0) && (capacity_ == 0) && (data_ == 0)) {
                // No data so just change the allocator
                allocator_ = alloc;
            } else {
                // Vector contains data, so create a copy with new allocator
                vector<T> temp(*this, alloc);
                // And swap the vectors.
                swap(temp);
            }
        }
    }

    allocator& getAllocator()
    {
        return allocator_.get();
    }

    const allocator& getAllocator() const
    {
        return allocator_.get();
    }

    explicit vector(allocator& alloc) noexcept : allocator_(alloc) {}

    vector() noexcept : allocator_(default_allocator()) {}

    vector(size_t size) : allocator_(default_allocator())
    {
        resize(size);
    }

    vector(size_t size, const_reference value) : allocator_(default_allocator())
    {
        resize(size, value);
    }

    vector(const vector& other) : allocator_(other.allocator_)
    {
        if (!other.empty()) {
            reserve(other.size());
            init_copy(data_, other.data_, other.size());
            size_ = other.size_;
        }
    }

    vector(const vector& other, allocator& alloc) : allocator_(alloc)
    {
        if (!other.empty()) {
            reserve(other.size());
            init_copy(data_, other.data_, other.size());
            size_ = other.size_;
        }
    }

    vector(vector&& other) noexcept
        : size_(other.size_), capacity_(other.capacity_), data_(other.data_), allocator_(other.allocator_)
    {
        /*
        NOTE: keep the allocator even during moves..
        Check this at a later point "other.allocator_ = { nullptr,nullptr,nullptr };"
        */
        other.size_ = 0;
        other.capacity_ = 0;
        other.data_ = nullptr;
    }

    template<class InputIt,
        enable_if_t<is_same<remove_const_t<typename InputIt::value_type>, value_type>::value, int> = 0>
    vector(InputIt first, InputIt last) : allocator_(default_allocator())
    {
        const auto size = static_cast<size_type>(last - first);
        reserve(size);
        init_copy(data_, first, last);
        size_ = size;
    }

    vector(const_pointer first, const_pointer last) : allocator_(default_allocator())
    {
        const auto size = static_cast<size_type>(last - first);
        reserve(size);
        init_copy(data_, first, size);
        size_ = size;
    }

#ifdef BASE_VECTOR_HAS_INITIALIZE_LIST
    // The only way to get initializer_lists is to use std::initializer_list.
    // Also initializer_lists are bad since they cause copies. please avoid them.
    vector(std::initializer_list<T> init) : allocator_(default_allocator())
    {
        const size_t size = init.size();
        reserve(size);
        init_copy(data_, init.begin(), size);
        size_ = size;
    }

    vector& operator=(std::initializer_list<T> ilist)
    {
        const size_t size = ilist.size();
        allocator_ = default_allocator();
        clear();
        reserve(size);
        init_copy(data_, ilist.begin(), size);
        size_ = size;
        return *this;
    }
#endif

    ~vector()
    {
        destroy(begin(), end());
        allocator_.free(data_);
    }

    vector& operator=(const vector& other)
    {
        if (&other != this) {
            /* NOTE: check how STD handles the allocator propagation (there IS a flag
            "std::allocator_traits<allocator_type>::propagate_on_container_swap::value") and see if it's usable for us?
            or do we want different semantics.. Check this at a later point "other.allocator_ = {
            nullptr,nullptr,nullptr };" DO NOT USE THE ALLOCATOR FROM other!!!
            */
            if (other.size_ > capacity_) {
                // need to create new storage.
                clear();
                reserve(other.size_);
                init_copy(data_, other.data_, other.size_);
            } else {
                // use existing storage
                size_t tocopy = size_; // can copy to this many slots.
                size_t tocreate = other.size_;
                size_t todestroy = 0;
                if (size_ > other.size_) {
                    todestroy = size_ - other.size_;
                }
                if (tocopy > tocreate) {
                    tocopy = tocreate;
                }
                tocreate = tocreate - tocopy;
                const_pointer src = other.data_;
                pointer dst = data_;
                if (tocopy > 0) {
                    // first copy to the initialized objects..
                    copy(dst, src, src + tocopy);
                    dst += tocopy;
                    src += tocopy;
                }
                if (tocreate > 0) {
                    // then create the new ones..
                    init_copy(dst, src, src + tocreate);
                    dst += tocreate;
                }
                if (todestroy > 0) {
                    // destroy the remaining objects.
                    destroy(iterator(dst), iterator(dst + todestroy));
                }
            }
            size_ = other.size_;
        }
        return *this;
    }

    vector& operator=(vector&& other) noexcept
    {
        if (&other != this) {
            if (data_) {
                // destroy all old objects.
                destroy(begin(), end());
                // Free old memory..
                allocator_.free(data_);
                data_ = nullptr;
                size_ = 0;
                capacity_ = 0;
            }
            allocator_ = other.allocator_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            data_ = other.data_;
            /* NOTE: keep the allocator even during moves..
            Check this at a later point "other.allocator_ = { nullptr,nullptr,nullptr };"
            */
            other.size_ = 0;
            other.capacity_ = 0;
            other.data_ = nullptr;
        }
        return *this;
    }

    // Iterators
    iterator begin()
    {
        return iterator(data_);
    }

    iterator end()
    {
        return iterator(data_ + size_);
    }

    const_iterator begin() const
    {
        return const_iterator(data_);
    }

    const_iterator end() const
    {
        return const_iterator(data_ + size_);
    }

    const_iterator cbegin() const noexcept
    {
        return begin();
    }

    const_iterator cend() const noexcept
    {
        return end();
    }

    reverse_iterator rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }

    const_reverse_iterator crbegin() const noexcept
    {
        return rbegin();
    }

    const_reverse_iterator crend() const noexcept
    {
        return rend();
    }

    // Capacity
    size_type size() const
    {
        return size_;
    }

    size_type max_size() const noexcept
    {
        return size_type(-1);
    }

    size_type capacity() const noexcept
    {
        return capacity_;
    }

    bool empty() const noexcept
    {
        return size_ == 0;
    }

    void reserve(size_type count)
    {
        finalize(setup_storage(count), size_);
    }

    void resize(size_type count)
    {
        // destroy the objects that we don't need anymore
        if (size_ > count) {
            destroy(begin() + static_cast<difference_type>(count), end());
            size_ = count;
        }
        pointer tmp = setup_storage(count);
        BASE_ASSERT(capacity_ >= count);
        // default initialize any new objects.
        if (count > size_) {
            init_value(tmp + size_, count - size_);
        }
        finalize(tmp, size_);
        size_ = count;
    }

    void resize(size_t count, const_reference value)
    {
        // destroy the objects that we don't need anymore
        if (size_ > count) {
            destroy(begin() + static_cast<difference_type>(count), end());
            size_ = count;
        }
        pointer tmp = setup_storage(count);
        BASE_ASSERT(capacity_ >= count);
        // copy initialize any new objects.
        if (count > size_) {
            init_fill(tmp + size_, count - size_, value);
        }
        finalize(tmp, size_);
        size_ = count;
    }

    void shrink_to_fit()
    {
        if (size_ != capacity_) {
            pointer tmp = (pointer)allocator_.alloc(size_ * sizeof(value_type));
            finalize(tmp, size_);
            capacity_ = size_;
        }
    }

    // Element access
    reference at(size_type index)
    {
        // If index out-of-range, undefined behaviour
        return data_[index];
    }

    const_reference at(size_type index) const
    {
        // If index out-of-range, undefined behaviour
        return data_[index];
    }

    reference operator[](size_type index)
    {
        // If index out-of-range, undefined behaviour
        return data_[index];
    }

    const_reference operator[](size_type index) const
    {
        // If index out-of-range, undefined behaviour
        return data_[index];
    }

    reference front()
    {
        // If container is empty, undefined behaviour
        return *begin();
    }

    const_reference front() const
    {
        // If container is empty, undefined behaviour
        return *begin();
    }

    reference back()
    {
        // If container is empty, undefined behaviour
        return *(--end());
    }

    const_reference back() const
    {
        // If container is empty, undefined behaviour
        return *(--end());
    }

    const_pointer data() const noexcept
    {
        return data_;
    }

    pointer data() noexcept
    {
        return data_;
    }

    // Modifiers
    void swap(vector& other) noexcept
    {
        // NOTE: check how STD handles the allocator propagation (there IS a flag
        // "std::allocator_traits<allocator_type>::propagate_on_container_swap::value") and see if it's usable for us?
        // or do we want different semantics..
        // swap allocators
        const auto ta = allocator_;
        allocator_ = other.allocator_;
        other.allocator_ = ta;

        const auto ts = size_;
        size_ = other.size_;
        other.size_ = ts;

        const auto tc = capacity_;
        capacity_ = other.capacity_;
        other.capacity_ = tc;

        const auto td = data_;
        data_ = other.data_;
        other.data_ = td;
    }

    iterator insert(const_iterator pos, T&& value)
    {
        difference_type p = pos - cbegin();
        pointer tmp = allocate_if_needed(size_ + 1U);
        pointer res = nullptr;
        if (data_ != tmp) {
            pointer begin = data_;
            pointer insrt = begin + p;
            pointer end = begin + size_;
            res = init_move(tmp, begin, static_cast<size_type>(p)); // move old objects from before pos
            pointer next = init_move(res, &value, 1);
            init_move(next, insrt, size_ - p);       // move old objects from after pos
            destroy(iterator(begin), iterator(end)); // Destroy the moved from items..
            // free old storage
            allocator_.free(data_);
            data_ = tmp;
        } else {
            res = ((pointer)pos.ptr());
            if (cend() == pos) {
                init_move(res, &value, 1);
            } else {
                reverse_move(end().ptr() - 1, res, end().ptr());
                *res = BASE_NS::move(value); // move in the new item.
            }
        }
        size_++;
        return iterator(res);
    }

    iterator insert(const_iterator pos, const T& value)
    {
        difference_type p = pos - cbegin();
        pointer tmp = allocate_if_needed(size_ + 1U);
        pointer res = nullptr;
        if (tmp != data_) {
            pointer begin = data_;
            pointer insrt = begin + p;
            pointer end = begin + size_;
            // Use new storage.
            res = init_move(tmp, begin, static_cast<size_type>(p)); // move old objects from before pos
            pointer next = init_copy(res, &value, 1);
            init_move(next, insrt, size_ - p);       // move old objects from after pos
            destroy(iterator(begin), iterator(end)); // Destroy the moved from items..
            // free old storage
            allocator_.free(data_);
            data_ = tmp;
        } else {
            res = ((pointer)pos.ptr());
            if (cend() == pos) {
                init_copy(res, &value, 1);
            } else {
                reverse_move(end().ptr() - 1, res, end().ptr());
                *res = value; // copy in the new item. (could inplace initialize?)
            }
        }
        size_++;
        return iterator(res);
    }

    iterator insert(const_iterator pos, size_type count, const T& value)
    {
        // NOTE: do this properly
        if (count == 0) {
            return iterator((pointer)pos.ptr());
        }
        difference_type p = pos - cbegin();
        pointer tmp = allocate_if_needed(size_ + count);
        pointer res = nullptr;
        if (tmp != data_) {
            pointer begin = data_;
            pointer insrt = begin + p;
            pointer end = begin + size_;
            // Use new storage.
            res = init_move(tmp, begin, static_cast<size_type>(p)); // move old objects from before pos
            pointer next = init_fill(res, count, value);
            init_move(next, insrt, size_ - p);       // move old objects from after pos
            destroy(iterator(begin), iterator(end)); // Destroy the moved from items..
            // free old storage
            allocator_.free(data_);
            data_ = tmp;
        } else {
            res = ((pointer)pos.ptr());
            if (cend() != pos) {
                reverse_move(end().ptr() - 1, res, end().ptr() + count - 1);
            }
            init_fill(res, count, value);
        }
        size_ += count;
        return iterator(res);
    }

    iterator insert(const_iterator pos, const_iterator first, const_iterator last)
    {
        return insert_impl(pos, first, last);
    }

    iterator insert(const_iterator pos, iterator first, iterator last)
    {
        return insert_impl(pos, first, last);
    }

    template<class InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last)
    {
        return insert_impl(pos, first, last);
    }

    void append(size_type count, const T& value)
    {
        if (count == 0) {
            return;
        }
        pointer tmp = allocate_if_needed(static_cast<size_type>(size_ + count));
        if (tmp != data_) {
            pointer begin = data_;
            size_type size = size_;
            init_move(tmp, begin, size);                      // Move old objects
            destroy(iterator(begin), iterator(begin + size)); // Destroy the moved from items..
            // Free old storage
            allocator_.free(data_);
            data_ = tmp;
        }
        init_fill(tmp + size_, count, value);
        size_ += count;
    }

    template<class InputIt,
        enable_if_t<is_same<remove_const_t<typename InputIt::value_type>, value_type>::value, int> = 0>
    void append(InputIt first, InputIt last)
    {
        if (first == last) {
            return;
        }
        difference_type cnt = last - first;
        pointer tmp = allocate_if_needed(static_cast<size_type>(size_ + cnt));
        if (tmp != data_) {
            pointer begin = data_;
            size_type size = size_;
            init_move(tmp, begin, size);                      // Move old objects
            destroy(iterator(begin), iterator(begin + size)); // Destroy the moved from items..
            // Free old storage
            allocator_.free(data_);
            data_ = tmp;
        }
        init_copy(tmp + size_, first, last); // Copy the new objects
        size_ += cnt;
    }

    void append(const_pointer first, const_pointer last)
    {
        if (first == last) {
            return;
        }
        difference_type cnt = last - first;
        pointer tmp = allocate_if_needed(static_cast<size_type>(size_ + cnt));
        if (tmp != data_) {
            pointer begin = data_;
            size_type size = size_;
            init_move(tmp, begin, size);                      // Move old objects
            destroy(iterator(begin), iterator(begin + size)); // Destroy the moved from items..
            // Free old storage
            allocator_.free(data_);
            data_ = tmp;
        }
        init_copy(tmp + size_, first, cnt); // Copy the new objects
        size_ += cnt;
    }

    iterator erase(const_iterator pos)
    {
        BASE_ASSERT(pos >= cbegin());
        BASE_ASSERT(pos <= cend());
        // todo: validate.
        if (pos == cend()) {
            return end();
        }
        return erase(pos, pos + 1);
    }

    iterator erase(const_iterator first, const_iterator last)
    {
        BASE_ASSERT(first >= cbegin());
        BASE_ASSERT(first <= cend());
        BASE_ASSERT(last >= cbegin());
        BASE_ASSERT(last <= cend());
        if (first == last) {
            // handle the zero range erase..
            if (last == cend()) {
                return end();
            }
        } else {
            // move from [last -> end] to first
            if (last < cend()) {
                move((pointer)last.ptr(), end().ptr(), (pointer)first.ptr());
            }

            // destroy the leftovers.
            const auto newSize = static_cast<difference_type>(size_) - (last - first);
            destroy(begin() + newSize, end());
            size_ = static_cast<size_type>(newSize);
        }
        return iterator((pointer)first.ptr());
    }

    template<class... Args>
    iterator emplace(iterator pos, Args&&... args)
    {
        const difference_type p = pos - begin();
        pointer tmp = allocate_if_needed(size_ + 1U);
        pointer res = nullptr;
        if (tmp != data_) {
            pointer bgin = begin().ptr();
            pointer insrt = pos.ptr();
            pointer ed = end().ptr();
            // Use new storage.
            res = init_move(tmp, bgin, static_cast<size_type>(p)); // move old objects from before pos
            pointer next = res + 1;
            ::new (res) T(forward<Args>(args)...);
            init_move(next, insrt, size_ - p);     // move old objects from after pos
            destroy(iterator(bgin), iterator(ed)); // Destroy the moved from items..
            // free old storage
            allocator_.free(data_);
            data_ = tmp;
        } else {
            // move objects after pos..
            res = pos.ptr();
            if (pos != end()) {
                reverse_move(end().ptr() - 1, res, end().ptr());
                destroy_at(pos);
            }
            ::new (res) T(forward<Args>(args)...);
        }
        size_++;
        return iterator(res);
    }

    template<class... Args>
    reference emplace_back(Args&&... args)
    {
        pointer tmp = allocate_if_needed(size_ + 1U);
        pointer p = tmp + size_;
        ::new (p) T(forward<Args>(args)...);
        finalize(tmp, size_);
        size_++;
        return *p;
    }

    void push_back(const value_type& val)
    {
        pointer tmp = allocate_if_needed(size_ + 1U);
        init_copy(tmp + size_, &val, 1);
        finalize(tmp, size_);
        size_++;
    }

    void push_back(value_type&& val)
    {
        pointer tmp = allocate_if_needed(size_ + 1U);
        init_move(tmp + size_, &val, 1);
        finalize(tmp, size_);
        size_++;
    }

    void pop_back()
    {
        if (size_ > 0) {
            destroy_at(end() - 1);
            size_--;
        }
    }

    void clear()
    {
        destroy(begin(), end()); // destroy old objects
        size_ = 0;
    }

    size_type size_in_bytes() const
    {
        return size_ * sizeof(value_type);
    }

    size_type capacity_in_bytes() const
    {
        return capacity_ * sizeof(value_type);
    }

protected:
    // Helpers for uninitialized memory.
    void uninitialized_default_construct(pointer first, pointer last)
    {
        if (first == last) {
            return;
        }
        BASE_ASSERT(last > first);
        for (; first < last; ++first) {
            ::new (reinterpret_cast<void*>(first)) T;
        }
    }

    void uninitialized_value_construct(pointer first, pointer last)
    {
        if (first == last) {
            return;
        }
        BASE_ASSERT(last > first);
        for (; first < last; ++first) {
            ::new (reinterpret_cast<void*>(first)) T();
        }
    }

    void uninitialized_copy(const_pointer first, const_pointer last, pointer d_first)
    {
        if (first == last) {
            return;
        }
        BASE_ASSERT(last > first);
        // use copy constructor.
        for (; first < last; ++first, ++d_first) {
            ::new (reinterpret_cast<void*>(d_first)) T(*first);
        }
    }

    void uninitialized_fill(pointer first, const_pointer last, const_reference value)
    {
        if (first == last) {
            return;
        }
        BASE_ASSERT(last > first);
        // use copy constructor.
        for (; first < last; ++first) {
            ::new (first) T(value);
        }
    }

    void uninitialized_move(pointer first, const_pointer last, pointer d_first)
    {
        if (first == last) {
            return;
        }
        BASE_ASSERT(last > first);
        // use move constructor...
        for (; first < last; ++first, ++d_first) {
            ::new (d_first) T(BASE_NS::move(*first));
        }
    }

    // helpers for initialized memory
    // helper for range insert which may or may not need to convert values
    template<class InputIt>
    void copy(pointer pos, InputIt first, InputIt last)
    {
        if (first == last) {
            return;
        }
        BASE_ASSERT(last > first);
        using TypeToInsert = BASE_NS::remove_const_t<BASE_NS::remove_reference_t<decltype(*first)>>;
        constexpr auto is_trivially_copy_constructable =
            __is_trivially_constructible(value_type, add_lvalue_reference_t<const TypeToInsert>);
        constexpr auto matching_type = BASE_NS::is_same_v<TypeToInsert, value_type>;
        constexpr auto is_random_access =
            has_iterator_category<InputIt>::value &&
            BASE_NS::is_same_v<typename has_iterator_category<InputIt>::category, BASE_NS::random_access_iterator_tag>;
        if constexpr (BASE_NS::is_pointer_v<InputIt> && matching_type && is_trivially_copy_constructable) {
            const difference_type count = last - first;
            CloneData(pos, count * sizeof(value_type), first, count * sizeof(TypeToInsert));
        } else if constexpr (is_random_access && has_ptr_method<InputIt>::value && matching_type &&
                             is_trivially_copy_constructable) {
            const difference_type count = last.ptr() - first.ptr();
            CloneData(pos, count * sizeof(value_type), first.ptr(), count * sizeof(TypeToInsert));
        } else {
            constexpr auto convertible_type =
                BASE_NS::is_convertible_v<BASE_NS::remove_const_t<BASE_NS::remove_reference_t<decltype(*first)>>,
                    value_type>;
            static_assert(matching_type || convertible_type, "Invalid input type");
            for (; first < last; ++first, ++pos) {
                if constexpr (matching_type) {
                    // Same type so, no need to convert first.
                    *pos = *first;
                } else if constexpr (convertible_type) {
                    *pos = value_type(*first);
                }
            }
        }
    }

    void move(pointer first, const_pointer last, pointer d_first) // last>first
    {
        if (first == last) {
            return;
        }
        BASE_ASSERT(last > first);
        for (; first < last; ++first, ++d_first) {
            *d_first = BASE_NS::move(*first);
        }
    }

    // same as move but goes backwards. (first>=last) NOTE: range is inclusive so not the same as others.. fix later
    void reverse_move(pointer first, pointer last, pointer dst)
    {
        BASE_ASSERT(first >= last);
        const auto endPtr = end().ptr();
        for (; first >= last; first--, dst--) {
            if (dst >= endPtr) {
                // uninitialized storage, use move initialize.
                init_move(dst, first, 1);
            } else {
                // Hit initialized storage, continue with move.
                break;
            }
        }
        // now move assign the rest..
        for (; first >= last; first--, dst--) {
            *dst = BASE_NS::move(*first);
        }
    }

    void destroy_at(iterator pos)
    {
        pointer c = pos.ptr();
        if constexpr (!__is_trivially_destructible(value_type)) {
            c->~T();
        }
        if constexpr (BASE_CONTAINERS_ENABLE_POISON) {
            // Poison the memory.
            ClearToValue(c, sizeof(value_type), POISON, sizeof(value_type));
        }
    }

    void destroy(iterator first, iterator last)
    {
        if (first == last) {
            return;
        }
        if constexpr (!__is_trivially_destructible(value_type)) {
            pointer fPtr = first.ptr();
            pointer lPtr = last.ptr();
            for (pointer c = fPtr; c != lPtr; ++c) {
                c->~T();
                if constexpr (BASE_CONTAINERS_ENABLE_POISON) {
                    // Poison the memory.
                    ClearToValue(c, sizeof(value_type), POISON, sizeof(value_type));
                }
            }
        } else if constexpr (BASE_CONTAINERS_ENABLE_POISON) {
            // Poison the memory.
            pointer fPtr = first.ptr();
            pointer lPtr = last.ptr();
            size_t todo = (uintptr_t)lPtr - (uintptr_t)fPtr;
            ClearToValue(fPtr, todo, POISON, todo);
        }
    }

    // Helpers for unitialized memory that select correct method based on T (value_type)
    // Does memcpy if possible then move construct or finally a copy construct..
    pointer init_move(pointer dst, pointer src, size_type size)
    {
        if (size == 0) {
            return dst;
        }
        constexpr bool triviallyMovable = __is_trivially_constructible(value_type, value_type);
        constexpr bool noExceptMove = EXCEPTION_GUARANTEE && __is_nothrow_constructible(value_type, value_type);
        constexpr bool exceptMove = !EXCEPTION_GUARANTEE && __is_constructible(value_type, value_type);
        constexpr bool copyable = __is_constructible(value_type, add_lvalue_reference_t<const value_type>);
        constexpr bool noCopyExceptMove = EXCEPTION_GUARANTEE && __is_constructible(value_type, value_type);
        if constexpr (triviallyMovable) {
            // trivial move is valid.
            CloneData(dst, size * sizeof(value_type), src, size * sizeof(value_type));
        } else if constexpr (noExceptMove || exceptMove) {
            // move constructor...
            uninitialized_move(src, src + size, dst);
        } else if constexpr (copyable) { // is_copy_constructable (ie. can construct from "const value_type&")
            // copy constructor..
            uninitialized_copy(src, src + size, dst);
        } else if constexpr (noCopyExceptMove) { // is_move_constructable
            // use move constructor even if it could throw.. (no copy constructor)
            uninitialized_move(src, src + size, dst);
        } else {
            // Neither move nor copy is possible, but fail like std::vector.
            uninitialized_copy(src, src + size, dst);
        }
        return dst + size;
    }

    pointer init_value(pointer dst, size_t count)
    {
        if (count == 0) {
            return dst;
        }
        // NOTE: verify correctness. (__is_trivially_constructable might not be enough)
        if constexpr (!__is_trivially_constructible(value_type)) {
            uninitialized_value_construct(dst, dst + count);
        } else {
            // trivial value initializing.. (ZERO)
            const size_t size = count * sizeof(value_type);
            ClearToValue(dst, size, 0, size);
        }
        return dst + count;
    }

    pointer init_fill(pointer dst, size_t count, const_reference value)
    {
        if (count == 0) {
            return dst;
        }
        if constexpr (!__is_trivially_constructible(value_type)) {
            uninitialized_fill(dst, dst + count, value);
        } else {
            for (pointer p = dst; p != dst + count; p++) {
                CloneData(p, sizeof(value_type), &value, sizeof(value_type));
            }
        }
        return dst + count;
    }

    // Does a memcpy if possible otherwise uses copy constructor.
    pointer init_copy(pointer dst, const_pointer src, size_type size)
    {
        if (size == 0) {
            return dst;
        }
        if constexpr (__is_trivially_constructible(value_type, add_lvalue_reference_t<const value_type>)) {
            // is_trivially_copy_constructable
            CloneData(dst, size * sizeof(value_type), src, size * sizeof(value_type));
        } else if constexpr (__is_constructible(value_type, add_lvalue_reference_t<const value_type>)) {
            // is_copy_constructable
            uninitialized_copy(src, src + size, dst);
        } else {
            // FAIL..
            // fall back to copy initialize.. (which should fail to compile since no copy constructor, just like
            // std:vector)
            uninitialized_copy(src, src + size, dst);
        }
        return dst + size;
    }

    // helper for range insert which may or may not need to convert values
    template<class InputIt>
    pointer init_copy(pointer pos, InputIt first, InputIt last)
    {
        BASE_ASSERT(last >= first);
        constexpr auto matching_type =
            BASE_NS::is_same_v<BASE_NS::remove_const_t<BASE_NS::remove_reference_t<decltype(*first)>>, value_type>;
        constexpr auto is_random_access =
            has_iterator_category<InputIt>::value &&
            BASE_NS::is_same_v<typename has_iterator_category<InputIt>::category, BASE_NS::random_access_iterator_tag>;
        if constexpr (BASE_NS::is_pointer_v<InputIt> && matching_type) {
            const difference_type cnt = last - first;
            return init_copy(pos, first, static_cast<size_type>(cnt));
        } else if constexpr (is_random_access && has_ptr_method<InputIt>::value && matching_type) {
            const difference_type cnt = last.ptr() - first.ptr();
            return init_copy(pos, first.ptr(), static_cast<size_type>(cnt));
        } else {
            constexpr auto convertible_type =
                BASE_NS::is_convertible_v<BASE_NS::remove_const_t<BASE_NS::remove_reference_t<decltype(*first)>>,
                    value_type>;
            static_assert(matching_type || convertible_type, "Invalid input type");
            for (InputIt cur = first; cur != last; ++cur) {
                if constexpr (matching_type) {
                    // Same type so, no need to convert first.
                    init_copy(pos, &*cur, 1U);
                } else if constexpr (convertible_type) {
                    value_type tmp = static_cast<value_type>(*cur);
                    init_copy(pos, &tmp, 1U);
                }
                ++pos;
            }
            return pos;
        }
    }

    pointer allocate_if_needed(size_t newSize)
    {
        if (newSize > capacity_) {
            size_type newCapacity = capacity_ * 2u; // double the capacity
            if (newCapacity < newSize) {
                newCapacity = newSize;
            }
            return setup_storage(newCapacity);
        }
        return data_;
    }

    // helpers to handle raw storage.
    pointer setup_storage(size_t count)
    {
        pointer tmp = data_;
        if (count > capacity_) { // if requested capacity is greater than the current one..
            // .. allocate new storage.
            tmp = (pointer)allocator_.alloc(count * sizeof(value_type));
            BASE_ASSERT_MSG(tmp, "vector memory allocation failed.");
            // note: we update the capacity here, even though the actual storage has not been changed yet.
            capacity_ = count;
        }
        return tmp;
    }

    // move constructs first "todo" objects from data_ to tmp and makes tmp the new data_.
    void finalize(pointer tmp, size_t todo)
    {
        // Move old items to new storage.
        if (tmp != data_) {
            if (tmp && todo > 0) {
                // Move old items to new storage.
                init_move(tmp, data_, todo);
                destroy(begin(), begin() + static_cast<difference_type>(todo)); // Destroy the moved from items..
            }
            // free old storage
            allocator_.free(data_);
            // Use new storage.
            data_ = tmp;
        }
    }

    template<class InputIt>
    iterator insert_impl(const_iterator pos, InputIt first, InputIt last)
    {
        if (first == last) {
            return iterator((pointer)pos.ptr());
        }
        pointer res = nullptr;

        difference_type cnt = last - first;
        pointer tmp = allocate_if_needed(static_cast<size_type>(size_ + cnt));
        if (tmp != data_) {
            difference_type p = pos - cbegin();
            pointer begin = data_;
            pointer insrt = begin + p;
            pointer end = begin + size_;
            // Fill new storage
            res = init_move(tmp, begin, static_cast<size_type>(p));    // move old objects from before pos
            pointer next = init_copy(res, first, last);                // copy new objects
            init_move(next, insrt, static_cast<size_type>(size_ - p)); // move old objects from after pos
            destroy(iterator(begin), iterator(end));                   // Destroy the moved from items..
            // Free old storage
            allocator_.free(data_);
            data_ = tmp;
        } else {
            res = ((pointer)pos.ptr());
            if (cend() == pos) {
                res = init_copy(res, first, last);
            } else {
                pointer end = data_ + size_;
                reverse_move(end - 1, res, end + (cnt - 1));
                // copy over the existing items.
                const difference_type intializedSlots = end - res;
                const auto cnt2 = (intializedSlots > cnt) ? cnt : intializedSlots;
                copy(res, first, first + cnt2);
                // init-copy over the uninitialized ones..
                init_copy(res + cnt2, first + cnt2, last);
            }
        }
        size_ += cnt;

        return iterator(res);
    }

    template<typename Iterator, typename = void>
    struct has_iterator_category : BASE_NS::false_type {
        using category = void;
    };

    template<typename Iterator>
    struct has_iterator_category<Iterator, void_t<typename Iterator::iterator_category>> : BASE_NS::true_type {
        using category = typename Iterator::iterator_category;
    };

    template<typename Iterator>
    using ptr_fn = decltype(BASE_NS::declval<Iterator>().ptr());

    template<typename Iterator, typename = void>
    struct has_ptr_method : BASE_NS::false_type {};

    template<typename Iterator>
    struct has_ptr_method<Iterator, BASE_NS::void_t<ptr_fn<Iterator>>> : BASE_NS::true_type {};

    size_type size_ {}, capacity_ {};
    pointer data_ {};

    // Wrapper to create a "re-seatable" reference.
    class Wrapper {
    public:
        inline Wrapper(allocator& a) : allocator_(&a) {}

        inline Wrapper& operator=(allocator& a)
        {
            allocator_ = &a;
            return *this;
        }

        inline void* alloc(size_type size)
        {
            if ((allocator_) && (allocator_->alloc)) {
                return allocator_->alloc(allocator_->instance, size);
            }
            return nullptr;
        }

        inline void free(void* ptr)
        {
            if ((allocator_) && (allocator_->free)) {
                allocator_->free(allocator_->instance, ptr);
            }
        }

        allocator& get()
        {
            BASE_ASSERT(allocator_ != nullptr);
            return *allocator_;
        }

        const allocator& get() const
        {
            BASE_ASSERT(allocator_ != nullptr);
            return *allocator_;
        }

    private:
        allocator* allocator_ { nullptr };
    } allocator_;
};
BASE_END_NAMESPACE()

#endif // API_BASE_CONTAINERS_VECTOR_H
