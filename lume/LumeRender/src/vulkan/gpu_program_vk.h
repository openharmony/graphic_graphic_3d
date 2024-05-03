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

#ifndef VULKAN_GPU_PROGRAM_VK_H
#define VULKAN_GPU_PROGRAM_VK_H

#include <vulkan/vulkan_core.h>

#include <render/device/gpu_resource_desc.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>

#include "device/gpu_program.h"

RENDER_BEGIN_NAMESPACE()
class ShaderModuleVk;
class Device;

struct GpuShaderProgramPlatformDataVk final {
    // modules are not owned by the GpuShaderProgram
    VkShaderModule vert { VK_NULL_HANDLE };
    VkShaderModule frag { VK_NULL_HANDLE };
};

class GpuShaderProgramVk final : public GpuShaderProgram {
public:
    GpuShaderProgramVk(Device& aDevice, const GpuShaderProgramCreateData& aCreateData);
    ~GpuShaderProgramVk() = default;

    const GpuShaderProgramPlatformDataVk& GetPlatformData() const;
    const ShaderReflection& GetReflection() const override;

private:
    ShaderModuleVk* vertShaderModule_ { nullptr };
    ShaderModuleVk* fragShaderModule_ { nullptr };

    GpuShaderProgramPlatformDataVk plat_;
    BASE_NS::vector<ShaderSpecialization::Constant> constants_;

    ShaderReflection reflection_;
};

struct GpuComputeProgramPlatformDataVk final {
    // module is not owned by the GpuShaderProgram
    VkShaderModule comp { VK_NULL_HANDLE };
};

class GpuComputeProgramVk final : public GpuComputeProgram {
public:
    GpuComputeProgramVk(Device& aDevice, const GpuComputeProgramCreateData& aCreateData);
    ~GpuComputeProgramVk() = default;

    const GpuComputeProgramPlatformDataVk& GetPlatformData() const;
    const ComputeShaderReflection& GetReflection() const override;

private:
    ShaderModuleVk* shaderModule_ { nullptr };

    GpuComputeProgramPlatformDataVk plat_;
    BASE_NS::vector<ShaderSpecialization::Constant> constants_;
    ComputeShaderReflection reflection_;
};
RENDER_END_NAMESPACE()

#endif // VULKAN_GPU_PROGRAM_VK_H
