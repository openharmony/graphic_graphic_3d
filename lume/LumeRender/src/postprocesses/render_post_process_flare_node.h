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

#ifndef RENDER_POSTPROCESS_RENDER_POSTPROCESS_FLARE_NODE_H
#define RENDER_POSTPROCESS_RENDER_POSTPROCESS_FLARE_NODE_H

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

RENDER_BEGIN_NAMESPACE()
class RenderPostProcessFlareNode : public CORE_NS::IInterfaceHelper<IRenderPostProcessNode, IRenderPostProcess> {
public:
    static constexpr auto UID = UID_RENDER_POST_PROCESS_FLARE;

    using Ptr = BASE_NS::refcnt_ptr<RenderPostProcessFlareNode>;

    struct EffectProperties {
        bool enabled { false };
        BASE_NS::Math::Vec3 flarePos { 0.0f, 0.0f, 0.0f };
        float intensity { 1.0f };
    };

    RenderPostProcessFlareNode();

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

    // direct access in render nodes
    RenderPostProcessFlareNode::EffectProperties propertiesData;

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

    NodeInputs nodeInputsData;
    NodeOutputs nodeOutputsData;

private:
    CORE_NS::PropertyApiImpl<RenderPostProcessFlareNode::EffectProperties> properties_;
    EffectProperties effectProperties_;

    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    static constexpr uint64_t INVALID_CAM_ID { 0xFFFFFFFFffffffff };
    struct JsonInputs {
        BASE_NS::string customCameraName;
        uint64_t customCameraId { INVALID_CAM_ID };
    };
    JsonInputs jsonInputs_;

    struct PushConstantStruct {
        BASE_NS::Math::Vec4 texSizeInvTexSize;
        BASE_NS::Math::Vec4 rt;
    };

    void EvaluateOutput();
    PushConstantStruct GetPushDataStruct(uint32_t width, uint32_t height) const;

    RenderAreaRequest renderAreaRequest_;
    bool useRequestedRenderArea_ { false };

    struct PipelineData {
        RENDER_NS::IShaderManager::GraphicsShaderData gsd;
        RENDER_NS::RenderHandle pso;
    };
    PipelineData pipelineData_;

    RENDER_NS::RenderHandleReference tempTarget_;

    CORE_NS::PropertyApiImpl<NodeInputs> inputProperties_;
    CORE_NS::PropertyApiImpl<NodeOutputs> outputProperties_;

    RENDER_NS::BindableImage defaultInput_;

    RENDER_NS::DescriptorCounts descriptorCounts_;
    bool valid_ { false };
};
RENDER_END_NAMESPACE()

#endif // RENDER_POSTPROCESS_RENDER_POSTPROCESS_FLARE_NODE_H
