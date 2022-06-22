/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "gtest/gtest.h"

#include "engine_service.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {
class EngineServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void EngineServiceTest::SetUpTestCase() {}
void EngineServiceTest::TearDownTestCase() {}
void EngineServiceTest::SetUp() {}
void EngineServiceTest::TearDown() {}

/**
 * @tc.name: CreateMessage001
 * @tc.desc: create message with task
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(EngineServiceTest, CreateMessage001, TestSize.Level1)
{
    const std::function<EngineService::EngineTask> task = []() {};
    EngineService::Message msg(task);
}

/**
 * @tc.name: CreateMessage002
 * @tc.desc: create message with rval
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(EngineServiceTest, CreateMessage002, TestSize.Level1)
{
    const std::function<EngineService::EngineTask> task = []() {};
    EngineService::Message msgRightVal(task);
    EngineService::Message msg(std::move(msgRightVal));
}

/**
 * @tc.name: CreateMessage003
 * @tc.desc: create message with operator =
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(EngineServiceTest, CreateMessage003, TestSize.Level1)
{
    const std::function<EngineService::EngineTask> task = []() {};
    EngineService::Message msgRightVal(task);
    EngineService::Message msg = std::move(msgRightVal);
}

/**
 * @tc.name: ExecuteMessage001
 * @tc.desc: execute message
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(EngineServiceTest, ExecuteMessage001, TestSize.Level1)
{
    const std::function<EngineService::EngineTask> task = []() {};
    EngineService::Message msg(task);
    msg.Execute();
    ASSERT_TRUE(msg.ftr_.valid());
    if (msg.ftr_.valid()) {
        msg.ftr_.get();
    }
    EXPECT_TRUE(!msg.ftr_.valid());
}

/**
 * @tc.name: FinishMessage001
 * @tc.desc: finish message
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(EngineServiceTest, FinishMessage001, TestSize.Level1)
{
    const std::function<EngineService::EngineTask> task = []() {};
    EngineService::Message msg(task);
    msg.Finish();
    ASSERT_TRUE(msg.ftr_.valid());
    if (msg.ftr_.valid()) {
        msg.ftr_.get();
    }
    EXPECT_TRUE(!msg.ftr_.valid());
}

/**
 * @tc.name: GetEngineServiceInstance001
 * @tc.desc: get engine service
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(EngineServiceTest, GetEngineServiceInstance001, TestSize.Level1)
{
    EngineService::GetInstance();
}

/**
 * @tc.name: Register001
 * @tc.desc: register view to service with EGLContext
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(EngineServiceTest, Register001, TestSize.Level1)
{
    EngineService& service = EngineService::GetInstance();
    EGLContext context = (void*)0x11111111;
    service.Register(0, context);
}

/**
 * @tc.name: Unregister001
 * @tc.desc: unregister view
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(EngineServiceTest, Unregister001, TestSize.Level1)
{
    EngineService& service = EngineService::GetInstance();
    service.Unregister(0);
}

/**
 * @tc.name: PushSyncMessage001
 * @tc.desc: push sync message
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(EngineServiceTest, PushSyncMessage001, TestSize.Level1)
{
    EngineService& service = EngineService::GetInstance();
    EGLContext context = (void*)0x11111111;
    service.Register(1, context);
    service.PushSyncMessage([]() {});
}

/**
 * @tc.name: PushSAsncMessage001
 * @tc.desc: push async message
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(EngineServiceTest, PushSAsncMessage001, TestSize.Level1)
{
    EngineService& service = EngineService::GetInstance();
    EGLContext context = (void*)0x11111111;
    service.Register(3, context);
    std::shared_future<void> ftr = service.PushAsyncMessage([]() {});
    ASSERT_TRUE(ftr.valid());
}
} // namespace OHOS::Render3D