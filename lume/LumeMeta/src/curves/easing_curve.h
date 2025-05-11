/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: IEasingCurve API implementation
 * Author: Lauri Jaaskela
 * Create: 2021-10-05
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
#define META_DECLARE_EASING_CURVE(name)                                                          \
    class name##EasingCurve final : public IntroduceInterfaces<BaseObject, IEasingCurve> {       \
        META_OBJECT(name##EasingCurve, META_NS::ClassId::name##EasingCurve, IntroduceInterfaces) \
        float Transform(float t) const override;                                                 \
    };

META_DECLARE_EASING_CURVE(Linear)
META_DECLARE_EASING_CURVE(InQuad)
META_DECLARE_EASING_CURVE(OutQuad)
META_DECLARE_EASING_CURVE(InOutQuad)
META_DECLARE_EASING_CURVE(InCubic)
META_DECLARE_EASING_CURVE(OutCubic)
META_DECLARE_EASING_CURVE(InOutCubic)
META_DECLARE_EASING_CURVE(InSine)
META_DECLARE_EASING_CURVE(OutSine)
META_DECLARE_EASING_CURVE(InOutSine)
META_DECLARE_EASING_CURVE(InQuart)
META_DECLARE_EASING_CURVE(OutQuart)
META_DECLARE_EASING_CURVE(InOutQuart)
META_DECLARE_EASING_CURVE(InQuint)
META_DECLARE_EASING_CURVE(OutQuint)
META_DECLARE_EASING_CURVE(InOutQuint)
META_DECLARE_EASING_CURVE(InExpo)
META_DECLARE_EASING_CURVE(OutExpo)
META_DECLARE_EASING_CURVE(InOutExpo)
META_DECLARE_EASING_CURVE(InCirc)
META_DECLARE_EASING_CURVE(OutCirc)
META_DECLARE_EASING_CURVE(InOutCirc)
META_DECLARE_EASING_CURVE(InBack)
META_DECLARE_EASING_CURVE(OutBack)
META_DECLARE_EASING_CURVE(InOutBack)
META_DECLARE_EASING_CURVE(InElastic)
META_DECLARE_EASING_CURVE(OutElastic)
META_DECLARE_EASING_CURVE(InOutElastic)
META_DECLARE_EASING_CURVE(InBounce)
META_DECLARE_EASING_CURVE(OutBounce)
META_DECLARE_EASING_CURVE(InOutBounce)
META_DECLARE_EASING_CURVE(StepStart)
META_DECLARE_EASING_CURVE(StepEnd)

#undef META_DECLARE_EASING_CURVE

} // namespace Easing
} // namespace Curves

META_END_NAMESPACE()

#endif
