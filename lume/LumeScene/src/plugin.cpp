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

// clang-format off
#include <meta/interface/object_macros.h>
#include <meta/interface/intf_object_registry.h>
// clang-format on

#include <scene/base/namespace.h>

#include <3d/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>
#include <render/implementation_uids.h>

#include <meta/base/plugin.h>

#include "application_context.h"
#include "asset/asset_object.h"
#include "component/animation_component.h"
#include "component/camera_component.h"
#include "component/camera_effect_component.h"
#include "component/environment_component.h"
#include "component/generic_component.h"
#include "component/layer_component.h"
#include "component/light_component.h"
#include "component/material_component.h"
#include "component/mesh_component.h"
#include "component/node_component.h"
#include "component/postprocess_component.h"
#include "component/transform_component.h"
#include "containable_object.h"
#include "core/ecs_object.h"
#include "ecs_component/entity_owner_component_info.h"
#include "ecs_component/resource_component_info.h"
#include "mesh/mesh.h"
#include "mesh/mesh_creator.h"
#include "mesh/sampler.h"
#include "mesh/submesh.h"
#include "mesh/texture.h"
#include "node/camera_node.h"
#include "node/light_node.h"
#include "node/light_probe_group_node.h"
#include "node/mesh_node.h"
#include "node/node.h"
#include "postprocess/bloom.h"
#include "postprocess/blur.h"
#include "postprocess/color_adjustments.h"
#include "postprocess/color_conversion.h"
#include "postprocess/color_fringe.h"
#include "postprocess/dof.h"
#include "postprocess/fxaa.h"
#include "postprocess/lens_flare.h"
#include "postprocess/motion_blur.h"
#include "postprocess/postprocess.h"
#include "postprocess/taa.h"
#include "postprocess/tonemap.h"
#include "postprocess/upscale.h"
#include "postprocess/vignette.h"
#include "postprocess/white_balance.h"
#include "render_configuration.h"
#include "render_context.h"
#include "resource/ecs_animation.h"
#include "resource/effect.h"
#include "resource/environment.h"
#include "resource/image.h"
#include "resource/material.h"
#include "resource/node_template.h"
#include "resource/occlusion_material.h"
#include "resource/render_resource_manager.h"
#include "resource/shader.h"
#include "scene.h"
#include "scene_manager.h"
#include "serialization/external_node.h"
#include "template/animation_template.h"
#include "template/environment_template.h"
#include "template/image_template.h"
#include "template/material_template.h"
#include "template/mesh_template.h"
#include "template/postprocess_template.h"
#include "util.h"

static CORE_NS::IPluginRegister* gPluginRegistry{nullptr};

CORE_BEGIN_NAMESPACE()
IPluginRegister& GetPluginRegister()
{
    return *gPluginRegistry;
}
CORE_END_NAMESPACE()

SCENE_BEGIN_NAMESPACE()

namespace Internal {
void RegisterEngineAccess();
void UnregisterEngineAccess();
void RegisterPostProcessEngineAccess();
void UnregisterPostProcessEngineAccess();
void RegisterAnys();
void UnRegisterAnys();

void RegisterComponentTypes()
{
    META_NS::RegisterObjectType<GenericComponent>();
    META_NS::RegisterObjectType<CameraComponent>();
    META_NS::RegisterObjectType<TransformComponent>();
    META_NS::RegisterObjectType<PostProcessComponent>();
    META_NS::RegisterObjectType<AnimationComponent>();
    META_NS::RegisterObjectType<EnvironmentComponent>();
    META_NS::RegisterObjectType<LayerComponent>();
    META_NS::RegisterObjectType<LightComponent>();
    META_NS::RegisterObjectType<MaterialComponent>();
    META_NS::RegisterObjectType<MeshComponent>();
    META_NS::RegisterObjectType<RenderMeshComponent>();
    META_NS::RegisterObjectType<NodeComponent>();
    META_NS::RegisterObjectType<MorphComponent>();
}

void UnregisterComponentTypes()
{
    META_NS::UnregisterObjectType<GenericComponent>();
    META_NS::UnregisterObjectType<CameraComponent>();
    META_NS::UnregisterObjectType<TransformComponent>();
    META_NS::UnregisterObjectType<PostProcessComponent>();
    META_NS::UnregisterObjectType<AnimationComponent>();
    META_NS::UnregisterObjectType<EnvironmentComponent>();
    META_NS::UnregisterObjectType<LayerComponent>();
    META_NS::UnregisterObjectType<LightComponent>();
    META_NS::UnregisterObjectType<MaterialComponent>();
    META_NS::UnregisterObjectType<MeshComponent>();
    META_NS::UnregisterObjectType<RenderMeshComponent>();
    META_NS::UnregisterObjectType<NodeComponent>();
    META_NS::UnregisterObjectType<MorphComponent>();
}

void RegisterTemplateTypes()
{
    META_NS::RegisterObjectType<AnimationTemplate>();
    META_NS::RegisterObjectType<MaterialTemplate>();
    META_NS::RegisterObjectType<MeshTemplate>();
    META_NS::RegisterObjectType<EnvironmentTemplate>();
    META_NS::RegisterObjectType<ImageTemplate>();
    META_NS::RegisterObjectType<PostProcessTemplate>();

    META_NS::RegisterObjectType<EnvironmentTemplateAccess>();
    META_NS::RegisterObjectType<MaterialTemplateAccess>();
    META_NS::RegisterObjectType<PostProcessTemplateAccess>();
    META_NS::RegisterObjectType<OcclusionMaterialTemplateAccess>();
    META_NS::RegisterObjectType<NodeTemplate>();
    META_NS::RegisterObjectType<NodeTemplateContext>();
}

void UnregisterTemplateTypes()
{
    META_NS::UnregisterObjectType<AnimationTemplate>();
    META_NS::UnregisterObjectType<MaterialTemplate>();
    META_NS::UnregisterObjectType<MeshTemplate>();
    META_NS::UnregisterObjectType<EnvironmentTemplate>();
    META_NS::UnregisterObjectType<ImageTemplate>();
    META_NS::UnregisterObjectType<PostProcessTemplate>();

    META_NS::UnregisterObjectType<EnvironmentTemplateAccess>();
    META_NS::UnregisterObjectType<MaterialTemplateAccess>();
    META_NS::UnregisterObjectType<PostProcessTemplateAccess>();
    META_NS::UnregisterObjectType<OcclusionMaterialTemplateAccess>();
    META_NS::UnregisterObjectType<NodeTemplate>();
    META_NS::UnregisterObjectType<NodeTemplateContext>();
}

void RegisterPostProcessTypes()
{
    META_NS::RegisterObjectType<Bloom>();
    META_NS::RegisterObjectType<ColorAdjustments>();
    META_NS::RegisterObjectType<Tonemap>();
    META_NS::RegisterObjectType<Blur>();
    META_NS::RegisterObjectType<MotionBlur>();
    META_NS::RegisterObjectType<ColorConversion>();
    META_NS::RegisterObjectType<ColorFringe>();
    META_NS::RegisterObjectType<DepthOfField>();
    META_NS::RegisterObjectType<Fxaa>();
    META_NS::RegisterObjectType<Taa>();
    META_NS::RegisterObjectType<Vignette>();
    META_NS::RegisterObjectType<LensFlare>();
    META_NS::RegisterObjectType<Upscale>();
    META_NS::RegisterObjectType<WhiteBalance>();
    META_NS::RegisterObjectType<PostProcess>();
}

void UnregisterPostProcessTypes()
{
    META_NS::UnregisterObjectType<Bloom>();
    META_NS::UnregisterObjectType<ColorAdjustments>();
    META_NS::UnregisterObjectType<Tonemap>();
    META_NS::UnregisterObjectType<Blur>();
    META_NS::UnregisterObjectType<MotionBlur>();
    META_NS::UnregisterObjectType<ColorConversion>();
    META_NS::UnregisterObjectType<ColorFringe>();
    META_NS::UnregisterObjectType<DepthOfField>();
    META_NS::UnregisterObjectType<Fxaa>();
    META_NS::UnregisterObjectType<Taa>();
    META_NS::UnregisterObjectType<Vignette>();
    META_NS::UnregisterObjectType<LensFlare>();
    META_NS::UnregisterObjectType<Upscale>();
    META_NS::UnregisterObjectType<WhiteBalance>();
    META_NS::UnregisterObjectType<PostProcess>();
}

void RegisterAssetTypes()
{
    META_NS::RegisterObjectType<Environment>();
    META_NS::RegisterObjectType<EcsAnimation>();
    META_NS::RegisterObjectType<RenderConfiguration>();
    META_NS::RegisterObjectType<Material>();
    META_NS::RegisterObjectType<Shader>();
    META_NS::RegisterObjectType<SubMesh>();
    META_NS::RegisterObjectType<Mesh>();
    META_NS::RegisterObjectType<MeshCreator>();
    META_NS::RegisterObjectType<Texture>();
    META_NS::RegisterObjectType<Sampler>();
    META_NS::RegisterObjectType<OcclusionMaterial>();
    META_NS::RegisterObjectType<AssetObject>();
}

void UnregisterAssetTypes()
{
    META_NS::UnregisterObjectType<Environment>();
    META_NS::UnregisterObjectType<EcsAnimation>();
    META_NS::UnregisterObjectType<RenderConfiguration>();
    META_NS::UnregisterObjectType<Material>();
    META_NS::UnregisterObjectType<Shader>();
    META_NS::UnregisterObjectType<SubMesh>();
    META_NS::UnregisterObjectType<Mesh>();
    META_NS::UnregisterObjectType<MeshCreator>();
    META_NS::UnregisterObjectType<Texture>();
    META_NS::UnregisterObjectType<Sampler>();
    META_NS::UnregisterObjectType<OcclusionMaterial>();
    META_NS::UnregisterObjectType<AssetObject>();
}

}  // namespace Internal

using namespace CORE_NS;

static PluginToken RegisterInterfaces(IPluginRegister& pluginRegistry)
{
    // Initializing dynamic plugin.
    // Plugin registry access via the provided registry instance which is saved here.
    gPluginRegistry = &pluginRegistry;

    pluginRegistry.RegisterTypeInfo(ENTITY_OWNER_COMPONENT_TYPE_INFO);
    pluginRegistry.RegisterTypeInfo(RESOURCE_COMPONENT_TYPE_INFO);

    Internal::RegisterAnys();
    Internal::RegisterEngineAccess();
    Internal::RegisterPostProcessEngineAccess();

    META_NS::RegisterObjectType<SceneManager>();
    META_NS::RegisterObjectType<SceneObject>();

    META_NS::RegisterObjectType<Node>();
    META_NS::RegisterObjectType<CameraNode>();
    META_NS::RegisterObjectType<LightNode>();
    META_NS::RegisterObjectType<MeshNode>();
    META_NS::RegisterObjectType<LightProbeGroupNode>();

    Internal::RegisterComponentTypes();

    META_NS::RegisterObjectType<EcsObject>();
    META_NS::RegisterObjectType<Image>();
    META_NS::RegisterObjectType<ExternalImage>();

    Internal::RegisterPostProcessTypes();
    Internal::RegisterAssetTypes();

    META_NS::RegisterObjectType<RenderResourceManager>();
    META_NS::RegisterObjectType<ExternalNode>();

    Internal::RegisterTemplateTypes();

    META_NS::RegisterObjectType<ApplicationContext>();
    META_NS::RegisterObjectType<RenderContext>();

    META_NS::RegisterObjectType<ContainableObject>();
    META_NS::RegisterObjectType<CameraEffectComponent>();
    META_NS::RegisterObjectType<Effect>();

    return {};
}
static void UnregisterInterfaces(PluginToken)
{
    // Tear down the deferred-load thread pool while LumeEngine is still
    // loaded — the pool's worker join uses ITaskQueueFactory code paths
    // owned by that plugin.
    RenderResourceManager::Shutdown();

    META_NS::UnregisterObjectType<SceneManager>();
    META_NS::UnregisterObjectType<SceneObject>();

    META_NS::UnregisterObjectType<Node>();
    META_NS::UnregisterObjectType<CameraNode>();
    META_NS::UnregisterObjectType<LightNode>();
    META_NS::UnregisterObjectType<MeshNode>();
    META_NS::UnregisterObjectType<LightProbeGroupNode>();

    Internal::UnregisterComponentTypes();

    META_NS::UnregisterObjectType<EcsObject>();
    META_NS::UnregisterObjectType<ExternalImage>();
    META_NS::UnregisterObjectType<Image>();

    Internal::UnregisterPostProcessTypes();
    Internal::UnregisterAssetTypes();

    META_NS::UnregisterObjectType<RenderResourceManager>();
    META_NS::UnregisterObjectType<ExternalNode>();

    Internal::UnregisterTemplateTypes();

    META_NS::UnregisterObjectType<ApplicationContext>();
    META_NS::UnregisterObjectType<RenderContext>();

    META_NS::UnregisterObjectType<ContainableObject>();
    META_NS::UnregisterObjectType<CameraEffectComponent>();
    META_NS::UnregisterObjectType<Effect>();

    Internal::UnregisterPostProcessEngineAccess();
    Internal::UnregisterEngineAccess();
    Internal::UnRegisterAnys();

    // remove all weak refs still in the object registry referring to scene
    META_NS::GetObjectRegistry().Purge();

    GetPluginRegister().UnregisterTypeInfo(RESOURCE_COMPONENT_TYPE_INFO);
    GetPluginRegister().UnregisterTypeInfo(ENTITY_OWNER_COMPONENT_TYPE_INFO);
}
static const char* VersionString()
{
    return "2.0";
}

static constexpr BASE_NS::Uid UID_SCENE_METADATA_IMPORTER_PLUGIN{"a959e575-0394-4189-95f9-e0f34b747cca"};
static constexpr BASE_NS::Uid PLUGIN_DEPENDENCIES[]{RENDER_NS::UID_RENDER_PLUGIN,
    CORE3D_NS::UID_3D_PLUGIN,
    META_NS::META_OBJECT_PLUGIN_UID,
    UID_SCENE_METADATA_IMPORTER_PLUGIN};

SCENE_END_NAMESPACE()

extern "C" {
#if _MSC_VER
_declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
    CORE_NS::IPlugin gPluginData{{CORE_NS::IPlugin::UID},
        "Scene",
        /** Version information of the plugin. */
        CORE_NS::Version{SCENE_NS::UID_SCENE_PLUGIN, SCENE_NS::VersionString},
        SCENE_NS::RegisterInterfaces,
        SCENE_NS::UnregisterInterfaces,
        SCENE_NS::PLUGIN_DEPENDENCIES};
}
