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

#ifndef META_API_CURVES_H
#define META_API_CURVES_H

#include <meta/api/object.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/curves/intf_bezier.h>
#include <meta/interface/curves/intf_easing_curve.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Wrapper class for objects which implement IEasingCurve.
 */
class EasingCurve : public Object {
public:
    META_INTERFACE_OBJECT(EasingCurve, Object, IEasingCurve)
    operator ICurve1D::Ptr() const noexcept
    {
        return GetPtr<ICurve1D>();
    }
    /// @see IEasingCurve::Transform
    auto Transform(float t) const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(Transform(t));
    }
};

namespace Curves {

#define META_EASING_CURVE_INTERFACE_OBJECT(Class)                                       \
    class Class : public EasingCurve {                                                  \
    public:                                                                             \
        Class(const IInterfacePtr& ptr) : EasingCurve(ptr)                              \
        {}                                                                              \
        Class(const ::META_NS::InitializationType& t)                                   \
            : EasingCurve(::META_NS::CreateObjectInstance(ClassId::Class##EasingCurve)) \
        {}                                                                              \
    };

/// Linear easing curve
META_EASING_CURVE_INTERFACE_OBJECT(Linear)
/// InQuad easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InQuad)
/// OutQuad easing curve
META_EASING_CURVE_INTERFACE_OBJECT(OutQuad)
/// InOutQuad easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InOutQuad)
/// InCubic easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InCubic)
/// OutCubic easing curve
META_EASING_CURVE_INTERFACE_OBJECT(OutCubic)
/// InOutCubic easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InOutCubic)
/// InSine easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InSine)
/// OutSine easing curve
META_EASING_CURVE_INTERFACE_OBJECT(OutSine)
/// InOutSine easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InOutSine)
/// InQuart easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InQuart)
/// OutQuart easing curve
META_EASING_CURVE_INTERFACE_OBJECT(OutQuart)
/// InOutQuart easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InOutQuart)
/// InQuint easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InQuint)
/// OutQuint easing curve
META_EASING_CURVE_INTERFACE_OBJECT(OutQuint)
/// InOutQuint easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InOutQuint)
/// InExpo easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InExpo)
/// OutExpo easing curve
META_EASING_CURVE_INTERFACE_OBJECT(OutExpo)
/// InOutExpo easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InOutExpo)
/// InCirc easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InCirc)
/// OutCirc easing curve
META_EASING_CURVE_INTERFACE_OBJECT(OutCirc)
/// InOutCirc easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InOutCirc)
/// InBack easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InBack)
/// OutBack easing curve
META_EASING_CURVE_INTERFACE_OBJECT(OutBack)
/// InOutBack easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InOutBack)
/// InElastic easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InElastic)
/// OutElastic easing curve
META_EASING_CURVE_INTERFACE_OBJECT(OutElastic)
/// InOutElastic easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InOutElastic)
/// InBounce easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InBounce)
/// OutBounce easing curve
META_EASING_CURVE_INTERFACE_OBJECT(OutBounce)
/// InOutBounce easing curve
META_EASING_CURVE_INTERFACE_OBJECT(InOutBounce)
/// StepStart easing curve
META_EASING_CURVE_INTERFACE_OBJECT(StepStart)
/// StepEnd easing curve
META_EASING_CURVE_INTERFACE_OBJECT(StepEnd)

/**
 * @brief Wrapper class for objects which implement ICubicBezier.
 */
class CubicBezier : public EasingCurve {
public:
    META_INTERFACE_OBJECT(CubicBezier, EasingCurve, ICubicBezier)
    META_INTERFACE_OBJECT_INSTANTIATE(CubicBezier, ClassId::CubicBezierEasingCurve)
    /// @see ICubicBezier::ControlPoint1
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec2, ControlPoint1)
    /// @see ICubicBezier::ControlPoint1
    META_INTERFACE_OBJECT_PROPERTY(BASE_NS::Math::Vec2, ControlPoint2)
};

} // namespace Curves

/// Return a default object which implements ICubicBezier
template<>
inline auto CreateObjectInstance<ICubicBezier>()
{
    return Curves::CubicBezier(CreateNew);
}

/// Return a default object which implements ICubicBezier.
template<>
inline auto CreateObjectInstance<IEasingCurve>()
{
    // By default return an InOutCubic easing curve for IEasingCurve
    return Curves::InOutCubic(CreateNew);
}

META_END_NAMESPACE()

#endif // META_API_CURVES_H
