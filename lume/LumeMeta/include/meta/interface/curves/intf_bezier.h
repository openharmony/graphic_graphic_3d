/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: IBezier interface
 * Author: Lauri Jaaskela
 * Create: 2023-08-08
 */
#ifndef META_INTERFACE_IBEZIER_H
#define META_INTERFACE_IBEZIER_H

#include <base/math/vector.h>

#include <meta/ext/implementation_macros.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ICubicBezier, "e7144ea4-2abc-48d3-9302-f2ff83cdb3c5")

/**
 * @brief Interface for a cubic bezier function which interpolates between [0,1] using
 *        ControlPoint1 and ControlPoint2 as the control points for the curve.
 *        The start point is implicitly [0,0] and the end point is implicitly [1,1].
 */
class ICubicBezier : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ICubicBezier)
public:
    /**
     * @brief First control point. X coordinate must be within [0,1].
     */
    META_PROPERTY(BASE_NS::Math::Vec2, ControlPoint1)
    /**
     * @brief Second control point. X coordinate must be within [0,1].
     */
    META_PROPERTY(BASE_NS::Math::Vec2, ControlPoint2)
};

META_END_NAMESPACE()

#endif
