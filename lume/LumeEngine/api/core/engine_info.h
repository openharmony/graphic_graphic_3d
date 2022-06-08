/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_ENGINEINFO_H
#define API_CORE_ENGINEINFO_H

#include <cstdint>

#include <base/containers/string.h>
#include <core/namespace.h>
#include <core/os/intf_platform.h>

CORE_BEGIN_NAMESPACE()
/** \addtogroup group_engineinfo
 *  @{
 */
/** Version info */
struct VersionInfo {
    /** Name, default is "no_name" */
    BASE_NS::string name { "no_name" };
    /** Major number of version */
    uint32_t versionMajor { 0 };
    /** Minor number of version */
    uint32_t versionMinor { 0 };
    /** Patch number of version */
    uint32_t versionPatch { 0 };
};

/** Context info */
struct ContextInfo {};

/** Engine create info */
struct EngineCreateInfo {
    /** Platform */
    PlatformCreateInfo platformCreateInfo;
    /** Application version */
    VersionInfo applicationVersion;
    /** Application context */
    ContextInfo applicationContext;
};
/** @} */
CORE_END_NAMESPACE()

#endif // API_CORE_ENGINEINFO_H
