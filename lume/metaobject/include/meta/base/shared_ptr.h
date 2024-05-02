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

#ifndef META_BASE_SHARED_PTR_H
#define META_BASE_SHARED_PTR_H

#include <meta/base/meta_types.h>

#include "shared_ptr_internals.h"

BASE_BEGIN_NAMESPACE()

/**
 * @brief C++ standard like weak_ptr.
 */
template<typename T>
class weak_ptr final : public Internals::PtrCountedBase<T> {
public:
    using element_type = BASE_NS::remove_extent_t<T>;
    weak_ptr() = default;
    ~weak_ptr()
    {
        if (this->control_) {
            this->control_->ReleaseWeak();
        }
    };

    weak_ptr(nullptr_t) {}
    weak_ptr(const shared_ptr<T>& p) : Internals::PtrCountedBase<T>(p)
    {
        if (this->control_) {
            this->control_->AddWeak();
        }
    }
    weak_ptr(const weak_ptr& p) noexcept : Internals::PtrCountedBase<T>(p)
    {
        if (this->control_) {
            this->control_->AddWeak();
        }
    }
    weak_ptr(weak_ptr&& p) noexcept : Internals::PtrCountedBase<T>(p)
    {
        p.InternalReset();
    }

    // NOLINTNEXTLINE(readability-identifier-naming) to keep std like syntax
    shared_ptr<T> lock() const
    {
        return shared_ptr<T>(*this);
    }
    weak_ptr& operator=(weak_ptr&& p) noexcept
    {
        if (this != &p) {
            reset();
            this->control_ = p.control_;
            this->pointer_ = p.pointer_;
            p.InternalReset();
        }
        return *this;
    }
    weak_ptr& operator=(const weak_ptr& p) noexcept
    {
        if (this != &p) {
            reset();
            this->control_ = p.control_;
            this->pointer_ = p.pointer_;
            if (this->control_) {
                this->control_->AddWeak();
            }
        }
        return *this;
    }
    weak_ptr& operator=(const shared_ptr<T>& p)
    {
        reset();
        this->control_ = p.control_;
        this->pointer_ = p.pointer_;
        if (this->control_) {
            this->control_->AddWeak();
        }
        return *this;
    }

    weak_ptr& operator=(nullptr_t) noexcept
    {
        reset();
        return *this;
    }
    // NOLINTNEXTLINE(readability-identifier-naming) to keep std like syntax
    void reset()
    {
        if (this->control_) {
            this->control_->ReleaseWeak();
            this->InternalReset();
        }
    }

    /*"implicit" casting constructors */
    template<class U, class = Internals::EnableIfPointerConvertible<U, T>>
    weak_ptr(const shared_ptr<U>& p)
        // handle casting by using functionality in shared_ptr. (creates an aliased shared_ptr to original.)
        : weak_ptr(shared_ptr<T>(p))
    {}

    template<class U, class = Internals::EnableIfPointerConvertible<U, T>>
    weak_ptr(const weak_ptr<U>& p) : weak_ptr(shared_ptr<T>(p.lock()))
    {}

    /* "implicit" casting move */
    template<class U, class = Internals::EnableIfPointerConvertible<U, T>>
    weak_ptr(weak_ptr<U>&& p) noexcept : weak_ptr(shared_ptr<T>(p.lock()))
    {
        p.reset();
    }

    /* "implicit" casting operators */
    template<class U, class = Internals::EnableIfPointerConvertible<U, T>>
    weak_ptr& operator=(const shared_ptr<U>& p)
    {
        // handle casting by using functionality in shared_ptr. (creates an aliased shared_ptr to original.)
        *this = shared_ptr<T>(p);
        return *this;
    }

    template<class U, class = Internals::EnableIfPointerConvertible<U, T>>
    weak_ptr& operator=(const weak_ptr<U>& p)
    {
        // first lock the given weak ptr. (to see if it has expired, and to get a pointer that can be cast)
        *this = shared_ptr<T>(p.lock());
        return *this;
    }

    // NOLINTNEXTLINE(readability-identifier-naming) to keep std like syntax
    bool expired() const noexcept
    {
        return !this->control_ || this->control_->GetStrongCount() == 0;
    }

private:
    friend class shared_ptr<T>;
    template<typename>
    friend class weak_ptr;
};

/**
 * @brief C++ standard like shared_ptr with IInterface support for reference counting.
 */
template<typename T>
class shared_ptr final : public Internals::PtrCountedBase<T> {
public:
    using element_type = BASE_NS::remove_extent_t<T>;
    using weak_type = weak_ptr<T>;

    constexpr shared_ptr() noexcept = default;
    constexpr shared_ptr(nullptr_t) noexcept {}
    shared_ptr(const shared_ptr& p) noexcept : Internals::PtrCountedBase<T>(p)
    {
        if (this->control_) {
            this->control_->AddStrongCopy();
        }
    }
    shared_ptr(shared_ptr&& p) noexcept : Internals::PtrCountedBase<T>(p)
    {
        p.InternalReset();
    }
    explicit shared_ptr(T* ptr)
    {
        if (ptr) {
            ConstructBlock(ptr);
        }
    }

    template<typename Deleter>
    shared_ptr(T* ptr, Deleter deleter)
    {
        if (ptr) {
            ConstructBlock(ptr, BASE_NS::move(deleter));
        }
    }

    explicit shared_ptr(const weak_type& p) noexcept : Internals::PtrCountedBase<T>(p)
    {
        if (this->control_) {
            if (!this->control_->AddStrongLock()) {
                this->InternalReset();
            }
        }
    }
    template<class Y>
    shared_ptr(const shared_ptr<Y>& r, T* ptr) noexcept : Internals::PtrCountedBase<T>(r.control_)
    {
        if (this->control_ && ptr) {
            this->control_->AddStrongCopy();
            this->pointer_ = const_cast<deletableType*>(ptr);
        } else {
            this->InternalReset();
        }
    }
    template<class U, class = Internals::EnableIfPointerConvertible<U, T>>
    shared_ptr(shared_ptr<U>&& p) noexcept : Internals::PtrCountedBase<T>(p.control_)
    {
        if (this->control_) {
            void* ptr = nullptr;
            if constexpr (BASE_NS::is_same_v<T, const BASE_NS::remove_const_t<U>> ||
                          !META_NS::HasGetInterfaceMethod_v<U>) {
                ptr = p.get();
            } else {
                // make a proper interface cast here.
                if constexpr (BASE_NS::is_const_v<T>) {
                    ptr = const_cast<void*>(static_cast<const void*>(p->GetInterface(T::UID)));
                } else {
                    ptr = static_cast<void*>(p->GetInterface(T::UID));
                }
            }
            if (ptr) {
                this->pointer_ = static_cast<deletableType*>(ptr);
                p.InternalReset();
            } else {
                this->InternalReset();
                p.reset();
            }
        }
    }
    template<class U, class = Internals::EnableIfPointerConvertible<U, T>>
    shared_ptr(const shared_ptr<U>& p) noexcept : shared_ptr(shared_ptr<U>(p)) // use the above move constructor
    {}

    template<class U, class D, class = Internals::EnableIfPointerConvertible<U, T>>
    shared_ptr(unique_ptr<U, D>&& p) noexcept
    {
        if (p) {
            ConstructBlock(p.release(), BASE_NS::move(p.get_deleter()));
        }
    }

    ~shared_ptr()
    {
        if (this->control_) {
            this->control_->Release();
        }
    }
    T* operator->() const noexcept
    {
        return get();
    }
    T& operator*() const noexcept
    {
        return *get();
    }
    explicit operator bool() const
    {
        return get();
    }
    bool operator==(const shared_ptr& other) const noexcept
    {
        return get() == other.get();
    }
    bool operator!=(const shared_ptr& other) const noexcept
    {
        return !(*this == other);
    }
    // NOLINTNEXTLINE(readability-identifier-naming) to keep std like syntax
    void reset()
    {
        if (this->control_) {
            this->control_->Release();
            this->InternalReset();
        }
    }
    // NOLINTNEXTLINE(readability-identifier-naming) to keep std like syntax
    void reset(T* ptr)
    {
        if (ptr != this->pointer_) {
            reset();
            if (ptr) {
                ConstructBlock(ptr);
            }
        }
    }
    template<typename Deleter>
    // NOLINTNEXTLINE(readability-identifier-naming) to keep std like syntax
    void reset(T* ptr, Deleter deleter)
    {
        if (ptr != this->pointer_) {
            reset();
            if (ptr) {
                ConstructBlock(ptr, BASE_NS::move(deleter));
            }
        }
    }

    shared_ptr& operator=(nullptr_t) noexcept
    {
        reset();
        return *this;
    }
    shared_ptr& operator=(const shared_ptr& o)
    {
        if (this != &o) {
            reset();
            this->control_ = o.control_;
            this->pointer_ = o.pointer_;
            if (this->control_) {
                this->control_->AddStrongCopy();
            }
        }
        return *this;
    }
    shared_ptr& operator=(shared_ptr&& o) noexcept
    {
        if (this != &o) {
            reset();
            this->control_ = o.control_;
            this->pointer_ = o.pointer_;
            o.InternalReset();
        }
        return *this;
    }
    // NOLINTNEXTLINE(readability-identifier-naming) to keep std like syntax
    void swap(shared_ptr& p)
    {
        auto tp = p.pointer_;
        auto tc = p.control_;
        p.pointer_ = this->pointer_;
        p.control_ = this->control_;
        this->pointer_ = tp;
        this->control_ = tc;
    }
    // NOLINTNEXTLINE(readability-identifier-naming) to keep std like syntax
    element_type* get() const noexcept
    {
        return this->pointer_;
    }

private:
    using deletableType = BASE_NS::remove_const_t<T>;

    void ConstructBlock(T* ptr)
    {
        static_assert(sizeof(T), "type has to be complete when constructing control block");
        if constexpr (BASE_NS::is_convertible_v<deletableType*, CORE_NS::IInterface*>) {
            this->control_ = new Internals::RefCountedObjectStorageBlock(ptr);
        } else {
            this->control_ = new Internals::StorageBlock(ptr);
        }
        this->pointer_ = ptr;
    }
    template<typename Deleter>
    void ConstructBlock(T* ptr, Deleter deleter)
    {
        this->control_ = new Internals::StorageBlockWithDeleter(ptr, BASE_NS::move(deleter));
        this->pointer_ = ptr;
    }

    template<typename>
    friend class weak_ptr;
    template<typename>
    friend class shared_ptr;
};

BASE_END_NAMESPACE()

// NOLINTBEGIN(readability-identifier-naming) to keep std like syntax
template<class U, class T>
BASE_NS::shared_ptr<U> static_pointer_cast(const BASE_NS::shared_ptr<T>& ptr)
{
    if (ptr) {
        return BASE_NS::shared_ptr<U>(ptr, static_cast<U*>(ptr.get()));
    }
    return {};
}

template<class U, class T>
BASE_NS::shared_ptr<U> interface_pointer_cast(const BASE_NS::shared_ptr<T>& ptr)
{
    static_assert(META_NS::HasGetInterfaceMethod_v<T>, "T::GetInterface not defined");
    static_assert(META_NS::HasGetInterfaceMethod_v<U>, "U::GetInterface not defined");
    if (ptr) {
        if constexpr (BASE_NS::is_same_v<U, T>) {
            // same type.
            return ptr;
        } else {
            return BASE_NS::shared_ptr<U>(ptr, static_cast<U*>(static_cast<void*>(ptr->GetInterface(U::UID))));
        }
    }
    return {};
}

template<class U, class T>
BASE_NS::shared_ptr<const U> interface_pointer_cast(const BASE_NS::shared_ptr<const T>& ptr)
{
    static_assert(META_NS::HasGetInterfaceMethod_v<T>, "T::GetInterface not defined");
    static_assert(META_NS::HasGetInterfaceMethod_v<U>, "U::GetInterface not defined");
    if (ptr) {
        if constexpr (BASE_NS::is_same_v<U, T>) {
            // same type.
            return ptr;
        } else {
            return BASE_NS::shared_ptr<const U>(
                ptr, static_cast<const U*>(static_cast<const void*>(ptr->GetInterface(U::UID))));
        }
    }
    return {};
}

template<class U, class T>
BASE_NS::shared_ptr<U> interface_pointer_cast(const BASE_NS::weak_ptr<T>& weak)
{
    return interface_pointer_cast<U>(weak.lock());
}

template<class U, class T>
BASE_NS::shared_ptr<const U> interface_pointer_cast(const BASE_NS::weak_ptr<const T>& weak)
{
    return interface_pointer_cast<const U>(weak.lock());
}

template<class U, class T>
U* interface_cast(const BASE_NS::shared_ptr<T>& ptr)
{
    static_assert(META_NS::HasGetInterfaceMethod_v<T>, "T::GetInterface not defined");
    static_assert(META_NS::HasGetInterfaceMethod_v<U>, "U::GetInterface not defined");
    if (ptr) {
        if constexpr (BASE_NS::is_same_v<U, T>) {
            // same type.
            return ptr.get();
        } else {
            return static_cast<U*>(static_cast<void*>(ptr->GetInterface(U::UID)));
        }
    }
    return {};
}

template<class U, class T>
const U* interface_cast(const BASE_NS::shared_ptr<const T>& ptr)
{
    static_assert(META_NS::HasGetInterfaceMethod_v<T>, "T::GetInterface not defined");
    static_assert(META_NS::HasGetInterfaceMethod_v<U>, "U::GetInterface not defined");
    if (ptr) {
        if constexpr (BASE_NS::is_same_v<U, T>) {
            // same type.
            return ptr.get();
        } else {
            return static_cast<const U*>(static_cast<const void*>(ptr->GetInterface(U::UID)));
        }
    }
    return {};
}

template<class U>
U* interface_cast(CORE_NS::IInterface* ptr)
{
    static_assert(META_NS::HasGetInterfaceMethod_v<U>, "U::GetInterface not defined");
    if (ptr) {
        return static_cast<U*>(static_cast<void*>(ptr->GetInterface(U::UID)));
    }
    return {};
}

template<class U>
const U* interface_cast(const CORE_NS::IInterface* ptr)
{
    static_assert(META_NS::HasGetInterfaceMethod_v<U>, "U::GetInterface not defined");
    if (ptr) {
        return static_cast<const U*>(static_cast<const void*>(ptr->GetInterface(U::UID)));
    }
    return {};
}
// NOLINTEND(readability-identifier-naming) to keep std like syntax

template<typename T, typename... Args>
BASE_NS::shared_ptr<T> CreateShared(Args&&... args)
{
    return BASE_NS::shared_ptr<T>(new T(BASE_NS::forward<Args>(args)...));
}

META_TYPE(BASE_NS::shared_ptr<const CORE_NS::IInterface>)
META_TYPE(BASE_NS::shared_ptr<CORE_NS::IInterface>)
META_TYPE(BASE_NS::weak_ptr<const CORE_NS::IInterface>)
META_TYPE(BASE_NS::weak_ptr<CORE_NS::IInterface>)

#endif
