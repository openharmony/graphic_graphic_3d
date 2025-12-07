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

#include "test_framework.h"
#include "util/linear_allocator.h"

using namespace CORE_NS;

/**
 * @tc.name: allocate
 * @tc.desc: Tests for Allocate. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilLinearAllocatorTest, allocate, testing::ext::TestSize.Level1)
{
    {
        constexpr size_t size = 1024u;
        auto allocator = LinearAllocator(size);

        for (size_t i = 0; i < 10; ++i) {
            auto ptr = allocator.Allocate(16);
            ASSERT_TRUE(ptr);
        }

        auto ptr = allocator.Allocate(size);
        ASSERT_FALSE(ptr);
    }
}

/**
 * @tc.name: alignedAllocate
 * @tc.desc: Tests for Aligned Allocate. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilLinearAllocatorTest, alignedAllocate, testing::ext::TestSize.Level1)
{
    constexpr size_t size = 1024u;
    auto allocator = LinearAllocator(size);

    auto ptr = (ptrdiff_t)allocator.Allocate(1);
    ASSERT_TRUE(ptr);

    auto ptr2 = (ptrdiff_t)allocator.Allocate(1);
    ASSERT_TRUE(ptr2);
    ASSERT_TRUE((ptr + 1) == ptr2);

    constexpr size_t alignment = 256u;
    auto ptr3 = (ptrdiff_t)allocator.Allocate(1, alignment);
    ASSERT_TRUE(ptr3);
    ASSERT_TRUE((ptr3 & (alignment - 1)) == 0);

    auto ptr4 = (ptrdiff_t)allocator.Allocate(1);
    ASSERT_TRUE(ptr4);

    auto ptr5 = (ptrdiff_t)allocator.Allocate(1, alignment);
    ASSERT_TRUE(ptr5);
    ASSERT_TRUE((ptr5 & (alignment - 1)) == 0);
}

/**
 * @tc.name: alignedAllocator
 * @tc.desc: Tests for Aligned Allocator. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilLinearAllocatorTest, alignedAllocator, testing::ext::TestSize.Level1)
{
    constexpr size_t size = 1024u;
    constexpr size_t alignment = 4096u;

    auto allocator = LinearAllocator(size, alignment);

    auto ptr = (ptrdiff_t)allocator.Allocate(16);
    ASSERT_TRUE(ptr);
    ASSERT_TRUE((ptr & (alignment - 1)) == 0);
}

/**
 * @tc.name: reset
 * @tc.desc: Tests for Reset. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilLinearAllocatorTest, reset, testing::ext::TestSize.Level1)
{
    constexpr size_t size = 1024u;
    auto allocator = LinearAllocator(size);

    constexpr size_t bigAllocation = 1000u;
    auto ptr = allocator.Allocate(bigAllocation);
    ASSERT_TRUE(ptr);

    ASSERT_FALSE(allocator.Allocate(bigAllocation));

    allocator.Reset();

    auto ptr2 = allocator.Allocate(bigAllocation);
    ASSERT_TRUE(ptr2);
    ASSERT_TRUE(ptr == ptr2);
}