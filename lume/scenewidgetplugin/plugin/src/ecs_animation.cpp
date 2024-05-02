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
#include "ecs_animation.h"

#include <algorithm>
#include <PropertyTools/property_api_impl.inl>
#include <PropertyTools/property_data.h>

#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <core/intf_engine.h>

#include <meta/api/make_callback.h>

#include "ecs_util.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;
using namespace META_NS;

SCENE_BEGIN_NAMESPACE()

namespace {

bool EndsWith(string_view str, string_view postfix)
{
    return str.size() >= postfix.size() && 0 == str.compare(str.size() - postfix.size(), postfix.size(), postfix);
}

template<typename type>
const BASE_NS::Uid GetArrayTypeUid()
{
    return UidFromType<type[]>();
}

PropertyData::PropertyOffset RLock(IPropertyHandle& targetHandle, string_view property)
{
    PropertyData pData;

    string path, name;
    auto containerHandle = ResolveContainerProperty(targetHandle, string(property), path, name);
    if (containerHandle) {
        if (auto po = pData.RLock(*containerHandle, name); po) {
            return po;
        }
    }

    if (auto po = pData.RLock(targetHandle, property); po) {
        return po;
    }

    return PropertyData::PropertyOffset();
}

BASE_NS::string ResolvePathToAnimationRoot(IEcs& ecs, Entity root, Entity target)
{
    BASE_NS::string result;

    CORE3D_NS::INodeSystem* nodeSystem = GetSystem<INodeSystem>(ecs);
    if (nodeSystem) {
        auto rootNode = nodeSystem->GetNode(root);
        auto node = nodeSystem->GetNode(target);
        if (rootNode && node) {
            if (rootNode->IsAncestorOf(*node)) {
                auto current = node;
                while (current) {
                    // Found root node, break.
                    if (current == rootNode) {
                        break;
                    }

                    // Add this to path.
                    if (result.empty()) {
                        result = current->GetName();
                    } else {
                        result = current->GetName() + "/" + result;
                    }

                    // Process parent next.
                    current = current->GetParent();
                }
            }
        }
    }

    return result;
}

void UpdateTrackTargets(IEcs& ecs, Entity animationEntity, Entity rootNode)
{
    auto& nameManager_ = *GetManager<INameComponentManager>(ecs);
    auto animationManager = GetManager<IAnimationComponentManager>(ecs);
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(ecs);
    auto& entityManager = ecs.GetEntityManager();

    auto* nodeSystem = GetSystem<INodeSystem>(ecs);
    CORE_ASSERT(nodeSystem);
    if (!nodeSystem) {
        return;
    }
    auto* node = nodeSystem->GetNode(rootNode);
    CORE_ASSERT(node);
    if (!node) {
        return;
    }

    if (const ScopedHandle<const AnimationComponent> animationData = animationManager->Read(animationEntity);
        animationData) {
        vector<Entity> targetEntities;
        targetEntities.reserve(animationData->tracks.size());
        std::transform(animationData->tracks.begin(), animationData->tracks.end(), std::back_inserter(targetEntities),
            [&nameManager = nameManager_, &node](const Entity& trackEntity) {
                if (auto nameHandle = nameManager.Read(trackEntity); nameHandle) {
                    if (nameHandle->name.empty()) {
                        return node->GetEntity();
                    } else {
                        if (auto targetNode = node->LookupNodeByPath(nameHandle->name); targetNode) {
                            return targetNode->GetEntity();
                        }
                    }
                }
                return Entity {};
            });
        if (animationData->tracks.size() == targetEntities.size()) {
            auto targetIt = targetEntities.begin();
            for (const auto& trackEntity : animationData->tracks) {
                if (auto track = animationTrackManager->Write(trackEntity); track) {
                    if (track->target) {
                        CORE_LOG_D("AnimationTrack %s already targetted",
                            to_hex(static_cast<const Entity&>(track->target).id).data());
                    }
                    track->target = entityManager.GetReferenceCounted(*targetIt);
                }
                ++targetIt;
            }
        }
    }
}
} // namespace

IProperty::Ptr EcsTrackAnimation::Keyframes() const
{
    return keyframes_;
}

size_t EcsTrackAnimation::AddKeyframe(float timestamp, const META_NS::IAny::ConstPtr& from)
{
    // TODO: Implement.
    return {};
}

bool EcsTrackAnimation::RemoveKeyframe(size_t index)
{
    // TODO: Implement.
    return false;
}
void EcsTrackAnimation::RemoveAllKeyframes()
{
    // TODO: Implement.
}

bool EcsTrackAnimation::Build(const META_NS::IMetadata::Ptr&)
{
    auto& registry = META_NS::GetObjectRegistry();

    auto arr = ConstructArrayProperty<float>("Keyframes", {}, META_NS::ObjectFlagBits::NONE);
    keyframes_ = arr.GetProperty();
    if (!keyframes_) {
        CORE_LOG_E("Invalid property type for EcsTrackAnimation: <float>");
        return false;
    }
    return true;
}

void EcsTrackAnimation::Step(const META_NS::IClock::ConstPtr& clock)
{
    // TODO: Implement.
}

void EcsTrackAnimation::Start()
{
    // TODO: Implement.
    META_NS::Invoke<META_NS::IOnChanged>(OnStarted());
}

void EcsTrackAnimation::Stop()
{
}

void EcsTrackAnimation::Pause()
{
}

void EcsTrackAnimation::Restart()
{
    Stop();
    Start();
}

void EcsTrackAnimation::Finish()
{
    // TODO: Implement.
}

void EcsTrackAnimation::Seek(float position)
{
    // TODO: Implement.
}

BASE_NS::string EcsAnimation::GetName() const
{
    return Name()->GetValue();
}

bool EcsAnimation::Build(const META_NS::IMetadata::Ptr& /*data*/)
{
    GetSelf<META_NS::IRequiredInterfaces>()->SetRequiredInterfaces({ IEcsTrackAnimation::UID });

    // TODO: Loop count.
    // TODO: Running.
    Name()->OnChanged()->AddHandler(MakeCallback<IOnChanged>([this] { OnNamePropertyChanged(); }));
    Duration()->OnChanged()->AddHandler(MakeCallback<IOnChanged>([this] { OnDurationPropertyChanged(); }));
    Progress()->OnChanged()->AddHandler(MakeCallback<IOnChanged>([this] { OnProgressPropertyChanged(); }));

    return true;
}

bool EcsAnimation::SetRootEntity(CORE_NS::Entity entity)
{
    if (EntityUtil::IsValid(root_)) {
        // Cannot change root entity, once set.
        return entity == root_;
    }

    root_ = entity;
    return true;
}

CORE_NS::Entity EcsAnimation::GetRootEntity() const
{
    return root_;
}

bool EcsAnimation::Retarget(CORE_NS::Entity entity)
{
    if (ecs_) {
        UpdateTrackTargets(*ecs_, GetEntity(), entity);
        root_ = entity;

        return true;
    }
    return false;
}

void EcsAnimation::SetEntity(CORE_NS::IEcs& ecs, CORE_NS::Entity entity)
{
    ecs_ = &ecs;
    entity_ = ecs_->GetEntityManager().GetReferenceCounted(entity);
    animationStateManager_ = nullptr;

    for (auto manager : ecs.GetComponentManagers()) {
        if (manager->GetName() == "AnimationStateComponent") {
            animationStateManager_ = manager;
            break;
        }
    }

    animationManager_ = GetManager<IAnimationComponentManager>(*ecs_);
    animationTrackManager_ = GetManager<IAnimationTrackComponentManager>(*ecs_);
    animationInputManager_ = GetManager<IAnimationInputComponentManager>(*ecs_);
    animationOutputManager_ = GetManager<IAnimationOutputComponentManager>(*ecs_);
    nameManager_ = GetManager<INameComponentManager>(*ecs_);

    OnAnimationNameChanged(IEcs::ComponentListener::EventType::MODIFIED);
    OnAnimationChanged(IEcs::ComponentListener::EventType::MODIFIED);

    if (auto ecsListener = ecsListener_.lock()) {
        auto po = GetSelf<SCENE_NS::IEcsProxyObject>();
        ecsListener->AddEntity(entity_, po, *nameManager_);
        ecsListener->AddEntity(entity_, po, *animationStateManager_);
        ecsListener->AddEntity(entity_, po, *animationManager_);
        ecsListener->AddEntity(entity_, po, *animationTrackManager_);
        ecsListener->AddEntity(entity_, po, *animationInputManager_);
    }

    // If the animation root is not set, then try to resolve it.
    if (!EntityUtil::IsValid(root_)) {
        root_ = TryResolveAnimationRoot();
    }

    META_ACCESS_PROPERTY(Valid)->SetValue(true);
}

CORE_NS::Entity EcsAnimation::TryResolveAnimationRoot()
{
    if (!ecs_) {
        return CORE_NS::Entity {};
    }

    CORE3D_NS::INodeSystem* nodeSystem = GetSystem<INodeSystem>(*ecs_);

    // We will try to resolve the animation root from the first animation track.
    auto tracks = META_NS::GetAll<IEcsTrackAnimation>(GetSelf<META_NS::IContainer>());
    if (tracks.empty()) {
        // No way to resolve, no tracks.
        return {};
    }

    IEcsTrackAnimation::Ptr track = tracks.at(0);

    BASE_NS::string path;
    if (auto nameHandle = nameManager_->Read(track->GetEntity()); nameHandle) {
        // The name of the track actually represents the path to the animated entity.
        path = nameHandle->name;
    }

    ISceneNode* node = nullptr;
    if (auto trackHandle = animationTrackManager_->Read(track->GetEntity()); trackHandle) {
        // This is the node that is being animated by the path.
        node = nodeSystem->GetNode(trackHandle->target);
        if (!node) {
            return {};
        }

        // Resolve the path to parent.
        while (!path.empty()) {
            BASE_NS::string suffix = node->GetName();
            if (EndsWith(path, suffix)) {
                // Remove name of this node from the path.
                path = string(path.substr(0, path.length() - suffix.length()));
                if (EndsWith(path, "/")) {
                    path = string(path.substr(0, path.length() - 1));
                }
                // Go up the tree.
                node = node->GetParent();
            } else {
                // Error.
                node = nullptr;
                break;
            }
        }
    }

    if (node) {
        SetRootEntity(node->GetEntity());
    }

    return node ? node->GetEntity() : Entity {};
}

void EcsAnimation::AddAnimation(const IAnimation::Ptr& animation)
{
    GetSelf<META_NS::IContainer>()->Add(interface_pointer_cast<IObject>(animation));
    // ToDo: Can we rely that someone has added an listener for tracks
}

void EcsAnimation::RemoveAnimation(const IAnimation::Ptr& animation)
{
    GetSelf<META_NS::IContainer>()->Remove(interface_pointer_cast<IObject>(animation));
    // ToDo: In principle, the tracks should deal common listener and removing parent, might be worth checking though
}

vector<IAnimation::Ptr> EcsAnimation::GetAnimations() const
{
    return META_NS::GetAll<IAnimation>(GetSelf<META_NS::IContainer>());
}

void EcsAnimation::DoComponentEvent(
    IEcs::ComponentListener::EventType type, const IComponentManager& componentManager, const CORE_NS::Entity& entity)
{
    bool isAnimationNameChange = componentManager.GetUid() == INameComponentManager::UID;
    bool isAnimationChange = componentManager.GetUid() == IAnimationComponentManager::UID;
    bool isAnimationStateChange = componentManager.GetUid() == animationStateManager_->GetUid();
    bool isAnimationInputChange = componentManager.GetUid() == IAnimationInputComponentManager::UID;
    bool isAnimationTrackChange = componentManager.GetUid() == IAnimationTrackComponentManager::UID;

    if (isAnimationChange || isAnimationStateChange) {
        // For animation and animation state, we are interested about changes concerning entity_.

        if (isAnimationChange) {
            // Animation has changed for this entity.
            OnAnimationChanged(type);
        } else if (isAnimationStateChange) {
            // Animation state has changed for this entity.
            OnAnimationStateChanged(type);
        }
    } else if (isAnimationTrackChange) {
        OnAnimationTracksChanged(type, entity);
    } else if (isAnimationInputChange) {
        OnAnimationInputsChanged(type, entity);
    } else if (isAnimationNameChange) {
        OnAnimationNameChanged(type);
    }
}

void EcsAnimation::OnAnimationStateChanged(IEcs::ComponentListener::EventType event)
{
    // This function is triggered when ECS dispatch changes at the end of the frame.
    if (!ecs_ || event != IEcs::ComponentListener::EventType::MODIFIED) {
        return;
    }

    // Propagate changes back to object's properties.
    auto stateHandle = animationStateManager_->GetData(entity_);
    const auto metaData = stateHandle->Owner()->MetaData();
    for (const auto& data : metaData) {
        if (data.name == "time") {
            auto* time = static_cast<const float*>(
                static_cast<const void*>(static_cast<const uint8_t*>(stateHandle->RLock()) + data.offset));

            updateGuard_ = true;
            auto duration = GetValue(TotalDuration()).ToSecondsFloat();
            if (duration > 0) {
                SetProgress(*time / duration);
            } else {
                SetProgress(0);
            }
            updateGuard_ = false;

            stateHandle->RUnlock();
            break;
        }
    }
}

void EcsAnimation::OnAnimationNameChanged(IEcs::ComponentListener::EventType event)
{
    // This function is triggered when ECS dispatch changes at the end of the frame.
    if (!ecs_ || event != IEcs::ComponentListener::EventType::MODIFIED) {
        return;
    }

    if (nameManager_->HasComponent(GetEntity())) {
        updateGuard_ = true;
        SetValue(Name(), nameManager_->Get(GetEntity()).name);
        updateGuard_ = false;
    }
}

void EcsAnimation::OnAnimationChanged(IEcs::ComponentListener::EventType event)
{
    // This function is triggered when ECS dispatch changes at the end of the frame.
    if (!ecs_ || event == IEcs::ComponentListener::EventType::DESTROYED) {
        return;
    }

    // Propagate changes back to object's properties.
    auto handle = animationManager_->Read(entity_);
    if (handle) {
        updateGuard_ = true;
        repeatCount_ = static_cast<int32_t>(handle->repeatCount);
        // Update animation duration.
        SetValue(META_ACCESS_PROPERTY(Duration), TimeSpan::Seconds(handle->duration));
        updateGuard_ = false;

        // Update animation tracks, if needed.
        if (IsAnimationTrackArrayModified()) {
            GatherAnimationTracks();
        }
    }
}

void EcsAnimation::OnAnimationTracksChanged(IEcs::ComponentListener::EventType event, CORE_NS::Entity entity)
{
    bool animationHasNewOrRemovedTracks = false;

    auto tracks = META_NS::GetAll<IEcsTrackAnimation>(GetSelf<META_NS::IContainer>());

    // Go through all created / modified / destroyed track entities.
    auto iterator = std::find_if(
        tracks.begin(), tracks.end(), [entity](const auto& track) { return track->GetEntity() == entity; });
    if (iterator != tracks.end()) {
        // Animation track was reported changed.
        if (event == IEcs::ComponentListener::EventType::MODIFIED) {
            // An animation track has been modified, so update it.
            auto index = std::distance(tracks.begin(), iterator);

            auto track = tracks.at(index);
            OnAnimationTrackChanged(*track, entity);

        } else {
            // If we have new or removed tracks, we will simply update all.
            animationHasNewOrRemovedTracks = true;
        }
    }

    // If new or removed tracks, refresh all.
    if (animationHasNewOrRemovedTracks) {
        GatherAnimationTracks();
    }
}

void EcsAnimation::OnAnimationInputsChanged(IEcs::ComponentListener::EventType event, CORE_NS::Entity entity)
{
    if (!ecs_) {
        return;
    }

    auto tracks = META_NS::GetAll<IEcsTrackAnimation>(GetSelf<META_NS::IContainer>());
    // Go through all created / modified / destroyed track entities.
    for (size_t i = 0; i < tracks.size(); ++i) {
        auto track = tracks.at(i);
        if (auto trackHandle = animationTrackManager_->Read(track->GetEntity()); trackHandle) {
            if (trackHandle->timestamps == entity) {
                // Timestamps for this track have changed.
                OnAnimationTimestampsChanged(*track, trackHandle->timestamps);
                break;
            }
        }
    }
}

bool EcsAnimation::IsAnimationTrackArrayModified()
{
    if (!ecs_) {
        return false;
    }

    auto tracks = META_NS::GetAll<IEcsTrackAnimation>(GetSelf<META_NS::IContainer>());
    auto handle = animationManager_->Read(entity_);
    if (handle) {
        if (handle->tracks.size() != tracks.size()) {
            // The amount of tracks is different.
            return true;
        }

        for (size_t i = 0; i < handle->tracks.size(); ++i) {
            if (tracks.at(i)->GetEntity() != handle->tracks[i]) {
                // Track entity id has changed.
                return true;
            }
        }
    }

    return false;
}

void EcsAnimation::GatherAnimationTracks()
{
    if (!ecs_) {
        return;
    }

    // Take copy of the animation tracks.
    auto oldTracks = META_NS::GetAll<IEcsTrackAnimation>(GetSelf<META_NS::IContainer>());

    // Clear tracks.
    GetSelf<META_NS::IContainer>()->RemoveAll();

    // Update animation tracks.
    auto handle = animationManager_->Read(entity_);
    if (handle) {
        for (const auto& track : handle->tracks) {
            // See if we have this track stored.
            auto it = std::find_if(oldTracks.begin(), oldTracks.end(), [track](const auto& t) {
                if (t->GetEntity() == track) {
                    return true;
                }
                return false;
            });

            IEcsTrackAnimation::Ptr animationTrack;
            if (it != oldTracks.end()) {
                animationTrack = *it;
            } else {
                animationTrack =
                    META_NS::GetObjectRegistry().Create<IEcsTrackAnimation>(SCENE_NS::ClassId::EcsTrackAnimation);
                interface_pointer_cast<SCENE_NS::IEcsProxyObject>(animationTrack)
                    ->SetCommonListener(ecsListener_.lock());
            }

            OnAnimationTrackChanged(*animationTrack, track);
            GetSelf<META_NS::IContainer>()->Add(interface_pointer_cast<IObject>(animationTrack));
        }
    }
}

void EcsAnimation::OnAnimationTrackChanged(IEcsTrackAnimation& track, Entity trackEntity)
{
    if (!ecs_ || updateGuard_) {
        return;
    }

    if (auto trackHandle = animationTrackManager_->Read(trackEntity); trackHandle) {
        auto nameComponent = nameManager_->Get(trackHandle->target.operator Entity());

        track.SetEntity(trackEntity);

        auto named = interface_cast<INamed>(&track);
        if (named) {
            SetValue(named->Name(), nameComponent.name + '.' + trackHandle->property.data());
        }
        OnAnimationTimestampsChanged(track, trackHandle->timestamps);
    }
}

void EcsAnimation::UpdateTimestamps(IEcsTrackAnimation& track, Entity timestampEntity)
{
    if (!ecs_) {
        return;
    }

    if (animationInputManager_->HasComponent(timestampEntity)) {
        const auto timestamps = animationInputManager_->Get(timestampEntity);
        const auto trackAnimation = interface_cast<ITrackAnimation>(&track);
        const auto timedAnimation = interface_cast<ITimedAnimation>(&track);

        vector<float> times;
        times.reserve(timestamps.timestamps.size());

        const auto duration = META_NS::GetValue(timedAnimation->Duration()).ToSecondsFloat();

        for (const auto timestamp : timestamps.timestamps) {
            const auto offset = (duration > 0) ? (timestamp / duration) : 0.0f;
            times.push_back(offset);
        }

        trackAnimation->Timestamps()->SetValue(times);
    }
}
void EcsAnimation::OnAnimationTimestampsChanged(IEcsTrackAnimation& track, Entity timestampEntity)
{
    if (!ecs_ || updateGuard_) {
        return;
    }

    UpdateTimestamps(track, timestampEntity);

    // If any of the tracks in this animation is sharing the same timestamps, then make all tracks read-only.
    bool hasSharedTimestamps = false;

    auto tracks = META_NS::GetAll<IEcsTrackAnimation>(GetSelf<META_NS::IContainer>());
    for (const auto& other : tracks) {
        if (other->GetEntity() != track.GetEntity()) {
            if (auto trackHandle = animationTrackManager_->Read(other->GetEntity()); trackHandle) {
                if (trackHandle->timestamps == timestampEntity) {
                    hasSharedTimestamps = true;
                    break;
                }
            }
        }
    }

    if (GetValue(ReadOnly()) != hasSharedTimestamps) {
        META_ACCESS_PROPERTY(ReadOnly)->SetValue(hasSharedTimestamps);
    }
}

void EcsAnimation::SetDuration(uint32_t ms)
{
    if (!ecs_) {
        return;
    }

    const auto newDuration = TimeSpan::Milliseconds(ms);
    const auto tracks = META_NS::GetAll<IEcsTrackAnimation>(GetSelf<META_NS::IContainer>());
    for (const auto& track : tracks) {
        if (const auto trackHandle = animationTrackManager_->Read(track->GetEntity()); trackHandle) {
            interface_cast<META_NS::ITimedAnimation>(track)->Duration()->SetValue(newDuration);
            UpdateTimestamps(*track, trackHandle->timestamps);
        }
    }

    META_ACCESS_PROPERTY(Duration)->SetValue(newDuration);
}

void EcsAnimation::AddKey(IEcsTrackAnimation::Ptr track, float time)
{
    if (!ecs_) {
        return;
    }

    updateGuard_ = true;

    auto handle = animationTrackManager_->Read(track->GetEntity());
    if (handle) {
        auto propertyData = GetProperty(handle->component, handle->target, handle->property);
        if (propertyData) {
            SetKeyFrameData(track->GetEntity(), time, propertyData.data);
        }
    }

    UpdateAnimationTrackDuration(track->GetEntity());

    updateGuard_ = false;

    uint32_t timeMs = static_cast<uint32_t>(time * 1000.0f);
    if (GetValue(Duration()).ToMilliseconds() < timeMs) {
        META_ACCESS_PROPERTY(Duration)->SetValue(TimeSpan::Milliseconds(timeMs));
    }

    OnAnimationTrackChanged(*track, track->GetEntity());
}

void EcsAnimation::RemoveKey(IEcsTrackAnimation::Ptr track, uint32_t index)
{
    if (!ecs_) {
        return;
    }

    updateGuard_ = true;

    // remove timestamp at pos
    if (auto animationTrack = animationTrackManager_->Read(track->GetEntity()); animationTrack) {
        auto stampsEntity = animationTrack->timestamps.operator Entity();
        auto inputData = animationInputManager_->Write(stampsEntity);
        vector<float>::iterator iit = inputData->timestamps.begin();
        inputData->timestamps.erase(iit + index);
        // remove data at pos
        auto dataEntity = animationTrack->data.operator Entity();
        if (auto outputData = animationOutputManager_->Write(dataEntity); outputData) {
            auto targetEntity = animationTrack->target.operator Entity();

            auto manager = ecs_->GetComponentManager(animationTrack->component);

            auto target = manager->GetData(targetEntity);
            auto poffset = RLock(*target, animationTrack->property);
            vector<uint8_t>::iterator oit = outputData->data.begin();
            outputData->data.erase(oit + (index * poffset.property->size),
                oit + (index * poffset.property->size) + poffset.property->size);
        }
    }

    updateGuard_ = false;

    OnAnimationTrackChanged(*track, track->GetEntity());
}

void EcsAnimation::UpdateKey(IEcsTrackAnimation::Ptr track, uint32_t oldKeyIndex, uint32_t newKeyIndex, float time)
{
    if (!ecs_) {
        return;
    }

    updateGuard_ = true;

    {
        auto animationTrack = animationTrackManager_->Read(track->GetEntity());
        auto stampsEntity = animationTrack->timestamps.operator Entity();
        auto inputData = animationInputManager_->Write(stampsEntity);

        auto dataEntity = animationTrack->data.operator Entity();
        auto outputData = animationOutputManager_->Write(dataEntity);
        auto targetEntity = animationTrack->target.operator Entity();

        vector<float>::iterator iit = inputData->timestamps.begin();
        inputData->timestamps.erase(iit + oldKeyIndex);

        auto manager = ecs_->GetComponentManager(animationTrack->component);

        auto target = manager->GetData(targetEntity);
        auto poffset = RLock(*target, animationTrack->property);
        if (poffset) {
            vector<uint8_t>::iterator oit = outputData->data.begin();

            vector<uint8_t> moveData(oit + (oldKeyIndex * poffset.property->size),
                oit + (oldKeyIndex * poffset.property->size) + poffset.property->size);

            outputData->data.erase(oit + (oldKeyIndex * poffset.property->size),
                oit + (oldKeyIndex * poffset.property->size) + poffset.property->size);

            inputData->timestamps.insert(iit + newKeyIndex, time);

            vector<uint8_t>::iterator noit = outputData->data.begin();
            outputData->data.insert(
                noit + (newKeyIndex * poffset.property->size), std::begin(moveData), std::end(moveData));
        }
    }

    updateGuard_ = false;

    uint32_t timeMs = static_cast<uint32_t>(time * 1000.0f);
    if (GetValue(Duration()).ToMilliseconds() < timeMs) {
        META_ACCESS_PROPERTY(Duration)->SetValue(TimeSpan::Milliseconds(timeMs));
    }

    OnAnimationTrackChanged(*track, track->GetEntity());
}

void EcsAnimation::OnDestroyAnimationTrack(IEcsTrackAnimation::Ptr track)
{
    if (ecs_) {
        if (auto trackHandle = animationTrackManager_->Read(track->GetEntity()); trackHandle) {
            // Destroy timestamps and keys.
            ecs_->GetEntityManager().Destroy(trackHandle->timestamps);
            ecs_->GetEntityManager().Destroy(trackHandle->data);
        }

        {
            // Remove track from animation.
            const auto animationHandle = animationManager_->Write(GetEntity());
            auto it = std::find(animationHandle->tracks.begin(), animationHandle->tracks.end(), track->GetEntity());
            if (it != animationHandle->tracks.end()) {
                animationHandle->tracks.erase(it);
            }
        }

        // Destroy track.
        ecs_->GetEntityManager().Destroy(track->GetEntity());
    }

    GetSelf<META_NS::IContainer>()->Remove(interface_pointer_cast<IObject>(track));
}

BASE_NS::vector<CORE_NS::EntityReference> EcsAnimation::GetAllRelatedEntities() const
{
    BASE_NS::vector<CORE_NS::EntityReference> result;

    if (animationManager_) {
        auto animationHandle = animationManager_->Read(GetEntity());
        if (animationHandle) {
            for (auto& track : animationHandle->tracks) {
                const auto trackHandle = animationTrackManager_->Read(track);
                if (trackHandle) {
                    if (EntityUtil::IsValid(trackHandle->timestamps)) {
                        result.push_back(ecs_->GetEntityManager().GetReferenceCounted(trackHandle->timestamps));
                    }

                    if (EntityUtil::IsValid(trackHandle->data)) {
                        result.push_back(ecs_->GetEntityManager().GetReferenceCounted(trackHandle->data));
                    }

                    result.push_back(ecs_->GetEntityManager().GetReferenceCounted(track));
                }
            }
        }

        // Add self.
        result.push_back(ecs_->GetEntityManager().GetReferenceCounted(GetEntity()));
    }

    return result;
}

IEcsTrackAnimation::Ptr EcsAnimation::CreateAnimationTrack(
    CORE_NS::Entity rootEntity, CORE_NS::Entity targetEntity, BASE_NS::string_view fullPropertyPath)
{
    // Extract property path.
    auto separatorPosition = fullPropertyPath.find_first_of('.');
    if (!ecs_ || separatorPosition == BASE_NS::string::npos) {
        return {};
    }

    IComponentManager* componentManager { nullptr };
    auto componentManagerName = fullPropertyPath.substr(0, separatorPosition);
    auto propertyPath = fullPropertyPath.substr(separatorPosition + 1);

    for (const auto& manager : ecs_->GetComponentManagers()) {
        if (manager->GetName() == componentManagerName) {
            componentManager = manager;
            break;
        }
    }

    if (!componentManager) {
        return {};
    }

    updateGuard_ = true;

    EntityReference animationTrack;

    if (IPropertyHandle* targetHandle = componentManager->GetData(targetEntity); targetHandle) {
        if (auto po = RLock(*targetHandle, propertyPath); po) {
            BASE_NS::string targetName = "Unnamed";
            if (nameManager_->HasComponent(targetEntity)) {
                targetName = nameManager_->Get(targetEntity).name;
            }

            const auto timeStamps = ecs_->GetEntityManager().CreateReferenceCounted();
            {
                NameComponent nameComponent;
                nameComponent.name = "TimeStamps - " + targetName + ":" + propertyPath;
                nameManager_->Set(timeStamps, nameComponent);

                animationInputManager_->Create(timeStamps);
            }

            const auto keys = ecs_->GetEntityManager().CreateReferenceCounted();
            {
                NameComponent nameComponent;
                nameComponent.name = "Keys - " + targetName + ":" + propertyPath;
                nameManager_->Set(keys, nameComponent);

                animationOutputManager_->Create(keys);
                auto keysHandle = animationOutputManager_->Write(keys);
                keysHandle->type = po.property->type;
            }

            const auto targetRef = ecs_->GetEntityManager().GetReferenceCounted(targetEntity);
            animationTrack = ecs_->GetEntityManager().CreateReferenceCounted();
            {
                NameComponent nameComponent;
                nameComponent.name = ResolvePathToAnimationRoot(*ecs_, rootEntity, targetEntity);
                nameManager_->Set(animationTrack, nameComponent);

                animationTrackManager_->Create(animationTrack);
                auto trackHandle = animationTrackManager_->Write(animationTrack);
                trackHandle->component = componentManager->GetUid();
                trackHandle->property = propertyPath;
                trackHandle->interpolationMode = AnimationTrackComponent::Interpolation::LINEAR;
                trackHandle->timestamps = timeStamps;
                trackHandle->data = keys;
                trackHandle->target = targetRef;
            }

            const auto animationHandle = animationManager_->Write(GetEntity());
            animationHandle->tracks.emplace_back(animationTrack);
        }
    }

    updateGuard_ = false;

    IEcsTrackAnimation::Ptr track =
        META_NS::GetObjectRegistry().Create<IEcsTrackAnimation>(SCENE_NS::ClassId::EcsTrackAnimation);
    OnAnimationTrackChanged(*track, animationTrack);

    GetSelf<META_NS::IContainer>()->Add(interface_pointer_cast<IObject>(track));

    return track;
}

IEcsTrackAnimation::Ptr EcsAnimation::GetAnimationTrack(CORE_NS::Entity target, BASE_NS::string_view fullPropertyPath)
{
    // Extract property path.
    auto separatorPosition = fullPropertyPath.find_first_of('.');
    if (!ecs_ || separatorPosition == BASE_NS::string::npos) {
        return {};
    }

    auto propertyPath = fullPropertyPath.substr(separatorPosition + 1);

    auto tracks = META_NS::GetAll<IEcsTrackAnimation>(GetSelf<META_NS::IContainer>());
    for (auto& track : tracks) {
        const auto trackHandle = animationTrackManager_->Read(track->GetEntity());
        if (trackHandle) {
            if (trackHandle->target == target && trackHandle->property == propertyPath) {
                return track;
            }
        }
    }

    return {};
}

void EcsAnimation::DestroyAnimationTrack(IEcsTrackAnimation::Ptr track)
{
    updateGuard_ = true;
    OnDestroyAnimationTrack(track);
    updateGuard_ = false;
}

void EcsAnimation::DestroyAllAnimationTracks()
{
    updateGuard_ = true;
    auto container = GetSelf<META_NS::IContainer>();
    while (container->GetSize() > 0) {
        auto topmost = container->GetAt(0);
        if (auto track = interface_pointer_cast<IEcsTrackAnimation>(topmost)) {
            OnDestroyAnimationTrack(track);
        } else {
            container->Remove(topmost);
        }
    }
    updateGuard_ = false;
}

void EcsAnimation::Destroy()
{
    SCENE_PLUGIN_VERBOSE_LOG("Tearing down: %s", META_NS::GetValue(Name()).c_str());

    if (ecs_ && entity_) {
        if (auto ecsListener = ecsListener_.lock()) {
            auto po = GetSelf<SCENE_NS::IEcsProxyObject>();
            ecsListener->RemoveEntity(entity_, po, *nameManager_);
            ecsListener->RemoveEntity(entity_, po, *animationStateManager_);
            ecsListener->RemoveEntity(entity_, po, *animationManager_);
            ecsListener->RemoveEntity(entity_, po, *animationTrackManager_);
            ecsListener->RemoveEntity(entity_, po, *animationInputManager_);
        }
    }

    root_ = {};
    entity_ = {};
}

void EcsAnimation::OnDurationPropertyChanged()
{
    SetValue(META_ACCESS_PROPERTY(TotalDuration), GetValue(Duration()) * repeatCount_);
    if (!ecs_ || updateGuard_) {
        return;
    }

    auto handle = animationManager_->Write(entity_);
    if (handle) {
        handle->duration = GetValue(Duration()).ToSecondsFloat();
    }
}

void EcsAnimation::OnNamePropertyChanged()
{
    if (!ecs_ || updateGuard_) {
        return;
    }

    auto handle = nameManager_->Write(entity_);
    if (handle) {
        handle->name = GetValue(Name());
    }
}

void EcsAnimation::OnProgressPropertyChanged()
{
    if (updateGuard_) {
        return;
    }

    auto progress = GetValue(Progress());
    auto duration = GetValue(TotalDuration()).ToMilliseconds();
    SetTime(uint32_t(progress * duration));
}

CORE_NS::Entity EcsAnimation::GetEntity() const
{
    return entity_;
}

void EcsAnimation::SetProgress(float progress)
{
    META_ACCESS_PROPERTY(Progress)->SetValue(Base::Math::clamp(progress, 0.0f, 1.0f));
}

void EcsAnimation::SetTime(uint32_t value)
{
    if (ecs_) {
        auto stateHandle = animationStateManager_->GetData(entity_);
        const auto metaData = stateHandle->Owner()->MetaData();
        for (const auto& data : metaData) {
            if (data.name == "time") {
                auto* time =
                    static_cast<float*>(static_cast<void*>(static_cast<uint8_t*>(stateHandle->WLock()) + data.offset));
                *time = value / 1000.0f;
                stateHandle->WUnlock();
                break;
            }
        }
    }
}

void EcsAnimation::Step(const IClock::ConstPtr& clock)
{
    auto duration = GetValue(TotalDuration()).ToSecondsFloat();
    if (duration <= 0.0f) {
        return;
    }

    if (!lastFrameTime_) {
        lastFrameTime_ = clock->GetTime();
    }

    auto step = (clock->GetTime() - *lastFrameTime_).ToSecondsFloat();
    SetProgress(GetValue(Progress()) + (step / duration));

    lastFrameTime_ = clock->GetTime();
}

void EcsAnimation::Start()
{
    auto loopAnimation_ = true;

    if (animationManager_) {
        if (auto animation = animationManager_->Write(entity_); animation) {
            animation->state = AnimationComponent::PlaybackState::PAUSE;
            if (loopAnimation_) {
                animation->repeatCount = AnimationComponent::REPEAT_COUNT_INFINITE;
            } else {
                animation->repeatCount = 0;
            }
        }
    }

    lastFrameTime_.reset();
    META_ACCESS_PROPERTY(Running)->SetValue(true);
    META_NS::Invoke<META_NS::IOnChanged>(OnStarted());
}

void EcsAnimation::Stop()
{
    if (animationManager_) {
        if (auto animation = animationManager_->Write(entity_); animation) {
            animation->state = AnimationComponent::PlaybackState::PAUSE;
        }
    }
    lastFrameTime_.reset();
    META_ACCESS_PROPERTY(Running)->SetValue(false);
}

void EcsAnimation::Pause()
{
    if (animationManager_) {
        if (auto animation = animationManager_->Write(entity_); animation) {
            animation->state = AnimationComponent::PlaybackState::PAUSE;
        }
    }
    META_ACCESS_PROPERTY(Running)->SetValue(false);
}

void EcsAnimation::Restart()
{
    Stop();
    Start();
}

void EcsAnimation::Finish()
{
    Seek(1.0f);
}

void EcsAnimation::Seek(float position)
{
    // TODO: Implement.
    SetProgress(position);
}

EcsAnimation::Data EcsAnimation::GetProperty(Uid componentUid, Entity entity, string property) const
{
    Data data;
    if (ecs_) {
        auto cm = ecs_->GetComponentManager(componentUid);
        if (IPropertyHandle* targetHandle = cm->GetData(entity); targetHandle) {
            if (auto po = RLock(*targetHandle, property); po) {
                data.property = &(po.property->type);
                const uint8_t* src = reinterpret_cast<uint8_t*>(po.offset);
                data.data.resize(po.property->size);
                CloneData(data.data.data(), data.data.size(), src, po.property->size);
            }
        }
    }
    return data;
}

void EcsAnimation::SetKeyFrameData(Entity animationTrack, float timeStamp, vector<uint8_t> valueData)
{
    if (!ecs_) {
        return;
    }

    BASE_ASSERT(EntityUtil::IsValid(animationTrack));
    auto trackHandle = animationTrackManager_->Read(animationTrack);
    if (trackHandle) {
        const auto timeStamps = trackHandle->timestamps;
        const auto keys = trackHandle->data;
        BASE_ASSERT(EntityUtil::IsValid(timeStamps));
        BASE_ASSERT(EntityUtil::IsValid(keys));

        size_t index = 0;
        bool replace = false;
        // insert/replace the timestamp
        // find out the position where keyframe belongs based on timestamp
        auto timeLineHandle = animationInputManager_->Write(timeStamps);
        if (timeLineHandle) {
            auto& data = timeLineHandle->timestamps;
            const size_t oldCount = data.size();
            const size_t newCount = oldCount + 1;

            for (vector<float>::iterator it = data.begin(); it < data.end(); it++) {
                if (*it == timeStamp) {
                    replace = true;
                    break;
                }
                if (*it > timeStamp) {
                    break;
                }
                index++;
            }

            if (!replace) {
                // reserve space for one more
                data.reserve(newCount);
                vector<float>::iterator insertIterator = data.begin() + index;
                data.insert(insertIterator, timeStamp);
            }
        }

        auto keysHandle = animationOutputManager_->Write(keys);
        if (keysHandle) {
            auto& data = keysHandle->data;
            const size_t oldSize = data.size();
            const size_t oldCount = oldSize / valueData.size();

            if (!replace) {
                const size_t newCount = oldCount + 1;
                const size_t newSize = newCount * valueData.size();
                // reserve space for one more
                data.reserve(newSize);
                vector<unsigned char>::iterator insertIterator = data.begin() + (index * valueData.size());
                data.insert(insertIterator, valueData.begin(), valueData.end());
            } else {
                auto dst = data.data() + index * valueData.size();
                CloneData(dst, valueData.size(), valueData.data(), valueData.size());
            }
        }
    }
}

void EcsAnimation::UpdateAnimationTrackDuration(Entity animationTrack)
{
    if (ecs_) {
        auto duration = GetTrackDuration(animationTrack);
        auto animationTrackHandle = animationTrackManager_->Read(animationTrack);
        if (animationTrackHandle) {
            auto target = animationTrackHandle->target;
            if (EntityUtil::IsValid(target)) {
                if (IPropertyHandle* targetHandle = animationManager_->GetData(target); targetHandle) {
                    PropertyData pData;
                    if (auto po = pData.WLock(*targetHandle, "duration"); po) {
                        float* dst = reinterpret_cast<float*>(po.offset);
                        *dst = duration;
                    }
                }
            }
        }
    }
}

float EcsAnimation::GetTrackDuration(Entity animationTrack)
{
    BASE_ASSERT(EntityUtil::IsValid(animationTrack));

    float duration = 0.0f;

    if (ecs_) {
        auto trackHandle = animationTrackManager_->Read(animationTrack);
        if (trackHandle) {
            const auto timeStamps = trackHandle->timestamps;
            BASE_ASSERT(EntityUtil::IsValid(timeStamps));

            auto timeLineHandle = animationInputManager_->Read(timeStamps);
            if (timeLineHandle) {
                auto& stamps = timeLineHandle->timestamps;
                for (auto& t : stamps) {
                    if (t > duration) {
                        duration = t;
                    }
                }
            }
        }
    }

    return duration;
}
/*
bool EcsAnimation::Export(Serialization::IExportContext& context, Serialization::ClassPrimitive& value) const
{
    return ObjectContainerFwd::Export(context, value);
}

bool EcsAnimation::Import(Serialization::IImportContext& context, const Serialization::ClassPrimitive& value)
{
    return ObjectContainerFwd::Import(context, value);
}
*/
void RegisterEcsAnimationObjectType()
{
    META_NS::GetObjectRegistry().RegisterObjectType<EcsTrackAnimation>();
    META_NS::GetObjectRegistry().RegisterObjectType<EcsAnimation>();
}

void UnregisterEcsAnimationObjectType()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<EcsTrackAnimation>();
    META_NS::GetObjectRegistry().UnregisterObjectType<EcsAnimation>();
}

SCENE_END_NAMESPACE()
