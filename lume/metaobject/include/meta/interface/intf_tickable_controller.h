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

#ifndef META_INTERFACE_ITICKABLE_CONTROLLER_H
#define META_INTERFACE_ITICKABLE_CONTROLLER_H

#include <meta/base/interface_macros.h>
#include <meta/base/time_span.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_iterable.h>
#include <meta/interface/intf_tickable.h>

META_BEGIN_NAMESPACE()

REGISTER_INTERFACE(ITickableController, "6b730c19-8a43-4a00-8e41-fda0bc78d294")

/**
 * @brief The ITickableController interface is implemented by a controller objects for startables.
 * @note The default implementation is META_NS::ClassId::StartableController.
 */
class ITickableController : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITickableController)
public:
    /**
     * @brief Return a list of all tickable objects handled by a controller.
     */
    virtual BASE_NS::vector<ITickable::Ptr> GetTickables() const = 0;
    /**
     * @brief Call ITickable::Tick for all tickable objects handled by the controller in a given
     *        order.
     * @param time Current time.
     */
    virtual void TickAll(const TimeSpan& time) = 0;
    /**
     * @brief Interval for ticking the tickables which are part of the controller's hierarchy.
     *        The default value is TimeSpan::Infinite, meaning that the tickables are not automatically ticked.
     */
    META_PROPERTY(META_NS::TimeSpan, TickInterval)
    /**
     * @brief The order in which the tickables should be ticked.
     *        The default value is TraversalType::DEPTH_FIRST_PRE_ORDER.
     */
    META_PROPERTY(META_NS::TraversalType, TickOrder)
    /**
     * @brief Set the queue id of the queue where ITickable::Tick() should be called when ticking.
     * @param queueId The id of the task queue. if {}, a new thread should be created for ticking.
     * @return True if the queue id was successfully set, false otherwise.
     */
    virtual bool SetTickableQueueuId(const BASE_NS::Uid& queueId) = 0;
};

META_END_NAMESPACE()

#endif // META_INTERFACE_ITICKABLE_CONTROLLER_H
