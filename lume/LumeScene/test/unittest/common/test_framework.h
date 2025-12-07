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

#ifndef TEST_FRAMEWORK
#define TEST_FRAMEWORK

#include <gtest/gtest.h>

// Switch between HPPTest and GTest macros
#if defined(UNIT_TESTS_USE_HCPPTEST)
#define UNIT_TEST(TestSuiteName, TestName, TestType) HWTEST(TestSuiteName, TestName, TestType)
#define UNIT_TEST_F(TestFixtureName, TestName, TestType) HWTEST_F(TestFixtureName, TestName, TestType)
#define UNIT_TEST_P(TestFixtureName, TestName, TestType) HWTEST_P(TestFixtureName, TestName, TestType)
// HWTYPED_TEST, HWTYPED_TEST_P
#else // GTest
#define UNIT_TEST(TestSuiteName, TestName, ...) TEST(TestSuiteName, TestName)
#define UNIT_TEST_F(TestFixtureName, TestName, ...) TEST_F(TestFixtureName, TestName)
#define UNIT_TEST_P(TestFixtureName, TestName, ...) TEST_P(TestFixtureName, TestName)
#endif

#endif // TEST_FRAMEWORK