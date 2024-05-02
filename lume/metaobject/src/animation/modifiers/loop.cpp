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
#include "loop.h"

#include <algorithm>
#include <limits>

#include <meta/api/make_callback.h>
#include <meta/api/util.h>
#include <meta/interface/builtin_objects.h>

META_BEGIN_NAMESPACE()

namespace AnimationModifiers {

bool Loop::Build(const IMetadata::Ptr& data)
{
    LoopCount()->OnChanged()->AddHandler(META_ACCESS_EVENT(OnChanged));
    return true;
}

bool Loop::ProcessOnGetDuration(DurationData& duration) const
{
    const auto loopCount = META_ACCESS_PROPERTY_VALUE(LoopCount);
    if (loopCount == 0) {
        // Invalid
        return false;
    }
    if (loopCount > 0) {
        duration.loopCount = duration.loopCount < 0 ? duration.loopCount : duration.loopCount + loopCount - 1;
    } else {
        duration.loopCount = -1;
    }
    return true;
}

bool Loop::ProcessOnStep(StepData& step) const
{
    return true;
}

} // namespace AnimationModifiers

META_END_NAMESPACE()
