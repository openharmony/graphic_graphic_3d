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

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/intf_graphics_context.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/fixed_string.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/os/intf_platform.h>
#include <render/device/intf_shader_manager.h>

#include "TestRunner.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;
using namespace RENDER_NS;
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

class GpuTestDefaultMaterialTest : public testing::Test {
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
 * @tc.name: BaseShaders
 * @tc.desc: test BaseShaders
 * @tc.type: FUNC
 */
HWTEST_F(GpuTestDefaultMaterialTest, BaseShaders, TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.

    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;
    auto rhcMgr = GetManager<IRenderHandleComponentManager>(*ecs);

    const auto& shaderMgr = renderContext->GetDevice().GetShaderManager();
    const auto& sceneUtil = graphicsContext->GetSceneUtil();

    // default values
    {
        MaterialComponent::Shader materialShader;
        MaterialComponent::Shader depthShader;

        ISceneUtil::MaterialShaderInfo info {};
        sceneUtil.GetDefaultMaterialShaderData(*ecs, info, materialShader, depthShader);

        ASSERT_TRUE(materialShader.shader);
        ASSERT_TRUE(depthShader.shader);
        {
            const auto gsHandle = rhcMgr->GetRenderHandleReference(materialShader.graphicsState);
            const GraphicsState gs = shaderMgr.GetGraphicsState(gsHandle);
            ASSERT_TRUE(gs.colorBlendState.colorAttachmentCount > 0);
            ASSERT_TRUE(info.alphaBlend == gs.colorBlendState.colorAttachments[0].enableBlend);
            ASSERT_TRUE(info.cullModeFlags == gs.rasterizationState.cullModeFlags);
            ASSERT_TRUE(info.frontFace == gs.rasterizationState.frontFace);
        }
        {
            const auto gsHandle = rhcMgr->GetRenderHandleReference(depthShader.graphicsState);
            const GraphicsState gs = shaderMgr.GetGraphicsState(gsHandle);
            ASSERT_TRUE(gs.colorBlendState.colorAttachmentCount == 0);
            ASSERT_TRUE(info.cullModeFlags == gs.rasterizationState.cullModeFlags);
            ASSERT_TRUE(info.frontFace == gs.rasterizationState.frontFace);
        }
    }
    // variations
    auto checkValues = [&sceneUtil, &ecs, &shaderMgr, &rhcMgr](const ISceneUtil::MaterialShaderInfo& info,
                            MaterialComponent::Shader& materialShader, MaterialComponent::Shader& depthShader) {
        sceneUtil.GetDefaultMaterialShaderData(*ecs, info, materialShader, depthShader);
        ASSERT_TRUE(materialShader.shader);
        if (info.alphaBlend) {
            ASSERT_FALSE(depthShader.shader);
        } else {
            ASSERT_TRUE(depthShader.shader);
        }
        {
            const auto gsHandle = rhcMgr->GetRenderHandleReference(materialShader.graphicsState);
            const GraphicsState gs = shaderMgr.GetGraphicsState(gsHandle);
            ASSERT_TRUE(gs.colorBlendState.colorAttachmentCount > 0);
            ASSERT_TRUE(info.alphaBlend == gs.colorBlendState.colorAttachments[0].enableBlend);
            ASSERT_TRUE(info.cullModeFlags == gs.rasterizationState.cullModeFlags);
            ASSERT_TRUE(info.frontFace == gs.rasterizationState.frontFace);
        }
        if (!info.alphaBlend) {
            const auto gsHandle = rhcMgr->GetRenderHandleReference(depthShader.graphicsState);
            const GraphicsState gs = shaderMgr.GetGraphicsState(gsHandle);
            ASSERT_TRUE(gs.colorBlendState.colorAttachmentCount == 0);
            ASSERT_TRUE(info.cullModeFlags == gs.rasterizationState.cullModeFlags);
            ASSERT_TRUE(info.frontFace == gs.rasterizationState.frontFace);
        }
    };
    // no blend
    {
        MaterialComponent::Shader materialShader;
        MaterialComponent::Shader depthShader;
        checkValues(ISceneUtil::MaterialShaderInfo { false, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                        RENDER_NS::FrontFace::CORE_FRONT_FACE_COUNTER_CLOCKWISE },
            materialShader, depthShader);
    }
    {
        MaterialComponent::Shader materialShader;
        MaterialComponent::Shader depthShader;
        checkValues(ISceneUtil::MaterialShaderInfo { false, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                        RENDER_NS::FrontFace::CORE_FRONT_FACE_CLOCKWISE },
            materialShader, depthShader);
    }

    // blend
    {
        MaterialComponent::Shader materialShader;
        MaterialComponent::Shader depthShader;
        checkValues(ISceneUtil::MaterialShaderInfo { true, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                        RENDER_NS::FrontFace::CORE_FRONT_FACE_COUNTER_CLOCKWISE },
            materialShader, depthShader);
    }
    {
        MaterialComponent::Shader materialShader;
        MaterialComponent::Shader depthShader;
        checkValues(ISceneUtil::MaterialShaderInfo { true, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                        RENDER_NS::FrontFace::CORE_FRONT_FACE_CLOCKWISE },
            materialShader, depthShader);
    }
}

/**
 * @tc.name: ByRenderSlot
 * @tc.desc: test ByRenderSlot
 * @tc.type: FUNC
 */
HWTEST_F(GpuTestDefaultMaterialTest, ByRenderSlot, TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.

    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;
    auto rhcMgr = GetManager<IRenderHandleComponentManager>(*ecs);

    const auto& shaderMgr = renderContext->GetDevice().GetShaderManager();
    const auto& sceneUtil = graphicsContext->GetSceneUtil();

    MaterialComponent::Shader shader;
    // default values
    {
        ISceneUtil::MaterialShaderInfo info {};
        sceneUtil.GetDefaultMaterialShaderData(
            *ecs, info, DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE, shader);

        {
            const auto gsHandle = rhcMgr->GetRenderHandleReference(shader.graphicsState);
            const GraphicsState gs = shaderMgr.GetGraphicsState(gsHandle);
            ASSERT_TRUE(gs.colorBlendState.colorAttachmentCount > 0);
            ASSERT_TRUE(info.alphaBlend == gs.colorBlendState.colorAttachments[0].enableBlend);
            ASSERT_TRUE(info.cullModeFlags == gs.rasterizationState.cullModeFlags);
            ASSERT_TRUE(info.frontFace == gs.rasterizationState.frontFace);
        }
    }
    // variations
    auto checkValues = [&sceneUtil, &shaderMgr, &ecs, &rhcMgr, &shader](const ISceneUtil::MaterialShaderInfo& info,
                            const string_view renderSlot) {
        sceneUtil.GetDefaultMaterialShaderData(*ecs, info, renderSlot, shader);
        {
            const auto gsHandle = rhcMgr->GetRenderHandleReference(shader.graphicsState);
            const GraphicsState gs = shaderMgr.GetGraphicsState(gsHandle);
            ASSERT_TRUE(info.cullModeFlags == gs.rasterizationState.cullModeFlags);
            ASSERT_TRUE(info.frontFace == gs.rasterizationState.frontFace);
        }
    };
    // opaque
    checkValues(ISceneUtil::MaterialShaderInfo { false, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                    RENDER_NS::FrontFace::CORE_FRONT_FACE_COUNTER_CLOCKWISE },
        DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE);
    checkValues(ISceneUtil::MaterialShaderInfo { false, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                    RENDER_NS::FrontFace::CORE_FRONT_FACE_CLOCKWISE },
        DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE);

    // translucent
    checkValues(ISceneUtil::MaterialShaderInfo { true, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                    RENDER_NS::FrontFace::CORE_FRONT_FACE_COUNTER_CLOCKWISE },
        DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT);
    checkValues(ISceneUtil::MaterialShaderInfo { true, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                    RENDER_NS::FrontFace::CORE_FRONT_FACE_CLOCKWISE },
        DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT);

    // shadow
    checkValues(ISceneUtil::MaterialShaderInfo { false, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                    RENDER_NS::FrontFace::CORE_FRONT_FACE_COUNTER_CLOCKWISE },
        DefaultMaterialShaderConstants::RENDER_SLOT_DEPTH);
    checkValues(ISceneUtil::MaterialShaderInfo { false, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                    RENDER_NS::FrontFace::CORE_FRONT_FACE_CLOCKWISE },
        DefaultMaterialShaderConstants::RENDER_SLOT_DEPTH);
}
