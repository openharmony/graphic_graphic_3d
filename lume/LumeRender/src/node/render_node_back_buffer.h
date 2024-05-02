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

#ifndef RENDER_RENDER__NODE__RENDER_NODE_BACK_BUFFER_H
#define RENDER_RENDER__NODE__RENDER_NODE_BACK_BUFFER_H

#include <base/containers/string.h>
#include <base/util/uid.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class IRenderNodeRenderDataStoreManager;

class RenderNodeBackBuffer final : public IRenderNode {
public:
    RenderNodeBackBuffer() = default;
    ~RenderNodeBackBuffer() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override {};
    void ExecuteFrame(IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "f1dc030b-1081-4ca5-a195-1d8bfc1a036c" };
    static constexpr char const* TYPE_NAME = "RenderNodeBackBuffer";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void CheckForPsoSpecilization(const PostProcessConfiguration& postProcessConfiguration);
    PostProcessConfiguration GetPostProcessConfiguration(const IRenderNodeRenderDataStoreManager& dataStoreMgr);
    RenderHandle UpdateColorAttachmentSize();
    void ParseRenderNodeInputs();

    // Json resources which might need re-fetching
    struct JsonInputs {
        RenderNodeGraphInputs::InputRenderPass renderPass;
        RenderNodeGraphInputs::InputResources resources;

        RenderNodeGraphInputs::RenderDataStore renderDataStore;

        bool hasChangeableRenderPassHandles { false };
        bool hasChangeableResourceHandles { false };
    };
    JsonInputs jsonInputs_;

    RenderNodeHandles::InputRenderPass inputRenderPass_;
    RenderNodeHandles::InputResources inputResources_;
    RenderHandle shader_;
    RenderHandle psoHandle_;
    RenderPass renderPass_;

    struct BackBufferDefinition {
        uint32_t width { 0 };
        uint32_t height { 0 };
        BASE_NS::Format format { BASE_NS::Format::BASE_FORMAT_UNDEFINED };
    };
    BackBufferDefinition currentBackBuffer_;
    ViewportDesc currentViewportDesc_;
    ScissorDesc currentScissorDesc_;

    RenderPostProcessConfiguration currentRenderPostProcessConfiguration_;
    PipelineLayout pipelineLayout_;
    IPipelineDescriptorSetBinder::Ptr pipelineDescriptorSetBinder_;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_BACK_BUFFER_H
