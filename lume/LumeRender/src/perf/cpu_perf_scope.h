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
#ifndef RENDER_PERF_CPU_PERF_SCOPE_H
#define RENDER_PERF_CPU_PERF_SCOPE_H

#if (RENDER_PERF_ENABLED == 1)
#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/implementation_uids.h>
#include <core/perf/intf_performance_data_manager.h>
#include <render/namespace.h>

#include "perf/cpu_timer.h"

RENDER_BEGIN_NAMESPACE()
class CpuPerfScopeInternal final {
public:
    inline CpuPerfScopeInternal(
        const BASE_NS::string_view category, const BASE_NS::string_view subCategory, const BASE_NS::string_view name);
    inline ~CpuPerfScopeInternal();

    inline void Stop();

private:
    CORE_NS::IPerformanceDataManager* manager_ { nullptr };
    CORE_NS::IPerformanceDataManager::TimerHandle timerName_;
    BASE_NS::string subCategory_;
    BASE_NS::string name_;
};

inline CpuPerfScopeInternal::CpuPerfScopeInternal(
    const BASE_NS::string_view category, const BASE_NS::string_view subCategory, const BASE_NS::string_view name)
    : subCategory_(subCategory), name_(name)
{
    if (auto* inst = CORE_NS::GetInstance<CORE_NS::IPerformanceDataManagerFactory>(CORE_NS::UID_PERFORMANCE_FACTORY);
        inst) {
        manager_ = inst->Get(category);
    }
    if (manager_) {
        timerName_ = manager_->BeginTimer();
    }
}

inline CpuPerfScopeInternal::~CpuPerfScopeInternal()
{
    Stop();
}

inline void CpuPerfScopeInternal::Stop()
{
    if (manager_) {
        manager_->UpdateData(subCategory_, name_, manager_->EndTimer(timerName_));
        manager_ = nullptr;
    }
}
RENDER_END_NAMESPACE()

// Helper to concatenate macro values.
#define RENDER_CONCAT_NOEXP(value0, value1) value0##value1
#define RENDER_CONCAT(value0, value1) RENDER_CONCAT_NOEXP(value0, value1)

#define RENDER_CPU_PERF_BEGIN(timerName, category, subCategory, name) \
    RENDER_NS::CpuPerfScopeInternal timerName(category, subCategory, name);
#define RENDER_CPU_PERF_END(timerName) timerName.Stop();
#define RENDER_CPU_PERF_SCOPE(category, subCategory, name) \
    RENDER_NS::CpuPerfScopeInternal RENDER_CONCAT(cpuPerfScope_, __LINE__)(category, subCategory, name)

#else
#define RENDER_CPU_PERF_BEGIN(timerName, category, subCategory, name)
#define RENDER_CPU_PERF_END(timerName)
#define RENDER_CPU_PERF_SCOPE(category, subCategory, name)
#endif

#endif // RENDER_PERF_CPU_PERF_SCOPE_H
