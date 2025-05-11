/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Property animation implementations
 * Author: Lauri Jaaskela
 * Create: 2023-12-20
 */

#ifndef META_SRC_ANIMATION_PROPERTY_ANIMATION_H
#define META_SRC_ANIMATION_PROPERTY_ANIMATION_H

#include <meta/interface/property/property.h>

#include "animation.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class PropertyAnimation final : public PropertyAnimationFwd<ITimedAnimation> {
    META_OBJECT(PropertyAnimation, META_NS::ClassId::PropertyAnimation, PropertyAnimationFwd)
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
    void OnAnimationStateChanged(const IAnimationInternal::AnimationStateChangedInfo& info) override;
    void Evaluate() override;
    AnimationState::AnimationStateParams GetParams() override;
    void OnPropertyChanged(const TargetProperty& property, const IStackProperty::Ptr& previous) override;

    IAny::Ptr from_;
    IAny::Ptr to_;
    IAny::Ptr currentValue_;
    bool evalChanged_ {};
};

} // namespace Internal

META_END_NAMESPACE()

#endif // META_SRC_ANIMATION_PROPERTY_ANIMATION_H
