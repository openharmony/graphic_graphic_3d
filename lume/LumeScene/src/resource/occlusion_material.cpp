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
#include "occlusion_material.h"

#include <scene/ext/util.h>

#include <3d/ecs/components/name_component.h>

#include <meta/api/engine/util.h>
#include <meta/api/make_callback.h>
#include <meta/interface/engine/intf_engine_value_manager.h>

SCENE_BEGIN_NAMESPACE()

CORE_NS::Entity OcclusionMaterial::CreateEntity(const IInternalScene::Ptr& scene)
{
    CORE_NS::Entity entity {};
    if (scene) {
        const auto ecs = scene->GetEcsContext().GetNativeEcs();
        if (ecs) {
            const auto mcm = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(*ecs);
            const auto ncm = CORE_NS::GetManager<CORE3D_NS::INameComponentManager>(*ecs);
            if (mcm && ncm) {
                entity = ecs->GetEntityManager().Create();
                mcm->Create(entity);
                ncm->Create(entity);
                if (auto wh = mcm->Write(entity)) {
                    // Occlusion material enforces the material type
                    wh->type = CORE3D_NS::MaterialComponent::Type::OCCLUSION;
                    // Object with occlusion material should not cast shadows
                    wh->materialLightingFlags &= ~CORE3D_NS::MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT;
                }
            }
        }
    }
    return entity;
}

bool OcclusionMaterialTemplateAccess::SetValuesFromTemplate(
    const CORE_NS::IResource::ConstPtr& templ, const CORE_NS::IResource::Ptr& resource) const
{
    const auto ores = interface_cast<META_NS::IObjectResource>(templ);
    const auto mat = interface_cast<IMaterial>(resource);
    if (ores && mat && templ->GetResourceType() != templateType_.ToUid()) {
        const auto id = interface_cast<META_NS::IDerivedFromTemplate>(resource);
        if (id) {
            id->SetTemplateId(templ->GetResourceId());
        }
        return true;
    }
    CORE_LOG_W("Invalid resource type");
    return false;
}

SCENE_END_NAMESPACE()
