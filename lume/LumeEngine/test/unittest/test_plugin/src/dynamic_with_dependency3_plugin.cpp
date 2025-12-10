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
#include "dynamic_with_dependency3_uids.h"
#include "dynamic_with_dependency_uids.h"

namespace UTest3 {
const char* GetVersionInfo();
CORE_NS::PluginToken RegisterInterfaces(CORE_NS::IPluginRegister&);
void UnregisterInterfaces(CORE_NS::PluginToken);
constexpr BASE_NS::Uid PLUGIN_DEPENDENCIES[] = { UTest2::UID_SHARED_PLUGIN };
} // namespace UTest3

namespace {
extern "C" {
PLUGIN_DATA(CoreTestShared) {
    { CORE_NS::IPlugin::UID },
    // name of plugin.
    "Shared Test Plugin v3",
    // Version information of the plugin.
    { UTest3::UID_SHARED_PLUGIN, UTest3::GetVersionInfo },
    UTest3::RegisterInterfaces,
    UTest3::UnregisterInterfaces,
    { UTest3::PLUGIN_DEPENDENCIES },
};
}
} // namespace

namespace UTest3 {
using namespace CORE_NS;

namespace {
struct GlobalToken {
    IPluginRegister& pluginRegistry;
    SharedGlobalTest test;
    InterfaceTypeInfo interfaceInfo {
        this,
        UID_SHARED_GLOBAL_TEST_IMPL,
        CORE_NS::GetName<UTest::ITest>().data(),
        [](IClassFactory& factory, PluginToken token) -> IInterface* { return new SharedGlobalTest; },
        [](IClassRegister& registry, PluginToken token) -> IInterface* {
            return &static_cast<GlobalToken*>(token)->test;
        },
    };
};
} // namespace

const char* GetVersionInfo()
{
    return "dynamic 3.0";
}

CORE_NS::PluginToken RegisterInterfaces(CORE_NS::IPluginRegister& pluginRegistry)
{
    GlobalToken* token = new GlobalToken { pluginRegistry };
    return token;
}

void UnregisterInterfaces(PluginToken token)
{
    GlobalToken* state = static_cast<GlobalToken*>(token);
    delete state;
}
} // namespace UTest3
