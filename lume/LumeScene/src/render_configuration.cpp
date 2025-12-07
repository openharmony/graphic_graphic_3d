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

#include <core/property/property_types.h>

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(BASE_NS::string);
CORE_END_NAMESPACE()

#include <scene/ext/intf_ecs_context.h>

#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <core/ecs/entity.h>
#include <core/property/property_handle_util.h>
#include <render/datastore/intf_render_data_store_manager.h>

#include <meta/api/metadata_util.h>

#include "entity_converting_value.h"
#include "render_configuration.h"

SCENE_BEGIN_NAMESPACE()

static constexpr auto SHADOW_RESOLUTION_PROP_NAME = "ShadowResolution";

static constexpr auto SHADOW_QUALITY_PROP_NAME = "ShadowQuality";

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
                   new InterfacePtrEntityValue<IEnvironment>(ep, { GetInternalScene(), ClassId::Environment })));
    }
    return AttachEngineProperty(p, path);
}

BASE_NS::string RenderConfiguration::GetName() const
{
    return "RenderConfigurationComponent";
}

BASE_NS::Math::UVec2 RenderConfiguration::GetDefaultShadowResolution() const
{
    constexpr CORE3D_NS::IRenderDataStoreDefaultLight::ShadowQualityResolutions defaults;

    BASE_NS::Math::UVec2 resolution = defaults.normal;
    auto quality = GetProperty<SceneShadowQuality>(SHADOW_QUALITY_PROP_NAME, META_NS::MetadataQuery::EXISTING);
    if (quality) {
        switch (META_NS::GetValue(quality)) {
            case SceneShadowQuality::LOW:
                resolution = defaults.low;
                break;
            case SceneShadowQuality::NORMAL:
                resolution = defaults.normal;
                break;
            case SceneShadowQuality::HIGH:
                resolution = defaults.high;
                break;
            case SceneShadowQuality::ULTRA:
                resolution = defaults.ultra;
                break;
        }
    }
    return resolution;
}

void RenderConfiguration::OnMetadataConstructed(const META_NS::StaticMetadata& m, CORE_NS::IInterface& i)
{
    auto p = interface_cast<META_NS::IProperty>(&i);
    if (p && p->GetName() == SHADOW_RESOLUTION_PROP_NAME) {
        auto typed = META_NS::TypedPropertyLock<BASE_NS::Math::UVec2>(p);
        if (typed) {
            typed->SetDefaultValue(GetDefaultShadowResolution());
        }
    }
}

void RenderConfiguration::OnPropertyChanged(const META_NS::IProperty& property)
{
    const auto name = property.GetName();
    if (name == SHADOW_RESOLUTION_PROP_NAME) {
        bool isset = META_NS::IsValueSet(property);
        auto value = META_NS::GetValue<BASE_NS::Math::UVec2>(property.GetValue());
        if (isset) {
            value = META_NS::GetValue<BASE_NS::Math::UVec2>(property.GetValue());
        }
        isset &= value.x > 0 && value.y > 0; // Require size to be larger than 0x0 even if set
        auto scene = GetInternalScene();
        if (scene) {
            scene->RunDirectlyOrInTask( // need notice
                [this, scene, isset, value = BASE_NS::move(value)]() { UpdateShadowResolution(*scene, isset, value); });
        }
    } else if (name == SHADOW_QUALITY_PROP_NAME) {
        auto resolution =
            GetProperty<BASE_NS::Math::UVec2>(SHADOW_RESOLUTION_PROP_NAME, META_NS::MetadataQuery::EXISTING);
        if (resolution) {
            resolution->SetDefaultValue(GetDefaultShadowResolution());
        }
    }
}

BASE_NS::refcnt_ptr<CORE3D_NS::IRenderDataStoreDefaultLight> RenderConfiguration::GetDefaultLightRenderDataStore(
    IInternalScene& scene)
{
    const auto ecs = scene.GetEcsContext().GetNativeEcs();
    if (ecs) {
        const auto preprocessor = CORE_NS::GetSystem<CORE3D_NS::IRenderPreprocessorSystem>(*ecs);
        if (preprocessor) {
            const auto dsLightName =
                CORE_NS::GetPropertyValue<BASE_NS::string>(preprocessor->GetProperties(), "dataStoreLight");
            const auto& manager = scene.GetRenderContext().GetRenderDataStoreManager();
            return manager.GetRenderDataStore(dsLightName);
        }
    }
    return {};
}

void RenderConfiguration::UpdateShadowResolution(IInternalScene& scene, bool isset, BASE_NS::Math::UVec2 value)
{
    if (const auto store = GetDefaultLightRenderDataStore(scene)) {
        CORE3D_NS::IRenderDataStoreDefaultLight::ShadowQualityResolutions resolutions; // defaults
        if (isset) {
            // If an override value is set, place the value as the resolution for every shadow
            // quality, effectively setting the shadow map resolution to the desired value
            // regardless of the shadow quality.
            resolutions.low = value;
            resolutions.normal = value;
            resolutions.high = value;
            resolutions.ultra = value;
        }
        store->SetShadowQualityResolutions(resolutions, {});
    }
}

SCENE_END_NAMESPACE()
