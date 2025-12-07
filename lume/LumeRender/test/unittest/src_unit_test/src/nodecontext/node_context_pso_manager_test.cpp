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

#include <device/device.h>
#include <device/shader_manager.h>
#include <nodecontext/node_context_pso_manager.h>

#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>

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
void TestNodeContextPsoManager(const UTest::EngineResources& engine)
{
    ShaderManager& shaderMgr = static_cast<ShaderManager&>(engine.device->GetShaderManager());
    Device& device = *static_cast<Device*>(engine.device);
    NodeContextPsoManager psoMgr { device, shaderMgr };
    {
        PipelineLayout pl;
        auto handle0 = psoMgr.GetComputePsoHandle({}, pl, {});
        auto handle1 = psoMgr.GetComputePsoHandle({}, pl, {});
        ASSERT_EQ(handle0, handle1);
    }
    {
        RenderHandle plHandle {};
        auto handle = psoMgr.GetComputePsoHandle({}, plHandle, {});
        ASSERT_EQ(RenderHandle {}, handle);
    }
    {
        RenderHandleReference shaderHandle =
            shaderMgr.GetShaderHandle("rendershaders://computeshader/GfxComputeGenericRenderNodeTest.shader");
        RenderHandleReference plHandle = shaderMgr.GetReflectionPipelineLayoutHandle(shaderHandle);
        auto handle = psoMgr.GetComputePsoHandle(shaderHandle.GetHandle(), plHandle.GetHandle(), {});
        ASSERT_NE(RenderHandle {}, handle);
    }
    {
        GraphicsState gs;
        PipelineLayout pl;
        VertexInputDeclarationView vid;
        ShaderSpecializationConstantDataView spec;
        constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT,
            CORE_DYNAMIC_STATE_ENUM_SCISSOR };
        auto handle0 = psoMgr.GetGraphicsPsoHandle({}, gs, pl, vid, spec, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        auto handle1 = psoMgr.GetGraphicsPsoHandle({}, gs, pl, vid, spec, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        ASSERT_EQ(handle0, handle1);
    }
    {
        RenderHandleReference shaderHandle =
            shaderMgr.GetShaderHandle("rendershaders://shader/ShaderPipelineBinderTest.shader");
        auto handle = psoMgr.GetGraphicsPsoHandle(shaderHandle.GetHandle(), shaderHandle.GetHandle(), PipelineLayout {},
            VertexInputDeclarationView {}, {}, {});
        ASSERT_NE(RenderHandle {}, handle);
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    {
        auto pl = psoMgr.GetComputePsoPipelineLayout({});
        for (const auto& layout : pl.descriptorSetLayouts) {
            EXPECT_EQ(layout.set, PipelineLayoutConstants::INVALID_INDEX);
        }
    }
    {
        auto pl = psoMgr.GetGraphicsPsoPipelineLayout({});
        for (const auto& layout : pl.descriptorSetLayouts) {
            EXPECT_EQ(layout.set, PipelineLayoutConstants::INVALID_INDEX);
        }
    }
#endif
}
} // namespace

/**
 * @tc.name: NodeContextPsoManagerTest
 * @tc.desc: Tests NodeContextPsoManager by creating pipeline state objects and validating their behaviour.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_NodeContextPsoManager, NodeContextPsoManagerTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);
    TestNodeContextPsoManager(engine);
    UTest::DestroyEngine(engine);
}
