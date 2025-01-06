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

#ifndef SCENE_SRC_COMPONENT_FACTORIES_H
#define SCENE_SRC_COMPONENT_FACTORIES_H

#include <scene/ext/intf_internal_scene.h>
#include <text_3d/ecs/components/text_component.h>

#include "component/animation_component.h"
#include "component/camera_component.h"
#include "component/environment_component.h"
#include "component/layer_component.h"
#include "component/light_component.h"
#include "component/material_component.h"
#include "component/mesh_component.h"
#include "component/node_component.h"
#include "component/postprocess_component.h"
#include "component/text_component.h"
#include "component/transform_component.h"

SCENE_BEGIN_NAMESPACE()

struct ComponentFactory : public META_NS::IntroduceInterfaces<IComponentFactory> {
    ComponentFactory(META_NS::ClassInfo info) : info_(info) {}

    IComponent::Ptr CreateComponent(const IEcsObject::Ptr& eobj) override
    {
        auto p = META_NS::GetObjectRegistry().Create<IComponent>(info_);
        if (auto acc = interface_cast<IEcsObjectAccess>(p)) {
            if (!acc->SetEcsObject(eobj)) {
                return nullptr;
            }
        }
        return p;
    }

private:
    META_NS::ClassInfo info_;
};

inline void AddBuiltinComponentFactories(IInternalScene::Ptr s)
{
    s->RegisterComponent(
        CORE3D_NS::ITransformComponentManager::UID, CreateShared<ComponentFactory>(ClassId::TransformComponent));
    s->RegisterComponent(
        CORE3D_NS::ICameraComponentManager::UID, CreateShared<ComponentFactory>(ClassId::CameraComponent));
    s->RegisterComponent(
        CORE3D_NS::ILightComponentManager::UID, CreateShared<ComponentFactory>(ClassId::LightComponent));
    s->RegisterComponent(
        CORE3D_NS::IPostProcessComponentManager::UID, CreateShared<ComponentFactory>(ClassId::PostProcessComponent));
    s->RegisterComponent(
        CORE3D_NS::IAnimationComponentManager::UID, CreateShared<ComponentFactory>(ClassId::AnimationComponent));
    s->RegisterComponent(
        CORE3D_NS::IEnvironmentComponentManager::UID, CreateShared<ComponentFactory>(ClassId::EnvironmentComponent));
    s->RegisterComponent(
        CORE3D_NS::IMaterialComponentManager::UID, CreateShared<ComponentFactory>(ClassId::MaterialComponent));
    s->RegisterComponent(CORE3D_NS::IMeshComponentManager::UID, CreateShared<ComponentFactory>(ClassId::MeshComponent));
    s->RegisterComponent(
        CORE3D_NS::IRenderMeshComponentManager::UID, CreateShared<ComponentFactory>(ClassId::RenderMeshComponent));
    s->RegisterComponent(
        CORE3D_NS::ILayerComponentManager::UID, CreateShared<ComponentFactory>(ClassId::LayerComponent));
    s->RegisterComponent(CORE3D_NS::INodeComponentManager::UID, CreateShared<ComponentFactory>(ClassId::NodeComponent));
    s->RegisterComponent(TEXT3D_NS::ITextComponentManager::UID, CreateShared<ComponentFactory>(ClassId::TextComponent));
}

SCENE_END_NAMESPACE()

#endif