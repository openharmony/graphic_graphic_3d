/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Clock implementation based on std::chrono
 * Author: Slawomir Grabowski
 * Create: 2022-07-19
 */

#include <chrono>

#include <meta/base/namespace.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_clock.h>

#include "base_object.h"

META_BEGIN_NAMESPACE()

class SystemClock : public IntroduceInterfaces<BaseObject, IClock> {
    META_OBJECT(SystemClock, ClassId::SystemClock, IntroduceInterfaces)
public:
    TimeSpan GetTime() const override
    {
        return TimeSpan::Microseconds(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
                                          .count());
    };
};

IObjectFactory::Ptr GetSystemClockFactory()
{
    return SystemClock::GetFactory();
}

META_END_NAMESPACE()
