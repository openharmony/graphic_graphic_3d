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
#ifndef RENDER_PERF_CPU_PERF_SCOPE_H
#define RENDER_PERF_CPU_PERF_SCOPE_H

#include <core/perf/cpu_perf_scope.h>

#if defined(CORE_PERF_ENABLED) && (CORE_PERF_ENABLED == 1)
CORE_BEGIN_NAMESPACE()
template<>
struct CORE_NS::PerformanceTraceSubsystem<2> {
    static constexpr bool IsEnabled()
    {
#if (RENDER_PERF_ENABLED == 1)
        return true;
#else
        return false;
#endif
    }
};
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
using PROFILER_SUBSYSTEM_RENDERER = CORE_NS::PerformanceTraceSubsystem<2>;
RENDER_END_NAMESPACE()
#endif

constexpr uint32_t RENDER_PROFILER_DEFAULT_COLOR { 0xff5c00 };
#define RENDER_CPU_PERF_BEGIN(timerName, subCategory, name, ...)                                    \
    CORE_PROFILER_PERF_BEGIN(timerName, "RENDER", subCategory, name, RENDER_PROFILER_DEFAULT_COLOR, \
        RENDER_NS::PROFILER_SUBSYSTEM_RENDERER, ##__VA_ARGS__)
#define RENDER_CPU_PERF_END(timerName) CORE_PROFILER_PERF_END(timerName)
#define RENDER_CPU_PERF_SCOPE(subCategory, name, ...)                                    \
    CORE_PROFILER_PERF_SCOPE("RENDER", subCategory, name, RENDER_PROFILER_DEFAULT_COLOR, \
        RENDER_NS::PROFILER_SUBSYSTEM_RENDERER, ##__VA_ARGS__)

#endif