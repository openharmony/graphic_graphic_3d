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

#include <3d/implementation_uids.h>
#include <3d/render/render_data_defines_3d.h>
#include <3d/util/intf_render_util.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_class_register.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>

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

class RenderUtilTest : public testing::Test {
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
 * @tc.name: GetRenderNodeGraphDescTest
 * @tc.desc: test GetRenderNodeGraphDesc
 * @tc.type: FUNC
 */
HWTEST_F(RenderUtilTest, GetRenderNodeGraphDescTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

    auto& renderUtil = graphicsContext->GetRenderUtil();

    {
        RenderCamera renderCamera;
        renderCamera.customRenderNodeGraphFile = "file:///data/local/test_data/test_core3d_rng_hdrp.rng";
        renderCamera.flags = RenderCamera::CAMERA_FLAG_REFLECTION_BIT;
        auto desc = renderUtil.GetRenderNodeGraphDesc({}, renderCamera, 0u);
        EXPECT_EQ(13, desc.nodes.size());
    }
    {
        RenderScene renderScene;
        renderScene.customRenderNodeGraphFile = "file:///data/local/test_data/test_core3d_rng_hdrp.rng";
        auto desc = renderUtil.GetRenderNodeGraphDesc(renderScene, 0u);
        EXPECT_EQ(13, desc.nodes.size());
    }
    {
        RenderCamera renderCamera;
        renderCamera.customPostProcessRenderNodeGraphFile = "file:///data/local/test_data/test_core3d_rng_hdrp.rng";
        auto descs = renderUtil.GetRenderNodeGraphDescs({}, renderCamera, 0u);
        EXPECT_EQ(13, descs.postProcess.nodes.size());
    }
    {
        RenderCamera renderCamera;
        renderCamera.customRenderNodeGraphFile = "file:///data/local/test_data/NonExistingRng.rng";
        auto desc = renderUtil.GetRenderNodeGraphDesc({}, renderCamera, 0u);
        EXPECT_EQ(0, desc.nodes.size());
    }
    {
        RenderCamera renderCamera;
        renderCamera.flags = 0u;
        renderCamera.renderPipelineType = RenderCamera::RenderPipelineType::LIGHT_FORWARD;
        auto desc = renderUtil.GetRenderNodeGraphDesc({}, renderCamera, 0u);
        EXPECT_EQ("3drendernodegraphs://core3d_rng_cam_scene_lwrp.rng", desc.renderNodeGraphUri);
    }
    {
        RenderCamera renderCamera;
        renderCamera.flags = RenderCamera::CAMERA_FLAG_MSAA_BIT;
        renderCamera.renderPipelineType = RenderCamera::RenderPipelineType::LIGHT_FORWARD;
        auto desc = renderUtil.GetRenderNodeGraphDesc({}, renderCamera, 0u);
        EXPECT_EQ("3drendernodegraphs://core3d_rng_cam_scene_lwrp_msaa.rng", desc.renderNodeGraphUri);
    }
    {
        RenderCamera renderCamera;
        renderCamera.flags = 0u;
        auto desc = renderUtil.GetRenderNodeGraphDesc({}, renderCamera, 0u);
        EXPECT_EQ("3drendernodegraphs://core3d_rng_cam_scene_hdrp.rng", desc.renderNodeGraphUri);
    }
    {
        RenderCamera renderCamera;
        renderCamera.flags = RenderCamera::CAMERA_FLAG_MSAA_BIT;
        auto desc = renderUtil.GetRenderNodeGraphDesc({}, renderCamera, 0u);
        EXPECT_EQ("3drendernodegraphs://core3d_rng_cam_scene_hdrp_msaa.rng", desc.renderNodeGraphUri);
    }
}
