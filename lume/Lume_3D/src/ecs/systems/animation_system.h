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

#ifndef CORE_ECS_ANIMATIONSYSTEM_H
#define CORE_ECS_ANIMATIONSYSTEM_H

#include <ComponentTools/component_query.h>
#include <PropertyTools/property_api_impl.h>

#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/namespace.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <core/ecs/entity.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>

#include "ecs/components/initial_transform_component.h"

CORE_BEGIN_NAMESPACE()
class IEcs;
class IPropertyApi;
class IPropertyHandle;
struct PropertyTypeDecl;
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
class AnimationPlayback;
struct AnimationTrackComponent;
struct AnimationOutputComponent;
struct AnimationStateComponent;

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

    struct Properties {
        size_t minTaskSize;
    };

    struct PropertyEntry;
    struct InterpolationData;

private:
    struct Data {
        // for checking the type of the data
        const CORE_NS::PropertyTypeDecl* property;
        // for accessing the data. data() include offset and size() equals sizeof(type)
        BASE_NS::array_view<uint8_t> data;
        // for calling WUnlock automatically when data is not needed
        CORE_NS::ScopedHandle<uint8_t> handle;
    };

    struct FrameData {
        float currentOffset;
        size_t currentFrameIndex;
        size_t nextFrameIndex;
    };

    struct TrackValues {
        InitialTransformComponent initial;
        InitialTransformComponent result;
        float timePosition { 0.f };
        float weight { 0.f };
        bool stopped { true };
        bool forward { true };
        bool updated { false };
    };

    class InitTask;
    class AnimateTask;

    // IEcs::ComponentListener
    void OnComponentEvent(EventType type, const CORE_NS::IComponentManager& componentManager,
        BASE_NS::array_view<const CORE_NS::Entity> entities) override;
    void OnAnimationComponentsCreated(BASE_NS::array_view<const CORE_NS::Entity> entities);
    void OnAnimationComponentsUpdated(BASE_NS::array_view<const CORE_NS::Entity> entities);
    void OnAnimationTrackComponentsUpdated(BASE_NS::array_view<const CORE_NS::Entity> entities);

    void UpdateAnimation(AnimationStateComponent& state, const AnimationComponent& animation,
        const CORE_NS::ComponentQuery& trackQuery, float delta);
    void InitializeTrackValues();
    void ResetToInitialTrackValues();
    void WriteUpdatedTrackValues();

    void InitializeTrackValues(BASE_NS::array_view<const uint32_t> resultIndices);
    void ResetTargetProperties(BASE_NS::array_view<const uint32_t> resultIndices);
    void Calculate(BASE_NS::array_view<const uint32_t> resultIndices);
    void AnimateTracks(BASE_NS::array_view<const uint32_t> resultIndices);
    void ApplyResults(BASE_NS::array_view<const uint32_t> resultIndices);

    const PropertyEntry& GetEntry(const AnimationTrackComponent& track);
    void InitializeInitialDataComponent(CORE_NS::Entity trackEntity, const AnimationTrackComponent& animationTrack);

    CORE_NS::IEcs& ecs_;
    bool active_ = true;
    Properties systemProperties_ { 128U };
    CORE_NS::PropertyApiImpl<Properties> systemPropertyApi_;

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
    uint32_t animationGeneration_ {};

    BASE_NS::vector<uint32_t> animationOrder_;
    BASE_NS::vector<uint32_t> trackOrder_;
    BASE_NS::vector<PropertyEntry> propertyCache_;

    CORE_NS::IThreadPool::Ptr threadPool_;

    size_t taskSize_ { 0U };
    size_t tasks_ { 0U };
    size_t remaining_ { 0U };

    uint64_t taskId_ { 0U };

    BASE_NS::vector<CORE_NS::IThreadPool::IResult::Ptr> taskResults_;

    BASE_NS::vector<InitTask> initTasks_;
    uint64_t initTaskStart_ { 0U };
    uint64_t initTaskCount_ { 0U };

    BASE_NS::vector<AnimateTask> animTasks_;
    uint64_t animTaskStart_ { 0U };
    uint64_t animTaskCount_ { 0U };

    BASE_NS::vector<TrackValues> trackValues_;
    BASE_NS::vector<FrameData> frameIndices_;
};

/** @ingroup group_ecs_systems_ianimation */
/** Return name of this system
 */
inline constexpr BASE_NS::string_view GetName(const IAnimationSystem*)
{
    return "AnimationSystem";
}
CORE3D_END_NAMESPACE()

#endif // CORE_ECS_ANIMATIONSYSTEM_H
