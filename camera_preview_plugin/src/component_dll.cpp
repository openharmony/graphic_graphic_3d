/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <3d/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/implementation_uids.h>
#include <core/io/intf_file_manager.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_decl.h>
#include <core/log.h>
#include <render/device/intf_device.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_plugin.h>
#include <render/intf_render_context.h>

extern "C" const void *const CAM_PREVIEW_BIN[];
extern "C" const uint64_t CAM_PREVIEW_BIN_SIZE;

#include "cam_preview/namespace.h"
#include "cam_preview/implementation_uids.h"

CORE_BEGIN_NAMESPACE()
#if defined(CORE_PLUGIN) && (CORE_PLUGIN == 1)
IPluginRegister *gPluginRegistry{nullptr};
IPluginRegister &GetPluginRegister()
{
    return *gPluginRegistry;
}
#endif
CORE_END_NAMESPACE()

namespace {
static constexpr RENDER_NS::IShaderManager::ShaderFilePathDesc SHADER_FILE_PATHS{
    "campreviewshaders://",
    "campreviewshaderstates://",
    "campreviewpipelinelayouts://",
    "campreviewvertexinputdeclarations://",
};
}

namespace CamPreview {
using CORE_NS::GetName;
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

IComponentManager *IMotphysVfxEffectComponentManagerInstance(IEcs &);
void IMotphysVfxEffectComponentManagerDestroy(IComponentManager *);

ISystem *IMotphysVfxSystemInstance(IEcs &);
void IMotphysVfxSystemDestroy(ISystem *);

IComponentManager *IMotphysVfxStateComponentManagerInstance(IEcs &);
void IMotphysVfxStateComponentManagerDestroy(IComponentManager *);

const char *GetVersionInfo()
{
    return "GIT_TAG: 2024.12.24 GIT_REVISION: 34eac7a GIT_BRANCH: release";
}

namespace {

constexpr string_view ROFS = "campreviewrofs";
constexpr string_view SHADERS = "campreviewshaders";
constexpr string_view VIDS = "campreviewvertexinputdeclarations";
constexpr string_view PIDS = "campreviewpipelinelayouts";
constexpr string_view SSTATES = "campreviewshaderstates";

constexpr string_view SHADER_PATH = "campreviewrofs://shaders/";
constexpr string_view VID_PATH = "campreviewrofs://vertexinputdeclarations/";
constexpr string_view PID_PATH = "campreviewrofs://pipelinelayouts/";
constexpr string_view SHADER_STATE_PATH = "campreviewrofs://shaderstates/";

constexpr BASE_NS::Uid PLUGIN_DEPENDENCIES[] = {RENDER_NS::UID_RENDER_PLUGIN, CORE3D_NS::UID_3D_PLUGIN};

PluginToken Initialize(IRenderContext &context)
{
    auto &fm = context.GetEngine().GetFileManager();
    // Create campreviewrofs:// filesystem that points to embedded asset files.
    auto rofs = fm.CreateROFilesystem(CAM_PREVIEW_BIN, CAM_PREVIEW_BIN_SIZE);

    fm.RegisterFilesystem(ROFS, move(rofs));
    // And register it under the needed proxy protocols.
    fm.RegisterPath(SHADERS, SHADER_PATH, false);
    fm.RegisterPath(VIDS, VID_PATH, false);
    fm.RegisterPath(PIDS, PID_PATH, false);
    fm.RegisterPath(SSTATES, SHADER_STATE_PATH, false);

    context.GetDevice().GetShaderManager().LoadShaderFiles(SHADER_FILE_PATHS);
    return &context;
}

void Uninitialize(PluginToken token)
{
    auto context = static_cast<IRenderContext *>(token);
    context->GetDevice().GetShaderManager().UnloadShaderFiles(SHADER_FILE_PATHS);
    auto &fm = context->GetEngine().GetFileManager();
    fm.UnregisterPath(SHADERS, SHADER_PATH);
    fm.UnregisterPath(VIDS, VID_PATH);
    fm.UnregisterPath(PIDS, PID_PATH);
    fm.UnregisterPath(SSTATES, SHADER_STATE_PATH);
    fm.UnregisterFilesystem(ROFS);
}

PluginToken InitializeEcs(IEcs &ecs)
{
    return {};
}

void UninitializeEcs(PluginToken token)
{}

static constexpr IRenderPlugin RENDER_PLUGIN(Initialize, Uninitialize);

static constexpr IEcsPlugin ECS_PLUGIN(InitializeEcs, UninitializeEcs);
}  // namespace

PluginToken RegisterInterfaces(IPluginRegister &pluginRegistry)
{
#if defined(CORE_PLUGIN) && (CORE_PLUGIN == 1)
    gPluginRegistry = &pluginRegistry;
#endif

    pluginRegistry.RegisterTypeInfo(RENDER_PLUGIN);
    pluginRegistry.RegisterTypeInfo(ECS_PLUGIN);
    return &pluginRegistry;
}

void UnregisterInterfaces(PluginToken token)
{
    if (!token) {
        CORE_LOG_D("An null pointer was passed to UnregisterInterfaces function.");
        return;
    }
    auto &pluginRegistry = *static_cast<IPluginRegister *>(token);

    pluginRegistry.UnregisterTypeInfo(ECS_PLUGIN);
    pluginRegistry.UnregisterTypeInfo(RENDER_PLUGIN);
}
}  // namespace CamPreview

extern "C" {
PLUGIN_DATA(CamPreviewPlugin) {
    {CORE_NS::IPlugin::UID},
    "CamPreviewPlugin",
    {CamPreview::UID_CAMERA_PREVIEW_PLUGIN, CamPreview::GetVersionInfo},
    CamPreview::RegisterInterfaces,
    CamPreview::UnregisterInterfaces,
    {CamPreview::PLUGIN_DEPENDENCIES},
};
}
