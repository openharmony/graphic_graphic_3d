/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <scene_metadata_importer/plugin.h>

#include <3d/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>
#include <render/implementation_uids.h>

#include <meta/base/plugin.h>

#include "light_probe_group_ser.h"
#include "metadata_importer.h"
#include "node_instantiator.h"
#include "scene_exporter.h"
#include "scene_importer.h"
#include "scene_ser.h"
#include "types/animation_type.h"
#include "types/environment_type.h"
#include "types/gltf_scene_type.h"
#include "types/image_type.h"
#include "types/material_type.h"
#include "types/node_template_type.h"
#include "types/postprocess_type.h"
#include "types/scene_type.h"
#include "types/shader_type.h"

static CORE_NS::IPluginRegister* gPluginRegistry{nullptr};

CORE_BEGIN_NAMESPACE()
IPluginRegister& GetPluginRegister()
{
    return *gPluginRegistry;
}
CORE_END_NAMESPACE()

SCENE_MIMP_BEGIN_NAMESPACE()

namespace Internal {
void RegisterSerializers();
void UnRegisterSerializers();
}  // namespace Internal

using namespace CORE_NS;

static PluginToken RegisterInterfaces(IPluginRegister& pluginRegistry)
{
    using namespace SCENE_NS;
    // Initializing dynamic plugin.
    // Plugin registry access via the provided registry instance which is saved here.
    gPluginRegistry = &pluginRegistry;

    Internal::RegisterSerializers();

    META_NS::RegisterObjectType<MetadataImporter>();
    META_NS::RegisterObjectType<AnimationResourceType>();
    META_NS::RegisterObjectType<SceneResourceType>();
    META_NS::RegisterObjectType<GltfSceneResourceType>();
    META_NS::RegisterObjectType<ImageResourceType>();
    META_NS::RegisterObjectType<ShaderResourceType>();
    META_NS::RegisterObjectType<EnvironmentResourceType>();
    META_NS::RegisterObjectType<MaterialResourceType>();
    META_NS::RegisterObjectType<OcclusionMaterialResourceType>();
    META_NS::RegisterObjectType<PostProcessResourceType>();
    META_NS::RegisterObjectType<SceneSer>();
    META_NS::RegisterObjectType<ComponentSer>();
    META_NS::RegisterObjectType<SceneNodeSer>();
    META_NS::RegisterObjectType<SceneExternalNodeSer>();
    META_NS::RegisterObjectType<ExternalAttachment>();
    META_NS::RegisterObjectType<SceneExporter>();
    META_NS::RegisterObjectType<SceneImporter>();
    META_NS::RegisterObjectType<NodeInstantiator>();
    META_NS::RegisterObjectType<ExportedNode>();
    META_NS::RegisterObjectType<NodeTemplateType>();
    META_NS::RegisterObjectType<LightProbeGroupSer>();

    return {};
}
static void UnregisterInterfaces(PluginToken)
{
    using namespace SCENE_NS;
    META_NS::UnregisterObjectType<AnimationResourceType>();
    META_NS::UnregisterObjectType<SceneResourceType>();
    META_NS::UnregisterObjectType<GltfSceneResourceType>();
    META_NS::UnregisterObjectType<ImageResourceType>();
    META_NS::UnregisterObjectType<ShaderResourceType>();
    META_NS::UnregisterObjectType<EnvironmentResourceType>();
    META_NS::UnregisterObjectType<MaterialResourceType>();
    META_NS::UnregisterObjectType<OcclusionMaterialResourceType>();
    META_NS::UnregisterObjectType<PostProcessResourceType>();
    META_NS::UnregisterObjectType<SceneSer>();
    META_NS::UnregisterObjectType<ComponentSer>();
    META_NS::UnregisterObjectType<SceneNodeSer>();
    META_NS::UnregisterObjectType<SceneExternalNodeSer>();
    META_NS::UnregisterObjectType<ExternalAttachment>();
    META_NS::UnregisterObjectType<SceneExporter>();
    META_NS::UnregisterObjectType<SceneImporter>();
    META_NS::UnregisterObjectType<NodeInstantiator>();
    META_NS::UnregisterObjectType<ExportedNode>();
    META_NS::UnregisterObjectType<MetadataImporter>();
    META_NS::UnregisterObjectType<NodeTemplateType>();
    META_NS::UnregisterObjectType<LightProbeGroupSer>();

    Internal::UnRegisterSerializers();
}
static const char* VersionString()
{
    return "1.0";
}

static constexpr BASE_NS::Uid PLUGIN_DEPENDENCIES[]{
    RENDER_NS::UID_RENDER_PLUGIN, CORE3D_NS::UID_3D_PLUGIN, META_NS::META_OBJECT_PLUGIN_UID};

SCENE_MIMP_END_NAMESPACE()

extern "C" {
#if _MSC_VER
_declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
    CORE_NS::IPlugin gPluginData{{CORE_NS::IPlugin::UID},
        "SceneMetadataImporter",
        /** Version information of the plugin. */
        CORE_NS::Version{SCENE_MIMP_NS::UID_SCENE_METADATA_IMPORTER_PLUGIN, SCENE_MIMP_NS::VersionString},
        SCENE_MIMP_NS::RegisterInterfaces,
        SCENE_MIMP_NS::UnregisterInterfaces,
        SCENE_MIMP_NS::PLUGIN_DEPENDENCIES};
}
