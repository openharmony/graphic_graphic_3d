/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_OS_OHOS_PLATFORM_CREATE_INFO_H
#define API_CORE_OS_OHOS_PLATFORM_CREATE_INFO_H

#ifdef __OHOS_PLATFORM__

#include <base/containers/string.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()

/** Platform create info */
struct PlatformCreateInfo {
    /** Core root path */
    BASE_NS::string coreRootPath = "./";
    /** Application root path */
    BASE_NS::string appRootPath = "./";
    /** Application plugin path */
    BASE_NS::string appPluginPath = "./";

    /**HapInfo*/
    BASE_NS::string hapPath = "./";
    BASE_NS::string bundleName = "./";
    BASE_NS::string moduleName = "./";
};
CORE_END_NAMESPACE()

#endif // __OHOS_PLATFORM__

#endif // API_CORE_OS_OHOS_PLATFORM_CREATE_INFO_H
