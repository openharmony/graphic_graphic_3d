/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef VULKAN_PIPELINE_STATE_OBJECT_VK_H
#define VULKAN_PIPELINE_STATE_OBJECT_VK_H

#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

#include "device/pipeline_state_object.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class GpuComputeProgram;
class GpuShaderProgram;
struct PipelineLayout;
struct LowLevelRenderPassData;
struct LowLevelPipelineLayoutData;

struct PipelineStateObjectPlatformDataVk final {
    VkPipeline pipeline { VK_NULL_HANDLE };
    VkPipelineLayout pipelineLayout { VK_NULL_HANDLE };
};

class GraphicsPipelineStateObjectVk final : public GraphicsPipelineStateObject {
public:
    GraphicsPipelineStateObjectVk(Device& device, const GpuShaderProgram& gpuShaderProgram,
        const GraphicsState& graphicsState, const PipelineLayout& pipelineLayout,
        const VertexInputDeclarationView& vertexInputDeclaration,
        const ShaderSpecializationConstantDataView& specializationConstants, const DynamicStateFlags dynamicStateFlags,
        const RenderPassDesc& renderPassDesc,
        const BASE_NS::array_view<const RenderPassSubpassDesc>& renderPassSubpassDescs, const uint32_t subpassIndex,
        const LowLevelRenderPassData& renderPassData, const LowLevelPipelineLayoutData& pipelineLayoutData);
    ~GraphicsPipelineStateObjectVk();

    const PipelineStateObjectPlatformDataVk& GetPlatformData() const;

private:
    Device& device_;

    PipelineStateObjectPlatformDataVk plat_;
};

class ComputePipelineStateObjectVk final : public ComputePipelineStateObject {
public:
    ComputePipelineStateObjectVk(Device& device, const GpuComputeProgram& gpuComputeProgram,
        const PipelineLayout& pipelineLayout, const ShaderSpecializationConstantDataView& specializationConstants,
        const LowLevelPipelineLayoutData& pipelineLayoutData);
    ~ComputePipelineStateObjectVk();

    const PipelineStateObjectPlatformDataVk& GetPlatformData() const;

private:
    Device& device_;

    PipelineStateObjectPlatformDataVk plat_;
};
RENDER_END_NAMESPACE()

#endif // VULKAN_PIPELINE_STATE_OBJECT_VK_H
