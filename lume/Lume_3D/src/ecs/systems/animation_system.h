/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef CORE_ECS_ANIMATIONSYSTEM_H
#define CORE_ECS_ANIMATIONSYSTEM_H

#include <ComponentTools/component_query.h>

#include <3d/ecs/systems/intf_animation_system.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <core/ecs/entity.h>
#include <core/namespace.h>

#include "ecs/components/animation_state_component.h"
#include "ecs/systems/animation_playback.h"

CORE_BEGIN_NAMESPACE()
class IEcs;
class IPropertyApi;
class IPropertyHandle;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IAnimationPlayback;
class INameComponentManager;
class ISceneNode;
class IAnimationComponentManager;
class IAnimationInputComponentManager;
class IAnimationOutputComponentManager;
class IAnimationStateComponentManager;
class IAnimationTrackComponentManager;
class IInitialTransformComponentManager;
struct AnimationTrackComponent;

class AnimationSystem final : public IAnimationSystem, CORE_NS::IEcs::ComponentListener {
public:
    explicit AnimationSystem(CORE_NS::IEcs& ecs);
    ~AnimationSystem() override = default;

    BASE_NS::string_view GetName() const override;
    BASE_NS::Uid GetUid() const override;

    CORE_NS::IPropertyHandle* GetProperties() override;
    const CORE_NS::IPropertyHandle* GetProperties() const override;
    void SetProperties(const CORE_NS::IPropertyHandle&) override;

    bool IsActive() const override;
    void SetActive(bool state) override;

    void Initialize() override;
    bool Update(bool frameRenderingQueued, uint64_t time, uint64_t delta) override;
    void Uninitialize() override;

    const CORE_NS::IEcs& GetECS() const override;

    // IAnimationSystem.
    IAnimationPlayback* CreatePlayback(CORE_NS::Entity const& animationEntity, ISceneNode const& node) override;
    IAnimationPlayback* CreatePlayback(const CORE_NS::Entity animationEntity,
        const BASE_NS::array_view<const CORE_NS::Entity> targetEntities) override;
    void DestroyPlayback(IAnimationPlayback* playback) override;

    size_t GetPlaybackCount() const override;
    IAnimationPlayback* GetPlayback(size_t index) const override;

private:
    struct Data {
        // for checking the type of the data
        const CORE_NS::PropertyTypeDecl* property;
        // for accessing the data. data() include offset and size() equals sizeof(type)
        BASE_NS::array_view<uint8_t> data;
        // for calling WUnlock automatically when data is not needed
        CORE_NS::ScopedHandle<uint8_t> handle;
    };

    // IEcs::ComponentListener
    void OnComponentEvent(EventType type, const CORE_NS::IComponentManager& componentManager,
        BASE_NS::array_view<const CORE_NS::Entity> entities) override;
    void OnAnimationComponentsCreated(BASE_NS::array_view<const CORE_NS::Entity> entities);
    void OnAnimationComponentsUpdated(BASE_NS::array_view<const CORE_NS::Entity> entities);
    void OnAnimationTrackComponentsUpdated(BASE_NS::array_view<const CORE_NS::Entity> entities);

    void NotifyAnimationStarted(CORE_NS::Entity entity);
    void NotifyAnimationFinished(CORE_NS::Entity entity);
    void NotifyAnimationStateChanged();

    BASE_NS::array_view<const uint8_t> GetInitialValue(CORE_NS::IComponentManager::ComponentId componentId);
    Data GetAnimatedProperty(CORE_NS::IComponentManager::ComponentId componentId);

    void Update(AnimationStateComponent& state, const AnimationComponent& animation,
        const CORE_NS::ComponentQuery& query, float delta);
    bool UpdateTracks(
        AnimationStateComponent& state, const AnimationComponent& animation, const CORE_NS::ComponentQuery& query);
    void ResetTime(AnimationStateComponent& state, const AnimationComponent& animation);
    void AnimateTrack(const CORE_NS::ComponentQuery& query, AnimationStateComponent::TrackState& track,
        AnimationTrackComponent const& animationTrack, float offset, float weight, size_t currentFrameIndex,
        size_t nextFrameIndex);

    CORE_NS::IEcs& ecs_;
    bool active_ = true;

    BASE_NS::vector<BASE_NS::unique_ptr<AnimationPlayback>> animations_;

    IInitialTransformComponentManager& initialTransformManager_;
    IAnimationComponentManager& animationManager_;
    IAnimationInputComponentManager& inputManager_;
    IAnimationOutputComponentManager& outputManager_;
    IAnimationStateComponentManager& stateManager_;
    IAnimationTrackComponentManager& animationTrackManager_;
    INameComponentManager& nameManager_;

    CORE_NS::ComponentQuery trackQuery_;
    CORE_NS::ComponentQuery animationQuery_;
    uint32_t stateGeneration_ {};
};
CORE3D_END_NAMESPACE()

#endif // CORE_ECS_ANIMATIONSYSTEM_H
