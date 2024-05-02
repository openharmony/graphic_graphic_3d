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
#ifndef SCENEPLUGIN_INTF_SUBMESHBRIDGE_H
#define SCENEPLUGIN_INTF_SUBMESHBRIDGE_H
#include <scene_plugin/namespace.h>

#include <3d/ecs/components/mesh_component.h>

#include <meta/base/types.h>
#include <meta/interface/property/intf_property.h>

#include "intf_node_private.h"
SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(ISubMeshBridge, "8be436af-9556-44ff-9f9c-3472b168cef4")
class ISubMeshBridge : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISubMeshBridge, InterfaceId::ISubMeshBridge)
public:
    virtual void Initialize(CORE3D_NS::IMeshComponentManager* componentManager, CORE_NS::Entity entity,
        META_NS::IProperty::Ptr submeshes, INodeEcsInterfacePrivate::Ptr node) = 0;
    virtual void SetRenderSortLayerOrder(size_t index, uint8_t value) = 0;
    virtual void SetMaterialToEcs(size_t index, SCENE_NS::IMaterial::Ptr& material) = 0;
    virtual void SetAABBMin(size_t index, BASE_NS::Math::Vec3) = 0;
    virtual void SetAABBMax(size_t index, BASE_NS::Math::Vec3) = 0;
    virtual void RemoveSubmesh(int32_t index) = 0;
    virtual CORE_NS::Entity GetEntity() const = 0;
};

REGISTER_INTERFACE(ISubMeshPrivate, "bda354b5-7747-488f-b0f6-50c3d98bee2d")
class ISubMeshPrivate : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISubMeshPrivate, InterfaceId::ISubMeshPrivate)
public:
    virtual void AddSubmeshBrigde(ISubMeshBridge::Ptr bridge) = 0;
    virtual void RemoveSubmeshBrigde(ISubMeshBridge::Ptr bridge) = 0;
    virtual CORE_NS::Entity GetEntity() const = 0;

    virtual void SetDefaultMaterial(SCENE_NS::IMaterial::Ptr material) = 0;
    virtual void RestoreDefaultMaterial() = 0;

    virtual void SetOverrideMaterial(SCENE_NS::IMaterial::Ptr material) = 0;
};

SCENE_END_NAMESPACE()
#endif