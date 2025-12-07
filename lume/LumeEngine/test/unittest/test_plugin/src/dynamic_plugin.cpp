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

#include <core/ecs/intf_component_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_decl.h>

#include "dynamic_uids.h"
#include "static_uids.h"

namespace UTest {
const char* GetVersionInfo();
CORE_NS::PluginToken RegisterInterfaces(CORE_NS::IPluginRegister&);
void UnregisterInterfaces(CORE_NS::PluginToken);
} // namespace UTest

namespace {
extern "C" {
PLUGIN_DATA(CoreTestShared) {
    { CORE_NS::IPlugin::UID },
    // name of plugin.
    "Shared Test Plugin",
    // Version information of the plugin.
    { UTest::UID_SHARED_PLUGIN, UTest::GetVersionInfo },
    UTest::RegisterInterfaces,
    UTest::UnregisterInterfaces,
    {},
};
}
} // namespace

namespace UTest {
using namespace CORE_NS;

namespace {
struct GlobalToken {
    IPluginRegister& pluginRegistry;
    SharedGlobalTest test;
    InterfaceTypeInfo interfaceInfo {
        this,
        UID_SHARED_GLOBAL_TEST_IMPL,
        CORE_NS::GetName<ITest>().data(),
        nullptr,
        [](IClassRegister& registry, PluginToken token) -> IInterface* {
            return &static_cast<GlobalToken*>(token)->test;
        },
    };
};

struct EngineToken {
    IEngine& engine;
    InterfaceTypeInfo interfaceInfo {
        this,
        UID_SHARED_ENGINE_TEST_IMPL,
        CORE_NS::GetName<ITest>().data(),
        [](IClassFactory& factory, PluginToken token) -> IInterface* { return new SharedTest; },
        nullptr,
    };
};

PluginToken CreatePlugin(IEngine& engine)
{
    EngineToken* token = new EngineToken { engine };

    auto& registry = *engine.GetInterface<IClassRegister>();

    registry.RegisterInterfaceType(token->interfaceInfo);

    return token;
}

void DestroyPlugin(PluginToken token)
{
    EngineToken* state = static_cast<EngineToken*>(token);

    auto& registry = *state->engine.GetInterface<IClassRegister>();

    registry.UnregisterInterfaceType(state->interfaceInfo);

    delete state;
}

constexpr IEnginePlugin ENGINE_PLUGIN(CreatePlugin, DestroyPlugin);
} // namespace

const char* GetVersionInfo()
{
    return "dynamic 1.0";
}

CORE_NS::PluginToken RegisterInterfaces(CORE_NS::IPluginRegister& pluginRegistry)
{
    GlobalToken* token = new GlobalToken { pluginRegistry };

    pluginRegistry.RegisterTypeInfo(ENGINE_PLUGIN);
    pluginRegistry.GetClassRegister().RegisterInterfaceType(token->interfaceInfo);

    return token;
}

void UnregisterInterfaces(PluginToken token)
{
    GlobalToken* state = static_cast<GlobalToken*>(token);

    state->pluginRegistry.GetClassRegister().UnregisterInterfaceType(state->interfaceInfo);
    state->pluginRegistry.UnregisterTypeInfo(ENGINE_PLUGIN);

    delete state;
}
} // namespace UTest
