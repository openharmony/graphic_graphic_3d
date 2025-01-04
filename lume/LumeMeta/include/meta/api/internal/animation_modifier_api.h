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

#ifndef META_API_ANIMATION_MODIFIER_API_H
#define META_API_ANIMATION_MODIFIER_API_H

#include <meta/api/internal/object_api.h>
#include <meta/base/namespace.h>
#include <meta/interface/animation/intf_animation_modifier.h>

META_BEGIN_NAMESPACE()

namespace Internal {

template<class FinalClass, const META_NS::ClassInfo& Class>
class AnimationModifierInterfaceAPI : public ObjectInterfaceAPI<FinalClass, Class> {
    META_INTERFACE_API(AnimationModifierInterfaceAPI)
    META_API_OBJECT_CONVERTIBLE(META_NS::IAnimationModifier)
    META_API_OBJECT_CONVERTIBLE(META_NS::IAttachment)
public:
};

} // namespace Internal

META_END_NAMESPACE()

#endif // META_API_ANIMATION_MODIFIER_API_H
