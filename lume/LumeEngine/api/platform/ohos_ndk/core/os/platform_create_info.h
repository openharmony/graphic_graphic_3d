/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef API_CORE_OS_OHOS_PLATFORM_CREATE_INFO_H
#define API_CORE_OS_OHOS_PLATFORM_CREATE_INFO_H

#ifdef __OHOS__

#include <napi/native_api.h>
#include <platform/common/core/os/extensions_create_info.h>
#include <rawfile/raw_file_manager.h>

#include <base/containers/string.h>
#include <core/namespace.h>
CORE_BEGIN_NAMESPACE()

/** Platform create info */
struct PlatformCreateInfo {
    /* module env */
    napi_env jsEnv;
    /* Context */
    napi_value jsObject;
    /** Extra extensions */
    const PlatformCreateExtensionInfo* extensions = nullptr;

    // the following can actually be fetched from the 'jsObject'  (and jsObject can be fetched from the jsobject
    // "global".) but due to threading limitations, napi_* methods can only be called from single jsthread. so simply
    // let the app provide these.
    NativeResourceManager* resMan;
    BASE_NS::string appPluginPath;
};
CORE_END_NAMESPACE()

#endif  // __OHOS__

#endif  // API_CORE_OS_OHOS_PLATFORM_CREATE_INFO_H
