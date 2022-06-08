/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"
#include "ecs/components/animation_state_component.h"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE_BEGIN_NAMESPACE()
using CORE3D_NS::AnimationComponent;
using CORE3D_NS::AnimationStateComponent;
DECLARE_PROPERTY_TYPE(AnimationStateComponent::TrackState);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<AnimationStateComponent::TrackState>);

DECLARE_PROPERTY_TYPE(AnimationComponent::PlaybackState);

BEGIN_ENUM(AnimationPlaybackStateMetaData, AnimationComponent::PlaybackState)
DECL_ENUM(AnimationComponent::PlaybackState, STOP, "Stop")
DECL_ENUM(AnimationComponent::PlaybackState, PLAY, "Play")
DECL_ENUM(AnimationComponent::PlaybackState, PAUSE, "Pause")
END_ENUM(AnimationPlaybackStateMetaData, AnimationComponent::PlaybackState)
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class AnimationStateComponentManager final
    : public BaseManager<AnimationStateComponent, IAnimationStateComponentManager> {
    BEGIN_PROPERTY(AnimationStateComponent, ComponentMetadata)
#include "ecs/components/animation_state_component.h"
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit AnimationStateComponentManager(IEcs& ecs)
        : BaseManager<AnimationStateComponent, IAnimationStateComponentManager>(
              ecs, CORE_NS::GetName<AnimationStateComponent>())
    {}

    ~AnimationStateComponentManager() = default;

    size_t PropertyCount() const override
    {
        return componentMetaData_.size();
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < componentMetaData_.size()) {
            return &componentMetaData_[index];
        }
        return nullptr;
    }

    array_view<const Property> MetaData() const override
    {
        return componentMetaData_;
    }
};

IComponentManager* IAnimationStateComponentManagerInstance(IEcs& ecs)
{
    return new AnimationStateComponentManager(ecs);
}

void IAnimationStateComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<AnimationStateComponentManager*>(instance);
}
CORE_END_NAMESPACE()
