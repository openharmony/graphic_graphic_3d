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

#ifndef META_SRC_ANIMATION_MODIFIERS_LOOP_H
#define META_SRC_ANIMATION_MODIFIERS_LOOP_H

#include <meta/base/namespace.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation_modifier.h>
#include <meta/interface/animation/modifiers/intf_loop.h>
#include <meta/interface/intf_manual_clock.h>
#include <meta/interface/object_macros.h>

#include "../animation_modifier.h"

META_BEGIN_NAMESPACE()

namespace AnimationModifiers {

class Loop final : public AnimationModifierFwd<Loop, META_NS::ClassId::LoopAnimationModifier, AnimationModifiers::ILoop,
                       INotifyOnChange> {
public: // ILifecycle
    bool Build(const IMetadata::Ptr& data) override;

public: // IAnimationModifier
    bool ProcessOnGetDuration(DurationData& duration) const override;
    bool ProcessOnStep(StepData& step) const override;

public: // ILoop
    META_IMPLEMENT_INTERFACE_PROPERTY(ILoop, int32_t, LoopCount, 1)

public: // INotifyOnChange
    META_IMPLEMENT_INTERFACE_EVENT(INotifyOnChange, IOnChanged, OnChanged)

private:
    IOnChanged::InterfaceTypePtr updateDuration_;
    void UpdateDuration();
    Property<TimeSpan> duration_;
    Property<const TimeSpan> originalDuration_;

    META_NS::IManualClock::Ptr manualClock_;
};

} // namespace AnimationModifiers

META_END_NAMESPACE()

#endif // META_SRC_LOOP_ANIMATION_MODIFIER_H
