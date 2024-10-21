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

#include <gtest/gtest.h>

#include <meta/api/deferred_callback.h>
#include <meta/api/property/property_event_handler.h>
#include <meta/ext/object.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/object_macros.h>
#include <meta/interface/property/intf_property.h>

#include "src/test_runner.h"
#include "src/util.h"

META_BEGIN_NAMESPACE()

using namespace testing::ext;
namespace {

struct IOnTestInfo {
    constexpr static BASE_NS::Uid UID { "3e55eee7-f0e4-4363-b4fc-65d8b3574a4e" };
    constexpr static char const *name { "OnTest" };
};

using IOnTest = SimpleEvent<IOnTestInfo, void(int)>;

using Callable = SimpleEvent<IOnTestInfo, void()>::InterfaceType;

};


class DeferredCallbackTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        ResetTest();
    }
    void SetUp() override {}
    void TearDown() override {}
};


/**
 * @tc.name: DeferredCallbackTest
 * @tc.desc: test Call With Callable function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(DeferredCallbackTest, CallWithCallable, TestSize.Level1)
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
 * @tc.name: DeferredCallbackTest
 * @tc.desc: test CallWithEvent function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(DeferredCallbackTest, CallWithEvent, TestSize.Level1)
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
 * @tc.name: DeferredCallbackTest
 * @tc.desc: test PropertyOnChange function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(DeferredCallbackTest, PropertyOnChange, TestSize.Level1)
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
 * @tc.name: DeferredCallbackTest
 * @tc.desc: test PropertyChangedHandler function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(DeferredCallbackTest, PropertyChangedHandler, TestSize.Level1)
{
    auto& registry = META_NS::GetObjectRegistry();

    auto p = META_NS::ConstructProperty<int>(registry, "p", {});
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
 * @tc.name: DeferredCallbackTest
 * @tc.desc: test PropertyChangedHandlerId function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(DeferredCallbackTest, PropertyChangedHandlerId, TestSize.Level1)
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
META_END_NAMESPACE()