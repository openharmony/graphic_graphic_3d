/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Interpolator API
 * Author: Lauri Jaaskela
 * Create: 2022-11-28
 */

#ifndef META_INTERFACE_INTF_INTERPOLATOR_H
#define META_INTERFACE_INTF_INTERPOLATOR_H

#include <core/plugin/intf_interface.h>

#include <meta/base/namespace.h>
#include <meta/interface/curves/intf_curve_1d.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IInterpolator, "64378ee2-445e-4224-a3b0-f3c5680d0d12")

/**
 * @brief The IInterpolator interface defines an interface for objects which can interpolate
 *        between two values.
 *
 *        In most cases the Interpolate function can be re-entrant, and interpolator
 *        classes should be registered with META_REGISTER_SINGLETON_CLASS().
 */
class IInterpolator : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInterpolator, META_NS::InterfaceId::IInterpolator)
public:
    /**
     * @brief Calculate interpolated value between two values at position t.
     * @param output The output value.
     * @param from Start of the interpolation range, corresponding to t = 0.
     * @param to End of the interpolation range, corresponding to t = 1.
     * @param t The interpolation position.
     */
    virtual AnyReturnValue Interpolate(IAny& output, const IAny& from, const IAny& to, float t) const = 0;

    virtual bool IsCompatibleWith(TypeId id) const = 0;
};

META_END_NAMESPACE()

META_TYPE(META_NS::IInterpolator::Ptr)
META_TYPE(META_NS::IInterpolator::WeakPtr)

#endif // META_INTERFACE_INTF_INTERPOLATOR_H
