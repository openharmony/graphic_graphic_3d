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

#ifndef API_CORE_OS_COMMON_EXTENSIONS_CREATE_INFO_H
#define API_CORE_OS_COMMON_EXTENSIONS_CREATE_INFO_H

#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#if __OHOS_PLATFORM__
// Currently not implemented on OHOS
#else
#include <core/os/platform_trace_info.h>
#endif
CORE_BEGIN_NAMESPACE()
constexpr const int32_t PLATFORM_EXTENSION_UNDEFINED = 0;
constexpr const int32_t PLATFORM_EXTENSION_TRACE_USER = 1;
constexpr const int32_t PLATFORM_EXTENSION_TRACE_EXTENSION = 2;

class IPerformanceTrace;

struct PlatformCreateExtensionInfo {
    PlatformCreateExtensionInfo* next { nullptr };
    int32_t type { PLATFORM_EXTENSION_UNDEFINED };
};

#if __OHOS_PLATFORM__
// Currently not implemented on OHOS
#else
/** could be used to expose settings for a user-defined tracing api */
struct PlatformTraceInfoUsr : public PlatformCreateExtensionInfo {
    IPerformanceTrace* tracer;
};

/** could be used to expose settings from an standard plugin */
struct PlatformTraceInfoExt : public PlatformCreateExtensionInfo {
    BASE_NS::Uid plugin { "6151083a-d86f-4037-9059-97f8d0616161" };
    BASE_NS::Uid type { GetDefaultTrace() };
};
#endif
CORE_END_NAMESPACE()

#endif // API_CORE_OS_COMMON_PLATFORM_CREATE_INFO_H
