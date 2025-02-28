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

#include <base/util/uid.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_decl.h>
#include <font/implementation_uids.h>
#include <render/implementation_uids.h>
#include <render/intf_plugin.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>

#include "font_manager.h"

FONT_BEGIN_NAMESPACE()
namespace {
static CORE_NS::IPluginRegister* gPluginRegistry { nullptr };

static constexpr BASE_NS::Uid PLUGIN_DEPENDENCIES[] = { RENDER_NS::UID_RENDER_PLUGIN };

struct RenderPluginState final {
    RENDER_NS::IRenderContext& context_;
    FontManager::Ptr fontManager_;
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
    RenderPluginState(RENDER_NS::IRenderContext& context) : context_(context) {}
};

CORE_NS::PluginToken CreatePlugin(RENDER_NS::IRenderContext& context)
{
    RenderPluginState* state = new RenderPluginState { context };
    auto& registry = *context.GetInterface<CORE_NS::IClassRegister>();
    registry.RegisterInterfaceType(state->interfaceInfo_);
    return state;
}

void DestroyPlugin(CORE_NS::PluginToken token)
{
    if (token == nullptr) {
        return;
    }
    RenderPluginState* state = static_cast<RenderPluginState*>(token);
    auto& registry = *state->context_.GetInterface<CORE_NS::IClassRegister>();
    registry.UnregisterInterfaceType(state->interfaceInfo_);
    delete state;
}

static constexpr RENDER_NS::IRenderPlugin RENDER_PLUGIN(CreatePlugin, DestroyPlugin);
} // namespace

// implementation in the generated version.cpp
const char* GetVersionInfo();

CORE_NS::PluginToken RegisterInterfaces(CORE_NS::IPluginRegister& pluginRegistry)
{
    gPluginRegistry = &pluginRegistry;

    pluginRegistry.RegisterTypeInfo(RENDER_PLUGIN);
    return &pluginRegistry;
}

void UnregisterInterfaces(CORE_NS::PluginToken token)
{
    auto* pluginRegistry = static_cast<CORE_NS::IPluginRegister*>(token);
    pluginRegistry->UnregisterTypeInfo(RENDER_PLUGIN);
}
FONT_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
IPluginRegister& GetPluginRegister()
{
    return *FONT_NS::gPluginRegistry;
}
CORE_END_NAMESPACE()

extern "C" {
PLUGIN_DATA(FontPlugin) {
    { CORE_NS::IPlugin::UID },
    // name of plugin.
    "Font Plugin",
    // Version information of the plugin.
    { FONT_NS::UID_FONT_PLUGIN, FONT_NS::GetVersionInfo },
    FONT_NS::RegisterInterfaces,
    FONT_NS::UnregisterInterfaces,
    { FONT_NS::PLUGIN_DEPENDENCIES },
};
}
