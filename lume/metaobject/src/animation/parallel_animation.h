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

#ifndef META_SRC_ANIMATION_PARALLEL_ANIMATION_H
#define META_SRC_ANIMATION_PARALLEL_ANIMATION_H

#include "animation.h"
#include "staggered_animation_state.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class ParallelAnimation final
    : public BaseAnimationContainerFwd<ParallelAnimation, META_NS::ClassId::ParallelAnimation, IParallelAnimation> {
    using Super = BaseAnimationContainerFwd<ParallelAnimation, META_NS::ClassId::ParallelAnimation, IParallelAnimation>;

public:
    ParallelAnimation() = default;
    ~ParallelAnimation() override = default;

    bool Build(const IMetadata::Ptr& data) override;

private:
    AnimationState::AnimationStateParams GetParams() override;
    void Evaluate() override;
    void ChildrenChanged();

    IContainer& GetContainer() noexcept override
    {
        return state_.GetContainerInterface();
    }
    const IContainer& GetContainer() const noexcept override
    {
        return state_.GetContainerInterface();
    }
    Internal::ParallelAnimationState& GetState() noexcept override
    {
        return state_;
    }
    Internal::ParallelAnimationState state_;
};

} // namespace Internal

META_END_NAMESPACE()

#endif // META_SRC_ANIMATION_PARALLEL_ANIMATION_H
