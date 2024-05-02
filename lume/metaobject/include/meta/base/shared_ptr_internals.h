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

#ifndef META_BASE_SHARED_PTR_INTERNALS_H
#define META_BASE_SHARED_PTR_INTERNALS_H

#include <base/containers/unique_ptr.h>
#include <base/namespace.h>
#include <core/log.h>

#include <meta/base/atomics.h>
#include <meta/base/interface_macros.h>
#include <meta/base/interface_traits.h>

BASE_BEGIN_NAMESPACE()
namespace Internals {

template<typename U, typename T>
using EnableIfPointerConvertible = BASE_NS::enable_if_t<BASE_NS::is_convertible_v<U*, T*>>;

class ControlBlock {
public:
    ControlBlock() = default;
    virtual ~ControlBlock() = default;
    META_NO_COPY_MOVE(ControlBlock)

    void AddStrongCopy()
    {
        CORE_NS::AtomicIncrement(&strongCount_);
    }

    void AddWeak()
    {
        CORE_NS::AtomicIncrement(&weakCount_);
    }

    void Release()
    {
        if (CORE_NS::AtomicDecrement(&strongCount_) == 0) {
            Dispose();
            // The strong references are counted as one weak references to keep the control block alive
            ReleaseWeak();
        }
    }

    void ReleaseWeak()
    {
        // The weak count is initialised to 1, so releasing weak pointers does not destroy
        // the control block unless there is no more strong references
        if (CORE_NS::AtomicDecrement(&weakCount_) == 0) {
            Destroy();
        }
    }

    bool AddStrongLock()
    {
        return CORE_NS::AtomicIncrementIfNotZero(&strongCount_) != 0;
    }

    int32_t GetStrongCount() const
    {
        return CORE_NS::AtomicRead(&strongCount_);
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
    // for compatibility
    template<typename U>
    bool Compare(const PtrCountedBase<U>& p) const
    {
        return static_cast<const void*>(pointer_) == static_cast<const void*>(p.pointer_);
    }
    template<typename U>
    bool CompareOwner(const PtrCountedBase<U>& p) const
    {
        return control_ == p.control_;
    }

protected:
    PtrCountedBase() = default;
    PtrCountedBase(ControlBlock* c) : control_(c) {}
    ~PtrCountedBase() = default;
    PtrCountedBase(const PtrCountedBase&) = default;
    PtrCountedBase& operator=(const PtrCountedBase&) = default;
    PtrCountedBase(PtrCountedBase&&) = default;
    PtrCountedBase& operator=(PtrCountedBase&&) = default;

    void InternalReset()
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

#endif
