/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Loop animation modifier.
 * Author: Sami Enne
 * Create: 2023-02-08
 */

#ifndef META_SRC_ANIMATION_MODIFIERS_REVERSE_H
#define META_SRC_ANIMATION_MODIFIERS_REVERSE_H

#include <meta/base/namespace.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation_modifier.h>
#include <meta/interface/intf_manual_clock.h>
#include <meta/interface/object_macros.h>

#include "../animation_modifier.h"

META_BEGIN_NAMESPACE()

namespace AnimationModifiers {

class ReverseModifier final : public IntroduceInterfaces<AnimationModifierFwd> {
    META_OBJECT(ReverseModifier, META_NS::ClassId::ReverseAnimationModifier, IntroduceInterfaces, ClassId::Object)
public:
    bool Build(const IMetadata::Ptr& data) override;

public: // IAnimationModifier
    bool ProcessOnGetDuration(DurationData& duration) const override;
    bool ProcessOnStep(StepData& step) const override;
};

} // namespace AnimationModifiers

META_END_NAMESPACE()

#endif // META_SRC_ANIMATION_MODIFIERS_REVERSE_H
