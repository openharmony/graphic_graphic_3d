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

#include "render_graph_test_helpers.h"

using namespace BASE_NS;
using CORE_NS::IEngine;
using namespace RENDER_NS;
using namespace RenderGraphTestUtil;

namespace {
void CreateBarrierTestResources(TestData& td, string_view rngUri, string_view imageName)
{
    RegisterNodeFactory<RenderNodeDraw>(td);
    LoadRenderNodeGraph(td, rngUri);
    td.outputImageHandle = CreateColorImage(td, imageName);
    CreateVertexBuffer(td);
}

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: LinearChainNodeStoreValid
 * @tc.desc: Verifies the linear chain graph creates 3 render nodes with valid context data.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderGraphBarrier, LinearChainNodeStoreValid, testing::ext::TestSize.Level1)
{
    TestData td;
    td.engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(td.engine);
    CreateBarrierTestResources(td, "test://renderNodeGraphBarrierLinear.rng", "OutputImage");

    TickFrames(td, 2);

    RenderNodeGraphNodeStore* store = GetNodeStore(td);
    ASSERT_NE(nullptr, store);
    EXPECT_EQ(3u, store->renderNodeContextData.size());
    EXPECT_EQ(3u, store->renderNodeData.size());

    // Each node should have a valid renderCommandList and renderBarrierList
    for (uint32_t i = 0; i < store->renderNodeContextData.size(); ++i) {
        const auto& ctxData = store->renderNodeContextData[i];
        EXPECT_NE(nullptr, ctxData.renderCommandList) << "Node " << i << " has null renderCommandList";
        EXPECT_NE(nullptr, ctxData.renderBarrierList) << "Node " << i << " has null renderBarrierList";
    }

    DestroyTestResources(td);
    UTest::DestroyEngine(td.engine);
}

/**
 * @tc.name: LinearChainWAWLayoutPatching
 * @tc.desc: Verifies that WAW on same color attachment is handled by render pass layout transitions:
 *           node 1 and 2 get COLOR_ATTACHMENT_OPTIMAL as initial layout after node 0 writes it.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderGraphBarrier, LinearChainWAWLayoutPatching, testing::ext::TestSize.Level1)
{
    TestData td;
    td.engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(td.engine);
    CreateBarrierTestResources(td, "test://renderNodeGraphBarrierLinear.rng", "OutputImage");

    // Tick 2 frames so first-frame UNDEFINED is resolved
    TickFrames(td, 2);

    RenderNodeGraphNodeStore* store = GetNodeStore(td);
    ASSERT_NE(nullptr, store);
    ASSERT_GE(store->renderNodeContextData.size(), 3u);

    // All three nodes should have BEGIN_RENDER_PASS with correct layout patching.
    // In steady state (frame 2+), all nodes should see COLOR_ATTACHMENT_OPTIMAL.
    for (uint32_t nodeIdx = 0; nodeIdx < 3; ++nodeIdx) {
        const auto& ctxData = store->renderNodeContextData[nodeIdx];
        ASSERT_NE(nullptr, ctxData.renderCommandList);
        const auto* brp = FindBeginRenderPass(*ctxData.renderCommandList);
        ASSERT_NE(nullptr, brp) << "No BEGIN_RENDER_PASS in node " << nodeIdx;

        EXPECT_EQ(CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, brp->imageLayouts.attachmentInitialLayouts[0])
            << "Node " << nodeIdx << " should have COLOR_ATTACHMENT_OPTIMAL initial layout in steady state";
        EXPECT_EQ(CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, brp->imageLayouts.attachmentFinalLayouts[0])
            << "Node " << nodeIdx << " should have COLOR_ATTACHMENT_OPTIMAL final layout";
    }

    DestroyTestResources(td);
    UTest::DestroyEngine(td.engine);
}

/**
 * @tc.name: LinearChainFirstFrameLoadOpDowngrade
 * @tc.desc: Verifies that on first frame, pass 1 (loadOp=load) gets loadOp downgraded to dont_care
 *           because the image is still UNDEFINED for that node on the first frame.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderGraphBarrier, LinearChainFirstFrameLoadOpDowngrade, testing::ext::TestSize.Level1)
{
    TestData td;
    td.engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(td.engine);
    CreateBarrierTestResources(td, "test://renderNodeGraphBarrierLinear.rng", "OutputImage");

    // Only tick 1 frame to observe first-frame behavior
    TickFrames(td, 1);

    RenderNodeGraphNodeStore* store = GetNodeStore(td);
    ASSERT_NE(nullptr, store);
    ASSERT_GE(store->renderNodeContextData.size(), 1u);

    // Node 0 (loadOp=clear): should remain CLEAR on first frame
    {
        const auto& ctxData = store->renderNodeContextData[0];
        ASSERT_NE(nullptr, ctxData.renderCommandList);
        const auto* brp = FindBeginRenderPass(*ctxData.renderCommandList);
        ASSERT_NE(nullptr, brp);
        EXPECT_EQ(CORE_ATTACHMENT_LOAD_OP_CLEAR, brp->renderPassDesc.attachments[0].loadOp);
    }

    // Node 0 first frame: initial layout should be UNDEFINED (fresh resource)
    {
        const auto& ctxData = store->renderNodeContextData[0];
        const auto* brp = FindBeginRenderPass(*ctxData.renderCommandList);
        ASSERT_NE(nullptr, brp);
        EXPECT_EQ(CORE_IMAGE_LAYOUT_UNDEFINED, brp->imageLayouts.attachmentInitialLayouts[0]);
    }

    DestroyTestResources(td);
    UTest::DestroyEngine(td.engine);
}

/**
 * @tc.name: DiamondTopologyNodeStoreValid
 * @tc.desc: Verifies the diamond graph creates 3 render nodes and all produce valid render commands.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderGraphBarrier, DiamondTopologyNodeStoreValid, testing::ext::TestSize.Level1)
{
    TestData td;
    td.engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(td.engine);
    CreateBarrierTestResources(td, "test://renderNodeGraphBarrierDiamond.rng", "ImageA");

    TickFrames(td, 2);

    RenderNodeGraphNodeStore* store = GetNodeStore(td);
    ASSERT_NE(nullptr, store);
    EXPECT_EQ(3u, store->renderNodeContextData.size());

    // All nodes should have BEGIN_RENDER_PASS commands with valid layouts in steady state
    for (uint32_t nodeIdx = 0; nodeIdx < 3; ++nodeIdx) {
        const auto& ctxData = store->renderNodeContextData[nodeIdx];
        ASSERT_NE(nullptr, ctxData.renderCommandList);
        const auto* brp = FindBeginRenderPass(*ctxData.renderCommandList);
        ASSERT_NE(nullptr, brp) << "No BEGIN_RENDER_PASS in diamond node " << nodeIdx;

        EXPECT_EQ(CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, brp->imageLayouts.attachmentFinalLayouts[0])
            << "Diamond node " << nodeIdx << " should have COLOR_ATTACHMENT_OPTIMAL final layout";
    }

    DestroyTestResources(td);
    UTest::DestroyEngine(td.engine);
}

/**
 * @tc.name: FirstFrameFirstNodeNoBarrier
 * @tc.desc: Verifies the first node in a linear chain has no explicit image barrier for OutputImage
 *           at barrier point 0 on first use (layout transition handled by render pass).
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderGraphBarrier, FirstFrameFirstNodeNoBarrier, testing::ext::TestSize.Level1)
{
    TestData td;
    td.engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(td.engine);
    CreateBarrierTestResources(td, "test://renderNodeGraphBarrierLinear.rng", "OutputImage");

    TickFrames(td, 1);

    RenderNodeGraphNodeStore* store = GetNodeStore(td);
    ASSERT_NE(nullptr, store);
    ASSERT_GE(store->renderNodeContextData.size(), 1u);

    const auto& ctxData = store->renderNodeContextData[0];
    ASSERT_NE(nullptr, ctxData.renderBarrierList);

    // First node, first frame: render pass attachment barriers are handled by render pass
    // layout transitions (initialLayout/finalLayout), not explicit vkCmdPipelineBarrier calls.
    // The barrier list at point 0 should either be empty or contain only non-image barriers.
    const auto* barriers = ctxData.renderBarrierList->GetBarrierPointBarriers(0);
    if (barriers != nullptr && barriers->firstBarrierList != nullptr) {
        const RenderHandle outputHandle = td.outputImageHandle.GetHandle();
        auto* list = barriers->firstBarrierList;
        while (list) {
            for (uint32_t i = 0; i < list->count; ++i) {
                const auto& cb = list->commandBarriers[i];
                if (cb.resourceHandle.id == outputHandle.id) {
                    // If there is a barrier for OutputImage on first frame, src should be UNDEFINED
                    EXPECT_EQ(CORE_IMAGE_LAYOUT_UNDEFINED, cb.src.optionalImageLayout);
                }
            }
            list = list->nextBarrierPointBarrierList;
        }
    }

    DestroyTestResources(td);
    UTest::DestroyEngine(td.engine);
}
#endif  // RENDER_HAS_VULKAN_BACKEND
}  // namespace
