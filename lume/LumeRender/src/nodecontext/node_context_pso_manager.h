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

#ifndef RENDER_RENDER__NODE_CONTEXT_PSO_MANAGER_H
#define RENDER_RENDER__NODE_CONTEXT_PSO_MANAGER_H

#include <cstddef>
#include <cstdint>

#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "device/gpu_resource_handle_util.h"
#include "device/pipeline_state_object.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class ShaderManager;
struct PipelineLayout;
struct ViewportDesc;
struct ScissorDesc;
struct LowLevelRenderPassData;
struct LowLevelPipelineLayoutData;

struct ShaderSpecializationConstantDataWrapper {
    BASE_NS::vector<ShaderSpecialization::Constant> constants;
    BASE_NS::vector<uint32_t> data;
};
struct VertexInputDeclarationDataWrapper {
    BASE_NS::vector<VertexInputDeclaration::VertexInputBindingDescription> bindingDescriptions;
    BASE_NS::vector<VertexInputDeclaration::VertexInputAttributeDescription> attributeDescriptions;
};

/**
NodeContextPsoManager implementation.
*/
class NodeContextPsoManager final : public INodeContextPsoManager {
public:
    NodeContextPsoManager(Device& device, ShaderManager& shaderManager);
    ~NodeContextPsoManager() = default;

    void BeginBackendFrame();

    RenderHandle GetComputePsoHandle(const RenderHandle shader, const RenderHandle pipelineLayout,
        const ShaderSpecializationConstantDataView& shaderSpecialization) override;
    RenderHandle GetComputePsoHandle(const RenderHandle shader, const PipelineLayout& pipelineLayout,
        const ShaderSpecializationConstantDataView& shaderSpecialization) override;

    RenderHandle GetGraphicsPsoHandle(const RenderHandle shader, const RenderHandle graphicsState,
        const RenderHandle pipelineLayout, const RenderHandle vertexInputDeclaration,
        const ShaderSpecializationConstantDataView& shaderSpecialization,
        const BASE_NS::array_view<const DynamicStateEnum> dynamicStates) override;
    RenderHandle GetGraphicsPsoHandle(const RenderHandle shader, const RenderHandle graphicsState,
        const PipelineLayout& pipelineLayout, const VertexInputDeclarationView& vertexInputDeclarationView,
        const ShaderSpecializationConstantDataView& shaderSpecialization,
        const BASE_NS::array_view<const DynamicStateEnum> dynamicStates) override;
    RenderHandle GetGraphicsPsoHandle(const RenderHandle shader, const GraphicsState& graphicsState,
        const PipelineLayout& pipelineLayout, const VertexInputDeclarationView& vertexInputDeclarationView,
        const ShaderSpecializationConstantDataView& shaderSpecialization,
        const BASE_NS::array_view<const DynamicStateEnum> dynamicStates) override;

    const ComputePipelineStateObject* GetComputePso(
        const RenderHandle handle, const LowLevelPipelineLayoutData* pipelineLayoutData);
    // with GL(ES) psoStateHash is 0 and renderPassData, and pipelineLayoutData are nullptr
    // with vulkan psoStateHash has renderpass compatibility hash and additional descriptor set hash
    const GraphicsPipelineStateObject* GetGraphicsPso(const RenderHandle handle, const RenderPassDesc& renderPassDesc,
        const BASE_NS::array_view<const RenderPassSubpassDesc> renderPassSubpassDescs, const uint32_t subpassIndex,
        const uint64_t psoStateHash, const LowLevelRenderPassData* renderPassData,
        const LowLevelPipelineLayoutData* pipelineLayoutData);

    // only used for validation (just copy pipeline layout)
#if (RENDER_VALIDATION_ENABLED == 1)
    const PipelineLayout& GetComputePsoPipelineLayout(const RenderHandle handle) const;
    const PipelineLayout& GetGraphicsPsoPipelineLayout(const RenderHandle handle) const;
#endif

private:
    Device& device_;
    ShaderManager& shaderMgr_;

    // graphics state handle should be invalid if custom graphics state is given
    RenderHandle GetGraphicsPsoHandleImpl(const RenderHandle shaderHandle, const RenderHandle graphicsStateHandle,
        const PipelineLayout& pipelineLayout, const VertexInputDeclarationView& vertexInputDeclarationView,
        const ShaderSpecializationConstantDataView& shaderSpecialization,
        const BASE_NS::array_view<const DynamicStateEnum> dynamicStates, const GraphicsState* graphicsState);

    struct ComputePipelineStateCreationData {
        RenderHandle shaderHandle;
        PipelineLayout pipelineLayout;
        ShaderSpecializationConstantDataWrapper shaderSpecialization;
    };
    struct ComputePipelineStateCache {
        BASE_NS::vector<ComputePipelineStateCreationData> psoCreationData;
        BASE_NS::vector<BASE_NS::unique_ptr<ComputePipelineStateObject>> pipelineStateObjects;
        // hash (shader hash), resource handle
        BASE_NS::unordered_map<uint64_t, RenderHandle> hashToHandle;

        struct DestroyData {
            BASE_NS::unique_ptr<ComputePipelineStateObject> pso;
            uint64_t frameIndex { 0 };
        };
        BASE_NS::vector<DestroyData> pendingPsoDestroys;

#if (RENDER_VALIDATION_ENABLED == 1)
        BASE_NS::unordered_map<RenderHandle, PipelineLayout> handleToPipelineLayout;
#endif
    };
    ComputePipelineStateCache computePipelineStateCache_;

    struct GraphicsPipelineStateCreationData {
        RenderHandle shaderHandle;
        RenderHandle graphicsStateHandle;
        PipelineLayout pipelineLayout;

        VertexInputDeclarationDataWrapper vertexInputDeclaration;
        ShaderSpecializationConstantDataWrapper shaderSpecialization;
        BASE_NS::unique_ptr<GraphicsState> customGraphicsState;
        BASE_NS::vector<DynamicStateEnum> dynamicStates;
    };
    struct GraphicsPipelineStateCache {
        BASE_NS::vector<GraphicsPipelineStateCreationData> psoCreationData;
        // (handle.id (+ vk renderpass compatibility hash)
        // vulkan needs pso per every render pass configuration (for GL array would be enough, but we use
        // renderhandle.id)
        struct PsoData {
            BASE_NS::unique_ptr<GraphicsPipelineStateObject> pso;
            RenderHandle shaderHandle;
        };
        BASE_NS::unordered_map<uint64_t, PsoData> pipelineStateObjects;
        // hash (shader hash), resource handle
        BASE_NS::unordered_map<uint64_t, RenderHandle> hashToHandle;

        struct DestroyData {
            BASE_NS::unique_ptr<GraphicsPipelineStateObject> pso;
            uint64_t frameIndex { 0 };
        };
        BASE_NS::vector<DestroyData> pendingPsoDestroys;

#if (RENDER_VALIDATION_ENABLED == 1)
        BASE_NS::unordered_map<RenderHandle, PipelineLayout> handleToPipelineLayout;
#endif
    };
    GraphicsPipelineStateCache graphicsPipelineStateCache_;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE_CONTEXT_PSO_MANAGER_H
