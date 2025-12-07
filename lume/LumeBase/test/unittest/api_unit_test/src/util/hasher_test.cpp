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

#include <base/util/compile_time_hashes.h>
#include <base/util/hash.h>

#include "test_framework.h"

using namespace BASE_NS;

/**
 * @tc.name: HashCombine
 * @tc.desc: Tests for Hash Combine. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilHasherTest, HashCombine, testing::ext::TestSize.Level1)
{
    {
        uint64_t seed1 = 0;
        HashCombine(seed1, 1);

        uint64_t seed2 = 0;
        HashCombine(seed2, 1);

        EXPECT_EQ(seed1, seed2);

        seed2 = 0;
        HashCombine(seed2, hash((uint64_t)1));
        EXPECT_EQ(seed1, seed2);

        seed2 = 0;
        HashCombine(seed2, hash((uint32_t)1));
        EXPECT_EQ(seed1, seed2);

        seed2 = 0;
        HashCombine(seed2, hash((uint16_t)1));
        EXPECT_EQ(seed1, seed2);

        seed2 = 0;
        HashCombine(seed2, hash((uint8_t)1));
        EXPECT_EQ(seed1, seed2);

        seed1 = 0;
        HashCombine(seed1, -1);

        seed2 = 0;
        HashCombine(seed2, hash((int64_t)-1));
        EXPECT_EQ(seed1, seed2);

        seed2 = 0;
        HashCombine(seed2, hash((int32_t)-1));
        EXPECT_EQ(seed1, seed2);

        seed2 = 0;
        HashCombine(seed2, hash((int16_t)-1));
        EXPECT_EQ(seed1, seed2);

        seed2 = 0;
        HashCombine(seed2, hash((int8_t)-1));
        EXPECT_EQ(seed1, seed2);

        uint64_t val = 100;
        const void* pVoid = &val;
        val = 120;
        const uint64_t* pTyped = &val;
        EXPECT_EQ(hash(pVoid), hash(pTyped));
    }

    {
        uint64_t seed1 = 0;
        HashCombine(seed1, 1, 2);

        uint64_t seed2 = 0;
        HashCombine(seed2, 1);
        HashCombine(seed2, 2);

        EXPECT_EQ(seed1, seed2);

        uint64_t seed3 = Hash(1, 2);
        EXPECT_EQ(seed1, seed3);
    }

    {
        uint64_t seed1 = 0;
        HashCombine(seed1, 4, 5, 6);

        uint64_t seed2 = 0;
        HashCombine(seed2, 4);
        HashCombine(seed2, 5);
        HashCombine(seed2, 6);

        EXPECT_EQ(seed1, seed2);

        uint64_t seed3 = Hash(4, 5, 6);
        EXPECT_EQ(seed1, seed3);
    }

    {
        uint64_t seeds[] = { 1, 2, 3 };
        auto itBegin = std::begin(seeds);
        auto itEnd = std::end(seeds);
        auto res = HashRange(itBegin, itEnd);
        uint64_t t = 0; // seeds[0];
        HashCombine(t, seeds[0]);
        HashCombine(t, seeds[1]);
        HashCombine(t, seeds[2]);
        EXPECT_EQ(res, t);
        t = 0;
        HashRange(t, itBegin, itEnd);
        EXPECT_EQ(res, t);
    }
}

/**
 * @tc.name: FNV1aHash
 * @tc.desc: Tests for Fnv1A Hash. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilHasherTest, FNV1aHash, testing::ext::TestSize.Level1)
{
    static constexpr const char input[8] = "To hash";
    // CompileTime::FNV1aHash continues until null terminator i.e. 7 bytes.
    static constexpr const uint64_t t1 = CompileTime::FNV1aHash(input);
    auto t2 = FNV1aHash(input, 7);
    EXPECT_EQ(t1, t2);
    // Without length FNV1aHash uses sizeof which for the char[] will be 8 bytes.
    auto t3 = FNV1aHash(input, 8);
    auto t4 = FNV1aHash(input);
    EXPECT_EQ(t3, t4);
    EXPECT_NE(t1, t3);
}

/**
 * @tc.name: FNV1aHash32
 * @tc.desc: Tests for Fnv1A Hash32. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilHasherTest, FNV1aHash32, testing::ext::TestSize.Level1)
{
    constexpr char input[8] = "To hash";
    // Without length FNV1aHash uses sizeof which for the char[] will be 8 bytes.
    auto t1 = FNV1a32Hash(input);
    auto t2 = FNV1a32Hash(input, 8);
    auto t3 = FNV1a32Hash(input, 7);
    EXPECT_EQ(t1, t2);
    EXPECT_NE(t1, t3);
}
