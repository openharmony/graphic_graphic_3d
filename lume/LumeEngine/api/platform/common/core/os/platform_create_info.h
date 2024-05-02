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

#ifndef API_CORE_OS_COMMON_PLATFORM_CREATE_INFO_H
#define API_CORE_OS_COMMON_PLATFORM_CREATE_INFO_H

#include <base/containers/string.h>
#include <base/namespace.h>
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
