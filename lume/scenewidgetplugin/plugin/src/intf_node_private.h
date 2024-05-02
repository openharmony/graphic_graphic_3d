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
#ifndef INTF_NODE_PRIVATE_H
#define INTF_NODE_PRIVATE_H

#include <scene_plugin/interface/intf_ecs_object.h>
#include <scene_plugin/interface/intf_material.h>
#include <scene_plugin/interface/intf_scene.h>

#include "scene_holder.h"

enum NodeLifeCycle : uint32_t {
    NODE_LC_UNKNOWN = 0,
    NODE_LC_MIRROR_EXISTING = 1 << 1,
    NODE_LC_CLONED = 1 << 2,
    NODE_LC_CREATED = 1 << 3,
};

REGISTER_INTERFACE(INodeEcsInterfacePrivate, "f78b73b2-1f9e-45d8-9938-93525604ada0")
class INodeEcsInterfacePrivate : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, INodeEcsInterfacePrivate, InterfaceId::INodeEcsInterfacePrivate)
public:
    virtual bool Initialize(SCENE_NS::IEcsScene::Ptr& scene, SCENE_NS::IEcsObject::Ptr& ecsObject,
        SCENE_NS::INode::Ptr parent, const BASE_NS::string& path, const BASE_NS::string& name,
        SceneHolder::WeakPtr sceneHolder, CORE_NS::Entity entity) = 0;
    virtual bool CompleteInitialization(const BASE_NS::string& path) = 0;

    virtual SCENE_NS::IEcsScene::Ptr EcsScene() const = 0;
    virtual SCENE_NS::IEcsObject::Ptr EcsObject() const = 0;
    virtual SceneHolder::Ptr SceneHolder() const = 0;

    virtual void SetPath(const BASE_NS::string& path, const BASE_NS::string& name, CORE_NS::Entity entity) = 0;

    virtual void ClaimOwnershipOfEntity(bool ownsEntity) = 0;

    // Node is potentially moving to a new position within a parent
    virtual void SetIndex(size_t index) = 0;

    // Node was removed from container, we don't currently reflect this as such to ecs
    // We may have to change that someday (even soon)
    virtual void RemoveIndex(size_t index) = 0;

    virtual META_NS::IProperty::Ptr GetLifecycleInfo(bool create = true) = 0;

    virtual bool ShouldExport() const = 0;
};

REGISTER_INTERFACE(ITextureStorage, "259a9809-d458-4eb4-a5c5-6b0134780d2d")
class ITextureStorage : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITextureStorage, InterfaceId::ITextureStorage)
public:
    virtual SCENE_NS::ITextureInfo::Ptr GetTextureInfo(size_t ix) const = 0;
};

REGISTER_INTERFACE(IMaterialInputBindable, "a3438f52-6edf-4e9e-b86a-68cc92d01ae5")
class IMaterialInputBindable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMaterialInputBindable, InterfaceId::IMaterialInputBindable)
public:
    virtual void Bind(const ITextureStorage::Ptr& infos) = 0;
};

REGISTER_INTERFACE(ISceneHolderRef, "57dd6eab-aeb8-40b0-b606-512787c8773b")
class ISceneHolderRef : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISceneHolderRef, InterfaceId::IMaterialInputBindable)
public:
    virtual void SetSceneHolder(const SceneHolder::WeakPtr& sh) = 0;
    virtual SceneHolder::Ptr SceneHolder() = 0;
    // ToDo: This should be part of some other interface
    virtual void SetIndex(size_t ix) {}
};

SCENE_BEGIN_NAMESPACE()
REGISTER_INTERFACE(IPendingRequestData, "197c37b3-dd55-45af-9b14-aada2b224046")
template<typename T>
class IPendingRequestData : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPendingRequestData, InterfaceId::IPendingRequestData)
public:
    virtual void Add(const T& data) = 0;
    virtual void MarkReady() = 0;
    virtual BASE_NS::vector<BASE_NS::string>& MetaData() = 0;
    virtual BASE_NS::vector<T>& MutableData() = 0;
};

REGISTER_CLASS(PendingDistanceRequest, "d645fcd5-871c-4780-89a9-899df02b7121", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(
    PendingGraphicsStateRequest, "2d21475b-19be-4110-938a-f37724f518c4", META_NS::ObjectCategoryBits::NO_CATEGORY)
REGISTER_CLASS(PendingVec3Request, "36eb3eb2-f81d-4db5-8ab7-3c76e8674ecf", META_NS::ObjectCategoryBits::NO_CATEGORY)
SCENE_END_NAMESPACE()

#endif
