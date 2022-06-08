/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_OS_WINDOWS_PLATFORM_WINDOWS_H
#define CORE_OS_WINDOWS_PLATFORM_WINDOWS_H


#include <base/containers/string.h>
#include <core/namespace.h>
#include <core/os/intf_platform.h>

CORE_BEGIN_NAMESPACE()
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
    BASE_NS::string RegisterDefaultPaths(IFileManager& fileManage) override;
    void RegisterPluginLocations(IPluginRegister& registry) override;

private:
    PlatformData plat_;
};
CORE_END_NAMESPACE()


#endif // CORE_OS_WINDOWS_PLATFORM_WINDOWS_H
