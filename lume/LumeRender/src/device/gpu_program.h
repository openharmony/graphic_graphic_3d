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

#ifndef DEVICE_GPU_PROGRAM_H
#define DEVICE_GPU_PROGRAM_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class ShaderModule;

/** Compute shader reflection */
struct ComputeShaderReflection {
    /** Pipeline layout */
    PipelineLayout pipelineLayout;
    /** Shader specialization constant view */
    ShaderSpecializationConstantView shaderSpecializationConstantView;

    /** Thread group size X */
    uint32_t threadGroupSizeX { 0 };
    /** Thread group size Y */
    uint32_t threadGroupSizeY { 0 };
    /** Thread group size Z */
    uint32_t threadGroupSizeZ { 0 };
};

/** Shader reflection */
struct ShaderReflection {
    /** Pipeline layout */
    PipelineLayout pipelineLayout;
    /** Shader specialization constant view */
    ShaderSpecializationConstantView shaderSpecializationConstantView;
    /** Vertex input declaration view */
    VertexInputDeclarationView vertexInputDeclarationView;
};

struct GpuShaderProgramCreateData {
    ShaderModule* vertShaderModule { nullptr };
    ShaderModule* fragShaderModule { nullptr };
};

/** Shader program.
 * Compiled shader module and shader reflection.
 */
class GpuShaderProgram {
public:
    GpuShaderProgram() = default;
    virtual ~GpuShaderProgram() = default;

    GpuShaderProgram(const GpuShaderProgram&) = delete;
    GpuShaderProgram& operator=(const GpuShaderProgram&) = delete;

    /* Reflection data is a reference and has array_views. Do not hold on to the data. */
    virtual const ShaderReflection& GetReflection() const = 0;
};

struct GpuComputeProgramCreateData {
    ShaderModule* compShaderModule { nullptr };
};

/** Compute shader program.
 * Compiled shader module and shader reflection.
 */
class GpuComputeProgram {
public:
    GpuComputeProgram() = default;
    virtual ~GpuComputeProgram() = default;

    GpuComputeProgram(const GpuComputeProgram&) = delete;
    GpuComputeProgram& operator=(const GpuComputeProgram&) = delete;

    /* Reflection data is a reference and has array_views. Do not hold on to the data. */
    virtual const ComputeShaderReflection& GetReflection() const = 0;
};
RENDER_END_NAMESPACE()

#endif // DEVICE_GPU_PROGRAM_H
