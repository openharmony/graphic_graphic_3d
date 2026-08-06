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

#ifndef TEST_RUNNER_COMMON
#define TEST_RUNNER_COMMON

#include "log_util.h"

namespace Test {

// Function to run GTests
int RunGoogleTest(int argc, char** argv);

// Interface to be implemented in each platform that runs GTests
class TestApp {
public:
    virtual ~TestApp() = default;

    // Intialize resources and variables required by the app
    virtual void Init() = 0;

    // Run the app
    virtual void Run() = 0;

    // Free resources required by the app
    virtual void Terminate() = 0;
};

}  // namespace Test

#endif  // TEST_RUNNER_COMMON
