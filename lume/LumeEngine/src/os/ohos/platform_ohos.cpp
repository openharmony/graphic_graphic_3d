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

#include "platform_ohos.h"

#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <core/namespace.h>

#include "os/ohos/ohos_filesystem.h"
#include "os/platform.h"

CORE_BEGIN_NAMESPACE()
PlatformOHOS::PlatformOHOS(PlatformCreateInfo const& createInfo)
{
    plat_.coreRootPath = createInfo.coreRootPath;
    if (createInfo.corePluginPath.empty()) {
        plat_.corePluginPath = plat_.coreRootPath;
    } else {
        plat_.corePluginPath = createInfo.corePluginPath;
    }
    plat_.appRootPath = createInfo.appRootPath;
    plat_.appPluginPath = createInfo.appPluginPath;
    plat_.hapPath = createInfo.hapPath;
    plat_.bundleName = createInfo.bundleName;
    plat_.moduleName = createInfo.moduleName;
    plat_.resourceManager = createInfo.resourceManager;
}

BASE_NS::string PlatformOHOS::RegisterDefaultPaths(IFileManager& fileManager) const
{
    // register HapFilesystem
    const BASE_NS::string hapPath = plat_.hapPath;
    const BASE_NS::string bundleName = plat_.bundleName;
    const BASE_NS::string moduleName = plat_.moduleName;
    auto resManager = plat_.resourceManager;
    fileManager.RegisterFilesystem("OhosRawFile",
        IFilesystem::Ptr{new Core::OhosFilesystem(hapPath, bundleName, moduleName, resManager)});
    CORE_LOG_I("Registered hapFilesystem by Platform: 'hapPath:%s bundleName:%s moduleName:%s'",
        hapPath.c_str(), bundleName.c_str(), moduleName.c_str());
    const BASE_NS::string coreDirectory = "file://" + plat_.coreRootPath;
    return coreDirectory;
}

PlatformOHOS::~PlatformOHOS() {}

void PlatformOHOS::RegisterPluginLocations(IPluginRegister& registry)
{
    constexpr BASE_NS::string_view fileproto("file://");
    if (!plat_.corePluginPath.empty()) {
        registry.RegisterPluginPath(fileproto + plat_.corePluginPath);
    }
    if (!plat_.appPluginPath.empty()) {
        if (plat_.appPluginPath != plat_.corePluginPath) {
            registry.RegisterPluginPath(fileproto + plat_.appPluginPath);
        }
    }
}

const PlatformData& PlatformOHOS::GetPlatformData() const
{
    return plat_;
}

CORE_NS::IPlatform::Ptr Platform::Create(PlatformCreateInfo const& createInfo)
{
    return CORE_NS::IPlatform::Ptr(new PlatformOHOS(createInfo));
}

const CORE_NS::PlatformCreateExtensionInfo* Platform::Extensions(PlatformCreateInfo const& createInfo)
{
    return createInfo.extensions;
}
CORE_END_NAMESPACE()
