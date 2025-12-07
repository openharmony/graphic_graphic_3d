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

#include <meta/api/container/observer.h>
#include <meta/api/util.h>
#include <meta/base/ref_uri.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_startable_controller.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/intf_tickable_controller.h>

#include "helpers/testing_macros.h"
#include "helpers/testing_objects.h"

META_BEGIN_NAMESPACE()

namespace UTest {

MATCHER_P(ObjectEq, expected, "")
{
    return interface_pointer_cast<IObject>(arg) == interface_pointer_cast<IObject>(expected);
}
MATCHER(ObjectIsNull, "")
{
    return interface_pointer_cast<IObject>(arg) == nullptr;
}

class API_StartableControllerTest : public ::testing::Test {
    void SetUp() override
    {
        container_ = interface_pointer_cast<IContainer>(CreateTestContainer("Base"));
        ASSERT_NE(container_, nullptr);
        container_->Add(CreateTestType<IObject>("Object1_1"));
        container_->Add(CreateTestType<IObject>("SameNameDifferentUid"));
        container_->Add(CreateTestType<IObject>("ObjectDupe"));
        container1_1_ = interface_pointer_cast<IContainer>(CreateTestContainer("Container1_1"));
        container_->Add(interface_pointer_cast<IObject>(container1_1_));
        container1_1_->Add(CreateTestType<IObject>("Object2_1"));
        container1_1_->Add(CreateTestType<IObject>("ObjectDupe"));
        container1_1_->Add(CreateTestType<IObject>("SameNameDifferentUid"));
        container_->Add(CreateTestType<IObject>("Object1_3"));
        container2_1_ = interface_pointer_cast<IContainer>(CreateTestContainer("SameNameDifferentUid"));
        container2_1_->Add(contentObject_);
        contentObject_.SetContent(contentChild_);
        container1_1_->Add(interface_pointer_cast<IObject>(container2_1_));
        container_->Add(containerChild_);

        controller_ = GetObjectRegistry().Create<IStartableController>(ClassId::StartableObjectController);
        ASSERT_TRUE(controller_);
    }
    void TearDown() override
    {
        controller_.reset();
        container_.reset();
    }

protected:
    IStartable::Ptr CreateStartable(
        const IObject::Ptr& attachTo, const BASE_NS::string_view name, StartBehavior behavior = StartBehavior::MANUAL)
    {
        auto startable = CreateTestStartable<IStartable>(name);
        if (auto target = interface_cast<IAttach>(attachTo)) {
            target->Attach(interface_pointer_cast<IAttachment>(startable));
        }
        SetValue(startable->StartableMode(), behavior);
        return startable;
    }

    void AttachController()
    {
        auto controller = interface_pointer_cast<IObjectHierarchyObserver>(controller_);
        ASSERT_TRUE(controller);
        controller->SetTarget(interface_pointer_cast<IObject>(container_));
    }

    IContainer::Ptr container_;
    IContainer::Ptr container1_1_;
    IContainer::Ptr container2_1_;
    META_NS::Object containerChild_ { CreateInstance(ClassId::Object) };
    META_NS::ContentObject contentObject_ { CreateInstance(ClassId::ContentObject) };
    META_NS::Object contentChild_ { CreateInstance(ClassId::Object) };
    META_NS::IStartableController::Ptr controller_;
};

using namespace ::testing;

/**
 * @tc.name: StartAll
 * @tc.desc: Tests for Start All. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StartableControllerTest, StartAll, testing::ext::TestSize.Level1)
{
    using Operation = ITestStartable::Operation;
    auto startable = CreateStartable(interface_pointer_cast<IObject>(container2_1_), "Startable");
    auto test = interface_cast<ITestStartable>(startable);
    EXPECT_EQ(GetValue(startable->StartableMode()), StartBehavior::MANUAL);
    EXPECT_EQ(GetValue(startable->StartableState()), StartableState::ATTACHED);
    // StartBehavior = MANUAL
    test->StartRecording();
    AttachController();
    EXPECT_EQ(GetValue(startable->StartableState()), StartableState::ATTACHED);
    EXPECT_THAT(test->StopRecording(), IsEmpty());

    test->StartRecording();
    SetValue(startable->StartableMode(), StartBehavior::AUTOMATIC);
    EXPECT_EQ(GetValue(startable->StartableState()), StartableState::ATTACHED);

    controller_->StartAll();
    EXPECT_EQ(GetValue(startable->StartableState()), StartableState::STARTED);
    EXPECT_THAT(test->StopRecording(), ElementsAre(Operation::START));
}

/**
 * @tc.name: StopAll
 * @tc.desc: Tests for Stop All. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StartableControllerTest, StopAll, testing::ext::TestSize.Level1)
{
    AttachController();
    using Operation = ITestStartable::Operation;
    auto startable = CreateStartable(interface_pointer_cast<IObject>(container2_1_), "Startable");
    auto test = interface_cast<ITestStartable>(startable);

    test->StartRecording();
    SetValue(startable->StartableMode(), StartBehavior::AUTOMATIC);
    EXPECT_EQ(GetValue(startable->StartableState()), StartableState::ATTACHED);

    controller_->StartAll();
    EXPECT_EQ(GetValue(startable->StartableState()), StartableState::STARTED);

    controller_->StopAll();
    EXPECT_THAT(test->StopRecording(), ElementsAre(Operation::START, Operation::STOP));
}

/**
 * @tc.name: Lifecycle
 * @tc.desc: Tests for Lifecycle. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StartableControllerTest, Lifecycle, testing::ext::TestSize.Level1)
{
    AttachController();
    using Operation = ITestStartable::Operation;
    auto startable = CreateStartable(nullptr, "Startable");
    SetValue(startable->StartableMode(), StartBehavior::AUTOMATIC);
    auto test = interface_cast<ITestStartable>(startable);
    test->StartRecording();
    auto att = interface_pointer_cast<IAttachment>(startable);
    EXPECT_TRUE(AttachmentContainer(contentObject_).Attach(att));
    EXPECT_EQ(GetValue(startable->StartableState()), StartableState::STARTED);
    EXPECT_TRUE(AttachmentContainer(contentObject_).Detach(att));
    EXPECT_THAT(
        test->StopRecording(), ElementsAre(Operation::ATTACH, Operation::START, Operation::STOP, Operation::DETACH));
}

/**
 * @tc.name: GetAll
 * @tc.desc: Tests for Get All. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StartableControllerTest, GetAll, testing::ext::TestSize.Level1)
{
    AttachController();
    EXPECT_THAT(controller_->GetAllStartables(), IsEmpty());

    auto startable1 = CreateStartable(contentObject_, "Startable1");
    auto startable2 = CreateStartable(interface_pointer_cast<IObject>(container2_1_), "Startable2");
    auto startable3 = CreateStartable(interface_pointer_cast<IObject>(container_), "Startable3");
    EXPECT_THAT(controller_->GetAllStartables(), UnorderedElementsAre(startable1, startable2, startable3));
}

/**
 * @tc.name: InvalidState
 * @tc.desc: Test functions when controller is in invalid state
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StartableControllerTest, InvalidState, testing::ext::TestSize.Level1)
{
    auto testController = [](const IStartableController::Ptr& controller, bool expectSuccess) {
        EXPECT_EQ(controller->StartAll(), expectSuccess);
        EXPECT_EQ(
            controller->StartAll(static_cast<IStartableController::ControlBehavior>(-1)), expectSuccess); // NOLINT

        EXPECT_EQ(controller->StopAll(), expectSuccess);
        EXPECT_EQ(controller->StopAll(static_cast<IStartableController::ControlBehavior>(-1)), expectSuccess); // NOLINT
    };
    testController(controller_, false);
    AttachController();
    testController(controller_, true);
    SetValue(controller_->TraversalType(), TraversalType::BREADTH_FIRST_ORDER);
    testController(controller_, true);

    static constexpr auto invalidQueueId = BASE_NS::Uid { "11111111-1111-1111-1111-111111111111" };
    controller_->SetStartableQueueId(invalidQueueId, invalidQueueId);
    testController(controller_, true);
}

/**
 * @tc.name: DynamicStart
 * @tc.desc: Tests for Dynamic Start. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StartableControllerTest, DynamicStart, testing::ext::TestSize.Level1)
{
    AttachController();
    using Operation = ITestStartable::Operation;
    // Create a container with a child with attached startable. Then check that the startable is started
    // when the container becomes part of a hierarchy controller by our controller
    auto container = CreateTestContainer("TestContainer");
    auto startable = CreateTestStartable("Startable");
    auto startable2 = CreateTestStartable("Startable2"); // Manual
    auto startableIntf = interface_pointer_cast<IStartable>(startable);
    auto startable2Intf = interface_pointer_cast<IStartable>(startable2);
    auto startableAttachment = interface_pointer_cast<IAttachment>(startable);
    auto startable2Attachment = interface_pointer_cast<IAttachment>(startable2);
    auto containerIntf = interface_pointer_cast<IContainer>(container);
    ASSERT_TRUE(containerIntf);
    ASSERT_TRUE(startableIntf);
    ASSERT_TRUE(startable2Intf);
    ASSERT_TRUE(startableAttachment);
    ASSERT_TRUE(startable2Attachment);
    startable->StartRecording();
    startable2->StartRecording();
    SetValue(startable2Intf->StartableMode(), StartBehavior::MANUAL);
    SetValue(startableIntf->StartableMode(), StartBehavior::AUTOMATIC);
    META_NS::AttachmentContainer object(CreateInstance(ClassId::Object));
    object.Attach(startableAttachment);
    ASSERT_TRUE(containerIntf->Add(object));
    ASSERT_TRUE(interface_cast<IAttach>(containerIntf)->Attach(startable2Attachment));
    EXPECT_THAT(startable->GetOps(), ElementsAre(Operation::ATTACH));
    EXPECT_TRUE(container_->Add(container));
    EXPECT_THAT(startable->GetOps(), ElementsAre(Operation::ATTACH, Operation::START));
    EXPECT_TRUE(container_->Remove(container));
    EXPECT_THAT(startable->GetOps(), ElementsAre(Operation::ATTACH, Operation::START, Operation::STOP));
    EXPECT_THAT(startable2->GetOps(), ElementsAre(Operation::ATTACH));
}

/**
 * @tc.name: StartOrder
 * @tc.desc: Tests for Start Order. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StartableControllerTest, StartOrder, testing::ext::TestSize.Level1)
{
    BASE_NS::vector<uint32_t> startOrder;
    BASE_NS::vector<uint32_t> stopOrder;
    auto startableL0 = CreateTestStartable("StartableL0", StartBehavior::AUTOMATIC);
    auto startableL1_1 = CreateTestStartable("StartableL1_1", StartBehavior::AUTOMATIC);
    auto startableL2 = CreateTestStartable("StartableL2", StartBehavior::AUTOMATIC);
    auto startableL1_2 = CreateTestStartable("StartableL1_2", StartBehavior::AUTOMATIC);

    /*
    The relevant hierarchy and where startables are attached
    container_                              <-- startableL0,   started 4th, stopped 1st
       +-----container1_1_                  <-- startableL1_1, started 2nd, stopped 3th
                  +-------contentObject_    <-- startableL2_1, started 1st, stopped 4th
       +-----containerChild_                <-- startableL1_2, started 3rd, stopped 2nd
    */
    startableL0->AddOnStarted([&]() { startOrder.push_back(4); });
    startableL1_1->AddOnStarted([&]() { startOrder.push_back(2); });
    startableL2->AddOnStarted([&]() { startOrder.push_back(1); });
    startableL1_2->AddOnStarted([&]() { startOrder.push_back(3); });
    startableL0->AddOnStopped([&]() { stopOrder.push_back(1); });
    startableL1_1->AddOnStopped([&]() { stopOrder.push_back(3); });
    startableL2->AddOnStopped([&]() { stopOrder.push_back(4); });
    startableL1_2->AddOnStopped([&]() { stopOrder.push_back(2); });

    auto startableL0Obj = interface_pointer_cast<IAttachment>(startableL0);
    auto startableL1_1Obj = interface_pointer_cast<IAttachment>(startableL1_1);
    auto startableL2Obj = interface_pointer_cast<IAttachment>(startableL2);
    auto startableL1_2Obj = interface_pointer_cast<IAttachment>(startableL1_2);

    auto level0Attach = interface_cast<IAttach>(container_);
    auto level1_1Attach = interface_cast<IAttach>(container1_1_);
    auto level2Attach = interface_cast<IAttach>(contentObject_);
    auto level1_2Attach = interface_cast<IAttach>(containerChild_);

    level0Attach->Attach(startableL0Obj);
    level1_1Attach->Attach(startableL1_1Obj);
    level2Attach->Attach(startableL2Obj);
    level1_2Attach->Attach(startableL1_2Obj);

    EXPECT_THAT(startOrder, IsEmpty());
    EXPECT_THAT(stopOrder, IsEmpty());

    AttachController();
    EXPECT_THAT(startOrder, ElementsAre(1, 2, 3, 4));
    EXPECT_THAT(stopOrder, IsEmpty());
    controller_->StopAll();
    EXPECT_THAT(startOrder, ElementsAre(1, 2, 3, 4));
    EXPECT_THAT(stopOrder, ElementsAre(1, 2, 3, 4));
}

/**
 * @tc.name: Tick
 * @tc.desc: Tests for Tick. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StartableControllerTest, Tick, testing::ext::TestSize.Level1)
{
    BASE_NS::vector<uint32_t> tickOrder;
    auto startableL0 = CreateTestStartable("StartableL0", StartBehavior::AUTOMATIC);
    auto startableL1_1 = CreateTestStartable("StartableL1_1", StartBehavior::AUTOMATIC);
    auto startableL2 = CreateTestStartable("StartableL2", StartBehavior::AUTOMATIC);
    auto startableL1_2 = CreateTestStartable("StartableL1_2", StartBehavior::AUTOMATIC);
    auto startableL3 = CreateTestStartable("StartableL3", StartBehavior::AUTOMATIC);

    /*
    The relevant hierarchy and where startables are attached
    container_                                   <-- startableL0,   ticked 1st
       +-----container1_1_                       <-- startableL1_1, ticked 2nd
                  +-------contentObject_         <-- startableL2_1, ticked 3rd
                                +--contentChild_ <-- startableL3,   ticked 4th
       +-----containerChild_                     <-- startableL1_2, ticked 5th
    */
    startableL0->AddOnTicked([&]() { tickOrder.push_back(1); });
    startableL1_1->AddOnTicked([&]() { tickOrder.push_back(2); });
    startableL2->AddOnTicked([&]() { tickOrder.push_back(3); });
    startableL3->AddOnTicked([&]() { tickOrder.push_back(4); });
    startableL1_2->AddOnTicked([&]() { tickOrder.push_back(5); });

    auto startableL0Obj = interface_pointer_cast<IAttachment>(startableL0);
    auto startableL1_1Obj = interface_pointer_cast<IAttachment>(startableL1_1);
    auto startableL2Obj = interface_pointer_cast<IAttachment>(startableL2);
    auto startableL1_2Obj = interface_pointer_cast<IAttachment>(startableL1_2);
    auto startableL3Obj = interface_pointer_cast<IAttachment>(startableL3);

    auto level0Attach = interface_cast<IAttach>(container_);
    auto level1_1Attach = interface_cast<IAttach>(container1_1_);
    auto level2Attach = interface_cast<IAttach>(contentObject_);
    auto level1_2Attach = interface_cast<IAttach>(containerChild_);
    auto level3Attach = interface_cast<IAttach>(contentChild_);

    EXPECT_TRUE(level0Attach->Attach(startableL0Obj));
    EXPECT_TRUE(level1_1Attach->Attach(startableL1_1Obj));
    EXPECT_TRUE(level2Attach->Attach(startableL2Obj));
    EXPECT_TRUE(level1_2Attach->Attach(startableL1_2Obj));
    EXPECT_TRUE(level3Attach->Attach(startableL3Obj));

    EXPECT_THAT(tickOrder, IsEmpty());

    AttachController();
    EXPECT_THAT(tickOrder, IsEmpty());

    auto ticker = interface_cast<ITickableController>(controller_);
    ASSERT_TRUE(ticker);

    TimeSpan time = TimeSpan::Zero();
    ticker->TickAll(time);
    EXPECT_THAT(tickOrder, ElementsAre(1, 2, 3, 4, 5));

    SetValue(ticker->TickOrder(), TraversalType::BREADTH_FIRST_ORDER);
    tickOrder.clear();
    ticker->TickAll(time);
    EXPECT_THAT(tickOrder, ElementsAre(1, 2, 5, 3, 4));

    SetValue(ticker->TickOrder(), TraversalType::DEPTH_FIRST_POST_ORDER);
    tickOrder.clear();
    ticker->TickAll(time);
    EXPECT_THAT(tickOrder, ElementsAre(4, 3, 2, 5, 1));

    SetValue(ticker->TickOrder(), TraversalType::DEPTH_FIRST_PRE_ORDER);
    tickOrder.clear();
    ticker->TickAll(time);
    EXPECT_THAT(tickOrder, ElementsAre(1, 2, 3, 4, 5));

    // Move child, should affect tick order
    EXPECT_TRUE(container_->Move(containerChild_, 0));
    tickOrder.clear();
    ticker->TickAll(time);
    EXPECT_THAT(tickOrder, ElementsAre(1, 5, 2, 3, 4));
}

/**
 * @tc.name: TickInterval
 * @tc.desc: Tests for Tick Interval. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StartableControllerTest, TickInterval, testing::ext::TestSize.Level1)
{
    BASE_NS::vector<uint32_t> tickOrder;
    auto startableL0 = CreateTestStartable("StartableL0", StartBehavior::AUTOMATIC);
    auto startableL1_1 = CreateTestStartable("StartableL1_1", StartBehavior::AUTOMATIC);
    auto startableL2 = CreateTestStartable("StartableL2", StartBehavior::AUTOMATIC);
    auto startableL1_2 = CreateTestStartable("StartableL1_2", StartBehavior::AUTOMATIC);

    /*
    The relevant hierarchy and where startables are attached
    container_                              <-- startableL0,   ticked 1st
       +-----container1_1_                  <-- startableL1_1, ticked 2nd
                  +-------contentObject_    <-- startableL2_1, ticked 3rd
       +-----containerChild_                <-- startableL1_2, ticked 4th
    */
    startableL0->AddOnTicked([&]() { tickOrder.push_back(1); });
    startableL1_1->AddOnTicked([&]() { tickOrder.push_back(2); });
    startableL2->AddOnTicked([&]() { tickOrder.push_back(3); });
    startableL1_2->AddOnTicked([&]() { tickOrder.push_back(4); });

    auto startableL0Obj = interface_pointer_cast<IAttachment>(startableL0);
    auto startableL1_1Obj = interface_pointer_cast<IAttachment>(startableL1_1);
    auto startableL2Obj = interface_pointer_cast<IAttachment>(startableL2);
    auto startableL1_2Obj = interface_pointer_cast<IAttachment>(startableL1_2);

    auto level0Attach = interface_cast<IAttach>(container_);
    auto level1_1Attach = interface_cast<IAttach>(container1_1_);
    auto level2Attach = interface_cast<IAttach>(contentObject_);
    auto level1_2Attach = interface_cast<IAttach>(containerChild_);

    level0Attach->Attach(startableL0Obj);
    level1_1Attach->Attach(startableL1_1Obj);
    level2Attach->Attach(startableL2Obj);
    level1_2Attach->Attach(startableL1_2Obj);

    EXPECT_THAT(tickOrder, IsEmpty());

    AttachController();
    EXPECT_THAT(tickOrder, IsEmpty());

    auto ticker = interface_cast<ITickableController>(controller_);
    ASSERT_TRUE(ticker);

    SetValue(ticker->TickInterval(), TimeSpan::Milliseconds(90));
    EXPECT_THAT(tickOrder, IsEmpty());
    EXPECT_THAT_TIMED(200, tickOrder, ElementsAre(1, 2, 3, 4));
    EXPECT_THAT_TIMED(200, tickOrder, ElementsAre(1, 2, 3, 4, 1, 2, 3, 4));
    SetValue(ticker->TickInterval(), TimeSpan::Infinite());
    EXPECT_THAT_TIMED(200, tickOrder, ElementsAre(1, 2, 3, 4, 1, 2, 3, 4));
}

/**
 * @tc.name: Queue
 * @tc.desc: Tests for Queue. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StartableControllerTest, Queue, testing::ext::TestSize.Level1)
{
    auto startQueue = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto startQueueId = interface_cast<IObjectInstance>(startQueue)->GetInstanceId();
    GetTaskQueueRegistry().RegisterTaskQueue(startQueue, startQueueId.ToUid());

    auto stopQueue = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto stopQueueId = interface_cast<IObjectInstance>(stopQueue)->GetInstanceId();
    GetTaskQueueRegistry().RegisterTaskQueue(stopQueue, stopQueueId.ToUid());

    EXPECT_TRUE(controller_->SetStartableQueueId(startQueueId.ToUid(), stopQueueId.ToUid()));
    BASE_NS::vector<ITestStartable::Operation> expectedOps;

    AttachController();
    using Operation = ITestStartable::Operation;
    auto startable = CreateStartable(nullptr, "Startable", StartBehavior::AUTOMATIC);
    auto test = interface_cast<ITestStartable>(startable);
    test->StartRecording();
    auto att = interface_pointer_cast<IAttachment>(startable);
    // Attach/detach/attach, this should lead to only 1 Start (and no stops) being called
    // once we run our task queue
    EXPECT_TRUE(AttachmentContainer(contentObject_).Attach(att));
    expectedOps.push_back(Operation::ATTACH);
    EXPECT_TRUE(AttachmentContainer(contentObject_).Detach(att));
    expectedOps.push_back(Operation::DETACH);
    EXPECT_TRUE(AttachmentContainer(contentObject_).Attach(att));
    expectedOps.push_back(Operation::ATTACH);
    EXPECT_EQ(GetValue(startable->StartableState()), StartableState::ATTACHED);
    EXPECT_THAT(test->GetOps(), ElementsAreArray(expectedOps));

    // Shouldn't do anything as we don't have any stop ops
    stopQueue->ProcessTasks();
    EXPECT_EQ(GetValue(startable->StartableState()), StartableState::ATTACHED);
    EXPECT_THAT(test->GetOps(), ElementsAreArray(expectedOps));

    // Process queue, startable should be started.
    startQueue->ProcessTasks();
    expectedOps.push_back(Operation::START);
    EXPECT_EQ(GetValue(startable->StartableState()), StartableState::STARTED);
    EXPECT_THAT(test->GetOps(), ElementsAreArray(expectedOps));

    // Stop all
    EXPECT_TRUE(controller_->StopAll());
    EXPECT_EQ(GetValue(startable->StartableState()), StartableState::STARTED);
    EXPECT_THAT(test->GetOps(), ElementsAreArray(expectedOps));

    // Shouldn't do anything as we don't have any start ops
    startQueue->ProcessTasks();
    EXPECT_EQ(GetValue(startable->StartableState()), StartableState::STARTED);
    EXPECT_THAT(test->GetOps(), ElementsAreArray(expectedOps));

    // Process tasks, this should actually stop the startable
    stopQueue->ProcessTasks();
    expectedOps.push_back(Operation::STOP);
    EXPECT_EQ(GetValue(startable->StartableState()), StartableState::ATTACHED);
    EXPECT_THAT(test->GetOps(), ElementsAreArray(expectedOps));

    // Detach
    EXPECT_TRUE(AttachmentContainer(contentObject_).Detach(att));
    expectedOps.push_back(Operation::DETACH);
    EXPECT_EQ(GetValue(startable->StartableState()), StartableState::DETACHED);
    EXPECT_THAT(test->GetOps(), ElementsAreArray(expectedOps));

    stopQueue->ProcessTasks();
    EXPECT_EQ(GetValue(startable->StartableState()), StartableState::DETACHED);
    EXPECT_THAT(test->GetOps(), ElementsAreArray(expectedOps));

    GetTaskQueueRegistry().UnregisterTaskQueue(startQueueId.ToUid());
    GetTaskQueueRegistry().UnregisterTaskQueue(stopQueueId.ToUid());
}

/**
 * @tc.name: AddingInsideStart
 * @tc.desc: Tests for Adding Inside Start. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StartableControllerTest, AddingInsideStart, testing::ext::TestSize.Level1)
{
    BASE_NS::vector<uint32_t> startOrder;

    auto startable0 = CreateTestStartable("Startable0", StartBehavior::AUTOMATIC);
    auto startable1 = CreateTestStartable("Startable1", StartBehavior::AUTOMATIC);
    auto startableAdded = CreateTestStartable("StartableAdded", StartBehavior::AUTOMATIC);

    startable1->AddOnStarted([&]() { startOrder.push_back(1); });
    startable1->AddOnStarted([&]() {
        auto attach = interface_cast<IAttach>(container1_1_);
        auto startable = interface_pointer_cast<IAttachment>(startableAdded);
        attach->Attach(startable);
    });
    startableAdded->AddOnStarted([&]() { startOrder.push_back(2); });
    startable0->AddOnStarted([&]() { startOrder.push_back(3); });

    container_->Add(startable0);
    container_->Add(startable1);

    auto startableObj = interface_pointer_cast<IAttachment>(startable0);
    auto startable1Obj = interface_pointer_cast<IAttachment>(startable1);

    auto attach1 = interface_cast<IAttach>(container_);
    auto attach2 = interface_cast<IAttach>(container1_1_);

    attach1->Attach(startableObj);
    attach2->Attach(startable1Obj);

    EXPECT_THAT(startOrder, IsEmpty());
    AttachController();
    EXPECT_THAT(startOrder, ElementsAre(1, 2, 3));
    controller_->StopAll();
}

/**
 * @tc.name: QueueAddingInsideStart
 * @tc.desc: Tests for Queue Adding Inside Start. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StartableControllerTest, QueueAddingInsideStart, testing::ext::TestSize.Level1)
{
    auto startQueue = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto startQueueId = interface_cast<IObjectInstance>(startQueue)->GetInstanceId();
    GetTaskQueueRegistry().RegisterTaskQueue(startQueue, startQueueId.ToUid());

    EXPECT_TRUE(controller_->SetStartableQueueId(startQueueId.ToUid(), {}));

    BASE_NS::vector<uint32_t> startOrder;

    auto startable0 = CreateTestStartable("Startable0", StartBehavior::AUTOMATIC);
    auto startable1 = CreateTestStartable("Startable1", StartBehavior::AUTOMATIC);
    auto startableAdded = CreateTestStartable("StartableAdded", StartBehavior::AUTOMATIC);

    startable1->AddOnStarted([&]() { startOrder.push_back(1); });
    startable1->AddOnStarted([&]() {
        auto attach = interface_cast<IAttach>(container1_1_);
        auto startable = interface_pointer_cast<IAttachment>(startableAdded);
        attach->Attach(startable);
    });
    startableAdded->AddOnStarted([&]() { startOrder.push_back(2); });
    startable0->AddOnStarted([&]() { startOrder.push_back(3); });

    container_->Add(startable0);
    container_->Add(startable1);

    auto startableObj = interface_pointer_cast<IAttachment>(startable0);
    auto startable1Obj = interface_pointer_cast<IAttachment>(startable1);

    auto attach1 = interface_cast<IAttach>(container_);
    auto attach2 = interface_cast<IAttach>(container1_1_);

    attach1->Attach(startableObj);
    attach2->Attach(startable1Obj);

    EXPECT_THAT(startOrder, IsEmpty());
    AttachController();
    startQueue->ProcessTasks();
    startQueue->ProcessTasks(); // needs second run if things got queued after first run
    EXPECT_THAT(startOrder, ElementsAre(1, 2, 3));
    controller_->StopAll();
}

} // namespace UTest
META_END_NAMESPACE()
