/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Base animation modifier implementation
 * Author: Lauri Jaaskela
 * Create: 2023-12-20
 */
#ifndef META_SRC_ANIMATION_MODIFIER_H
#define META_SRC_ANIMATION_MODIFIER_H

#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/ext/attachment/attachment.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/animation/intf_animation_modifier.h>

META_BEGIN_NAMESPACE()

class AnimationModifierFwd : public IntroduceInterfaces<AttachmentFwd, IAnimationModifier> {
public:
    using Super = IntroduceInterfaces<AttachmentFwd, IAnimationModifier>;

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
