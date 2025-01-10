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

#ifndef API_CORE_UTIL_PARALLEL_SORT_H
#define API_CORE_UTIL_PARALLEL_SORT_H

#include <algorithm>
#include <iterator>

#include <core/threading/intf_thread_pool.h>

CORE_BEGIN_NAMESPACE()

// Helper class for running lambda as a ThreadPool task.
template<typename Fn>
class FunctionTask final : public IThreadPool::ITask {
public:
    explicit FunctionTask(Fn&& func) : func_(BASE_NS::move(func)) {};

    void operator()() override
    {
        func_();
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    Fn func_;
};

template<typename Fn>
inline IThreadPool::ITask::Ptr CreateFunctionTask(Fn&& func)
{
    return IThreadPool::ITask::Ptr { new FunctionTask<Fn>(BASE_NS::move(func)) };
}

template<class RandomIt, class Compare>
void ParallelSort(RandomIt first, RandomIt last, Compare comp, IThreadPool* threadPool)
{
    const auto totalSize = std::distance(first, last);
    if (totalSize <= 1) {
        return;
    }

    const auto numThreads = std::max(threadPool->GetNumberOfThreads(), 1u);
    const auto partSize = (totalSize + numThreads - 1) / numThreads;

    BASE_NS::vector<std::pair<RandomIt, RandomIt>> partitions;

    // partition create
    RandomIt partStart = first;
    for (size_t ii = 0; ii < numThreads && partStart != last; ii++) {
        RandomIt partEnd = partStart;
        if (std::distance(partStart, last) > partSize) {
            std::advance(partEnd, partSize);
        } else {
            partEnd = last;
        }

        partitions.push_back({ partStart, partEnd });
        partStart = partEnd;
    }

    // partition parallel sort
    BASE_NS::vector<IThreadPool::IResult::Ptr> sortResults;

    for (const auto& part : partitions) {
        auto task = CreateFunctionTask([part, &comp]() { std::sort(part.first, part.second, comp); });
        auto result = threadPool->Push(BASE_NS::move(task));
        sortResults.push_back(BASE_NS::move(result));
    }

    // sort task completion wait
    for (auto& result : sortResults) {
        result->Wait();
    }

    // partition merge
    while (partitions.size() > 1) {
        BASE_NS::vector<IThreadPool::IResult::Ptr> mergeResults;
        BASE_NS::vector<std::pair<RandomIt, RandomIt>> newPartitions;

        for (size_t ii = 0; ii + 1 < partitions.size(); ii += 2) {  // 2: step size
            const auto begin1 = partitions[ii].first;
            const auto end1 = partitions[ii].second;
            const auto begin2 = partitions[ii + 1].first;
            const auto end2 = partitions[ii + 1].second;

            auto task =
                CreateFunctionTask([begin1, end1, end2, &comp]() { std::inplace_merge(begin1, end1, end2, comp); });
            auto result = threadPool->Push(BASE_NS::move(task));
            mergeResults.push_back(BASE_NS::move(result));

            newPartitions.push_back({ begin1, end2 });
        }

        if (partitions.size() % 2 == 1) { // 2: for partition
            newPartitions.push_back(partitions.back());
        }

        // merge task completion wait
        for (auto& result : mergeResults) {
            result->Wait();
        }

        partitions = BASE_NS::move(newPartitions);
    }
}

template<class RandomIt>
void ParallelSort(RandomIt first, RandomIt last, IThreadPool* threadPool)
{
    using ValueType = typename std::iterator_traits<RandomIt>::value_type;
    ParallelSort(first, last, std::less<ValueType>(), threadPool);
}

CORE_END_NAMESPACE()

#endif
