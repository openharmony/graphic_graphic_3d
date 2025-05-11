/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Speed animation modifier
 * Author: Lauri Jaaskela
 * Create: 2023-07-24
 */

#ifndef META_INTERFACE_ANIMATION_MODIFIERS_INTF_SPEED_H
#define META_INTERFACE_ANIMATION_MODIFIERS_INTF_SPEED_H

#include <meta/base/meta_types.h>
#include <meta/interface/interface_macros.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ISpeed, "1822d284-d256-4088-98ea-01ff620e709e")

namespace AnimationModifiers {

/**
 * @brief Defines interface for the loop animation modifier
 */
class ISpeed : public CORE_NS::IInterface {
    META_INTERFACE(IInterface, ISpeed)
public:
    /**
     * @brief Speed factor to use for the animation.
     *        Negative factor will cause the animation to run in reverse.
     *        The default value is 1.
     */
    META_PROPERTY(float, SpeedFactor)
};

} // namespace AnimationModifiers

META_END_NAMESPACE()

META_TYPE(META_NS::AnimationModifiers::ISpeed::Ptr)

#endif // META_INTERFACE_ANIMATION_MODIFIERS_INTF_SPEED_H
