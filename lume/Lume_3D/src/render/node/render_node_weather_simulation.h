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

#ifndef CORE3D_NODE_RENDER_NODE_WEATHER_SIMULATION_H
#define CORE3D_NODE_RENDER_NODE_WEATHER_SIMULATION_H

#include <3d/namespace.h>
#include <base/util/uid.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "render/datastore/render_data_store_weather.h"

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;
class IRenderNodeContextManager;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class RenderNodeWeatherSimulation final : public RENDER_NS::IRenderNode {
public:
    RenderNodeWeatherSimulation() = default;
    ~RenderNodeWeatherSimulation() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "6722cfd9-ff0c-4b8c-bdc1-1ecbd9ae11e1" };
    static constexpr char const* TYPE_NAME = "RenderNodeWeatherSimulation";
    static constexpr RENDER_NS::IRenderNode::BackendFlags BACKEND_FLAGS =
        IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr RENDER_NS::IRenderNode::ClassType CLASS_TYPE = RENDER_NS::IRenderNode::ClassType::CLASS_TYPE_NODE;
    static RENDER_NS::IRenderNode* Create();
    static void Destroy(RENDER_NS::IRenderNode* instance);

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void InitializeRippleBuffers(RENDER_NS::IRenderCommandList& cmdList);

    void ParseRenderNodeInputs();

    // Json resources which might need re-fetching
    struct JsonInputs {
        RENDER_NS::RenderNodeGraphInputs::InputResources resources;
        RENDER_NS::RenderNodeGraphInputs::InputResources dispatchResources;
        RENDER_NS::RenderNodeGraphInputs::RenderDataStore renderDataStore;
        RENDER_NS::RenderNodeGraphInputs::RenderDataStore renderDataStoreSpecialization;

        bool hasChangeableResourceHandles { false };
        bool hasChangeableDispatchHandles { false };
    };
    JsonInputs jsonInputs_;

    RENDER_NS::RenderHandle initShader_;
    RENDER_NS::RenderHandle shader_;

    RENDER_NS::IPipelineDescriptorSetBinder::Ptr pipelineDescriptorSetBinder_;
    RENDER_NS::PipelineLayout pipelineLayout_;
    RENDER_NS::RenderHandle psoHandle_;

    RENDER_NS::IPipelineDescriptorSetBinder::Ptr initPipelineDescriptorSetBinder_;
    RENDER_NS::PipelineLayout initPipelineLayout_;
    RENDER_NS::RenderHandle initPsoHandle_;

    RENDER_NS::RenderHandle defaultMaterialSam_;
    RENDER_NS::RenderHandle rippleTextureHandle_;
    RENDER_NS::RenderHandle rippleTextureHandle1_;
    RENDER_NS::RenderHandle rippleInputArgsBuffer_;

    RenderDataStoreWeather* dataStoreWeather_ { nullptr };

    uint32_t pingPongIdx { 0 };
    bool areTextureInit { false };
};
CORE3D_END_NAMESPACE()

#endif // CORE3D_NODE_RENDER_NODE_WEATHER_SIMULATION_H
