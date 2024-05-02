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

#include "gpu_program_vk.h"

#include <algorithm>
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <base/containers/array_view.h>
#include <base/containers/vector.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_program_util.h"
#include "device/shader_module.h"
#include "util/log.h"
#include "vulkan/device_vk.h"
#include "vulkan/shader_module_vk.h"
#include "vulkan/validate_vk.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
GpuShaderProgramVk::GpuShaderProgramVk(Device& device, const GpuShaderProgramCreateData& createData)
    : GpuShaderProgram()
{
    PLUGIN_ASSERT(createData.vertShaderModule);
    PLUGIN_ASSERT(createData.fragShaderModule);

    // combine vertex and fragment shader data
    if (createData.vertShaderModule && createData.fragShaderModule) {
        vertShaderModule_ = static_cast<ShaderModuleVk*>(createData.vertShaderModule);
        fragShaderModule_ = static_cast<ShaderModuleVk*>(createData.fragShaderModule);
        auto& pipelineLayout = reflection_.pipelineLayout;

        { // vert
            const ShaderModuleVk& mod = *vertShaderModule_;
            plat_.vert = ((const ShaderModulePlatformDataVk&)mod.GetPlatformData()).shaderModule;
            pipelineLayout = mod.GetPipelineLayout();
            const auto& sscv = mod.GetSpecilization();
            // has sort inside
            GpuProgramUtil::CombineSpecializationConstants(sscv.constants, constants_);

            // not owned, directly reflected from vertex shader module
            const auto& vidv = mod.GetVertexInputDeclaration();
            reflection_.vertexInputDeclarationView.bindingDescriptions = vidv.bindingDescriptions;
            reflection_.vertexInputDeclarationView.attributeDescriptions = vidv.attributeDescriptions;
        }
        { // frag
            const ShaderModuleVk& mod = *fragShaderModule_;
            plat_.frag = ((const ShaderModulePlatformDataVk&)mod.GetPlatformData()).shaderModule;

            const auto& sscv = mod.GetSpecilization();
            // has sort inside
            GpuProgramUtil::CombineSpecializationConstants(sscv.constants, constants_);

            const auto& reflPl = mod.GetPipelineLayout();
            // has sort inside
            GpuProgramUtil::CombinePipelineLayouts({ &reflPl, 1u }, pipelineLayout);
        }

        reflection_.shaderSpecializationConstantView.constants =
            array_view<ShaderSpecialization::Constant const>(constants_.data(), constants_.size());
    }
}

const GpuShaderProgramPlatformDataVk& GpuShaderProgramVk::GetPlatformData() const
{
    return plat_;
}

const ShaderReflection& GpuShaderProgramVk::GetReflection() const
{
    return reflection_;
}

GpuComputeProgramVk::GpuComputeProgramVk(Device& device, const GpuComputeProgramCreateData& createData)
    : GpuComputeProgram()
{
    PLUGIN_ASSERT(createData.compShaderModule);

    if (createData.compShaderModule) {
        shaderModule_ = static_cast<ShaderModuleVk*>(createData.compShaderModule);
        {
            const ShaderModuleVk& mod = *shaderModule_;
            plat_.comp = ((const ShaderModulePlatformDataVk&)mod.GetPlatformData()).shaderModule;
            // copy needed data
            reflection_.pipelineLayout = mod.GetPipelineLayout();
            const auto& tgs = mod.GetThreadGroupSize();
            reflection_.threadGroupSizeX = Math::max(1u, tgs.x);
            reflection_.threadGroupSizeY = Math::max(1u, tgs.y);
            reflection_.threadGroupSizeZ = Math::max(1u, tgs.z);
            const auto& sscv = mod.GetSpecilization();
            constants_ =
                vector<ShaderSpecialization::Constant>(sscv.constants.cbegin().ptr(), sscv.constants.cend().ptr());
        }

        reflection_.shaderSpecializationConstantView.constants =
            array_view<ShaderSpecialization::Constant const>(constants_.data(), constants_.size());
    }
}

const GpuComputeProgramPlatformDataVk& GpuComputeProgramVk::GetPlatformData() const
{
    return plat_;
}

const ComputeShaderReflection& GpuComputeProgramVk::GetReflection() const
{
    return reflection_;
}
RENDER_END_NAMESPACE()
