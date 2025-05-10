/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Loop animation modifier
 * Author: Sami Enne
 * Create: 2023-02-08
 */

#ifndef META_INTERFACE_ANIMATION_MODIFIERS_INTF_LOOP_H
#define META_INTERFACE_ANIMATION_MODIFIERS_INTF_LOOP_H

#include <meta/base/meta_types.h>
#include <meta/interface/interface_macros.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ILoop, "08f22131-c65d-4c12-b351-5fc784aa5278")

namespace AnimationModifiers {

/**
 * @brief Defines interface for the loop animation modifier
 */
class ILoop : public CORE_NS::IInterface {
    META_INTERFACE(IInterface, ILoop)
public:
    /**
     * @brief The number of times the animation should loop. Default 1.
     *        *  0: Animation is not run at all.
     *        * >0: Loop the animation this many times.
     *        * <0: Loop the animation indefinitely.
     */
    META_PROPERTY(int32_t, LoopCount)
};

} // namespace AnimationModifiers

META_END_NAMESPACE()

META_TYPE(META_NS::AnimationModifiers::ILoop::Ptr)

#endif // META_INTERFACE_ANIMATION_MODIFIERS_INTF_LOOP_H
