/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Keyframe animation implementations
 * Author: Lauri Jaaskela
 * Create: 2023-12-20
 */

#ifndef META_SRC_ANIMATION_KEYFRAME_ANIMATION_H
#define META_SRC_ANIMATION_KEYFRAME_ANIMATION_H

#include "animation.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class KeyframeAnimation final
    : public IntroduceInterfaces<PropertyAnimationFwd<IKeyframeAnimation>, IStartableAnimation> {
    META_OBJECT(KeyframeAnimation, META_NS::ClassId::KeyframeAnimation, IntroduceInterfaces)
public:
    KeyframeAnimation() = default;
    ~KeyframeAnimation() override = default;

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IKeyframeAnimation, IAny::Ptr, From)
    META_STATIC_PROPERTY_DATA(IKeyframeAnimation, IAny::Ptr, To)
    META_END_STATIC_DATA()
    META_IMPLEMENT_PROPERTY(IAny::Ptr, From)
    META_IMPLEMENT_PROPERTY(IAny::Ptr, To)

protected: // IStartableAnimation
    void Pause() override;
    void Restart() override;
    void Seek(float position) override;
    void Start() override;
    void Stop() override;
    void Finish() override;

public: // IModifier
    EvaluationResult ProcessOnGet(IAny& value) override;

protected: // IAnimationInternal
    void OnAnimationStateChanged(const AnimationStateChangedInfo& info) override;

private:
    void Evaluate() override;
    AnimationState::AnimationStateParams GetParams() override;
    void OnPropertyChanged(const TargetProperty& property, const IStackProperty::Ptr& previous) override;
    void Initialize();
    IAny::Ptr currentValue_;
};

} // namespace Internal

META_END_NAMESPACE()

#endif // META_SRC_KEYFRAME_ANIMATION_H
