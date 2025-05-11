/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: ClassId::CubicBezierEasingCurve implementation
 * Author: Lauri Jaaskela
 * Create: 2023-08-08
 */
#ifndef META_SRC_CUBIC_BEZIER_EASING_CURVE_H
#define META_SRC_CUBIC_BEZIER_EASING_CURVE_H

#include <meta/base/namespace.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/curves/intf_bezier.h>
#include <meta/interface/curves/intf_easing_curve.h>

#include "../object.h"

META_BEGIN_NAMESPACE()

namespace Curves {
namespace Easing {

class CubicBezierEasingCurve final : public IntroduceInterfaces<MetaObject, IEasingCurve, ICubicBezier> {
    META_OBJECT(CubicBezierEasingCurve, ClassId::CubicBezierEasingCurve, IntroduceInterfaces)

public:
    bool Build(const IMetadata::Ptr& meta) override;
    float Transform(float t) const override;

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(ICubicBezier, BASE_NS::Math::Vec2, ControlPoint1, BASE_NS::Math::Vec2(0, 0))
    META_STATIC_PROPERTY_DATA(ICubicBezier, BASE_NS::Math::Vec2, ControlPoint2, BASE_NS::Math::Vec2(1, 1))
    META_END_STATIC_DATA()
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec2, ControlPoint1)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec2, ControlPoint2)

private:
    void UpdateCoefficients();
    float GetLinearX(float t) const noexcept;

    constexpr float GetX(float t) const noexcept
    {
        return ((coeff_[0].x * t + coeff_[1].x) * t + coeff_[2].x) * t;
    }
    constexpr float GetY(float t) const noexcept
    {
        return ((coeff_[0].y * t + coeff_[1].y) * t + coeff_[2].y) * t;
    }
    constexpr float GetDX(float t) const noexcept
    {
        return (3.f * coeff_[0].x * t + 2.f * coeff_[1].x) * t + coeff_[2].x;
    }

    BASE_NS::Math::Vec2 coeff_[3];
};

} // namespace Easing
} // namespace Curves

META_END_NAMESPACE()

#endif // META_SRC_CUBIC_BEZIER_EASING_CURVE_H
