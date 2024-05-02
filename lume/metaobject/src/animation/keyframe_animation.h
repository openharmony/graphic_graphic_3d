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

#ifndef META_SRC_ANIMATION_KEYFRAME_ANIMATION_H
#define META_SRC_ANIMATION_KEYFRAME_ANIMATION_H

#include "animation.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class KeyframeAnimation final : public PropertyAnimationFwd<KeyframeAnimation, META_NS::ClassId::KeyframeAnimation,
                                    IKeyframeAnimation, IStartableAnimation> {
    using Super = PropertyAnimationFwd<KeyframeAnimation, META_NS::ClassId::KeyframeAnimation, IKeyframeAnimation,
        IStartableAnimation>;

public:
    KeyframeAnimation() = default;
    ~KeyframeAnimation() override = default;

protected: // IKeyframeAnimation
    META_IMPLEMENT_INTERFACE_PROPERTY(IKeyframeAnimation, IAny::Ptr, From);
    META_IMPLEMENT_INTERFACE_PROPERTY(IKeyframeAnimation, IAny::Ptr, To);

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
