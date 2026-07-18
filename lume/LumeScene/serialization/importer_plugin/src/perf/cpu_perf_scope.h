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
#ifndef SCENE_IMP_PERF_CPU_PERF_SCOPE_H
#define SCENE_IMP_PERF_CPU_PERF_SCOPE_H

#include <scene_importer/base/namespace.h>

#include <core/perf/cpu_perf_scope.h>

#if defined(CORE_PERF_ENABLED) && (CORE_PERF_ENABLED == 1)
CORE_BEGIN_NAMESPACE()
template <>
struct PerformanceTraceSubsystem<4> {
    static constexpr bool IsEnabled()
    {
        return true;
    }
};
CORE_END_NAMESPACE()

SCENE_IMP_BEGIN_NAMESPACE()
using PROFILER_SUBSYSTEM_SCENE_IMP = CORE_NS::PerformanceTraceSubsystem<4>;
SCENE_IMP_END_NAMESPACE()
#endif

constexpr uint32_t SCENE_IMP_PROFILER_DEFAULT_COLOR{0x60c060};
#define SCENE_IMP_CPU_PERF_BEGIN(timerName, subCategory, name, ...) \
    CORE_PROFILER_PERF_BEGIN(timerName,                             \
        "SCENE_IMP",                                                \
        subCategory,                                                \
        name,                                                       \
        SCENE_IMP_PROFILER_DEFAULT_COLOR,                           \
        SCENE_IMP_NS::PROFILER_SUBSYSTEM_SCENE_IMP,                 \
        ##__VA_ARGS__)
#define SCENE_IMP_CPU_PERF_END(timerName) CORE_PROFILER_PERF_END(timerName)
#define SCENE_IMP_CPU_PERF_SCOPE(subCategory, name, ...) \
    CORE_PROFILER_PERF_SCOPE("SCENE_IMP",                \
        subCategory,                                     \
        name,                                            \
        SCENE_IMP_PROFILER_DEFAULT_COLOR,                \
        SCENE_IMP_NS::PROFILER_SUBSYSTEM_SCENE_IMP,      \
        ##__VA_ARGS__)

#endif
