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

#ifndef META_INTERFACE_ICURVE_1D_H
#define META_INTERFACE_ICURVE_1D_H

#include <meta/base/namespace.h>
#include <meta/interface/curves/intf_curve.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ICurve1D, "3752a2b4-0972-4af3-a20d-7832ed60c65f")

/**
 * @brief ICurve1D is the base interface for all 1D curves which transform
 *        a one-dimensional curve along an axis.
 *
 *        A specialization of ICurve1D is IEasingCurve, which adds the requirement
 *        that the easing curve starts at 0 and ends at 1.
 */
class ICurve1D : public ICurve<float> {
    META_INTERFACE(ICurve<float>, ICurve1D)
public:
    /**
     * @brief The transformation function for this curve.
     * @param t The point along the curve where the transformation should happen.
     * @return Transformed value.
     */
    float Transform(float t) const override = 0;
};

META_END_NAMESPACE()

META_TYPE(META_NS::ICurve1D::WeakPtr)
META_TYPE(META_NS::ICurve1D::Ptr)

#endif
