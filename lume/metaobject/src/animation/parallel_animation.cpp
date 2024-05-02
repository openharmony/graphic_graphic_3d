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

#include "parallel_animation.h"

#include <meta/api/util.h>

META_BEGIN_NAMESPACE()

namespace Internal {

AnimationState::AnimationStateParams ParallelAnimation::GetParams()
{
    AnimationState::AnimationStateParams params;
    params.owner = GetSelf<IAnimation>();
    params.runningProperty = META_ACCESS_PROPERTY(Running);
    params.progressProperty = META_ACCESS_PROPERTY(Progress);
    params.totalDuration = META_ACCESS_PROPERTY(TotalDuration);
    return params;
}

bool ParallelAnimation::Build(const IMetadata::Ptr& data)
{
    if (Super::Build(data)) {
        GetState().OnChildrenChanged()->AddHandler(MakeCallback<IOnChanged>(this, &ParallelAnimation::ChildrenChanged));
        return true;
    }
    return false;
}

void ParallelAnimation::Evaluate()
{
    if (GetState().Evaluate() == AnyReturn::SUCCESS) {
        NotifyChanged();
    }
}

void ParallelAnimation::ChildrenChanged()
{
    SetValue(META_ACCESS_PROPERTY(Valid), GetState().IsValid());
    NotifyChanged();
}

} // namespace Internal

META_END_NAMESPACE()
