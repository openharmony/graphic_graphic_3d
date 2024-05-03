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

#include "animation_system.h"

#include <PropertyTools/property_api_impl.inl>
#include <PropertyTools/property_data.h>
#include <PropertyTools/property_macros.h>
#include <algorithm>
#include <cfloat>
#include <cinttypes>
#include <numeric>

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_input_component.h>
#include <3d/ecs/components/animation_output_component.h>
#include <3d/ecs/components/animation_state_component.h>
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
#include <core/perf/cpu_perf_scope.h>
#include <core/perf/intf_performance_data_manager.h>
#include <core/plugin/intf_plugin_register.h>

#include "ecs/components/initial_transform_component.h"
#include "ecs/systems/animation_playback.h"

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;

struct AnimationSystem::PropertyEntry {
    uint64_t componentAndProperty;
    IComponentManager* component;
    uintptr_t propertyOffset;
    const Property* property;
};

struct AnimationSystem::InterpolationData {
    AnimationTrackComponent::Interpolation mode;
    size_t startIndex;
    size_t endIndex;

    float t;
    float weight;
};

class AnimationSystem::InitTask final : public IThreadPool::ITask {
public:
    InitTask(AnimationSystem& system, size_t offset, size_t count) : system_(system), offset_(offset), count_(count) {};

    void operator()() override
    {
        const auto results = array_view(static_cast<const uint32_t*>(system_.trackOrder_.data()) + offset_, count_);
        system_.InitializeTrackValues(results);
    }

protected:
    void Destroy() override {}

private:
    AnimationSystem& system_;
    size_t offset_;
    size_t count_;
};

class AnimationSystem::AnimateTask final : public IThreadPool::ITask {
public:
    AnimateTask(AnimationSystem& system, size_t offset, size_t count)
        : system_(system), offset_(offset), count_(count) {};

    void operator()() override
    {
        const auto results = array_view(static_cast<const uint32_t*>(system_.trackOrder_.data()) + offset_, count_);
        system_.Calculate(results);
        system_.AnimateTracks(results);
    }

protected:
    void Destroy() override {}

private:
    AnimationSystem& system_;
    size_t offset_;
    size_t count_;
};

namespace {
BEGIN_PROPERTY(AnimationSystem::Properties, SystemMetadata)
    DECL_PROPERTY2(AnimationSystem::Properties, minTaskSize, "Task size", PropertyFlags::IS_SLIDER)
END_PROPERTY();

constexpr PropertyTypeDecl PROPERTY_HANDLE_PTR_T = PROPERTYTYPE(CORE_NS::IPropertyHandle*);

// values contain in-tangent, spline vertex, out-tangent.
template<typename T>
struct SplineValues {
    T inTangent;
    T splineVertex;
    T outTangent;
};

template<typename To, typename From>
inline To Cast(From* from)
{
    if constexpr (is_const_v<remove_pointer_t<From>>) {
        return static_cast<const To>(static_cast<const void*>(from));
    } else {
        return static_cast<To>(static_cast<void*>(from));
    }
}

template<typename To, typename From>
inline To Cast(From& from)
{
    if constexpr (is_const_v<remove_reference_t<From>>) {
        return *static_cast<const remove_reference_t<To>*>(static_cast<const void*>(&from));
    } else {
        return *static_cast<remove_reference_t<To>*>(static_cast<void*>(&from));
    }
}

template<typename To, typename From>
static inline To Get(From&& from)
{
    if constexpr (is_same_v<remove_const_t<remove_reference_t<To>>, float>) {
        return from.initialData.floatValue;
    }
    if constexpr (is_same_v<remove_const_t<remove_reference_t<To>>, Math::Vec2>) {
        return from.initialData.vec2Value;
    }
    if constexpr (is_same_v<remove_const_t<remove_reference_t<To>>, Math::Vec3>) {
        return from.initialData.vec3Value;
    }
    if constexpr (is_same_v<remove_const_t<remove_reference_t<To>>, Math::Vec4>) {
        return from.initialData.vec4Value;
    }
}

template<class T>
T Step(const T& start, const T& end, float offset)
{
    return (offset > 0.5f) ? end : start;
}

Math::Quat InterpolateQuatSplineGLTF(const Math::Vec4& start, const Math::Vec4& outTangent, const Math::Vec4& end,
    const Math::Vec4& inTangent, float offset)
{
    const Math::Vec4 vec = Math::Hermite(start, outTangent, end, inTangent, offset);
    return Normalize(Cast<Math::Quat>(vec));
}

constexpr float Lerp(const float& v1, const float& v2, float t)
{
    t = Math::clamp01(t);
    return v1 + (v2 - v1) * t;
}

void Assign(const PropertyTypeDecl& type, uint8_t* dst, const InitialTransformComponent& src)
{
    switch (type) {
        // Primitive types
        case PropertyType::FLOAT_T: {
            *Cast<float*>(dst) = src.initialData.floatValue;
            break;
        }

        // Extended types
        case PropertyType::VEC2_T: {
            *Cast<Math::Vec2*>(dst) = src.initialData.vec2Value;
            break;
        }

        case PropertyType::VEC3_T: {
            *Cast<Math::Vec3*>(dst) = src.initialData.vec3Value;
            break;
        }

        case PropertyType::VEC4_T: {
            *Cast<Math::Vec4*>(dst) = src.initialData.vec4Value;
            break;
        }

        case PropertyType::QUAT_T: {
            *Cast<Math::Quat*>(dst) = src.initialData.quatValue;
            break;
        }

        case PropertyType::FLOAT_VECTOR_T: {
            // target property is an actual vector
            auto* dstVector = Cast<vector<float>*>(dst);
            // while initial values is the contents of the vector i.e. an array.
            const auto& srcVec = src.initialData.floatVectorValue;
            const auto* srcF = srcVec.data();
            const auto count = Math::min(srcVec.size(), dstVector->size());
            std::copy(srcF, srcF + count, dstVector->data());
            break;
        }
        default: {
            CORE_LOG_ONCE_D(to_string(type.typeHash), "AnimateTrack failed, unsupported type %s", type.name.data());
            break;
        }
    }
}

template<typename T>
inline void Mult(uint8_t* dst, const T& src)
{
    auto* dstType = Cast<T*>(dst);
    *dstType = src * *dstType;
}

template<typename T>
inline void Add(uint8_t* dst, const T src)
{
    auto* dstType = Cast<T*>(dst);
    *dstType = *dstType + src;
}

void Add(const PropertyTypeDecl& type, uint8_t* dst, const InitialTransformComponent& src)
{
    switch (type) {
        // Primitive types
        case PropertyType::FLOAT_T: {
            Add<float>(dst, src.initialData.floatValue);
            break;
        }

        // Extended types
        case PropertyType::VEC2_T: {
            Add<Math::Vec2>(dst, src.initialData.vec2Value);
            break;
        }

        case PropertyType::VEC3_T: {
            Add<Math::Vec3>(dst, src.initialData.vec3Value);
            break;
        }

        case PropertyType::VEC4_T: {
            Add<Math::Vec4>(dst, src.initialData.vec4Value);
            break;
        }

        case PropertyType::QUAT_T: {
            Mult<Math::Quat>(dst, src.initialData.quatValue);
            break;
        }

        case PropertyType::FLOAT_VECTOR_T: {
            // target property is an actual vector
            auto* dstVector = Cast<vector<float>*>(dst);
            // while result is the contents of the vector i.e. an array.
            auto& srcVector = src.initialData.floatVectorValue;
            const auto* srcF = srcVector.data();
            const auto count = Math::min(srcVector.size(), dstVector->size());
            std::transform(srcF, srcF + count, dstVector->data(), dstVector->data(),
                [](float initial, float result) { return initial + result; });
            break;
        }
        default: {
            CORE_LOG_ONCE_D(to_string(type.typeHash), "AnimateTrack failed, unsupported type %s", type.name.data());
            break;
        }
    }
}

struct AnimationKeyDataOffsets {
    size_t startKeyOffset;
    size_t endKeyOffset;
};

template<typename T>
void Interpolate(const AnimationSystem::InterpolationData& interpolation, array_view<const uint8_t> frameValues,
    const InitialTransformComponent& initialValue, InitialTransformComponent& animatedValue)
{
    auto values = array_view(Cast<const T*>(frameValues.data()), frameValues.size() / sizeof(T));
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
                const auto& p0 = Cast<const SplineValues<T>&>(values[interpolation.startIndex]);
                const auto& p1 = Cast<const SplineValues<T>&>(values[interpolation.endIndex]);
                result = Math::Hermite(p0.splineVertex, p0.outTangent, p1.splineVertex, p1.inTangent, interpolation.t);
                break;
            }
        }
        const T& initial = Get<const T&>(initialValue);
        // Could assign to animatedValue directly but that happens via a jump to operator=. Types have been checked in
        // AnimateTracks.
        T& target = Get<T&>(animatedValue);
        // Convert result to relative and apply weight.
        target = (result - initial) * interpolation.weight;
    } else {
        Get<T&>(animatedValue) = {};
    }
}

template<>
void Interpolate<Math::Quat>(const AnimationSystem::InterpolationData& interpolation,
    array_view<const uint8_t> frameValues, const InitialTransformComponent& initialValue,
    InitialTransformComponent& animatedValue)
{
    static constexpr Math::Quat identity(0.0f, 0.0f, 0.0f, 1.0f);
    auto values = array_view(Cast<const Math::Vec4*>(frameValues.data()), frameValues.size() / sizeof(Math::Vec4));
    if ((interpolation.startIndex < values.size()) && (interpolation.endIndex < values.size())) {
        Math::Quat result;

        switch (interpolation.mode) {
            case AnimationTrackComponent::Interpolation::STEP: {
                result = Step(Cast<const Math::Quat&>(values[interpolation.startIndex]),
                    Cast<const Math::Quat&>(values[interpolation.endIndex]), interpolation.t);
                break;
            }

            default:
            case AnimationTrackComponent::Interpolation::LINEAR: {
                result = Slerp(Cast<const Math::Quat&>(values[interpolation.startIndex]),
                    Cast<const Math::Quat&>(values[interpolation.endIndex]), interpolation.t);
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
        const Math::Quat inverseInitialRotation = Inverse(initialValue.initialData.quatValue);
        const Math::Quat delta = Normalize(Slerp(identity, result * inverseInitialRotation, interpolation.weight));

        animatedValue.initialData.quatValue = delta;
    } else {
        animatedValue.initialData.quatValue = identity;
    }
}

void AnimateArray(const AnimationSystem::InterpolationData& interpolation, array_view<const uint8_t> frameValues,
    const InitialTransformComponent& initialValue, InitialTransformComponent& animatedValue)
{
    const auto initialValues =
        array_view(initialValue.initialData.floatVectorValue.data(), initialValue.initialData.floatVectorValue.size());

    auto animated = array_view(
        animatedValue.initialData.floatVectorValue.data(), animatedValue.initialData.floatVectorValue.size());
    const auto targetCount = animated.size();

    // Re-calculate the start/end offsets as each value is actually an array of values.
    const size_t startDataOffset = interpolation.startIndex * targetCount;
    const size_t endDataOffset = interpolation.endIndex * targetCount;

    if (interpolation.mode == AnimationTrackComponent::Interpolation::SPLINE) {
        const auto values = array_view(
            Cast<const SplineValues<float>*>(frameValues.data()), frameValues.size() / sizeof(SplineValues<float>));
        if (((startDataOffset + targetCount) <= values.size()) && ((endDataOffset + targetCount) <= values.size())) {
            auto p0 = values.data() + startDataOffset;
            auto p1 = values.data() + endDataOffset;
            const auto p0E = p0 + targetCount;
            auto iI = initialValues.cbegin();
            auto aI = animated.begin();
            for (; p0 != p0E; ++p0, ++p1, ++iI, ++aI) {
                *aI =
                    (Math::Hermite(p0->splineVertex, p0->outTangent, p1->splineVertex, p1->inTangent, interpolation.t) -
                        *iI) *
                    interpolation.weight;
            }
        }
    } else {
        const auto values = array_view(Cast<const float*>(frameValues.data()), frameValues.size() / sizeof(float));
        if (((startDataOffset + targetCount) <= values.size()) && ((endDataOffset + targetCount) <= values.size())) {
            auto p0 = values.data() + startDataOffset;
            auto p1 = values.data() + endDataOffset;
            const auto p0E = p0 + targetCount;
            auto iI = initialValues.cbegin();
            auto aI = animated.begin();
            if (interpolation.mode == AnimationTrackComponent::Interpolation::STEP) {
                for (; p0 != p0E; ++p0, ++p1, ++iI, ++aI) {
                    *aI = (Step(*p0, *p1, interpolation.t) - *iI) * interpolation.weight;
                }
            } else {
                for (; p0 != p0E; ++p0, ++p1, ++iI, ++aI) {
                    *aI = (Math::lerp(*p0, *p1, interpolation.t) - *iI) * interpolation.weight;
                }
            }
        }
    }
}

void AnimateTrack(const PropertyTypeDecl& type, const AnimationSystem::InterpolationData& interpolationData,
    const AnimationOutputComponent& outputComponent, const InitialTransformComponent& initialValue,
    InitialTransformComponent& resultValue)
{
    switch (type) {
        // Primitive types
        case PropertyType::FLOAT_T: {
            Interpolate<float>(interpolationData, outputComponent.data, initialValue, resultValue);
            break;
        }

        // Extended types
        case PropertyType::VEC2_T: {
            Interpolate<Math::Vec2>(interpolationData, outputComponent.data, initialValue, resultValue);
            break;
        }

        case PropertyType::VEC3_T: {
            Interpolate<Math::Vec3>(interpolationData, outputComponent.data, initialValue, resultValue);
            break;
        }

        case PropertyType::VEC4_T: {
            Interpolate<Math::Vec4>(interpolationData, outputComponent.data, initialValue, resultValue);
            break;
        }

        case PropertyType::QUAT_T: {
            Interpolate<Math::Quat>(interpolationData, outputComponent.data, initialValue, resultValue);
            break;
        }

        case PropertyType::FLOAT_VECTOR_T: {
            AnimateArray(interpolationData, outputComponent.data, initialValue, resultValue);
            break;
        }
        default: {
            CORE_LOG_ONCE_D(to_string(type.typeHash), "AnimateTrack failed, unsupported type %s", type.name.data());
            return;
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
        next = static_cast<size_t>(std::distance(begin, pos));
        current = next - 1;
    } else {
        // Find next frame (backward).
        current = static_cast<size_t>(std::distance(begin, pos));
        next = current ? current - 1 : 0;
    }

    // Clamp timestamps to valid range.
    currentFrameIndex = std::clamp(current, size_t(0), timestamps.size() - 1);
    nextFrameIndex = std::clamp(next, size_t(0), timestamps.size() - 1);
}

void UpdateStateAndTracks(IAnimationStateComponentManager& stateManager_,
    IAnimationTrackComponentManager& trackManager_, IAnimationInputComponentManager& inputManager_,
    Entity animationEntity, const AnimationComponent& animationComponent, array_view<const Entity> targetEntities)
{
    auto stateHandle = stateManager_.Write(animationEntity);
    stateHandle->trackStates.reserve(animationComponent.tracks.size());

    auto& entityManager = stateManager_.GetEcs().GetEntityManager();
    auto targetIt = targetEntities.begin();
    for (const auto& trackEntity : animationComponent.tracks) {
        stateHandle->trackStates.push_back({ Entity(trackEntity), 0U });
        auto& trackState = stateHandle->trackStates.back();

        if (auto track = trackManager_.Write(trackEntity); track) {
            track->target = entityManager.GetReferenceCounted(*targetIt);

            if (auto inputData = inputManager_.Read(track->timestamps); inputData) {
                trackState.length = inputData->timestamps.size();
            }
        }
        ++targetIt;
    }
}

void CopyInitialDataComponent(
    InitialTransformComponent& dst, const Property* property, const array_view<const uint8_t> src)
{
    switch (property->type) {
        case PropertyType::FLOAT_T: {
            dst = *Cast<const float*>(src.data());
            break;
        }

        case PropertyType::VEC2_T: {
            dst = *Cast<const Math::Vec2*>(src.data());
            break;
        }

        case PropertyType::VEC3_T: {
            dst = *Cast<const Math::Vec3*>(src.data());
            break;
        }

        case PropertyType::VEC4_T: {
            dst = *Cast<const Math::Vec4*>(src.data());
            break;
        }

        case PropertyType::QUAT_T: {
            dst = *Cast<const Math::Quat*>(src.data());
            break;
        }

        case PropertyType::FLOAT_VECTOR_T: {
            dst = *Cast<const vector<float>*>(src.data());
            break;
        }
        default: {
            CORE_LOG_ONCE_D(
                to_string(property->type.typeHash), "AnimateTrack failed, unsupported type %s", property->name.data());
            return;
        }
    }
}

AnimationSystem::PropertyEntry FindDynamicProperty(const AnimationTrackComponent& animationTrack,
    const IPropertyHandle* dynamicProperties, AnimationSystem::PropertyEntry entry)
{
    if (dynamicProperties) {
        auto propertyName = string_view(animationTrack.property);
        if (propertyName.starts_with(entry.property->name)) {
            propertyName.remove_prefix(entry.property->name.size() + 1U);
        }

        auto c = PropertyData::FindProperty(dynamicProperties->Owner()->MetaData(), propertyName);
        if (c && c.propertyPath == propertyName) {
            entry.property = c.property;
            entry.propertyOffset = c.offset;
        }
    }
    return entry;
}

void ResetTime(AnimationStateComponent& state, const AnimationComponent& animation)
{
    state.time = (animation.duration != 0.f) ? std::fmod(state.time, animation.duration) : 0.f;
    if (state.time < 0.f) {
        // When speed is negative time will end up negative. Wrap around to end of animation length.
        state.time = animation.duration + state.time;
    }

    // This is called when SetTimePosition was called, animation was stopped, or playback is repeted. In all cases
    // mark the animation dirty so animation system will run at least one update cycle to get the animated object to
    // correct state.
    state.dirty = true;
}
} // namespace

AnimationSystem::AnimationSystem(IEcs& ecs)
    : ecs_(ecs), systemPropertyApi_(&systemProperties_, array_view(SystemMetadata)),
      initialTransformManager_(*(GetManager<IInitialTransformComponentManager>(ecs_))),
      animationManager_(*(GetManager<IAnimationComponentManager>(ecs_))),
      inputManager_(*(GetManager<IAnimationInputComponentManager>(ecs_))),
      outputManager_(*(GetManager<IAnimationOutputComponentManager>(ecs_))),
      stateManager_(*(GetManager<IAnimationStateComponentManager>(ecs_))),
      animationTrackManager_(*(GetManager<IAnimationTrackComponentManager>(ecs_))),
      nameManager_(*(GetManager<INameComponentManager>(ecs_))), threadPool_(ecs.GetThreadPool())
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
    return systemPropertyApi_.GetData();
}

const IPropertyHandle* AnimationSystem::GetProperties() const
{
    return systemPropertyApi_.GetData();
}

void AnimationSystem::SetProperties(const IPropertyHandle& data)
{
    if (data.Owner() != &systemPropertyApi_) {
        return;
    }
    if (const auto in = ScopedHandle<const AnimationSystem::Properties>(&data); in) {
        systemProperties_.minTaskSize = in->minTaskSize;
    }
}

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

    auto results = animationQuery_.GetResults();
    if (animationGeneration_ != animationManager_.GetGenerationCounter()) {
        // Handle stopped animations first to avoid glitches. Alternative would be filtering out tracks which are
        // stopped, but there's another track targeting the same property.
        animationOrder_.clear();
        animationOrder_.reserve(results.size());
        uint32_t index = 0U;
        uint32_t stopped = 0U;
        for (const auto& row : results) {
            auto animationHandle = animationManager_.Read(row.components[0]);
            if (animationHandle->state == AnimationComponent::PlaybackState::STOP) {
                animationOrder_.insert(animationOrder_.cbegin() + stopped, index);
                ++stopped;
            } else {
                animationOrder_.push_back(index);
            }
            ++index;
        }
    }

    // Reset trackValues_ to stopped state before walking through animations.
    trackValues_.resize(animationTrackManager_.GetComponentCount());
    for (auto& values : trackValues_) {
        values.stopped = true;
    }

    frameIndices_.resize(animationTrackManager_.GetComponentCount());

    const auto deltaS = static_cast<float>(delta) / 1000000.f;
    // For each animation update the playback state, time position and weight, and gather tracks in trackOrder_.
    trackOrder_.clear();
    trackOrder_.reserve(animationTrackManager_.GetComponentCount());
    for (const auto& index : animationOrder_) {
        const auto& row = results[index];
        bool setPaused = false;
        {
            auto animationHandle = animationManager_.Read(row.components[0]);
            auto stateHandle = stateManager_.Write(row.components[1]);
            if (animationHandle && stateHandle) {
                UpdateAnimation(*stateHandle, *animationHandle, trackQuery_, deltaS);
                setPaused =
                    (stateHandle->completed && animationHandle->state == AnimationComponent::PlaybackState::PLAY);
            }
        }
        if (setPaused) {
            animationManager_.Write(row.components[0])->state = AnimationComponent::PlaybackState::PAUSE;
        }
    }

    {
        // Would constructing a reordered ResultRow array be better than indices into the results? Would be one
        // indirection less in processing...
    }

    // Calculate how many tasks will be needed
    {
        const auto threadCount = threadPool_->GetNumberOfThreads() + 1;
        const auto resultCount = trackOrder_.size();
        taskSize_ = Math::max(systemProperties_.minTaskSize, resultCount / threadCount);
        if (taskSize_ == 0) {
            taskSize_ = 1;
        }
        tasks_ = ((resultCount / taskSize_)) ? (resultCount / taskSize_) - 1U : 0U;
        remaining_ = resultCount - (tasks_ * taskSize_);
    }

    taskId_ = 0U;
    taskResults_.clear();

    // Start tasks for filling the initial values to trackValues_.
    InitializeTrackValues();

    // For each track batch reset the target properties to initial values, wait for trackValues_ to be filled and start
    // new task to handle the actual animation.
    ResetToInitialTrackValues();

    // Update target properties, this will apply animations on top of current value.
    WriteUpdatedTrackValues();

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
    if (const auto animationHandle = animationManager_.Read(animationEntity);
        !animationHandle || (animationHandle->tracks.size() != targetEntities.size()) ||
        !std::all_of(targetEntities.begin(), targetEntities.end(),
            [](const Entity& entity) { return EntityUtil::IsValid(entity); })) {
        return nullptr;
    } else {
        if (!stateManager_.HasComponent(animationEntity)) {
            stateManager_.Create(animationEntity);
        }
        UpdateStateAndTracks(
            stateManager_, animationTrackManager_, inputManager_, animationEntity, *animationHandle, targetEntities);
    }
    // Create animation runtime.
    auto playback = make_unique<AnimationPlayback>(animationEntity, targetEntities, ecs_);

    IAnimationPlayback* result = playback.get();

    animations_.push_back(move(playback));

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
    if (index < animations_.size()) {
        return animations_[index].get();
    }
    return nullptr;
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
        if (auto animationHandle = animationManager_.Read(entity); animationHandle) {
            state.trackStates.reserve(animationHandle->tracks.size());

            for (const auto& trackEntity : animationHandle->tracks) {
                state.trackStates.push_back({ Entity(trackEntity), 0U });
                AnimationStateComponent::TrackState& trackState = state.trackStates.back();

                const uint32_t componentId = initialTransformManager_.GetComponentId(trackState.entity);
                if (componentId == IComponentManager::INVALID_COMPONENT_ID) {
                    // Store initial state for later use.
                    if (auto trackHandle = animationTrackManager_.Read(trackState.entity); trackHandle) {
                        InitializeInitialDataComponent(trackState.entity, *trackHandle);
                    }
                }
            }
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
                }
            }
            ++stateIt;
        }
    }
}

void AnimationSystem::OnAnimationTrackComponentsUpdated(BASE_NS::array_view<const CORE_NS::Entity> entities)
{
    for (const auto entity : entities) {
        if (auto trackHandle = animationTrackManager_.Read(entity); trackHandle) {
            InitializeInitialDataComponent(entity, *trackHandle);
        }
    }
}

void AnimationSystem::UpdateAnimation(AnimationStateComponent& state, const AnimationComponent& animation,
    const CORE_NS::ComponentQuery& trackQuery, float delta)
{
    const auto* const begin = trackQuery_.GetResults().data();
    switch (animation.state) {
        case AnimationComponent::PlaybackState::STOP: {
            // Mark the tracks of this animation as stopped so that they are skipped in later steps.
            for (const auto& trackState : state.trackStates) {
                if (!EntityUtil::IsValid(trackState.entity)) {
                    continue;
                }
                if (auto row = trackQuery.FindResultRow(trackState.entity); row) {
                    trackOrder_.push_back(static_cast<uint32_t>(std::distance(begin, row)));
                    trackValues_[row->components[1]].stopped = true;
                }
            }
            // Stopped animation doesn't need any actions.
            state.dirty = false;
            return;
        }
        case AnimationComponent::PlaybackState::PLAY: {
            if (!(state.options & AnimationStateComponent::FlagBits::MANUAL_PROGRESS)) {
                // Update time (in seconds).
                state.time += animation.speed * delta;
            }
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

    // All tracks completed?
    const auto timePosition = state.time + animation.startOffset;
    if ((timePosition >= (animation.duration + FLT_EPSILON)) || (timePosition <= -FLT_EPSILON)) {
        const bool repeat = (animation.repeatCount == AnimationComponent::REPEAT_COUNT_INFINITE) || // Infinite repeat.
                            animation.repeatCount > state.currentLoop; // Need to repeat until repeat count reached.
        if (repeat) {
            // Need to repeat, go to start.
            state.currentLoop++;

            ResetTime(state, animation);
        } else {
            // Animation completed.
            state.completed = true;
            state.time = Math::clamp(state.time, -FLT_EPSILON, animation.duration + FLT_EPSILON);
        }
    }

    // Set the animation's state to each track.
    const auto forward = (animation.speed >= 0.f);
    for (const auto& trackState : state.trackStates) {
        auto row = EntityUtil::IsValid(trackState.entity) ? trackQuery.FindResultRow(trackState.entity) : nullptr;
        if (row) {
            trackOrder_.push_back(static_cast<uint32_t>(std::distance(begin, row)));
            auto& track = trackValues_[row->components[1]];
            track.timePosition = timePosition;
            track.weight = animation.weight;
            track.stopped = false;
            track.forward = forward;
        }
    }
}

void AnimationSystem::InitializeTrackValues()
{
    initTasks_.clear();
    initTaskCount_ = 0U;
    if (tasks_) {
        // Reserve for twice as many tasks as both init and anim task results are in the same vector.
        taskResults_.reserve(taskResults_.size() + tasks_ * 2U);

        initTasks_.reserve(remaining_ ? (tasks_ + 1U) : tasks_);
        initTaskStart_ = taskId_;
        for (size_t i = 0; i < tasks_; ++i) {
            auto& task = initTasks_.emplace_back(*this, i * taskSize_, taskSize_);
            taskResults_.push_back(threadPool_->Push(IThreadPool::ITask::Ptr { &task }));
            ++taskId_;
        }
    }
    if (remaining_ >= systemProperties_.minTaskSize) {
        auto& task = initTasks_.emplace_back(*this, tasks_ * taskSize_, remaining_);
        taskResults_.push_back(threadPool_->Push(IThreadPool::ITask::Ptr { &task }));
        ++taskId_;
    } else {
        InitializeTrackValues(array_view(static_cast<const uint32_t*>(trackOrder_.data()), remaining_));
    }
    initTaskCount_ = taskId_ - initTaskStart_;
}

void AnimationSystem::ResetToInitialTrackValues()
{
    animTasks_.clear();
    animTasks_.reserve(remaining_ ? (tasks_ + 1U) : tasks_);
    animTaskStart_ = taskId_;
    animTaskCount_ = 0;
    auto batch = [this](size_t i, size_t offset, size_t count) {
        // Wait that initial values for this track batch have been filled
        taskResults_[i]->Wait();

        // Start task for calculating which keyframes are used and interpolate between them.
        auto& task = animTasks_.emplace_back(*this, offset, count);
        taskResults_.push_back(threadPool_->Push(IThreadPool::ITask::Ptr { &task }));
        ++taskId_;

        // While task is running reset the target property so we can later sum the results of multiple tracks.
        ResetTargetProperties(array_view(static_cast<const uint32_t*>(trackOrder_.data()) + offset, count));
    };
    for (size_t i = 0U; i < tasks_; ++i) {
        batch(i, i * taskSize_, taskSize_);
    }
    if (remaining_ >= systemProperties_.minTaskSize) {
        batch(tasks_, tasks_ * taskSize_, remaining_);
    } else {
        const auto results = array_view(static_cast<const uint32_t*>(trackOrder_.data()), remaining_);
        Calculate(results);
        AnimateTracks(results);
        ResetTargetProperties(results);
    }
    animTaskCount_ = taskId_ - animTaskStart_;
}

void AnimationSystem::WriteUpdatedTrackValues()
{
    auto batch = [this](size_t i, size_t offset, size_t count) {
        // Wait for animation task for this batch to finish.
        taskResults_[i + animTaskStart_]->Wait();

        // Apply the result of each track animation to the target property.
        ApplyResults(array_view(static_cast<const uint32_t*>(trackOrder_.data()) + offset, count));
    };
    for (size_t i = 0U; i < tasks_; ++i) {
        batch(i, i * taskSize_, taskSize_);
    }
    if (remaining_ >= systemProperties_.minTaskSize) {
        batch(tasks_, tasks_ * taskSize_, remaining_);
    } else {
        ApplyResults(array_view(static_cast<const uint32_t*>(trackOrder_.data()), remaining_));
    }
}

void AnimationSystem::ResetTargetProperties(array_view<const uint32_t> resultIndices)
{
    auto results = trackQuery_.GetResults();
    for (auto index : resultIndices) {
        const ComponentQuery::ResultRow& row = results[index];
        const auto trackId = row.components[1];

        const auto& values = trackValues_[trackId];

        auto trackHandle = animationTrackManager_.Read(trackId);
        auto entry = GetEntry(*trackHandle);
        if (entry.component && entry.property) {
            if (IPropertyHandle* targetHandle = entry.component->GetData(trackHandle->target); targetHandle) {
                // special handling for IPropertyHandle*. need to fetch the property types behind the pointer and
                // update targetHandle.
                if (entry.property->type == PROPERTY_HANDLE_PTR_T) {
                    auto componentBase = ScopedHandle<const uint8_t>(targetHandle);
                    auto* dynamicProperties = *Cast<IPropertyHandle* const*>(&*componentBase + entry.propertyOffset);
                    if (dynamicProperties) {
                        entry = FindDynamicProperty(*trackHandle, dynamicProperties, entry);
                        targetHandle = dynamicProperties;
                    } else {
                        continue;
                    }
                }
                if (entry.property->type == values.initial.type) {
                    if (auto baseAddress = ScopedHandle<uint8_t>(targetHandle); baseAddress) {
                        Assign(entry.property->type, &*baseAddress + entry.propertyOffset, values.initial);
                    }
                } else {
                    CORE_LOG_ONCE_D(to_string(Hash(trackId, entry.property->type.typeHash)),
                        "ResetTargetProperties failed, type mismatch %" PRIx64, entry.property->type.typeHash);
                }
            }
        }
    }
}

void AnimationSystem::InitializeTrackValues(array_view<const uint32_t> resultIndices)
{
    auto& initialTransformManager = initialTransformManager_;
    auto& trackValues = trackValues_;
    auto results = trackQuery_.GetResults();
    for (auto index : resultIndices) {
        const ComponentQuery::ResultRow& row = results[index];
        auto initialHandle = initialTransformManager.Read(row.components[0]);
        auto& values = trackValues[row.components[1]];
        values.initial = *initialHandle;
        values.result = *initialHandle;
        values.updated = false;
    }
}

void AnimationSystem::Calculate(array_view<const uint32_t> resultIndices)
{
    auto results = trackQuery_.GetResults();
    for (auto index : resultIndices) {
        const ComponentQuery::ResultRow& row = results[index];
        const auto trackId = row.components[1];
        if (trackValues_[trackId].stopped) {
            continue;
        }
        if (auto track = animationTrackManager_.Read(trackId); track) {
            if (const auto inputData = inputManager_.Read(track->timestamps); inputData) {
                const array_view<const float> timestamps = inputData->timestamps;
                // Ensure we have data.
                if (!timestamps.empty()) {
                    size_t currentFrameIndex;
                    size_t nextFrameIndex;

                    const auto currentTime = trackValues_[trackId].timePosition;
                    FindFrameIndices(
                        trackValues_[trackId].forward, currentTime, timestamps, currentFrameIndex, nextFrameIndex);

                    float currentOffset = 0.f;
                    if (currentFrameIndex != nextFrameIndex) {
                        const float startFrameTime = timestamps[currentFrameIndex];
                        const float endFrameTime = timestamps[nextFrameIndex];

                        currentOffset = (currentTime - startFrameTime) / (endFrameTime - startFrameTime);
                        currentOffset = std::clamp(currentOffset, 0.0f, 1.0f);
                    }

                    frameIndices_[trackId] = FrameData { currentOffset, currentFrameIndex, nextFrameIndex };
                }
            }
        }
    }
}

void AnimationSystem::AnimateTracks(array_view<const uint32_t> resultIndices)
{
    auto results = trackQuery_.GetResults();
    for (auto index : resultIndices) {
        const ComponentQuery::ResultRow& row = results[index];
        const auto trackId = row.components[1];
        auto& trackValues = trackValues_[trackId];
        if (trackValues.stopped) {
            continue;
        }
        if (auto trackHandle = animationTrackManager_.Read(trackId); trackHandle) {
            if (auto outputHandle = outputManager_.Read(trackHandle->data); outputHandle) {
                if (outputHandle->data.empty()) {
                    continue;
                }

                auto entry = GetEntry(*trackHandle);
                if (!entry.component || !entry.property) {
                    return;
                }

                // special handling for IPropertyHandle*. need to fetch the property types behind the pointer.
                if (entry.property->type == PROPERTY_HANDLE_PTR_T) {
                    if (const IPropertyHandle* targetHandle = entry.component->GetData(trackHandle->target);
                        targetHandle) {
                        auto componentBase = ScopedHandle<const uint8_t>(targetHandle);
                        auto* dynamicProperties =
                            *Cast<const IPropertyHandle* const*>(&*componentBase + entry.propertyOffset);
                        if (dynamicProperties) {
                            entry = FindDynamicProperty(*trackHandle, dynamicProperties, entry);
                        }
                    }
                }

                if ((outputHandle->type != entry.property->type) || (outputHandle->type != trackValues.initial.type) ||
                    (outputHandle->type != trackValues.result.type)) {
                    CORE_LOG_ONCE_D(to_string(Hash(trackId, outputHandle->type)),
                        "AnimateTrack failed, unexpected type %" PRIx64, outputHandle->type);
                    continue;
                }

                // spline interpolation takes three values: in-tangent, data point, and out-tangent
                const size_t inputCount =
                    (trackHandle->interpolationMode == AnimationTrackComponent::Interpolation::SPLINE) ? 3U : 1U;
                const FrameData& currentFrame = frameIndices_[trackId];
                const InterpolationData interpolationData { trackHandle->interpolationMode,
                    currentFrame.currentFrameIndex * inputCount, currentFrame.nextFrameIndex * inputCount,
                    currentFrame.currentOffset, trackValues.weight };

                // initial data is needed only when weight < 1 or there are multiple animations targeting the same
                // property.
                // identifying such tracks would save updating the initial data in AnimationSystem::Update and
                // interpolating between initial and result.
                AnimateTrack(
                    entry.property->type, interpolationData, *outputHandle, trackValues.initial, trackValues.result);
                trackValues.updated = true;
            }
        }
    }
}

void AnimationSystem::ApplyResults(array_view<const uint32_t> resultIndices)
{
    auto results = trackQuery_.GetResults();
    for (auto index : resultIndices) {
        const ComponentQuery::ResultRow& row = results[index];
        const auto trackId = row.components[1];
        auto& values = trackValues_[trackId];

        if (!values.stopped && values.updated) {
            values.updated = false;
            auto trackHandle = animationTrackManager_.Read(trackId);
            auto entry = GetEntry(*trackHandle);
            if (entry.component && entry.property) {
                if (IPropertyHandle* targetHandle = entry.component->GetData(trackHandle->target); targetHandle) {
                    // special handling for IPropertyHandle*. need to fetch the property types behind the pointer and
                    // update targetHandle.
                    if (entry.property->type == PROPERTY_HANDLE_PTR_T) {
                        auto componentBase = ScopedHandle<const uint8_t>(targetHandle);
                        auto* dynamicProperties =
                            *Cast<IPropertyHandle* const*>(&*componentBase + entry.propertyOffset);
                        if (dynamicProperties) {
                            entry = FindDynamicProperty(*trackHandle, dynamicProperties, entry);
                            targetHandle = dynamicProperties;
                        } else {
                            continue;
                        }
                    }
                    if (entry.property->type == values.result.type) {
                        if (auto baseAddress = ScopedHandle<uint8_t>(targetHandle); baseAddress) {
                            Add(entry.property->type, &*baseAddress + entry.propertyOffset, values.result);
                        }
                    } else {
                        CORE_LOG_ONCE_D(to_string(Hash(trackId, entry.property->type.typeHash)),
                            "ApplyResults failed, type mismatch %" PRIx64, entry.property->type.typeHash);
                    }
                }
            }
        }
    }
}

const AnimationSystem::PropertyEntry& AnimationSystem::GetEntry(const AnimationTrackComponent& track)
{
    // Cache component manager and property lookups
    const auto key = Hash(track.component, track.property);
    auto pos = std::lower_bound(std::begin(propertyCache_), std::end(propertyCache_), key,
        [](const PropertyEntry& element, uint64_t value) { return element.componentAndProperty < value; });
    // Add new manager-propery combinations
    if ((pos == std::end(propertyCache_)) || (pos->componentAndProperty != key)) {
        pos = propertyCache_.insert(pos, PropertyEntry { key, ecs_.GetComponentManager(track.component), 0U, nullptr });
    }
    // Try to get the component manager if it's missing
    if (!pos->component) {
        pos->component = ecs_.GetComponentManager(track.component);
    }

    // Try to get the property type if it's missing and there's a valid target
    if (pos->component && !pos->property && EntityUtil::IsValid(track.target)) {
        if (IPropertyHandle* targetHandle = pos->component->GetData(track.target); targetHandle) {
            const auto propertyOffset = PropertyData::FindProperty(targetHandle->Owner()->MetaData(), track.property);
            if (propertyOffset.property) {
                pos->propertyOffset = propertyOffset.offset;
                pos->property = propertyOffset.property;
            }
        }
    }
    return *pos;
}

void AnimationSystem::InitializeInitialDataComponent(Entity trackEntity, const AnimationTrackComponent& animationTrack)
{
    if (auto entry = GetEntry(animationTrack); entry.component && entry.property) {
        if (IPropertyHandle* targetHandle = entry.component->GetData(animationTrack.target); targetHandle) {
            initialTransformManager_.Create(trackEntity);
            auto dstHandle = initialTransformManager_.Write(trackEntity);

            auto componentBase = ScopedHandle<const uint8_t>(targetHandle);
            if (entry.property->type != PROPERTY_HANDLE_PTR_T) {
                auto src = array_view(&*componentBase + entry.propertyOffset, entry.property->size);
                CopyInitialDataComponent(*dstHandle, entry.property, src);
            } else {
                // special handling for property handles
                auto* dynamicProperties = *Cast<const IPropertyHandle* const*>(&*componentBase + entry.propertyOffset);
                if (dynamicProperties) {
                    entry = FindDynamicProperty(animationTrack, dynamicProperties, entry);
                    auto dynamicPropertiesBase = ScopedHandle<const uint8_t>(dynamicProperties);
                    auto src = array_view(&*dynamicPropertiesBase + entry.propertyOffset, entry.property->size);
                    CopyInitialDataComponent(*dstHandle, entry.property, src);
                }
            }
        }
    }
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
