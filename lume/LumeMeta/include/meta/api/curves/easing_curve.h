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

#ifndef META_API_EASING_CURVE_H
#define META_API_EASING_CURVE_H

#include <meta/api/internal/object_api.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/curves/intf_easing_curve.h>

META_BEGIN_NAMESPACE()

namespace Curves {
namespace Easing {

#define META_IMPLEMENT_EASING_CURVE_API(name)                                                     \
    class name : public Internal::ObjectInterfaceAPI<name, META_NS::ClassId::name##EasingCurve> { \
        META_API(name)                                                                            \
        META_API_OBJECT_CONVERTIBLE(META_NS::ICurve1D)                                            \
        META_API_OBJECT_CONVERTIBLE(META_NS::IEasingCurve)                                        \
    };

META_IMPLEMENT_EASING_CURVE_API(Linear)
META_IMPLEMENT_EASING_CURVE_API(InQuad)
META_IMPLEMENT_EASING_CURVE_API(OutQuad)
META_IMPLEMENT_EASING_CURVE_API(InOutQuad)
META_IMPLEMENT_EASING_CURVE_API(InCubic)
META_IMPLEMENT_EASING_CURVE_API(OutCubic)
META_IMPLEMENT_EASING_CURVE_API(InOutCubic)
META_IMPLEMENT_EASING_CURVE_API(InSine)
META_IMPLEMENT_EASING_CURVE_API(OutSine)
META_IMPLEMENT_EASING_CURVE_API(InOutSine)
META_IMPLEMENT_EASING_CURVE_API(InQuart)
META_IMPLEMENT_EASING_CURVE_API(OutQuart)
META_IMPLEMENT_EASING_CURVE_API(InOutQuart)
META_IMPLEMENT_EASING_CURVE_API(InQuint)
META_IMPLEMENT_EASING_CURVE_API(OutQuint)
META_IMPLEMENT_EASING_CURVE_API(InOutQuint)
META_IMPLEMENT_EASING_CURVE_API(InExpo)
META_IMPLEMENT_EASING_CURVE_API(OutExpo)
META_IMPLEMENT_EASING_CURVE_API(InOutExpo)
META_IMPLEMENT_EASING_CURVE_API(InCirc)
META_IMPLEMENT_EASING_CURVE_API(OutCirc)
META_IMPLEMENT_EASING_CURVE_API(InOutCirc)
META_IMPLEMENT_EASING_CURVE_API(InBack)
META_IMPLEMENT_EASING_CURVE_API(OutBack)
META_IMPLEMENT_EASING_CURVE_API(InOutBack)
META_IMPLEMENT_EASING_CURVE_API(InElastic)
META_IMPLEMENT_EASING_CURVE_API(OutElastic)
META_IMPLEMENT_EASING_CURVE_API(InOutElastic)
META_IMPLEMENT_EASING_CURVE_API(InBounce)
META_IMPLEMENT_EASING_CURVE_API(OutBounce)
META_IMPLEMENT_EASING_CURVE_API(InOutBounce)
META_IMPLEMENT_EASING_CURVE_API(StepStart)
META_IMPLEMENT_EASING_CURVE_API(StepEnd)
} // namespace Easing
} // namespace Curves

META_END_NAMESPACE()

#endif // META_API_EASING_CURVE_H
