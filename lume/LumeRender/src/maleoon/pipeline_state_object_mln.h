/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef MALEOON_PIPELINE_STATE_OBJECT_MLN_H
#define MALEOON_PIPELINE_STATE_OBJECT_MLN_H

#include <base/containers/vector.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/pipeline_state_object.h"

#include <maleoon/maleoon.h>

RENDER_BEGIN_NAMESPACE()

class Device;
class GpuComputeProgram;
class GpuShaderProgram;
struct GraphicsState;
struct LowLevelPipelineLayoutData;
struct PipelineLayout;
struct RenderPassDesc;
struct RenderPassSubpassDesc;

struct LowLevelRenderPassDataMln final : public LowLevelRenderPassData {
    MlnFormat attachmentFormats[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT]{};
    MlnSampleCountFlags attachmentSampleCounts[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT]{};
    uint32_t attachmentCount{0};
};

struct PipelineStateObjectPlatformDataMln {
    MlnProgram program{MLN_NULL_HANDLE};
    MlnProgramInterface programInterface{MLN_NULL_HANDLE};
    // Binding layouts created from PipelineLayout (owned, destroyed with PSO)
    BASE_NS::vector<MlnBindingLayout> bindingLayouts;

    // Dynamic state types declared in PSO (needed for MlnStateSetDescriptor.dynamicState at draw time)
    static constexpr uint32_t MAX_DYNAMIC_STATE_COUNT{16};
    uint32_t dynamicStateCount{0};
    MlnDynamicStateType dynamicStateTypes[MAX_DYNAMIC_STATE_COUNT]{};

    // Cached graphics state for dynamic stateSet at draw time
    MlnPrimitiveTopology topology{MLN_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
    uint32_t primitiveRestartEnable{0};
    uint32_t rasterizerDiscardEnable{0};
    MlnCullModeFlags cullMode{0};
    MlnFrontFace frontFace{MLN_FRONT_FACE_COUNTER_CLOCKWISE};
    uint32_t depthBiasEnable{0};
    float lineWidth{1.0f};
    uint32_t depthTestEnable{0};
    uint32_t depthWriteEnable{0};
    MlnCompareOp depthCompareOp{MLN_COMPARE_OP_LESS};
    uint32_t depthBoundsTestEnable{0};
    uint32_t stencilTestEnable{0};
    uint64_t staticStateHash{0};
};

class GraphicsPipelineStateObjectMln final : public GraphicsPipelineStateObject {
public:
    GraphicsPipelineStateObjectMln(Device& device, const GpuShaderProgram& gpuProgram,
        const GraphicsState& graphicsState, const PipelineLayout& pipelineLayout,
        const VertexInputDeclarationView& vertexInputDeclaration,
        const ShaderSpecializationConstantDataView& specializationConstants,
        BASE_NS::array_view<const DynamicStateEnum> dynamicStates,
        const BASE_NS::array_view<const RenderPassSubpassDesc>& renderPassSubpassDescs, uint32_t subpassIndex,
        const LowLevelRenderPassData* renderPassData, const LowLevelPipelineLayoutData* pipelineLayoutData);
    ~GraphicsPipelineStateObjectMln() override;

    const PipelineStateObjectPlatformDataMln& GetPlatformData() const;

private:
    Device& device_;
    PipelineStateObjectPlatformDataMln plat_;
};

class ComputePipelineStateObjectMln final : public ComputePipelineStateObject {
public:
    ComputePipelineStateObjectMln(Device& device, const GpuComputeProgram& gpuProgram,
        const PipelineLayout& pipelineLayout, const ShaderSpecializationConstantDataView& specializationConstants,
        const LowLevelPipelineLayoutData* pipelineLayoutData);
    ~ComputePipelineStateObjectMln() override;

    const PipelineStateObjectPlatformDataMln& GetPlatformData() const;

private:
    Device& device_;
    PipelineStateObjectPlatformDataMln plat_;
};

RENDER_END_NAMESPACE()

#endif  // MALEOON_PIPELINE_STATE_OBJECT_MLN_H
