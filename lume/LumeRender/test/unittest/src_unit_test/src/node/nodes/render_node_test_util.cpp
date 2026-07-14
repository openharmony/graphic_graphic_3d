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

#include "node/nodes/render_node_test_util.h"

#include <base/math/vector.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/pipeline_state_desc.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "test_framework.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()

void RenderNodeTestUtil::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    valid_ = true;

    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();
}

void RenderNodeTestUtil::PreExecuteFrame()
{}

void RenderNodeTestUtil::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    const RenderPass renderPass = renderNodeUtil.CreateRenderPass(inputRenderPass_);
    const ViewportDesc viewportDesc = renderNodeUtil.CreateDefaultViewport(renderPass);
    const ScissorDesc scissorDesc = renderNodeUtil.CreateDefaultScissor(renderPass);

    IRenderDataStorePostProcess::GlobalFactors factors;
    auto result = renderNodeUtil.GetRenderPostProcessConfiguration(factors);
    ASSERT_EQ(factors.userFactors[0], result.userFactors[0]);

    RenderNodeGraphInputs::InputResources resources;
    RenderNodeGraphInputs::Resource image;
    image.binding = 0u;
    image.set = 0u;
    resources.images.push_back(image);
    RenderNodeGraphInputs::Resource sampler;
    sampler.binding = 0u;
    sampler.set = 0u;
    resources.samplers.push_back(sampler);
    auto res = renderNodeUtil.CreateInputResources(resources);
    ASSERT_EQ(1, res.images.size());
    ASSERT_EQ(0, res.samplers.size());

    RenderNodeHandles::InputRenderPass rp;
    rp.subpassIndex = 4u;
    rp.subpassCount = 1u;
    rp.attachments.push_back({});
    auto outRenderPass = renderNodeUtil.CreateRenderPass(rp);
    ASSERT_EQ(1, outRenderPass.renderPassDesc.attachmentCount);

    // More attachments/indices than the fixed arrays hold must clamp, not overflow.
    RenderNodeHandles::InputRenderPass overflowRp;
    overflowRp.subpassIndex = 0u;
    overflowRp.subpassCount = 1u;
    const uint32_t overflow = PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT * 4u;
    for (uint32_t i = 0; i < overflow; ++i) {
        overflowRp.attachments.push_back({});
        overflowRp.inputAttachmentIndices.push_back(i);
        overflowRp.colorAttachmentIndices.push_back(i);
        overflowRp.resolveAttachmentIndices.push_back(i);
    }
    const auto clampedRenderPass = renderNodeUtil.CreateRenderPass(overflowRp);
    ASSERT_EQ(
        PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT, clampedRenderPass.renderPassDesc.attachmentCount);
    ASSERT_LE(clampedRenderPass.subpassDesc.inputAttachmentCount, PipelineStateConstants::MAX_INPUT_ATTACHMENT_COUNT);
    ASSERT_LE(clampedRenderPass.subpassDesc.colorAttachmentCount, PipelineStateConstants::MAX_COLOR_ATTACHMENT_COUNT);
    ASSERT_LE(
        clampedRenderPass.subpassDesc.resolveAttachmentCount, PipelineStateConstants::MAX_RESOLVE_ATTACHMENT_COUNT);

    // Exercise the array_view overload of GetDescriptorCounts (the PipelineLayout
    // overload is covered by the default render nodes; this one has no in-tree caller).
    DescriptorSetLayoutBinding bindings[2] = {};
    bindings[0].descriptorType = CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1U;
    bindings[1].descriptorType = CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    bindings[1].descriptorCount = 3U;
    const DescriptorCounts dc = renderNodeUtil.GetDescriptorCounts({bindings, 2U});
    ASSERT_EQ(2U, dc.counts.size());
    ASSERT_EQ(CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, dc.counts[0].type);
    ASSERT_EQ(1U, dc.counts[0].count);
    ASSERT_EQ(CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE, dc.counts[1].type);
    ASSERT_EQ(3U, dc.counts[1].count);
}

void RenderNodeTestUtil::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    inputRenderPassJson_ = parserUtil.GetInputRenderPass(jsonVal, "renderPass");

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(inputRenderPassJson_);
}

// for plugin / factory interface
IRenderNode* RenderNodeTestUtil::Create()
{
    return new RenderNodeTestUtil();
}

void RenderNodeTestUtil::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeTestUtil*>(instance);
}
RENDER_END_NAMESPACE()