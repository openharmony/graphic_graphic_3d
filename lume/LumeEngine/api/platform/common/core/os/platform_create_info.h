/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_OS_COMMON_PLATFORM_CREATE_INFO_H
#define API_CORE_OS_COMMON_PLATFORM_CREATE_INFO_H

#include <base/containers/string.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
/** @ingroup group_os_windows_platformcreateinfo */
/** Platform create info */
struct PlatformCreateInfo {
    /** Core root path */
    BASE_NS::string coreRootPath = "./";
    /** Application root path */
    BASE_NS::string appRootPath = "./";
    /** Application plugin path */
    BASE_NS::string appPluginPath = "./";
};
CORE_END_NAMESPACE()

#endif // API_CORE_OS_COMMON_PLATFORM_CREATE_INFO_H
