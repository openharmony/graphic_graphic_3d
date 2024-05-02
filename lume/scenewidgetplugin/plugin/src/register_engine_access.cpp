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
#include <scene_plugin/namespace.h>
#include <meta/base/namespace.h>
#include <meta/ext/any_builder.h>
#include <meta/ext/engine/internal_access.h>
#include <meta/interface/engine/intf_engine_data.h>
#include <meta/interface/intf_object_registry.h>
#include <scene_plugin/interface/intf_environment.h>

#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/post_process_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_state_component.h>
#include <3d/ecs/components/camera_component.h>

using CORE3D_NS::MaterialComponent;
using CORE3D_NS::EnvironmentComponent;
using CORE3D_NS::LightComponent;
using CORE3D_NS::MeshComponent;
using TonemapConfiguration = RENDER_NS::TonemapConfiguration;
using Submesh = MeshComponent::Submesh;
using BASE_NS::vector;
using CORE3D_NS::AnimationComponent;
using CORE3D_NS::AnimationStateComponent;
using CORE3D_NS::CameraComponent;

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(CORE3D_NS::EnvironmentComponent::Background);
DECLARE_PROPERTY_TYPE(CORE3D_NS::LightComponent::Type);
DECLARE_PROPERTY_TYPE(MaterialComponent::Type);
DECLARE_PROPERTY_TYPE(MaterialComponent::TextureInfo);
DECLARE_PROPERTY_TYPE(MaterialComponent::TextureTransform);
DECLARE_PROPERTY_TYPE(MaterialComponent::Shader);
DECLARE_PROPERTY_TYPE(TonemapConfiguration);
DECLARE_PROPERTY_TYPE(TonemapConfiguration::TonemapType);
DECLARE_PROPERTY_TYPE(vector<MeshComponent::Submesh>);
DECLARE_PROPERTY_TYPE(AnimationComponent::PlaybackState);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<AnimationStateComponent::TrackState>);
DECLARE_PROPERTY_TYPE(CameraComponent::RenderingPipeline);
DECLARE_PROPERTY_TYPE(CameraComponent::Culling);
DECLARE_PROPERTY_TYPE(CameraComponent::Projection);
CORE_END_NAMESPACE()

META_BEGIN_NAMESPACE()
META_TYPE(CORE3D_NS::EnvironmentComponent::Background);
META_TYPE(CORE3D_NS::LightComponent::Type);
META_TYPE(MaterialComponent::Type)
META_TYPE(MaterialComponent::Shader)
META_TYPE(MaterialComponent::TextureInfo)
META_TYPE(MaterialComponent::TextureTransform)
META_TYPE(TonemapConfiguration);
META_TYPE(TonemapConfiguration::TonemapType);
META_TYPE(Submesh)
META_TYPE(AnimationComponent::PlaybackState);
META_TYPE(AnimationStateComponent::TrackState);
META_TYPE(CameraComponent::RenderingPipeline);
META_TYPE(CameraComponent::Culling);
META_TYPE(CameraComponent::Projection);
META_END_NAMESPACE()

SCENE_BEGIN_NAMESPACE()

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
    META_NS::GetObjectRegistry().GetPropertyRegister().RegisterAny(CreateShared<META_NS::DefaultAnyBuilder<META_NS::Any<Prop>>>());
    RegisterEngineAccessImpl<Prop>();
}

template<typename Prop, typename AccessType>
void RegisterMapEngineAccessImpl()
{
    static_assert(CORE_NS::PropertySystem::is_defined<Prop>().value);

    auto& r = META_NS::GetObjectRegistry();
    r.GetEngineData().RegisterInternalValueAccess(META_NS::MetaType<Prop>::coreType,
        CreateShared < META_NS::EngineInternalValueAccessImpl<Prop, META_NS::Any<AccessType>, AccessType>> ());
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

    RegisterMapEngineAccessImpl<CameraComponent::Projection, uint8_t>();
    RegisterMapEngineAccessImpl<CameraComponent::Culling, uint8_t>();
    RegisterMapEngineAccessImpl<CameraComponent::RenderingPipeline, uint8_t>();
    RegisterMapEngineAccessImpl<AnimationComponent::PlaybackState, uint8_t>();
    RegisterMapEngineAccessImpl<EnvironmentComponent::Background, uint8_t>();
    RegisterMapEngineAccessImpl<LightComponent::Type, uint8_t>();
    RegisterMapEngineAccessImpl<MaterialComponent::Type, uint8_t>();
    RegisterMapEngineAccessImpl<TonemapConfiguration::TonemapType, uint32_t>();

    RegisterEngineAccessImplAndAny<MaterialComponent::TextureInfo>();
    RegisterEngineAccessImplAndAny<MaterialComponent::TextureTransform>();
    RegisterEngineAccessImplAndAny<MaterialComponent::Shader>();
    RegisterEngineAccessImpl<TonemapConfiguration>();
    RegisterEngineAccessImpl<BASE_NS::vector<Submesh>>();
    RegisterEngineAccessImpl<BASE_NS::vector<AnimationStateComponent::TrackState>>();
    RegisterEngineArrayAccessImpl<MaterialComponent::TextureInfo>();
}
void UnregisterEngineAccess()
{
    UnregisterEngineArrayAccessImpl<MaterialComponent::TextureInfo>();
    UnregisterEngineAccessImpl<BASE_NS::vector<AnimationStateComponent::TrackState>>();
    UnregisterEngineAccessImpl<BASE_NS::vector<Submesh>>();
    UnregisterEngineAccessImpl<TonemapConfiguration>();
    UnregisterEngineAccessImplAndAny<MaterialComponent::Shader>();
    UnregisterEngineAccessImplAndAny<MaterialComponent::TextureTransform>();
    UnregisterEngineAccessImplAndAny<MaterialComponent::TextureInfo>();

    auto& r = META_NS::GetObjectRegistry();
    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<TonemapConfiguration::TonemapType>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<MaterialComponent::Type>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<LightComponent::Type>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<EnvironmentComponent::Background>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<AnimationComponent::PlaybackState>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<CameraComponent::RenderingPipeline>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<CameraComponent::Culling>::coreType);
    r.GetEngineData().UnregisterInternalValueAccess(META_NS::MetaType<CameraComponent::Projection>::coreType);
}

SCENE_END_NAMESPACE()
