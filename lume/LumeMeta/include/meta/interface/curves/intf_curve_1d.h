/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: ICurve1D API for 1D curves.
 * Author: Lauri Jaaskela
 * Create: 2021-10-05
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
