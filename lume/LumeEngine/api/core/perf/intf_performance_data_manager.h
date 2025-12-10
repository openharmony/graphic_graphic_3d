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

#ifndef API_CORE_PERF_INTF_PERFORMANCE_DATA_MANAGER_H
#define API_CORE_PERF_INTF_PERFORMANCE_DATA_MANAGER_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>
#include <base/containers/pair.h>
#include <base/containers/refcnt_ptr.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>

CORE_BEGIN_NAMESPACE()
/** PerformanceDataManager for timings.
 * Internally synchronized.
 */
class IPerformanceTrace;
class IPerformanceDataManager : public IInterface {
public:
    using Ptr = BASE_NS::refcnt_ptr<IPerformanceDataManager>;
    static constexpr auto UID = BASE_NS::Uid { "aedc8e81-10ce-4c77-8fde-c238100a36ec" };

    static constexpr uint32_t TIMING_DATA_POOL_SIZE { 64u };
    static constexpr uint32_t TIMING_DATA_NAME_LENGTH { 64u };

    struct PerformanceTimingData {
        enum class DataType : uint8_t {
            /** Time in microseconds. */
            MICROSECONDS,
            /** Bytes, e.g. memory. */
            BYTES,
            /** Number of items, e.g. triangle count. */
            COUNT,
        };

        /** Latest value. */
        int64_t currentTime { 0 };
        /** Largest value. */
        int64_t maxTime { 0 };
        /** Smallest value. */
        int64_t minTime { 0 };
        /** Average of the last TIMING_DATA_POOL_SIZE values. */
        int64_t averageTime { 0 };
        /** Sum of the valus. */
        int64_t totalTime { 0 };
        /** Max sum of total time values. (Might be used with negative values) */
        int64_t maxTotalTime { 0 };
        /** Number of times the data has been updated. */
        int64_t counter { 0 };
        /** Array containing the last TIMING_DATA_POOL_SIZE values. */
        int64_t timings[TIMING_DATA_POOL_SIZE] { 0 };
        /** Array containing the last TIMING_DATA_POOL_SIZE average values. */
        int64_t averageTimings[TIMING_DATA_POOL_SIZE] { 0 };
        /** Type of the values. */
        DataType type { DataType::MICROSECONDS };
    };

    using NameToPerformanceMap =
        BASE_NS::unordered_map<BASE_NS::fixed_string<TIMING_DATA_NAME_LENGTH>, PerformanceTimingData>;

    struct PerformanceData {
        /** Name of the subcategory. */
        BASE_NS::fixed_string<TIMING_DATA_NAME_LENGTH> subCategory;
        /** Data so far gathered for the subcatagory. */
        NameToPerformanceMap timings;
    };

    IPerformanceDataManager(const IPerformanceDataManager&) = delete;
    IPerformanceDataManager& operator=(const IPerformanceDataManager&) = delete;

    /** Returns the category to which this performance data belongs to.
     * @return Name of the category given when calling IPerformanceDataManagerFactory::Get.
     */
    virtual BASE_NS::string_view GetCategory() const = 0;

    using TimerHandle = int64_t;

    /** Starts measuring time.
     * @return Handle which is used to stop the measurement.
     */
    virtual TimerHandle BeginTimer() = 0;

    /** Stops measuring time.
     * @param handle A handle previously returned from BeginTimer.
     * @return Time elapsed between Begin/EndTimer calls in microseconds.
     */
    virtual int64_t EndTimer(TimerHandle handle) = 0;

    /** Updates performance timing data.
     * @param subCategory Name of the subcategory.
     * @param name Name of the data entry.
     * @param microSeconds Time in microseconds.
     */
    virtual void UpdateData(
        const BASE_NS::string_view subCategory, const BASE_NS::string_view name, const int64_t microSeconds) = 0;

    /** Updates performance timing data.
     * @param subCategory Name of the subcategory.
     * @param name Name of the data entry.
     * @param value Current value.
     * @param type Type of the current value.
     */
    virtual void UpdateData(const BASE_NS::string_view subCategory, const BASE_NS::string_view name,
        const int64_t value, PerformanceTimingData::DataType type) = 0;

    /** Resets all performance data gathered for this category. */
    virtual void ResetData() = 0;

    /** Remove subcategory data.
     * @param subCategory A sub category to be removed.
     */
    virtual void RemoveData(const BASE_NS::string_view subCategory) = 0;

    /** Returns the performance data gathered for this category.
     * @return A list contains a name-timings lookuptable for each subcategory.
     */
    virtual BASE_NS::vector<PerformanceData> GetData() const = 0;

    /***/
    struct Event {
        /***/
        const char* name;
        /** Funtion name. */
        const char* function;
        /** Source file. */
        const char* file;
        /** Line number in source file. */
        uint32_t line;
        /** Event color. 0xRRGGBB */
        uint32_t color;
    };

    using CounterPair = BASE_NS::pair<BASE_NS::string_view, int64_t>;
    using CounterPairView = BASE_NS::array_view<CounterPair>;
    using ConstCounterPairView = BASE_NS::array_view<const CounterPair>;
    /** NOTE: string */
    using CounterPairVector = BASE_NS::vector<BASE_NS::pair<BASE_NS::string, int64_t>>;

    /** Get requested counters. By default gets the current.
     * Heavy method, do not call every frame, only when collecting data.
     * @param A given data set with pair< name, value>.
     */
    virtual void GetSelectedCounters(CounterPairView data) const = 0;

    struct ComparisonData {
        /** Zero means valid */
        uint32_t flags { 0U };
        /** Listed error counters with lhs - rhs value
         * The vector is empty when the flags are zero
         */
        CounterPairVector errorCounters;
    };
    /** Get requested counters. By default gets the current.
     * Heavy method, do not call every frame, only when collecting data.
     * @param A given data set with pair< name, value>.
     */
    virtual ComparisonData CompareCounters(ConstCounterPairView lhs, ConstCounterPairView rhs) const = 0;

protected:
    IPerformanceDataManager() = default;
    virtual ~IPerformanceDataManager() = default;
};

inline constexpr BASE_NS::string_view GetName(const IPerformanceDataManager*)
{
    return "IPerformanceDataManager";
}

class IPerformanceDataManagerFactory : public IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid { "58d60acd-a2d2-4f76-a9bb-5cf0a82ccd4f" };

    /** Get a performance data manager instance for a named category. The category can be any freeformed string.
     * @param category Name of the category.
     * @return Pointer to the instance grouping statistics for the given category.
     */
    virtual IPerformanceDataManager* Get(const BASE_NS::string_view category) = 0;

    /** Get performance data managers for all categories.
     * @return Array of instances.
     */
    virtual BASE_NS::vector<IPerformanceDataManager*> GetAllCategories() const = 0;

    virtual IPerformanceTrace* GetFirstPerformanceTrace() const = 0;
};

inline constexpr BASE_NS::string_view GetName(const IPerformanceDataManagerFactory*)
{
    return "IPerformanceDataManagerFactory";
}
CORE_END_NAMESPACE()

#endif // API_CORE_PERF_INTF_PERFORMANCE_DATA_MANAGER_H
