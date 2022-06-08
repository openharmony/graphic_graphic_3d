/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
        RenderHandle input;
        RenderHandle output;
        RenderHandle globalUbo;
        bool useCompute { false };
    };

    void Init(IRenderNodeContextManager& renderNodeContextMgr, const BloomInfo& bloomInfo);
    void PreExecute(IRenderNodeContextManager& renderNodeContextMgr, const BloomInfo& bloomInfo,
        const PostProcessConfiguration& ppConfig);
    void Execute(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList,
        const PostProcessConfiguration& ppConfig);

    DescriptorCounts GetDescriptorCounts() const;
    // call after Execute, to get the output
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
        const RenderPass& renderPass, const DynamicStateFlags dynamicStateFlags);
    void UpdateGlobalSet(IRenderCommandList& cmdList);

    static constexpr uint32_t TARGET_COUNT { 7u };
    struct Targets {
        std::array<RenderHandleReference, TARGET_COUNT> tex1;
        std::array<BASE_NS::Math::UVec2, TARGET_COUNT> tex1Size;
    };
    Targets targets_;

    struct PSOs {
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
