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

#include <meta/api/deferred_callback.h>
#include <meta/api/property/property_event_handler.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/object_macros.h>
#include <meta/interface/property/intf_property.h>

#include "helpers/util.h"
#include "test_framework.h"

META_BEGIN_NAMESPACE()

namespace UTest {

namespace {

struct IOnTestInfo {
    constexpr static BASE_NS::Uid UID { "3e55eee7-f0e4-4363-b4fc-65d8b3574a4e" };
    constexpr static char const* NAME { "OnTest" };
};
using IOnTest = SimpleEvent<IOnTestInfo, void(int)>;

using Callable = SimpleEvent<IOnTestInfo, void()>::InterfaceType;

} // namespace

/**
 * @tc.name: CallWithCallable
 * @tc.desc: Tests for Call With Callable. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_DeferredCallbackTest, CallWithCallable, testing::ext::TestSize.Level1)
{
    auto& registry = META_NS::GetObjectRegistry();

    int count = 0;
    auto q = registry.Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto callback = MakeDeferred<Callable>([&] { ++count; }, q);

    q->ProcessTasks();
    EXPECT_EQ(count, 0);

    callback->Invoke();
    callback->Invoke();

    EXPECT_EQ(count, 0);

    q->ProcessTasks();
    EXPECT_EQ(count, 2);
}

/**
 * @tc.name: CallWithEvent
 * @tc.desc: Tests for Call With Event. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_DeferredCallbackTest, CallWithEvent, testing::ext::TestSize.Level1)
{
    auto& registry = META_NS::GetObjectRegistry();

    int count = 0;
    auto q = registry.Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto callback = MakeDeferred<IOnTest>([&](int i) { count += i; }, q);

    q->ProcessTasks();
    EXPECT_EQ(count, 0);

    callback->Invoke(1);
    callback->Invoke(2);

    EXPECT_EQ(count, 0);

    q->ProcessTasks();
    EXPECT_EQ(count, 3);
}

/**
 * @tc.name: PropertyOnChange
 * @tc.desc: Tests for Property On Change. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_DeferredCallbackTest, PropertyOnChange, testing::ext::TestSize.Level1)
{
    auto& registry = META_NS::GetObjectRegistry();

    auto p = META_NS::ConstructProperty<int>(registry, "P", {});
    ASSERT_TRUE(p);

    int count = 0;
    auto q = registry.Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);

    auto t = p->OnChanged()->AddHandler(MakeDeferred<IOnChanged>([&] { ++count; }, q));

    p->SetValue(1);

    EXPECT_EQ(count, 0);
    q->ProcessTasks();
    EXPECT_EQ(count, 1);

    p->OnChanged()->RemoveHandler(t);

    p->SetValue(2);
    q->ProcessTasks();
    EXPECT_EQ(count, 1);
}

/**
 * @tc.name: PropertyChangedHandler
 * @tc.desc: Tests for Property Changed Handler. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_DeferredCallbackTest, PropertyChangedHandler, testing::ext::TestSize.Level1)
{
    auto& registry = META_NS::GetObjectRegistry();

    auto p = META_NS::ConstructProperty<int>(registry, "P", {});
    ASSERT_TRUE(p);

    int count = 0;
    auto q = registry.Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);

    PropertyChangedEventHandler handler_;
    handler_.Subscribe(
        p, [&] { ++count; }, q);

    p->SetValue(1);

    EXPECT_EQ(count, 0);
    q->ProcessTasks();
    EXPECT_EQ(count, 1);
}

/**
 * @tc.name: PropertyChangedHandlerId
 * @tc.desc: Tests for Property Changed Handler Id. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_DeferredCallbackTest, PropertyChangedHandlerId, testing::ext::TestSize.Level1)
{
    auto& registry = META_NS::GetObjectRegistry();

    auto p = META_NS::ConstructProperty<int>(registry, "P", {});
    ASSERT_TRUE(p);

    int count = 0;
    auto q = registry.Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto queueId = interface_cast<IObjectInstance>(q)->GetInstanceId();
    META_NS::GetTaskQueueRegistry().RegisterTaskQueue(q, queueId.ToUid());

    PropertyChangedEventHandler handler_;
    handler_.Subscribe(
        p, [&] { ++count; }, queueId.ToUid());

    p->SetValue(1);

    EXPECT_EQ(count, 0);
    q->ProcessTasks();
    EXPECT_EQ(count, 1);
    META_NS::GetTaskQueueRegistry().UnregisterTaskQueue(queueId.ToUid());
}

} // namespace UTest

META_END_NAMESPACE()
