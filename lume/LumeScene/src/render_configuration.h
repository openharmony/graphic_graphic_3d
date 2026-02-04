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

#ifndef SCENE_SRC_RENDER_CONFIGURATION_H
#define SCENE_SRC_RENDER_CONFIGURATION_H

#include <scene/ext/component.h>
#include <scene/ext/intf_create_entity.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/scene_property.h>
#include <scene/interface/intf_render_configuration.h>

#include <3d/render/intf_render_data_store_default_light.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class RenderConfiguration : public META_NS::IntroduceInterfaces<
    Component, IRenderConfiguration, ICreateEntity, META_NS::IPropertyOwner, META_NS::IMetadataOwner> {
    META_OBJECT(RenderConfiguration, ClassId::RenderConfiguration, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(
        IRenderConfiguration, IEnvironment::Ptr, Environment, "RenderConfigurationComponent.environment")
    SCENE_STATIC_PROPERTY_DATA(IRenderConfiguration, BASE_NS::string, RenderNodeGraphUri,
        "RenderConfigurationComponent.customRenderNodeGraphFile")
    SCENE_STATIC_PROPERTY_DATA(IRenderConfiguration, BASE_NS::string, PostRenderNodeGraphUri,
        "RenderConfigurationComponent.customPostSceneRenderNodeGraphFile")
    SCENE_STATIC_PROPERTY_DATA(
        IRenderConfiguration, SceneShadowType, ShadowType, "RenderConfigurationComponent.shadowType")
    SCENE_STATIC_PROPERTY_DATA(
        IRenderConfiguration, SceneShadowQuality, ShadowQuality, "RenderConfigurationComponent.shadowQuality")
    SCENE_STATIC_PROPERTY_DATA(
        IRenderConfiguration, SceneShadowSmoothness, ShadowSmoothness, "RenderConfigurationComponent.shadowSmoothness")
    META_STATIC_PROPERTY_DATA(IRenderConfiguration, BASE_NS::Math::UVec2, ShadowResolution)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(IEnvironment::Ptr, Environment)
    META_IMPLEMENT_PROPERTY(BASE_NS::string, RenderNodeGraphUri)
    META_IMPLEMENT_PROPERTY(BASE_NS::string, PostRenderNodeGraphUri)
    META_IMPLEMENT_PROPERTY(SceneShadowType, ShadowType)
    META_IMPLEMENT_PROPERTY(SceneShadowQuality, ShadowQuality)
    META_IMPLEMENT_PROPERTY(SceneShadowSmoothness, ShadowSmoothness)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::UVec2, ShadowResolution)

    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;
    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;
    BASE_NS::string GetName() const override;

protected: // IMetadataOwner
    void OnMetadataConstructed(const META_NS::StaticMetadata& m, CORE_NS::IInterface& i) override;

protected: // IPropertyOwner
    void OnPropertyChanged(const META_NS::IProperty& property) override;

private:
    BASE_NS::Math::UVec2 GetDefaultShadowResolution() const;
    void UpdateShadowResolution(IInternalScene& scene, bool isset, BASE_NS::Math::UVec2 value);
    BASE_NS::refcnt_ptr<CORE3D_NS::IRenderDataStoreDefaultLight> GetDefaultLightRenderDataStore(IInternalScene& scene);
};
SCENE_END_NAMESPACE()

#endif
