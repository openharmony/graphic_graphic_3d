/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Manual Clock implementation
 * Author: Slawomir Grabowski
 * Create: 2022-07-20
 */

#include <meta/base/namespace.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_manual_clock.h>

#include "base_object.h"

META_BEGIN_NAMESPACE()

class ManualClock final : public IntroduceInterfaces<BaseObject, IManualClock> {
    META_OBJECT(ManualClock, ClassId::ManualClock, BaseObject)
public:
    [[nodiscard]] TimeSpan GetTime() const override
    {
        return time_;
    }
    void SetTime(const META_NS::TimeSpan& time) override
    {
        time_ = time;
    }

    void IncrementTime(const META_NS::TimeSpan& time) override
    {
        time_ += time;
    }

private:
    TimeSpan time_;
};

IObjectFactory::Ptr GetManualClockFactory()
{
    return ManualClock::GetFactory();
}

META_END_NAMESPACE()
