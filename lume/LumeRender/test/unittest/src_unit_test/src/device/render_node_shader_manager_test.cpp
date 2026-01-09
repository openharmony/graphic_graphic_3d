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

#include <device/shader_manager.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using CORE_NS::IEngine;
using namespace RENDER_NS;

namespace {
void Validate(const UTest::EngineResources& er)
{
    ShaderManager& shaderMgr = static_cast<ShaderManager&>(er.context->GetDevice().GetShaderManager());
    RenderNodeShaderManager renderNodeShaderMgr { shaderMgr };
    {
        auto shaderHandle1 =
            renderNodeShaderMgr.GetShaderHandle("rendershaders://shader/RenderNodeShaderManagerTest.shader");
        auto pipelineLayout1 = shaderMgr.GetReflectionPipelineLayoutRef(shaderHandle1);
        IShaderManager::PipelineLayoutCreateInfo createInfo1 { "pl1", pipelineLayout1 };
        auto plHandle1 = shaderMgr.CreatePipelineLayout(createInfo1);
        auto shaderHandle2 =
            renderNodeShaderMgr.GetShaderHandle("rendershaders://shader/RenderNodeShaderManagerTest2.shader");
        auto pipelineLayout2 = shaderMgr.GetReflectionPipelineLayoutRef(shaderHandle2);
        IShaderManager::PipelineLayoutCreateInfo createInfo2 { "pl2", pipelineLayout2 };
        auto plHandle2 = shaderMgr.CreatePipelineLayout(createInfo2);
        auto shaderHandle3 =
            renderNodeShaderMgr.GetShaderHandle("rendershaders://shader/RenderNodeShaderManagerTest3.shader");
        auto pipelineLayout3 = shaderMgr.GetReflectionPipelineLayoutRef(shaderHandle3);
        IShaderManager::PipelineLayoutCreateInfo createInfo3 { "pl3", pipelineLayout3 };
        auto plHandle3 = shaderMgr.CreatePipelineLayout(createInfo3);
        {
            auto flags = renderNodeShaderMgr.GetCompatibilityFlags(plHandle1.GetHandle(), plHandle2.GetHandle());
            ASSERT_TRUE(flags & ShaderManager::CompatibilityFlagBits::COMPATIBLE_BIT);
            ASSERT_TRUE(flags & ShaderManager::CompatibilityFlagBits::EXACT_BIT);
        }
        {
            auto flags = renderNodeShaderMgr.GetCompatibilityFlags(plHandle1.GetHandle(), plHandle3.GetHandle());
            ASSERT_TRUE(flags & ShaderManager::CompatibilityFlagBits::COMPATIBLE_BIT);
            ASSERT_FALSE(flags & ShaderManager::CompatibilityFlagBits::EXACT_BIT);
        }
        IShaderManager::ShaderCreateInfo info;
        info.pipelineLayout = plHandle1.GetHandle();
        info.path = "newShader";
        IShaderManager::ShaderModulePath shaderPaths[2] = {{"rendershaders://shader/fullscreen_triangle.vert.spv",
                                                            ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT},
                                                           {"rendershaders://shader/ShaderPipelineBinderTest.frag.spv",
                                                            ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT}};
        info.shaderPaths = { shaderPaths, 2 };
        auto newShaderHandle = shaderMgr.CreateShader(info);
        ASSERT_TRUE(renderNodeShaderMgr.IsValid(newShaderHandle.GetHandle()));
        ASSERT_TRUE(renderNodeShaderMgr.IsShader(newShaderHandle.GetHandle()));
        ASSERT_FALSE(renderNodeShaderMgr.IsComputeShader(newShaderHandle.GetHandle()));
        {
            auto flags = renderNodeShaderMgr.GetCompatibilityFlags(shaderHandle2, newShaderHandle.GetHandle());
            ASSERT_TRUE(flags & ShaderManager::CompatibilityFlagBits::COMPATIBLE_BIT);
            ASSERT_TRUE(flags & ShaderManager::CompatibilityFlagBits::EXACT_BIT);
        }
        {
            auto flags = renderNodeShaderMgr.GetCompatibilityFlags(shaderHandle3, newShaderHandle.GetHandle());
            ASSERT_TRUE(flags & ShaderManager::CompatibilityFlagBits::COMPATIBLE_BIT);
            ASSERT_FALSE(flags & ShaderManager::CompatibilityFlagBits::EXACT_BIT);
        }
        {
            auto vid = renderNodeShaderMgr.GetReflectionVertexInputDeclaration(shaderHandle3);
            ASSERT_EQ(0, vid.bindingDescriptions.size());
        }
        {
            auto pl = renderNodeShaderMgr.GetReflectionPipelineLayout(shaderHandle3);
            ASSERT_EQ(32, pl.pushConstant.byteSize);
        }
        {
            auto plHandle = renderNodeShaderMgr.GetReflectionPipelineLayoutHandle(shaderHandle3);
            ASSERT_NE(RenderHandle {}, plHandle);
        }
        {
            auto plHandle = renderNodeShaderMgr.GetPipelineLayoutHandle("pl1");
            ASSERT_EQ(plHandle1.GetHandle(), plHandle);
        }
        {
            auto vid = renderNodeShaderMgr.GetVertexInputDeclarationView(shaderHandle3);
            ASSERT_EQ(0, vid.bindingDescriptions.size());
        }
        {
            auto vidHandle =
                renderNodeShaderMgr.GetVertexInputDeclarationHandleByShaderHandle(newShaderHandle.GetHandle());
            ASSERT_EQ(RenderHandle {}, vidHandle);
        }
        {
            auto vidHandle = renderNodeShaderMgr.GetVertexInputDeclarationHandle("nonExistingVid");
            ASSERT_EQ(RenderHandle {}, vidHandle);
        }
        {
            auto shaders = renderNodeShaderMgr.GetShaders({}, 0);
            ASSERT_EQ(0, shaders.size());
        }
        {
            auto shaders = renderNodeShaderMgr.GetShaders(2022u);
            ASSERT_EQ(0, shaders.size());
        }
        {
            GraphicsState graphicsState;
            graphicsState.depthStencilState.enableStencilTest = true;
            auto hash = renderNodeShaderMgr.HashGraphicsState(graphicsState);
            ASSERT_EQ(BASE_NS::hash(graphicsState), hash);
        }
        {
            auto shaderHandle =
                renderNodeShaderMgr.GetShaderHandle("rendershaders://shader/RenderNodeShaderManagerTest.shader", "");
            ASSERT_EQ(shaderHandle1, shaderHandle);
        }
        {
            auto shaderHandle = renderNodeShaderMgr.GetShaderHandle({}, 0u);
            ASSERT_EQ(RenderHandle {}, shaderHandle);
        }
        {
            auto slotId = renderNodeShaderMgr.GetRenderSlotId(RenderHandle {});
            ASSERT_EQ(~0u, slotId);
        }
        {
            auto slotId = renderNodeShaderMgr.GetRenderSlotId("");
            ASSERT_EQ(~0u, slotId);
        }
        {
            auto stateHandle = renderNodeShaderMgr.GetGraphicsStateHandle("");
            ASSERT_EQ(RenderHandle {}, stateHandle);
        }
        {
            auto stateHandle = renderNodeShaderMgr.GetGraphicsStateHandle("", "");
            ASSERT_EQ(RenderHandle {}, stateHandle);
        }
        {
            auto stateHandle = renderNodeShaderMgr.GetGraphicsStateHandle({}, ~0u);
            ASSERT_EQ(RenderHandle {}, stateHandle);
        }
        {
            auto slotData = renderNodeShaderMgr.GetRenderSlotData(~0u);
            ASSERT_EQ(~0u, slotData.renderSlotId);
            ASSERT_EQ(RenderHandle {}, slotData.shader.GetHandle());
            ASSERT_EQ(RenderHandle {}, slotData.graphicsState.GetHandle());
        }
        {
            auto stateHandle = renderNodeShaderMgr.GetGraphicsStateHandleByHash(~0u);
            ASSERT_EQ(RenderHandle {}, stateHandle);
        }
    }
}
} // namespace

/**
 * @tc.name: RenderNodeShaderManagerTest
 * @tc.desc: Tests RenderNodeShaderManager and ShaderManager by creating shaders, creating pipeline layouts, checking
 * for shader reflection, etc.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeShaderManager, RenderNodeShaderManagerTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);
    Validate(engine);
    UTest::DestroyEngine(engine);
}
