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

#ifndef APP__SRC__RENDER_POST_PROCESS_CMAA2_NODE_H
#define APP__SRC__RENDER_POST_PROCESS_CMAA2_NODE_H

#include <base/containers/unique_ptr.h>
#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_interface_helper.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.h>
#include <core/property_tools/property_macros.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/nodecontext/intf_render_post_process.h>
#include <render/nodecontext/intf_render_post_process_node.h>
#include <render/property/property_types.h>
#include <render/render_data_structures.h>

RENDER_BEGIN_NAMESPACE()

class RenderPostProcessCmaa2Node final
    : public CORE_NS::IInterfaceHelper<RENDER_NS::IRenderPostProcessNode, RENDER_NS::IRenderPostProcess> {
public:
    static constexpr auto UID = UID_RENDER_POST_PROCESS_CMAA2;

    RenderPostProcessCmaa2Node();
    ~RenderPostProcessCmaa2Node();

    using Ptr = BASE_NS::refcnt_ptr<RenderPostProcessCmaa2Node>;

    // from IRenderPostProcess
    CORE_NS::IPropertyHandle* GetProperties() override
    {
        return properties_.GetData();
    }

    BASE_NS::Uid GetRenderPostProcessNodeUid() override
    {
        return UID;
    }

    struct EffectProperties {
        enum class Quality { LOW = 0, MEDIUM = 1, HIGH = 2, ULTRA = 3 };

        bool enabled { true };
        Quality quality { EffectProperties::Quality::LOW };
    };

    void SetData(BASE_NS::array_view<const uint8_t> data) override
    {
        constexpr size_t propertyByteSize = sizeof(EffectProperties);
        if (data.size_bytes() == propertyByteSize) {
            BASE_NS::CloneData(&propertiesData_, propertyByteSize, data.data(), data.size_bytes());
        }
    }
    BASE_NS::array_view<const uint8_t> GetData() const override
    {
        return { reinterpret_cast<const uint8_t*>(&propertiesData_), sizeof(EffectProperties) };
    }

    // from IRenderPostProcessNode
    CORE_NS::IPropertyHandle* GetRenderInputProperties() override;
    CORE_NS::IPropertyHandle* GetRenderOutputProperties() override;
    RENDER_NS::DescriptorCounts GetRenderDescriptorCounts() const override;
    void SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest) override;

    RENDER_NS::IRenderNode::ExecuteFlags GetExecuteFlags() const override;
    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;

    struct ShaderParameters {
        float resX { 0.0f };
        float resY { 0.0f };
        float edgeThreshold { 0.07f };
        float padding { 0.0f };
    };

    struct NodeInputs {
        RENDER_NS::BindableImage input;
    };

    struct NodeOutputs {
        RENDER_NS::BindableImage output;
    };

    const float EDGE_THRESHOLD_LOW { 0.25f };
    const float EDGE_THRESHOLD_MEDIUM { 0.10f };
    const float EDGE_THRESHOLD_HIGH { 0.07f };
    const float EDGE_THRESHOLD_ULTRA { 0.05f };

    struct ControlBufferData {
        uint32_t itemCount { 0 };
        uint32_t shapeCandidateCount { 0 };
        uint32_t blendLocationCount { 0 };
        uint32_t deferredBlendItemCount { 0 };
    };

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };
    RENDER_NS::IRenderPostProcess::Ptr postProcess_;

    void EvaluateOutputImageCreation();
    void CreateTargets(const BASE_NS::Math::UVec2 baseSize);
    void CreateBuffers(const BASE_NS::Math::UVec2 baseSize);
    void CreatePsos();
    void CheckDescriptorSetNeed();
    void ClearControlBuffer(RENDER_NS::IRenderCommandList& cmdList);
    void ClearImages(RENDER_NS::IRenderCommandList& cmdList);
    RENDER_NS::RenderPass CreateRenderPass(
        const RENDER_NS::RenderHandle output, const BASE_NS::Math::UVec2& resolution) const;

    void RenderEdgeDetectionPass(RENDER_NS::IRenderCommandList& cmdList);
    void RenderProcessCandidatesPass(RENDER_NS::IRenderCommandList& cmdList);
    void RenderDeferredApplyPass(RENDER_NS::IRenderCommandList& cmdList);
    void RenderBlit(RENDER_NS::IRenderCommandList& cmdList);
    void UpdateIndirectArgs(RENDER_NS::IRenderCommandList& cmdList);

    RenderAreaRequest renderAreaRequest_;
    bool useRequestedRenderArea_ { false };

    EffectProperties propertiesData_;
    CORE_NS::PropertyApiImpl<EffectProperties> properties_;

    struct PSOs {
        RENDER_NS::RenderHandle clearPass;
        RENDER_NS::RenderHandle edgeDetectionPass;
        RENDER_NS::RenderHandle processCandidatesPass;
        RENDER_NS::RenderHandle deferredApplyPass;
        RENDER_NS::RenderHandle blit;
        RENDER_NS::RenderHandle iargs;
    };
    PSOs psos_;

    struct ShaderData {
        RENDER_NS::IShaderManager::ShaderData clearPass;
        RENDER_NS::IShaderManager::ShaderData edgeDetectionPass;
        RENDER_NS::IShaderManager::ShaderData processCandidatesPass;
        RENDER_NS::IShaderManager::ShaderData deferredApplyPass;
        RENDER_NS::IShaderManager::GraphicsShaderData blit;
        RENDER_NS::IShaderManager::ShaderData iargs;
    };
    ShaderData shaderData_;

    struct Binders {
        RENDER_NS::IDescriptorSetBinder::Ptr clearPass;
        RENDER_NS::IDescriptorSetBinder::Ptr edgeDetectionPass;
        RENDER_NS::IDescriptorSetBinder::Ptr processCandidatesPass;
        RENDER_NS::IDescriptorSetBinder::Ptr deferredApplyPass;
        RENDER_NS::IDescriptorSetBinder::Ptr blit;
        RENDER_NS::IDescriptorSetBinder::Ptr iargs;
    };
    Binders binders_;

    RENDER_NS::RenderHandleReference uniformBuffer_;
    ShaderParameters shaderParameters_;

    struct ComputeBuffers {
        RENDER_NS::RenderHandleReference controlBuffer;
        RENDER_NS::RenderHandleReference shapeCandidatesBuffer;
        RENDER_NS::RenderHandleReference blendLocationList;
        RENDER_NS::RenderHandleReference blendItemList;
    };
    ComputeBuffers computeBuffers_;

    struct Targets {
        BASE_NS::Math::UVec2 resolution { 0u, 0u };
        RENDER_NS::RenderHandleReference edgeImage;
        RENDER_NS::RenderHandleReference blendItemHeads;
        RENDER_NS::RenderHandleReference outComp;
    };
    Targets targets_;

    RENDER_NS::RenderHandleReference dispatchIndirectHandle_;

    BASE_NS::Math::UVec2 baseSize_ { 0u, 0u };

    NodeInputs nodeInputsData_;
    NodeOutputs nodeOutputsData_;
    CORE_NS::PropertyApiImpl<NodeInputs> inputProperties_;
    CORE_NS::PropertyApiImpl<NodeOutputs> outputProperties_;

    RENDER_NS::BindableImage currInput_;
    RENDER_NS::BindableImage currOutput_;

    RENDER_NS::BindableImage defaultInput_;
    RENDER_NS::BindableImage defaultSampler_;

    struct OwnOutputImageData {
        uint32_t width { 0u };
        uint32_t height { 0u };
        RENDER_NS::RenderHandleReference handle;
    };
    OwnOutputImageData ownOutputImageData_;

    RENDER_NS::DescriptorCounts descriptorCounts_;
    bool enabled_ { false };
    bool valid_ { false };
};
RENDER_END_NAMESPACE()

#endif // APP__SRC__RENDER_POST_PROCESS_CMAA2_NODE_H