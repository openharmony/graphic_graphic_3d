/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef META_TEST_MACROS_HEADER
#define META_TEST_MACROS_HEADER

#include <chrono>

#include <gtest/gtest.h>


#define META_WAIT_TRUE_TIMED(millis, arg)                                                \
    {                                                                                    \
        auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(millis); \
        while (!(arg) && std::chrono::steady_clock::now() < end) {                       \
            std::this_thread::sleep_for(std::chrono::milliseconds(1));                   \
        }                                                                                \
    }

#define EXPECT_TRUE_TIMED(millis, arg) \
    META_WAIT_TRUE_TIMED(millis, arg); \
    EXPECT_TRUE(arg);

#define EXPECT_EQ_TIMED(millis, arg, value)         \
    META_WAIT_TRUE_TIMED(millis, (arg) == (value)); \
    EXPECT_EQ(arg, value);

#define EXPECT_THAT_TIMED(millis, arg, matcher)                    \
    META_WAIT_TRUE_TIMED(millis, (testing::Matches(matcher)(arg))); \
    EXPECT_THAT(arg, matcher);

#endif