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
    plat_.appRootPath = createInfo.appRootPath;
    plat_.appPluginPath = createInfo.appPluginPath;
    plat_.hapPath = createInfo.hapPath;
    plat_.bundleName = createInfo.bundleName;
    plat_.moduleName = createInfo.moduleName;
}

BASE_NS::string PlatformOHOS::RegisterDefaultPaths(IFileManager& fileManager)
{
    // register HapFilesystem
    BASE_NS::string hapPath = plat_.hapPath;
    BASE_NS::string bundleName = plat_.bundleName;
    BASE_NS::string moduleName = plat_.moduleName;
    fileManager.RegisterFilesystem("OhosRawFile",
        IFilesystem::Ptr{new Core::OhosFilesystem(hapPath, bundleName, moduleName)});
    CORE_LOG_I("Registered hapFilesystem by Platform: 'hapPath:%s bundleName:%s moduleName:%s'",
        hapPath.c_str(), bundleName.c_str(), moduleName.c_str());
    const BASE_NS::string coreDirectory = "file://" + plat_.coreRootPath;
    fileManager.RegisterPath("plugins", coreDirectory, false);
    return coreDirectory;
}

PlatformOHOS::~PlatformOHOS() {}

void PlatformOHOS::RegisterPluginLocations(IPluginRegister& registry)
{
    constexpr BASE_NS::string_view fileproto("file://");
    registry.RegisterPluginPath(fileproto + GetPlatformData().coreRootPath);
    if (!GetPlatformData().appPluginPath.empty()) {
        registry.RegisterPluginPath(fileproto + GetPlatformData().appPluginPath);
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
CORE_END_NAMESPACE()
