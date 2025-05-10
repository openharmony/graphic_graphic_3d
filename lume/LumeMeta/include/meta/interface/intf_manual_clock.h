/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Definition of IManualClock interface
 * Author: Slawomir Grabowski
 * Create: 2022-07-20
 */

#ifndef META_INTERFACE_IMANUAL_CLOCK_H
#define META_INTERFACE_IMANUAL_CLOCK_H

#include <meta/base/namespace.h>
#include <meta/base/time_span.h>
#include <meta/interface/intf_clock.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IManualClock, "85b85ed8-014e-467f-bbd9-7a8829b340a2")

/**
 * @brief Clock for which you can explicitly set the current time.
 */
class IManualClock : public IClock {
    META_INTERFACE(IClock, IManualClock)
public:
    /**
     * @brief Get current time.
     */
    [[nodiscard]] META_NS::TimeSpan GetTime() const override = 0;
    /**
     * @brief Set current time.
     */
    virtual void SetTime(const META_NS::TimeSpan& time) = 0;
    /**
     * @brief Increment the current time.
     */
    virtual void IncrementTime(const META_NS::TimeSpan& time) = 0;
};

META_END_NAMESPACE()

#endif
