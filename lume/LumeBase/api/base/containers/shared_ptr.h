/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef API_BASE_CONTAINERS_SHARED_PTR_H
#define API_BASE_CONTAINERS_SHARED_PTR_H

#include <base/containers/shared_ptr_internals.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/namespace.h>

CORE_BEGIN_NAMESPACE()
class IInterface;
CORE_END_NAMESPACE()

BASE_BEGIN_NAMESPACE()
template<typename T>
class shared_ptr;
template<typename T>
class weak_ptr;

/**
 * @brief C++ standard like weak_ptr.
 */
template<typename T>
class weak_ptr final : public Internals::PtrCountedBase<T> {
public:
    using element_type = BASE_NS::remove_extent_t<T>;
    constexpr weak_ptr() noexcept = default;
    ~weak_ptr()
    {
        if (this->control_) {
            this->control_->ReleaseWeak();
        }
    }

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
        return this->use_count() == 0;
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
                          !BASE_NS::is_base_of_v<CORE_NS::IInterface, T>) {
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
    explicit operator bool() const noexcept
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
    shared_ptr& operator=(const shared_ptr& o) noexcept
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
    void swap(shared_ptr& p) noexcept
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

    // NOLINTNEXTLINE(readability-identifier-naming) to keep std like syntax
    int32_t weak_count() const noexcept
    {
        if (!this->control_) {
            return 0;
        }
        int32_t c = this->control_->GetWeakCount();
        if (c > 0) {
            // strong references are counted as single weak reference for the internal bookkeeping.
            --c;
        }
        return c;
    }

private:
    using deletableType = BASE_NS::remove_const_t<T>;

    void ConstructBlock(T* ptr)
    {
        static_assert(sizeof(T), "type has to be complete when constructing control block");
        if constexpr (BASE_NS::is_base_of_v<CORE_NS::IInterface, T>) {
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

template<typename T, typename... Args>
BASE_NS::shared_ptr<T> make_shared(Args&&... args)
{
    return BASE_NS::shared_ptr<T>(new T(BASE_NS::forward<Args>(args)...));
}
BASE_END_NAMESPACE()

// NOLINTNEXTLINE(readability-identifier-naming) to keep std like syntax
template<class U, class T>
BASE_NS::shared_ptr<U> static_pointer_cast(const BASE_NS::shared_ptr<T>& ptr)
{
    if (ptr) {
        return BASE_NS::shared_ptr<U>(ptr, static_cast<U*>(ptr.get()));
    }
    return {};
}

template<typename T, typename... Args>
BASE_NS::shared_ptr<T> CreateShared(Args&&... args)
{
    return BASE_NS::make_shared<T>(BASE_NS::forward<Args>(args)...);
}
#endif // API_BASE_CONTAINERS_SHARED_PTR_H
