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

#ifndef MALEOON_GPU_PROGRAM_MLN_H
#define MALEOON_GPU_PROGRAM_MLN_H

#include <base/containers/vector.h>
#include <render/namespace.h>

#include "device/gpu_program.h"
#include "maleoon/shader_module_mln.h"

#include <maleoon/maleoon.h>

RENDER_BEGIN_NAMESPACE()

struct GpuShaderProgramPlatformDataMln {
    // New API: no MlnShader handles. Store SPIR-V pointers for inline MlnShaderDescriptor.
    const uint32_t* vertSpvSource { nullptr };
    size_t vertSpvSourceSize { 0 };
    const uint32_t* fragSpvSource { nullptr };
    size_t fragSpvSourceSize { 0 };
};

class GpuShaderProgramMln final : public GpuShaderProgram {
public:
    explicit GpuShaderProgramMln(const GpuShaderProgramCreateData& createData);
    ~GpuShaderProgramMln() override = default;

    const ShaderReflection& GetReflection() const override;
    const GpuShaderProgramPlatformDataMln& GetPlatformData() const;

private:
    ShaderModuleMln* vertShaderModule_ { nullptr };
    ShaderModuleMln* fragShaderModule_ { nullptr };
    GpuShaderProgramPlatformDataMln plat_;
    BASE_NS::vector<ShaderSpecialization::Constant> constants_;
    ShaderReflection reflection_;
};

struct GpuComputeProgramPlatformDataMln {
    // New API: no MlnShader handle. Store SPIR-V pointer for inline MlnShaderDescriptor.
    const uint32_t* compSpvSource { nullptr };
    size_t compSpvSourceSize { 0 };
};

class GpuComputeProgramMln final : public GpuComputeProgram {
public:
    explicit GpuComputeProgramMln(const GpuComputeProgramCreateData& createData);
    ~GpuComputeProgramMln() override = default;

    const ComputeShaderReflection& GetReflection() const override;
    const GpuComputeProgramPlatformDataMln& GetPlatformData() const;

private:
    ShaderModuleMln* shaderModule_ { nullptr };
    GpuComputeProgramPlatformDataMln plat_;
    BASE_NS::vector<ShaderSpecialization::Constant> constants_;
    ComputeShaderReflection reflection_;
};

RENDER_END_NAMESPACE()

#endif // MALEOON_GPU_PROGRAM_MLN_H
