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
#ifndef META_SRC_CUBIC_BEZIER_EASING_CURVE_H
#define META_SRC_CUBIC_BEZIER_EASING_CURVE_H

#include <meta/base/namespace.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/curves/intf_bezier.h>
#include <meta/interface/curves/intf_easing_curve.h>

#include "../meta_object.h"

META_BEGIN_NAMESPACE()

namespace Curves {
namespace Easing {

class CubicBezierEasingCurve final : public META_NS::Internal::MetaObjectFwd<CubicBezierEasingCurve,
                                         ClassId::CubicBezierEasingCurve, IEasingCurve, ICubicBezier> {
    using Super = META_NS::Internal::MetaObjectFwd<CubicBezierEasingCurve, ClassId::CubicBezierEasingCurve,
        IEasingCurve, ICubicBezier>;
    META_IMPLEMENT_INTERFACE_PROPERTY(ICubicBezier, BASE_NS::Math::Vec2, ControlPoint1, BASE_NS::Math::Vec2(0, 0))
    META_IMPLEMENT_INTERFACE_PROPERTY(ICubicBezier, BASE_NS::Math::Vec2, ControlPoint2, BASE_NS::Math::Vec2(1, 1))

public:
    bool Build(const IMetadata::Ptr& meta) override;
    float Transform(float t) const override;

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
