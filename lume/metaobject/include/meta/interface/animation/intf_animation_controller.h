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

#ifndef META_INTERFACE_INTF_ANIMATION_CONTROLLER_H
#define META_INTERFACE_INTF_ANIMATION_CONTROLLER_H

#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/shared_ptr.h>
#include <meta/base/types.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_clock.h>
#include <meta/interface/intf_meta_object_lib.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IAnimationController, "e1c2d8bf-fc1f-45e5-b43d-f290531939ec")

class IAnimation;
class IClock;

/**
 * @brief IAnimationController defines an interface for a class which can act as
 *        a controller for animations.
 */
class IAnimationController : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IAnimationController, META_NS::InterfaceId::IAnimationController)
public:
    /**
     * @brief The StepInfo struct contains information about a Step
     *        operation.
     */
    struct StepInfo {
        /** Number of animations that were stepped. */
        uint32_t stepped_;
        /** Number of animations still running after step. */
        uint32_t running_;
    };
    /**
     * @brief The number of animations handled by this controller.
     */
    META_READONLY_PROPERTY(uint32_t, Count)

    /**
     * @brief The number of animations running currently.
     */
    META_READONLY_PROPERTY(uint32_t, RunningCount)

    /**
     * @brief Returns weak references to all animations in this controller.
     */
    virtual BASE_NS::vector<BASE_NS::weak_ptr<IAnimation>> GetAnimations() const = 0;
    /**
     * @brief Returns weak references to all active animations in this controller.
     */
    virtual BASE_NS::vector<BASE_NS::weak_ptr<IAnimation>> GetRunning() const = 0;
    /**
     * @brief AddAnimation adds an animation for this controller to handle.
     * @param animation The animation to add.
     * @return True if the animation is handled by the controller after the function returns, false otherwise.
     */
    virtual bool AddAnimation(const BASE_NS::shared_ptr<IAnimation>& animation) = 0;
    /**
     * @brief Removes an animation from this controller.
     * @param animation The animation to remove.
     * @return True if the animation was removed from the controller, false otherwise.
     */
    virtual bool RemoveAnimation(const BASE_NS::shared_ptr<IAnimation>& animation) = 0;
    /**
     * @brief Removes all animations from the controller.
     */
    virtual void Clear() = 0;
    /**
     * @brief Steps all animations controller by this controller using the
     *        current time. To manually specify the timestamp, use
     *        Step(int64_t time).
     * @note The time used for stepping the animations can be retrieved
     *       from the returned StepInfo object.
     * @return Information about the step operation.
     */
    virtual StepInfo Step(const IClock::ConstPtr& clock) = 0;
};

META_END_NAMESPACE()

META_TYPE(META_NS::IAnimationController::WeakPtr)
META_TYPE(META_NS::IAnimationController::Ptr)

META_BEGIN_NAMESPACE()

/**
 * @brief Get global animation controller.
 */
inline META_NS::IAnimationController::Ptr GetAnimationController()
{
    return GetMetaObjectLib().GetAnimationController();
}

META_END_NAMESPACE()

#endif
