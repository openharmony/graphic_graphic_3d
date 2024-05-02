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

#ifndef CORE3D_RENDER__NODE__RENDER_COPY_HELPER_H
#define CORE3D_RENDER__NODE__RENDER_COPY_HELPER_H

#include <3d/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

CORE3D_BEGIN_NAMESPACE()
class RenderCopyHelper final {
public:
    RenderCopyHelper() = default;
    ~RenderCopyHelper() = default;

    struct CopyInfo {
        RENDER_NS::RenderHandle input;
        RENDER_NS::RenderHandle output;
        RENDER_NS::RenderHandle sampler; // if not given linear clamp is used
    };
    void Init(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, const CopyInfo& copyInfo);
    void PreExecute(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, const CopyInfo& copyInfo);
    void Execute(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, RENDER_NS::IRenderCommandList& cmdList);

    RENDER_NS::DescriptorCounts GetDescriptorCounts() const;

private:
    CopyInfo copyInfo_;
    struct RenderDataHandles {
        RENDER_NS::RenderHandle shader;
        RENDER_NS::PipelineLayout pipelineLayout;
        RENDER_NS::RenderHandle pso;
        RENDER_NS::RenderHandle sampler;
    };
    RenderDataHandles renderData_;

    RENDER_NS::IDescriptorSetBinder::Ptr binder_;
};
CORE3D_END_NAMESPACE()

#endif // CORE3D_RENDER__NODE__RENDER_COPY_HELPER_H
