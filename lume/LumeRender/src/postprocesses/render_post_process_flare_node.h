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

#ifndef RENDER_POSTPROCESS_RENDER_POSTPROCESS_FLARE_NODE_H
#define RENDER_POSTPROCESS_RENDER_POSTPROCESS_FLARE_NODE_H

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
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/nodecontext/intf_render_post_process.h>
#include <render/nodecontext/intf_render_post_process_node.h>
#include <render/property/property_types.h>
#include <render/render_data_structures.h>

RENDER_BEGIN_NAMESPACE()
class RenderPostProcessFlareNode : public CORE_NS::IInterfaceHelper<RENDER_NS::IRenderPostProcessNode> {
public:
    static constexpr auto UID = BASE_NS::Uid("eeea01ce-c094-4559-a909-ea44172c7d43");

    using Ptr = BASE_NS::refcnt_ptr<RenderPostProcessFlareNode>;

    RenderPostProcessFlareNode();

    CORE_NS::IPropertyHandle* GetRenderInputProperties() override;
    CORE_NS::IPropertyHandle* GetRenderOutputProperties() override;
    RENDER_NS::DescriptorCounts GetRenderDescriptorCounts() const override;
    void SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest) override;

    RENDER_NS::IRenderNode::ExecuteFlags GetExecuteFlags() const override;
    void Init(const RENDER_NS::IRenderPostProcess::Ptr& postProcess,
        RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecute() override;
    void Execute(RENDER_NS::IRenderCommandList& cmdList) override;

    struct NodeInputs {
        RENDER_NS::BindableImage input;
    };
    struct NodeOutputs {
        RENDER_NS::BindableImage output;
    };

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };
    RENDER_NS::IRenderPostProcess::Ptr postProcess_;

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

    NodeInputs nodeInputsData_;
    NodeOutputs nodeOutputsData_;
    CORE_NS::PropertyApiImpl<NodeInputs> inputProperties_;
    CORE_NS::PropertyApiImpl<NodeOutputs> outputProperties_;

    RENDER_NS::BindableImage defaultInput_;

    RENDER_NS::DescriptorCounts descriptorCounts_;
    bool valid_ { false };
    bool enabled_ { false };
};
RENDER_END_NAMESPACE()

#endif // RENDER_POSTPROCESS_RENDER_POSTPROCESS_FLARE_NODE_H
