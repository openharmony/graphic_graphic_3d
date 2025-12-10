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

#ifndef TEST_ENVIRONMENT
#define TEST_ENVIRONMENT

#include <base/namespace.h>

#include "test_framework.h"

// Just making sure that RTTI is disabled. Remove this if we decide to enable RTTI at some point.
#if defined(_CPPRTTI) || defined(__GXX_RTTI)
#error RTTI not disabled
#endif

BASE_BEGIN_NAMESPACE()
namespace UTest {

namespace {
class TestRunnerEnv : public ::testing::Environment {
public:
    void SetUp() override {}

    void TearDown() override {}
};
} // namespace

} // namespace UTest
BASE_END_NAMESPACE()

#endif // TEST_ENVIRONMENT
