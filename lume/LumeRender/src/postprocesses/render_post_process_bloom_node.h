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

#ifndef RENDER_POSTPROCESS_RENDER_POST_PROCESS_BLOOM_NODE_H
#define RENDER_POSTPROCESS_RENDER_POST_PROCESS_BLOOM_NODE_H

#include <array>

#include <base/containers/array_view.h>
#include <core/plugin/intf_interface_helper.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property_tools/property_api_impl.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/implementation_uids.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_post_process.h>
#include <render/nodecontext/intf_render_post_process_node.h>

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;
class IRenderNodeRenderDataStoreManager;
struct RenderNodeGraphInputs;

class RenderPostProcessBloomNode final : public CORE_NS::IInterfaceHelper<IRenderPostProcessNode, IRenderPostProcess> {
public:
    static constexpr auto UID = UID_RENDER_POST_PROCESS_BLOOM;

    using Ptr = BASE_NS::refcnt_ptr<RenderPostProcessBloomNode>;

    RenderPostProcessBloomNode();
    ~RenderPostProcessBloomNode() = default;

    // from IRenderPostProcess
    CORE_NS::IPropertyHandle* GetProperties() override
    {
        return properties_.GetData();
    }

    BASE_NS::Uid GetRenderPostProcessNodeUid() override
    {
        return UID;
    }

    void SetData(BASE_NS::array_view<const uint8_t> data) override
    {
        constexpr size_t propertyByteSize = sizeof(EffectProperties);
        if (data.size_bytes() == propertyByteSize) {
            BASE_NS::CloneData(&propertiesData, propertyByteSize, data.data(), data.size_bytes());
        }
    }
    BASE_NS::array_view<const uint8_t> GetData() const override
    {
        return { reinterpret_cast<const uint8_t*>(&propertiesData), sizeof(EffectProperties) };
    }

    // from IRenderPostProcessNode
    CORE_NS::IPropertyHandle* GetRenderInputProperties() override;
    CORE_NS::IPropertyHandle* GetRenderOutputProperties() override;
    DescriptorCounts GetRenderDescriptorCounts() const override;
    void SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest) override;
    IRenderNode::ExecuteFlags GetExecuteFlags() const override;

    RenderHandle GetFinalTarget() const;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;

    // for plugin / factory interface
    static constexpr const char* TYPE_NAME = "RenderPostProcessBloomNode";

    struct EffectProperties {
        bool enabled { false };
        BloomConfiguration bloomConfiguration;
    };

    struct NodeInputs {
        BindableImage input;
    };
    struct NodeOutputs {
        BindableImage output;
    };

    EffectProperties propertiesData;
    NodeInputs nodeInputsData;
    NodeOutputs nodeOutputsData;

private:
    CORE_NS::PropertyApiImpl<EffectProperties> properties_;

    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void ComputeBloom(IRenderCommandList& cmdList);
    void ComputeDownscaleAndThreshold(const PushConstant& pc, IRenderCommandList& cmdList);
    void ComputeDownscale(const PushConstant& pc, IRenderCommandList& cmdList);
    void ComputeUpscale(const PushConstant& pc, IRenderCommandList& cmdList);
    void ComputeCombine(const PushConstant& pc, IRenderCommandList& cmdList);

    void RenderDownscaleAndThreshold(RenderPass& renderPass, const PushConstant& pc, IRenderCommandList& cmdList);
    void RenderDownscale(RenderPass& renderPass, const PushConstant& pc, IRenderCommandList& cmdList);
    void RenderUpscale(RenderPass& renderPass, const PushConstant& pc, IRenderCommandList& cmdList);
    void RenderCombine(RenderPass& renderPass, const PushConstant& pc, IRenderCommandList& cmdList);

    void GraphicsBloom(IRenderCommandList& cmdList);

    void CreateTargets(const BASE_NS::Math::UVec2 baseSize);
    void CreatePsos();
    void CreateComputePsos();
    void CreateRenderPsos();

    std::pair<RenderHandle, const PipelineLayout&> CreateAndReflectRenderPso(const BASE_NS::string_view shader);

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
        IDescriptorSetBinder::Ptr downscaleAndThreshold;
        std::array<IDescriptorSetBinder::Ptr, TARGET_COUNT> downscale;
        std::array<IDescriptorSetBinder::Ptr, TARGET_COUNT> upscale;
        IDescriptorSetBinder::Ptr combine;
    };
    Binders binders_;

    // calculated from the amount of textures and scale factor
    size_t frameScaleMaxCount_ { 0 };

    RenderHandleReference samplerHandle_;
    BASE_NS::Format format_ { BASE_NS::Format::BASE_FORMAT_UNDEFINED };

    ViewportDesc baseViewportDesc_;
    ScissorDesc baseScissorDesc_;

    RenderHandleReference postProcessUbo_;

    RenderAreaRequest renderAreaRequest_;
    bool useRequestedRenderArea_ { false };

    CORE_NS::PropertyApiImpl<NodeInputs> inputProperties_;
    CORE_NS::PropertyApiImpl<NodeOutputs> outputProperties_;

    bool valid_ { false };

    EffectProperties effectProperties_;
    BASE_NS::Math::UVec2 baseSize_ { 0U, 0U };
    BASE_NS::Math::Vec4 bloomParameters_ { 0.0f, 0.0f, 0.0f, 0.0f };
};
RENDER_END_NAMESPACE()

#endif // RENDER_POSTPROCESS_RENDER_POST_PROCESS_BLOOM_NODE_H
