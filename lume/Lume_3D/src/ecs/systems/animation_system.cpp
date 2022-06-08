/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "animation_system.h"

#include <PropertyTools/property_api_impl.inl>
#include <PropertyTools/property_data.h>
#include <algorithm>
#include <cinttypes>

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_input_component.h>
#include <3d/ecs/components/animation_output_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <base/math/quaternion_util.h>
#include <base/math/spline.h>
#include <base/util/uid_util.h>
#include <core/ecs/intf_ecs.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_plugin_register.h>

#include "ecs/components/animation_state_component.h"
#include "ecs/components/initial_transform_component.h"

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
namespace {
// values contain in-tangent, spline vertex, out-tangent.
template<typename T>
struct SplineValues {
    T inTangent;
    T splineVertex;
    T outTangent;
};

const Math::Quat& CastToQuat(const Math::Vec4& vec)
{
    return *((Math::Quat*)&vec);
}

template<class T>
T Step(const T& start, const T& end, float offset)
{
    return (offset > 0.5) ? end : start;
}

Math::Quat InterpolateQuatSplineGLTF(const Math::Vec4& start, const Math::Vec4& outTangent, const Math::Vec4& end,
    const Math::Vec4& inTangent, float offset)
{
    const Math::Vec4 vec = Math::Hermite(start, outTangent, end, inTangent, offset);
    return Normalize(CastToQuat(vec));
}

constexpr float Lerp(const float& v1, const float& v2, float t)
{
    t = Math::clamp01(t);
    return v1 + (v2 - v1) * t;
}

struct InterpolationData {
    AnimationTrackComponent::Interpolation mode;
    size_t startIndex;
    size_t endIndex;

    float t;
    float weight;
};

struct AnimationKeyDataOffsets {
    size_t startKeyOffset;
    size_t endKeyOffset;
};

template<typename T>
void Interpolate(const InterpolationData& interpolation, array_view<const uint8_t> frameValues,
    array_view<const uint8_t> initialValue, array_view<uint8_t> animatedValue)
{
    auto values = array_view(reinterpret_cast<const T*>(frameValues.data()), frameValues.size() / sizeof(T));
    if ((interpolation.startIndex < values.size()) && (interpolation.endIndex < values.size())) {
        T result;
        switch (interpolation.mode) {
            case AnimationTrackComponent::Interpolation::STEP:
                result = Step(values[interpolation.startIndex], values[interpolation.endIndex], interpolation.t);
                break;

            default:
            case AnimationTrackComponent::Interpolation::LINEAR:
                result = Lerp(values[interpolation.startIndex], values[interpolation.endIndex], interpolation.t);
                break;

            case AnimationTrackComponent::Interpolation::SPLINE: {
                auto const p0 = reinterpret_cast<const SplineValues<T>&>(values[interpolation.startIndex]);
                auto const p1 = reinterpret_cast<const SplineValues<T>&>(values[interpolation.endIndex]);
                result = Math::Hermite(p0.splineVertex, p0.outTangent, p1.splineVertex, p1.inTangent, interpolation.t);
                break;
            }
        }
        const T* initial = reinterpret_cast<const T*>(initialValue.data());
        T* target = reinterpret_cast<T*>(animatedValue.data());
        // Convert result to relative and apply weight.
        *target = *target + (result - *initial) * interpolation.weight;
    }
}

template<>
void Interpolate<Math::Quat>(const InterpolationData& interpolation, array_view<const uint8_t> frameValues,
    array_view<const uint8_t> initialValue, array_view<uint8_t> animatedValue)
{
    auto values =
        array_view(reinterpret_cast<const Math::Vec4*>(frameValues.data()), frameValues.size() / sizeof(Math::Vec4));
    if ((interpolation.startIndex < values.size()) && (interpolation.endIndex < values.size())) {
        Math::Quat result;

        switch (interpolation.mode) {
            case AnimationTrackComponent::Interpolation::STEP: {
                result = Step(CastToQuat(values[interpolation.startIndex]), CastToQuat(values[interpolation.endIndex]),
                    interpolation.t);
                break;
            }

            default:
            case AnimationTrackComponent::Interpolation::LINEAR: {
                result = Slerp(CastToQuat(values[interpolation.startIndex]), CastToQuat(values[interpolation.endIndex]),
                    interpolation.t);
                break;
            }

            case AnimationTrackComponent::Interpolation::SPLINE: {
                auto const p0 = reinterpret_cast<const SplineValues<Math::Vec4>*>(&values[interpolation.startIndex]);
                auto const p1 = reinterpret_cast<const SplineValues<Math::Vec4>*>(&values[interpolation.endIndex]);
                result = InterpolateQuatSplineGLTF(
                    p0->splineVertex, p0->outTangent, p1->splineVertex, p1->inTangent, interpolation.t);
                break;
            }
        }
        const Math::Quat* initial = reinterpret_cast<const Math::Quat*>(initialValue.data());
        const Math::Quat inverseInitialRotation = Inverse(*initial);
        constexpr Math::Quat identity(0.0f, 0.0f, 0.0f, 1.0f);
        const Math::Quat delta = Normalize(Slerp(identity, result * inverseInitialRotation, interpolation.weight));

        Math::Quat* target = reinterpret_cast<Math::Quat*>(animatedValue.data());
        *target = delta * *target;
    }
}

void AnimateMorph(
    const InterpolationData& interpolation, array_view<const uint8_t> frameValues, array_view<uint8_t> animatedValue)
{
    auto values = array_view(reinterpret_cast<const float*>(frameValues.data()), frameValues.size() / sizeof(float));
    if ((interpolation.startIndex < values.size()) && (interpolation.endIndex < values.size())) {
        vector<float>& animated = *reinterpret_cast<vector<float>*>(animatedValue.data());

        const auto targetCount = animated.size();

        // Re-calculate the start/end offsets.
        const size_t startDataOffset = interpolation.startIndex * targetCount;
        const size_t endDataOffset = interpolation.endIndex * targetCount;

        switch (interpolation.mode) {
            case AnimationTrackComponent::Interpolation::STEP: {
                auto const p0 = array_view(values.data() + startDataOffset, targetCount);
                auto const p1 = array_view(values.data() + endDataOffset, targetCount);
                std::transform(p0.begin(), p0.end(), p1.begin(), animated.begin(),
                    [t = interpolation.t, weight = interpolation.weight](
                        const auto p0, const auto p1) { return weight * Step(p0, p1, t); });
                break;
            }

            case AnimationTrackComponent::Interpolation::SPLINE: {
                auto const p0 = array_view(
                    reinterpret_cast<const SplineValues<float>*>(values.data() + startDataOffset), targetCount);
                auto const p1 = array_view(
                    reinterpret_cast<const SplineValues<float>*>(values.data() + endDataOffset), targetCount);
                std::transform(p0.begin(), p0.end(), p1.begin(), animated.begin(),
                    [t = interpolation.t, weight = interpolation.weight](const auto& p0, const auto& p1) {
                        return weight * Math::Hermite(p0.splineVertex, p0.outTangent, p1.splineVertex, p1.inTangent, t);
                    });

                break;
            }
            case AnimationTrackComponent::Interpolation::LINEAR:
            default: {
                auto const p0 = array_view(values.data() + startDataOffset, targetCount);
                auto const p1 = array_view(values.data() + endDataOffset, targetCount);
                std::transform(p0.begin(), p0.end(), p1.begin(), animated.begin(),
                    [t = interpolation.t, weight = interpolation.weight](
                        const auto p0, const auto p1) { return weight * Math::lerp(p0, p1, t); });
                break;
            }
        }
    }
}

void FindFrameIndices(const bool forward, const float currentTimestamp, const array_view<const float> timestamps,
    size_t& currentFrameIndex, size_t& nextFrameIndex)
{
    size_t current;
    size_t next;
    const auto begin = timestamps.begin();
    const auto end = timestamps.end();
    const auto pos = std::upper_bound(begin, end, currentTimestamp);
    if (forward) {
        // Find next frame (forward).
        next = std::distance(begin, pos);
        current = next - 1;
    } else {
        // Find next frame (backward).
        current = std::distance(begin, pos);
        next = current ? current - 1 : 0;
    }

    // Clamp timestamps to valid range.
    currentFrameIndex = std::clamp(current, size_t(0), timestamps.size() - 1);
    nextFrameIndex = std::clamp(next, size_t(0), timestamps.size() - 1);
}

void InitializeInitialDataComponent(Entity trackEntity, IEcs& ecs, const AnimationTrackComponent& animationTrack,
    IInitialTransformComponentManager& initialTransformManager)
{
    if (auto targetManager = ecs.GetComponentManager(animationTrack.component); targetManager) {
        if (IPropertyHandle* targetHandle = targetManager->GetData(animationTrack.target); targetHandle) {
            PropertyData propertyData;
            if (auto po = propertyData.RLock(*targetHandle, animationTrack.property); po) {
                initialTransformManager.Create(trackEntity);
                auto dstHandle = initialTransformManager.Write(trackEntity);
                dstHandle->references = 1;
                const auto* containerMethods = po.property->metaData.containerMethods;
                if (!containerMethods) {
                    auto src = reinterpret_cast<uint8_t*>(po.offset);
                    dstHandle->initialData.resize(po.property->size);
                    CloneData(dstHandle->initialData.data(), dstHandle->initialData.size(), src, po.property->size);
                } else {
                    const auto items = containerMethods->size(po.offset);
                    if (items) {
                        const auto byteSize = items * containerMethods->property.size;
                        dstHandle->initialData.resize(byteSize);
                        const auto first = containerMethods->get(po.offset, 0);
                        CloneData(dstHandle->initialData.data(), dstHandle->initialData.size(),
                            reinterpret_cast<const void*>(first), byteSize);
                    }
                }
            }
        }
    }
}
void RestoreFromInitialDataComponent(const IEcs& ecs, const AnimationTrackComponent& animationTrack,
    const IInitialTransformComponentManager& initialTransformManager, IComponentManager::ComponentId initialComponentId)
{
    if (auto targetManager = ecs.GetComponentManager(animationTrack.component); targetManager) {
        if (auto* targetHandle = targetManager->GetData(animationTrack.target); targetHandle) {
            PropertyData propertyData;
            if (auto po = propertyData.WLock(*targetHandle, animationTrack.property); po) {
                auto srcHandle = initialTransformManager.Read(initialComponentId);
                const auto* containerMethods = po.property->metaData.containerMethods;
                if (!containerMethods) {
                    auto dst = reinterpret_cast<uint8_t*>(po.offset);
                    CloneData(dst, po.property->size, srcHandle->initialData.data(), srcHandle->initialData.size());
                } else {
                    const auto items = containerMethods->size(po.offset);
                    if (items) {
                        auto dst = reinterpret_cast<uint8_t*>(containerMethods->get(po.offset, 0U));
                        const auto byteSize = items * containerMethods->property.size;
                        CloneData(dst, byteSize, srcHandle->initialData.data(), srcHandle->initialData.size());
                    }
                }
            }
        }
    }
}

float UpdateStateAndTracks(IAnimationStateComponentManager& stateManager_,
    IAnimationTrackComponentManager& trackManager_, IAnimationInputComponentManager& inputManager_,
    Entity animationEntity, const AnimationComponent& animationComponent, array_view<const Entity> targetEntities)
{
    float maxLength = 0.f;
    auto stateHandle = stateManager_.Write(animationEntity);
    stateHandle->trackStates.reserve(animationComponent.tracks.size());

    auto& entityManager = stateManager_.GetEcs().GetEntityManager();
    auto targetIt = targetEntities.begin();
    for (const auto& trackEntity : animationComponent.tracks) {
        stateHandle->trackStates.push_back({ Entity(trackEntity), 0U });
        auto& trackState = stateHandle->trackStates.back();

        if (auto track = trackManager_.Write(trackEntity); track) {
            if (track->target) {
                CORE_LOG_D(
                    "AnimationTrack %s already targetted", to_hex(static_cast<const Entity&>(track->target).id).data());
            }
            track->target = entityManager.GetReferenceCounted(*targetIt);

            if (auto inputData = inputManager_.Read(track->timestamps); inputData) {
                trackState.length = inputData->timestamps.size();
                if (trackState.length > 0) {
                    const float trackLength = inputData->timestamps[inputData->timestamps.size() - 1];
                    if (trackLength > maxLength) {
                        maxLength = trackLength;
                    }
                }
            }
        }
        ++targetIt;
    }
    stateHandle->length = maxLength;
    return maxLength;
}
} // namespace

AnimationSystem::AnimationSystem(IEcs& ecs)
    : ecs_(ecs), initialTransformManager_(*(GetManager<IInitialTransformComponentManager>(ecs_))),
      animationManager_(*(GetManager<IAnimationComponentManager>(ecs_))),
      inputManager_(*(GetManager<IAnimationInputComponentManager>(ecs_))),
      outputManager_(*(GetManager<IAnimationOutputComponentManager>(ecs_))),
      stateManager_(*(GetManager<IAnimationStateComponentManager>(ecs_))),
      animationTrackManager_(*(GetManager<IAnimationTrackComponentManager>(ecs_))),
      nameManager_(*(GetManager<INameComponentManager>(ecs_)))
{}

void AnimationSystem::SetActive(bool state)
{
    active_ = state;
}

bool AnimationSystem::IsActive() const
{
    return active_;
}

string_view AnimationSystem::GetName() const
{
    return CORE3D_NS::GetName(this);
}

Uid AnimationSystem::GetUid() const
{
    return UID;
}

IPropertyHandle* AnimationSystem::GetProperties()
{
    return nullptr;
}

const IPropertyHandle* AnimationSystem::GetProperties() const
{
    return nullptr;
}

void AnimationSystem::SetProperties(const IPropertyHandle&) {}

const IEcs& AnimationSystem::GetECS() const
{
    return ecs_;
}

void AnimationSystem::Initialize()
{
    ecs_.AddListener(animationManager_, *this);
    ecs_.AddListener(animationTrackManager_, *this);
    {
        const ComponentQuery::Operation operations[] = {
            { animationTrackManager_, ComponentQuery::Operation::REQUIRE },
        };
        trackQuery_.SetEcsListenersEnabled(true);
        trackQuery_.SetupQuery(initialTransformManager_, operations, true);
    }
    {
        const ComponentQuery::Operation operations[] = {
            { stateManager_, ComponentQuery::Operation::REQUIRE },
        };
        animationQuery_.SetEcsListenersEnabled(true);
        animationQuery_.SetupQuery(animationManager_, operations, false);
    }
}

bool AnimationSystem::Update(bool frameRenderingQueued, uint64_t time, uint64_t delta)
{
    if (!active_) {
        return false;
    }

    if ((stateGeneration_ == stateManager_.GetGenerationCounter()) && [](const auto& stateManager) -> bool {
            const auto states = stateManager.GetComponentCount();
            for (auto id = 0U; id < states; ++id) {
                if (stateManager.Read(id)->dirty) {
                    return false;
                }
            }
            return true;
        }(stateManager_)) {
        return false;
    }

    animationQuery_.Execute();
    trackQuery_.Execute();

    // Start animation from initial value.
    for (const auto& row : animationQuery_.GetResults()) {
        auto animationHandle = animationManager_.Read(row.components[0]);
        for (const auto& track : animationHandle->tracks) {
            if (const auto* trackRow = trackQuery_.FindResultRow(track); trackRow) {
                auto trackHandle = animationTrackManager_.Read(trackRow->components[1]);
                RestoreFromInitialDataComponent(ecs_, *trackHandle, initialTransformManager_, trackRow->components[0]);
            }
        }
    }

    // Update animations, this will apply animations on top of current transform.
    const auto deltaS = delta / 1000000.f;
    for (const auto& row : animationQuery_.GetResults()) {
        bool setPaused = false;
        {
            auto animationHandle = animationManager_.Read(row.components[0]);
            auto stateHandle = stateManager_.Write(row.components[1]);
            if (animationHandle && stateHandle) {
                Update(*stateHandle, *animationHandle, trackQuery_, deltaS);
                setPaused =
                    (stateHandle->completed && animationHandle->state == AnimationComponent::PlaybackState::PLAY);
            }
        }
        if (setPaused) {
            animationManager_.Write(row.components[0])->state = AnimationComponent::PlaybackState::PAUSE;
        }
    }
    stateGeneration_ = stateManager_.GetGenerationCounter();
    return !animationQuery_.GetResults().empty();
}

void AnimationSystem::Uninitialize()
{
    ecs_.RemoveListener(animationManager_, *this);
    ecs_.RemoveListener(animationTrackManager_, *this);

    trackQuery_.SetEcsListenersEnabled(false);
    animationQuery_.SetEcsListenersEnabled(false);

    animations_.clear();
}

IAnimationPlayback* AnimationSystem::CreatePlayback(Entity const& animationEntity, ISceneNode const& node)
{
    vector<Entity> targetEntities;
    if (const auto animationData = animationManager_.Read(animationEntity); animationData) {
        targetEntities.reserve(animationData->tracks.size());
        for (const auto& trackEntity : animationData->tracks) {
            if (auto nameHandle = nameManager_.Read(trackEntity); nameHandle) {
                if (auto targetNode = node.LookupNodeByPath(nameHandle->name); targetNode) {
                    targetEntities.push_back(targetNode->GetEntity());
                } else {
                    CORE_LOG_D("Cannot find target node for animation track '%s'.", nameHandle->name.data());
                    return nullptr;
                }
            } else {
                CORE_LOG_D("Cannot match unnamed animation track.");
                return nullptr;
            }
        }
    }
    return CreatePlayback(animationEntity, targetEntities);
}

IAnimationPlayback* AnimationSystem::CreatePlayback(
    const Entity animationEntity, const array_view<const Entity> targetEntities)
{
    // animationEntity must be have an animation component and track count must match target entites, and
    // target entities must be valid.
    float maxLength = 0.f;
    if (const auto animationHandle = animationManager_.Read(animationEntity);
        !animationHandle || (animationHandle->tracks.size() != targetEntities.size()) ||
        !std::all_of(targetEntities.begin(), targetEntities.end(),
            [](const Entity& entity) { return EntityUtil::IsValid(entity); })) {
        return nullptr;
    } else {
        if (!stateManager_.HasComponent(animationEntity)) {
            stateManager_.Create(animationEntity);
        }
        maxLength = UpdateStateAndTracks(
            stateManager_, animationTrackManager_, inputManager_, animationEntity, *animationHandle, targetEntities);
    }
    // Create animation runtime.
    auto playback = make_unique<AnimationPlayback>(animationEntity, targetEntities, ecs_);
    playback->SetDuration(maxLength);

    IAnimationPlayback* result = playback.get();

    animations_.emplace_back(move(playback));

    return result;
}

void AnimationSystem::DestroyPlayback(IAnimationPlayback* playback)
{
    for (auto it = animations_.cbegin(); it != animations_.cend(); ++it) {
        if (it->get() == playback) {
            animations_.erase(it);
            break;
        }
    }
}

size_t AnimationSystem::GetPlaybackCount() const
{
    return animations_.size();
}

IAnimationPlayback* AnimationSystem::GetPlayback(size_t index) const
{
    return animations_[index].get();
}

void AnimationSystem::NotifyAnimationStarted(Entity entity)
{
    const uint32_t componentId = initialTransformManager_.GetComponentId(entity);
    if (componentId != IComponentManager::INVALID_COMPONENT_ID) {
        // Increment ref count.
        initialTransformManager_.Write(componentId)->references += 1;
    } else {
        // Store initial state for later use.
        if (auto trackHandle = animationTrackManager_.Read(entity); trackHandle) {
            InitializeInitialDataComponent(entity, ecs_, *trackHandle, initialTransformManager_);
        }
    }
}

void AnimationSystem::NotifyAnimationFinished(Entity entity)
{
    CORE_ASSERT(EntityUtil::IsValid(entity));

    const uint32_t componentId = initialTransformManager_.GetComponentId(entity);
    if (componentId != IComponentManager::INVALID_COMPONENT_ID) {
        // Decrement ref count.
        auto shouldDestroy = [](IInitialTransformComponentManager& initialTransformManager,
                                 const uint32_t componentId) {
            auto srcHandle = initialTransformManager.Write(componentId);
            srcHandle->references -= 1;
            return (srcHandle->references == 0);
        };
        if (shouldDestroy(initialTransformManager_, componentId)) {
            // Restore original transform.
            if (auto trackHandle = animationTrackManager_.Read(entity); trackHandle) {
                RestoreFromInitialDataComponent(ecs_, *trackHandle, initialTransformManager_, componentId);
            }

            // Remove component.
            initialTransformManager_.Destroy(entity);
        }
    }
}

void AnimationSystem::NotifyAnimationStateChanged() {}

array_view<const uint8_t> AnimationSystem::GetInitialValue(IComponentManager::ComponentId componentId)
{
    const auto srcHandle = initialTransformManager_.Read(componentId);
    return srcHandle->initialData;
}

AnimationSystem::Data AnimationSystem::GetAnimatedProperty(IComponentManager::ComponentId componentId)
{
    Data data {};
    if (const auto trackHandle = animationTrackManager_.Read(componentId); trackHandle) {
        if (auto targetManager = ecs_.GetComponentManager(trackHandle->component); targetManager) {
            if (IPropertyHandle* targetHandle = targetManager->GetData(trackHandle->target); targetHandle) {
                PropertyData propertyData;
                if (auto po = propertyData.RLock(*targetHandle, trackHandle->property); po) {
                    auto src = reinterpret_cast<uint8_t*>(po.offset);
                    data.property = &(po.property->type);
                    data.handle = ScopedHandle<uint8_t>(targetHandle);
                    data.data = array_view<uint8_t>(src, src + po.property->size);
                }
            }
        }
    }
    return data;
}

void AnimationSystem::OnComponentEvent(
    EventType type, const IComponentManager& componentManager, const array_view<const Entity> entities)
{
    switch (type) {
        case IEcs::ComponentListener::EventType::CREATED: {
            if (&componentManager == &animationManager_) {
                OnAnimationComponentsCreated(entities);
            }
            break;
        }
        case IEcs::ComponentListener::EventType::DESTROYED: {
            if (&componentManager == &animationManager_) {
                for (const auto entity : entities) {
                    stateManager_.Destroy(entity);
                }
            }
            break;
        }
        case IEcs::ComponentListener::EventType::MODIFIED: {
            if (&componentManager == &animationManager_) {
                OnAnimationComponentsUpdated(entities);
            } else if (&componentManager == &animationTrackManager_) {
                OnAnimationTrackComponentsUpdated(entities);
            }
            break;
        }
    }
}

void AnimationSystem::OnAnimationComponentsCreated(BASE_NS::array_view<const CORE_NS::Entity> entities)
{
    for (const auto entity : entities) {
        stateManager_.Create(entity);
        auto stateHandle = stateManager_.Write(entity);
        AnimationStateComponent& state = *stateHandle;
        if (auto animationHandle = animationManager_.Write(entity); animationHandle) {
            state.trackStates.reserve(animationHandle->tracks.size());
            float maxLength = 0.f;
            for (const auto& trackEntity : animationHandle->tracks) {
                state.trackStates.push_back({ Entity(trackEntity), 0U });
                AnimationStateComponent::TrackState& trackState = state.trackStates.back();

                if (auto track = animationTrackManager_.Read(trackState.entity); track) {
                    if (auto inputData = inputManager_.Read(track->timestamps); inputData) {
                        auto& input = *inputData;
                        trackState.length = input.timestamps.size();
                        if (trackState.length > 0) {
                            const float trackLength = input.timestamps[input.timestamps.size() - 1];
                            if (trackLength > maxLength) {
                                maxLength = trackLength;
                            }
                        }
                    }
                }
                NotifyAnimationStarted(trackState.entity);
            }
            // By default set the playback duration to animation data length.
            animationHandle->duration = maxLength;
            state.length = maxLength;
        }
    }
}

void AnimationSystem::OnAnimationComponentsUpdated(BASE_NS::array_view<const CORE_NS::Entity> entities)
{
    for (const auto entity : entities) {
        auto stateHandle = stateManager_.Write(entity);
        auto animationHandle = animationManager_.Read(entity);
        const auto oldState = stateHandle->state;
        const auto newState = animationHandle->state;
        if (newState != AnimationComponent::PlaybackState::STOP) {
            stateHandle->dirty = true;
        }
        if (oldState != newState) {
            if (oldState == AnimationComponent::PlaybackState::STOP &&
                newState == AnimationComponent::PlaybackState::PLAY) {
                ResetTime(*stateHandle, *animationHandle);
                // if playing backwards and time has been reset to zero jump to the end of the animation.
                if ((animationHandle->speed < 0.f) && stateHandle->time == 0.f) {
                    stateHandle->time = animationHandle->duration;
                }
            } else if (newState == AnimationComponent::PlaybackState::STOP) {
                stateHandle->time = 0.0f;
                stateHandle->currentLoop = 0;
                stateHandle->completed = false;
            } else if (newState == AnimationComponent::PlaybackState::PLAY) {
                stateHandle->completed = false;
            }
            stateHandle->state = animationHandle->state;
        }

        float maxLength = 0.f;
        auto& trackStates = stateHandle->trackStates;
        if (trackStates.size() != animationHandle->tracks.size()) {
            trackStates.resize(animationHandle->tracks.size());
        }
        auto stateIt = trackStates.begin();
        for (const auto& trackEntity : animationHandle->tracks) {
            stateIt->entity = trackEntity;
            if (auto track = animationTrackManager_.Read(trackEntity); track) {
                if (auto inputData = inputManager_.Read(track->timestamps); inputData) {
                    auto& input = *inputData;
                    stateIt->length = input.timestamps.size();
                    if (stateIt->length > 0) {
                        const float trackLength = input.timestamps[input.timestamps.size() - 1];
                        if (trackLength > maxLength) {
                            maxLength = trackLength;
                        }
                    }
                }
            }
            ++stateIt;
        }
        stateHandle->length = maxLength;
    }
}

void AnimationSystem::OnAnimationTrackComponentsUpdated(BASE_NS::array_view<const CORE_NS::Entity> entities)
{
    for (const auto entity : entities) {
        if (auto trackHandle = animationTrackManager_.Read(entity); trackHandle) {
            InitializeInitialDataComponent(entity, ecs_, *trackHandle, initialTransformManager_);
        }
    }
}

void AnimationSystem::AnimateTrack(const ComponentQuery& query, AnimationStateComponent::TrackState& track,
    const AnimationTrackComponent& animationTrack, const float offset, const float weight,
    const size_t currentFrameIndex, const size_t nextFrameIndex)
{
    // spline interpolation takes three values: in-tangent, data point, and out-tangent
    const size_t inputCount =
        (animationTrack.interpolationMode == AnimationTrackComponent::Interpolation::SPLINE) ? 3U : 1U;

    const InterpolationData interpolationData { animationTrack.interpolationMode, currentFrameIndex * inputCount,
        nextFrameIndex * inputCount, offset, weight };
    const ComponentQuery::ResultRow* row = query.FindResultRow(track.entity);
    CORE_ASSERT(row);
    // initial data is needed only when weight < 1 or there are multiple animations targeting the same property.
    // identifying such tracks would save updating the initial data in AnimationSystem::Update and interpolating
    // between initial and result.
    const array_view<const uint8_t> initialValue = GetInitialValue(row->components[0]);
    Data animatedValue = GetAnimatedProperty(row->components[1]);
    if (!animatedValue.property) {
        // Animated property not found.
        return;
    }

    if (auto outputData = outputManager_.Read(animationTrack.data); outputData) {
        auto& output = *outputData;

        if (output.type != *animatedValue.property) {
            const auto id = to_string(Hash(track.entity.id, output.type));
            CORE_LOG_ONCE_D(id, "AnimateTrack failed, unexpected type %" PRIx64, output.type);
            return;
        }
        switch (*animatedValue.property) {
            /* Primitive types */
            case PropertyType::FLOAT_T: {
                Interpolate<float>(interpolationData, output.data, initialValue, animatedValue.data);
                break;
            }

            /* Extended types */
            case PropertyType::VEC2_T: {
                Interpolate<Math::Vec2>(interpolationData, output.data, initialValue, animatedValue.data);
                break;
            }

            case PropertyType::VEC3_T: {
                Interpolate<Math::Vec3>(interpolationData, output.data, initialValue, animatedValue.data);
                break;
            }

            case PropertyType::VEC4_T: {
                Interpolate<Math::Vec4>(interpolationData, output.data, initialValue, animatedValue.data);
                break;
            }

            case PropertyType::QUAT_T: {
                Interpolate<Math::Quat>(interpolationData, output.data, initialValue, animatedValue.data);
                break;
            }

            case PropertyType::FLOAT_VECTOR_T: {
                AnimateMorph(interpolationData, output.data, animatedValue.data);
                break;
            }
            default: {
                const auto id = to_string(Hash(track.entity.id, animatedValue.property->typeHash));
                CORE_LOG_ONCE_D(id, "AnimateTrack failed, unsupported type %s", animatedValue.property->name.data());
                break;
            }
        }
    }
}

void AnimationSystem::Update(
    AnimationStateComponent& state, const AnimationComponent& animation, const ComponentQuery& query, float delta)
{
    switch (animation.state) {
        case AnimationComponent::PlaybackState::STOP: {
            // Stopped animation doesn't need any actions.
            state.dirty = false;
            return;
        }
        case AnimationComponent::PlaybackState::PLAY: {
            // Update time (in seconds).
            state.time += animation.speed * delta;
            state.time = Math::clamp(state.time, -FLT_EPSILON, animation.duration + FLT_EPSILON);
            break;
        }
        case AnimationComponent::PlaybackState::PAUSE: {
            // Paused animation doesn't need updates after this one.
            state.dirty = false;
            break;
        }
        default:
            break;
    }

    bool allTracksCompleted = (state.time > animation.duration) || (state.time < 0.0f);
    if (!allTracksCompleted) {
        allTracksCompleted = UpdateTracks(state, animation, query);
    }

    // All tracks completed?
    if (allTracksCompleted) {
        const bool repeat = (animation.repeatCount == AnimationComponent::REPEAT_COUNT_INFINITE) || // Infinite repeat.
                            animation.repeatCount > state.currentLoop; // Need to repeat until repeat count reached.
        if (repeat) {
            // Need to repeat, go to start.
            state.currentLoop++;

            ResetTime(state, animation);
        } else {
            // Animation completed.
            state.completed = true;
        }

        // Apply animations.
        UpdateTracks(state, animation, query);
    }
}

bool AnimationSystem::UpdateTracks(
    AnimationStateComponent& state, const AnimationComponent& animation, const ComponentQuery& query)
{
    size_t completedTrackCount = 0;

    // Process all tracks.
    const auto trackCount = animation.tracks.size();
    for (size_t i = 0; i < trackCount; ++i) {
        // Ensure we have a target entity.
        AnimationStateComponent::TrackState& trackState = state.trackStates[i];
        if (!EntityUtil::IsValid(trackState.entity) || !query.FindResultRow(trackState.entity)) {
            continue;
        }

        if (auto track = animationTrackManager_.Read(animation.tracks[i]); track) {
            if (const auto inputData = inputManager_.Read(track->timestamps); inputData) {
                const array_view<const float> timestamps = inputData->timestamps;
                // Ensure we have data.
                if (!timestamps.empty()) {
                    const bool forward = (animation.speed >= 0.f);
                    const float currentTime = state.time + animation.startOffset;
                    size_t currentFrameIndex;
                    size_t nextFrameIndex;
                    FindFrameIndices(forward, currentTime, timestamps, currentFrameIndex, nextFrameIndex);

                    float currentOffset = 0;
                    if (currentFrameIndex != nextFrameIndex) {
                        const float startFrameTime = timestamps[currentFrameIndex];
                        const float endFrameTime = timestamps[nextFrameIndex];

                        currentOffset = (currentTime - startFrameTime) / (endFrameTime - startFrameTime);
                        currentOffset = std::clamp(currentOffset, 0.0f, 1.0f);
                    } else {
                        completedTrackCount++;
                    }

                    if (animation.weight > 0.0f) {
                        AnimateTrack(query, trackState, *track, currentOffset, animation.weight, currentFrameIndex,
                            nextFrameIndex);
                    }
                }
            }
        }
    }

    return completedTrackCount == trackCount;
}

void AnimationSystem::ResetTime(AnimationStateComponent& state, const AnimationComponent& animation)
{
    state.time = fmod(state.time, animation.duration);
    if (state.time < 0.f) {
        // When speed is negative time will end up negative. Wrap around to end of animation length.
        state.time = animation.duration + state.time;
    }

    // This is called when SetTimePosition was called, animation was stopped, or playback is repeted. In all cases
    // mark the animation dirty so animation system will run at least one update cycle to get the animated object to
    // correct state.
    state.dirty = true;
}

ISystem* IAnimationSystemInstance(IEcs& ecs)
{
    return new AnimationSystem(ecs);
}

void IAnimationSystemDestroy(ISystem* instance)
{
    delete static_cast<AnimationSystem*>(instance);
}
CORE3D_END_NAMESPACE()
