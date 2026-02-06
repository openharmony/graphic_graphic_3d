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

#include <nodecontext/render_node_graph_manager.h>
#include <nodecontext/render_node_manager.h>

#include <render/nodecontext/intf_render_node.h>
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
class RenderNodeDummy final : public IRenderNode {
public:
    RenderNodeDummy() = default;
    ~RenderNodeDummy() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override {};
    void PreExecuteFrame() override {};
    void ExecuteFrame(IRenderCommandList& cmdList) override {};
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "dc5da6ef-0214-4275-b7a3-c68421e76313" };
    static constexpr const char* TYPE_NAME = "RenderNodeDummy";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);
};

IRenderNode* RenderNodeDummy::Create()
{
    return new RenderNodeDummy();
}

void RenderNodeDummy::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDummy*>(instance);
}

class RenderNodeDummy2 final : public IRenderNode {
public:
    RenderNodeDummy2() = default;
    ~RenderNodeDummy2() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override {};
    void PreExecuteFrame() override {};
    void ExecuteFrame(IRenderCommandList& cmdList) override {};
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "8e85e0d0-4617-42c1-8e5e-5a125c33de12" };
    static constexpr const char* TYPE_NAME = "RenderNodeDummy2";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);
};

IRenderNode* RenderNodeDummy2::Create()
{
    return new RenderNodeDummy2;
}

void RenderNodeDummy2::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDummy2*>(instance);
}

void TestRenderNodeManager(const UTest::EngineResources& engine)
{
    RenderNodeGraphManager& renderNodeGraphMgr =
        static_cast<RenderNodeGraphManager&>(engine.context->GetRenderNodeGraphManager());
    RenderNodeManager& renderNodeMgr = renderNodeGraphMgr.GetRenderNodeManager();
    RenderNodeTypeInfo info { { RenderNodeTypeInfo::UID }, RenderNodeDummy::UID, RenderNodeDummy::TYPE_NAME,
        RenderNodeDummy::Create, RenderNodeDummy::Destroy, RenderNodeDummy::BACKEND_FLAGS,
        RenderNodeDummy::CLASS_TYPE };
    renderNodeMgr.AddRenderNodeFactory(info);
    {
        auto flags = renderNodeMgr.GetRenderNodeTypeInfoFlags("RenderNodeDummy");
        ASSERT_EQ(RenderNodeDummy::BACKEND_FLAGS, flags.backendFlags);
        ASSERT_EQ(RenderNodeDummy::CLASS_TYPE, flags.classType);
    }
    {
        auto node = renderNodeMgr.Create("RenderNodeDummy");
        ASSERT_NE(nullptr, node);
        renderNodeMgr.Destroy("RenderNodeDummy", node);
    }
    {
        auto node = renderNodeMgr.CreateRenderNode("RenderNodeDummy");
        ASSERT_NE(nullptr, node.get());
        node.reset();
    }
    renderNodeMgr.RemoveRenderNodeFactory(info);
    {
        auto flags = renderNodeMgr.GetRenderNodeTypeInfoFlags("RenderNodeDummy");
        ASSERT_EQ(0, flags.backendFlags);
        ASSERT_EQ(0, flags.classType);
    }
    {
        auto node = renderNodeMgr.Create("RenderNodeDummy");
        ASSERT_EQ(nullptr, node);
    }
    {
        auto node = renderNodeMgr.CreateRenderNode("RenderNodeDummy");
        ASSERT_EQ(nullptr, node.get());
    }
}

void TestRenderNodeGraphPatching(const UTest::EngineResources& engine)
{
    auto& renderNodeGraphMgr = engine.context->GetRenderNodeGraphManager();
    static constexpr RenderNodeTypeInfo info { { RenderNodeTypeInfo::UID }, RenderNodeDummy::UID,
        RenderNodeDummy::TYPE_NAME, RenderNodeDummy::Create, RenderNodeDummy::Destroy, RenderNodeDummy::BACKEND_FLAGS,
        RenderNodeDummy::CLASS_TYPE };
    static constexpr RenderNodeTypeInfo info2 { { RenderNodeTypeInfo::UID }, RenderNodeDummy2::UID,
        RenderNodeDummy2::TYPE_NAME, RenderNodeDummy2::Create, RenderNodeDummy2::Destroy,
        RenderNodeDummy2::BACKEND_FLAGS, RenderNodeDummy2::CLASS_TYPE };
    CORE_NS::GetPluginRegister().RegisterTypeInfo(info);
    CORE_NS::GetPluginRegister().RegisterTypeInfo(info2);

    RenderNodeGraphDesc rngDesc { "TestGraph", "TestGraphDataStore", "", {}, {} };
    rngDesc.nodes.push_back(RenderNodeDesc { RenderNodeDummy::TYPE_NAME, "MyFirstDummyNode", {}, {} });
    rngDesc.nodes.push_back(RenderNodeDesc { RenderNodeDummy::TYPE_NAME, "MySecondDummyNode", {}, {} });

    const auto insertDesc = RenderNodeDesc { RenderNodeDummy2::TYPE_NAME, "MyDummyNode2", {}, {} };
    RenderNodeDependency dependencies[] = { RenderNodeDependency {
        "RenderNodeDummy", RenderNodeDependency ::Position::AFTER_FIRST } };
    {
        renderNodeGraphMgr.AddRenderNodeInsertion(insertDesc, dependencies);
        auto patchedRngDesc = renderNodeGraphMgr.PatchRenderNodeGraph(rngDesc);
        EXPECT_EQ(patchedRngDesc.nodes.size(), 3U);
        EXPECT_EQ(patchedRngDesc.nodes[0U].typeName, RenderNodeDummy::TYPE_NAME);
        EXPECT_EQ(patchedRngDesc.nodes[1U].typeName, RenderNodeDummy2::TYPE_NAME);
        EXPECT_EQ(patchedRngDesc.nodes[2U].typeName, RenderNodeDummy::TYPE_NAME);
    }
    {
        dependencies[0U].position = RenderNodeDependency ::Position::AFTER_LAST;
        renderNodeGraphMgr.AddRenderNodeInsertion(insertDesc, dependencies);
        auto patchedRngDesc = renderNodeGraphMgr.PatchRenderNodeGraph(rngDesc);
        EXPECT_EQ(patchedRngDesc.nodes.size(), 3U);
        EXPECT_EQ(patchedRngDesc.nodes[0U].typeName, RenderNodeDummy::TYPE_NAME);
        EXPECT_EQ(patchedRngDesc.nodes[1U].typeName, RenderNodeDummy::TYPE_NAME);
        EXPECT_EQ(patchedRngDesc.nodes[2U].typeName, RenderNodeDummy2::TYPE_NAME);
    }
    {
        dependencies[0U].position = RenderNodeDependency ::Position::BEFORE_FIRST;
        renderNodeGraphMgr.AddRenderNodeInsertion(insertDesc, dependencies);
        auto patchedRngDesc = renderNodeGraphMgr.PatchRenderNodeGraph(rngDesc);
        EXPECT_EQ(patchedRngDesc.nodes.size(), 3U);
        EXPECT_EQ(patchedRngDesc.nodes[0U].typeName, RenderNodeDummy2::TYPE_NAME);
        EXPECT_EQ(patchedRngDesc.nodes[1U].typeName, RenderNodeDummy::TYPE_NAME);
        EXPECT_EQ(patchedRngDesc.nodes[2U].typeName, RenderNodeDummy::TYPE_NAME);
    }
    {
        dependencies[0U].position = RenderNodeDependency ::Position::BEFORE_LAST;
        renderNodeGraphMgr.AddRenderNodeInsertion(insertDesc, dependencies);
        auto patchedRngDesc = renderNodeGraphMgr.PatchRenderNodeGraph(rngDesc);
        EXPECT_EQ(patchedRngDesc.nodes.size(), 3U);
        EXPECT_EQ(patchedRngDesc.nodes[0U].typeName, RenderNodeDummy::TYPE_NAME);
        EXPECT_EQ(patchedRngDesc.nodes[1U].typeName, RenderNodeDummy2::TYPE_NAME);
        EXPECT_EQ(patchedRngDesc.nodes[2U].typeName, RenderNodeDummy::TYPE_NAME);
    }
    renderNodeGraphMgr.RemoveRenderNodeInsertion(insertDesc);
    CORE_NS::GetPluginRegister().UnregisterTypeInfo(info2);
    CORE_NS::GetPluginRegister().UnregisterTypeInfo(info);
}
} // namespace

/**
 * @tc.name: RenderNodeManagerTest
 * @tc.desc: Tests RenderNodeManager by registering a custom rendernode to the rendernode factory and creating the
 * custom render node.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeManager, RenderNodeManagerTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);
    TestRenderNodeManager(engine);
    UTest::DestroyEngine(engine);
}

/**
 * @tc.name: RenderNodeGraphPatchTest
 * @tc.desc: Tests RenderNodeManager by creating a render node graph wih custm nodes and patching the graph by inserting
 * a custom node.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeManager, RenderNodeGraphPatchTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);
    TestRenderNodeGraphPatching(engine);
    UTest::DestroyEngine(engine);
}
