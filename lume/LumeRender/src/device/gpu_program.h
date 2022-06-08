/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
    ShaderSpecilizationConstantView shaderSpecializationConstantView;

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
    ShaderSpecilizationConstantView shaderSpecializationConstantView;
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
