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

#include <base/containers/array_view.h>
#include <core/plugin/intf_interface_helper.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property_tools/property_api_impl.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_post_process.h>
#include <render/nodecontext/intf_render_post_process_node.h>

#include "nodecontext/render_node_copy_util.h"

// Tensor-Field Superresolution (TFS)

RENDER_BEGIN_NAMESPACE()
class RenderPostProcessUpscaleNode : public CORE_NS::IInterfaceHelper<IRenderPostProcessNode, IRenderPostProcess> {
public:
    static constexpr auto UID = UID_RENDER_POST_PROCESS_UPSCALE;

    using Ptr = BASE_NS::refcnt_ptr<RenderPostProcessUpscaleNode>;

    struct EffectProperties {
        bool enabled { false };
        UpscaleConfiguration upscaleConfiguration;
    };

    RenderPostProcessUpscaleNode();
    ~RenderPostProcessUpscaleNode() = default;

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
    RENDER_NS::DescriptorCounts GetRenderDescriptorCounts() const override;
    void SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest) override;

    RENDER_NS::IRenderNode::ExecuteFlags GetExecuteFlags() const override;
    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;

    struct NodeInputs {
        RENDER_NS::BindableImage input;
    };
    struct NodeOutputs {
        RENDER_NS::BindableImage output;
    };

    EffectProperties propertiesData;
    NodeInputs nodeInputsData;
    NodeOutputs nodeOutputsData;

private:
    CORE_NS::PropertyApiImpl<EffectProperties> properties_;

    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void RenderGradientPass(RENDER_NS::IRenderCommandList& cmdList);
    void RenderStructureTensorPass(RENDER_NS::IRenderCommandList& cmdList);
    void TensorFieldUpscalePass(RENDER_NS::IRenderCommandList& cmdList);

    void CreateTargets(const BASE_NS::Math::UVec2 baseSize);
    void CreatePsos();
    RenderPass CreateRenderPass(const RenderHandle output, const BASE_NS::Math::UVec2& resolution) const;
    void CheckDescriptorSetNeed();

    BASE_NS::Math::UVec2 baseSize_ { 0U, 0U };

    struct Targets {
        BASE_NS::Math::UVec2 inputResolution { 0U, 0U };
        BASE_NS::Math::UVec2 outputResolution { 0U, 0U };

        RenderHandleReference gradientTexture;

        RenderHandleReference tensorDataTexture;
    };
    Targets targets_;

    struct PSOs {
        RenderHandle gradientPass;
        RenderHandle structureTensorPass;
        RenderHandle finalUpscalePass;
    };
    PSOs psos_;

    struct ShaderData {
        IShaderManager::ShaderData gradientPass;
        IShaderManager::ShaderData structureTensorPass;
        IShaderManager::ShaderData finalUpscalePass;
    };
    ShaderData shaderData_;

    struct Binders {
        IDescriptorSetBinder::Ptr gradientPass;
        IDescriptorSetBinder::Ptr structureTensorPass;
        IDescriptorSetBinder::Ptr finalUpscalePass;
    };
    Binders binders_;

    RenderHandleReference samplerNearestHandle_;
    RenderNodeCopyUtil renderCopyOutput_;

    struct UpscalerPushConstant {
        BASE_NS::Math::Vec4 inputSize { 0.0f, 0.0f, 0.0f, 0.0f };  // 1/width, 1/height, width, height
        BASE_NS::Math::Vec4 outputSize { 0.0f, 0.0f, 0.0f, 0.0f }; // output width, height, 1/width, 1/height
        float smoothScale { 1.0f };

        float structureSensitivity { 1.0f };
        float edgeSharpness { 2.0f };

        float padding { 0.0f };
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