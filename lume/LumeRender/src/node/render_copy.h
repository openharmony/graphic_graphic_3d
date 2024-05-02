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

#ifndef RENDER_RENDER__NODE__RENDER_COPY_H
#define RENDER_RENDER__NODE__RENDER_COPY_H

#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;

class RenderCopy final {
public:
    RenderCopy() = default;
    ~RenderCopy() = default;

    enum class CopyType : uint32_t {
        BASIC_COPY = 0,
        LAYER_COPY = 1,
    };

    struct CopyInfo {
        BindableImage input;
        BindableImage output;
        RenderHandle sampler; // if not given linear clamp is used
        CopyType copyType { CopyType::BASIC_COPY };
    };
    void Init(IRenderNodeContextManager& renderNodeContextMgr, const CopyInfo& copyInfo);
    void PreExecute(IRenderNodeContextManager& renderNodeContextMgr, const CopyInfo& copyInfo);
    void Execute(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList);

    DescriptorCounts GetDescriptorCounts() const;

private:
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
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_COPY_H
