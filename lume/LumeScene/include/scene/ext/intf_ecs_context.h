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

#ifndef SCENE_EXT_IECS_CONTEXT_H
#define SCENE_EXT_IECS_CONTEXT_H

#include <scene/base/namespace.h>
#include <scene/ext/intf_ecs_object.h>

#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class IEcsContext : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEcsContext, "5576c6f2-364b-497f-b342-4fffeb022e53")
public:
    virtual bool CreateUnnamedRootNode() = 0;
    virtual CORE_NS::IEcs::Ptr GetNativeEcs() const = 0;
    virtual CORE_NS::IComponentManager* FindComponent(BASE_NS::string_view name) const = 0;

    template<typename ComponentType>
    CORE_NS::IComponentManager* FindComponent() const
    {
        return FindComponent(CORE_NS::GetName<ComponentType>());
    }

    virtual void AddDefaultComponents(CORE_NS::Entity ent) const = 0;
    virtual BASE_NS::string GetPath(CORE_NS::Entity ent) const = 0;
    virtual IEcsObject::Ptr GetEcsObject(CORE_NS::Entity) = 0;
    virtual void RemoveEcsObject(const IEcsObject::ConstPtr&) = 0;
    virtual CORE_NS::Entity GetRootEntity() const = 0;
    virtual CORE_NS::Entity GetParent(CORE_NS::Entity ent) const = 0;
    virtual CORE_NS::EntityReference GetRenderHandleEntity(const RENDER_NS::RenderHandleReference& handle) = 0;

    virtual bool IsNodeEntity(CORE_NS::Entity ent) const = 0;
    virtual bool RemoveEntity(CORE_NS::Entity ent) = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IEcsContext)

#endif
