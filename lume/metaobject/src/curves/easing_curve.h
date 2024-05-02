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
#ifndef META_SRC_EASING_CURVE_H
#define META_SRC_EASING_CURVE_H

#include <meta/base/namespace.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/curves/intf_easing_curve.h>

#include "../base_object.h"

META_BEGIN_NAMESPACE()

namespace Curves {
namespace Easing {

// Declares a class which implements IEasingCurve interface
#define DECLARE_EASING_CURVE(name)                                                                      \
    class name##EasingCurve final                                                                       \
        : public Internal::BaseObjectFwd<name##EasingCurve, ClassId::name##EasingCurve, IEasingCurve> { \
        float Transform(float t) const override;                                                        \
    };

DECLARE_EASING_CURVE(Linear)
DECLARE_EASING_CURVE(InQuad)
DECLARE_EASING_CURVE(OutQuad)
DECLARE_EASING_CURVE(InOutQuad)
DECLARE_EASING_CURVE(InCubic)
DECLARE_EASING_CURVE(OutCubic)
DECLARE_EASING_CURVE(InOutCubic)
DECLARE_EASING_CURVE(InSine)
DECLARE_EASING_CURVE(OutSine)
DECLARE_EASING_CURVE(InOutSine)
DECLARE_EASING_CURVE(InQuart)
DECLARE_EASING_CURVE(OutQuart)
DECLARE_EASING_CURVE(InOutQuart)
DECLARE_EASING_CURVE(InQuint)
DECLARE_EASING_CURVE(OutQuint)
DECLARE_EASING_CURVE(InOutQuint)
DECLARE_EASING_CURVE(InExpo)
DECLARE_EASING_CURVE(OutExpo)
DECLARE_EASING_CURVE(InOutExpo)
DECLARE_EASING_CURVE(InCirc)
DECLARE_EASING_CURVE(OutCirc)
DECLARE_EASING_CURVE(InOutCirc)
DECLARE_EASING_CURVE(InBack)
DECLARE_EASING_CURVE(OutBack)
DECLARE_EASING_CURVE(InOutBack)
DECLARE_EASING_CURVE(InElastic)
DECLARE_EASING_CURVE(OutElastic)
DECLARE_EASING_CURVE(InOutElastic)
DECLARE_EASING_CURVE(InBounce)
DECLARE_EASING_CURVE(OutBounce)
DECLARE_EASING_CURVE(InOutBounce)
DECLARE_EASING_CURVE(StepStart)
DECLARE_EASING_CURVE(StepEnd)

} // namespace Easing
} // namespace Curves

META_END_NAMESPACE()

#endif
