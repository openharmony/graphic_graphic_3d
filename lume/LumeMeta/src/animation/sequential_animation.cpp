/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Sequential animation implementations
 * Author: Lauri Jaaskela
 * Create: 2023-12-20
 */

#include "sequential_animation.h"

#include <meta/api/util.h>

META_BEGIN_NAMESPACE()

namespace Internal {

AnimationState::AnimationStateParams SequentialAnimation::GetParams()
{
    AnimationState::AnimationStateParams params;
    params.owner = GetSelf<IAnimation>();
    params.runningProperty = META_ACCESS_PROPERTY(Running);
    params.progressProperty = META_ACCESS_PROPERTY(Progress);
    params.totalDuration = META_ACCESS_PROPERTY(TotalDuration);
    return params;
}

bool SequentialAnimation::Build(const IMetadata::Ptr& data)
{
    if (Super::Build(data)) {
        if (auto changed = GetState().OnChildrenChanged()) {
            changed->AddHandler(MakeCallback<IOnChanged>(this, &SequentialAnimation::ChildrenChanged));
        }
        return true;
    }
    return false;
}

void SequentialAnimation::Evaluate()
{
    if (GetState().Evaluate() == AnyReturn::SUCCESS) {
        NotifyChanged();
    }
}

void SequentialAnimation::ChildrenChanged()
{
    SetValue(META_ACCESS_PROPERTY(Valid), GetState().IsValid());
    NotifyChanged();
}

} // namespace Internal

META_END_NAMESPACE()
