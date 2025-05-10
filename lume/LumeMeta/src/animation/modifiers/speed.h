/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Loop animation modifier.
 * Author: Sami Enne
 * Create: 2023-02-08
 */

#ifndef META_SRC_ANIMATION_MODIFIERS_SPEED_H
#define META_SRC_ANIMATION_MODIFIERS_SPEED_H

#include <meta/api/property/property_event_handler.h>
#include <meta/base/namespace.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation_modifier.h>
#include <meta/interface/animation/modifiers/intf_speed.h>
#include <meta/interface/intf_manual_clock.h>
#include <meta/interface/object_macros.h>

#include "../animation_modifier.h"

META_BEGIN_NAMESPACE()

namespace AnimationModifiers {

class SpeedModifier final
    : public IntroduceInterfaces<AnimationModifierFwd, AnimationModifiers::ISpeed, INotifyOnChange> {
    META_OBJECT(SpeedModifier, META_NS::ClassId::SpeedAnimationModifier, IntroduceInterfaces, ClassId::Object)
public:
    bool Build(const IMetadata::Ptr& data) override;

public: // IAnimationModifier
    bool ProcessOnGetDuration(DurationData& duration) const override;
    bool ProcessOnStep(StepData& step) const override;

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(AnimationModifiers::ISpeed, float, SpeedFactor, 1)
    META_STATIC_EVENT_DATA(INotifyOnChange, IOnChanged, OnChanged)
    META_END_STATIC_DATA()
    META_IMPLEMENT_PROPERTY(float, SpeedFactor)
    META_IMPLEMENT_EVENT(IOnChanged, OnChanged)

private:
    PropertyChangedEventHandler speedFactorChanged_;
};

} // namespace AnimationModifiers

META_END_NAMESPACE()

#endif // META_SRC_ANIMATION_MODIFIERS_SPEED_H
