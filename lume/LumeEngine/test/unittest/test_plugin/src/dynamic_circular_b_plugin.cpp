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

#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_decl.h>

#include "dynamic_circular_uids.h"

namespace UTestCycle {
const char* GetVersionInfoB()
{
    return "dynamic circular B 1.0";
}

CORE_NS::PluginToken RegisterInterfacesB(CORE_NS::IPluginRegister&)
{
    return nullptr;
}

void UnregisterInterfacesB(CORE_NS::PluginToken)
{}

constexpr BASE_NS::Uid PLUGIN_DEPENDENCIES_B[] = {UID_SHARED_PLUGIN_A};
}  // namespace UTestCycle

namespace {
extern "C" {
PLUGIN_DATA(CoreTestSharedCircularB){
    {CORE_NS::IPlugin::UID},
    "Shared Circular Test Plugin B",
    {UTestCycle::UID_SHARED_PLUGIN_B, UTestCycle::GetVersionInfoB},
    UTestCycle::RegisterInterfacesB,
    UTestCycle::UnregisterInterfacesB,
    {UTestCycle::PLUGIN_DEPENDENCIES_B},
};
}
}  // namespace
