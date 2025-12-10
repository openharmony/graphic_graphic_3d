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

#include <functional>
#include <thread>
#include <vector>

#include <base/containers/atomics.h>

#include <meta/base/namespace.h>

#include "test_framework.h"

META_BEGIN_NAMESPACE()
namespace UTest {

/**
 * @tc.name: LockUnlock
 * @tc.desc: Tests for Lock Unlock. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SpinLockTest, LockUnlock, testing::ext::TestSize.Level1)
{
    BASE_NS::SpinLock l;
    bool value = false;

    l.Lock();
    std::thread t { [&] {
        l.Lock();
        value = true;
        l.Unlock();
    } };
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    l.Unlock();
    if (t.joinable()) {
        t.join();
    }
    EXPECT_EQ(value, true);
}

/**
 * @tc.name: MultiLockUnlock
 * @tc.desc: Tests for Multi Lock Unlock. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SpinLockTest, MultiLockUnlock, testing::ext::TestSize.Level1)
{
    BASE_NS::SpinLock l;
    int value = 0;

    std::vector<std::thread> threads;
    for (int i = 0; i != 5; ++i) {
        threads.push_back(std::thread { [&] {
            int v = 0;
            while (v < 100) {
                l.Lock();
                v = ++value;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                l.Unlock();
            }
        } });
    }
    for (auto&& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    EXPECT_TRUE(value >= 100);
}

} // namespace UTest
META_END_NAMESPACE()
