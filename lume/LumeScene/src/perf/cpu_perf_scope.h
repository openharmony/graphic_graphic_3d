/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef SCENE_PERF_CPU_PERF_SCOPE_H
#define SCENE_PERF_CPU_PERF_SCOPE_H

#include <scene/base/namespace.h>

#include <core/perf/cpu_perf_scope.h>

#if defined(CORE_PERF_ENABLED) && (CORE_PERF_ENABLED == 1)
CORE_BEGIN_NAMESPACE()
template <>
struct PerformanceTraceSubsystem<3> {
    static constexpr bool IsEnabled()
    {
        return true;
    }
};
CORE_END_NAMESPACE()

SCENE_BEGIN_NAMESPACE()
using PROFILER_SUBSYSTEM_SCENE = CORE_NS::PerformanceTraceSubsystem<3>;
SCENE_END_NAMESPACE()
#endif

constexpr uint32_t SCENE_PROFILER_DEFAULT_COLOR{0x00a0ff};
#define SCENE_CPU_PERF_BEGIN(timerName, subCategory, name, ...) \
    CORE_PROFILER_PERF_BEGIN(timerName,                         \
        "SCENE",                                                \
        subCategory,                                            \
        name,                                                   \
        SCENE_PROFILER_DEFAULT_COLOR,                           \
        SCENE_NS::PROFILER_SUBSYSTEM_SCENE,                     \
        ##__VA_ARGS__)
#define SCENE_CPU_PERF_END(timerName) CORE_PROFILER_PERF_END(timerName)
#define SCENE_CPU_PERF_SCOPE(subCategory, name, ...) \
    CORE_PROFILER_PERF_SCOPE(                        \
        "SCENE", subCategory, name, SCENE_PROFILER_DEFAULT_COLOR, SCENE_NS::PROFILER_SUBSYSTEM_SCENE, ##__VA_ARGS__)

#endif
