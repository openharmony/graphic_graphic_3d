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

#include <chrono>
#include <thread>

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

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;
using namespace RENDER_NS;

namespace {} // namespace

/**
 * @tc.name: baseShaders
 * @tc.desc: Tests for Base Shaders. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GpuTest_DefaultMaterialTest, baseShaders, testing::ext::TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
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
    auto CheckValues = [&](const ISceneUtil::MaterialShaderInfo& info, MaterialComponent::Shader& materialShader,
                           MaterialComponent::Shader& depthShader) {
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
        CheckValues(ISceneUtil::MaterialShaderInfo { false, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                        RENDER_NS::FrontFace::CORE_FRONT_FACE_COUNTER_CLOCKWISE },
            materialShader, depthShader);
    }
    {
        MaterialComponent::Shader materialShader;
        MaterialComponent::Shader depthShader;
        CheckValues(ISceneUtil::MaterialShaderInfo { false, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                        RENDER_NS::FrontFace::CORE_FRONT_FACE_CLOCKWISE },
            materialShader, depthShader);
    }

    // blend
    {
        MaterialComponent::Shader materialShader;
        MaterialComponent::Shader depthShader;
        CheckValues(ISceneUtil::MaterialShaderInfo { true, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                        RENDER_NS::FrontFace::CORE_FRONT_FACE_COUNTER_CLOCKWISE },
            materialShader, depthShader);
    }
    {
        MaterialComponent::Shader materialShader;
        MaterialComponent::Shader depthShader;
        CheckValues(ISceneUtil::MaterialShaderInfo { true, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                        RENDER_NS::FrontFace::CORE_FRONT_FACE_CLOCKWISE },
            materialShader, depthShader);
    }
}

/**
 * @tc.name: byRenderSlot
 * @tc.desc: Tests for By Render Slot. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GpuTest_DefaultMaterialTest, byRenderSlot, testing::ext::TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
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
    auto CheckValues = [&](const ISceneUtil::MaterialShaderInfo& info, const string_view renderSlot) {
        sceneUtil.GetDefaultMaterialShaderData(*ecs, info, renderSlot, shader);
        {
            const auto gsHandle = rhcMgr->GetRenderHandleReference(shader.graphicsState);
            const GraphicsState gs = shaderMgr.GetGraphicsState(gsHandle);
            ASSERT_TRUE(info.cullModeFlags == gs.rasterizationState.cullModeFlags);
            ASSERT_TRUE(info.frontFace == gs.rasterizationState.frontFace);
        }
    };
    // opaque
    CheckValues(ISceneUtil::MaterialShaderInfo { false, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                    RENDER_NS::FrontFace::CORE_FRONT_FACE_COUNTER_CLOCKWISE },
        DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE);
    CheckValues(ISceneUtil::MaterialShaderInfo { false, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                    RENDER_NS::FrontFace::CORE_FRONT_FACE_CLOCKWISE },
        DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE);

    // translucent
    CheckValues(ISceneUtil::MaterialShaderInfo { true, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                    RENDER_NS::FrontFace::CORE_FRONT_FACE_COUNTER_CLOCKWISE },
        DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT);
    CheckValues(ISceneUtil::MaterialShaderInfo { true, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                    RENDER_NS::FrontFace::CORE_FRONT_FACE_CLOCKWISE },
        DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT);

    // shadow
    CheckValues(ISceneUtil::MaterialShaderInfo { false, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                    RENDER_NS::FrontFace::CORE_FRONT_FACE_COUNTER_CLOCKWISE },
        DefaultMaterialShaderConstants::RENDER_SLOT_DEPTH);
    CheckValues(ISceneUtil::MaterialShaderInfo { false, RENDER_NS::CullModeFlagBits::CORE_CULL_MODE_BACK_BIT,
                    RENDER_NS::FrontFace::CORE_FRONT_FACE_CLOCKWISE },
        DefaultMaterialShaderConstants::RENDER_SLOT_DEPTH);
}
