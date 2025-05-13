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

#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <3d/intf_plugin.h>
#include <base/util/uid.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_decl.h>
#include <font/implementation_uids.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <text_3d/ecs/components/text_component.h>
#include <text_3d/implementation_uids.h>

#include "ecs/systems/text_system.h"

// Rofs Data.
extern "C" const void* const BINARY_DATA_FOR_TEXT3D[];
extern "C" const uint64_t SIZE_OF_DATA_FOR_TEXT3D;

TEXT3D_BEGIN_NAMESPACE()
CORE_NS::IComponentManager* ITextComponentManagerInstance(CORE_NS::IEcs&);
CORE_NS::ISystem* TextSystemInstance(CORE_NS::IEcs&);

void ITextComponentManagerDestroy(CORE_NS::IComponentManager*);
void TextSystemDestroy(CORE_NS::ISystem*);

namespace {
static CORE_NS::IPluginRegister* gPluginRegistry { nullptr };

static constexpr RENDER_NS::IShaderManager::ShaderFilePathDesc SHADER_FILE_PATHS {
    "text3dshaders://",
    "",
    "",
    "",
};
constexpr BASE_NS::string_view ROFS = "text3drofs";
constexpr BASE_NS::string_view SHADERS = "text3dshaders";
constexpr BASE_NS::string_view SHADER_PATH = "text3drofs://shaders/";

static constexpr BASE_NS::Uid PLUGIN_DEPENDENCIES[] = { CORE3D_NS::UID_3D_PLUGIN, FONT_NS::UID_FONT_PLUGIN };

struct PluginState final {
    CORE3D_NS::IGraphicsContext& context_;
    /*
    CORE_NS::InterfaceTypeInfo interfaceInfo_ = CORE_NS::InterfaceTypeInfo {
        this,
        UID_FONT_MANAGER,
        "IFontManager",
        nullptr,
        [](CORE_NS::IClassRegister&, CORE_NS::PluginToken token) -> CORE_NS::IInterface* {
            if (token) {
                RenderPluginState* state = static_cast<RenderPluginState*>(token);
                if (!state->fontManager_) {
                    state->fontManager_.reset(new FontManager(state->context_));
                }
                return state->fontManager_.get();
            }
            return nullptr;
        },
    };
    */
    PluginState(CORE3D_NS::IGraphicsContext& context) : context_(context) {}
};

constexpr CORE_NS::ComponentManagerTypeInfo TEXT_COMPONENT_TYPE_INFO = {
    { CORE_NS::ComponentManagerTypeInfo::UID },
    ITextComponentManager::UID,
    CORE_NS::GetName<ITextComponentManager>().data(),
    ITextComponentManagerInstance,
    ITextComponentManagerDestroy,
};

constexpr CORE_NS::ComponentManagerTypeInfo COMPONENT_TYPE_INFOS[] = {
    TEXT_COMPONENT_TYPE_INFO,
};

// Text system dependencies.
static constexpr BASE_NS::Uid TEXT_SYSTEM_RW_DEPS[] = {
    CORE3D_NS::IRenderMeshComponentManager::UID,
    CORE3D_NS::IMaterialComponentManager::UID,
    CORE3D_NS::IMeshComponentManager::UID,
    CORE3D_NS::IRenderHandleComponentManager::UID,
};
static constexpr BASE_NS::Uid TEXT_SYSTEM_R_DEPS[] = {
    TEXT_COMPONENT_TYPE_INFO.uid,
};

constexpr CORE_NS::SystemTypeInfo SYSTEM_TYPE_INFOS[] = {
    {
        { CORE_NS::SystemTypeInfo::UID },
        TextSystem::UID,
        CORE_NS::GetName<TextSystem>().data(),
        TextSystemInstance,
        TextSystemDestroy,
        TEXT_SYSTEM_RW_DEPS,
        TEXT_SYSTEM_R_DEPS,
    },
};

CORE_NS::PluginToken CreatePlugin(CORE3D_NS::IGraphicsContext& context)
{
    PluginState* state = new PluginState { context };
    auto& renderContext = context.GetRenderContext();

    auto& fm = renderContext.GetEngine().GetFileManager();
    auto rofs = fm.CreateROFilesystem(BINARY_DATA_FOR_TEXT3D, SIZE_OF_DATA_FOR_TEXT3D);
    fm.RegisterFilesystem(ROFS, BASE_NS::move(rofs));
    fm.RegisterPath(SHADERS, SHADER_PATH, false);

    renderContext.GetDevice().GetShaderManager().LoadShaderFiles(SHADER_FILE_PATHS);

    return state;
}

void DestroyPlugin(CORE_NS::PluginToken token)
{
    PluginState* state = static_cast<PluginState*>(token);
    auto& renderContext = state->context_.GetRenderContext();

    renderContext.GetDevice().GetShaderManager().UnloadShaderFiles(SHADER_FILE_PATHS);

    auto& fm = renderContext.GetEngine().GetFileManager();
    fm.UnregisterPath(SHADERS, SHADER_PATH);
    fm.UnregisterFilesystem(ROFS);

    delete state;
}

static constexpr CORE3D_NS::I3DPlugin PLUGIN(CreatePlugin, DestroyPlugin);
} // namespace

// implementation in the generated version.cpp
const char* GetVersionInfo();

CORE_NS::PluginToken RegisterInterfaces(CORE_NS::IPluginRegister& pluginRegistry)
{
    gPluginRegistry = &pluginRegistry;
    for (const auto& info : COMPONENT_TYPE_INFOS) {
        pluginRegistry.RegisterTypeInfo(info);
    }
    for (const auto& info : SYSTEM_TYPE_INFOS) {
        pluginRegistry.RegisterTypeInfo(info);
    }
    pluginRegistry.RegisterTypeInfo(PLUGIN);
    return &pluginRegistry;
}

void UnregisterInterfaces(CORE_NS::PluginToken token)
{
    auto* pluginRegistry = static_cast<CORE_NS::IPluginRegister*>(token);
    pluginRegistry->UnregisterTypeInfo(PLUGIN);
    for (const auto& info : COMPONENT_TYPE_INFOS) {
        pluginRegistry->UnregisterTypeInfo(info);
    }
    for (const auto& info : SYSTEM_TYPE_INFOS) {
        pluginRegistry->UnregisterTypeInfo(info);
    }
}
TEXT3D_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
IPluginRegister& GetPluginRegister()
{
    return *TEXT3D_NS::gPluginRegistry;
}
CORE_END_NAMESPACE()

extern "C" {
PLUGIN_DATA(Text3DPlugin) {
    { CORE_NS::IPlugin::UID },
    // name of plugin.
    "3D Text Plugin",
    // Version information of the plugin.
    { TEXT3D_NS::UID_TEXT3D_PLUGIN, TEXT3D_NS::GetVersionInfo },
    TEXT3D_NS::RegisterInterfaces,
    TEXT3D_NS::UnregisterInterfaces,
    { TEXT3D_NS::PLUGIN_DEPENDENCIES },
};
}
