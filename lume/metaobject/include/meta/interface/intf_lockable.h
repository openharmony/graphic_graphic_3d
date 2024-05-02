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

#ifndef META_INTERFACE_ILOCKABLE_H
#define META_INTERFACE_ILOCKABLE_H

#include <cstdint>

#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/shared_ptr.h>
#include <meta/base/types.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Support locking
 */
META_REGISTER_INTERFACE(ILockable, "79ff7f69-93aa-43cb-b6f0-49ace40927ac")
class ILockable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ILockable)
public:
    /**
     * @brief Unique lock, lock for writing
     */
    virtual void Lock() const = 0;
    virtual void Unlock() const = 0;

    /**
     *  @brief Shared lock, lock for reading. This gives the implementation possibility
     *         to provide better granularity but might be implemented as unique lock as well.
     */
    virtual void LockShared() const = 0;
    virtual void UnlockShared() const = 0;
};

META_REGISTER_INTERFACE(IRecursivelyLockable, "77d7c7bb-2973-4477-acd8-86f7a21eff30")
/**
 * @brief Recursive version of the lockable.
 */
class IRecursivelyLockable : public ILockable {
    META_INTERFACE(CORE_NS::IInterface, IRecursivelyLockable)
};

enum class LockType { NO_LOCK = 0, UNIQUE_LOCK = 1, SHARED_LOCK = 2 };

META_END_NAMESPACE()

#endif
