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

#ifndef RENDER_NODE_RENDER_NODE_COPY_UTIL_H
#define RENDER_NODE_RENDER_NODE_COPY_UTIL_H

#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/nodecontext/intf_render_node_copy_util.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;

class RenderNodeCopyUtil final : public IRenderNodeCopyUtil {
public:
    RenderNodeCopyUtil() = default;
    ~RenderNodeCopyUtil() override = default;

    void Init(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecute() override;
    void Execute(IRenderCommandList& cmdList, const CopyInfo& copyInfo) override;

    DescriptorCounts GetRenderDescriptorCounts() const override;

    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override;

    void Ref() override;
    void Unref() override;

private:
    IRenderNodeContextManager* renderNodeContextMgr_;

    CopyInfo copyInfo_;
    struct RenderDataHandles {
        RenderHandle shader;
        PipelineLayout pipelineLayout;
        RenderHandle pso;
        RenderHandle sampler;

        RenderHandle shaderLayer;
        PipelineLayout pipelineLayoutLayer;
        RenderHandle psoLayer;
    };
    RenderDataHandles renderData_;

    IDescriptorSetBinder::Ptr binder_;

    uint32_t refCount_ { 0U };
};
RENDER_END_NAMESPACE()

#endif // RENDER_NODE_RENDER_NODE_COPY_UTIL_H
