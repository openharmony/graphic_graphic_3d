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

#ifndef VULKAN_PIPELINE_STATE_OBJECT_VK_H
#define VULKAN_PIPELINE_STATE_OBJECT_VK_H

#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <base/containers/vector.h>
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
        const ShaderSpecializationConstantDataView& specializationConstants,
        const BASE_NS::array_view<const DynamicStateEnum> dynamicStates,
        const BASE_NS::array_view<const RenderPassSubpassDesc>& renderPassSubpassDescs, const uint32_t subpassIndex,
        const LowLevelRenderPassData& renderPassData, const LowLevelPipelineLayoutData& pipelineLayoutData);
    ~GraphicsPipelineStateObjectVk() override;

    const PipelineStateObjectPlatformDataVk& GetPlatformData() const;

    struct SpecializationData {
        BASE_NS::vector<VkSpecializationMapEntry> vs;
        BASE_NS::vector<VkSpecializationMapEntry> fs;

        // data copied into this for additional modifications
        struct ShaderSpecializationConstantData {
            BASE_NS::vector<ShaderSpecialization::Constant> constants;
            BASE_NS::vector<uint32_t> data;
        };
        ShaderSpecializationConstantData constantData;

        SurfaceTransformFlags surfaceTransformFlags { 0U };

        void Clear()
        {
            vs.clear();
            fs.clear();
            constantData.constants.clear();
            constantData.data.clear();
            surfaceTransformFlags = 0U;
        }
    };

private:
    Device& device_;

    PipelineStateObjectPlatformDataVk plat_;
    SpecializationData specializationData_;
};

class ComputePipelineStateObjectVk final : public ComputePipelineStateObject {
public:
    ComputePipelineStateObjectVk(Device& device, const GpuComputeProgram& gpuComputeProgram,
        const PipelineLayout& pipelineLayout, const ShaderSpecializationConstantDataView& specializationConstants,
        const LowLevelPipelineLayoutData& pipelineLayoutData);
    ~ComputePipelineStateObjectVk() override;

    const PipelineStateObjectPlatformDataVk& GetPlatformData() const;

private:
    Device& device_;

    PipelineStateObjectPlatformDataVk plat_;
};
RENDER_END_NAMESPACE()

#endif // VULKAN_PIPELINE_STATE_OBJECT_VK_H
