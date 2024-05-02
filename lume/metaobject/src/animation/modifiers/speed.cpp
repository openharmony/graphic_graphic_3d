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
#include "speed.h"

META_BEGIN_NAMESPACE()

namespace AnimationModifiers {

bool SpeedImpl::Build(const IMetadata::Ptr& data)
{
    SpeedFactor()->OnChanged()->AddHandler(META_ACCESS_EVENT(OnChanged));
    return true;
}

bool SpeedImpl::ProcessOnGetDuration(DurationData& duration) const
{
    auto speed = BASE_NS::Math::abs(META_ACCESS_PROPERTY_VALUE(SpeedFactor));
    if (speed > 0.f) {
        duration.duration = duration.duration / speed;
    } else {
        duration.duration = TimeSpan::Infinite();
    }
    return true;
}
bool SpeedImpl::ProcessOnStep(StepData& step) const
{
    auto speed = BASE_NS::Math::abs(META_ACCESS_PROPERTY_VALUE(SpeedFactor));
    if (speed < 0) {
        step.progress = 1.f - step.progress;
        step.reverse = !step.reverse;
    }
    return true;
}

} // namespace AnimationModifiers

META_END_NAMESPACE()
