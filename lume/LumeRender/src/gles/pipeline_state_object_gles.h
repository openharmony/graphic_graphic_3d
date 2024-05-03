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

#ifndef GLES_PIPELINE_STATE_OBJECT_GLES_H
#define GLES_PIPELINE_STATE_OBJECT_GLES_H

#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>

#include "device/pipeline_state_object.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class DeviceGLES;
class GpuShaderProgram;
class GpuComputeProgram;
class GpuComputeProgramGLES;
class GpuShaderProgramGLES;
struct OES_Bind;

struct PipelineStateObjectPlatformDataGL final {
    const GpuComputeProgramGLES* computeShader;
    const GpuShaderProgramGLES* graphicsShader;
    GraphicsState graphicsState;
    PipelineLayout pipelineLayout;
    VertexInputDeclarationData vertexInputDeclaration;
    RenderPassDesc renderPassDesc;
    DynamicStateFlags dynamicStateFlags;
    uint32_t vao;
    uint32_t views;
};

class GraphicsPipelineStateObjectGLES final : public GraphicsPipelineStateObject {
public:
    GraphicsPipelineStateObjectGLES(Device& device, const GpuShaderProgram& gpuShaderProgram,
        const GraphicsState& graphicsState, const PipelineLayout& pipelineLayout,
        const VertexInputDeclarationView& vertexInputDeclaration,
        const ShaderSpecializationConstantDataView& specializationConstants,
        const BASE_NS::array_view<const DynamicStateEnum> dynamicStates, const RenderPassDesc& renderPassDesc,
        const BASE_NS::array_view<const RenderPassSubpassDesc>& renderPassSubpassDescs, const uint32_t subpassIndex);

    ~GraphicsPipelineStateObjectGLES();

    const PipelineStateObjectPlatformDataGL& GetPlatformData() const;

    GpuShaderProgramGLES* GetOESProgram(BASE_NS::array_view<const OES_Bind> oesBinds) const;

private:
    void MakeVAO() noexcept;
    DeviceGLES& device_;

    BASE_NS::unique_ptr<GpuShaderProgramGLES> specialized_;
    PipelineStateObjectPlatformDataGL plat_ {};

    mutable BASE_NS::unordered_map<BASE_NS::string, BASE_NS::unique_ptr<GpuShaderProgramGLES>>
        oesPrograms_; // generated dynamically.
};

class ComputePipelineStateObjectGLES final : public ComputePipelineStateObject {
public:
    ComputePipelineStateObjectGLES(Device& device, const GpuComputeProgram& gpuComputeProgram,
        const PipelineLayout& pipelineLayout, const ShaderSpecializationConstantDataView& specializationConstants);
    ~ComputePipelineStateObjectGLES();

    const PipelineStateObjectPlatformDataGL& GetPlatformData() const;

private:
    DeviceGLES& device_;

    BASE_NS::unique_ptr<GpuComputeProgramGLES> specialized_ { nullptr };
    PipelineStateObjectPlatformDataGL plat_ {};
};
RENDER_END_NAMESPACE()

#endif // GLES_PIPELINE_STATE_OBJECT_GLES_H
