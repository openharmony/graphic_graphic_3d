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

#include <algorithm>

#include <3d/render/intf_render_data_store_default_camera.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/intf_renderer.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

/**
 * @tc.name: CreateDestroyTest
 * @tc.desc: Tests for Create Destroy Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreDefaultCamera, CreateDestroyTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    constexpr BASE_NS::string_view dataStoreName = "DataStoreDefaultCamera0";
    auto dataStoreDefaultCamera = dsManager.Create(IRenderDataStoreDefaultCamera::UID, dataStoreName.data());
    ASSERT_TRUE(dataStoreDefaultCamera);
    ASSERT_EQ("RenderDataStoreDefaultCamera", dataStoreDefaultCamera->GetTypeName());
    ASSERT_EQ(dataStoreName, dataStoreDefaultCamera->GetName());
    ASSERT_EQ(IRenderDataStoreDefaultCamera::UID, dataStoreDefaultCamera->GetUid());
    ASSERT_EQ(0u, dataStoreDefaultCamera->GetFlags());

    EXPECT_TRUE(dsManager.GetRenderDataStore(dataStoreName));
    // Destruction is deferred
    dataStoreDefaultCamera.reset();
    // Render with no render node graph just to trigger destruction
    renderContext->GetRenderer().RenderFrame({});
    EXPECT_FALSE(dsManager.GetRenderDataStore(dataStoreName));
}

/**
 * @tc.name: GetAddCameraTest
 * @tc.desc: Tests for Get Add Camera Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreDefaultCamera, GetAddCameraTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    constexpr BASE_NS::string_view dataStoreName = "DataStoreDefaultCamera0";
    auto dataStore = dsManager.Create(IRenderDataStoreDefaultCamera::UID, dataStoreName.data());
    ASSERT_TRUE(dataStore);

    auto dataStoreDefaultCamera = static_cast<IRenderDataStoreDefaultCamera*>(dataStore.get());
    // Add cameras
    {
        RenderCamera camera;
        camera.name = "camera0";
        camera.id = 2u;
        camera.customRenderNodeGraphFile = "rng0";
        dataStoreDefaultCamera->AddCamera(camera);
    }
    {
        RenderCamera camera;
        camera.name = "camera1";
        camera.id = 2u;
        camera.customRenderNodeGraphFile = "rng1";
        dataStoreDefaultCamera->AddCamera(camera);
    }
    // Get added cameras
    {
        auto camera = dataStoreDefaultCamera->GetCamera("camera0");
        EXPECT_EQ("camera0", camera.name);
        EXPECT_EQ(2u, camera.id);
        EXPECT_EQ("rng0", camera.customRenderNodeGraphFile);
    }
    {
        auto camera = dataStoreDefaultCamera->GetCamera(2u);
        EXPECT_EQ("camera0", camera.name);
        EXPECT_EQ(2u, camera.id);
        EXPECT_EQ("rng0", camera.customRenderNodeGraphFile);
    }
    // Get non-existing cameras
    {
        auto camera = dataStoreDefaultCamera->GetCamera("NonExistingCamera");
        EXPECT_EQ(RenderCamera {}.name, camera.name);
        EXPECT_EQ(RenderCamera {}.id, camera.id);
        EXPECT_EQ(RenderCamera {}.customRenderNodeGraphFile, camera.customRenderNodeGraphFile);
    }
    {
        auto camera = dataStoreDefaultCamera->GetCamera(3u);
        EXPECT_EQ(RenderCamera {}.name, camera.name);
        EXPECT_EQ(RenderCamera {}.id, camera.id);
        EXPECT_EQ(RenderCamera {}.customRenderNodeGraphFile, camera.customRenderNodeGraphFile);
    }
    {
        EXPECT_EQ(2u, dataStoreDefaultCamera->GetCameraCount());
        EXPECT_EQ(0u, dataStoreDefaultCamera->GetCameraIndex("camera0"));
        EXPECT_EQ(1u, dataStoreDefaultCamera->GetCameraIndex("camera1"));
        EXPECT_EQ(0u, dataStoreDefaultCamera->GetCameraIndex(2u));
        EXPECT_EQ(~0u, dataStoreDefaultCamera->GetCameraIndex(3u));
        EXPECT_EQ(~0u, dataStoreDefaultCamera->GetCameraIndex("NonExistingCamera"));
    }
    // Add more than maximum number of cameras
    {
        for (uint32_t i = 0; i < DefaultMaterialCameraConstants::MAX_CAMERA_COUNT + 3; i++) {
            RenderCamera camera;
            camera.name = to_string(i);
            dataStoreDefaultCamera->AddCamera(camera);
        }
        EXPECT_EQ(DefaultMaterialCameraConstants::MAX_CAMERA_COUNT, dataStoreDefaultCamera->GetCameraCount());
    }
    // Destruction is deferred
    dataStore.reset();
    // Render with no render node graph just to trigger destruction
    renderContext->GetRenderer().RenderFrame({});
    EXPECT_FALSE(dsManager.GetRenderDataStore(dataStoreName));
}
