/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_OS_OHOS_PLATFORM_OHOS_H
#define CORE_OS_OHOS_PLATFORM_OHOS_H

#ifdef __OHOS_PLATFORM__

#include <base/containers/string.h>
#include <core/namespace.h>
#include <platform/ohos/core/os/intf_platform.h>

CORE_BEGIN_NAMESPACE()
struct PlatformData {
    /** Core root path */
    BASE_NS::string coreRootPath = "./";
    /** Application root path */
    BASE_NS::string appRootPath = "./";
    /** Application plugin path */
    BASE_NS::string appPluginPath = "./";

};

/** Interface for platform-specific functions. */
class PlatformOHOS final : public IPlatform {
public:
    explicit PlatformOHOS(PlatformCreateInfo const& createInfo);
    ~PlatformOHOS() override;

    const PlatformData& GetPlatformData() const override;
    BASE_NS::string RegisterDefaultPaths(IFileManager& fileManage) override;
    void RegisterPluginLocations(IPluginRegister& registry) override;

private:
    PlatformData plat_;
};

CORE_END_NAMESPACE()

#endif

#endif // CORE_OS_OHOS_PLATFORM_OHOS_H
