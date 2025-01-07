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

#ifndef API_CORE_PERF_INTF_PERFORMANCE_TRACE_H
#define API_CORE_PERF_INTF_PERFORMANCE_TRACE_H

#include <cstdint>

#include <base/containers/refcnt_ptr.h>
#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <core/perf/intf_performance_data_manager.h>
#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_plugin.h>

CORE_BEGIN_NAMESPACE()
/** IPerformanceTrace for events.
 * Internally synchronized.
 */
class IPerformanceTrace : public IInterface {
public:
    using Ptr = BASE_NS::refcnt_ptr<IPerformanceTrace>;
    static constexpr auto UID = BASE_NS::Uid { "18fc4522-29a3-4887-a2cf-0e170587edf9" };
    static constexpr auto TRACY_UID = BASE_NS::Uid { "1FC3A1DE-A352-4A7C-A2E0-C1FE208DABD4" };
    static constexpr auto ATRACE_UID = BASE_NS::Uid { "392C7588-86D5-47B9-963F-96412E439B9F" };

    IPerformanceTrace(const IPerformanceTrace&) = delete;
    IPerformanceTrace& operator=(const IPerformanceTrace&) = delete;

    /** Indicates start of an event.
     * @param location Details where the event was triggered.
     * @param text Optional label for the event.
     * @return Event tag which is passed to EndEvent.
     */
    virtual uintptr_t BeginEvent(const IPerformanceDataManager::Event& location, BASE_NS::string_view text) = 0;

    /** Indicates end of an event.
     * @param eventTag Event tag from BeginEvent.
     */
    virtual void EndEvent(uintptr_t eventTag) = 0;
    virtual void Message(const char* message, int callstack) = 0;
    virtual void Message(const char* message, size_t len, int callstack) = 0;
    virtual void AppInfo(const char* name, size_t len) = 0;
    virtual void Plot(const char* name, int64_t value) = 0;
    virtual void FrameBegin(const char* name) = 0;
    virtual void FrameEnd(const char* name) = 0;
    virtual void MemAllocNamed(const void* ptr, size_t size, bool secure, const char* name) = 0;
    virtual void MemFreeNamed(const void* ptr, bool secure, const char* name) = 0;

protected:
    IPerformanceTrace() = default;
    virtual ~IPerformanceTrace() = default;
};

inline constexpr BASE_NS::string_view GetName(const IPerformanceTrace*)
{
    return "IPerformanceTrace";
}

/** Information needed from the plugin for managing PerformanceTrace instances. */
struct PerformanceTraceTypeInfo : public ITypeInfo {
    /** TypeInfo UID for performance trace type info. */
    static constexpr BASE_NS::Uid UID { "fafc3bbb-b176-475c-b9bc-dd77cbd9e317" };

    using CreateLoaderFn = IPerformanceTrace::Ptr (*)(PluginToken);
    /* Token passed to creation (e.g. plugin specific data). */
    const PluginToken token;
    /** Unique ID of the performance trace implementation. */
    const BASE_NS::Uid uid;
    /** Pointer to function which is used to create performance trace instances. */
    const CreateLoaderFn createLoader;
};
CORE_END_NAMESPACE()

#endif // API_CORE_PERF_INTF_PERFORMANCE_TRACE_H
