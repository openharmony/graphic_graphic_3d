/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <png/implementation_uids.h>

#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_decl.h>
#include <core/plugin/intf_plugin_register.h>

#include "png/image_loader_png.h"

using namespace CORE_NS;

CORE_BEGIN_NAMESPACE()
#if defined(CORE_PLUGIN) && (CORE_PLUGIN == 1)
IPluginRegister* gPluginRegistry { nullptr };
IPluginRegister& GetPluginRegister()
{
    return *gPluginRegistry;
}
#endif
CORE_END_NAMESPACE()

namespace PNGPlugin {
#ifdef __OHOS_PLATFORM__
const char* GetVersionInfo()
{
    return "GIT_TAG: GIT_REVISION: cd1bd88 GIT_BRANCH: dev";
}
#else
const char* GetVersionInfo();
#endif

static constexpr CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo PNG_LOADER {
    { CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo::UID },
    nullptr,
    BASE_NS::Uid { "41c1b7d1-e8a6-4aee-851b-c84de689a984" },
    CreateImageLoaderPng,
    IMAGE_TYPES,
};

PluginToken RegisterInterfaces(IPluginRegister& pluginRegistry)
{
#if defined(CORE_PLUGIN) && (CORE_PLUGIN == 1)
    gPluginRegistry = &pluginRegistry;
#endif
    pluginRegistry.RegisterTypeInfo(PNG_LOADER);
    return &pluginRegistry;
}

void UnregisterInterfaces(PluginToken token)
{
    static_cast<IPluginRegister*>(token)->UnregisterTypeInfo(PNG_LOADER);
}
} // namespace PNGPlugin

extern "C" {
PLUGIN_DATA(PngPlugin) {
    { IPlugin::UID },
    "PNGPlugin",
    /** Version information of the plugin. */
    { PNGPlugin::UID_PNG_PLUGIN, PNGPlugin::GetVersionInfo },
    PNGPlugin::RegisterInterfaces,
    PNGPlugin::UnregisterInterfaces,
    {},
};
}
