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

#ifndef META_SRC_ANIMATION_PROPERTY_ANIMATION_H
#define META_SRC_ANIMATION_PROPERTY_ANIMATION_H

#include <meta/interface/property/property.h>

#include "animation.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class PropertyAnimation final
    : public PropertyAnimationFwd<PropertyAnimation, META_NS::ClassId::PropertyAnimation, ITimedAnimation> {
    using Super = PropertyAnimationFwd<PropertyAnimation, META_NS::ClassId::PropertyAnimation, ITimedAnimation>;

public:
    PropertyAnimation() = default;
    ~PropertyAnimation() override = default;

protected:
    bool Build(const IMetadata::Ptr& meta) override;

protected: // IAnimation
    void Step(const IClock::ConstPtr& clock) override;

public: // IModifier
    EvaluationResult ProcessOnGet(IAny& value) override;
    EvaluationResult ProcessOnSet(IAny& value, const IAny& current) override;

private:
    void Evaluate() override;
    AnimationState::AnimationStateParams GetParams() override;
    void OnPropertyChanged(const TargetProperty& property, const IStackProperty::Ptr& previous) override;

    IAny::Ptr from_;
    IAny::Ptr to_;
    IAny::Ptr currentValue_;
};

} // namespace Internal

META_END_NAMESPACE()

#endif // META_SRC_ANIMATION_PROPERTY_ANIMATION_H
