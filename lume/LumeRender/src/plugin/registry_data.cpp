/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include <core/plugin/intf_plugin.h>
#include <render/implementation_uids.h>
#include <render/namespace.h>
#if defined(CORE_PLUGIN) && (CORE_PLUGIN == 1)
#include <core/plugin/intf_plugin_decl.h>
#else
// Include the declarations directly from engine.
// NOTE: macro defined by cmake as CORE_STATIC_PLUGIN_HEADER="${CORE_ROOT_DIRECTORY}/src/static_plugin_decl.h"
// this is so that the core include directories are not leaked here, but we want this one header in this case.
#include CORE_STATIC_PLUGIN_HEADER
#endif

CORE_BEGIN_NAMESPACE()
class IPluginRegister;
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
const char* GetVersionInfo();
CORE_NS::PluginToken RegisterInterfaces(CORE_NS::IPluginRegister&);
void UnregisterInterfaces(CORE_NS::PluginToken);
RENDER_END_NAMESPACE()

namespace {
extern "C" {
PLUGIN_DATA(AGPRender) {
    { CORE_NS::IPlugin::UID },
    // name of plugin.
    "AGP Render",
    // Version information of the plugin.
    { RENDER_NS::UID_RENDER_PLUGIN, RENDER_NS::GetVersionInfo },
    RENDER_NS::RegisterInterfaces,
    RENDER_NS::UnregisterInterfaces,
    {},
};
DEFINE_STATIC_PLUGIN(AGPRender);
}
} // namespace
