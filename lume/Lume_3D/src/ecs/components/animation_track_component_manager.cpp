/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include <3d/ecs/components/animation_track_component.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE_BEGIN_NAMESPACE()
using CORE3D_NS::AnimationTrackComponent;
DECLARE_PROPERTY_TYPE(AnimationTrackComponent::Interpolation);

BEGIN_ENUM(AnimationInterpolationMetaData, AnimationTrackComponent::Interpolation)
DECL_ENUM(AnimationTrackComponent::Interpolation, STEP, "Step")
DECL_ENUM(AnimationTrackComponent::Interpolation, LINEAR, "Linear")
DECL_ENUM(AnimationTrackComponent::Interpolation, SPLINE, "Spline")
END_ENUM(AnimationInterpolationMetaData, AnimationTrackComponent::Interpolation)
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class AnimationTrackComponentManager final
    : public BaseManager<AnimationTrackComponent, IAnimationTrackComponentManager> {
    BEGIN_PROPERTY(AnimationTrackComponent, ComponentMetadata)
#include <3d/ecs/components/animation_track_component.h>
    END_PROPERTY();
    const array_view<const Property> ComponentMetaData { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit AnimationTrackComponentManager(IEcs& ecs)
        : BaseManager<AnimationTrackComponent, IAnimationTrackComponentManager>(
              ecs, CORE_NS::GetName<AnimationTrackComponent>())
    {}

    ~AnimationTrackComponentManager() = default;

    size_t PropertyCount() const override
    {
        return ComponentMetaData.size();
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < ComponentMetaData.size()) {
            return &ComponentMetaData[index];
        }
        return nullptr;
    }

    array_view<const Property> MetaData() const override
    {
        return ComponentMetaData;
    }
};

IComponentManager* IAnimationTrackComponentManagerInstance(IEcs& ecs)
{
    return new AnimationTrackComponentManager(ecs);
}

void IAnimationTrackComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<AnimationTrackComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
