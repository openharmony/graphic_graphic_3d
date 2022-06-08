/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "platform_windows.h"
#include "os/platform.h"

#include <core/namespace.h>
#include <core/io/intf_file_manager.h>

#include "io/path_tools.h"
#include "util/string_util.h"


CORE_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;

PlatformWindows::PlatformWindows(PlatformCreateInfo const& createInfo)
{
    // Convert the input paths to absolute.
    auto cwd = GetCurrentDirectory();
    string_view curDrive, curPath, curFilename, curExt;
    SplitPath(cwd, curDrive, curPath, curFilename, curExt);

    auto fixPath = [&](string_view pathRaw) -> string {
        if (pathRaw.empty()) {
            return {};
        }
        // fix slashes. (just change \\ to /)
        string_view pathIn = pathRaw;
        string tmp;
        if (pathIn.find("\\") != string_view::npos) {
            tmp = pathIn;
            StringUtil::FindAndReplaceAll(tmp, "\\", "/");
            pathIn = tmp;
        }

        string_view drive, path, filename, ext;
        SplitPath(pathIn, drive, path, filename, ext);
        string res = "/";
        if (drive.empty()) {
            // relative to current drive then
            res += curDrive;
        } else {
            res += drive;
        }
        res += ":";
        string normalizedPath;
        if (path.empty()) {
            normalizedPath = curPath;
        } else {
            if (path[0] != '/') {
                // relative path.
                normalizedPath = NormalizePath(curPath + path);
            } else {
                normalizedPath = NormalizePath(path);
            }
        }
        if (normalizedPath.empty()) {
            // Invalid path? how to handle this?
            // just fallback on current path for now. (hoping that it's somewhat safe)
            normalizedPath = curPath;
        }
        res += normalizedPath;
        return string(res.substr(1));
    };

    plat_.coreRootPath = fixPath(createInfo.coreRootPath);
    plat_.appRootPath = fixPath(createInfo.appRootPath);
    plat_.appPluginPath = fixPath(createInfo.appPluginPath);
}

const PlatformData& PlatformWindows::GetPlatformData() const
{
    return plat_;
}

string PlatformWindows::RegisterDefaultPaths(IFileManager& fileManager)
{
    // register path to system plugins (this does not actually do anything anymore, pluginregistry has it's one
    // filemanager instance etc..) Root path is the location where system plugins , non-rofs assets etc could be held.
    const string coreDirectory = "file://" + plat_.coreRootPath;

    // Create plugins:// protocol that points to plugin files under coredirectory.
    fileManager.RegisterPath("plugins", coreDirectory + "plugins/", false);

#if (CORE_EMBEDDED_ASSETS_ENABLED == 0) || (CORE_DEV_ENABLED == 1)
    const string assetRoot = coreDirectory + "assets/";

    // Create engine:// protocol that points to core asset files on the filesystem.
    CORE_LOG_I("Registered core asset path: '%score/'", assetRoot.c_str());
    fileManager.RegisterPath("engine", assetRoot + "core/", false);
#endif
    return coreDirectory;
}

void PlatformWindows::RegisterPluginLocations(IPluginRegister& registry)
{
    constexpr string_view fileproto("file://");
    registry.RegisterPluginPath(fileproto + GetPlatformData().coreRootPath + "plugins/");
    if (!GetPlatformData().appPluginPath.empty()) {
        registry.RegisterPluginPath(fileproto + GetPlatformData().appPluginPath);
    }
}

CORE_NS::IPlatform::Ptr Platform::Create(PlatformCreateInfo const& createInfo)
{
    return CORE_NS::IPlatform::Ptr(new PlatformWindows(createInfo));
}

CORE_END_NAMESPACE()

