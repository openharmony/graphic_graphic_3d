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

#include <ComponentTools/base_manager.h>

#include <base/containers/vector.h>
#include <core/util/parallel_sort.h>

#include "test_framework.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;

/**
 * @tc.name: validateParallelSort
 * @tc.desc: Tests for Validate Parallel Sort. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilParallelSortTest, validateParallelSort, testing::ext::TestSize.Level1)
{
    IEngine::Ptr engine = UTest::CreateEngine();
    IEcs::Ptr ecs = engine->CreateEcs();
    ASSERT_TRUE(ecs);

    Core::IThreadPool::Ptr threadPool = ecs->GetThreadPool();
    ASSERT_TRUE(threadPool);

    const int dataSize = 1000;

    BASE_NS::vector<int> dataForStdSort;
    BASE_NS::vector<int> dataForParallelSort;

    for (int ii = dataSize; ii > 0; ii--) {
        dataForParallelSort.push_back(ii);
        dataForStdSort.push_back(ii);
    }

    // std::sort
    std::sort(dataForStdSort.begin(), dataForStdSort.end());

    // ParallelSort
    CORE_NS::ParallelSort(dataForParallelSort.begin(), dataForParallelSort.end(), threadPool.get());

    for (int ii = 0; ii < dataSize; ii++) {
        ASSERT_EQ(dataForStdSort[ii], dataForParallelSort[ii]);
    }
}
