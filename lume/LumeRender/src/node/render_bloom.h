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

#ifndef RENDER_RENDER__NODE__RENDER_BLOOM_H
#define RENDER_RENDER__NODE__RENDER_BLOOM_H

#include <array>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/math/vector.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class RenderBloom final {
public:
    RenderBloom() = default;
    ~RenderBloom() = default;

    struct BloomInfo {
        BindableImage input;
        BindableImage output;
        RenderHandle globalUbo;
        bool useCompute { false };
    };

    void Init(IRenderNodeContextManager& renderNodeContextMgr, const BloomInfo& bloomInfo);
    void PreExecute(IRenderNodeContextManager& renderNodeContextMgr, const BloomInfo& bloomInfo,
        const PostProcessConfiguration& ppConfig);
    void Execute(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList,
        const PostProcessConfiguration& ppConfig);

    DescriptorCounts GetDescriptorCounts() const;
    // call after PreExecute, to get the output
    RenderHandle GetFinalTarget() const;

private:
    void ComputeBloom(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList);
    void ComputeDownscaleAndThreshold(const PushConstant& pc, IRenderCommandList& cmdList);
    void ComputeDownscale(const PushConstant& pc, IRenderCommandList& cmdList);
    void ComputeUpscale(const PushConstant& pc, IRenderCommandList& cmdList);
    void ComputeCombine(const PushConstant& pc, IRenderCommandList& cmdList);

    void GraphicsBloom(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList);

    void RenderDownscaleAndThreshold(RenderPass& renderPass, const PushConstant& pc, IRenderCommandList& cmdList);
    void RenderDownscale(RenderPass& renderPass, const PushConstant& pc, IRenderCommandList& cmdList);
    void RenderUpscale(RenderPass& renderPass, const PushConstant& pc, IRenderCommandList& cmdList);
    void RenderCombine(RenderPass& renderPass, const PushConstant& pc, IRenderCommandList& cmdList);

    void CreateTargets(IRenderNodeContextManager& renderNodeContextMgr, const BASE_NS::Math::UVec2 baseSize);
    void CreatePsos(IRenderNodeContextManager& renderNodeContextMgr);
    void CreateComputePsos(IRenderNodeContextManager& renderNodeContextMgr);
    void CreateRenderPsos(IRenderNodeContextManager& renderNodeContextMgr);
    std::pair<RenderHandle, const PipelineLayout&> CreateAndReflectRenderPso(
        IRenderNodeContextManager& renderNodeContextMgr, const BASE_NS::string_view shader,
        const RenderPass& renderPass);
    void UpdateGlobalSet(IRenderCommandList& cmdList);

    static constexpr uint32_t TARGET_COUNT { 7u };
    static constexpr uint32_t CORE_BLOOM_QUALITY_LOW { 1u };
    static constexpr uint32_t CORE_BLOOM_QUALITY_NORMAL { 2u };
    static constexpr uint32_t CORE_BLOOM_QUALITY_HIGH { 4u };
    static constexpr int CORE_BLOOM_QUALITY_COUNT { 3u };

    struct Targets {
        std::array<RenderHandleReference, TARGET_COUNT> tex1;
        // separate target needed in graphics bloom upscale
        std::array<RenderHandleReference, TARGET_COUNT> tex2;
        std::array<BASE_NS::Math::UVec2, TARGET_COUNT> tex1Size;
    };
    Targets targets_;

    struct PSOs {
        struct DownscaleHandles {
            RenderHandle regular;
            RenderHandle threshold;
        };

        std::array<DownscaleHandles, CORE_BLOOM_QUALITY_COUNT> downscaleHandles;
        std::array<DownscaleHandles, CORE_BLOOM_QUALITY_COUNT> downscaleHandlesCompute;

        RenderHandle downscaleAndThreshold;
        RenderHandle downscale;
        RenderHandle upscale;
        RenderHandle combine;

        ShaderThreadGroup downscaleAndThresholdTGS { 1, 1, 1 };
        ShaderThreadGroup downscaleTGS { 1, 1, 1 };
        ShaderThreadGroup upscaleTGS { 1, 1, 1 };
        ShaderThreadGroup combineTGS { 1, 1, 1 };
    };
    PSOs psos_;

    struct Binders {
        IDescriptorSetBinder::Ptr globalSet0;

        IDescriptorSetBinder::Ptr downscaleAndThreshold;
        std::array<IDescriptorSetBinder::Ptr, TARGET_COUNT> downscale;
        std::array<IDescriptorSetBinder::Ptr, TARGET_COUNT> upscale;
        IDescriptorSetBinder::Ptr combine;
    };
    Binders binders_;

    BASE_NS::Math::UVec2 baseSize_ { 0u, 0u };
    BASE_NS::Math::Vec4 bloomParameters_ { 0.0f, 0.0f, 0.0f, 0.0f };

    RenderHandleReference samplerHandle_;
    BASE_NS::Format format_ { BASE_NS::Format::BASE_FORMAT_UNDEFINED };

    ViewportDesc baseViewportDesc_;
    ScissorDesc baseScissorDesc_;

    bool bloomEnabled_ { false };
    BloomInfo bloomInfo_;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_BLOOM_H
