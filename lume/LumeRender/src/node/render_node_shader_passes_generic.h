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

#ifndef RENDER_RENDER__NODE__RENDER_NODE_FULLSCREEN_SHADER_BINDER_GENERIC_H
#define RENDER_RENDER__NODE__RENDER_NODE_FULLSCREEN_SHADER_BINDER_GENERIC_H

#include <base/util/uid.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "datastore/render_data_store_shader_passes.h"

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;
struct RenderNodeGraphInputs;

class RenderNodeShaderPassesGeneric final : public IRenderNode {
public:
    RenderNodeShaderPassesGeneric() = default;
    ~RenderNodeShaderPassesGeneric() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "2e258acc-caad-4035-9ab1-c6debca7f6fa" };
    static constexpr char const* TYPE_NAME = "RenderNodeShaderPassesGeneric";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };
    const RenderDataStoreShaderPasses* dsShaderPasses_ { nullptr };

    void ProcessExecuteData();
    void ExecuteFrameGraphics(IRenderCommandList& cmdList);
    void ExecuteFrameCompute(IRenderCommandList& cmdList);

    void ParseRenderNodeInputs();
    RenderHandle GetPsoHandleGraphics(
        const RenderPass& renderPass, const RenderHandle& shader, const PipelineLayout& pipelineLayout);
    RenderHandle GetPsoHandleCompute(const RenderHandle& shader, const PipelineLayout& pipelineLayout);
    bool ValidRenderPass(const RenderPassWithHandleReference& renderPass);

    // returns buffer offset
    uint32_t WriteLocalUboData(BASE_NS::array_view<const uint8_t> uboData);

    // Json resources which might need re-fetching
    struct JsonInputs {
        RenderNodeGraphInputs::RenderDataStore renderDataStore;
    };
    JsonInputs jsonInputs_;

    bool valid_ { false };
    struct LocalUboData {
        uint32_t byteSize { 0U };
        RenderHandleReference handle;

        uint8_t* mapData { nullptr };
        uint32_t currentOffset { 0U };
    };
    LocalUboData uboData_;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_FULLSCREEN_GENERIC_H