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
#ifndef META_API_LOCKING_H
#define META_API_LOCKING_H

#include <meta/interface/intf_lockable.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Helper class to acquire write lock and release it end of the scope.
 */
class InterfaceLock {
public:
    META_NO_COPY(InterfaceLock)

    explicit InterfaceLock(LockType type, const CORE_NS::IInterface* t) : type_(type), p_(interface_cast<ILockable>(t))
    {
        if (p_) {
            if (type_ == LockType::UNIQUE_LOCK) {
                p_->Lock();
            } else if (type_ == LockType::SHARED_LOCK) {
                p_->LockShared();
            }
        }
    }
    explicit InterfaceLock(LockType type, const CORE_NS::IInterface& t) : InterfaceLock(type, &t) {}
    template<typename Type>
    explicit InterfaceLock(LockType type, const BASE_NS::shared_ptr<Type>& t) : InterfaceLock(type, t.get())
    {}

    InterfaceLock(InterfaceLock&& l) noexcept : type_(l.type_), p_(l.p_)
    {
        l.p_ = nullptr;
    }

    InterfaceLock& operator=(InterfaceLock&& l) noexcept
    {
        type_ = l.type_;
        p_ = l.p_;
        l.p_ = nullptr;
        return *this;
    }

    ~InterfaceLock()
    {
        if (p_) {
            if (type_ == LockType::UNIQUE_LOCK) {
                p_->Unlock();
            } else if (type_ == LockType::SHARED_LOCK) {
                p_->UnlockShared();
            }
        }
    }

    explicit operator bool() const
    {
        return p_ != nullptr;
    }

private:
    LockType type_;
    const ILockable* p_;
};

/**
 * @brief Helper class to acquire write lock and release it end of the scope.
 */
class InterfaceUniqueLock {
public:
    META_NO_COPY(InterfaceUniqueLock)

    explicit InterfaceUniqueLock(const CORE_NS::IInterface* t) : p_(interface_cast<ILockable>(t))
    {
        if (p_) {
            p_->Lock();
        }
    }
    explicit InterfaceUniqueLock(const CORE_NS::IInterface& t) : InterfaceUniqueLock(&t) {}
    template<typename Type>
    explicit InterfaceUniqueLock(const BASE_NS::shared_ptr<Type>& t) : InterfaceUniqueLock(t.get())
    {}

    InterfaceUniqueLock(InterfaceUniqueLock&& l) noexcept : p_(l.p_)
    {
        l.p_ = nullptr;
    }

    InterfaceUniqueLock& operator=(InterfaceUniqueLock&& l) noexcept
    {
        p_ = l.p_;
        l.p_ = nullptr;
        return *this;
    }

    ~InterfaceUniqueLock()
    {
        if (p_) {
            p_->Unlock();
        }
    }

    explicit operator bool() const
    {
        return p_ != nullptr;
    }

private:
    const ILockable* p_;
};

/**
 * @brief Helper class to acquire read lock and release it end of the scope.
 */
class InterfaceSharedLock {
public:
    META_NO_COPY(InterfaceSharedLock)

    explicit InterfaceSharedLock(const CORE_NS::IInterface* t) : p_(interface_cast<ILockable>(t))
    {
        if (p_) {
            p_->LockShared();
        }
    }
    explicit InterfaceSharedLock(const CORE_NS::IInterface& t) : InterfaceSharedLock(&t) {}
    template<typename Type>
    explicit InterfaceSharedLock(const BASE_NS::shared_ptr<Type>& t) : InterfaceSharedLock(t.get())
    {}

    InterfaceSharedLock(InterfaceSharedLock&& l) noexcept : p_(l.p_)
    {
        l.p_ = nullptr;
    }

    InterfaceSharedLock& operator=(InterfaceSharedLock&& l) noexcept
    {
        p_ = l.p_;
        l.p_ = nullptr;
        return *this;
    }

    ~InterfaceSharedLock()
    {
        if (p_) {
            p_->UnlockShared();
        }
    }

    explicit operator bool() const
    {
        return p_ != nullptr;
    }

private:
    const ILockable* p_;
};

META_END_NAMESPACE()

#endif
