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

#include "util/log.h"
#include "mln_log.h"
#include "gpu_program_mln.h"

#include <core/log.h>
#include <render/namespace.h>

#include "device/gpu_program_util.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()

// ============================================================================
// GpuShaderProgramMln
// ============================================================================

GpuShaderProgramMln::GpuShaderProgramMln(const GpuShaderProgramCreateData& createData)
{
    MLN_LOG_INIT("GpuShaderProgramMln: creating graphics shader program (vert=%p, frag=%p)",
        createData.vertShaderModule, createData.fragShaderModule);
    if (createData.vertShaderModule) {
        vertShaderModule_ = static_cast<ShaderModuleMln*>(createData.vertShaderModule);
        const auto& vertPlat = vertShaderModule_->GetPlatformDataMln();
        plat_.vertSpvSource = vertPlat.spvSource;
        plat_.vertSpvSourceSize = vertPlat.spvSourceSize;
    }
    if (createData.fragShaderModule) {
        fragShaderModule_ = static_cast<ShaderModuleMln*>(createData.fragShaderModule);
        const auto& fragPlat = fragShaderModule_->GetPlatformDataMln();
        plat_.fragSpvSource = fragPlat.spvSource;
        plat_.fragSpvSourceSize = fragPlat.spvSourceSize;
    }

    // Combine specialization constants from both shaders (recalculate offsets)
    // Raw shader reflection returns offset=0 for all constants.
    // CombineSpecializationConstants assigns sequential offsets (0, 4, 8, ...) and sorts.
    if (vertShaderModule_) {
        const auto vertConst = vertShaderModule_->GetSpecilization();
        GpuProgramUtil::CombineSpecializationConstants(vertConst.constants, constants_);
    }
    if (fragShaderModule_) {
        const auto fragConst = fragShaderModule_->GetSpecilization();
        GpuProgramUtil::CombineSpecializationConstants(fragConst.constants, constants_);
    }

    // Combine pipeline layouts
    if (vertShaderModule_) {
        reflection_.pipelineLayout = vertShaderModule_->GetPipelineLayout();
    }
    if (fragShaderModule_) {
        // Merge fragment shader layout into the combined layout
        const auto& fragLayout = fragShaderModule_->GetPipelineLayout();
        auto& combinedLayout = reflection_.pipelineLayout;
        // Extend descriptor set layouts if fragment shader uses more sets
        for (uint32_t s = 0; s < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++s) {
            const auto& fragSet = fragLayout.descriptorSetLayouts[s];
            auto& combinedSet = combinedLayout.descriptorSetLayouts[s];
            for (size_t b = 0; b < fragSet.bindings.size(); ++b) {
                // Add fragment bindings that don't conflict
                bool found = false;
                for (size_t cb = 0; cb < combinedSet.bindings.size(); ++cb) {
                    if (combinedSet.bindings[cb].binding == fragSet.bindings[b].binding) {
                        combinedSet.bindings[cb].shaderStageFlags |= fragSet.bindings[b].shaderStageFlags;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    combinedSet.bindings.push_back(fragSet.bindings[b]);
                }
            }
            combinedSet.set = s;
        }
        // Merge push constants
        if (fragLayout.pushConstant.byteSize > 0) {
            auto& pc = combinedLayout.pushConstant;
            if (pc.byteSize == 0) {
                pc = fragLayout.pushConstant;
            } else {
                pc.shaderStageFlags |= fragLayout.pushConstant.shaderStageFlags;
                const uint32_t fragEnd = fragLayout.pushConstant.byteSize;
                if (fragEnd > pc.byteSize) {
                    pc.byteSize = fragEnd;
                }
            }
        }
    }

    reflection_.shaderSpecializationConstantView = { constants_ };

    // Vertex input from vertex shader only
    if (vertShaderModule_) {
        reflection_.vertexInputDeclarationView = vertShaderModule_->GetVertexInputDeclaration();
    }
}

const ShaderReflection& GpuShaderProgramMln::GetReflection() const
{
    return reflection_;
}

const GpuShaderProgramPlatformDataMln& GpuShaderProgramMln::GetPlatformData() const
{
    return plat_;
}

// ============================================================================
// GpuComputeProgramMln
// ============================================================================

GpuComputeProgramMln::GpuComputeProgramMln(const GpuComputeProgramCreateData& createData)
{
    MLN_LOG_INIT("GpuComputeProgramMln: creating compute shader program (comp=%p)", createData.compShaderModule);
    if (createData.compShaderModule) {
        shaderModule_ = static_cast<ShaderModuleMln*>(createData.compShaderModule);
        const auto& compPlat = shaderModule_->GetPlatformDataMln();
        plat_.compSpvSource = compPlat.spvSource;
        plat_.compSpvSourceSize = compPlat.spvSourceSize;

        reflection_.pipelineLayout = shaderModule_->GetPipelineLayout();

        const auto specConst = shaderModule_->GetSpecilization();
        constants_.clear();
        GpuProgramUtil::CombineSpecializationConstants(specConst.constants, constants_);
        reflection_.shaderSpecializationConstantView = { constants_ };

        const auto tgs = shaderModule_->GetThreadGroupSize();
        reflection_.threadGroupSizeX = tgs.x;
        reflection_.threadGroupSizeY = tgs.y;
        reflection_.threadGroupSizeZ = tgs.z;
    }
}

const ComputeShaderReflection& GpuComputeProgramMln::GetReflection() const
{
    return reflection_;
}

const GpuComputeProgramPlatformDataMln& GpuComputeProgramMln::GetPlatformData() const
{
    return plat_;
}

RENDER_END_NAMESPACE()
