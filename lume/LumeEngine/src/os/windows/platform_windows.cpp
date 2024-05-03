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

#include "platform_windows.h"

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <core/io/intf_file_manager.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/os/intf_platform.h>
#include <core/os/platform_create_info.h>
#include <core/plugin/intf_plugin_register.h>

#include "io/path_tools.h"
#include "os/platform.h"
#include "util/string_util.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;

PlatformWindows::PlatformWindows(PlatformCreateInfo const& createInfo)
{
    // Convert the input paths to absolute.
    auto cwd = GetCurrentDirectory();
    string_view curDrive;
    string_view curPath;
    string_view curFilename;
    string_view curExt;
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

        string_view drive;
        string_view path;
        string_view filename;
        string_view ext;
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
