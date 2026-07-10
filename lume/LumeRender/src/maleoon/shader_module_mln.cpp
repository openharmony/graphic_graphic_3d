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

#include "shader_module_mln.h"
#include "mln_log.h"
#include "util/log.h"

#include <cstring>
#include <core/log.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_program_util.h"
#include "device/shader_manager.h"
#include "maleoon/device_mln.h"
#include "mln_convert.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()

ShaderModuleMln::ShaderModuleMln(Device& device, const ShaderModuleCreateInfo& createInfo) : device_(device)
{
    MLN_LOG_INIT("ShaderModuleMln: creating shader module (stageFlags=0x%x, spvSize=%zu)",
        createInfo.shaderStageFlags,
        createInfo.spvData.size());
    shaderStageFlags_ = createInfo.shaderStageFlags;

    // Extract reflection data
    const auto& reflectionData = createInfo.reflectionData;
    pipelineLayout_ = reflectionData.GetPipelineLayout();

    // Specialization constants
    {
        constants_ = reflectionData.GetSpecializationConstants();
        sscv_.constants = constants_;
    }

    // Vertex input declarations (vertex shaders only)
    if (shaderStageFlags_ & CORE_SHADER_STAGE_VERTEX_BIT) {
        vertexInputAttributeDescriptions_ = reflectionData.GetInputDescriptions();
        for (const auto& attrib : vertexInputAttributeDescriptions_) {
            VertexInputDeclaration::VertexInputBindingDescription bindingDesc;
            bindingDesc.binding = attrib.binding;
            bindingDesc.stride = GpuProgramUtil::FormatByteSize(attrib.format);
            bindingDesc.vertexInputRate = VertexInputRate::CORE_VERTEX_INPUT_RATE_VERTEX;
            vertexInputBindingDescriptions_.push_back(bindingDesc);
        }
        vidv_.bindingDescriptions = vertexInputBindingDescriptions_;
        vidv_.attributeDescriptions = vertexInputAttributeDescriptions_;
    }

    // Thread group size (compute shaders)
    if (shaderStageFlags_ & CORE_SHADER_STAGE_COMPUTE_BIT) {
        const auto localSize = reflectionData.GetLocalSize();
        stg_ = {localSize.x, localSize.y, localSize.z};
    }

    // New API: no MlnShader handle. Make an owned copy of the SPIR-V bytecode.
    // The original spvData comes from ShaderManager/ShaderLoader and may be temporary.
    // Owning the copy guarantees the pointer stays valid until this ShaderModule is destroyed.
    if (!createInfo.spvData.empty()) {
        spvData_.resize(createInfo.spvData.size());
        std::copy(createInfo.spvData.begin(), createInfo.spvData.end(), spvData_.begin());
        plat_.spvSource = reinterpret_cast<const uint32_t*>(spvData_.data());
        plat_.spvSourceSize = spvData_.size();
        MLN_LOG_INIT("ShaderModuleMln: copied SPIR-V data (stageFlags=0x%x, spvBytes=%zu)",
            shaderStageFlags_,
            plat_.spvSourceSize);
    }
}

ShaderModuleMln::~ShaderModuleMln()
{
    // spvData_ vector is automatically freed. No MlnShader handle to destroy.
}

ShaderStageFlags ShaderModuleMln::GetShaderStageFlags() const
{
    return shaderStageFlags_;
}

const ShaderModulePlatformData& ShaderModuleMln::GetPlatformData() const
{
    return plat_;
}

const PipelineLayout& ShaderModuleMln::GetPipelineLayout() const
{
    return pipelineLayout_;
}

ShaderSpecializationConstantView ShaderModuleMln::GetSpecilization() const
{
    return sscv_;
}

VertexInputDeclarationView ShaderModuleMln::GetVertexInputDeclaration() const
{
    return vidv_;
}

ShaderThreadGroup ShaderModuleMln::GetThreadGroupSize() const
{
    return stg_;
}

const ShaderModulePlatformDataMln& ShaderModuleMln::GetPlatformDataMln() const
{
    return plat_;
}

RENDER_END_NAMESPACE()
