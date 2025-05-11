/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Animation modifier API
 * Author: Sami Enne
 * Create: 2023-02-08
 */

#ifndef META_INTERFACE_INTF_ANIMATION_MODIFIER_H
#define META_INTERFACE_INTF_ANIMATION_MODIFIER_H

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_clock.h>

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
