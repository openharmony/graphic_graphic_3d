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

#ifndef CORE_OS_WINDOWS_PLATFORM_WINDOWS_H
#define CORE_OS_WINDOWS_PLATFORM_WINDOWS_H

#include <base/containers/string.h>
#include <base/namespace.h>
#include <core/namespace.h>
#include <core/os/intf_platform.h>

CORE_BEGIN_NAMESPACE()
class IFileManager;
class IPluginRegister;
struct PlatformCreateInfo;

struct PlatformData {
    BASE_NS::string coreRootPath;
    BASE_NS::string appRootPath;
    BASE_NS::string appPluginPath;
};

/** Interface for platform-specific functions. */
class PlatformWindows final : public IPlatform {
public:
    explicit PlatformWindows(PlatformCreateInfo const& createInfo);
    ~PlatformWindows() override = default;

    const PlatformData& GetPlatformData() const override;
    void RegisterPluginLocations(IPluginRegister& registry) override;

private:
    PlatformData plat_;
};
CORE_END_NAMESPACE()

#endif // CORE_OS_WINDOWS_PLATFORM_WINDOWS_H
