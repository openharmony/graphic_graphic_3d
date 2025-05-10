/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Sequential animation implementations
 * Author: Lauri Jaaskela
 * Create: 2023-12-20
 */

#ifndef META_SRC_ANIMATION_SEQUENTIAL_ANIMATION_H
#define META_SRC_ANIMATION_SEQUENTIAL_ANIMATION_H

#include "animation.h"
#include "staggered_animation_state.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class SequentialAnimation final : public BaseAnimationContainerFwd<ISequentialAnimation> {
    META_OBJECT(SequentialAnimation, META_NS::ClassId::SequentialAnimation, BaseAnimationContainerFwd)
public:
    SequentialAnimation() = default;
    ~SequentialAnimation() override = default;

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
    Internal::SequentialAnimationState& GetState() noexcept override
    {
        return state_;
    }
    Internal::SequentialAnimationState state_;
};

} // namespace Internal

META_END_NAMESPACE()

#endif // META_SRC_ANIMATION_SEQUENTIAL_ANIMATION_H
