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

#ifndef META_INTERFACE_INTF_ANIMATION_MODIFIER_H
#define META_INTERFACE_INTF_ANIMATION_MODIFIER_H

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_clock.h>
#include <meta/interface/property/property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IAnimationModifier, "9da441d6-e0db-464a-80d4-a5657918c601")

/**
 * @brief Interface for animation modifiers.
 *
 * Animation modifiers are used to modify existing animations by attaching them with the IAttach mechanism.
 */
class IAnimationModifier : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IAnimationModifier, META_NS::InterfaceId::IAnimationModifier)
public:
    /**
     * @brief Struct used for modifying animation length by an animation modifier.
     */
    struct DurationData {
        constexpr DurationData() noexcept = default;
        explicit constexpr DurationData(TimeSpan duration) noexcept : duration(duration) {}
        /** Duration of one loop of the animation */
        TimeSpan duration { TimeSpan::Zero() };
        /** Loop count */
        int32_t loopCount { 1 };
    };
    /**
     * @brief Called by an animation during initialization, allowing the modifier to change the
     *        duration of the animation.
     * @param duration Duration info.
     * @return True if the modifier can be applied on the input data, false otherwise.
     */
    virtual bool ProcessOnGetDuration(DurationData& duration) const = 0;
    /**
     * @brief Struct used for modifying animation step by an animation modifier.
     */
    struct StepData {
        constexpr StepData() noexcept = default;
        explicit constexpr StepData(float progress) noexcept : progress(progress) {}
        /** Current progress. */
        float progress {};
        /** Is the animation reversing */
        bool reverse {};
    };
    /**
     * @brief Called by an animation during Step phase, allowing the modifier to change the
     *        current progress.
     * @param step Step info.
     * @return True if the modifier can be applied on the input data, false otherwise.
     */
    virtual bool ProcessOnStep(StepData& step) const = 0;
};

META_END_NAMESPACE()

#endif // META_INTERFACE_INTF_ANIMATION_MODIFIER_H
