/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: ITickable interface
 * Author: Lauri Jaaskela
 * Create: 2024-02-22
 */

#ifndef META_INTERFACE_ITICKABLE_H
#define META_INTERFACE_ITICKABLE_H

#include <meta/base/interface_macros.h>
#include <meta/base/time_span.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ITickable, "4ff70521-002a-4f64-8b42-20a82a11947d")

/**
 * @brief The ITickable interface can be implemented by startable objects who want notifcation
 *        from the startable object controller of a UI hierarchy during runtime.
 */
class ITickable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITickable)
public:
    /**
     * @brief Called by a startable object controller.
     * @param sinceLastTick Time since previous call.
     */
    virtual void Tick(const TimeSpan& time, const TimeSpan& sinceLastTick) = 0;
};

META_END_NAMESPACE()

#endif // META_INTERFACE_ITICKABLE_H
