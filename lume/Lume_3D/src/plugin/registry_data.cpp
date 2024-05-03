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

#include <core/plugin/intf_plugin.h>
#if defined(CORE_PLUGIN) && (CORE_PLUGIN == 1)
#include <core/plugin/intf_plugin_decl.h>
#else
// Include the declarations directly from engine.
// NOTE: macro defined by cmake as CORE_STATIC_PLUGIN_HEADER="${CORE_ROOT_DIRECTORY}/src/static_plugin_decl.h"
// this is so that the core include directories are not leaked here, but we want this one header in this case.
#include CORE_STATIC_PLUGIN_HEADER
#endif
#include <3d/implementation_uids.h>
#include <3d/namespace.h>
#include <render/implementation_uids.h>

CORE_BEGIN_NAMESPACE()
class IPluginRegister;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
CORE_NS::PluginToken RegisterInterfaces3D(CORE_NS::IPluginRegister&);
void UnregisterInterfaces3D(CORE_NS::PluginToken);
const char* GetVersionInfo();
CORE3D_END_NAMESPACE()

namespace {
constexpr BASE_NS::Uid PLUGIN_DEPENDENCIES[] = { RENDER_NS::UID_RENDER_PLUGIN };
extern "C" {
PLUGIN_DATA(AGP3D) {
    { CORE_NS::IPlugin::UID },
    // name of plugin.
    "AGP 3D (core)",
    // Version information of the plugin.
    { CORE3D_NS::UID_3D_PLUGIN, CORE3D_NS::GetVersionInfo },
    CORE3D_NS::RegisterInterfaces3D,
    CORE3D_NS::UnregisterInterfaces3D,
    { PLUGIN_DEPENDENCIES },
};
DEFINE_STATIC_PLUGIN(AGP3D);
}
} // namespace
