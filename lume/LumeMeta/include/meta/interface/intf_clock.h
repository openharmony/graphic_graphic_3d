/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Definition of IClock interface
 * Author: Slawomir Grabowski
 * Create: 2022-07-19
 */

#ifndef META_INTERFACE_ICLOCK_H
#define META_INTERFACE_ICLOCK_H

#include <cstdint>

#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/time_span.h>
#include <meta/base/types.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IClock, "a3021cdd-379e-45aa-ab0a-4d89e8657d0f")

/**
 * @brief Interface to get current time (current time of specific clock).
 */
class IClock : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IClock)
public:
    /**
     * @brief Get current time.
     */
    virtual TimeSpan GetTime() const = 0;
};

META_END_NAMESPACE()

#endif
