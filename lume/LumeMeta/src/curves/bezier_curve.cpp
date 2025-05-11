/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Bezier curve implementation.
 * Author: Lauri Jaaskela
 * Create: 2023-08-08
 */
#include "bezier_curve.h"

#include <base/math/mathf.h>

#include <meta/api/make_callback.h>
#include <meta/api/util.h>
#include <meta/interface/property/property_events.h>

META_BEGIN_NAMESPACE()

namespace Curves {
namespace Easing {

bool CubicBezierEasingCurve::Build(const IMetadata::Ptr& meta)
{
    if (Super::Build(meta)) {
        auto update = MakeCallback<IOnChanged>(this, &CubicBezierEasingCurve::UpdateCoefficients);
        META_ACCESS_PROPERTY(ControlPoint1)->OnChanged()->AddHandler(update);
        META_ACCESS_PROPERTY(ControlPoint2)->OnChanged()->AddHandler(update);
        UpdateCoefficients();
        return true;
    }
    return false;
}

float CubicBezierEasingCurve::Transform(float t) const
{
    // First, solve "linear" X coordinate for t [0..1], then sample the bezier's Y at that X.
    return GetY(GetLinearX(t));
}

void CubicBezierEasingCurve::UpdateCoefficients()
{
    auto cp1 = META_NS::GetValue(META_ACCESS_PROPERTY(ControlPoint1));
    auto cp2 = META_NS::GetValue(META_ACCESS_PROPERTY(ControlPoint2));

    // Clamp x-coordinates of our control points between [0,1]
    cp1.x = BASE_NS::Math::clamp01(cp1.x);
    cp2.x = BASE_NS::Math::clamp01(cp2.x);

    // Cache the polynomial coefficients, first and last control points are implicity [0,0] and [1,1]
    coeff_[2] = cp1 * 3.f;
    coeff_[1] = (cp2 - cp1) * 3.f - coeff_[2];
    coeff_[0] = BASE_NS::Math::Vec2(1.f, 1.f) - coeff_[2] - coeff_[1];
}

constexpr bool IsFloatNull(float v, float epsilon) noexcept
{
    return BASE_NS::Math::abs(v) < epsilon;
}

float CubicBezierEasingCurve::GetLinearX(float t) const noexcept
{
    static constexpr int newtonMaxIter = 8;
    static constexpr float epsilon = 0.001f;

    if (t < 0) {
        return 0.f;
    }
    if (t > 1.f) {
        return 1.f;
    }

    float x;
    float dx;
    float tt = t;

    // Fast iteration with Newton's method
    for (int i = 0; i < newtonMaxIter; i++) {
        if (x = GetX(tt) - t; IsFloatNull(x, epsilon)) {
            return tt;
        }
        if (dx = GetDX(tt); IsFloatNull(dx, BASE_NS::Math::EPSILON)) {
            break;
        }
        tt = tt - x / dx;
    }

    // Didn't get a result with fast iteration, fallback to linear bisection
    float t0 = 0.f;
    float t1 = 1.f;

    while (t0 < t1) {
        if (x = GetX(tt); IsFloatNull(x - t, epsilon)) {
            return tt;
        }
        if (t > x) {
            t0 = tt;
        } else {
            t1 = tt;
        }
        tt = (t1 - t0) * .5f + t0;
    }

    // Fallback
    return tt;
}

} // namespace Easing
} // namespace Curves

META_END_NAMESPACE()
