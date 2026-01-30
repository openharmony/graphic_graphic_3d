/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "3d_widget_adapter_test.h"

#include "3d_widget_adapter_log.h"
#include "graphics_manager.h"

namespace OHOS::Render3D {
void WidgetAdapter3DTest::SetUpTestCase()
{}

void WidgetAdapter3DTest::TearDownTestCase()
{}

void WidgetAdapter3DTest::SetUp()
{}

void WidgetAdapter3DTest::TearDown()
{}

bool EngineTest::LoadEngineLib()
{
    return true;
}

bool EngineTest::InitEngine(EGLContext eglContext, const PlatformData &data)
{
    (void)(eglContext);
    (void)(data);
    return true;
}

void EngineTest::InitializeScene(uint32_t key)
{
    (void)(key);
}

void EngineTest::SetupCameraViewPort(uint32_t width, uint32_t height)
{
    (void)(width);
    (void)(height);
}

void EngineTest::SetupCameraTransform(
    const Position &position, const Vec3 &lookAt, const Vec3 &up, const Quaternion &rotation)
{
    (void)(position);
    (void)(lookAt);
    (void)(up);
    (void)(rotation);
}

void EngineTest::SetupCameraViewProjection(float zNear, float zFar, float fovDegrees)
{
    (void)(zNear);
    (void)(zFar);
    (void)(fovDegrees);
}

void EngineTest::LoadSceneModel(const std::string &modelPath)
{
    (void)(modelPath);
}

void EngineTest::LoadEnvModel(const std::string &modelPath, BackgroundType type)
{
    (void)(modelPath);
    (void)(type);
}

void EngineTest::OnTouchEvent(const PointerEvent &event)
{
    (void)(event);
}

void EngineTest::OnWindowChange(const TextureInfo &textureInfo)
{
    (void)(textureInfo);
}

void EngineTest::UpdateGeometries(const std::vector<std::shared_ptr<Geometry>> &shapes)
{
    (void)(shapes);
}

void EngineTest::UpdateGLTFAnimations(const std::vector<std::shared_ptr<GLTFAnimation>> &animations)
{
    (void)(animations);
}

void EngineTest::UpdateLights(const std::vector<std::shared_ptr<Light>> &lights)
{
    (void)(lights);
}

void EngineTest::UpdateCustomRender(const std::shared_ptr<CustomRenderDescriptor> &customRender)
{
    (void)(customRender);
}

void EngineTest::UpdateShaderPath(const std::string &shaderPath)
{
    (void)(shaderPath);
}

void EngineTest::UpdateImageTexturePaths(const std::vector<std::string> &imageTextures)
{
    (void)(imageTextures);
}

void EngineTest::UpdateShaderInputBuffer(const std::shared_ptr<ShaderInputBuffer> &shaderInputBuffer)
{
    (void)(shaderInputBuffer);
}

bool EngineTest::NeedsRepaint()
{
    return true;
}

namespace {
/**
 * @tc.name: Initialize1
 * @tc.desc: Verify set valid engine
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, Initialize1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);

    auto engine = std::make_unique<EngineTest>();
    bool ret = adapter.Initialize(std::move(engine));
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: Initialize2
 * @tc.desc: Verify set null engine
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, Initialize2, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);

    bool ret = adapter.Initialize(nullptr);
    ASSERT_EQ(ret, false);
}

/**
 * @tc.name: OnWindowChange1
 * @tc.desc: Verify WidgetAdapter WindowChange
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, OnWindowChange1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));

    TextureInfo texture{};
    bool ret = adapter.OnWindowChange(texture);
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: SetupCameraTransform1
 * @tc.desc: Verify WidgetAdapter setup Camera Transform
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, SetupCameraTransform1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);
    Position position{};
    Vec3 lookAt{};
    Vec3 up{};
    Quaternion rotation{};

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));

    bool ret = adapter.SetupCameraTransform(position, lookAt, up, rotation);
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: SetupCameraViewProjection1
 * @tc.desc: Verify WidgetAdapter setup projection
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, SetupCameraViewProjection1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);
    float near = 0.0f;
    float far = 0.0f;
    float degree = 0.0f;

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));

    bool ret = adapter.SetupCameraViewProjection(near, far, degree);
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: SetupCameraViewport1
 * @tc.desc: Verify WidgetAdapter setup Viewport
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, SetupCameraViewport1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);
    uint32_t width = 0;
    uint32_t height = 0;

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));

    bool ret = adapter.SetupCameraViewport(width, height);
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: OnTouchEvent1
 * @tc.desc: Verify WidgetAdapter handle touch event
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, OnTouchEvent1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);
    PointerEvent event{};

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));
    bool ret = adapter.OnTouchEvent(event);
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: NeedsRepaint1
 * @tc.desc: Verify WidgetAdapter handle Repaint
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, NeedsRepaint1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));
    bool ret = adapter.NeedsRepaint();
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: DrawFrame1
 * @tc.desc: Verify WidgetAdapter render frame
 * @tc.type: FUNC
 * @tc.require: SR000GUI0P
 */
HWTEST_F(WidgetAdapter3DTest, DrawFrame1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));
    bool ret = adapter.DrawFrame();
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: UpdateGeometries1
 * @tc.desc: Verify WidgetAdapter Update geometries
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, UpdateGeometries1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));
    bool ret = adapter.UpdateGeometries({});
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: UpdateGLTFAnimations1
 * @tc.desc: Verify update gltf animation
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, UpdateGLTFAnimations1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));
    bool ret = adapter.UpdateGLTFAnimations({});
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: UpdateLights1
 * @tc.desc: Verify update lights
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, UpdateLights1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));
    bool ret = adapter.UpdateLights({});
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: UpdateCustomRender1
 * @tc.desc: Verify update CustomRender
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, UpdateCustomRender1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));
    bool ret = adapter.UpdateCustomRender(nullptr);
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: UpdateShaderPath1
 * @tc.desc: Verify update ShaderPath
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, UpdateShaderPath1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);
    std::string shaderPath = "path";

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));
    bool ret = adapter.UpdateShaderPath(shaderPath);
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: UpdateImageTexturePaths1
 * @tc.desc: Verify update ImageTexturePaths
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, UpdateImageTexturePaths1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));
    bool ret = adapter.UpdateImageTexturePaths({});
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: UpdateShaderInputBuffer1
 * @tc.desc: Verify update ImageTexturePaths
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, UpdateShaderInputBuffer1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));
    bool ret = adapter.UpdateShaderInputBuffer(nullptr);
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: LoadSceneModel1
 * @tc.desc: Verify Load SceneModel
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, LoadSceneModel1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);
    std::string scene = "scene";

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));
    bool ret = adapter.LoadSceneModel(scene);
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: LoadEnvModel1
 * @tc.desc: Verify Load EnvModel
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, LoadEnvModel1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);
    std::string enviroment = "enviroment";
    BackgroundType type{};

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));
    bool ret = adapter.LoadEnvModel(enviroment, type);
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: UnloadSceneModel1
 * @tc.desc: Verify UnLoad SceneModel
 * @tc.type: FUNC
 * @tc.require: SR000GUGO2
 */
HWTEST_F(WidgetAdapter3DTest, UnloadSceneModel1, testing::ext::TestSize.Level1)
{
    WidgetAdapter adapter(0U);

    auto engine = std::make_unique<EngineTest>();
    adapter.Initialize(std::move(engine));
    bool ret = adapter.UnloadSceneModel();
    ASSERT_EQ(ret, true);
}

/**

 * @tc.name: UnloadEnvModel1

 * @tc.desc: Verify UnLoad EnvModel

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, UnloadEnvModel1, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);



    auto engine = std::make_unique<EngineTest>();

    adapter.Initialize(std::move(engine));

    bool ret = adapter.UnloadEnvModel();

    ASSERT_EQ(ret, true);

}



/**

 * @tc.name: DeInitEngine1

 * @tc.desc: Verify DeInitEngine with valid engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, DeInitEngine1, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);



    auto engine = std::make_unique<EngineTest>();

    adapter.Initialize(std::move(engine));

    

    // Test DeInitEngine after successful initialization

    adapter.DeInitEngine();

    // Since it's a void function, we just verify it doesn't crash

}



/**

 * @tc.name: DeInitEngine2

 * @tc.desc: Verify DeInitEngine with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, DeInitEngine2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);

    

    // Test DeInitEngine without initialization (engine_ is nullptr)

    adapter.DeInitEngine();

    // Should handle nullptr gracefully

}



/**

 * @tc.name: SetupCameraTransform2

 * @tc.desc: Verify SetupCameraTransform with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, SetupCameraTransform2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);

    Position position{};

    Vec3 lookAt{};

    Vec3 up{};

    Quaternion rotation{};



    // Test with null engine

    bool ret = adapter.SetupCameraTransform(position, lookAt, up, rotation);

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: SetupCameraViewProjection2

 * @tc.desc: Verify SetupCameraViewProjection with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, SetupCameraViewProjection2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);

    float near = 0.0f;

    float far = 0.0f;

    float degree = 0.0f;



    // Test with null engine

    bool ret = adapter.SetupCameraViewProjection(near, far, degree);

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: SetupCameraViewport2

 * @tc.desc: Verify SetupCameraViewport with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, SetupCameraViewport2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);

    uint32_t width = 0;

    uint32_t height = 0;



    // Test with null engine

    bool ret = adapter.SetupCameraViewport(width, height);

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: OnTouchEvent2

 * @tc.desc: Verify OnTouchEvent with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, OnTouchEvent2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);

    PointerEvent event{};



    // Test with null engine

    bool ret = adapter.OnTouchEvent(event);

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: DrawFrame2

 * @tc.desc: Verify DrawFrame with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUI0P

 */

HWTEST_F(WidgetAdapter3DTest, DrawFrame2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);



    // Test with null engine

    bool ret = adapter.DrawFrame();

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: DrawFrame3

 * @tc.desc: Verify DrawFrame first frame logic

 * @tc.type: FUNC

 * @tc.require: SR000GUI0P

 */

HWTEST_F(WidgetAdapter3DTest, DrawFrame3, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);



    auto engine = std::make_unique<EngineTest>();

    adapter.Initialize(std::move(engine));

    

    // First call should set firstFrame_ to false

    bool ret1 = adapter.DrawFrame();

    ASSERT_EQ(ret1, true);

    

    // Second call should execute the else branch

    bool ret2 = adapter.DrawFrame();

    ASSERT_EQ(ret2, true);

}



/**

 * @tc.name: UpdateGeometries2

 * @tc.desc: Verify UpdateGeometries with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, UpdateGeometries2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);



    // Test with null engine

    bool ret = adapter.UpdateGeometries({});

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: UpdateGLTFAnimations2

 * @tc.desc: Verify UpdateGLTFAnimations with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, UpdateGLTFAnimations2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);



    // Test with null engine

    bool ret = adapter.UpdateGLTFAnimations({});

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: UpdateLights2

 * @tc.desc: Verify UpdateLights with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, UpdateLights2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);



    // Test with null engine

    bool ret = adapter.UpdateLights({});

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: UpdateCustomRender2

 * @tc.desc: Verify UpdateCustomRender with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, UpdateCustomRender2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);



    // Test with null engine

    bool ret = adapter.UpdateCustomRender(nullptr);

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: UpdateShaderPath2

 * @tc.desc: Verify UpdateShaderPath with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, UpdateShaderPath2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);

    std::string shaderPath = "path";



    // Test with null engine

    bool ret = adapter.UpdateShaderPath(shaderPath);

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: UpdateImageTexturePaths2

 * @tc.desc: Verify UpdateImageTexturePaths with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, UpdateImageTexturePaths2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);



    // Test with null engine

    bool ret = adapter.UpdateImageTexturePaths({});

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: UpdateShaderInputBuffer2

 * @tc.desc: Verify UpdateShaderInputBuffer with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, UpdateShaderInputBuffer2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);



    // Test with null engine

    bool ret = adapter.UpdateShaderInputBuffer(nullptr);

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: OnWindowChange2

 * @tc.desc: Verify OnWindowChange with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, OnWindowChange2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);

    TextureInfo texture{};



    // Test with null engine

    bool ret = adapter.OnWindowChange(texture);

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: LoadSceneModel2

 * @tc.desc: Verify LoadSceneModel with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, LoadSceneModel2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);

    std::string scene = "scene";



    // Test with null engine

    bool ret = adapter.LoadSceneModel(scene);

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: LoadEnvModel2

 * @tc.desc: Verify LoadEnvModel with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, LoadEnvModel2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);

    std::string enviroment = "enviroment";

    BackgroundType type{};



    // Test with null engine

    bool ret = adapter.LoadEnvModel(enviroment, type);

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: UnloadSceneModel2

 * @tc.desc: Verify UnloadSceneModel with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, UnloadSceneModel2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);



    // Test with null engine

    bool ret = adapter.UnloadSceneModel();

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: UnloadEnvModel2

 * @tc.desc: Verify UnloadEnvModel with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, UnloadEnvModel2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);



    // Test with null engine

    bool ret = adapter.UnloadEnvModel();

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: NeedsRepaint2

 * @tc.desc: Verify NeedsRepaint with null engine

 * @tc.type: FUNC

 * @tc.require: SR000GUGO2

 */

HWTEST_F(WidgetAdapter3DTest, NeedsRepaint2, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);



    // Test with null engine

    bool ret = adapter.NeedsRepaint();

    ASSERT_EQ(ret, false);

}



/**

 * @tc.name: DrawFrame4

 * @tc.desc: Verify PerformDrawFrame logic through DrawFrame

 * @tc.type: FUNC

 * @tc.require: SR000GUI0P

 */

HWTEST_F(WidgetAdapter3DTest, DrawFrame4, testing::ext::TestSize.Level1)

{

    WidgetAdapter adapter(0U);



    auto engine = std::make_unique<EngineTest>();

    adapter.Initialize(std::move(engine));

    

    // Test multiple DrawFrame calls to verify both first frame and subsequent frames

    for (int i = 0; i < 3; i++) {

        bool ret = adapter.DrawFrame();

        ASSERT_EQ(ret, true);

    }

}



}  // namespace

}  // namespace OHOS::Render3D
