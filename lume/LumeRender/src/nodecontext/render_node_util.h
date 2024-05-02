/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef RENDER_RENDER__RENDER_NODE_UTIL_H
#define RENDER_RENDER__RENDER_NODE_UTIL_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/render_data_structures.h>

RENDER_BEGIN_NAMESPACE()
struct RenderableList;
struct PipelineLayout;
class IRenderNodeContextManager;

class RenderNodeUtil final : public IRenderNodeUtil {
public:
    explicit RenderNodeUtil(IRenderNodeContextManager& renderNodeContextMgr);
    ~RenderNodeUtil() override = default;

    RenderNodeHandles::InputRenderPass CreateInputRenderPass(
        const RenderNodeGraphInputs::InputRenderPass& renderPass) const override;
    RenderNodeHandles::InputResources CreateInputResources(
        const RenderNodeGraphInputs::InputResources& inputResources) const override;

    RenderPass CreateRenderPass(const RenderNodeHandles::InputRenderPass& renderPass) const override;
    PipelineLayout CreatePipelineLayout(const RenderHandle& shaderHandle) const override;

    DescriptorCounts GetDescriptorCounts(const PipelineLayout& pipelineLayout) const override;
    DescriptorCounts GetDescriptorCounts(const BASE_NS::array_view<DescriptorSetLayoutBinding> bindings) const override;

    IPipelineDescriptorSetBinder::Ptr CreatePipelineDescriptorSetBinder(
        const PipelineLayout& pipelineLayout) const override;

    void BindResourcesToBinder(const RenderNodeHandles::InputResources& resources,
        IPipelineDescriptorSetBinder& pipelineDescriptorSetBinder) const override;

    ViewportDesc CreateDefaultViewport(const RenderPass& renderPass) const override;
    ScissorDesc CreateDefaultScissor(const RenderPass& renderPass) const override;

    RenderPostProcessConfiguration GetRenderPostProcessConfiguration(
        const PostProcessConfiguration& input) const override;
    RenderPostProcessConfiguration GetRenderPostProcessConfiguration(
        const IRenderDataStorePostProcess::GlobalFactors& globalFactors) const override;

    bool HasChangeableResources(const RenderNodeGraphInputs::InputRenderPass& renderPass) const override;
    bool HasChangeableResources(const RenderNodeGraphInputs::InputResources& resources) const override;

private:
    IRenderNodeContextManager& renderNodeContextMgr_;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__RENDER_NODE_SCENE_UTIL_H
