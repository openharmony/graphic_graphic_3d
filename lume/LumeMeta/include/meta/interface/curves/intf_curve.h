/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: ICurve API.
 * Author: Lauri Jaaskela
 * Create: 2021-10-05
 */
#ifndef META_INTERFACE_ICURVE_H
#define META_INTERFACE_ICURVE_H

#include <meta/base/namespace.h>
#include <meta/interface/intf_object.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ICurve, "9ec41bb4-d5a9-4c18-afa4-686908e76adb")

/**
 * @brief ICurve is the base interface for all curves. A curve transforms a parameter
 *        along a curve, specified by each class which implements the ICurve interface.
 */
template<class T>
class ICurve : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ICurve)
public:
    /**
     * @brief The transformation function for this curve. Transform function should be
     *        re-entrant.
     * @param t The point along the curve where the transformation should happen.
     * @return Transformed value.
     */
    virtual T Transform(float t) const = 0;
};

META_END_NAMESPACE()

#endif
