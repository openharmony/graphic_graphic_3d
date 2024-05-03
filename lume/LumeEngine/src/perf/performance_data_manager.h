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

#ifndef CORE__PERF__PERFORMANCE_DATA_MANAGER_H
#define CORE__PERF__PERFORMANCE_DATA_MANAGER_H

#include <cstdint>
#include <map>
#include <mutex>

#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/namespace.h>
#include <core/perf/intf_performance_data_manager.h>

CORE_BEGIN_NAMESPACE()
// if CORE_DEV_ENABLED defined the manager methods are empty

/** PerformanceDataManager.
 * Internally synchronized global singleton for timings.
 */
class PerformanceDataManager final : public IPerformanceDataManager {
public:
    struct InternalData;
    using TypeDataSet = std::map<BASE_NS::fixed_string<TIMING_DATA_NAME_LENGTH>, InternalData>;

    ~PerformanceDataManager();
    explicit PerformanceDataManager(const BASE_NS::string_view category);

    PerformanceDataManager(const PerformanceDataManager&) = delete;
    PerformanceDataManager& operator=(const PerformanceDataManager&) = delete;

    BASE_NS::string_view GetCategory() const override;

    TimerHandle BeginTimer() override;
    int64_t EndTimer(TimerHandle handle) override;

    void UpdateData(
        const BASE_NS::string_view subCategory, const BASE_NS::string_view name, const int64_t microSeconds) override;
    void ResetData() override;
    BASE_NS::vector<PerformanceData> GetData() const override;

    void DumpToLog() const;

    void RemoveData(const BASE_NS::string_view subCategory) override;

    // IInterface
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

private:
    const BASE_NS::string category_;

#if (CORE_PERF_ENABLED == 1)
    mutable std::mutex dataMutex_;

    TypeDataSet data_;
#endif
};

class PerformanceDataManagerFactory final : public IPerformanceDataManagerFactory {
public:
    IPerformanceDataManager* Get(const BASE_NS::string_view category) override;
    BASE_NS::vector<IPerformanceDataManager*> GetAllCategories() const override;

    // IInterface
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

private:
#if (CORE_PERF_ENABLED == 1)
    mutable std::mutex mutex_;
    BASE_NS::unordered_map<BASE_NS::string, BASE_NS::unique_ptr<PerformanceDataManager>> managers_;
#endif
};
CORE_END_NAMESPACE()

#endif // CORE__UTIL__PERFORMANCE_DATA_MANAGER_H
