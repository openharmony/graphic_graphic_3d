/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: "plugin registrations"
 * Author: Jani Kattelus
 * Create: 2022-05-25
 */

#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>

#include <meta/base/plugin.h>

#include "meta_object_lib.h"

namespace {
static CORE_NS::IPluginRegister* g_pluginRegistry { nullptr };
} // namespace

CORE_BEGIN_NAMESPACE()
IPluginRegister& GetPluginRegister()
{
    return *g_pluginRegistry;
}
CORE_END_NAMESPACE()

using namespace CORE_NS;
using namespace META_NS;

META_BEGIN_NAMESPACE()

namespace {

MetaObjectLib* g_state = nullptr;

// Register our interface to the GLOBAL registry.
constexpr CORE_NS::InterfaceTypeInfo INTERFACE = { &g_state, IMetaObjectLib::UID, "Meta Object Lib", nullptr,
    [](CORE_NS::IClassRegister& registry, CORE_NS::PluginToken token) -> CORE_NS::IInterface* {
        return *static_cast<IMetaObjectLib**>(token);
    } };

PluginToken RegisterInterfaces(IPluginRegister& pluginRegistry)
{
    // Initializing dynamic plugin.
    // Pluginregistry access via the provided registry instance which is saved here.
    g_pluginRegistry = &pluginRegistry;
    if (g_state == nullptr) {
        g_state = new MetaObjectLib;

        auto& classRegister = CORE_NS::GetPluginRegister().GetClassRegister();
        classRegister.RegisterInterfaceType(INTERFACE);

        g_state->Initialize();
    }
    return g_state;
}
void UnregisterInterfaces(PluginToken token)
{
    if (token != g_state) {
        return;
    }
    if (g_state) {
        g_state->Uninitialize();

        delete g_state;
        g_state = nullptr;

        auto& classRegister = CORE_NS::GetPluginRegister().GetClassRegister();
        classRegister.UnregisterInterfaceType(INTERFACE);
    }

    g_pluginRegistry = nullptr;
}
const char* VersionString()
{
    return META_VERSION_STRING;
}

// const BASE_NS::Uid plugin_deps[] {};
} // namespace

META_END_NAMESPACE()

extern "C" {
#if defined(_MSC_VER)
_declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
    // NOLINTNEXTLINE(readability-identifier-naming) to keep LumeEngine convention
    CORE_NS::IPlugin gPluginData { { IPlugin::UID }, "MetaObject",
        /** Version information of the plugin. */
        CORE_NS::Version { META_NS::META_OBJECT_PLUGIN_UID, META_NS::VersionString }, META_NS::RegisterInterfaces,
        META_NS::UnregisterInterfaces, {} /*plugin_deps*/ };
}
