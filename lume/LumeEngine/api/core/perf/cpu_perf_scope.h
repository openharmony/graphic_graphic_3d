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
#ifndef API_CORE_PERF_CPU_PERF_SCOPE_H
#define API_CORE_PERF_CPU_PERF_SCOPE_H

#if (CORE_PERF_ENABLED == 1)

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <core/implementation_uids.h>
#include <core/namespace.h>
#include <core/perf/intf_performance_data_manager.h>
#include <core/perf/intf_performance_trace.h>
#include <core/plugin/intf_class_register.h>

CORE_BEGIN_NAMESPACE()

// Placeholder interface for disabled paths, disabled path uses static typing so template deducation so basic compiler
// can strip the calls.
class NullIPerformanceTrace {
    inline constexpr uintptr_t BeginEvent(const IPerformanceDataManager::Event& location, BASE_NS::string_view text)
    {
        return 0;
    }
    inline constexpr void EndEvent(uintptr_t eventTag) {}
    inline constexpr void Message(const char* message, int callstack) {}
    inline constexpr void Message(const char* message, size_t len, int callstack) {}
    inline constexpr void AppInfo(const char* name, size_t len) {}
    inline constexpr void Plot(const char* name, int64_t value) {}
    inline constexpr void FrameBegin(const char* name) {}
    inline constexpr void FrameEnd(const char* name) {}
    inline constexpr void MemAllocNamed(const void* ptr, size_t size, bool secure, const char* name) {}
    inline constexpr void MemFreeNamed(const void* ptr, bool secure, const char* name) {}
};

// Setups two code paths, one enabled and one disabled
constexpr int PROFILER_ENABLED = 1;
constexpr int PROFILER_DISABLED = 0;

constexpr uint32_t CORE_PROFILER_DEFAULT_COLOR { 0xff0000 };

// Use template specialisation to handle two code paths for enabled and disabled
template<int>
inline auto* GetTracer();

template<>
inline auto* GetTracer<PROFILER_ENABLED>()
{
    static IPerformanceTrace* tracer { nullptr };
    if (!tracer) {
        if (auto* inst = GetInstance<IPerformanceDataManagerFactory>(UID_PERFORMANCE_FACTORY)) {
            tracer = inst->GetFirstPerformanceTrace();
        }
    }

    return tracer;
}
template<>
inline auto* GetTracer<PROFILER_DISABLED>()
{
    static NullIPerformanceTrace tracer;
    return &tracer;
}

// Use template specialisation to handle two code paths for enabled and disabled
template<int>
class CpuPerfScopeI;

template<>
class CpuPerfScopeI<PROFILER_DISABLED> final {
public:
    inline CpuPerfScopeI(const BASE_NS::string_view category, const BASE_NS::string_view subCategory,
        const BASE_NS::string_view name, const IPerformanceDataManager::Event& location)
    {}
    inline void Stop() {}
};

template<>
class CpuPerfScopeI<PROFILER_ENABLED> final {
public:
    CpuPerfScopeI(const BASE_NS::string_view category, const BASE_NS::string_view subCategory,
        const BASE_NS::string_view name, const IPerformanceDataManager::Event& location)
        : subCategory_(subCategory), name_(name)
    {
        if (auto* inst = GetInstance<IPerformanceDataManagerFactory>(UID_PERFORMANCE_FACTORY); inst) {
            manager_ = inst->Get(category);
        }

        if (manager_) {
            timerName_ = manager_->BeginTimer();
            if (auto tracer = GetTracer<PROFILER_ENABLED>()) {
                token_ = tracer->BeginEvent(location, name);
            }
        }
    }
    ~CpuPerfScopeI()
    {
        Stop();
    }
    inline void Stop()
    {
        if (manager_) {
            if (auto tracer = GetTracer<PROFILER_ENABLED>()) {
                tracer->EndEvent(token_);
            }
            manager_->UpdateData(subCategory_, name_, manager_->EndTimer(timerName_));
            manager_ = nullptr;
        }
    }

protected:
    IPerformanceDataManager* manager_ { nullptr };
    IPerformanceDataManager::TimerHandle timerName_ {};
    uintptr_t token_ {};
    BASE_NS::string subCategory_;
    BASE_NS::string name_;
};
using CpuPerfScope = CpuPerfScopeI<PROFILER_ENABLED>;

template<int N>
struct PerformanceTraceSubsystem;

template<>
struct PerformanceTraceSubsystem<1> {
    static constexpr bool IsEnabled()
    {
        return true;
    }
};

using PROFILER_SUBSYSTEM_DEFAULT = PerformanceTraceSubsystem<1>;

template<int N>
struct PerformanceTraceSeverity {
    static constexpr bool IsEnabled()
    {
        if constexpr (N <= 1000) {
            return true;
        } else {
            return false;
        }
    }
};

using PROFILER_DEFAULT = PerformanceTraceSeverity<1000>;
using PROFILER_TRACE = PerformanceTraceSeverity<2000>;

template<int z = 0, typename x = PROFILER_SUBSYSTEM_DEFAULT, typename y = PROFILER_DEFAULT>
inline constexpr bool PerformanceTraceEnabled()
{
    constexpr bool a = x::IsEnabled() && y::IsEnabled() ? true : false;
    return a;
}

template<>
inline constexpr bool PerformanceTraceEnabled<0, void, void>()
{
    return PerformanceTraceEnabled<0, PROFILER_SUBSYSTEM_DEFAULT, PROFILER_DEFAULT>();
}

CORE_END_NAMESPACE()

// Helper to concatenate macro values.
#define CORE_CONCAT_NOEXP(value0, value1) value0##value1
#define CORE_CONCAT(value0, value1) CORE_CONCAT_NOEXP(value0, value1)

#define CORE_PROFILER_TOKEN(arg) arg
#define CORE_PROFILER_ARGS(...) CORE_NS::PerformanceTraceEnabled<0, ##__VA_ARGS__>()

#define CORE_PROFILER_PERF_BEGIN(timerName, category, subCategory, name, color, ...)                                \
    static constexpr const auto CORE_CONCAT(eventLocation, __LINE__) =                                              \
        CORE_NS::IPerformanceDataManager::Event { category "::" subCategory, __func__, __FILE__, __LINE__, color }; \
    CORE_NS::CpuPerfScopeI<CORE_PROFILER_ARGS(__VA_ARGS__)> timerName(                                              \
        category, subCategory, name, CORE_CONCAT(eventLocation, __LINE__))
#define CORE_PROFILER_PERF_END(timerName) timerName.Stop()

#define CORE_PROFILER_PERF_SCOPE(category, subCategory, name, color, ...)                                        \
    static constexpr const auto CORE_CONCAT(eventLocation, __LINE__) = CORE_NS::IPerformanceDataManager::Event { \
        category "::" subCategory,                                                                               \
        __func__,                                                                                                \
        __FILE__,                                                                                                \
        __LINE__,                                                                                                \
        color,                                                                                                   \
    };                                                                                                           \
    CORE_NS::CpuPerfScopeI<CORE_PROFILER_ARGS(__VA_ARGS__)> CORE_CONCAT(cpuPerfScope_, __LINE__)(                \
        category, subCategory, name, CORE_CONCAT(eventLocation, __LINE__))

#define CORE_PROFILER_SYMBOL(name, value) constexpr const char* name = value

#define CORE_PROFILER_APPNAME(txt, size, ...)                                  \
    if (auto tracer = CORE_NS::GetTracer<CORE_PROFILER_ARGS(__VA_ARGS__)>()) { \
        tracer->AppInfo(txt, size);                                            \
    }

#define CORE_PROFILER_MESSAGEL(msg, ...)                                       \
    if (auto tracer = CORE_NS::GetTracer<CORE_PROFILER_ARGS(__VA_ARGS__)>()) { \
        tracer->Message(msg, 0);                                               \
    }

#define CORE_PROFILER_MESSAGE(msg, size, ...)                                  \
    if (auto tracer = CORE_NS::GetTracer<CORE_PROFILER_ARGS(__VA_ARGS__)>()) { \
        tracer->Message(msg, size);                                            \
    }

#define CORE_PROFILER_PLOT(name, val, ...)                                     \
    if (auto tracer = CORE_NS::GetTracer<CORE_PROFILER_ARGS(__VA_ARGS__)>()) { \
        tracer->Plot(name, val);                                               \
    }

#define CORE_PROFILER_MARK_GLOBAL_FRAME_CHANGED(...)                           \
    if (auto tracer = CORE_NS::GetTracer<CORE_PROFILER_ARGS(__VA_ARGS__)>()) { \
        tracer->GlobalFrameChanged();                                          \
    }

#define CORE_PROFILER_MARK_FRAME_START(name, ...)                              \
    if (auto tracer = CORE_NS::GetTracer<CORE_PROFILER_ARGS(__VA_ARGS__)>()) { \
        tracer->FrameBegin(name);                                              \
    }

#define CORE_PROFILER_MARK_FRAME_END(name, ...)                                \
    if (auto tracer = CORE_NS::GetTracer<CORE_PROFILER_ARGS(__VA_ARGS__)>()) { \
        tracer->FrameEnd(name);                                                \
    }

#define CORE_PROFILER_ALLOC_N(ptr, size, name, ...)                            \
    if (auto tracer = CORE_NS::GetTracer<CORE_PROFILER_ARGS(__VA_ARGS__)>()) { \
        tracer->MemAllocNamed(ptr, size, false, name);                         \
    }

#define CORE_PROFILER_FREE_N(ptr, name, ...)                                   \
    if (auto tracer = CORE_NS::GetTracer<CORE_PROFILER_ARGS(__VA_ARGS__)>()) { \
        tracer->MemFreeNamed(ptr, false, name);                                \
    }

#define CORE_CPU_PERF_BEGIN(timerName, category, subCategory, name, color, ...) \
    CORE_PROFILER_PERF_BEGIN(                                                   \
        timerName, category, subCategory, name, color, CORE_NS::PROFILER_SUBSYSTEM_DEFAULT, ##__VA_ARGS__)
#define CORE_CPU_PERF_END(timerName) CORE_PROFILER_PERF_END(timerName)
#define CORE_CPU_PERF_SCOPE(category, subCategory, name, color, ...) \
    CORE_PROFILER_PERF_SCOPE(category, subCategory, name, color, CORE_NS::PROFILER_SUBSYSTEM_DEFAULT, ##__VA_ARGS__)
#else
#define CORE_PROFILER_PERF_BEGIN(timerName, category, subCategory, name, color, ...)
#define CORE_PROFILER_PERF_END(timerName)
#define CORE_PROFILER_PERF_SCOPE(category, subCategory, name, color, ...)
#define CORE_PROFILER_SYMBOL(name, value)
#define CORE_PROFILER_APPNAME(name, ...)
#define CORE_PROFILER_MESSAGEL(msg, ...)
#define CORE_PROFILER_MESSAGE(msg, size, ...)
#define CORE_PROFILER_PLOT(name, val, ...)
#define CORE_PROFILER_MARK_GLOBAL_FRAME_CHANGED(...)
#define CORE_PROFILER_MARK_FRAME_START(name, ...)
#define CORE_PROFILER_MARK_FRAME_END(name, ...)
#define CORE_PROFILER_ALLOC_N(ptr, size, name, ...)
#define CORE_PROFILER_FREE_N(ptr, name, ...)
#define CORE_CPU_PERF_BEGIN(timerName, category, subCategory, name, ...)
#define CORE_CPU_PERF_END(timerName)
#define CORE_CPU_PERF_SCOPE(category, subCategory, name, ...)
#endif

#endif // API_CORE_PERF_CPU_PERF_SCOPE_H
