/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
    ShaderSpecilizationConstantView GetSpecilization() const override;

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
    ShaderSpecilizationConstantView sscv_;
    VertexInputDeclarationView vidv_;
    ShaderThreadGroup stg_;
};
RENDER_END_NAMESPACE()

#endif // VULKAN_SHADER_MODULE_VK_H
