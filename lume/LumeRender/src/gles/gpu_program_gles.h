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

#ifndef GLES_GPU_PROGRAM_GLES_H
#define GLES_GPU_PROGRAM_GLES_H

#include <base/containers/array_view.h>
#include <base/containers/unique_ptr.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

#include "device/gpu_program.h"
#include "gles/spirv_cross_helper_structs_gles.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class DeviceGLES;
class ShaderModuleGLES;
struct PushConstantReflection;
struct OES_Bind {
    uint8_t set { 0 }, bind { 0 };
};
struct Binder {
    uint32_t set;
    uint32_t bind;
    DescriptorType type;
    // [descriptorarrayindex][glindices] for separate sample/image case, there can be multiple texture units to bind to,
    // for other resources the second(inner) vector length is always 1.
    BASE_NS::vector<BASE_NS::vector<uint32_t>> id;
};

struct GpuShaderProgramPlatformDataGL final {
    uint32_t program { 0 };
    int32_t flipLocation { Gles::INVALID_LOCATION };
    BASE_NS::array_view<Binder> resourceList;
    BASE_NS::array_view<Gles::PushConstantReflection> pushConstants;
    int32_t inputs[Gles::ResourceLimits::MAX_VERTEXINPUT_ATTRIBUTES] {};
    const ShaderModuleGLES* vertShaderModule_ { nullptr };
    const ShaderModuleGLES* fragShaderModule_ { nullptr };
};

class GpuShaderProgramGLES final : public GpuShaderProgram {
public:
    GpuShaderProgramGLES(Device& device, const GpuShaderProgramCreateData& createData);
    ~GpuShaderProgramGLES();

    const GpuShaderProgramPlatformDataGL& GetPlatformData() const;
    const ShaderReflection& GetReflection() const override;

    BASE_NS::unique_ptr<GpuShaderProgramGLES> Specialize(
        const ShaderSpecializationConstantDataView& specialization, uint32_t views) const;

    BASE_NS::unique_ptr<GpuShaderProgramGLES> OesPatch(
        const BASE_NS::array_view<const OES_Bind>& binds, uint32_t views) const;

private:
    BASE_NS::unique_ptr<GpuShaderProgramGLES> Specialize(const ShaderSpecializationConstantDataView& specData,
        const BASE_NS::array_view<const OES_Bind>& oesBinds, uint32_t views) const;
    void FilterInputs(GpuShaderProgramGLES& ret) const;
    GpuShaderProgramGLES(Device& device);
    DeviceGLES& device_;
    GpuShaderProgramPlatformDataGL plat_;
    BASE_NS::vector<ShaderSpecialization::Constant> constants_;
    ShaderReflection reflection_;

    BASE_NS::vector<Binder> resourceList;
    BASE_NS::vector<Gles::PushConstantReflection> pushConstants;
    // copy of specialization data used..
    BASE_NS::vector<uint32_t> specializedWith;
};

struct GpuComputeProgramPlatformDataGL final {
    uint32_t program { 0 };
    int32_t flipLocation { Gles::INVALID_LOCATION };
    BASE_NS::array_view<Binder> resourceList;
    BASE_NS::array_view<Gles::PushConstantReflection> pushConstants;
    const ShaderModuleGLES* module_ { nullptr };
};

class GpuComputeProgramGLES final : public GpuComputeProgram {
public:
    GpuComputeProgramGLES(Device& device, const GpuComputeProgramCreateData& createData);
    ~GpuComputeProgramGLES();

    const GpuComputeProgramPlatformDataGL& GetPlatformData() const;
    const ComputeShaderReflection& GetReflection() const override;

    BASE_NS::unique_ptr<GpuComputeProgramGLES> Specialize(
        const ShaderSpecializationConstantDataView& specialization) const;

private:
    GpuComputeProgramGLES(Device& device);
    DeviceGLES& device_;
    GpuComputeProgramPlatformDataGL plat_;
    BASE_NS::vector<ShaderSpecialization::Constant> constants_;
    ComputeShaderReflection reflection_;
    BASE_NS::vector<Binder> resourceList;
    BASE_NS::vector<Gles::PushConstantReflection> pushConstants;
};
RENDER_END_NAMESPACE()

#endif // GLES_GPU_PROGRAM_GLES_H
