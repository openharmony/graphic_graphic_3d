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

#include <base/containers/fixed_string.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/plugin/intf_plugin.h>
#include <render/namespace.h>

#include "render_context.h"

// Include the declarations directly from engine.
// NOTE: macro defined by cmake as CORE_STATIC_PLUGIN_HEADER="${CORE_ROOT_DIRECTORY}/src/static_plugin_decl.h"
// this is so that the core include directories are not leaked here, but we want this one header in this case.
#ifndef __APPLE__
#include CORE_STATIC_PLUGIN_HEADER
#include "registry_data.cpp"
#endif

// Rofs Data.
extern "C" {
extern const void* const BINARYDATAFORRENDER[];
extern const uint64_t SIZEOFDATAFORRENDER;
}

RENDER_BEGIN_NAMESPACE()
using namespace CORE_NS;
using BASE_NS::fixed_string;
using BASE_NS::string_view;
namespace {
constexpr string_view ROFS = "rofsRndr";
constexpr string_view SHADERS = "render_shaders";
constexpr string_view VIDS = "render_vertexinputdeclarations";
constexpr string_view PIDS = "render_pipelinelayouts";
constexpr string_view SSTATES = "render_shaderstates";
constexpr string_view RENDERDATA = "render_renderdataconfigurations";

constexpr string_view SHADER_PATH = "rofsRndr://shaders/";
constexpr string_view VID_PATH = "rofsRndr://vertexinputdeclarations/";
constexpr string_view PID_PATH = "rofsRndr://pipelinelayouts/";
constexpr string_view SHADER_STATE_PATH = "rofsRndr://shaderstates/";
constexpr string_view RENDERDATA_PATH = "rofsRndr://renderdataconfigurations/";

PluginToken CreatePlugin(IEngine& engine)
{
    RenderPluginState* token = new RenderPluginState { engine, {} };
    auto& registry = *engine.GetInterface<IClassRegister>();

    registry.RegisterInterfaceType(token->interfaceInfo_);

    IFileManager& fileManager = engine.GetFileManager();
#if (RENDER_EMBEDDED_ASSETS_ENABLED == 1)
    // Create engine:// protocol that points to embedded asset files.
    fileManager.RegisterFilesystem(ROFS, fileManager.CreateROFilesystem(BINARYDATAFORRENDER, SIZEOFDATAFORRENDER));
    fileManager.RegisterPath(SHADERS, SHADER_PATH, false);
    fileManager.RegisterPath(VIDS, VID_PATH, false);
    fileManager.RegisterPath(PIDS, PID_PATH, false);
    fileManager.RegisterPath(SSTATES, SHADER_STATE_PATH, false);
    fileManager.RegisterPath(RENDERDATA, RENDERDATA_PATH, false);
#endif

    return token;
}

void DestroyPlugin(PluginToken token)
{
    RenderPluginState* state = static_cast<RenderPluginState*>(token);
    IFileManager& fileManager = state->engine_.GetFileManager();
#if (RENDER_EMBEDDED_ASSETS_ENABLED == 1)
    fileManager.UnregisterPath(SHADERS, SHADER_PATH);
    fileManager.UnregisterPath(VIDS, VID_PATH);
    fileManager.UnregisterPath(PIDS, PID_PATH);
    fileManager.UnregisterPath(SSTATES, SHADER_STATE_PATH);
    fileManager.UnregisterPath(RENDERDATA, RENDERDATA_PATH);
    fileManager.UnregisterFilesystem(ROFS);
#endif

    auto& registry = *state->engine_.GetInterface<IClassRegister>();
    registry.UnregisterInterfaceType(state->interfaceInfo_);

    delete state;
}

constexpr IEnginePlugin ENGINE_PLUGIN(CreatePlugin, DestroyPlugin);
} // namespace

extern "C" void InitRegistry(CORE_NS::IPluginRegister& pluginRegistry);

PluginToken RegisterInterfaces(IPluginRegister& pluginRegistry)
{
    InitRegistry(pluginRegistry);
    pluginRegistry.RegisterTypeInfo(ENGINE_PLUGIN);

    return &pluginRegistry;
}

void UnregisterInterfaces(PluginToken token)
{
    IPluginRegister* pluginRegistry = static_cast<IPluginRegister*>(token);
    pluginRegistry->UnregisterTypeInfo(ENGINE_PLUGIN);
}
RENDER_END_NAMESPACE()
