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

#ifndef API_BASE_CONTAINERS_SHARED_PTR_INTERNALS_H
#define API_BASE_CONTAINERS_SHARED_PTR_INTERNALS_H

#include <base/containers/atomics.h>
#include <base/containers/type_traits.h>
#include <base/namespace.h>
#include <base/util/log.h>

BASE_BEGIN_NAMESPACE()
namespace Internals {
template<typename U, typename T>
using EnableIfPointerConvertible = BASE_NS::enable_if_t<BASE_NS::is_convertible_v<U*, T*>>;

class ControlBlock {
public:
    ControlBlock() = default;
    virtual ~ControlBlock() = default;

    ControlBlock(const ControlBlock&) = delete;
    ControlBlock& operator=(const ControlBlock&) = delete;
    ControlBlock(ControlBlock&&) = delete;
    ControlBlock& operator=(ControlBlock&&) = delete;

    void AddStrongCopy() noexcept
    {
        BASE_NS::AtomicIncrementRelaxed(&strongCount_);
    }

    void AddWeak() noexcept
    {
        BASE_NS::AtomicIncrementRelaxed(&weakCount_);
    }

    void Release()
    {
        if (BASE_NS::AtomicDecrementRelease(&strongCount_) == 0) {
            BASE_NS::AtomicFenceAcquire();
            Dispose();
            // The strong references are counted as one weak references to keep the control block alive
            ReleaseWeak();
        }
    }

    void ReleaseWeak()
    {
        // The weak count is initialised to 1, so releasing weak pointers does not destroy
        // the control block unless there is no more strong references
        if (BASE_NS::AtomicDecrementRelease(&weakCount_) == 0) {
            BASE_NS::AtomicFenceAcquire();
            Destroy();
        }
    }

    bool AddStrongLock() noexcept
    {
        return BASE_NS::AtomicIncrementIfNotZero(&strongCount_) != 0;
    }

    int32_t GetStrongCount() const noexcept
    {
        return BASE_NS::AtomicReadRelaxed(&strongCount_);
    }
    int32_t GetWeakCount() const noexcept
    {
        return BASE_NS::AtomicReadRelaxed(&weakCount_);
    }

    virtual void Destroy() = 0;
    virtual void Dispose() = 0;

private:
    // Count of the weak references +1 for the strong references
    int32_t weakCount_ { 1 };
    int32_t strongCount_ { 1 };
};

class ptr_base {
protected:
    ptr_base() = default;
    ~ptr_base() = default;
    ptr_base(const ptr_base&) = default;
    ptr_base(ptr_base&&) = default;
    ptr_base& operator=(const ptr_base&) = default;
    ptr_base& operator=(ptr_base&&) = default;
};

template<typename Type>
class PtrCountedBase : public ptr_base {
public:
    // NOLINTNEXTLINE(readability-identifier-naming) to keep std like syntax
    int32_t use_count() const noexcept
    {
        return control_ ? control_->GetStrongCount() : 0;
    }

protected:
    PtrCountedBase() = default;
    PtrCountedBase(ControlBlock* c) : control_(c) {}
    ~PtrCountedBase() = default;
    PtrCountedBase(const PtrCountedBase&) = default;
    PtrCountedBase& operator=(const PtrCountedBase&) = default;
    PtrCountedBase(PtrCountedBase&&) = default;
    PtrCountedBase& operator=(PtrCountedBase&&) = default;

    void InternalReset() noexcept
    {
        control_ = nullptr;
        pointer_ = nullptr;
    }

protected:
    Type* pointer_ {};
    ControlBlock* control_ {};

    template<typename>
    friend class PtrCountedBase;
};

template<typename T>
using DefaultDeleterType = void (*)(T*);

template<typename T>
class StorageBlock final : public ControlBlock {
public:
    explicit StorageBlock(T* ptr) : ptr_ { ptr }
    {
        BASE_ASSERT(ptr != nullptr);
    }

    void Destroy() final
    {
        delete this;
    }

    void Dispose() final
    {
        delete ptr_;
    }

private:
    T* ptr_ { nullptr };
};

template<typename T, typename Deleter = DefaultDeleterType<T>>
class StorageBlockWithDeleter final : public ControlBlock {
public:
    StorageBlockWithDeleter(T* ptr, Deleter deleter) : ptr_ { ptr }, deleter_ { BASE_NS::move(deleter) }
    {
        BASE_ASSERT(ptr != nullptr);
    }

    void Destroy() final
    {
        delete this;
    }

    void Dispose() final
    {
        deleter_(ptr_);
    }

private:
    T* ptr_ { nullptr };
    Deleter deleter_;
};

template<typename T>
class RefCountedObjectStorageBlock final : public ControlBlock {
public:
    using deletableType = BASE_NS::remove_const_t<T>;

    explicit RefCountedObjectStorageBlock(T* ptr) : ptr_ { ptr }
    {
        BASE_ASSERT(ptr != nullptr);
        const_cast<deletableType*>(ptr)->Ref();
    }

    void Destroy() final
    {
        delete this;
    }

    void Dispose() final
    {
        const_cast<deletableType*>(ptr_)->Unref();
    }

private:
    T* ptr_ { nullptr };
};
} // namespace Internals
BASE_END_NAMESPACE()

#endif // API_BASE_CONTAINERS_SHARED_PTR_INTERNALS_H
