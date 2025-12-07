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

#include <cstdint>

#include <base/containers/type_traits.h>
#include <base/util/algorithm.h>

#include "test_framework.h"

/**
 * @tc.name: Less
 * @tc.desc: Tests for Less. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Algorithm, Less, testing::ext::TestSize.Level1)
{
    static_assert(BASE_NS::Less<int>()(3, 4));
    static_assert(!BASE_NS::Less<int>()(4, 3));
    static_assert(BASE_NS::Less()(3, 4));
    static_assert(!BASE_NS::Less()(4, 3));
    EXPECT_TRUE(BASE_NS::Less<int>()(3, 4));
    EXPECT_FALSE(BASE_NS::Less<int>()(4, 3));
    EXPECT_TRUE(BASE_NS::Less()(3, 4));
    EXPECT_FALSE(BASE_NS::Less()(4, 3));
}

/**
 * @tc.name: InsertionSort
 * @tc.desc: Tests for Insertion Sort. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Algorithm, InsertionSort, testing::ext::TestSize.Level1)
{
    std::uint8_t data[] = { 0, 7, 14, 3, 7, 12, 9, 1, 6, 11, 2, 8, 13, 4, 15, 5, 10 };
    const auto elements = BASE_NS::extent_v<decltype(data)>;
    // sort with default less comparison
    BASE_NS::InsertionSort(data, data + elements);
    for (auto i = 0U; i < elements - 1U; ++i) {
        EXPECT_LE(data[i], data[i + 1U]);
    }

    // sort with a custom comparison
    BASE_NS::InsertionSort(data, data + elements, [](const auto& lhs, const auto& rhs) { return lhs > rhs; });
    for (auto i = 0U; i < elements - 1U; ++i) {
        EXPECT_GE(data[i], data[i + 1U]);
    }
}

/**
 * @tc.name: LowerBound
 * @tc.desc: Tests for Lower Bound. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Algorithm, LowerBound, testing::ext::TestSize.Level1)
{
    std::uint8_t data[] = { 7, 14, 3, 7, 9, 1, 6, 11, 2, 8, 13, 4, 15, 5, 10, 16 };
    const auto elements = BASE_NS::extent_v<decltype(data)>;
    BASE_NS::InsertionSort(data, data + elements);

    {
        // search with default less comparison
        std::uint8_t needle = 4U;
        auto pos = BASE_NS::LowerBound(data, data + elements, needle);
        EXPECT_NE(pos, (data + elements));
        EXPECT_EQ(*pos, needle);

        // for values not found pos should point to the first value greater than needle, or end
        needle = 0U;
        pos = BASE_NS::LowerBound(data, data + elements, needle);
        EXPECT_GT(*pos, needle);
        needle = 12U;
        pos = BASE_NS::LowerBound(data, data + elements, needle);
        EXPECT_GT(*pos, needle);
        EXPECT_LT(*(pos - 1), needle);

        needle = 17U;
        pos = BASE_NS::LowerBound(data, data + elements, needle);
        EXPECT_EQ(pos, (data + elements));
    }
    {
        // search with default less comparison and explicit limit for linear search
        const std::uint8_t needle = 14U;
        auto pos = BASE_NS::LowerBound<std::uint8_t*, std::uint8_t, 0U>(data, data + elements, needle);
        EXPECT_NE(pos, (data + elements));
        EXPECT_EQ(*pos, needle);
    }

    BASE_NS::InsertionSort(data, data + elements, [](const auto& lhs, const auto& rhs) { return lhs > rhs; });

    {
        // search with custom comparison
        const std::uint8_t needle = 15U;
        auto pos = BASE_NS::LowerBound(
            data, data + elements, needle, [](const auto& lhs, const auto& rhs) { return lhs > rhs; });
        EXPECT_NE(pos, (data + elements));
        EXPECT_EQ(*pos, needle);
    }
}
