/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <meta/base/namespace.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_manual_clock.h>

#include "base_object.h"

META_BEGIN_NAMESPACE()

class ManualClock final : public Internal::BaseObjectFwd<ManualClock, ClassId::ManualClock, IManualClock> {
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
