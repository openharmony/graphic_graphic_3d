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

#ifndef API_BASE_CONTAINERS_REFCNT_PTR_H
#define API_BASE_CONTAINERS_REFCNT_PTR_H

#include <base/containers/type_traits.h>
#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
template<class T>
class refcnt_ptr {
public:
    using pointer = BASE_NS::remove_reference_t<T>*;
    using element_type = T;

    constexpr refcnt_ptr() noexcept {}

    constexpr refcnt_ptr(nullptr_t) noexcept {}

    explicit constexpr refcnt_ptr(pointer ptr) noexcept : ptr_(ptr)
    {
        if (ptr) {
            ptr->Ref();
        }
    }

    refcnt_ptr(const refcnt_ptr& ptr) : refcnt_ptr(ptr.get()) {}

    template<class U>
    refcnt_ptr(const refcnt_ptr<U>& ptr) : refcnt_ptr(static_cast<T*>(ptr.get()))
    {}

    refcnt_ptr(refcnt_ptr&& ptr) noexcept : ptr_(exchange(ptr.ptr_, nullptr)) {}

    template<class U>
    refcnt_ptr(refcnt_ptr<U>&& ptr) noexcept : ptr_(static_cast<T*>(ptr.release()))
    {}

    ~refcnt_ptr()
    {
        if (ptr_) {
            ptr_->Unref();
        }
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
        if (ptr_ != ptr) {
            pointer old_ptr = ptr_;
            ptr_ = ptr;
            if (ptr_) {
                ptr_->Ref();
            }
            if (old_ptr) {
                old_ptr->Unref();
            }
        }
    }

    refcnt_ptr& operator=(nullptr_t) noexcept
    {
        reset();
        return *this;
    }

    refcnt_ptr& operator=(const refcnt_ptr& r) noexcept
    {
        reset(r.get());
        return *this;
    }

    template<class U>
    refcnt_ptr& operator=(const refcnt_ptr<U>& r) noexcept
    {
        reset(r.get());
        return *this;
    }

    refcnt_ptr& operator=(refcnt_ptr&& r) noexcept
    {
        reset();
        ptr_ = exchange(r.ptr_, nullptr);
        return *this;
    }

    template<class U>
    refcnt_ptr& operator=(refcnt_ptr<U>&& r) noexcept
    {
        reset();
        ptr_ = r.release();
        return *this;
    }

    void swap(refcnt_ptr& other) noexcept
    {
        ptr_ = exchange(other.ptr_, ptr_);
    }

    explicit operator bool() const noexcept
    {
        return (ptr_ != nullptr);
    }

    bool operator==(const refcnt_ptr& other) const noexcept
    {
        return (ptr_ == other.ptr_);
    }

    bool operator!=(const refcnt_ptr& other) const noexcept
    {
        return (ptr_ != other.ptr_);
    }

    pointer operator->() const noexcept
    {
        return ptr_;
    }

    typename BASE_NS::add_lvalue_reference<T>::type operator*() const
    {
        return *ptr_;
    }

    bool operator!=(nullptr_t) const noexcept
    {
        return ptr_ != nullptr;
    }

    bool operator==(nullptr_t) const noexcept
    {
        return ptr_ == nullptr;
    }

protected:
    pointer ptr_ { nullptr };
};
BASE_END_NAMESPACE()

#endif // API_BASE_CONTAINERS_REFCNT_PTR_H
