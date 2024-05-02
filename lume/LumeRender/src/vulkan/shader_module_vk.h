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

#ifndef VULKAN_SHADER_MODULE_VK_H
#define VULKAN_SHADER_MODULE_VK_H

#include <vulkan/vulkan_core.h>

#include <render/device/gpu_resource_desc.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>

#include "device/shader_module.h"

RENDER_BEGIN_NAMESPACE()
class Device;
struct ShaderModuleCreateInfo;

struct ShaderModulePlatformDataVk : ShaderModulePlatformData {
    VkShaderModule shaderModule { VK_NULL_HANDLE };
};

class ShaderModuleVk final : public ShaderModule {
public:
    ShaderModuleVk(Device& device, const ShaderModuleCreateInfo& createInfo);
    ~ShaderModuleVk();

    ShaderStageFlags GetShaderStageFlags() const override;

    const ShaderModulePlatformData& GetPlatformData() const override;

    const PipelineLayout& GetPipelineLayout() const override;
    ShaderSpecializationConstantView GetSpecilization() const override;

    VertexInputDeclarationView GetVertexInputDeclaration() const override;
    ShaderThreadGroup GetThreadGroupSize() const override;

private:
    Device& device_;

    ShaderStageFlags shaderStageFlags_ { 0u };
    ShaderModulePlatformDataVk plat_;

    BASE_NS::vector<VertexInputDeclaration::VertexInputBindingDescription> vertexInputBindingDescriptions_;
    BASE_NS::vector<VertexInputDeclaration::VertexInputAttributeDescription> vertexInputAttributeDescriptions_;
    BASE_NS::vector<ShaderSpecialization::Constant> constants_;

    PipelineLayout pipelineLayout_;
    ShaderSpecializationConstantView sscv_;
    VertexInputDeclarationView vidv_;
    ShaderThreadGroup stg_;
};
RENDER_END_NAMESPACE()

#endif // VULKAN_SHADER_MODULE_VK_H
