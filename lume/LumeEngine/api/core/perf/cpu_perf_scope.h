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
#ifndef API_CORE_PERF_CPU_PERF_SCOPE_H
#define API_CORE_PERF_CPU_PERF_SCOPE_H

#if (CORE_PERF_ENABLED == 1)

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <core/implementation_uids.h>
#include <core/namespace.h>
#include <core/perf/intf_performance_data_manager.h>
#include <core/plugin/intf_class_register.h>

CORE_BEGIN_NAMESPACE()
class CpuPerfScope final {
public:
    inline CpuPerfScope(
        const BASE_NS::string_view category, const BASE_NS::string_view subCategory, const BASE_NS::string_view name);
    inline ~CpuPerfScope();
    inline void Stop();

protected:
    IPerformanceDataManager* manager_ { nullptr };
    IPerformanceDataManager::TimerHandle timerName_;
    BASE_NS::string subCategory_;
    BASE_NS::string name_;
};

inline CpuPerfScope::CpuPerfScope(
    const BASE_NS::string_view category, const BASE_NS::string_view subCategory, const BASE_NS::string_view name)
    : subCategory_(subCategory), name_(name)
{
    if (auto* inst = GetInstance<IPerformanceDataManagerFactory>(UID_PERFORMANCE_FACTORY); inst) {
        manager_ = inst->Get(category);
    }
    if (manager_) {
        timerName_ = manager_->BeginTimer();
    }
}

inline CpuPerfScope::~CpuPerfScope()
{
    Stop();
}

inline void CpuPerfScope::Stop()
{
    if (manager_) {
        manager_->UpdateData(subCategory_, name_, manager_->EndTimer(timerName_));
        manager_ = nullptr;
    }
}
CORE_END_NAMESPACE()

// Helper to concatenate macro values.
#define CORE_CONCAT_NOEXP(value0, value1) value0##value1
#define CORE_CONCAT(value0, value1) CORE_CONCAT_NOEXP(value0, value1)

#define CORE_CPU_PERF_BEGIN(timerName, category, subCategory, name) \
    CORE_NS::CpuPerfScope timerName(category, subCategory, name);
#define CORE_CPU_PERF_END(timerName) timerName.Stop();
#define CORE_CPU_PERF_SCOPE(category, subCategory, name) \
    CORE_NS::CpuPerfScope CORE_CONCAT(cpuPerfScope_, __LINE__)(category, subCategory, name)

#else
#define CORE_CPU_PERF_BEGIN(timerName, category, subCategory, name)
#define CORE_CPU_PERF_END(timerName)
#define CORE_CPU_PERF_SCOPE(category, subCategory, name)
#endif

#endif // API_CORE_PERF_CPU_PERF_SCOPE_H
