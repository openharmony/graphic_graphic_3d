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

#ifndef MALEOON_SHADER_MODULE_MLN_H
#define MALEOON_SHADER_MODULE_MLN_H

#include <base/containers/vector.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

#include "device/shader_module.h"

#include <maleoon/maleoon.h>

RENDER_BEGIN_NAMESPACE()

class Device;
struct ShaderModuleCreateInfo;

struct ShaderModulePlatformDataMln : ShaderModulePlatformData {
    // New API: no MlnShader handle. SPIR-V pointer for inline MlnShaderDescriptor at PSO creation.
    // Points into ShaderModuleMln::spvData_ (owned copy), guaranteed valid for module lifetime.
    const uint32_t* spvSource { nullptr };
    size_t spvSourceSize { 0 };
};

class ShaderModuleMln final : public ShaderModule {
public:
    ShaderModuleMln(Device& device, const ShaderModuleCreateInfo& createInfo);
    ~ShaderModuleMln() override;

    ShaderStageFlags GetShaderStageFlags() const override;
    const ShaderModulePlatformData& GetPlatformData() const override;
    const PipelineLayout& GetPipelineLayout() const override;
    ShaderSpecializationConstantView GetSpecilization() const override;
    VertexInputDeclarationView GetVertexInputDeclaration() const override;
    ShaderThreadGroup GetThreadGroupSize() const override;

    const ShaderModulePlatformDataMln& GetPlatformDataMln() const;

private:
    Device& device_;
    ShaderStageFlags shaderStageFlags_ { 0 };
    ShaderModulePlatformDataMln plat_;
    BASE_NS::vector<uint8_t> spvData_;  // Owned copy of SPIR-V bytecode (4-byte aligned by vector allocator)
    PipelineLayout pipelineLayout_;
    BASE_NS::vector<ShaderSpecialization::Constant> constants_;
    ShaderSpecializationConstantView sscv_;
    BASE_NS::vector<VertexInputDeclaration::VertexInputBindingDescription> vertexInputBindingDescriptions_;
    BASE_NS::vector<VertexInputDeclaration::VertexInputAttributeDescription> vertexInputAttributeDescriptions_;
    VertexInputDeclarationView vidv_;
    ShaderThreadGroup stg_ { 0, 0, 0 };
};

RENDER_END_NAMESPACE()

#endif // MALEOON_SHADER_MODULE_MLN_H
