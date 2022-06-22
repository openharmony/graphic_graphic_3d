/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "gtest/gtest.h"

#include "hualei/hualei_engine.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {
class HUALEIEngineTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void HUALEIEngineTest::SetUpTestCase() {}
void HUALEIEngineTest::TearDownTestCase() {}
void HUALEIEngineTest::SetUp() {}
void HUALEIEngineTest::TearDown() {}

/**
 * @tc.name: CreateEngine001
 * @tc.desc: create engine with platform info
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(HUALEIEngineTest, CreateEngine001, TestSize.Level1)
{
    const IPlatformCreateInfo info;
    std::unique_ptr<HUALEIEngine> engine = std::make_unique<HUALEIEngine>(info);
    EXPECT_TRUE(nullptr != engine);
}

/**
 * @tc.name: CreateTargetSceneTexture001
 * @tc.desc: create texture
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(HUALEIEngineTest, CreateTargetSceneTexture001, TestSize.Level1)
{
    const IPlatformCreateInfo info;
    std::unique_ptr<HUALEIEngine> engine = std::make_unique<HUALEIEngine>(info);
    ASSERT_TRUE(nullptr != engine);
    engine->CreateTargetSceneTexture(0, 100, 100);
}

/**
 * @tc.name: SetTargetSceneTexture001
 * @tc.desc: set texture
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(HUALEIEngineTest, SetTargetSceneTexture001, TestSize.Level1)
{
    const IPlatformCreateInfo info;
    std::unique_ptr<HUALEIEngine> engine = std::make_unique<HUALEIEngine>(info);
    ASSERT_TRUE(nullptr != engine);
    TextureInfo textureInfo = engine->CreateTargetSceneTexture(0, 100, 100);
    engine->SetTargetSceneTexture(0, textureInfo);
}

/**
 * @tc.name: ImportScene001
 * @tc.desc: import 3d scenes
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(HUALEIEngineTest, ImportScene001, TestSize.Level1)
{
    const IPlatformCreateInfo info;
    std::unique_ptr<HUALEIEngine> engine = std::make_unique<HUALEIEngine>(info);
    ASSERT_TRUE(nullptr != engine);
    engine->ImportScene(0, "path");
}

/**
 * @tc.name: ImportBackgroundScene001
 * @tc.desc: import 3d background scene
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(HUALEIEngineTest, ImportBackgroundScene001, TestSize.Level1)
{
    const IPlatformCreateInfo info;
    std::unique_ptr<HUALEIEngine> engine = std::make_unique<HUALEIEngine>(info);
    ASSERT_TRUE(nullptr != engine);
    engine->ImportBackgroundScene(0, "path");
}

/**
 * @tc.name: SetUpCamera001
 * @tc.desc: set up camera
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(HUALEIEngineTest, SetUpCamera001, TestSize.Level1)
{
    const IPlatformCreateInfo info;
    std::unique_ptr<HUALEIEngine> engine = std::make_unique<HUALEIEngine>(info);
    ASSERT_TRUE(nullptr != engine);
    IEngine::CameraInfo cameraInfo;
    engine->SetUpCamera(0, "camera_node", cameraInfo);
}

/**
 * @tc.name: SetUpLight001
 * @tc.desc: set up light
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(HUALEIEngineTest, SetUpLight001, TestSize.Level1)
{
    const IPlatformCreateInfo info;
    std::unique_ptr<HUALEIEngine> engine = std::make_unique<HUALEIEngine>(info);
    ASSERT_TRUE(nullptr != engine);
    IEngine::LightInfo lightInfo;
    engine->SetUpLight(0, "light_node", lightInfo);
}

/**
 * @tc.name: SetUpAnimation001
 * @tc.desc: start animation
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(HUALEIEngineTest, SetUpAnimation001, TestSize.Level1)
{
    const IPlatformCreateInfo info;
    std::unique_ptr<HUALEIEngine> engine = std::make_unique<HUALEIEngine>(info);
    ASSERT_TRUE(nullptr != engine);
    engine->SetUpAnimation(0, "animation_name");
}

/**
 * @tc.name: SetAnimationState001
 * @tc.desc: set animation state play
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(HUALEIEngineTest, SetAnimationState001, TestSize.Level1)
{
    const IPlatformCreateInfo info;
    std::unique_ptr<HUALEIEngine> engine = std::make_unique<HUALEIEngine>(info);
    ASSERT_TRUE(nullptr != engine);
    engine->SetAnimationState(0, "animation_name", IEngine::AnimationState::PLAY);
}

/**
 * @tc.name: SetAnimationState002
 * @tc.desc: set animation state pause
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(HUALEIEngineTest, SetAnimationState002, TestSize.Level1)
{
    const IPlatformCreateInfo info;
    std::unique_ptr<HUALEIEngine> engine = std::make_unique<HUALEIEngine>(info);
    ASSERT_TRUE(nullptr != engine);
    engine->SetAnimationState(0, "animation_name", IEngine::AnimationState::PAUSE);
}

/**
 * @tc.name: SetAnimationState003
 * @tc.desc: set animation state stop
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(HUALEIEngineTest, SetAnimationState003, TestSize.Level1)
{
    const IPlatformCreateInfo info;
    std::unique_ptr<HUALEIEngine> engine = std::make_unique<HUALEIEngine>(info);
    ASSERT_TRUE(nullptr != engine);
    engine->SetAnimationState(0, "animation_name", IEngine::AnimationState::STOP);
}

/**
 * @tc.name: RenderFrame001
 * @tc.desc: render frame
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(HUALEIEngineTest, RenderFrame001, TestSize.Level1)
{
    const IPlatformCreateInfo info;
    std::unique_ptr<HUALEIEngine> engine = std::make_unique<HUALEIEngine>(info);
    ASSERT_TRUE(nullptr != engine);
    engine->RenderFrame(0);
}
}