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
#ifndef META_SRC_ANIMATION_MODIFIER_H
#define META_SRC_ANIMATION_MODIFIER_H

#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/ext/attachment/attachment.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/animation/intf_animation_modifier.h>

META_BEGIN_NAMESPACE()

template<class FinalClass, const META_NS::ClassInfo& ClassInfo, class... Interfaces>
class AnimationModifierFwd
    : public META_NS::AttachmentFwd<FinalClass, ClassInfo, META_NS::IAnimationModifier, Interfaces...> {
public:
    using Super = META_NS::AttachmentFwd<FinalClass, ClassInfo, META_NS::IAnimationModifier, Interfaces...>;

public:
    bool AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext) override
    {
        return true;
    }
    bool DetachFrom(const META_NS::IAttach::Ptr& target) override
    {
        return true;
    }

public: // IAnimationModifier
    bool ProcessOnGetDuration(IAnimationModifier::DurationData& duration) const override
    {
        return true;
    }
    bool ProcessOnStep(IAnimationModifier::StepData& step) const override
    {
        return true;
    }
};

META_END_NAMESPACE()

#endif // META_SRC_ANIMATION_MODIFIER_H
