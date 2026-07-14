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

#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/ecs/systems/intf_render_system.h>
#include <3d/implementation_uids.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_system.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_filesystem_api.h>
#include <core/log.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_decl.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/device/intf_device.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_plugin.h>
#include <render/intf_render_context.h>

#include "mrt_namespace.h"
#include "implementation_uids.h"
#include "intf_mrt_system.h"

// Rofs Data.
extern "C" const void* const MRT_BIN[];
extern "C" const uint64_t MRT_BIN_SIZE;

CORE_BEGIN_NAMESPACE()
#if defined(CORE_PLUGIN) && (CORE_PLUGIN == 1)
IPluginRegister* gPluginRegistry{nullptr};
IPluginRegister& GetPluginRegister()
{
    return *gPluginRegistry;
}
#endif
CORE_END_NAMESPACE()

MRT_BEGIN_NAMESPACE()
using CORE_NS::GetName;
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

ISystem* MRTSystemInstance(IEcs&);
void MRTSystemDestroy(ISystem*);

const char* GetVersionInfo()
{
    return "GIT_TAG: 2026.04.17 GIT_REVISION: I3aa5e2278795fe8e0f59d01662637ed8af56ce2c GIT_BRANCH: release";
}

namespace {
static constexpr RENDER_NS::IShaderManager::ShaderFilePathDesc SHADER_FILE_PATHS{
    "mrtshaders://",
    "mrtshaderstates://",
    "mrtpipelinelayouts://",
    "mrtvertexinputdeclarations://",
};

constexpr BASE_NS::Uid PLUGIN_DEPENDENCIES[] = {RENDER_NS::UID_RENDER_PLUGIN, CORE3D_NS::UID_3D_PLUGIN};

constexpr string_view ROFS = "mrt_rofs";
constexpr string_view FILE = "mrt_file";
constexpr string_view SHADERS = "mrtshaders";
constexpr string_view VIDS = "mrtvertexinputdeclarations";
constexpr string_view PIDS = "mrtpipelinelayouts";
constexpr string_view SSTATES = "mrtshaderstates";
constexpr string_view IMAGES = "mrtimages";
constexpr string_view TEXTURES = "mrttextures";

constexpr string_view SHADER_PATH = "mrt_rofs://shaders/";
constexpr string_view VID_PATH = "mrt_rofs://vertexinputdeclarations/";
constexpr string_view PID_PATH = "mrt_rofs://pipelinelayouts/";
constexpr string_view SHADER_STATE_PATH = "mrt_rofs://shaderstates/";
constexpr string_view IMAGE_PATH = "mrt_rofs://images/";
constexpr string_view TEXTURES_PATH = "mrt_rofs://textures/";

constexpr SystemTypeInfo gSystems[] = {
    {
        {SystemTypeInfo::UID},
        IMRTSystem::UID,
        GetName<IMRTSystem>().data(),
        MRTSystemInstance,
        MRTSystemDestroy,
        {},
        {},
        IRenderPreprocessorSystem::UID,
        IRenderSystem::UID,
    },
};

PluginToken Initialize(IRenderContext& context)
{
    auto& fm = context.GetEngine().GetFileManager();
    auto factory = CORE_NS::GetInstance<IFileSystemApi>(UID_FILESYSTEM_API_FACTORY);
    // Create mrt_rofs:// filesystem that points to embedded asset files.
    fm.RegisterFilesystem(ROFS, fm.CreateROFilesystem(MRT_BIN, MRT_BIN_SIZE));
    if (factory) {
        fm.RegisterFilesystem(FILE, factory->CreateStdFileSystem("file://"));
    } else {
        CORE_LOG_E("MRT get fileSystem failed");
    }
    // And register it under the needed proxy protocols.
    fm.RegisterPath(SHADERS, SHADER_PATH, false);
    fm.RegisterPath(VIDS, VID_PATH, false);
    fm.RegisterPath(PIDS, PID_PATH, false);
    fm.RegisterPath(SSTATES, SHADER_STATE_PATH, false);
    fm.RegisterPath(IMAGES, IMAGE_PATH, false);
    fm.RegisterPath(TEXTURES, TEXTURES_PATH, false);
    auto& shaderMgr = context.GetDevice().GetShaderManager();
    shaderMgr.LoadShaderFiles(SHADER_FILE_PATHS);

    shaderMgr.LoadShaderFile("mrt_rofs://shaders/shader/mrt_dm_env.shader");
    shaderMgr.LoadShaderFile("mrt_rofs://shaders/shader/mrt_dm_fw.shader");
    CORE_LOG_I("Initialize MRT ROFS: %s", ROFS.data());
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
    fm.UnregisterPath(SSTATES, SHADER_STATE_PATH);
    fm.UnregisterFilesystem(ROFS);
}

PluginToken InitializeEcs(IEcs& ecs)
{
    return {};
}
void UninitializeEcs(PluginToken token)
{}

static constexpr IRenderPlugin RENDER_PLUGIN(Initialize, Uninitialize);

static constexpr IEcsPlugin ECS_PLUGIN(InitializeEcs, UninitializeEcs);
}  // namespace

PluginToken RegisterInterfaces(IPluginRegister& pluginRegistry)
{
#if defined(CORE_PLUGIN) && (CORE_PLUGIN == 1)
    gPluginRegistry = &pluginRegistry;
#endif
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
    pluginRegistry.UnregisterTypeInfo(ECS_PLUGIN);
    pluginRegistry.UnregisterTypeInfo(RENDER_PLUGIN);
}
MRT_END_NAMESPACE()

extern "C" {
PLUGIN_DATA(PluginMRT){
    {CORE_NS::IPlugin::UID},
    "PluginMRT",
    {Mrt::UID_MRT_PLUGIN, Mrt::GetVersionInfo},
    Mrt::RegisterInterfaces,
    Mrt::UnregisterInterfaces,
    {Mrt::PLUGIN_DEPENDENCIES},
};
}
