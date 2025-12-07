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

#ifndef CORE_TEST_PLUGIN_DYNAMIC_UIDS
#define CORE_TEST_PLUGIN_DYNAMIC_UIDS

#include <base/util/uid.h>

#include "intf_test.h"

namespace UTest {
constexpr BASE_NS::Uid UID_SHARED_PLUGIN { "12345678-1234-1234-1234-deadbeef0020" };

constexpr BASE_NS::Uid UID_SHARED_GLOBAL_TEST_IMPL { "12345678-1234-1234-1234-deadbeef0021" };
constexpr BASE_NS::Uid UID_SHARED_ENGINE_TEST_IMPL { "12345678-1234-1234-1234-deadbeef0022" };

class SharedTest final : public Test {
public:
    SharedTest() = default;

    Type GetType() const override
    {
        return Type::SHARED;
    }
};

class SharedGlobalTest final : public Test {
public:
    SharedGlobalTest() = default;

    Type GetType() const override
    {
        return Type::SHARED_GLOBAL;
    }
};
} // namespace UTest
#endif // CORE_TEST_PLUGIN_DYNAMIC_UIDS