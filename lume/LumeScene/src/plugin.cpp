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

#include "asset/asset_object.h"
#include "bitmap.h"
#include "component/animation_component.h"
#include "component/camera_component.h"
#include "component/environment_component.h"
#include "component/generic_component.h"
#include "component/layer_component.h"
#include "component/light_component.h"
#include "component/material_component.h"
#include "component/mesh_component.h"
#include "component/node_component.h"
#include "component/postprocess_component.h"
#include "component/text_component.h"
#include "component/transform_component.h"
#include "core/ecs_object.h"
#include "ecs_animation.h"
#include "environment.h"
#include "mesh/material.h"
#include "mesh/mesh.h"
#include "mesh/mesh_creator.h"
#include "mesh/mesh_resource.h"
#include "mesh/shader.h"
#include "mesh/submesh.h"
#include "mesh/texture.h"
#include "node/camera_node.h"
#include "node/light_node.h"
#include "node/mesh_node.h"
#include "node/node.h"
#include "node/text_node.h"
#include "postprocess/bloom.h"
#include "postprocess/postprocess.h"
#include "postprocess/tonemap.h"
#include "render_configuration.h"
#include "scene.h"
#include "scene_manager.h"

static CORE_NS::IPluginRegister* gPluginRegistry { nullptr };

CORE_BEGIN_NAMESPACE()
IPluginRegister& GetPluginRegister()
{
    return *gPluginRegistry;
}
CORE_END_NAMESPACE()

SCENE_BEGIN_NAMESPACE()

void RegisterEngineAccess();
void UnregisterEngineAccess();

using namespace CORE_NS;

static PluginToken RegisterInterfaces(IPluginRegister& pluginRegistry)
{
    // Initializing dynamic plugin.
    // Plugin registry access via the provided registry instance which is saved here.
    gPluginRegistry = &pluginRegistry;

    RegisterEngineAccess();

    META_NS::RegisterObjectType<SceneManager>();
    META_NS::RegisterObjectType<SceneObject>();

    META_NS::RegisterObjectType<Node>();
    META_NS::RegisterObjectType<CameraNode>();
    META_NS::RegisterObjectType<LightNode>();
    META_NS::RegisterObjectType<MeshNode>();
    META_NS::RegisterObjectType<TextNode>();

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
    META_NS::RegisterObjectType<TextComponent>();

    META_NS::RegisterObjectType<EcsObject>();
    META_NS::RegisterObjectType<Bitmap>();
    META_NS::RegisterObjectType<Bloom>();
    META_NS::RegisterObjectType<Tonemap>();
    META_NS::RegisterObjectType<PostProcess>();

    META_NS::RegisterObjectType<Environment>();
    META_NS::RegisterObjectType<EcsAnimation>();
    META_NS::RegisterObjectType<RenderConfiguration>();
    META_NS::RegisterObjectType<Material>();
    META_NS::RegisterObjectType<Shader>();
    META_NS::RegisterObjectType<SubMesh>();
    META_NS::RegisterObjectType<Mesh>();
    META_NS::RegisterObjectType<MeshCreator>();
    META_NS::RegisterObjectType<MeshResource>();
    META_NS::RegisterObjectType<Texture>();

    META_NS::RegisterObjectType<AssetObject>();

    return {};
}
static void UnregisterInterfaces(PluginToken)
{
    META_NS::UnregisterObjectType<SceneManager>();
    META_NS::UnregisterObjectType<SceneObject>();

    META_NS::UnregisterObjectType<Node>();
    META_NS::UnregisterObjectType<CameraNode>();
    META_NS::UnregisterObjectType<LightNode>();
    META_NS::UnregisterObjectType<MeshNode>();
    META_NS::UnregisterObjectType<TextNode>();

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
    META_NS::UnregisterObjectType<TextComponent>();

    META_NS::UnregisterObjectType<EcsObject>();
    META_NS::UnregisterObjectType<Bitmap>();
    META_NS::UnregisterObjectType<Bloom>();
    META_NS::UnregisterObjectType<Tonemap>();
    META_NS::UnregisterObjectType<PostProcess>();

    META_NS::UnregisterObjectType<Environment>();
    META_NS::UnregisterObjectType<EcsAnimation>();
    META_NS::UnregisterObjectType<RenderConfiguration>();
    META_NS::UnregisterObjectType<Material>();
    META_NS::UnregisterObjectType<Shader>();
    META_NS::UnregisterObjectType<SubMesh>();
    META_NS::UnregisterObjectType<Mesh>();
    META_NS::UnregisterObjectType<MeshCreator>();
    META_NS::UnregisterObjectType<MeshResource>();
    META_NS::UnregisterObjectType<Texture>();

    META_NS::UnregisterObjectType<AssetObject>();

    UnregisterEngineAccess();

    // remove all weak refs still in the object registry referring to scene
    META_NS::GetObjectRegistry().Purge();
}
static const char* VersionString()
{
    return "2.0";
}

const BASE_NS::Uid plugin_deps[] { RENDER_NS::UID_RENDER_PLUGIN, CORE3D_NS::UID_3D_PLUGIN,
    META_NS::META_OBJECT_PLUGIN_UID };

SCENE_END_NAMESPACE()

extern "C" {
#if _MSC_VER
_declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
    CORE_NS::IPlugin gPluginData { { CORE_NS::IPlugin::UID }, "Scene",
        /** Version information of the plugin. */
        CORE_NS::Version { SCENE_NS::UID_SCENE_PLUGIN, SCENE_NS::VersionString }, SCENE_NS::RegisterInterfaces,
        SCENE_NS::UnregisterInterfaces, SCENE_NS::plugin_deps };
}
