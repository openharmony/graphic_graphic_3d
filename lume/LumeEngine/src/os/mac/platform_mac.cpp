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

#include "platform_mac.h"

#include <CoreFoundation/CFBundle.h>

#include <core/io/intf_file_manager.h>
#include <core/namespace.h>

#include "io/path_tools.h"
#include "os/platform.h"
#include "util/string_util.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::string;
using BASE_NS::string_view;

PlatformMac::PlatformMac(PlatformCreateInfo const& createInfo)
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

        string normalizedPath;
        if (pathIn.empty()) {
            normalizedPath = curPath;
        } else {
            if (pathIn[0] != '/') {
                // relative path.
                normalizedPath = NormalizePath(curPath + pathIn);
            } else {
                normalizedPath = NormalizePath(pathIn);
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

const PlatformData& PlatformMac::GetPlatformData() const
{
    return plat_;
}

void PlatformMac::RegisterPluginLocations(IPluginRegister& registry)
{
    const BASE_NS::string fileproto("file://");
    BASE_NS::string pluginDirectory;
    CFBundleRef main_bundle = CFBundleGetMainBundle();
    if (main_bundle != NULL) {
        CFURLRef bundle_url = CFBundleCopyBundleURL(main_bundle);
        if (bundle_url != NULL) {
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
            char raw_dir[MAXPATHLEN];
            if (CFURLGetFileSystemRepresentation(bundle_url, true, (UInt8*)raw_dir, MAXPATHLEN)) {
                pluginDirectory = fileproto + BASE_NS::string(raw_dir) + "/Contents/PlugIns/";
            }
            CFRelease(bundle_url);
        }
    }

    registry.RegisterPluginPath(pluginDirectory);
    if (!GetPlatformData().appPluginPath.empty()) {
        registry.RegisterPluginPath(fileproto + GetPlatformData().appPluginPath);
    }
}

CORE_NS::IPlatform::Ptr Platform::Create(PlatformCreateInfo const& createInfo)
{
    return CORE_NS::IPlatform::Ptr(new PlatformMac(createInfo));
}

CORE_END_NAMESPACE()
