/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef CORE_TEST_PLUGIN_DYNAMIC_CIRCULAR_UIDS
#define CORE_TEST_PLUGIN_DYNAMIC_CIRCULAR_UIDS

#include <base/util/uid.h>

namespace UTestCycle {
constexpr BASE_NS::Uid UID_SHARED_PLUGIN_A{"12345678-1234-1234-1234-deadbeef0050"};
constexpr BASE_NS::Uid UID_SHARED_PLUGIN_B{"12345678-1234-1234-1234-deadbeef0051"};
}  // namespace UTestCycle

#endif  // CORE_TEST_PLUGIN_DYNAMIC_CIRCULAR_UIDS
