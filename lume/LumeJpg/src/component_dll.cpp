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

#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_decl.h>
#include <core/plugin/intf_plugin_register.h>
#include <jpg/implementation_uids.h>

#include "jpg/image_loader_jpg.h"

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

namespace JPGPlugin {
#ifdef __OHOS_PLATFORM__
const char* GetVersionInfo()
{
    return "GIT_TAG: GIT_REVISION: 8796bff GIT_BRANCH: dev";
}
#else
const char* GetVersionInfo();
#endif

static constexpr CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo JPG_LOADER {
    { CORE_NS::IImageLoaderManager::ImageLoaderTypeInfo::UID },
    nullptr,
    BASE_NS::Uid { "8a8010f4-f262-412f-95d1-389348b4a6ba" },
    CreateImageLoaderJPG,
    IMAGE_TYPES,
};

PluginToken RegisterInterfaces(IPluginRegister& pluginRegistry)
{
#if defined(CORE_PLUGIN) && (CORE_PLUGIN == 1)
    gPluginRegistry = &pluginRegistry;
#endif
    pluginRegistry.RegisterTypeInfo(JPG_LOADER);
    return &pluginRegistry;
}

void UnregisterInterfaces(PluginToken token)
{
    static_cast<IPluginRegister*>(token)->UnregisterTypeInfo(JPG_LOADER);
}
} // namespace JPGPlugin

extern "C" {
PLUGIN_DATA(JPGPlugin) {
    { IPlugin::UID },
    "JPGPlugin",
    /** Version information of the plugin. */
    { JPGPlugin::UID_JPG_PLUGIN, JPGPlugin::GetVersionInfo },
    JPGPlugin::RegisterInterfaces,
    JPGPlugin::UnregisterInterfaces,
    {},
};
}
