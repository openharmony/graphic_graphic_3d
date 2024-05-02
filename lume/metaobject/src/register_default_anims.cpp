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
#include <meta/base/namespace.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_registry.h>

#include "animation/animation_controller.h"
#include "animation/interpolator.h"
#include "animation/keyframe_animation.h"
#include "animation/modifiers/loop.h"
#include "animation/modifiers/reverse.h"
#include "animation/modifiers/speed.h"
#include "animation/parallel_animation.h"
#include "animation/property_animation.h"
#include "animation/sequential_animation.h"
#include "animation/track_animation.h"
#include "curves/bezier_curve.h"
#include "curves/easing_curve.h"

META_BEGIN_NAMESPACE()

namespace Internal {

#define DECL_NO(A, B) { ObjectTypeInfo::UID }, ClassId::A, (B)
#define DECL_AN(A, B) { ObjectTypeInfo::UID }, ClassId::A, (B)

static constexpr ObjectTypeInfo OBJECTS[] = {
    AnimationController::OBJECT_INFO,
    KeyframeAnimation::OBJECT_INFO,
    PropertyAnimation::OBJECT_INFO,
    TrackAnimation::OBJECT_INFO,
    ParallelAnimation::OBJECT_INFO,
    SequentialAnimation::OBJECT_INFO,
    AnimationModifiers::Loop::OBJECT_INFO,
    AnimationModifiers::Reverse::OBJECT_INFO,
    AnimationModifiers::SpeedImpl::OBJECT_INFO,

    Curves::Easing::LinearEasingCurve::OBJECT_INFO,
    Curves::Easing::InQuadEasingCurve::OBJECT_INFO,
    Curves::Easing::OutQuadEasingCurve::OBJECT_INFO,
    Curves::Easing::InOutQuadEasingCurve::OBJECT_INFO,
    Curves::Easing::InCubicEasingCurve::OBJECT_INFO,
    Curves::Easing::OutCubicEasingCurve::OBJECT_INFO,
    Curves::Easing::InOutCubicEasingCurve::OBJECT_INFO,
    Curves::Easing::InSineEasingCurve::OBJECT_INFO,
    Curves::Easing::OutSineEasingCurve::OBJECT_INFO,
    Curves::Easing::InOutSineEasingCurve::OBJECT_INFO,
    Curves::Easing::InQuartEasingCurve::OBJECT_INFO,
    Curves::Easing::OutQuartEasingCurve::OBJECT_INFO,
    Curves::Easing::InOutQuartEasingCurve::OBJECT_INFO,
    Curves::Easing::InQuintEasingCurve::OBJECT_INFO,
    Curves::Easing::OutQuintEasingCurve::OBJECT_INFO,
    Curves::Easing::InOutQuintEasingCurve::OBJECT_INFO,
    Curves::Easing::InExpoEasingCurve::OBJECT_INFO,
    Curves::Easing::OutExpoEasingCurve::OBJECT_INFO,
    Curves::Easing::InOutExpoEasingCurve::OBJECT_INFO,
    Curves::Easing::InCircEasingCurve::OBJECT_INFO,
    Curves::Easing::OutCircEasingCurve::OBJECT_INFO,
    Curves::Easing::InOutCircEasingCurve::OBJECT_INFO,
    Curves::Easing::InBackEasingCurve::OBJECT_INFO,
    Curves::Easing::OutBackEasingCurve::OBJECT_INFO,
    Curves::Easing::InOutBackEasingCurve::OBJECT_INFO,
    Curves::Easing::InElasticEasingCurve::OBJECT_INFO,
    Curves::Easing::OutElasticEasingCurve::OBJECT_INFO,
    Curves::Easing::InOutElasticEasingCurve::OBJECT_INFO,
    Curves::Easing::InBounceEasingCurve::OBJECT_INFO,
    Curves::Easing::OutBounceEasingCurve::OBJECT_INFO,
    Curves::Easing::InOutBounceEasingCurve::OBJECT_INFO,
    Curves::Easing::StepStartEasingCurve::OBJECT_INFO,
    Curves::Easing::StepEndEasingCurve::OBJECT_INFO,
    Curves::Easing::CubicBezierEasingCurve::OBJECT_INFO,
};

void RegisterBuiltInAnimations(IObjectRegistry& registry)
{
    for (auto& t : OBJECTS) {
        registry.RegisterObjectType(t.GetFactory());
    }

    RegisterDefaultInterpolators(registry);
}
void UnRegisterBuiltInAnimations(IObjectRegistry& registry)
{
    for (auto& t : OBJECTS) {
        registry.UnregisterObjectType(t.GetFactory());
    }

    UnRegisterDefaultInterpolators(registry);
}
} // namespace Internal
META_END_NAMESPACE()
