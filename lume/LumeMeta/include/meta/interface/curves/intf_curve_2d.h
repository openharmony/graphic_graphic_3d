/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: ICurve2D API for 2D curves.
 * Author: Lauri Jaaskela
 * Create: 2021-10-05
 */
#ifndef META_INTERFACE_ICURVE_2D_H
#define META_INTERFACE_ICURVE_2D_H

#include <base/math/vector.h>

#include <meta/base/namespace.h>
#include <meta/interface/curves/intf_curve.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ICurve2D, "1faeb3a5-54bc-461f-aaf0-7c5097167ba1")

/**
 * @brief ICurve is the base interface for all curves which transform a
 *        2D curve along an axis.
 */
class ICurve2D : public ICurve<BASE_NS::Math::Vec2> {
    META_INTERFACE(ICurve<BASE_NS::Math::Vec2>, ICurve2D)
public:
    /**
     * @brief The transformation function for this curve.
     * @param t The point along the curve where the transformation should happen.
     * @return Transformed value.
     */
    virtual BASE_NS::Math::Vec2 Transform(float t) const override = 0;
};

META_END_NAMESPACE()

#endif
