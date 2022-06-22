/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "gtest/gtest.h"

#include "scene_view_core.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {
class SceneViewCoreTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void SceneViewCoreTest::SetUpTestCase() {}
void SceneViewCoreTest::TearDownTestCase() {}
void SceneViewCoreTest::SetUp() {}
void SceneViewCoreTest::TearDown() {}

/**
 * @tc.name: GetSceneViewCore001
 * @tc.desc: get instance
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(SceneViewCoreTest, GetSceneViewCore001, TestSize.Level1)
{
    SceneViewCore::GetInstance();
}

/**
 * @tc.name: CreateEngine001
 * @tc.desc: create engine with platform info
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(SceneViewCoreTest, CreateEngine001, TestSize.Level1)
{
    SceneViewCore& core = SceneViewCore::GetInstance();
    const IPlatformCreateInfo info;
    core.CreateEngine(EngineFactory::EngineType::HUALEI, info);
}

/**
 * @tc.name: CreateTargetSceneTexture001
 * @tc.desc: create texture
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(SceneViewCoreTest, CreateTargetSceneTexture001, TestSize.Level1)
{
    SceneViewCore& core = SceneViewCore::GetInstance();
    const IPlatformCreateInfo info;
    core.CreateEngine(EngineFactory::EngineType::HUALEI, info);
    core.CreateTargetSceneTexture(0, 100, 100);
}

/**
 * @tc.name: SetTargetSceneTexture001
 * @tc.desc: set texture
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(SceneViewCoreTest, SetTargetSceneTexture001, TestSize.Level1)
{
    SceneViewCore& core = SceneViewCore::GetInstance();
    const IPlatformCreateInfo info;
    core.CreateEngine(EngineFactory::EngineType::HUALEI, info);
    TextureInfo textureInfo = core.CreateTargetSceneTexture(0, 100, 100);
    core.SetTargetSceneTexture(0, textureInfo);
}

/**
 * @tc.name: ImportScene001
 * @tc.desc: import 3d scenes
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(SceneViewCoreTest, ImportScene001, TestSize.Level1)
{
    SceneViewCore& core = SceneViewCore::GetInstance();
    const IPlatformCreateInfo info;
    core.CreateEngine(EngineFactory::EngineType::HUALEI, info);
    core.ImportScene(0, "path");
}

/**
 * @tc.name: ImportBackgroundScene001
 * @tc.desc: import 3d background scene
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(SceneViewCoreTest, ImportBackgroundScene001, TestSize.Level1)
{
    SceneViewCore& core = SceneViewCore::GetInstance();
    const IPlatformCreateInfo info;
    core.CreateEngine(EngineFactory::EngineType::HUALEI, info);
    core.ImportBackgroundScene(0, "path");
}

/**
 * @tc.name: SetUpCamera001
 * @tc.desc: set up camera
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(SceneViewCoreTest, SetUpCamera001, TestSize.Level1)
{
    SceneViewCore& core = SceneViewCore::GetInstance();
    const IPlatformCreateInfo info;
    core.CreateEngine(EngineFactory::EngineType::HUALEI, info);
    CameraInfo cameraInfo;
    core.SetUpCamera(0, "camera_node", cameraInfo);
}

/**
 * @tc.name: SetUpLight001
 * @tc.desc: set up light
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(SceneViewCoreTest, SetUpLight001, TestSize.Level1)
{
    SceneViewCore& core = SceneViewCore::GetInstance();
    const IPlatformCreateInfo info;
    core.CreateEngine(EngineFactory::EngineType::HUALEI, info);
    LightInfo lightInfo;
    core.SetUpLight(0, "light_node", lightInfo);
}

/**
 * @tc.name: SetUpAnimation001
 * @tc.desc: start animation
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(SceneViewCoreTest, SetUpAnimation001, TestSize.Level1)
{
    SceneViewCore& core = SceneViewCore::GetInstance();
    const IPlatformCreateInfo info;
    core.CreateEngine(EngineFactory::EngineType::HUALEI, info);
    core.SetUpAnimation(0, "animation_name");
}

/**
 * @tc.name: SetAnimationState001
 * @tc.desc: set animation state play
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(SceneViewCoreTest, SetAnimationState001, TestSize.Level1)
{
    SceneViewCore& core = SceneViewCore::GetInstance();
    const IPlatformCreateInfo info;
    core.CreateEngine(EngineFactory::EngineType::HUALEI, info);
    core.SetAnimationState(0, "animation_name", AnimationState::PLAY);
}

/**
 * @tc.name: SetAnimationState002
 * @tc.desc: set animation state pause
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(SceneViewCoreTest, SetAnimationState002, TestSize.Level1)
{
    SceneViewCore& core = SceneViewCore::GetInstance();
    const IPlatformCreateInfo info;
    core.CreateEngine(EngineFactory::EngineType::HUALEI, info);
    core.SetAnimationState(0, "animation_name", AnimationState::PAUSE);
}

/**
 * @tc.name: SetAnimationState003
 * @tc.desc: set animation state stop
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(SceneViewCoreTest, SetAnimationState003, TestSize.Level1)
{
    SceneViewCore& core = SceneViewCore::GetInstance();
    const IPlatformCreateInfo info;
    core.CreateEngine(EngineFactory::EngineType::HUALEI, info);
    core.SetAnimationState(0, "animation_name", AnimationState::STOP);
}

/**
 * @tc.name: RenderFrame001
 * @tc.desc: render frame
 * @tc.type: FUNC
 * @tc.require: AR000GVALD
 * @tc.author:
 */
HWTEST_F(SceneViewCoreTest, RenderFrame001, TestSize.Level1)
{
    SceneViewCore& core = SceneViewCore::GetInstance();
    const IPlatformCreateInfo info;
    core.CreateEngine(EngineFactory::EngineType::HUALEI, info);
    core.RenderFrame(0);
}
}