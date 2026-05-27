/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <nodecontext/render_barrier_list.h>

#include "node/nodes/render_node_depth_stencil.h"
#include "render_graph_test_helpers.h"

using namespace BASE_NS;
using CORE_NS::IEngine;
using namespace RENDER_NS;
using namespace RenderGraphTestUtil;

namespace {
void CreateLayoutTestResources(TestData& td, string_view rngUri, bool withDepth)
{
    RegisterNodeFactory<RenderNodeDraw>(td);
    if (withDepth) {
        RegisterNodeFactory<RenderNodeDepthStencil>(td);
    }
    LoadRenderNodeGraph(td, rngUri);
    td.outputImageHandle = CreateColorImage(td, "OutputImage");
    if (withDepth) {
        td.depthImageHandle = CreateDepthImage(td, "DepthImage");
    }
    CreateVertexBuffer(td);
}

// Reproduces Phase 5 scenario: debug marker before render pass shifts command indices
class RenderNodeDrawWithDebugMarker final : public IRenderNode {
public:
    RenderNodeDrawWithDebugMarker() = default;
    ~RenderNodeDrawWithDebugMarker() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override
    {
        inner_.InitNode(renderNodeContextMgr);
    }

    void PreExecuteFrame() override
    {
        inner_.PreExecuteFrame();
    }

    void ExecuteFrame(IRenderCommandList& cmdList) override;

    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    static constexpr BASE_NS::Uid UID{"b7e23f01-9a45-4c12-8d6e-f1a2b3c4d5e6"};
    static constexpr const char* TYPE_NAME = "RenderNodeDrawWithDebugMarker";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;

    static IRenderNode* Create()
    {
        return new RenderNodeDrawWithDebugMarker();
    }

    static void Destroy(IRenderNode* instance)
    {
        delete static_cast<RenderNodeDrawWithDebugMarker*>(instance);
    }

private:
    RenderNodeDraw inner_;
};

void RenderNodeDrawWithDebugMarker::ExecuteFrame(IRenderCommandList& cmdList)
{
    // Insert debug marker BEFORE the render pass — this shifts
    // command indices and triggers the Phase 5 barrier point bug
    // if the fix is reverted.
    cmdList.BeginDebugMarker("OITPass", BASE_NS::Math::Vec4{1.0f, 0.0f, 0.0f, 1.0f});
    inner_.ExecuteFrame(cmdList);
    cmdList.EndDebugMarker();
}

void CreateOITTestResources(TestData& td, string_view rngUri, bool withDebugMarkerNode)
{
    RegisterNodeFactory<RenderNodeDraw>(td);

    if (withDebugMarkerNode) {
        RegisterNodeFactory<RenderNodeDrawWithDebugMarker>(td);
    }

    LoadRenderNodeGraph(td, rngUri);

    // OIT accumulation image (RGBA16_SFLOAT, matching real OIT sample)
    CreateColorImage(td, "OITAccImage", BASE_FORMAT_R16G16B16A16_SFLOAT);

    // OIT revealage image
    CreateColorImage(td, "OITRevImage", BASE_FORMAT_R16G16B16A16_SFLOAT);

    // Compositing output image
    td.outputImageHandle = CreateColorImage(td, "OutputImage");

    CreateVertexBuffer(td);
}

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: FirstFrameUndefinedToColorAttachment
 * @tc.desc: Verifies first frame transitions color attachment from UNDEFINED to COLOR_ATTACHMENT_OPTIMAL.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderGraphLayout, FirstFrameUndefinedToColorAttachment, testing::ext::TestSize.Level1)
{
    TestData td;
    td.engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(td.engine);
    CreateLayoutTestResources(td, "test://renderNodeGraphBarrierLinear.rng", false);

    TickFrames(td, 1);

    RenderNodeGraphNodeStore* store = GetNodeStore(td);
    ASSERT_NE(nullptr, store);
    ASSERT_GE(store->renderNodeContextData.size(), 1u);

    const auto& ctxData = store->renderNodeContextData[0];
    ASSERT_NE(nullptr, ctxData.renderCommandList);
    const auto* brp = FindBeginRenderPass(*ctxData.renderCommandList);
    ASSERT_NE(nullptr, brp) << "No BEGIN_RENDER_PASS command found in node 0";

    // First frame: image starts UNDEFINED
    EXPECT_EQ(CORE_IMAGE_LAYOUT_UNDEFINED, brp->imageLayouts.attachmentInitialLayouts[0]);
    EXPECT_EQ(CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, brp->imageLayouts.attachmentFinalLayouts[0]);

    DestroyTestResources(td);
    UTest::DestroyEngine(td.engine);
}

/**
 * @tc.name: SteadyStateColorAttachment
 * @tc.desc: Verifies steady-state frames have COLOR_ATTACHMENT_OPTIMAL initial layout.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderGraphLayout, SteadyStateColorAttachment, testing::ext::TestSize.Level1)
{
    TestData td;
    td.engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(td.engine);
    CreateLayoutTestResources(td, "test://renderNodeGraphBarrierLinear.rng", false);

    // Tick 3 frames: frame 1 starts UNDEFINED, frames 2+ are steady-state
    TickFrames(td, 3);

    RenderNodeGraphNodeStore* store = GetNodeStore(td);
    ASSERT_NE(nullptr, store);
    ASSERT_GE(store->renderNodeContextData.size(), 1u);

    const auto& ctxData = store->renderNodeContextData[0];
    ASSERT_NE(nullptr, ctxData.renderCommandList);
    const auto* brp = FindBeginRenderPass(*ctxData.renderCommandList);
    ASSERT_NE(nullptr, brp) << "No BEGIN_RENDER_PASS command found in node 0";

    // After multiple frames, the image should already be in COLOR_ATTACHMENT_OPTIMAL
    EXPECT_EQ(CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, brp->imageLayouts.attachmentInitialLayouts[0]);
    EXPECT_EQ(CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, brp->imageLayouts.attachmentFinalLayouts[0]);

    DestroyTestResources(td);
    UTest::DestroyEngine(td.engine);
}

/**
 * @tc.name: LoadOpDowngradeOnUndefined
 * @tc.desc: Verifies Pass1 sees COLOR_ATTACHMENT_OPTIMAL initial layout after Pass0 clears on first frame.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderGraphLayout, LoadOpDowngradeOnUndefined, testing::ext::TestSize.Level1)
{
    TestData td;
    td.engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(td.engine);
    CreateLayoutTestResources(td, "test://renderNodeGraphBarrierLinear.rng", false);

    TickFrames(td, 1);

    RenderNodeGraphNodeStore* store = GetNodeStore(td);
    ASSERT_NE(nullptr, store);
    ASSERT_GE(store->renderNodeContextData.size(), 2u);

    // Pass1 (node index 1) loads from the image that Pass0 cleared.
    // After Pass0's render pass, the image is in COLOR_ATTACHMENT_OPTIMAL.
    const auto& ctxData = store->renderNodeContextData[1];
    ASSERT_NE(nullptr, ctxData.renderCommandList);
    const auto* brp = FindBeginRenderPass(*ctxData.renderCommandList);
    ASSERT_NE(nullptr, brp) << "No BEGIN_RENDER_PASS command found in node 1";

    // Pass1 should see the image as COLOR_ATTACHMENT_OPTIMAL (left by Pass0)
    EXPECT_EQ(CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, brp->imageLayouts.attachmentInitialLayouts[0]);

    DestroyTestResources(td);
    UTest::DestroyEngine(td.engine);
}

/**
 * @tc.name: DepthAttachmentLayout
 * @tc.desc: Verifies depth attachment transitions to DEPTH_STENCIL_ATTACHMENT_OPTIMAL.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderGraphLayout, DepthAttachmentLayout, testing::ext::TestSize.Level1)
{
    TestData td;
    td.engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(td.engine);
    CreateLayoutTestResources(td, "test://renderNodeGraphBarrierDepth.rng", true);

    TickFrames(td, 2);

    RenderNodeGraphNodeStore* store = GetNodeStore(td);
    ASSERT_NE(nullptr, store);
    ASSERT_GE(store->renderNodeContextData.size(), 1u);

    const auto& ctxData = store->renderNodeContextData[0];
    ASSERT_NE(nullptr, ctxData.renderCommandList);
    const auto* brp = FindBeginRenderPass(*ctxData.renderCommandList);
    ASSERT_NE(nullptr, brp) << "No BEGIN_RENDER_PASS command found in depth pass";

    // Depth attachment (index 1) final layout should be DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    EXPECT_EQ(CORE_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, brp->imageLayouts.attachmentFinalLayouts[1]);

    DestroyTestResources(td);
    UTest::DestroyEngine(td.engine);
}

/**
 * @tc.name: LinearChainLastPassFinalLayout
 * @tc.desc: Verifies the last pass in a linear chain has COLOR_ATTACHMENT_OPTIMAL as final layout.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderGraphLayout, LinearChainLastPassFinalLayout, testing::ext::TestSize.Level1)
{
    TestData td;
    td.engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(td.engine);
    CreateLayoutTestResources(td, "test://renderNodeGraphBarrierLinear.rng", false);

    TickFrames(td, 2);

    RenderNodeGraphNodeStore* store = GetNodeStore(td);
    ASSERT_NE(nullptr, store);
    ASSERT_GE(store->renderNodeContextData.size(), 3u);

    // Last node (index 2) in the linear chain
    const auto& ctxData = store->renderNodeContextData[2];
    ASSERT_NE(nullptr, ctxData.renderCommandList);
    const auto* brp = FindBeginRenderPass(*ctxData.renderCommandList);
    ASSERT_NE(nullptr, brp) << "No BEGIN_RENDER_PASS command found in last pass";

    EXPECT_EQ(CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, brp->imageLayouts.attachmentFinalLayouts[0]);

    DestroyTestResources(td);
    UTest::DestroyEngine(td.engine);
}

/**
 * @tc.name: OITMergedSubpassLayout
 * @tc.desc: Verifies merged subpass layout transitions match the OIT topology:
 *           UNDEFINED->COLOR_ATTACHMENT_OPTIMAL through merged passes,
 *           steady-state COLOR_ATTACHMENT_OPTIMAL on compositing.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderGraphLayout, OITMergedSubpassLayout, testing::ext::TestSize.Level1)
{
    TestData td;
    td.engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(td.engine);
    CreateOITTestResources(td, "test://renderNodeGraphOITMergedSubpass.rng", false);

    // Frame 1: UNDEFINED->COLOR_ATTACHMENT_OPTIMAL
    // Frame 2: steady state
    TickFrames(td, 2);

    RenderNodeGraphNodeStore* store = GetNodeStore(td);
    ASSERT_NE(nullptr, store);
    ASSERT_GE(store->renderNodeContextData.size(), 4u);

    // Merged pass node 0: steady-state initial layout should be COLOR_ATTACHMENT_OPTIMAL
    const auto* brp0 = FindBeginRenderPass(*store->renderNodeContextData[0].renderCommandList);
    ASSERT_NE(nullptr, brp0) << "No BEGIN_RENDER_PASS in merged pass node 0";
    EXPECT_EQ(CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, brp0->imageLayouts.attachmentInitialLayouts[0]);
    EXPECT_EQ(CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, brp0->imageLayouts.attachmentInitialLayouts[1]);

    // Compositing pass (node 3): initial layout should be COLOR_ATTACHMENT_OPTIMAL for OutputImage
    const auto* brpC = FindBeginRenderPass(*store->renderNodeContextData[3].renderCommandList);
    ASSERT_NE(nullptr, brpC) << "No BEGIN_RENDER_PASS in compositing node";
    EXPECT_EQ(CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, brpC->imageLayouts.attachmentInitialLayouts[0]);

    DestroyTestResources(td);
    UTest::DestroyEngine(td.engine);
}

/**
 * @tc.name: OITMergedSubpassLayoutWithDebugMarker
 * @tc.desc: Regression test for Phase 5 barrier point index fix.
 *           Debug marker before render pass shifts command indices;
 *           verifies barriers are inserted at the correct barrier point index
 *           (not at the shifted command list index).
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderGraphLayout, OITMergedSubpassLayoutWithDebugMarker, testing::ext::TestSize.Level1)
{
    TestData td;
    td.engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(td.engine);
    CreateOITTestResources(td, "test://renderNodeGraphOITMergedSubpassDebugMarker.rng", true);

    TickFrames(td, 2);

    RenderNodeGraphNodeStore* store = GetNodeStore(td);
    ASSERT_NE(nullptr, store);
    ASSERT_GE(store->renderNodeContextData.size(), 4u);

    // Verify barriers are stored at the correct barrier point index.
    // The Phase 5 bug: debug markers shift command list indices, so
    // (commandListIndex - 1) points to the debug marker instead of the
    // barrier point. Barriers stored at the wrong key are invisible to
    // the Vulkan backend.
    const auto& ctxData0 = store->renderNodeContextData[0];
    const auto* bp = FindBarrierPoint(*ctxData0.renderCommandList);
    ASSERT_NE(nullptr, bp) << "No BARRIER_POINT in debug-marker merged node 0";
    const uint32_t bpIndex = bp->barrierPointIndex;

    ASSERT_NE(nullptr, ctxData0.renderBarrierList.get()) << "No barrier list for node 0";
    EXPECT_TRUE(ctxData0.renderBarrierList->HasBarriers(bpIndex))
        << "Barriers missing at barrier point index " << bpIndex << " — debug marker likely shifted the insertion key";

    const auto* bpBarriers = ctxData0.renderBarrierList->GetBarrierPointBarriers(bpIndex);
    ASSERT_NE(nullptr, bpBarriers) << "GetBarrierPointBarriers returned null for index " << bpIndex;
    EXPECT_GT(bpBarriers->fullCommandBarrierCount, 0u)
        << "Expected barriers for OIT color attachments at barrier point " << bpIndex;

    DestroyTestResources(td);
    UTest::DestroyEngine(td.engine);
}
#endif  // RENDER_HAS_VULKAN_BACKEND
}  // namespace
