/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Parallel animation implementations
 * Author: Lauri Jaaskela
 * Create: 2023-12-20
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
        if (auto changed = GetState().OnChildrenChanged()) {
            changed->AddHandler(MakeCallback<IOnChanged>(this, &ParallelAnimation::ChildrenChanged));
        }
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
