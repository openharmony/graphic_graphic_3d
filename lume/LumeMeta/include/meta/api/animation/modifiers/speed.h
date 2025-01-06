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

#ifndef META_API_ANIMATION_MODIFIERS_SPEED_H
#define META_API_ANIMATION_MODIFIERS_SPEED_H

#include <meta/api/internal/animation_modifier_api.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/modifiers/intf_speed.h>

META_BEGIN_NAMESPACE()

namespace AnimationModifiers {

/**
 * @brief Modifier for animations to set a playback speed factor for the animation.
 * Example:
 *     animation.Attach(AnimationModifiers::Speed().Speed(0.5f));
 *   will run the original animation at half speed.
 * Default value for speed is 1 (i.e. not altering behaviour).
 */
class Speed final : public Internal::AnimationModifierInterfaceAPI<Speed, META_NS::ClassId::SpeedAnimationModifier> {
    META_API(Speed)
    META_API_OBJECT_CONVERTIBLE(META_NS::AnimationModifiers::ISpeed)
    META_API_CACHE_INTERFACE(META_NS::AnimationModifiers::ISpeed, Speed)
public:
    /**
     * @brief Set the speed factor.
     */
    META_API_INTERFACE_PROPERTY_CACHED(Speed, SpeedFactor, float)
};

} // namespace AnimationModifiers

META_END_NAMESPACE()

#endif // META_API_ANIMATION_MODIFIERS_SPEED_H
