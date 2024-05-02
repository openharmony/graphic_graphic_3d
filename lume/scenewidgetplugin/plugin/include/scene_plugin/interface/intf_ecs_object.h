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

#ifndef SCENEPLUGIN_INTF_ECSOBJECT_H
#define SCENEPLUGIN_INTF_ECSOBJECT_H

#include <scene_plugin/interface/intf_entity_collection.h>
#include <scene_plugin/namespace.h>

#include <base/containers/vector.h>
#include <base/containers/unordered_map.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_ecs.h>
#include <core/plugin/intf_interface.h>

#include <meta/base/types.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/property/property.h>

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IEcsValueInformation, "20c0b2da-59d1-4f63-93be-a5015f3c88f1")
class IEcsValueInformation : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEcsValueInformation, InterfaceId::IEcsValueInformation)
public:
    virtual BASE_NS::string_view GetPath() const = 0;
    virtual CORE_NS::Entity GetEntity() const = 0;
    virtual CORE_NS::IEcs::Ptr GetEcs() const = 0;
    virtual BASE_NS::Uid GetComponentUid() const = 0;
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IEcsValueInformation::Ptr);
META_TYPE(SCENE_NS::IEcsValueInformation::WeakPtr);

SCENE_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IEcsObject, "f3eb2abf-56c5-49a2-bcf7-c1e8807bb764")
REGISTER_CLASS(EcsObject, "067c9c4f-ecde-417b-b3be-d4e48dfe4d14", META_NS::ObjectCategoryBits::NO_CATEGORY)

class IEcsObject : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEcsObject, InterfaceId::IEcsObject)
public:
    enum EcsObjectStatus {
        /** Instance exists, but the properties are not bound*/
        ECS_OBJ_STATUS_DISCONNECTED = 0,
        /** The node is bound to 3D scene*/
        ECS_OBJ_STATUS_CONNECTED = 1,
    };
    META_READONLY_PROPERTY(uint8_t, ConnectionStatus)

    virtual CORE_NS::IEcs::Ptr GetEcs() const = 0;
    virtual void SetEntity(CORE_NS::IEcs::Ptr ecs, CORE_NS::Entity entity) = 0;
    virtual CORE_NS::Entity GetEntity() const = 0;
    virtual void BindObject(CORE_NS::IEcs::Ptr ecsInstance, CORE_NS::Entity entity) = 0;

    // Update property mappings according the targets. Refresh component bindings
    virtual void DefineTargetProperties(
        BASE_NS::unordered_map<BASE_NS::string_view, BASE_NS::vector<BASE_NS::string_view>> names) = 0;

    // Maintain a list of entities that refer to this entity, e.g. animation host
    virtual BASE_NS::vector<CORE_NS::Entity> GetAttachments() = 0;
    virtual void AddAttachment(CORE_NS::Entity entity) = 0;
    virtual void RemoveAttachment(CORE_NS::Entity entity) = 0;

    virtual void Activate() = 0;
    virtual void Deactivate() = 0;
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IEcsObject::Ptr);
META_TYPE(SCENE_NS::IEcsObject::WeakPtr);

#endif // SCENEPLUGIN_INTF_ECSOBJECT_H
