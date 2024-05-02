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

#ifndef API_CORE_ENGINEINFO_H
#define API_CORE_ENGINEINFO_H

#include <cstdint>

#include <base/containers/string.h>
#include <base/namespace.h>
#include <core/namespace.h>
#include <core/os/platform_create_info.h>

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
