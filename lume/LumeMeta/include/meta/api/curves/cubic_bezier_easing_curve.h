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

#ifndef META_API_CUBIC_BEZIER_EASING_CURVE_H
#define META_API_CUBIC_BEZIER_EASING_CURVE_H

#include <meta/api/internal/object_api.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/curves/intf_bezier.h>
#include <meta/interface/curves/intf_easing_curve.h>

META_BEGIN_NAMESPACE()

namespace Curves {
namespace Easing {

class CubicBezierEasingCurve
    : public Internal::ObjectInterfaceAPI<CubicBezierEasingCurve, META_NS::ClassId::CubicBezierEasingCurve> {
    META_API(CubicBezierEasingCurve)
    META_API_OBJECT_CONVERTIBLE(META_NS::ICurve1D)
    META_API_OBJECT_CONVERTIBLE(META_NS::IEasingCurve)
    META_API_OBJECT_CONVERTIBLE(META_NS::ICubicBezier)
    META_API_CACHE_INTERFACE(META_NS::ICubicBezier, CubicBezier)
public:
    META_API_INTERFACE_PROPERTY_CACHED(CubicBezier, ControlPoint1, BASE_NS::Math::Vec2)
    META_API_INTERFACE_PROPERTY_CACHED(CubicBezier, ControlPoint2, BASE_NS::Math::Vec2)
};

} // namespace Easing
} // namespace Curves

META_END_NAMESPACE()

#endif // META_API_CUBIC_BEZIER_EASING_CURVE_H
