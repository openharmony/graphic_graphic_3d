/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "3d_test_plugin.h"

#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <3d/intf_plugin.h>
#include <core/ecs/intf_component_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_decl.h>

namespace TestPlugin3D {
const char* GetVersionInfo();
CORE_NS::PluginToken RegisterInterfaces(CORE_NS::IPluginRegister&);
void UnregisterInterfaces(CORE_NS::PluginToken);
constexpr BASE_NS::Uid PLUGIN_DEPENDENCIES[] = { CORE3D_NS::UID_3D_PLUGIN };
} // namespace TestPlugin3D

namespace {
extern "C" {
PLUGIN_DATA(Core3DTestPlugin) {
    { CORE_NS::IPlugin::UID },
    // name of plugin.
    "3D Test Plugin",
    // Version information of the plugin.
    { TestPlugin3D::UID_3D_TEST_PLUGIN, TestPlugin3D::GetVersionInfo },
    TestPlugin3D::RegisterInterfaces,
    TestPlugin3D::UnregisterInterfaces,
    TestPlugin3D::PLUGIN_DEPENDENCIES,
};
}
} // namespace

namespace TestPlugin3D {
using namespace CORE_NS;
using namespace CORE3D_NS;

namespace {
struct GlobalToken {
    IPluginRegister& pluginRegistry;
    GlobalImpl test;
    InterfaceTypeInfo interfaceInfo {
        this,
        UID_3D_GLOBAL_TEST_IMPL,
        CORE_NS::GetName<ITest>().data(),
        nullptr,
        [](IClassRegister& registry, PluginToken token) -> IInterface* {
            return &static_cast<GlobalToken*>(token)->test;
        },
    };
};

struct Token {
    IGraphicsContext& context;
    InterfaceTypeInfo interfaceInfo {
        this,
        UID_3D_CONTEXT_TEST_IMPL,
        CORE_NS::GetName<ITest>().data(),
        [](IClassFactory& factory, PluginToken token) -> IInterface* { return new ContextImpl; },
        nullptr,
    };
};

PluginToken CreatePlugin(IGraphicsContext& context)
{
    Token* token = new Token { context };

    auto& registry = *context.GetInterface<IClassRegister>();

    registry.RegisterInterfaceType(token->interfaceInfo);

    return token;
}

void DestroyPlugin(PluginToken token)
{
    Token* state = static_cast<Token*>(token);

    auto& registry = *state->context.GetInterface<IClassRegister>();

    registry.UnregisterInterfaceType(state->interfaceInfo);

    delete state;
}

constexpr I3DPlugin TEST_PLUGIN(CreatePlugin, DestroyPlugin);
} // namespace

const char* GetVersionInfo()
{
    return "3D test 1.0";
}

CORE_NS::PluginToken RegisterInterfaces(CORE_NS::IPluginRegister& pluginRegistry)
{
    GlobalToken* token = new GlobalToken { pluginRegistry, {} };

    pluginRegistry.RegisterTypeInfo(TEST_PLUGIN);
    pluginRegistry.GetClassRegister().RegisterInterfaceType(token->interfaceInfo);

    return token;
}

void UnregisterInterfaces(PluginToken token)
{
    GlobalToken* state = static_cast<GlobalToken*>(token);

    state->pluginRegistry.GetClassRegister().UnregisterInterfaceType(state->interfaceInfo);
    state->pluginRegistry.UnregisterTypeInfo(TEST_PLUGIN);

    delete state;
}
} // namespace TestPlugin3D
