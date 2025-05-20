/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef RENDER_POSTPROCESS_RENDER_POSTPROCESS_UPSCALE_NODE_H
#define RENDER_POSTPROCESS_RENDER_POSTPROCESS_UPSCALE_NODE_H

#include <array>

#include <base/containers/unique_ptr.h>
#include <base/math/matrix_util.h>
#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_interface_helper.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.h>
#include <core/property_tools/property_macros.h>
#include <render/device/intf_shader_manager.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/nodecontext/intf_render_post_process.h>
#include <render/nodecontext/intf_render_post_process_node.h>
#include <render/property/property_types.h>
#include <render/render_data_structures.h>

#include "nodecontext/render_node_copy_util.h"

RENDER_BEGIN_NAMESPACE()
class RenderPostProcessUpscaleNode : public CORE_NS::IInterfaceHelper<RENDER_NS::IRenderPostProcessNode> {
public:
    static constexpr auto UID = BASE_NS::Uid("81321991-8314-4c74-bc6a-487eed297550");

    using Ptr = BASE_NS::refcnt_ptr<RenderPostProcessUpscaleNode>;

    struct EffectProperties {
        bool enabled { true };
        float ratio = 1.5f;
    };

    RenderPostProcessUpscaleNode();
    ~RenderPostProcessUpscaleNode() = default;

    CORE_NS::IPropertyHandle* GetRenderInputProperties() override;
    CORE_NS::IPropertyHandle* GetRenderOutputProperties() override;
    RENDER_NS::DescriptorCounts GetRenderDescriptorCounts() const override;
    void SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest) override;

    RENDER_NS::IRenderNode::ExecuteFlags GetExecuteFlags() const override;
    void Init(const RENDER_NS::IRenderPostProcess::Ptr& postProcess,
        RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecute() override;
    void Execute(RENDER_NS::IRenderCommandList& cmdList) override;

    void SetCameraData();

    struct NodeInputs {
        RENDER_NS::BindableImage input;
        RENDER_NS::BindableImage depth;
        RENDER_NS::BindableImage velocity;
        RENDER_NS::BindableImage history;
    };
    struct NodeOutputs {
        RENDER_NS::BindableImage output;
    };

    NodeInputs nodeInputsData;
    NodeOutputs nodeOutputsData;

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };
    RENDER_NS::IRenderPostProcess::Ptr postProcess_;

    // Super resolution functions for each pass
    void ComputeLuminancePyramid(const PushConstant& pc, RENDER_NS::IRenderCommandList& cmdList);
    void ComputeReconstructAndDilate(const PushConstant& pc, RENDER_NS::IRenderCommandList& cmdList);
    void ComputeDebug(const PushConstant& pc, RENDER_NS::IRenderCommandList& cmdList);
    void ComputeDepthClip(const PushConstant& pc, RENDER_NS::IRenderCommandList& cmdList);
    void ComputeCreateLocks(const PushConstant& pc, RENDER_NS::IRenderCommandList& cmdList);
    void ComputeAccumulate(const PushConstant& pc, RENDER_NS::IRenderCommandList& cmdList);
    void ComputeRcas(const PushConstant& pc, RENDER_NS::IRenderCommandList& cmdList);

    void CreateTargets(const BASE_NS::Math::UVec2 baseSize);
    void CreatePsos();

    BASE_NS::Math::UVec2 baseSize_ { 0U, 0U };

    // Mip level count for Luminance Pyramid
    uint32_t mipLevels_ { 1U };

    static constexpr uint32_t TARGET_COUNT { 10U };

    struct Targets {
        BASE_NS::Math::UVec2 renderResolution { 0U, 0U };
        BASE_NS::Math::UVec2 displayResolution { 0U, 0U };

        // ---------- Compute Luminance pass textures ----------
        std::array<RenderHandleReference, TARGET_COUNT> tex1;
        std::array<BASE_NS::Math::UVec2, TARGET_COUNT> tex1Size;

        // ---------- Dilate and Reconstruct pass textures ----------
        RenderHandleReference estPrevDepth;
        RenderHandleReference dilatedDepth;

        // Two textures for ping-ponging history/current dilated motion vectors
        std::array<RenderHandleReference, 2U> dilatedMotionVectors;
        uint32_t motionVectorIdx { 0U };

        RenderHandleReference lockInputLuma;

        // ---------- Debug pass textures ----------
        RenderHandleReference debugImage;

        // ---------- Depth Clip pass textures ----------
        RenderHandleReference adjustedColorBuffer;
        RenderHandleReference dilatedReactiveMask;

        // ---------- Create Locks Pass texture ----------
        RenderHandleReference newLocksMask;

        // ---------- Accumulate Pass History Textures (ping-pong) ----------
        std::array<RenderHandleReference, 2U> historyColorAndReactive;
        std::array<RenderHandleReference, 2U> historyLockStatus;
        std::array<RenderHandleReference, 2U> historyLuma;

        //----------- RCAS pass (optional) -------------
        RenderHandleReference rcas_final;
        bool rcas_enabled = false;

        RenderHandleReference finalColor;
        RenderHandle cameraUbo;
        uint32_t historyBufferIdx { 0U };
    };
    Targets targets_;

    struct PSOs {
        // Luminance Pyramid pass. Last mip level is 1x1 texture containing exposure (avg luminance)
        RenderHandle luminanceDownscale;
        RenderHandle luminancePyramid;
        RenderHandle reconstructAndDilate;
        RenderHandle debugPass;
        RenderHandle depthClipPass;
        RenderHandle locksPass;
        RenderHandle accumulatePass;
        RenderHandle rcasPass;

        ShaderThreadGroup luminanceDownscaleTGS { 1, 1, 1 };
        ShaderThreadGroup luminancePyramidTGS { 1, 1, 1 };
        ShaderThreadGroup reconstructAndDilateTGS { 1, 1, 1 };
        ShaderThreadGroup debugPassTGS { 1, 1, 1 };
        ShaderThreadGroup depthClipPassTGS { 1, 1, 1 };
        ShaderThreadGroup locksPassTGS { 1, 1, 1 };
        ShaderThreadGroup accumulatePassTGS { 1, 1, 1 };
        ShaderThreadGroup rcasPassTGS { 1, 1, 1 };
    };
    PSOs psos_;

    struct Binders {
        IDescriptorSetBinder::Ptr luminanceDownscale;
        std::array<IDescriptorSetBinder::Ptr, TARGET_COUNT> luminancePyramid;
        IDescriptorSetBinder::Ptr reconstructAndDilate;
        IDescriptorSetBinder::Ptr debugPass;
        IDescriptorSetBinder::Ptr depthClipPass;
        IDescriptorSetBinder::Ptr locksPass;
        IDescriptorSetBinder::Ptr accumulatePass;
        IDescriptorSetBinder::Ptr rcasPass;
    };
    Binders binders_;

    RenderHandleReference samplerHandle_;

    RenderNodeCopyUtil renderCopyOutput_;

    void EvaluateOutput();

    struct LockPassPushConstant {
        BASE_NS::Math::Vec4 renderSizeInvSize { 0.0f, 0.0f, 0.0f, 0.0f };
        BASE_NS::Math::Vec4 displaySizeInvSize { 0.0f, 0.0f, 0.0f, 0.0f };
    };

    struct AccumulatePassPushConstant {
        BASE_NS::Math::Vec4 displaySizeInvSize { 0.0f, 0.0f, 0.0f, 0.0f };
        BASE_NS::Math::Vec4 viewportSizeInvSize { 0.0f, 0.0f, 0.0f, 0.0f };
        float exposure = 1.0f;
        uint32_t frameIndex = 0; // Current frame count/index
        uint32_t jitterSequenceLength = 16U;
        float avgLanczosWeightPerFrame = 1.0f / 16.0f;
        float maxAccumulationLanczosWeight = 0.98f;
    };

    RenderAreaRequest renderAreaRequest_;
    bool useRequestedRenderArea_ { false };

    CORE_NS::PropertyApiImpl<NodeInputs> inputProperties_;
    CORE_NS::PropertyApiImpl<NodeOutputs> outputProperties_;

    bool valid_ { false };

    EffectProperties effectProperties_;
};
RENDER_END_NAMESPACE()

#endif // RENDER_POSTPROCESS_RENDER_POSTPROCESS_UPSCALE_NODE_H
