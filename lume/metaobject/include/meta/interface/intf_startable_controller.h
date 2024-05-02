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

#ifndef META_INTERFACE_ISTARTABLE_CONTROLLER_H
#define META_INTERFACE_ISTARTABLE_CONTROLLER_H

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_startable.h>

META_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IStartableController, "dd8e8c3d-f81a-4e33-8c42-6db05eb65e24")

/**
 * @brief The IStartableController interface is implemented by controller objects for startables.
 * @note The default implementation is META_NS::ClassId::StartableController.
 */
class IStartableController : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IStartableController)
public:
    /**
     * @brief The ControlBehavior enum defines the modes which can be used to set which
     *        startables are affected by a control operation such as StartAll or StopAll.
     */
    enum class ControlBehavior : uint32_t {
        /** Control startables whose StartableMode=StartBehavior::AUTOMATIC. */
        CONTROL_AUTOMATIC = 0,
        /** Control all startables regardless of their StartableMode, both AUTOMATIC and
         *  MANUAL startables are affected by the operation. */
        CONTROL_ALL = 1,
    };
    /**
     * @brief The traversal order in which the hierarchy should be traversed when starting/stopping
     *        the startables.
     */
    META_PROPERTY(META_NS::TraversalType, TraversalType)
    /**
     * @brief Controls how the controller should handle new startables that are added to the hierarchy.
     *        If StartBehavior::AUTOMATIC, new startables whose StartableMode==StartBehavior::AUTOMATIC
     *        are started automatically, and startables that are removed from the hierarchy are stopped
     *        automatically.
     *        If StartBehavior::MANUAL, new startables are not started/stopped by the controller
     *        automatically.
     */
    META_PROPERTY(META_NS::StartBehavior, StartBehavior)
    /**
     * @brief Start all startables which are part of the target hierarchy, using the order specified
     *        by TraversalType.
     * @param behavior Control which startables are started.
     * @return True if successful, false otherwise.
     */
    virtual bool StartAll(ControlBehavior behavior) = 0;
    /**
     * @brief Stops all startables which are part of the target hierarchy, in reverse order to
     *        the order in which the startables were started by StartAll.
     * @param behavior Control which startables are started.
     * @return True if successful, false otherwise.
     */
    virtual bool StopAll(ControlBehavior behavior) = 0;
    /**
     * @brief Returns all startables currently being controlled.
     */
    virtual BASE_NS::vector<IStartable::Ptr> GetAllStartables() const = 0;
    /**
     * @brief Sets the task queue id of the task queue which should be used to Start/Stop startables
     *        handled by the controller.
     * @param startStartableQueueId Task queue id to use for starting startables. If {}, use current thread for all
     *        operations.
     * @param stopStartableQueueId Task queue id to use for stopping startables. If {}, use current thread for all
     *        operations. Note that setting stopStartableQueueId may alter the order in which Detach and Stop operations
     *        happen for a startable which also implements IAttachment, as Stop operation is deferred to the given task
     *        queue. This may cause Stop to be called after Detach.
     * @return True if queue id was valid, false otherise.
     */
    virtual bool SetStartableQueueId(
        const BASE_NS::Uid& startStartableQueueId, const BASE_NS::Uid& stopStartableQueueId) = 0;
    /**
     * @brief Starts all StartBehavior::AUTOMATIC startables
     */
    bool StartAll()
    {
        return StartAll(ControlBehavior::CONTROL_AUTOMATIC);
    }
    /**
     * @brief Stops all StartBehavior::AUTOMATIC startables
     */
    bool StopAll()
    {
        return StopAll(ControlBehavior::CONTROL_AUTOMATIC);
    }
};

META_END_NAMESPACE()

#endif
