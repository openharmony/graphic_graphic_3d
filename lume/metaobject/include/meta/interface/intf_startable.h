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

#ifndef META_INTERFACE_ISTARTABLE_H
#define META_INTERFACE_ISTARTABLE_H

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_container.h>

META_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IStartable, "b27ea024-7ebf-4d2a-a166-309bc0cf4d4f")

enum class StartBehavior : uint32_t {
    /** The startable should not be started automatically */
    MANUAL = 0,
    /** The startable should be started automatically when it has been made part of an object hierarchy
        managed by a StartableObjectController. */
    AUTOMATIC = 1,
};

enum class StartableState : uint32_t {
    /** The startable has not been attached to a hierarchy. */
    DETACHED = 0,
    /** The startable is part of a hierarchy but has not been started. */
    ATTACHED = 1,
    /** The startable is started. */
    STARTED = 2
};

META_END_NAMESPACE()

META_TYPE(META_NS::StartableState)
META_TYPE(META_NS::StartBehavior)

META_BEGIN_NAMESPACE()

/**
 * @brief The IStartable interface can be implemented by objects which can be started and stopped.
 * @note Usually IStartable is implemented by objects that are placed in an object hierarchy and
 *       whose running state is controller by an IStartableController implementation.
 */
class IStartable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IStartable)

public:
    /**
     * @brief The startable mode. If StartBehavior::AUTOMATIC, the startable will be
     *        started automatically by a controller.
     */
    META_PROPERTY(META_NS::StartBehavior, StartableMode)
    /**
     * @brief State of the startable.
     */
    META_READONLY_PROPERTY(META_NS::StartableState, StartableState)
    /**
     * @brief Start the startable.
     * @return True if successful, false otherwise.
     */
    virtual bool Start() = 0;
    /**
     * @brief Stop the startable.
     * @return True if successful, false otherwise.
     */
    virtual bool Stop() = 0;
};

META_END_NAMESPACE()

#endif
