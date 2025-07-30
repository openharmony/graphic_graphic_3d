/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>

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

#include "TestRunner.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;
using namespace testing::ext;

namespace {
struct TestContext {
    std::shared_ptr<ISceneInit> sceneInit_ = nullptr;
    CORE_NS::IEcs::Ptr ecs_;
};
static TestContext g_context;

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
static constexpr BASE_NS::Uid ENGINE_THREAD{"2070e705-d061-40e4-bfb7-90fad2c280af"};
static constexpr BASE_NS::Uid APP_THREAD{"b2e8cef3-453a-4651-b564-5190f8b5190d"};
static constexpr BASE_NS::Uid IO_QUEUE{"be88e9a0-9cd8-45ab-be48-937953dc258f"};
static constexpr BASE_NS::Uid JS_RELEASE_THREAD{"3784fa96-b25b-4e9c-bbf1-e897d36f73af"};

bool SceneDispose(TestContext &context)
{
    context.ecs_ = nullptr;
    context.sceneInit_ = nullptr;
    return true;
}

bool SceneCreate(TestContext &context)
{
    context.sceneInit_ = CreateTestScene();
    context.sceneInit_->LoadPluginsAndInit();
    if (!context.sceneInit_->GetEngineInstance().engine_) {
        WIDGET_LOGE("fail to get engine");
        return false;
    }
    context.ecs_ = context.sceneInit_->GetEngineInstance().engine_->CreateEcs();
    if (!context.ecs_) {
        WIDGET_LOGE("fail to get ecs");
        return false;
    }
    auto factory = GetInstance<Core::ISystemGraphLoaderFactory>(UID_SYSTEM_GRAPH_LOADER);
    auto systemGraphLoader = factory->Create(context.sceneInit_->GetEngineInstance().engine_->GetFileManager());
    systemGraphLoader->Load("rofs3D://systemGraph.json", *(context.ecs_));
    auto& ecs = *(context.ecs_);
    ecs.Initialize();

    using namespace SCENE_NS;
#if SCENE_META_TEST
    auto fun = [&context]() {
        auto &obr = META_NS::GetObjectRegistry();

        context.params_ = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        if (!context.params_) {
            CORE_LOG_E("default obj null");
        }
        context.scene_ =
            interface_pointer_cast<SCENE_NS::IScene>(obr.Create(SCENE_NS::ClassId::Scene, context.params_));
        
        auto onLoaded = META_NS::MakeCallback<META_NS::IOnChanged>([&context]() {
            bool complete = false;
            auto status = context.scene_->Status()->GetValue();
            if (status == SCENE_NS::IScene::SCENE_STATUS_READY) {
                // still in engine thread
                complete = true;
            } else if (status == SCENE_NS::IScene::SCENE_STATUS_LOADING_FAILED) {
                // make sure we don't have anything in result if error
                complete = true;
            }

            if (complete) {
                if (context.scene_) {
                    auto &obr = META_NS::GetObjectRegistry();
                    // make sure we have renderconfig
                    auto rc = context.scene_->RenderConfiguration()->GetValue();
                    if (!rc) {
                        // Create renderconfig
                        rc = obr.Create<SCENE_NS::IRenderConfiguration>(SCENE_NS::ClassId::RenderConfiguration);
                        context.scene_->RenderConfiguration()->SetValue(rc);
                    }

                    interface_cast<IEcsScene>(context.scene_)
                        ->RenderMode()
                        ->SetValue(IEcsScene::RenderMode::RENDER_ALWAYS);
                    auto duh = context.params_->GetArrayPropertyByName<IntfWeakPtr>("Scenes");
                    if (!duh) {
                        return ;
                    }
                    duh->AddValue(interface_pointer_cast<CORE_NS::IInterface>(context.scene_));
                }
            }
        });
        context.scene_->Asynchronous()->SetValue(false);
        context.scene_->Uri()->SetValue("scene://empty");
        return META_NS::IAny::Ptr{};
    };
    // Should it be possible to cancel? (ie. do we need to store the token for something ..)
    META_NS::GetTaskQueueRegistry()
        .GetTaskQueue(ENGINE_THREAD)
        ->AddWaitableTask(META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>(BASE_NS::move(fun)))
        ->Wait();
#endif
    return true;
}
} // namespace

class RenderDataStoreDefaultCameraTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SceneCreate(g_context);
    }
    static void TearDownTestSuite()
    {
        SceneDispose(g_context);
    }
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: CreateDestroyTest
 * @tc.desc: test CreateDestroy
 * @tc.type: FUNC
 */
HWTEST_F(RenderDataStoreDefaultCameraTest, CreateDestroyTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

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
 * @tc.desc: test GetAddCamera
 * @tc.type: FUNC
 */
HWTEST_F(RenderDataStoreDefaultCameraTest, GetAddCameraTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

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
