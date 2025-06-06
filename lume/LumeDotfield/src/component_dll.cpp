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

#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/ecs/systems/intf_render_system.h>
#include <3d/implementation_uids.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_system.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_decl.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_plugin.h>
#include <render/intf_render_context.h>

#include "dotfield/ecs/components/dotfield_component.h"
#include "dotfield/ecs/systems/dotfield_system.h"
#include "dotfield/implementation_uids.h"
#include "dotfield/render/intf_render_data_store_default_dotfield.h"
#include "render/render_data_store_default_dotfield.h"
#include "render/render_node_dotfield_simulation.h"

// Rofs Data.
extern "C" const void* const DOTFIELD_BIN[];
extern "C" const uint64_t DOTFIELD_BIN_SIZE;

CORE_BEGIN_NAMESPACE()
#if defined(CORE_PLUGIN) && (CORE_PLUGIN == 1)
IPluginRegister* gPluginRegistry { nullptr };
IPluginRegister& GetPluginRegister()
{
    return *gPluginRegistry;
}
#endif
CORE_END_NAMESPACE()

namespace {
static constexpr RENDER_NS::IShaderManager::ShaderFilePathDesc SHADER_FILE_PATHS {
    "dotfieldshaders://",
    "dotfieldshaderstates://",
    "dotfieldpipelinelayouts://",
    "dotfieldvertexinputdeclarations://",
};
}

namespace Dotfield {
using CORE_NS::GetName;
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

#ifdef __OHOS_PLATFORM__
const char* GetVersionInfo()
{
    return "GIT_TAG: GIT_REVISION: xxx GIT_BRANCH: dev";
}
#else
const char* GetVersionInfo();
#endif

constexpr string_view ROFS = "dotfieldrofs";
constexpr string_view SHADERS = "dotfieldshaders";
constexpr string_view VIDS = "dotfieldvertexinputdeclarations";
constexpr string_view PIDS = "dotfieldpipelinelayouts";
constexpr string_view SSTATES = "dotfieldshaderstates";

constexpr string_view SHADER_PATH = "dotfieldrofs://shaders/";
constexpr string_view VID_PATH = "dotfieldrofs://vertexinputdeclarations/";
constexpr string_view PID_PATH = "dotfieldrofs://pipelinelayouts/";
constexpr string_view SHADER_STATE_PATH = "dotfieldrofs://shaderstates/";

constexpr BASE_NS::Uid PLUGIN_DEPENDENCIES[] = { RENDER_NS::UID_RENDER_PLUGIN, CORE3D_NS::UID_3D_PLUGIN };

IComponentManager* IDotfieldComponentManagerInstance(IEcs&);
ISystem* IDotfieldSystemInstance(IEcs&);

void IDotfieldComponentManagerDestroy(IComponentManager*);
void IDotfieldSystemDestroy(ISystem*);

constexpr RenderDataStoreTypeInfo RenderDataDefaultDotfieldTypeInfo = {
    { RenderDataStoreTypeInfo::UID },
    RenderDataStoreDefaultDotfield::UID,
    RenderDataStoreDefaultDotfield::TYPE_NAME,
    RenderDataStoreDefaultDotfield::Create,
};

constexpr RenderDataStoreTypeInfo gRenderDataStores[] = {
    RenderDataDefaultDotfieldTypeInfo,
};

constexpr RenderNodeTypeInfo RenderNodeDotfieldSimulationTypeInfo = {
    { RenderNodeTypeInfo::UID },
    RenderNodeDotfieldSimulation::UID,
    RenderNodeDotfieldSimulation::TYPE_NAME,
    RenderNodeDotfieldSimulation::Create,
    RenderNodeDotfieldSimulation::Destroy,
    RenderNodeDotfieldSimulation::BACKEND_FLAGS,
    RenderNodeDotfieldSimulation::CLASS_TYPE,
    // after
    BASE_NS::Uid { "a25b27ea-a4ff-4b64-87c7-a7865cccfd92" },
    // before
    {},
};

constexpr RenderNodeTypeInfo gRenderNodes[] = {
    RenderNodeDotfieldSimulationTypeInfo,
};

constexpr ComponentManagerTypeInfo DotfieldComponentTypeInfo = {
    { ComponentManagerTypeInfo::UID },
    IDotfieldComponentManager::UID,
    GetName<IDotfieldComponentManager>().data(),
    IDotfieldComponentManagerInstance,
    IDotfieldComponentManagerDestroy,
};

constexpr ComponentManagerTypeInfo gComponentManagers[] = { DotfieldComponentTypeInfo };

constexpr Uid DotfieldSourceSystemRDeps[] = {
    ITransformComponentManager::UID,
    IDotfieldComponentManager::UID,
};

constexpr SystemTypeInfo gSystems[] = {
    {
        { SystemTypeInfo::UID },
        IDotfieldSystem::UID,
        GetName<IDotfieldSystem>().data(),
        IDotfieldSystemInstance,
        IDotfieldSystemDestroy,
        {},
        DotfieldSourceSystemRDeps,
        {},
        CORE3D_NS::IRenderPreprocessorSystem::UID,
    },
};

PluginToken Initialize(IRenderContext& context)
{
    auto& fm = context.GetEngine().GetFileManager();
    // Create dotfieldrofs:// filesystem that points to embedded asset files.

    auto rofs = fm.CreateROFilesystem(DOTFIELD_BIN, DOTFIELD_BIN_SIZE);

    fm.RegisterFilesystem(ROFS, move(rofs));
    // And register it under the needed proxy protocols.
    fm.RegisterPath(SHADERS, SHADER_PATH, false);
    fm.RegisterPath(VIDS, VID_PATH, false);
    fm.RegisterPath(PIDS, PID_PATH, false);
    fm.RegisterPath(SSTATES, SHADER_STATE_PATH, false);

    auto& renderDataStoreManager = context.GetRenderDataStoreManager();
    {
        auto rsp = refcnt_ptr<IRenderDataStorePod>(
            renderDataStoreManager.Create(IRenderDataStorePod::UID, "DotfieldPodStore"));
        if (rsp) {
            struct {
                Math::Vec4 texSizeInvTexSize { 2.f, 2.f, 1.f / 2.f, 1.f / 2.f };
            } config;
            rsp->CreatePod("TexSizeConfig", "TexSizeShaderConfig", arrayviewU8(config));
        }
    }

    context.GetDevice().GetShaderManager().LoadShaderFiles(SHADER_FILE_PATHS);
    return &context;
}

void Uninitialize(PluginToken token)
{
    auto context = static_cast<IRenderContext*>(token);
    context->GetDevice().GetShaderManager().UnloadShaderFiles(SHADER_FILE_PATHS);
    auto& fm = context->GetEngine().GetFileManager();
    fm.UnregisterPath(SHADERS, SHADER_PATH);
    fm.UnregisterPath(VIDS, VID_PATH);
    fm.UnregisterPath(PIDS, PID_PATH);
    fm.UnregisterFilesystem(ROFS);
}

PluginToken InitializeEcs(IEcs&)
{
    return {};
}

void UninitializeEcs(PluginToken) {}

static constexpr IRenderPlugin RENDER_PLUGIN(Initialize, Uninitialize);

static constexpr IEcsPlugin ECS_PLUGIN(InitializeEcs, UninitializeEcs);

PluginToken RegisterInterfaces(IPluginRegister& pluginRegistry)
{
#if defined(CORE_PLUGIN) && (CORE_PLUGIN == 1)
    gPluginRegistry = &pluginRegistry;
#endif
    for (const auto& info : gRenderDataStores) {
        pluginRegistry.RegisterTypeInfo(info);
    }
    for (const auto& info : gRenderNodes) {
        pluginRegistry.RegisterTypeInfo(info);
    }
    for (const auto& info : gComponentManagers) {
        pluginRegistry.RegisterTypeInfo(info);
    }
    for (const auto& info : gSystems) {
        pluginRegistry.RegisterTypeInfo(info);
    }
    pluginRegistry.RegisterTypeInfo(RENDER_PLUGIN);
    pluginRegistry.RegisterTypeInfo(ECS_PLUGIN);
    return &pluginRegistry;
}

void UnregisterInterfaces(PluginToken token)
{
    auto& pluginRegistry = *static_cast<IPluginRegister*>(token);
    for (const auto& info : gSystems) {
        pluginRegistry.UnregisterTypeInfo(info);
    }
    for (const auto& info : gComponentManagers) {
        pluginRegistry.UnregisterTypeInfo(info);
    }
    for (const auto& info : gRenderNodes) {
        pluginRegistry.UnregisterTypeInfo(info);
    }
    for (const auto& info : gRenderDataStores) {
        pluginRegistry.UnregisterTypeInfo(info);
    }
    pluginRegistry.UnregisterTypeInfo(ECS_PLUGIN);
    pluginRegistry.UnregisterTypeInfo(RENDER_PLUGIN);
}

} // namespace Dotfield

extern "C" {
PLUGIN_DATA(DotfieldPlugin) {
    { CORE_NS::IPlugin::UID },
    "DotfieldPlugin",
    { Dotfield::UID_DOTFIELD_PLUGIN, Dotfield::GetVersionInfo },
    Dotfield::RegisterInterfaces,
    Dotfield::UnregisterInterfaces,
    { Dotfield::PLUGIN_DEPENDENCIES },
};
}