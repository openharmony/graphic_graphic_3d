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

#ifndef RENDER_POSTPROCESS_RENDER_POST_PROCESS_COMBINED_NODE_H
#define RENDER_POSTPROCESS_RENDER_POST_PROCESS_COMBINED_NODE_H

#include <base/containers/array_view.h>
#include <core/plugin/intf_interface_helper.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property_tools/property_api_impl.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_post_process.h>
#include <render/nodecontext/intf_render_post_process_node.h>

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;
class IRenderNodeRenderDataStoreManager;
struct RenderNodeGraphInputs;

class RenderPostProcessCombinedNode final
    : public CORE_NS::IInterfaceHelper<IRenderPostProcessNode, IRenderPostProcess> {
public:
    static constexpr auto UID = UID_RENDER_POST_PROCESS_COMBINED;

    using Ptr = BASE_NS::refcnt_ptr<RenderPostProcessCombinedNode>;

    RenderPostProcessCombinedNode();
    ~RenderPostProcessCombinedNode() = default;

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

    struct UboHandles {
        // first 512 aligned is global post process
        // after (256) we have effect local data
        RenderHandleReference postProcess;
    };

    struct EffectProperties {
        bool enabled { false };
        bool glOptimizedLayerCopyEnabled { false };
        PostProcessConfiguration postProcessConfiguration;
    };

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(IRenderCommandList& cmdList) override;

    // for plugin / factory interface
    static constexpr const char* TYPE_NAME = "RenderPostProcessCombinedNode";

    struct NodeInputs {
        BindableImage input;
        BindableImage bloomFinalTarget;
        BindableImage dirtMask;
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

    RenderPass CreateRenderPass(const RenderHandle input);
    void UpdatePostProcessData(const PostProcessConfiguration& postProcessConfiguration);
    void CheckDescriptorSetNeed();

    struct EffectData {
        IShaderManager::ShaderData sd;
        RenderHandle pso;
    };
    EffectData combineData_;
    // additional optimized layer copy for GL(ES)
    EffectData combineDataLayer_;

    RenderHandle reservedUboHandle_;
    IRenderNodeGpuResourceManager::MappedGpuBufferData acquiredGpuBufferData_;
    struct Binders {
        IDescriptorSetBinder::Ptr set0;
        IDescriptorSetBinder::Ptr combineBinder;
        IDescriptorSetBinder::Ptr combineLayerBinder;
    };
    Binders binders_;

    struct DefaultSamplers {
        RenderHandle nearest;
        RenderHandle linear;
    };
    DefaultSamplers samplers_;

    RenderAreaRequest renderAreaRequest_;
    bool useRequestedRenderArea_ { false };

    CORE_NS::PropertyApiImpl<NodeInputs> inputProperties_;
    CORE_NS::PropertyApiImpl<NodeOutputs> outputProperties_;

    RENDER_NS::DescriptorCounts descriptorCounts_;
    bool valid_ { false };

    EffectProperties effectProperties_;
};
RENDER_END_NAMESPACE()

#endif // RENDER_POSTPROCESS_RENDER_POST_PROCESS_COMBINED_NODE_H
