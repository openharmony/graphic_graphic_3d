/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "platform_ohos.h"
#include "os/platform.h"

#include <core/log.h>
#include <core/namespace.h>
#include <core/io/intf_file_manager.h>

CORE_BEGIN_NAMESPACE()

PlatformOHOS::PlatformOHOS(PlatformCreateInfo const& createInfo)
{
    plat_.coreRootPath = createInfo.coreRootPath;
    plat_.appRootPath = createInfo.appRootPath;
    plat_.appPluginPath = createInfo.appPluginPath;
}

PlatformOHOS::~PlatformOHOS()
{
}

BASE_NS::string PlatformOHOS::RegisterDefaultPaths(IFileManager& fileManager)
{
    // register path to system plugins (this does not actually do anything anymore, pluginregistry has it's one
    // filemanager instance etc..) Root path is the location where system plugins , non-rofs assets etc could be held.
    const BASE_NS::string coreDirectory = "file://" + plat_.coreRootPath;

    // Create plugins:// protocol that points to plugin files under coredirectory.
    fileManager.RegisterPath("plugins", coreDirectory, false);

#if (CORE_EMBEDDED_ASSETS_ENABLED == 0) || (CORE_DEV_ENABLED == 1)
    const BASE_NS::string assetRoot = plat_.appRootPath + "assets/";

    // Create engine:// protocol that points to core asset files on the filesystem.
    CORE_LOG_I("Registered core asset path: '%score/'", assetRoot.c_str());
    fileManager.RegisterPath("engine", assetRoot + "core/", false);
#endif
    return coreDirectory;
}

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
