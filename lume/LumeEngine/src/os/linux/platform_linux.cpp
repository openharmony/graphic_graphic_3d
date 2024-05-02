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

#include "platform_linux.h"

#include <core/io/intf_file_manager.h>
#include <core/namespace.h>
#include <core/os/platform_create_info.h>

#include "io/path_tools.h"
#include "os/platform.h"
#include "util/string_util.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;

PlatformLinux::PlatformLinux(PlatformCreateInfo const& createInfo)
{
    // Convert the input paths to absolute.
    auto cwd = GetCurrentDirectory();
    string_view curPath = cwd;

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

        string res = "/";
        string_view path = pathIn;

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

const PlatformData& PlatformLinux::GetPlatformData() const
{
    return plat_;
}

void PlatformLinux::RegisterPluginLocations(IPluginRegister& registry)
{
    constexpr string_view fileproto("file://");
    registry.RegisterPluginPath(fileproto + GetPlatformData().coreRootPath + "plugins/");
    if (!GetPlatformData().appPluginPath.empty()) {
        registry.RegisterPluginPath(fileproto + GetPlatformData().appPluginPath);
    }
}

CORE_NS::IPlatform::Ptr Platform::Create(PlatformCreateInfo const& createInfo)
{
    return CORE_NS::IPlatform::Ptr(new PlatformLinux(createInfo));
}

CORE_END_NAMESPACE()
