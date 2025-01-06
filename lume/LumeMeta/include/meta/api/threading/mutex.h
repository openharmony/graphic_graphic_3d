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

#ifndef META_API_THREADING_MUTEX_H
#define META_API_THREADING_MUTEX_H

#include <meta/base/interface_macros.h>

#include "primitive_api.h"

CORE_BEGIN_NAMESPACE()

/**
 * @brief Mutex class that can be used across shared library boundaries and in public headers.
 *        It is not copyable or movable.
 */
class Mutex {
public:
    META_NO_COPY_MOVE(Mutex)

    /**
     * @brief Construct mutex using sync api.
     */
    Mutex() : api_(GetSyncApi().mutex)
    {
        api_.create(MutexType::NORMAL, handle_);
    }

    /**
     * @brief Destroy mutex.
     */
    ~Mutex()
    {
        api_.destroy(handle_);
    }

    /**
     * @brief Lock mutex, call it again for already locked mutex has undefined behaviour.
     */
    void Lock()
    {
        api_.lock(handle_);
    }

    /**
     * @brief Unlock mutex, calling it for not locked mutex has undefined behaviour.
     */
    void Unlock()
    {
        api_.unlock(handle_);
    }

private:
    SyncApi::MutexApi api_;
    MutexHandle handle_;
};

/**
 * @brief General-purpose mutex ownership wrapper, allows to transfer ownership by moving.
 */
template<class Mutex>
class UniqueLock {
public:
    META_NO_COPY(UniqueLock)
    /**
     * @brief Construct lock from mutex and lock it.
     */
    explicit UniqueLock(Mutex& mutex) : mutex_(&mutex)
    {
        Lock();
    }

    /**
     * @brief Construct by moving from another lock.
     */
    UniqueLock(UniqueLock&& lock) : locked_(lock.locked_), mutex_(lock.mutex_)
    {
        lock.mutex_ = nullptr;
        lock.locked_ = false;
    }

    /**
     * @brief Destroy lock, unlocks associated mutex if it is locked.
     */
    ~UniqueLock()
    {
        if (mutex_ && locked_) {
            mutex_->Unlock();
        }
    }

    /**
     * @brief Move from another lock, unlock the previously associated mutex if locked.
     */
    UniqueLock& operator=(UniqueLock&& lock)
    {
        if (&lock == this) {
            return *this;
        }
        if (mutex_ && locked_) {
            mutex_->Unlock();
        }
        locked_ = lock.locked_;
        mutex_ = lock.mutex_;
        lock.mutex_ = nullptr;
        lock.locked_ = false;
        return *this;
    }

    /**
     * @brief Lock the mutex, must have associated mutex which is not locked.
     */
    void Lock()
    {
        CORE_ASSERT_MSG(mutex_ && !locked_, "trying to lock mutex in invalid state");
        mutex_->Lock();
        locked_ = true;
    }

    /**
     * @brief Unlock the mutex, must have associated mutex which is locked.
     */
    void Unlock()
    {
        CORE_ASSERT_MSG(mutex_ && locked_, "trying to unlock mutex in invalid state");
        mutex_->Unlock();
        locked_ = false;
    }

    /**
     * @brief Returns true if this lock has associated mutex that is locked with this lock.
     */
    explicit operator bool() const
    {
        return mutex_ && locked_;
    }

private:
    bool locked_ {};
    Mutex* mutex_;
};

CORE_END_NAMESPACE()

#endif
