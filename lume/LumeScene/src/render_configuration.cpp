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

#include "render_configuration.h"

#include <scene/ext/intf_ecs_context.h>

#include <3d/ecs/components/render_configuration_component.h>
#include <core/ecs/entity.h>

#include "entity_converting_value.h"

SCENE_BEGIN_NAMESPACE()

CORE_NS::Entity RenderConfiguration::CreateEntity(const IInternalScene::Ptr& scene)
{
    CORE_NS::Entity entity {};
    if (!scene) {
        return entity;
    }
    // First try to get our scene's root node's entity
    if (auto rootNode = scene->GetRootNode()) {
        if (auto access = interface_cast<IEcsObjectAccess>(rootNode)) {
            if (auto ecso = access->GetEcsObject()) {
                entity = ecso->GetEntity();
            }
        }
    }
    // No root node so create a new entity
    if (!CORE_NS::EntityUtil::IsValid(entity)) {
        auto& ecs = scene->GetEcsContext();
        if (auto rconfig = ecs.FindComponent<CORE3D_NS::RenderConfigurationComponent>()) {
            if (auto necs = ecs.GetNativeEcs()) {
                entity = necs->GetEntityManager().Create();
                rconfig->Create(entity);
            }
        }
    }
    return entity;
}

bool RenderConfiguration::InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path)
{
    if (p->GetName() == "Environment") {
        auto ep = object_->CreateProperty(path).GetResult();
        auto i = interface_cast<META_NS::IStackProperty>(p);
        return ep && i &&
               i->PushValue(META_NS::IValue::Ptr(
                   new InterfacePtrEntityValue<IEnvironment>(ep, { object_->GetScene(), ClassId::Environment })));
    }
    return AttachEngineProperty(p, path);
}

BASE_NS::string RenderConfiguration::GetName() const
{
    return "RenderConfigurationComponent";
}

SCENE_END_NAMESPACE()
