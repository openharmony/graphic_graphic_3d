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
