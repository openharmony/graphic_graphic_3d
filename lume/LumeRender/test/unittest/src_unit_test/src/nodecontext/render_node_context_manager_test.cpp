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
#include <nodecontext/node_context_descriptor_set_manager.h>
#include <nodecontext/node_context_pso_manager.h>
#include <nodecontext/render_command_list.h>
#include <nodecontext/render_node_context_manager.h>
#include <nodecontext/render_node_graph_share_manager.h>
#include <util/linear_allocator.h>

#include <render/nodecontext/intf_render_node_graph_manager.h>

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
void TestRenderNodeContextManager(const UTest::EngineResources& engine)
{
    ShaderManager& shaderMgr = (ShaderManager&)engine.device->GetShaderManager();
    GpuResourceManager& gpuResourceMgr = (GpuResourceManager&)engine.device->GetGpuResourceManager();
    auto descSetMgr = ((Device*)engine.device)->CreateNodeContextDescriptorSetManager();
    NodeContextPsoManager psoMgr { *(Device*)engine.device, shaderMgr };

    RenderCommandList cmdList { "TestRenderNode", *descSetMgr, gpuResourceMgr, psoMgr, {}, {} };
    RenderNodeGraphShareDataManager dataMgr { {} };

    RenderNodeContextManager::CreateInfo createInfo { *engine.context, {}, {}, {}, {}, *descSetMgr, psoMgr, cmdList,
        dataMgr };
    RenderNodeContextManager contextMgr { createInfo };
    ASSERT_NE(nullptr, contextMgr.GetRenderNodeContextInterface(RenderCommandList::UID));

    ASSERT_EQ(0, contextMgr.GetNodeName().size());
    ASSERT_EQ(0, contextMgr.GetRenderNodeGraphInputs().gpuQueueWaitForSignals.nodeNames.size());

    ASSERT_TRUE(contextMgr.GetInterface(CORE_NS::IInterface::UID));
    ASSERT_TRUE(contextMgr.GetInterface(CORE_NS::IInterface::UID)->GetInterface<IRenderNodeContextManager>());
    ASSERT_TRUE(contextMgr.GetInterface(IRenderNodeContextManager::UID));
    ASSERT_FALSE(contextMgr.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));

    const auto& constContextMgr = contextMgr;
    ASSERT_TRUE(constContextMgr.GetInterface(CORE_NS::IInterface::UID));
    ASSERT_TRUE(constContextMgr.GetInterface(CORE_NS::IInterface::UID)->GetInterface<IRenderNodeContextManager>());
    ASSERT_TRUE(constContextMgr.GetInterface(IRenderNodeContextManager::UID));
    ASSERT_FALSE(constContextMgr.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));
}
} // namespace

/**
 * @tc.name: RenderNodeContextManagerTest
 * @tc.desc: Tests for RenderNodeContextManager by creating it and querying for render node context aswell as
 * IRenderNodeContextManager.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeContextManager, RenderNodeContextManagerTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);
    TestRenderNodeContextManager(engine);
    UTest::DestroyEngine(engine);
}
