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

#include <atomic>

#include <core/ecs/intf_component_manager.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/property_types.h>

#include "static_uids.h"

// Include the declarations directly from engine.
// NOTE: macro defined by cmake as CORE_STATIC_PLUGIN_HEADER="${CORE_ROOT_DIRECTORY}/src/static_plugin_decl.h"
// this is so that the core include directories are not leaked here, but we want this one header in this case.
#include CORE_STATIC_PLUGIN_HEADER

namespace UTest {
const char* GetVersionInfo();
CORE_NS::PluginToken RegisterInterfacesStatic(CORE_NS::IPluginRegister&);
void UnregisterInterfacesStatic(CORE_NS::PluginToken);
} // namespace UTest

extern "C" {
PLUGIN_DATA(CoreTestStatic) {
    { CORE_NS::IPlugin::UID },
    // name of plugin.
    "Static Test Plugin",
    // Version information of the plugin.
    { UTest::UID_STATIC_PLUGIN, UTest::GetVersionInfo },
    UTest::RegisterInterfacesStatic,
    UTest::UnregisterInterfacesStatic,
    {},
};
DEFINE_STATIC_PLUGIN(CoreTestStatic);
}

// Rofs Data.
extern "C" {
extern const void* const BINARYDATAFORRENDER[];
extern const uint64_t SIZEOFDATAFORRENDER;
}

namespace UTest {
using namespace CORE_NS;

namespace {
struct EngineToken {
    IEngine& engine;
    InterfaceTypeInfo interfaceInfo {
        this,
        UID_STATIC_ENGINE_TEST_IMPL,
        CORE_NS::GetName<ITest>().data(),
        [](IClassFactory& factory, CORE_NS::PluginToken token) -> IInterface* { return new StaticTest; },
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

struct GlobalToken {
    IPluginRegister& pluginRegistry;
    StaticGlobalTest test;
    InterfaceTypeInfo interfaceInfo {
        this,
        UID_STATIC_GLOBAL_TEST_IMPL,
        CORE_NS::GetName<ITest>().data(),
        [](IClassFactory& factory, PluginToken token) -> IInterface* { return new StaticGlobalTest; },
        [](IClassRegister& registry, PluginToken token) -> IInterface* {
            return &static_cast<GlobalToken*>(token)->test;
        },
    };
};
} // namespace

const char* GetVersionInfo()
{
    return "static 1.0";
}

CORE_NS::PluginToken RegisterInterfacesStatic(IPluginRegister& pluginRegistry)
{
    GlobalToken* token = new GlobalToken { pluginRegistry };

    pluginRegistry.RegisterTypeInfo(ENGINE_PLUGIN);
    pluginRegistry.GetClassRegister().RegisterInterfaceType(token->interfaceInfo);

    return token;
}

void UnregisterInterfacesStatic(PluginToken token)
{
    GlobalToken* state = static_cast<GlobalToken*>(token);

    state->pluginRegistry.GetClassRegister().UnregisterInterfaceType(state->interfaceInfo);
    state->pluginRegistry.UnregisterTypeInfo(ENGINE_PLUGIN);

    delete state;
}
} // namespace UTest
