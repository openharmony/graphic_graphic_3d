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

#ifndef RENDER_RENDER__NODE__RENDER_NODE_BLOOM_H
#define RENDER_RENDER__NODE__RENDER_NODE_BLOOM_H

#include <base/util/uid.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>

#include "node/render_bloom.h"

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;
class IRenderNodeContextManager;
class IPipelineDescriptorSetBinder;
class IRenderNodeRenderDataStoreManager;
struct RenderNodeGraphInputs;

class RenderNodeBloom final : public IRenderNode {
public:
    RenderNodeBloom() = default;
    ~RenderNodeBloom() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(IRenderCommandList& cmdList) override;

    ExecuteFlags GetExecuteFlags() const override;

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "cc25f8bd-bad6-48f9-8cb2-af24681643b6" };
    static constexpr char const* TYPE_NAME = "RenderNodeBloom";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void ParseRenderNodeInputs();
    void ProcessPostProcessConfiguration(const IRenderNodeRenderDataStoreManager& dataStoreMgr);
    void UpdatePostProcessData(const PostProcessConfiguration& postProcessConfiguration);

    // Json resources which might need re-fetching
    struct JsonInputs {
        RenderNodeGraphInputs::InputResources resources;
        RenderNodeGraphInputs::RenderDataStore renderDataStore;

        bool hasChangeableResourceHandles { false };
    };
    JsonInputs jsonInputs_;
    RenderNodeHandles::InputRenderPass inputRenderPass_;
    RenderNodeHandles::InputResources inputResources_;

    RenderBloom renderBloom_;
    PostProcessConfiguration ppConfig_;
    RenderHandleReference postProcessUbo_;

    bool valid_ { false };
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_BLOOM_H
