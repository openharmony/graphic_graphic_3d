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

#include <test_framework.h>
#include <type_traits>

#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_task_queue_registry.h>

META_BEGIN_NAMESPACE()

namespace UTest {

constexpr BASE_NS::Uid CustomTaskQueueId { "7a55939d-3b89-479f-9e6a-40a13b80434d" };

/**
 * @tc.name: Register
 * @tc.desc: Tests for Register. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueRegistryTest, Register, testing::ext::TestSize.Level1)
{
    auto& taskqRegistry = GetTaskQueueRegistry();
    auto& objRegistry = GetObjectRegistry();

    auto queue = objRegistry.Create<ITaskQueue>(ClassId::PollingTaskQueue);
    ASSERT_NE(queue, nullptr);

    // Register unexisting null
    EXPECT_FALSE(taskqRegistry.RegisterTaskQueue({}, CustomTaskQueueId));
    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));

    // Register valid
    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.RegisterTaskQueue(queue, CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));

    // Register again
    EXPECT_TRUE(taskqRegistry.RegisterTaskQueue(queue, CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));

    // Register null, this should remove
    EXPECT_TRUE(taskqRegistry.RegisterTaskQueue({}, CustomTaskQueueId));
    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));

    EXPECT_TRUE(taskqRegistry.UnregisterAllTaskQueues());
}

/**
 * @tc.name: Replace
 * @tc.desc: Tests for Replace. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueRegistryTest, Replace, testing::ext::TestSize.Level1)
{
    auto& taskqRegistry = GetTaskQueueRegistry();
    auto& objRegistry = GetObjectRegistry();

    auto queue1 = objRegistry.Create<ITaskQueue>(ClassId::PollingTaskQueue);
    auto queue2 = objRegistry.Create<ITaskQueue>(ClassId::PollingTaskQueue);
    ASSERT_NE(queue1, nullptr);
    ASSERT_NE(queue2, nullptr);

    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.RegisterTaskQueue(queue1, CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));
    EXPECT_EQ(taskqRegistry.GetTaskQueue(CustomTaskQueueId), queue1);

    // Replace
    EXPECT_TRUE(taskqRegistry.RegisterTaskQueue(queue2, CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));
    EXPECT_EQ(taskqRegistry.GetTaskQueue(CustomTaskQueueId), queue2);

    EXPECT_TRUE(taskqRegistry.UnregisterAllTaskQueues());
}

/**
 * @tc.name: UnRegister
 * @tc.desc: Tests for Un Register. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueRegistryTest, UnRegister, testing::ext::TestSize.Level1)
{
    auto& taskqRegistry = GetTaskQueueRegistry();
    auto& objRegistry = GetObjectRegistry();

    auto queue = objRegistry.Create<ITaskQueue>(ClassId::PollingTaskQueue);
    ASSERT_NE(queue, nullptr);

    // Register valid
    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.RegisterTaskQueue(queue, CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));
    EXPECT_TRUE(taskqRegistry.UnregisterTaskQueue(CustomTaskQueueId));
    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));

    EXPECT_FALSE(taskqRegistry.UnregisterTaskQueue(CustomTaskQueueId));
    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));

    EXPECT_FALSE(taskqRegistry.UnregisterTaskQueue({}));
    EXPECT_FALSE(taskqRegistry.HasTaskQueue(CustomTaskQueueId));

    EXPECT_TRUE(taskqRegistry.UnregisterAllTaskQueues());
}

/**
 * @tc.name: ConstructPromise
 * @tc.desc: Test promise object construction
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueRegistryTest, ConstructPromise, testing::ext::TestSize.Level1)
{
    auto& t = GetTaskQueueRegistry();
    EXPECT_TRUE(t.ConstructPromise());
}

} // namespace UTest

META_END_NAMESPACE()
