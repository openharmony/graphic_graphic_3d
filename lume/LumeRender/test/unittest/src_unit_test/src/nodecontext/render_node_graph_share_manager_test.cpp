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

#include <nodecontext/render_node_graph_share_manager.h>

#include <render/device/intf_gpu_resource_manager.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>

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
RenderHandleReference CreateBuffer(const UTest::EngineResources& engine)
{
    GpuBufferDesc bufferDesc;
    bufferDesc.byteSize = 16;
    bufferDesc.engineCreationFlags =
        CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE |
        CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER | CORE_ENGINE_BUFFER_CREATION_DEFERRED_DESTROY;
    bufferDesc.usageFlags =
        CORE_BUFFER_USAGE_TRANSFER_SRC_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT | CORE_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    return engine.device->GetGpuResourceManager().Create(bufferDesc);
}
void TestRenderGraphShareDataManager(const UTest::EngineResources& engine)
{
    RenderNodeGraphShareDataManager rngShareDataMgr { {} };

    RenderHandleReference buffer0 = CreateBuffer(engine);
    RenderHandleReference buffer1 = CreateBuffer(engine);
    RenderHandleReference buffer2 = CreateBuffer(engine);
    {
        IRenderNodeGraphShareManager::NamedResource inputs[] = { { "buffer0", buffer0.GetHandle() },
            { "buffer2", buffer2.GetHandle() } };
        IRenderNodeGraphShareManager::NamedResource outputs[] = { { "buffer2", buffer2.GetHandle() },
            { "buffer1", buffer1.GetHandle() } };
        rngShareDataMgr.BeginFrame(nullptr, nullptr, 5, { inputs, countof(inputs) }, { outputs, countof(outputs) });
        {
            auto result = rngShareDataMgr.GetRenderNodeGraphInputs();
            ASSERT_EQ(countof(inputs), result.size());
            for (uint32_t i = 0; i < result.size(); ++i) {
                ASSERT_EQ(inputs[i].handle, result[i]);
            }
        }
        {
            auto result = rngShareDataMgr.GetRenderNodeGraphOutputs();
            ASSERT_EQ(countof(outputs), result.size());
            for (uint32_t i = 0; i < result.size(); ++i) {
                ASSERT_EQ(outputs[i].handle, result[i]);
            }
        }
    }
    {
        rngShareDataMgr.RegisterRenderNodeName(0u, "Node0", "Node0_fullname");
        rngShareDataMgr.RegisterRenderNodeName(1u, "Node1", "Node1_fullname");
        rngShareDataMgr.RegisterRenderNodeName(2u, "Node2", "Node2_fullname");
        rngShareDataMgr.RegisterRenderNodeName(3u, "Node3", "Node3_fullname");
        rngShareDataMgr.RegisterRenderNodeName(4u, "Node4", "Node4_fullname");
        ASSERT_EQ(0u, rngShareDataMgr.GetRenderNodeIndex("Node0"));
        ASSERT_EQ(1u, rngShareDataMgr.GetRenderNodeIndex("Node1"));
        ASSERT_EQ(2u, rngShareDataMgr.GetRenderNodeIndex("Node2"));
        ASSERT_EQ(3u, rngShareDataMgr.GetRenderNodeIndex("Node3"));
        ASSERT_EQ(4u, rngShareDataMgr.GetRenderNodeIndex("Node4"));
        ASSERT_EQ(~0u, rngShareDataMgr.GetRenderNodeIndex("Node5"));
    }
    {
        rngShareDataMgr.RegisterRenderNodeOutput(2, "Node2Output0", buffer0.GetHandle());
        rngShareDataMgr.RegisterRenderNodeOutput(2, "Node2Output1", buffer1.GetHandle());
        rngShareDataMgr.RegisterRenderNodeOutput(3, "Node3Output0", buffer2.GetHandle());
        {
            auto result = rngShareDataMgr.GetRenderNodeOutputs(2);
            ASSERT_EQ(2, result.size());
            ASSERT_EQ("Node2Output0", result[0].name);
            ASSERT_EQ(buffer0.GetHandle(), result[0].handle);
            ASSERT_EQ("Node2Output1", result[1].name);
            ASSERT_EQ(buffer1.GetHandle(), result[1].handle);
        }
        {
            auto result = rngShareDataMgr.GetRenderNodeOutputs(3);
            ASSERT_EQ(1, result.size());
            ASSERT_EQ("Node3Output0", result[0].name);
            ASSERT_EQ(buffer2.GetHandle(), result[0].handle);
        }
        {
            auto result = rngShareDataMgr.GetRenderNodeOutputs(4);
            ASSERT_EQ(0, result.size());
        }
        {
            auto result = rngShareDataMgr.GetRenderNodeOutputs(8);
            ASSERT_EQ(0, result.size());
        }
    }
    {
        RenderNodeGraphOutputResource resources[2];
        resources[0].name = "out0";
        resources[0].nodeName = "node0";
        resources[1].name = "out1";
        resources[1].nodeName = "node1";
        RenderNodeGraphGlobalShareDataManager rngGlobalShareDataMgr;
        RenderNodeGraphShareDataManager rngShareDataMgr2 { { resources, countof(resources) } };
        IRenderNodeGraphShareManager::NamedResource inputs[] = { { "buffer0", buffer0.GetHandle() },
            { "buffer2", buffer2.GetHandle() } };
        IRenderNodeGraphShareManager::NamedResource outputs[] = { { "buffer2", buffer2.GetHandle() } };
        rngGlobalShareDataMgr.BeginFrame();
        rngShareDataMgr2.BeginFrame(
            &rngGlobalShareDataMgr, &rngShareDataMgr, 5, { inputs, countof(inputs) }, { outputs, countof(outputs) });
        rngShareDataMgr2.RegisterRenderNodeName(0u, "node1", "node1_fullname");
        rngShareDataMgr2.RegisterRenderNodeOutput(0u, "", {});
        rngShareDataMgr2.RegisterRenderNodeOutput(0u, "out1", buffer0.GetHandle());
        for (uint32_t i = 0; i < RenderNodeGraphShareDataManager::MAX_RENDER_NODE_GRAPH_RES_COUNT + 1; ++i) {
            rngShareDataMgr2.RegisterRenderNodeOutput(0u, to_string(i), buffer0.GetHandle());
        }
        rngShareDataMgr2.PrepareExecuteFrame();
        rngShareDataMgr2.RegisterRenderNodeOutput(0u, "", {});
        auto namedInputView = rngShareDataMgr2.GetNamedRenderNodeGraphInputs();
        ASSERT_EQ(2u, namedInputView.size());
        ASSERT_EQ(inputs[0].name, namedInputView[0].name);
        ASSERT_EQ(inputs[0].handle, namedInputView[0].handle);
        ASSERT_EQ(inputs[1].name, namedInputView[1].name);
        ASSERT_EQ(inputs[1].handle, namedInputView[1].handle);
        auto namedOutputView = rngShareDataMgr2.GetNamedRenderNodeGraphOutputs();
        ASSERT_EQ(2u, namedOutputView.size());
        ASSERT_EQ(outputs[0].name, namedOutputView[0].name);
        ASSERT_EQ(outputs[0].handle, namedOutputView[0].handle);
        ASSERT_EQ(resources[1].name, namedOutputView[1].name);
        ASSERT_EQ(buffer0.GetHandle(), namedOutputView[1].handle);
        {
            auto prevOutput = rngShareDataMgr2.GetPrevRenderNodeGraphOutputs();
            ASSERT_EQ(2u, prevOutput.size());
        }
        {
            auto prevOutput = rngShareDataMgr2.GetNamedPrevRenderNodeGraphOutputs();
            ASSERT_EQ(2u, prevOutput.size());
        }
        {
            auto prevOutput = rngShareDataMgr.GetPrevRenderNodeGraphOutputs();
            ASSERT_EQ(0u, prevOutput.size());
        }
        {
            auto prevOutput = rngShareDataMgr.GetNamedPrevRenderNodeGraphOutputs();
            ASSERT_EQ(0u, prevOutput.size());
        }
        RenderNodeGraphShareManager rngShareMgr(rngShareDataMgr2);
        ASSERT_NE(RenderHandle {}, rngShareMgr.GetPrevRenderNodeGraphOutput(0u));
        ASSERT_NE(RenderHandle {}, rngShareMgr.GetNamedPrevRenderNodeGraphOutput("buffer1"));
    }
}
void TestRenderGraphShareManager(const UTest::EngineResources& engine)
{
    RenderNodeGraphGlobalShareDataManager rngGlobalShareDataMgr;
    RenderNodeGraphShareDataManager rngShareDataMgr { {} };
    RenderNodeGraphShareManager rngShareMgr(rngShareDataMgr);
    ASSERT_TRUE(rngShareMgr.GetInterface(CORE_NS::IInterface::UID));
    ASSERT_TRUE(rngShareMgr.GetInterface(CORE_NS::IInterface::UID)->GetInterface<IRenderNodeGraphShareManager>());
    ASSERT_TRUE(rngShareMgr.GetInterface(IRenderNodeGraphShareManager::UID));
    ASSERT_FALSE(rngShareMgr.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));
    const auto& constRngShareMgr = rngShareMgr;
    ASSERT_TRUE(constRngShareMgr.GetInterface(CORE_NS::IInterface::UID));
    ASSERT_TRUE(constRngShareMgr.GetInterface(CORE_NS::IInterface::UID)->GetInterface<IRenderNodeGraphShareManager>());
    ASSERT_TRUE(constRngShareMgr.GetInterface(IRenderNodeGraphShareManager::UID));
    ASSERT_FALSE(constRngShareMgr.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));

    RenderHandleReference buffer0 = CreateBuffer(engine);
    RenderHandleReference buffer1 = CreateBuffer(engine);
    RenderHandleReference buffer2 = CreateBuffer(engine);
    {
        IRenderNodeGraphShareManager::NamedResource inputs[] = { { "buffer0", buffer0.GetHandle() },
            { "buffer2", buffer2.GetHandle() } };
        IRenderNodeGraphShareManager::NamedResource outputs[] = { { "buffer2", buffer2.GetHandle() },
            { "buffer1", buffer1.GetHandle() } };
        rngGlobalShareDataMgr.BeginFrame();
        rngShareDataMgr.BeginFrame(
            &rngGlobalShareDataMgr, nullptr, 5, { inputs, countof(inputs) }, { outputs, countof(outputs) });
        {
            auto result = rngShareDataMgr.GetRenderNodeGraphInputs();
            ASSERT_EQ(countof(inputs), result.size());
            for (uint32_t i = 0; i < result.size(); ++i) {
                ASSERT_EQ(inputs[i].handle, result[i]);
            }
        }
        {
            auto result = rngShareDataMgr.GetRenderNodeGraphOutputs();
            ASSERT_EQ(countof(outputs), result.size());
            for (uint32_t i = 0; i < result.size(); ++i) {
                ASSERT_EQ(outputs[i].handle, result[i]);
            }
        }
    }

    rngShareMgr.Init(2, "Node2", "Node2_fullname");
    rngGlobalShareDataMgr.BeginFrame();
    rngShareMgr.BeginFrame(2); // 2: param
    {
        RenderHandle outputs[] = { buffer0.GetHandle(), buffer1.GetHandle(), buffer2.GetHandle() };
        rngShareMgr.RegisterRenderNodeOutputs({ outputs, countof(outputs) });
        rngShareMgr.RegisterGlobalRenderNodeOutput("Name1", buffer0.GetHandle());
        rngShareMgr.RegisterGlobalRenderNodeOutput("Name2", buffer1.GetHandle());
        rngShareMgr.RegisterGlobalRenderNodeOutput("Name3", buffer2.GetHandle());

        const auto& globalOutputs = rngShareMgr.GetRegisteredGlobalRenderNodeOutputs("Node2_fullname");
        ASSERT_TRUE(globalOutputs.size() == countof(outputs));
        for (uint32_t i = 0; i < countof(outputs); ++i) {
            ASSERT_EQ(outputs[i], rngShareMgr.GetRegisteredRenderNodeOutput("Node2", i));
            if (i < globalOutputs.size()) {
                ASSERT_EQ(outputs[i], globalOutputs[i].handle);
            }
        }
    }
    {
        rngShareDataMgr.RegisterRenderNodeOutput(2, "Node2Output0", buffer0.GetHandle());
        rngShareDataMgr.RegisterRenderNodeOutput(2, "Node2Output1", buffer1.GetHandle());
        rngShareDataMgr.RegisterRenderNodeOutput(1, "Node1Output0", buffer2.GetHandle());
        {
            auto result = rngShareMgr.GetRegisteredRenderNodeOutput("Node2", "Node2Output0");
            ASSERT_EQ(buffer0.GetHandle(), result);
        }
        {
            auto result = rngShareMgr.GetRegisteredRenderNodeOutput("Node2", "NonExistingResource");
            ASSERT_EQ(RenderHandle {}, result);
        }
        {
            auto result = rngShareMgr.GetRegisteredPrevRenderNodeOutput(0);
            ASSERT_EQ(buffer2.GetHandle(), result);
        }
    }
    {
        RenderHandle inputs[] = { buffer0.GetHandle(), buffer2.GetHandle() };
        auto result = rngShareMgr.GetRenderNodeGraphInputs();
        ASSERT_EQ(countof(inputs), result.size());
        for (uint32_t i = 0; i < result.size(); ++i) {
            ASSERT_EQ(inputs[i], result[i]);
            ASSERT_EQ(inputs[i], rngShareMgr.GetRenderNodeGraphInput(i));
        }
        ASSERT_EQ(RenderHandle {}, rngShareMgr.GetRenderNodeGraphInput(3));
    }
    {
        RenderHandle inputs[] = { buffer0.GetHandle(), buffer2.GetHandle() };
        string names[] = { "buffer0", "buffer2" };
        auto result = rngShareMgr.GetNamedRenderNodeGraphInputs();
        ASSERT_EQ(countof(inputs), result.size());
        for (uint32_t i = 0; i < result.size(); ++i) {
            ASSERT_EQ(inputs[i], result[i].handle);
            ASSERT_EQ(names[i], result[i].name);
        }
    }
    {
        auto outputs = rngShareMgr.GetRenderNodeGraphOutputs();
        ASSERT_EQ(2u, outputs.size());
        ASSERT_EQ(buffer2.GetHandle(), outputs[0]);
        ASSERT_EQ(buffer2.GetHandle(), rngShareMgr.GetRenderNodeGraphOutput(0u));
        ASSERT_EQ(buffer1.GetHandle(), outputs[1]);
        ASSERT_EQ(buffer1.GetHandle(), rngShareMgr.GetRenderNodeGraphOutput(1u));
        ASSERT_EQ(RenderHandle {}, rngShareMgr.GetRenderNodeGraphOutput(2u));
    }
    {
        auto outputs = rngShareMgr.GetNamedRenderNodeGraphOutputs();
        ASSERT_EQ(2u, outputs.size());
        ASSERT_EQ(buffer2.GetHandle(), outputs[0].handle);
        ASSERT_EQ("buffer2", outputs[0].name);
        ASSERT_EQ(buffer1.GetHandle(), outputs[1].handle);
        ASSERT_EQ("buffer1", outputs[1].name);
    }
    {
        auto outputs = rngShareMgr.GetPrevRenderNodeGraphOutputs();
        ASSERT_EQ(0u, outputs.size());
        ASSERT_EQ(RenderHandle {}, rngShareMgr.GetPrevRenderNodeGraphOutput(1u));
    }
    {
        auto outputs = rngShareMgr.GetNamedPrevRenderNodeGraphOutputs();
        ASSERT_EQ(0u, outputs.size());
        ASSERT_EQ(RenderHandle {}, rngShareMgr.GetNamedPrevRenderNodeGraphOutput(""));
    }
    {
        RenderHandle handles[10];
        for (uint32_t i = 0; i < countof(handles); ++i) {
            handles[i] = {};
        }
        rngShareMgr.RegisterRenderNodeOutputs({ handles, countof(handles) });
    }
}
} // namespace

/**
 * @tc.name: RenderNodeGraphShareDataManagerTest
 * @tc.desc: Tests for RenderNodeGraphShareDataManager by registering inputs and output resources of many nodes and
 * verifying the resources are registered and queryable via the class.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeGraphShareManager, RenderNodeGraphShareDataManagerTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);
    TestRenderGraphShareDataManager(engine);
    UTest::DestroyEngine(engine);
}

/**
 * @tc.name: RenderNodeGraphShareManagerTest
 * @tc.desc: Tests for RenderNodeGraphShareManager, which is a wrapper for RenderNodeGraphShareDataManager with a
 * specific render node it manages, by veryfying wrapper functions produce expected results.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeGraphShareManager, RenderNodeGraphShareManagerTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);
    TestRenderGraphShareManager(engine);
    UTest::DestroyEngine(engine);
}
