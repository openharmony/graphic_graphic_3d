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

#include <scene/base/namespace.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_environment.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_text.h>
#include <text_3d/ecs/components/text_component.h>

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_state_component.h>
#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/render_configuration_component.h>

#include <meta/base/namespace.h>
#include <meta/ext/any_builder.h>
#include <meta/ext/engine/internal_access.h>
#include <meta/interface/engine/intf_engine_data.h>
#include <meta/interface/intf_object_registry.h>

using CORE3D_NS::EnvironmentComponent;
using CORE3D_NS::LightComponent;
using CORE3D_NS::MaterialComponent;
using CORE3D_NS::MeshComponent;
using Submesh = MeshComponent::Submesh;
using BASE_NS::Format;
using BASE_NS::vector;
using CORE3D_NS::AnimationComponent;
using CORE3D_NS::AnimationStateComponent;
using CORE3D_NS::CameraComponent;
using CORE3D_NS::RenderConfigurationComponent;

using TEXT3D_NS::TextComponent;

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(EnvironmentComponent::Background);
DECLARE_PROPERTY_TYPE(LightComponent::Type);
DECLARE_PROPERTY_TYPE(MaterialComponent::Type);
DECLARE_PROPERTY_TYPE(MaterialComponent::RenderSort);
DECLARE_PROPERTY_TYPE(MaterialComponent::Shader);
DECLARE_PROPERTY_TYPE(MaterialComponent::TextureInfo);
DECLARE_PROPERTY_TYPE(MaterialComponent::TextureTransform);

DECLARE_PROPERTY_TYPE(vector<MeshComponent::Submesh>);
DECLARE_PROPERTY_TYPE(AnimationComponent::PlaybackState);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<AnimationStateComponent::TrackState>);
DECLARE_PROPERTY_TYPE(CameraComponent::RenderingPipeline);
DECLARE_PROPERTY_TYPE(CameraComponent::Culling);
DECLARE_PROPERTY_TYPE(CameraComponent::Projection);
DECLARE_PROPERTY_TYPE(CameraComponent::TargetUsage);
DECLARE_PROPERTY_TYPE(vector<CameraComponent::TargetUsage>);
DECLARE_PROPERTY_TYPE(Format);

DECLARE_PROPERTY_TYPE(RenderConfigurationComponent::SceneShadowType);
DECLARE_PROPERTY_TYPE(RenderConfigurationComponent::SceneShadowQuality);
DECLARE_PROPERTY_TYPE(RenderConfigurationComponent::SceneShadowSmoothness);

DECLARE_PROPERTY_TYPE(TextComponent::FontMethod);
CORE_END_NAMESPACE()

META_TYPE(CORE3D_NS::EnvironmentComponent::Background);
META_TYPE(CORE3D_NS::LightComponent::Type);
META_TYPE(CORE3D_NS::MaterialComponent::Type);
META_TYPE(CORE3D_NS::MaterialComponent::RenderSort);
META_TYPE(CORE3D_NS::MaterialComponent::Shader);
META_TYPE(CORE3D_NS::MaterialComponent::TextureInfo);
META_TYPE(CORE3D_NS::MaterialComponent::TextureTransform);

META_TYPE(RENDER_NS::RenderHandle)

META_TYPE(CORE3D_NS::MeshComponent::Submesh)
META_TYPE(CORE3D_NS::AnimationComponent::PlaybackState);
META_TYPE(CORE3D_NS::AnimationStateComponent::TrackState);
META_TYPE(CORE3D_NS::CameraComponent::RenderingPipeline);
META_TYPE(CORE3D_NS::CameraComponent::Culling);
META_TYPE(CORE3D_NS::CameraComponent::Projection);
META_TYPE(CORE3D_NS::CameraComponent::TargetUsage);

META_TYPE(CORE3D_NS::RenderConfigurationComponent::SceneShadowType);
META_TYPE(CORE3D_NS::RenderConfigurationComponent::SceneShadowQuality);
META_TYPE(CORE3D_NS::RenderConfigurationComponent::SceneShadowSmoothness);

META_TYPE(TEXT3D_NS::TextComponent::FontMethod);

SCENE_BEGIN_NAMESPACE()

namespace Internal {

template<typename Prop>
void RegisterEngineAccessImpl()
{
    static_assert(CORE_NS::PropertySystem::is_defined<Prop>().value);
    META_NS::GetObjectRegistry().GetEngineData().RegisterInternalValueAccess(
        META_NS::MetaType<Prop>::coreType, CreateShared<META_NS::EngineInternalValueAccess<Prop>>());
}

template<typename Prop>
void RegisterEngineArrayAccessImpl()
{
    static_assert(CORE_NS::PropertySystem::is_defined<Prop>().value);
    META_NS::GetObjectRegistry().GetEngineData().RegisterInternalValueAccess(
        META_NS::MetaType<Prop[]>::coreType, CreateShared<META_NS::EngineInternalArrayValueAccess<Prop>>());
}

template<typename Prop>
void RegisterEngineAccessImplAndAny()
{
    META_NS::GetObjectRegistry().GetPropertyRegister().RegisterAny(
        CreateShared<META_NS::DefaultAnyBuilder<META_NS::Any<Prop>>>());
    RegisterEngineAccessImpl<Prop>();
}

template<typename Prop, typename AccessType>
void RegisterMapEngineAccessImpl()
{
    static_assert(CORE_NS::PropertySystem::is_defined<Prop>().value);

    auto& r = META_NS::GetObjectRegistry();
    r.GetEngineData().RegisterInternalValueAccess(
        META_NS::MetaType<Prop>::coreType, CreateShared<META_NS::EngineInternalValueAccess<Prop, AccessType>>());
}

template<typename Prop>
void UnregisterEngineAccessImpl()
{
    META_NS::GetObjectRegistry().GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<Prop>::coreType);
}

template<typename Prop>
void UnregisterEngineArrayAccessImpl()
{
    META_NS::GetObjectRegistry().GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<Prop[]>::coreType);
}

template<typename Prop>
void UnregisterEngineAccessImplAndAny()
{
    UnregisterEngineAccessImpl<Prop>();
    META_NS::GetObjectRegistry().GetPropertyRegister().UnregisterAny(META_NS::Any<Prop>::StaticGetClassId());
}

void RegisterEngineAccess()
{
    static_assert(CORE_NS::PropertySystem::is_defined<EnvironmentComponent::Background>().value);

    RegisterMapEngineAccessImpl<CameraComponent::Projection, CameraProjection>();
    RegisterMapEngineAccessImpl<CameraComponent::Culling, CameraCulling>();
    RegisterMapEngineAccessImpl<CameraComponent::RenderingPipeline, CameraPipeline>();

    RegisterMapEngineAccessImpl<RenderConfigurationComponent::SceneShadowType, SceneShadowType>();
    RegisterMapEngineAccessImpl<RenderConfigurationComponent::SceneShadowQuality, SceneShadowQuality>();
    RegisterMapEngineAccessImpl<RenderConfigurationComponent::SceneShadowSmoothness, SceneShadowSmoothness>();

    RegisterMapEngineAccessImpl<EnvironmentComponent::Background, EnvBackgroundType>();
    RegisterMapEngineAccessImpl<LightComponent::Type, LightType>();
    RegisterMapEngineAccessImpl<MaterialComponent::Type, MaterialType>();

    RegisterMapEngineAccessImpl<TextComponent::FontMethod, SCENE_NS::FontMethod>();

    RegisterEngineAccessImplAndAny<MaterialComponent::RenderSort>();
    RegisterEngineAccessImplAndAny<MaterialComponent::Shader>();
    RegisterEngineAccessImplAndAny<MaterialComponent::TextureInfo>();
    RegisterEngineAccessImplAndAny<MaterialComponent::TextureTransform>();

    RegisterEngineAccessImpl<RENDER_NS::RenderHandle>();

    RegisterEngineAccessImpl<BASE_NS::vector<Submesh>>();
    RegisterEngineAccessImpl<BASE_NS::vector<AnimationStateComponent::TrackState>>();
    RegisterEngineAccessImpl<BASE_NS::vector<CameraComponent::TargetUsage>>();
    RegisterEngineAccessImpl<AnimationComponent::PlaybackState>();
    RegisterEngineArrayAccessImpl<MaterialComponent::TextureInfo>();
}
void UnregisterEngineAccess()
{
    UnregisterEngineArrayAccessImpl<MaterialComponent::TextureInfo>();
    UnregisterEngineAccessImpl<BASE_NS::vector<CameraComponent::TargetUsage>>();
    UnregisterEngineAccessImpl<BASE_NS::vector<AnimationStateComponent::TrackState>>();
    UnregisterEngineAccessImpl<BASE_NS::vector<Submesh>>();

    UnregisterEngineAccessImpl<RENDER_NS::RenderHandle>();

    UnregisterEngineAccessImplAndAny<MaterialComponent::RenderSort>();
    UnregisterEngineAccessImplAndAny<MaterialComponent::Shader>();
    UnregisterEngineAccessImplAndAny<MaterialComponent::TextureInfo>();
    UnregisterEngineAccessImplAndAny<MaterialComponent::TextureTransform>();

    UnregisterEngineAccessImplAndAny<RenderConfigurationComponent::SceneShadowType>();
    UnregisterEngineAccessImplAndAny<RenderConfigurationComponent::SceneShadowQuality>();
    UnregisterEngineAccessImplAndAny<RenderConfigurationComponent::SceneShadowSmoothness>();

    auto& r = META_NS::GetObjectRegistry();
    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<MaterialComponent::Type>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<LightComponent::Type>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<EnvironmentComponent::Background>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<AnimationComponent::PlaybackState>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<CameraComponent::RenderingPipeline>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<CameraComponent::Culling>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<CameraComponent::Projection>::coreType);

    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<TextComponent::FontMethod>::coreType);
}

} // namespace Internal
SCENE_END_NAMESPACE()
