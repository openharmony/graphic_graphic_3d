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

#ifndef CORE_PERF_PERFORMANCE_DATA_MANAGER_H
#define CORE_PERF_PERFORMANCE_DATA_MANAGER_H

#include <cstdint>
#include <mutex>

#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/intf_logger.h>
#include <core/namespace.h>
#include <core/perf/intf_performance_data_manager.h>
#include <core/perf/intf_performance_trace.h>
#include <core/plugin/intf_plugin_register.h>

CORE_BEGIN_NAMESPACE()
// if CORE_DEV_ENABLED defined the manager methods are empty
class PerformanceDataManagerFactory;
/** PerformanceDataManager.
 * Internally synchronized global singleton for timings.
 */
class PerformanceDataManager final : public IPerformanceDataManager {
public:
    struct InternalData;
    using TypeDataSet = BASE_NS::unordered_map<BASE_NS::fixed_string<TIMING_DATA_NAME_LENGTH>, InternalData>;

    ~PerformanceDataManager();
    PerformanceDataManager(const BASE_NS::string_view category, PerformanceDataManagerFactory& factory);

    PerformanceDataManager(const PerformanceDataManager&) = delete;
    PerformanceDataManager& operator=(const PerformanceDataManager&) = delete;

    BASE_NS::string_view GetCategory() const override;

    TimerHandle BeginTimer() override;
    int64_t EndTimer(TimerHandle handle) override;

    void UpdateData(
        const BASE_NS::string_view subCategory, const BASE_NS::string_view name, const int64_t microSeconds) override;
    void UpdateData(const BASE_NS::string_view subCategory, const BASE_NS::string_view name, const int64_t value,
        PerformanceTimingData::DataType type) override;

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
    PerformanceDataManagerFactory& factory_;
#if (CORE_PERF_ENABLED == 1)
    mutable std::mutex dataMutex_;

    TypeDataSet data_;
#endif
};

class PerformanceTraceLogger final : public ILogger::IOutput {
    PerformanceDataManagerFactory* factory_;
    void Write(ILogger::LogLevel logLevel, BASE_NS::string_view filename, int lineNumber,
        BASE_NS::string_view message) override;

private:
    friend class PerformanceDataManagerFactory;
    PerformanceTraceLogger(PerformanceDataManagerFactory* factory) : factory_(factory) {}
    ~PerformanceTraceLogger() override = default;
    void Destroy() override
    {
        delete this;
    }
};

class PerformanceDataManagerFactory final : public IPerformanceDataManagerFactory {
public:
    explicit PerformanceDataManagerFactory(IPluginRegister& registry);
    ~PerformanceDataManagerFactory() override;

    IPerformanceDataManager* Get(const BASE_NS::string_view category) override;
    BASE_NS::vector<IPerformanceDataManager*> GetAllCategories() const override;

    // IInterface
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;
    struct RegisteredPerformanceTrace {
        BASE_NS::Uid uid;
        IPerformanceTrace::Ptr instance;
    };

    BASE_NS::array_view<const RegisteredPerformanceTrace> GetPerformanceTraces() const;
    IPerformanceTrace* GetFirstPerformanceTrace() const override;
    void SetPerformanceTrace(const BASE_NS::Uid& uid, IPerformanceTrace::Ptr&& trace);
    void RemovePerformanceTrace(const BASE_NS::Uid& uid);
    ILogger::IOutput::Ptr GetLogger();

private:
#if (CORE_PERF_ENABLED == 1)
    mutable std::mutex mutex_;
    BASE_NS::unordered_map<BASE_NS::string, BASE_NS::unique_ptr<PerformanceDataManager>> managers_;
#endif
    BASE_NS::vector<RegisteredPerformanceTrace> perfTraces_;
};

IPerformanceTrace::Ptr CreatePerfTraceTracy(PluginToken);
CORE_END_NAMESPACE()

#endif // CORE_UTIL_PERFORMANCE_DATA_MANAGER_H
