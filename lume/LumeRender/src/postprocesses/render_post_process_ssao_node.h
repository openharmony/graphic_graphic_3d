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

#ifndef APP__SRC__RENDER_POST_PROCESS_SSAO_NODE_H
#define APP__SRC__RENDER_POST_PROCESS_SSAO_NODE_H

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

class RenderPostProcessSsaoNode final
    : public CORE_NS::IInterfaceHelper<RENDER_NS::IRenderPostProcessNode, RENDER_NS::IRenderPostProcess> {
public:
    static constexpr auto UID = UID_RENDER_POST_PROCESS_SSAO;

    RenderPostProcessSsaoNode();
    ~RenderPostProcessSsaoNode();

    using Ptr = BASE_NS::refcnt_ptr<RenderPostProcessSsaoNode>;

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
        float time { 0.0f };
        float hasValidNormalTex { 0.0f };

        float radius { 0.5f };
        float bias { 0.025f };
        float intensity { 1.5f };
        float contrast { 1.6f };
    };

    struct NodeInputs {
        RENDER_NS::BindableImage input;
        RENDER_NS::BindableImage depth;
        RENDER_NS::BindableImage velocity;
        RENDER_NS::BindableBuffer camera;
    };
    struct NodeOutputs {
        RENDER_NS::BindableImage output;
    };

    struct EffectProperties {
        bool enabled { true };
        float radius { 0.5f };
        float intensity { 1.0f };
        float bias { 0.025f };
        float contrast { 1.2f };
    };

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };
    RENDER_NS::IRenderPostProcess::Ptr postProcess_;

    void EvaluateOutputImageCreation();
    void CreateTargets(const BASE_NS::Math::UVec2 baseSize);
    void CreatePsos();
    RENDER_NS::RenderPass CreateRenderPass(
        const RENDER_NS::RenderHandle output, const BASE_NS::Math::UVec2& resolution) const;
    void CheckDescriptorSetNeed();

    void RenderDownscalePass(RENDER_NS::IRenderCommandList& cmdList);
    void RenderSSAOPass(RENDER_NS::IRenderCommandList& cmdList);
    void RenderBlurPass(RENDER_NS::IRenderCommandList& cmdList);
    void RenderAreaNoisePass(RENDER_NS::IRenderCommandList& cmdList);

    RenderAreaRequest renderAreaRequest_;
    bool useRequestedRenderArea_ { false };
    bool velocityNormalIsValid_ { false };

    EffectProperties propertiesData_;
    CORE_NS::PropertyApiImpl<EffectProperties> properties_;

    struct PSOs {
        RENDER_NS::RenderHandle downscalePass;
        RENDER_NS::RenderHandle ssaoPass;
        RENDER_NS::RenderHandle blurPass;
        RENDER_NS::RenderHandle areaNoisePass;
    };
    PSOs psos_;

    struct ShaderData {
        RENDER_NS::IShaderManager::GraphicsShaderData downscalePass;
        RENDER_NS::IShaderManager::GraphicsShaderData ssaoPass;
        RENDER_NS::IShaderManager::GraphicsShaderData blurPass;
        RENDER_NS::IShaderManager::GraphicsShaderData areaNoisePass;
    };
    ShaderData shaderData_;

    struct Binders {
        RENDER_NS::IDescriptorSetBinder::Ptr downscalePass;
        RENDER_NS::IDescriptorSetBinder::Ptr ssaoPass;
        RENDER_NS::IDescriptorSetBinder::Ptr blurPass;
        RENDER_NS::IDescriptorSetBinder::Ptr areaNoisePass;
    };
    Binders binders_;

    RENDER_NS::RenderHandleReference gpuBuffer_;
    ShaderParameters shaderParameters_;

    struct Targets {
        BASE_NS::Math::UVec2 inputResolution { 0U, 0U };
        BASE_NS::Math::UVec2 outputResolution { 0U, 0U };
        BASE_NS::Math::UVec2 ssaoResolution { 0U, 0U };

        RENDER_NS::RenderHandleReference halfResDepth;
        RENDER_NS::RenderHandleReference halfResColor;
        RENDER_NS::RenderHandleReference ssaoTexture;
        RENDER_NS::RenderHandleReference ssaoAreaNoiseTexture;
    };
    Targets targets_;

    BASE_NS::Math::UVec2 baseSize_ { 0U, 0U };

    NodeInputs nodeInputsData_;
    NodeOutputs nodeOutputsData_;
    CORE_NS::PropertyApiImpl<NodeInputs> inputProperties_;
    CORE_NS::PropertyApiImpl<NodeOutputs> outputProperties_;

    RENDER_NS::BindableImage currInput_;
    RENDER_NS::BindableImage currDepthInput_;
    RENDER_NS::BindableImage currVelocityNormalInput_;
    RENDER_NS::BindableImage currOutput_;

    RENDER_NS::BindableImage defaultInput_;
    RENDER_NS::BindableImage defaultLinearSampler_;

    RENDER_NS::RenderHandle cameraUbo_;

    struct OwnOutputImageData {
        uint32_t width { 0U };
        uint32_t height { 0U };
        RENDER_NS::RenderHandleReference handle;
    };
    OwnOutputImageData ownOutputImageData_;

    RENDER_NS::DescriptorCounts descriptorCounts_;
    bool enabled_ { false };
    bool valid_ { false };
};
RENDER_END_NAMESPACE()

#endif // APP__SRC__RENDER_POST_PROCESS_SSAO_NODE_H