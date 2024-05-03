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

#ifndef RENDER_RENDER__NODE__RENDER_BLUR_H
#define RENDER_RENDER__NODE__RENDER_BLUR_H

#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>
#include <render/shaders/common/render_blur_common.h>

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;

class RenderBlur final {
public:
    RenderBlur() = default;
    ~RenderBlur() = default;

    struct BlurInfo {
        BindableImage blurTarget;
        RenderHandle globalUbo;
        bool upScale { false };
        uint32_t scaleType { CORE_BLUR_TYPE_DOWNSCALE_RGBA };
        uint32_t blurType { CORE_BLUR_TYPE_RGBA };
    };
    void Init(IRenderNodeContextManager& renderNodeContextMgr, const BlurInfo& blurInfo);
    void PreExecute(IRenderNodeContextManager& renderNodeContextMgr, const BlurInfo& blurInfo,
        const PostProcessConfiguration& ppConfig);
    void Execute(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList,
        const PostProcessConfiguration& ppConfig);

    DescriptorCounts GetDescriptorCounts() const;

private:
    void RenderData(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList,
        const RenderPass& renderPassBase, const PostProcessConfiguration& ppConfig);
    void RenderGaussian(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList,
        const RenderPass& renderPassBase, const PostProcessConfiguration& ppConfig);

    void UpdateGlobalSet(IRenderCommandList& cmdList);
    void CreateTargets(IRenderNodeContextManager& renderNodeContextMgr, const BASE_NS::Math::UVec2 baseSize);

    struct ImageData {
        RenderHandle mipImage;
        uint32_t mipCount { 0 };
        BASE_NS::Format format { BASE_NS::Format::BASE_FORMAT_UNDEFINED };
        BASE_NS::Math::UVec2 size { 0u, 0u };
    };
    ImageData imageData_;
    RenderHandle globalUbo_;

    // additional target
    struct TemporaryTarget {
        RenderHandleReference tex;
        BASE_NS::Math::UVec2 texSize { 0, 0 };
        BASE_NS::Format format { BASE_NS::Format::BASE_FORMAT_UNDEFINED };
    };
    TemporaryTarget tempTarget_;

    struct RenderDataHandles {
        RenderHandle shader;
        PipelineLayout pipelineLayout;
        RenderHandle psoScale;
        RenderHandle psoBlur;
    };
    RenderDataHandles renderData_;

    IDescriptorSetBinder::Ptr globalSet0_;
    BASE_NS::vector<IDescriptorSetBinder::Ptr> binders_;
    RenderHandle samplerHandle_;
    BlurInfo blurInfo_;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_BLUR_H
