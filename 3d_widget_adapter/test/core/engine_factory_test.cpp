/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "gtest/gtest.h"

#include "engine_factory.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {
class EngineFactoryTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void EngineFactoryTest::SetUpTestCase() {}
void EngineFactoryTest::TearDownTestCase() {}
void EngineFactoryTest::SetUp() {}
void EngineFactoryTest::TearDown() {}

/**
 * @tc.name: CreateEngine001
 * @tc.desc: create engine with EngineType HUALEI
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(EngineFactoryTest, CreateEngine001, TestSize.Level1)
{
    const IPlatformCreateInfo info;
    std::unique_ptr<IEngine> engine = EngineFactory::CreateEngine(EngineFactory::EngineType::HUALEI, info);
    EXPECT_TRUE(nullptr != engine);
}
} // namespace OHOS::Render3D