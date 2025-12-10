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

#include "TestRunner.h"

#include <gtest/gtest.h>

namespace UTest {

class MinimalistPrinter : public testing::EmptyTestEventListener {
    // Called before a test starts.
    void OnTestStart(const testing::TestInfo& test_info) override
    {
        testing::internal::CaptureStdout();
    }

    // Called after a failed assertion or a SUCCESS().
    void OnTestPartResult(const testing::TestPartResult& test_part_result) override {}

    // Called after a test ends.
    void OnTestEnd(const testing::TestInfo& test_info) override
    {
        auto result = testing::internal::GetCapturedStdout();
        if (test_info.result()->Failed()) {
            fwrite(result.data(), 1, result.size(), stdout);
        }
    }
};

class TestRunnerEnv : public ::testing::Environment {
public:
};
} // namespace UTest

int main(int argc, char** argv)
{
    printf("Running main() from TestRunner.cpp\n");
    ::testing::InitGoogleTest(&argc, argv);

    // Gets hold of the event listener list.
    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    // Adds a listener to the end.  GoogleTest takes the ownership.
    listeners.Append(new UTest::MinimalistPrinter);

    ::testing::AddGlobalTestEnvironment(new UTest::TestRunnerEnv());
    return RUN_ALL_TESTS();
}

// Just making sure that RTTI is disabled. Remove this if we decide to enable RTTI at some point.
#if defined(_CPPRTTI) || defined(__GXX_RTTI)
// #error RTTI not disabled
#endif
