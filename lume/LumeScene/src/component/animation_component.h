
#ifndef SCENE_SRC_COMPONENT_ANIMATION_COMPONENT_H
#define SCENE_SRC_COMPONENT_ANIMATION_COMPONENT_H

#include <scene/ext/component.h>

#include <3d/ecs/components/animation_component.h>
#include <core/ecs/entity_reference.h>

#include <meta/base/meta_types.h>
#include <meta/ext/object.h>

META_TYPE(CORE3D_NS::AnimationComponent::PlaybackState)

SCENE_BEGIN_NAMESPACE()

class IInternalAnimation : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInternalAnimation, "2674d7bb-354d-45c4-8e09-f4c5fd9aa2e4")
public:
    META_PROPERTY(CORE3D_NS::AnimationComponent::PlaybackState, State)
    META_PROPERTY(uint32_t, RepeatCount)
    META_PROPERTY(float, StartOffset)
    META_PROPERTY(float, Duration)
    META_PROPERTY(float, Weight)
    META_PROPERTY(float, Speed)
    META_ARRAY_PROPERTY(CORE_NS::EntityReference, Tracks)

    // Part of AnimationStateComponent
    META_PROPERTY(float, Time)
    META_READONLY_PROPERTY(bool, Completed);
};

META_REGISTER_CLASS(
    AnimationComponent, "8f89820a-6484-4a5e-9ee3-814ed2a1f8f9", META_NS::ObjectCategoryBits::NO_CATEGORY)

class AnimationComponent : public META_NS::IntroduceInterfaces<Component, IInternalAnimation> {
    META_OBJECT(AnimationComponent, SCENE_NS::ClassId::AnimationComponent, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_PROPERTY_DATA(
        IInternalAnimation, CORE3D_NS::AnimationComponent::PlaybackState, State, "AnimationComponent.state")
    SCENE_STATIC_PROPERTY_DATA(IInternalAnimation, uint32_t, RepeatCount, "AnimationComponent.repeatCount")
    SCENE_STATIC_PROPERTY_DATA(IInternalAnimation, float, StartOffset, "AnimationComponent.startOffset")
    SCENE_STATIC_PROPERTY_DATA(IInternalAnimation, float, Duration, "AnimationComponent.duration")
    SCENE_STATIC_PROPERTY_DATA(IInternalAnimation, float, Weight, "AnimationComponent.weight")
    SCENE_STATIC_PROPERTY_DATA(IInternalAnimation, float, Speed, "AnimationComponent.speed")
    SCENE_STATIC_ARRAY_PROPERTY_DATA(IInternalAnimation, CORE_NS::EntityReference, Tracks, "AnimationComponent.tracks")
    SCENE_STATIC_PROPERTY_DATA(IInternalAnimation, float, Time, "AnimationStateComponent.time")
    SCENE_STATIC_PROPERTY_DATA(IInternalAnimation, bool, Completed, "AnimationStateComponent.completed")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(CORE3D_NS::AnimationComponent::PlaybackState, State)
    META_IMPLEMENT_PROPERTY(uint32_t, RepeatCount)
    META_IMPLEMENT_PROPERTY(float, StartOffset)
    META_IMPLEMENT_PROPERTY(float, Duration)
    META_IMPLEMENT_PROPERTY(float, Weight)
    META_IMPLEMENT_PROPERTY(float, Speed)
    META_IMPLEMENT_ARRAY_PROPERTY(CORE_NS::EntityReference, Tracks)

    META_IMPLEMENT_PROPERTY(float, Time)
    META_IMPLEMENT_READONLY_PROPERTY(bool, Completed)
public:
    BASE_NS::string GetName() const override;
    bool SetEcsObject(const IEcsObject::Ptr& obj) override;
};

SCENE_END_NAMESPACE()

#endif
