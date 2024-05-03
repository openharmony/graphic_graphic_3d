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

#ifndef API_BASE_CONTAINERS_UNIQUE_PTR_H
#define API_BASE_CONTAINERS_UNIQUE_PTR_H

#include <cstddef>

#include <base/containers/type_traits.h>
#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
template<class T>
struct default_delete {
    constexpr default_delete() noexcept = default;

    template<class U>
    default_delete(const default_delete<U>& d) noexcept
    {}

    void operator()(T* ptr) const
    {
        static_assert(sizeof(T), "can't delete an incomplete type");
        delete ptr;
    }

    template<class U>
    void operator()(U* ptr) const
    {
        static_assert(sizeof(U), "can't delete an incomplete type");
        delete ptr;
    }
};

template<class T>
struct default_delete<T[]> {
    constexpr default_delete() noexcept = default;

    template<class U>
    default_delete(const default_delete<U[]>& d) noexcept
    {}

    void operator()(T* ptr) const
    {
        static_assert(sizeof(T), "can't delete an incomplete type");
        delete[] ptr;
    }

    template<class U>
    void operator()(U* ptr) const
    {
        static_assert(sizeof(U), "can't delete an incomplete type");
        delete[] ptr;
    }
};

template<class T, class D = default_delete<T>>
class unique_ptr {
public:
    using pointer = BASE_NS::remove_reference_t<T>*;
    using element_type = T;
    using deleter_type = D;

    constexpr unique_ptr() noexcept {};

    constexpr unique_ptr(nullptr_t) noexcept {}

    explicit unique_ptr(pointer p) noexcept : ptr_(p) {}

    // D is non-reference
    template<class dt = D, enable_if_t<!is_reference_v<dt>, int> = 0>
    unique_ptr(pointer p, const D& deleter) noexcept : ptr_(p), deleter_(BASE_NS::forward<decltype(deleter)>(deleter))
    {}
    template<class dt = D, enable_if_t<!is_reference_v<dt>, int> = 0>
    unique_ptr(pointer p, BASE_NS::remove_reference_t<D>&& deleter) noexcept
        : ptr_(p), deleter_(BASE_NS::forward<decltype(deleter)>(deleter))
    {}

    // D is lvalue-reference "A&"
    template<class d = D, class dt = BASE_NS::remove_reference_t<D>,
        BASE_NS::enable_if_t<(BASE_NS::is_lvalue_reference_v<d> && !BASE_NS::is_const_v<dt>), int> = 0>
    unique_ptr(pointer p, D& deleter) noexcept : ptr_(p), deleter_(BASE_NS::forward<decltype(deleter)>(deleter))
    {}
    template<class d = D, class dt = BASE_NS::remove_reference_t<D>,
        BASE_NS::enable_if_t<(BASE_NS::is_lvalue_reference_v<d> && !BASE_NS::is_const_v<dt>), int> = 0>
    unique_ptr(pointer p, BASE_NS::remove_reference_t<D>&& deleter) = delete;

    // D is lvalue-reference "const A&"
    template<class d = D, class dt = BASE_NS::remove_reference_t<D>,
        BASE_NS::enable_if_t<(BASE_NS::is_lvalue_reference_v<d> && BASE_NS::is_const_v<dt>), int> = 0>
    unique_ptr(pointer p, const D& deleter) noexcept : ptr_(p), deleter_(BASE_NS::forward<decltype(deleter)>(deleter))
    {}
    template<class d = D, class dt = BASE_NS::remove_reference_t<D>,
        BASE_NS::enable_if_t<(BASE_NS::is_lvalue_reference_v<d> && BASE_NS::is_const_v<dt>), int> = 0>
    unique_ptr(pointer p, const BASE_NS::remove_reference_t<D>&& deleter) = delete;

    // if E is a reference type, this deleter is copy constructed from u's deleter
    template<class U, class E, enable_if_t<!is_array_v<U> && is_reference_v<E>, int> = 0>
    unique_ptr(unique_ptr<U, E>&& u) noexcept : ptr_(u.release()), deleter_(u.get_deleter())
    {}

    // if E is a non-reference type, this D is move constructed from u's D
    template<class U, class E, enable_if_t<!is_array_v<U> && !is_reference_v<E>, int> = 0>
    unique_ptr(unique_ptr<U, E>&& u) noexcept : ptr_(u.release()), deleter_(BASE_NS::move(u.get_deleter()))
    {}

    ~unique_ptr()
    {
        if (ptr_) {
            deleter_(ptr_);
        }
    }

    deleter_type& get_deleter() noexcept
    {
        return deleter_;
    }
    const deleter_type& get_deleter() const noexcept
    {
        return deleter_;
    }

    pointer get() const noexcept
    {
        return ptr_;
    }

    pointer release() noexcept
    {
        pointer res = ptr_;
        ptr_ = nullptr;
        return res;
    }

    void reset(pointer ptr = pointer()) noexcept
    {
        pointer old_ptr = ptr_;
        ptr_ = ptr;
        if (old_ptr) {
            deleter_(old_ptr);
        }
    }

    unique_ptr& operator=(nullptr_t) noexcept
    {
        reset();
        return *this;
    }

    unique_ptr& operator=(unique_ptr&& r) noexcept
    {
        reset(r.release());
        deleter_ = r.get_deleter();
        return *this;
    }

    template<class U, class E>
    unique_ptr& operator=(unique_ptr<U, E>&& r) noexcept
    {
        reset(r.release());
        deleter_ = r.get_deleter();
        return *this;
    }

    void swap(unique_ptr& other) noexcept
    {
        pointer tmp = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = tmp;
        auto tmp2 = deleter_;
        deleter_ = other.deleter_;
        other.deleter_ = tmp2;
    }

    explicit operator bool() const noexcept
    {
        return (ptr_ != nullptr);
    }

    pointer operator->() const noexcept
    {
        return ptr_;
    }

    typename BASE_NS::add_lvalue_reference<T>::type operator*() const
    {
        return *ptr_;
    }

    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;

protected:
    pointer ptr_ { nullptr };
    D deleter_;
};

template<class T, class D>
class unique_ptr<T[], D> {
public:
    using pointer = BASE_NS::remove_reference_t<T>*;
    using element_type = T;
    using deleter_type = D;

    constexpr unique_ptr() noexcept {}

    constexpr unique_ptr(nullptr_t) noexcept {}

    explicit unique_ptr(pointer p) noexcept : ptr_(p) {}

    template<class U>
    explicit unique_ptr(U p) noexcept : ptr_(p)
    {}

    // D is non-reference
    template<class dt = D, enable_if_t<!is_reference_v<dt>, int> = 0>
    unique_ptr(pointer p, const D& d) noexcept : ptr_(p), deleter_(BASE_NS::forward<decltype(d)>(d))
    {}
    template<class dt = D, enable_if_t<!is_reference_v<dt>, int> = 0>
    unique_ptr(pointer p, BASE_NS::remove_reference_t<D>&& d) noexcept
        : ptr_(p), deleter_(BASE_NS::forward<decltype(d)>(d))
    {}

    // D is lvalue-reference "A&"
    template<class d = D, class dt = BASE_NS::remove_reference_t<D>,
        BASE_NS::enable_if_t<(BASE_NS::is_lvalue_reference_v<d> && !BASE_NS::is_const_v<dt>), int> = 0>
    unique_ptr(pointer p, D& deleter) noexcept : ptr_(p), deleter_(BASE_NS::forward<decltype(deleter)>(deleter))
    {}
    template<class d = D, class dt = BASE_NS::remove_reference_t<D>,
        BASE_NS::enable_if_t<(BASE_NS::is_lvalue_reference_v<d> && !BASE_NS::is_const_v<dt>), int> = 0>
    unique_ptr(pointer p, BASE_NS::remove_reference_t<D>&& deleter) = delete;

    // D is lvalue-reference "const A&"
    template<class d = D, class dt = BASE_NS::remove_reference_t<D>,
        BASE_NS::enable_if_t<(BASE_NS::is_lvalue_reference_v<d> && BASE_NS::is_const_v<dt>), int> = 0>
    unique_ptr(pointer p, const D& deleter) noexcept : ptr_(p), deleter_(BASE_NS::forward<decltype(deleter)>(deleter))
    {}
    template<class d = D, class dt = BASE_NS::remove_reference_t<D>,
        BASE_NS::enable_if_t<(BASE_NS::is_lvalue_reference_v<d> && BASE_NS::is_const_v<dt>), int> = 0>
    unique_ptr(pointer p, const BASE_NS::remove_reference_t<D>&& deleter) = delete;

    // if E is a reference type, this deleter is copy constructed from u's deleter
    template<class U, class E, enable_if_t<is_array_v<U> && is_reference_v<E>, int> = 0>
    unique_ptr(unique_ptr<U, E>&& u) noexcept : ptr_(u.release()), deleter_(u.get_deleter())
    {}

    // if E is a non-reference type, this D is move constructed from u's D
    template<class U, class E, enable_if_t<is_array_v<U> && !is_reference_v<E>, int> = 0>
    unique_ptr(unique_ptr<U, E>&& u) noexcept : ptr_(u.release()), deleter_(BASE_NS::move(u.get_deleter()))
    {}

    ~unique_ptr()
    {
        if (ptr_) {
            deleter_(ptr_);
        }
    }

    deleter_type& get_deleter() noexcept
    {
        return deleter_;
    }

    const deleter_type& get_deleter() const noexcept
    {
        return deleter_;
    }

    pointer get() const noexcept
    {
        return ptr_;
    }

    pointer release() noexcept
    {
        pointer res = ptr_;
        ptr_ = nullptr;
        return res;
    }

    template<class U>
    void reset(U ptr) noexcept
    {
        pointer old_ptr = ptr_;
        ptr_ = ptr;
        if (old_ptr) {
            deleter_(old_ptr);
        }
    }

    void reset(nullptr_t p = nullptr) noexcept
    {
        reset(pointer());
    }

    unique_ptr& operator=(nullptr_t) noexcept
    {
        reset();
        return *this;
    }

    unique_ptr& operator=(unique_ptr&& r) noexcept
    {
        reset(r.release());
        deleter_ = r.get_deleter();
        return *this;
    }

    template<class U, class E>

    unique_ptr& operator=(unique_ptr<U, E>&& r) noexcept
    {
        reset(r.release());
        deleter_ = r.get_deleter();
        return *this;
    }

    void swap(unique_ptr& other) noexcept
    {
        pointer tmp = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = tmp;
        auto tmp2 = deleter_;
        deleter_ = other.deleter_;
        other.deleter_ = tmp2;
    }

    explicit operator bool() const noexcept
    {
        return (ptr_ != nullptr);
    }

    T& operator[](size_t i) const
    {
        return ptr_[i];
    }

    unique_ptr(const unique_ptr&) = delete;

    unique_ptr& operator=(const unique_ptr&) = delete;

protected:
    pointer ptr_ { nullptr };
    D deleter_;
};

// equality comparisons
template<class T1, class D1, class T2, class D2>
bool operator==(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
{
    return x.get() == y.get();
}
template<class T1, class D1>
bool operator==(const unique_ptr<T1, D1>& x, nullptr_t)
{
    return x.get() == nullptr;
}
template<class T1, class D1>
bool operator==(nullptr_t, const unique_ptr<T1, D1>& x)
{
    return x.get() == nullptr;
}

// in-equality comparisons
template<class T1, class D1, class T2, class D2>
bool operator!=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
{
    return x.get() != y.get();
}
template<class T1, class D1>
bool operator!=(const unique_ptr<T1, D1>& x, nullptr_t)
{
    return x.get() != nullptr;
}
template<class T1, class D1>
bool operator!=(nullptr_t, const unique_ptr<T1, D1>& x)
{
    return x.get() != nullptr;
}

// non-array types
template<class T, class... Args, BASE_NS::enable_if_t<!BASE_NS::is_array_v<T>, int> = 0>
unique_ptr<T> make_unique(Args&&... args)
{
    return unique_ptr<T>(new T(BASE_NS::forward<Args>(args)...));
}

// arrays with unknown bound
template<class T, BASE_NS::enable_if_t<BASE_NS::is_array_v<T> && BASE_NS::extent_v<T> == 0, int> = 0>
unique_ptr<T> make_unique(size_t size)
{
    return unique_ptr<T>(new typename BASE_NS::remove_extent_t<T>[size]());
}

// arrays with known bound. (not-allowed)
template<class T, class... Args, BASE_NS::enable_if_t<BASE_NS::is_array_v<T> && BASE_NS::extent_v<T> != 0, int> = 0>
void make_unique(Args&&... args) = delete;
BASE_END_NAMESPACE()

#endif // API_BASE_CONTAINERS_UNIQUE_PTR_H