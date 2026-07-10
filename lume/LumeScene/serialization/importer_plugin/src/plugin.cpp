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
#include <scene_importer/plugin.h>

#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>

#include <meta/base/plugin.h>

#include "diagnostics.h"
#include "importer.h"
#include "loaders/animation_type.h"
#include "loaders/environment_type.h"
#include "loaders/gltf_scene_type.h"
#include "loaders/image_type.h"
#include "loaders/material_type.h"
#include "loaders/mesh_type.h"
#include "loaders/node_template_type.h"
#include "loaders/postprocess_type.h"
#include "loaders/scene_type.h"
#include "loaders/shader_type.h"
#include "loaders/template_resource_types.h"
#include "objects/index.h"
#include "objects/node_template_internal.h"
#include "scene_file_loader.h"

static CORE_NS::IPluginRegister* gPluginRegistry{nullptr};

CORE_BEGIN_NAMESPACE()
IPluginRegister& GetPluginRegister()
{
    return *gPluginRegistry;
}
CORE_END_NAMESPACE()

SCENE_IMP_BEGIN_NAMESPACE()

using namespace CORE_NS;

// Single source of truth for the object types this plugin registers.
// RegisterAll / UnregisterAll both iterate the list left-to-right.
template<typename... Types>
struct TypeList {
    static void RegisterAll()
    {
        (META_NS::RegisterObjectType<Types>(), ...);
    }
    static void UnregisterAll()
    {
        (META_NS::UnregisterObjectType<Types>(), ...);
    }
};

using PluginTypes = TypeList<Diagnostics, Importer, SceneFileLoader, NodeTemplatePayload, NodeTemplateInstantiator,
    GltfAnimationResourceType, MetaAnimationResourceType, EnvironmentResourceType, GltfSceneResourceType,
    ImageResourceType, MaterialResourceType, MeshResourceType, MeshResourceTemplateType, OcclusionMaterialResourceType,
    NodeTemplateResourceType, PostProcessResourceType, SceneResourceType, ShaderResourceType,
    MaterialTemplateResourceType, OcclusionMaterialTemplateResourceType, PostProcessTemplateResourceType,
    AnimationTemplateResourceType, EnvironmentTemplateResourceType>;

static PluginToken RegisterInterfaces(IPluginRegister& pluginRegistry)
{
    // Initializing dynamic plugin.
    // Plugin registry access via the provided registry instance which is saved here.
    gPluginRegistry = &pluginRegistry;
    PluginTypes::RegisterAll();
    return {};
}
static void UnregisterInterfaces(PluginToken)
{
    PluginTypes::UnregisterAll();
}
static const char* VersionString()
{
    return "1.0";
}

static constexpr BASE_NS::Uid PLUGIN_DEPENDENCIES[]{SCENE_NS::UID_SCENE_PLUGIN};

SCENE_IMP_END_NAMESPACE()

extern "C" {
#if _MSC_VER
_declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
    CORE_NS::IPlugin gPluginData{{CORE_NS::IPlugin::UID},
        "SceneImporter",
        /** Version information of the plugin. */
        CORE_NS::Version{SCENE_IMP_NS::UID_SCENE_IMPORTER_PLUGIN, SCENE_IMP_NS::VersionString},
        SCENE_IMP_NS::RegisterInterfaces,
        SCENE_IMP_NS::UnregisterInterfaces,
        SCENE_IMP_NS::PLUGIN_DEPENDENCIES};
}
