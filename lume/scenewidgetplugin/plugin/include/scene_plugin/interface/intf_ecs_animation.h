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
#ifndef SCENE_PLUGIN_INTF_ECS_ANIMATION_H
#define SCENE_PLUGIN_INTF_ECS_ANIMATION_H

#include <scene_plugin/namespace.h>

#include <core/ecs/intf_ecs.h>

#include <meta/ext/object.h>
#include <meta/ext/event_impl.h>
#include <meta/interface/animation/intf_animation.h>

SCENE_BEGIN_NAMESPACE()

class IEcsTrackAnimation : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEcsTrackAnimation, "fba5e210-b382-4391-b589-3e9b356556d5")
public:
    virtual void SetEntity(CORE_NS::Entity entity) = 0;
    virtual CORE_NS::Entity GetEntity() const = 0;
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IEcsTrackAnimation::Ptr);
META_TYPE(SCENE_NS::IEcsTrackAnimation::WeakPtr);

SCENE_BEGIN_NAMESPACE()

REGISTER_CLASS(EcsAnimation, "267fb911-f25a-4921-9909-8c71c4b499fe", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(EcsTrackAnimation, "0c8441e9-ed38-4bb4-9946-e0f71ae796cc", META_NS::ObjectCategoryBits::NO_CATEGORY)

class IEcsAnimation : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEcsAnimation, "6eab865c-6aaf-472d-bf23-302df559c97e")

public:
    META_READONLY_PROPERTY(bool, ReadOnly)

    virtual bool SetRootEntity(CORE_NS::Entity entity) = 0;
    virtual CORE_NS::Entity GetRootEntity() const = 0;

    virtual void SetEntity(CORE_NS::IEcs& ecs, CORE_NS::Entity entity) = 0;
    virtual CORE_NS::Entity GetEntity() const = 0;

    virtual bool Retarget(CORE_NS::Entity entity) = 0;

    virtual void SetDuration(uint32_t ms) = 0;

    virtual void AddKey(IEcsTrackAnimation::Ptr track, float time) = 0;

    // Remove key and timestamp at index.
    virtual void RemoveKey(IEcsTrackAnimation::Ptr track, uint32_t index) = 0;

    // Change key data position index.
    virtual void UpdateKey(IEcsTrackAnimation::Ptr track, uint32_t oldKeyIndex, uint32_t newKeyIndex, float time) = 0;

    virtual IEcsTrackAnimation::Ptr CreateAnimationTrack(
        CORE_NS::Entity rootEntity, CORE_NS::Entity target, BASE_NS::string_view property) = 0;
    virtual IEcsTrackAnimation::Ptr GetAnimationTrack(CORE_NS::Entity target, BASE_NS::string_view property) = 0;
    virtual void DestroyAnimationTrack(IEcsTrackAnimation::Ptr track) = 0;
    virtual void DestroyAllAnimationTracks() = 0;
    virtual void Destroy() = 0;

    virtual BASE_NS::vector<CORE_NS::EntityReference> GetAllRelatedEntities() const = 0;
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IEcsAnimation::Ptr);
META_TYPE(SCENE_NS::IEcsAnimation::WeakPtr);

void RegisterEcsAnimationObjectType();
void UnregisterEcsAnimationObjectType();

#endif // SCENE_PLUGIN_INTF_ECS_ANIMATION_H
