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

#include "shader_module_vk.h"

#include <algorithm>
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_program_util.h"
#include "device/shader_manager.h"
#include "util/log.h"
#include "vulkan/device_vk.h"
#include "vulkan/validate_vk.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
VkShaderModule CreateShaderModule(const VkDevice device, array_view<const uint8_t> data)
{
    PLUGIN_ASSERT(data.size() > 0);
    VkShaderModule shaderModule { VK_NULL_HANDLE };

    constexpr VkShaderModuleCreateFlags shaderModuleCreateFlags { 0 };
    const VkShaderModuleCreateInfo shaderModuleCreateInfo {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,   // sType
        nullptr,                                       // pNext
        shaderModuleCreateFlags,                       // flags
        static_cast<uint32_t>(data.size()),            // codeSize
        reinterpret_cast<const uint32_t*>(data.data()) // pCode
    };

    VALIDATE_VK_RESULT(vkCreateShaderModule(device, // device
        &shaderModuleCreateInfo,                    // pCreateInfo
        nullptr,                                    // pAllocator
        &shaderModule));                            // pShaderModule

    return shaderModule;
}
} // namespace

ShaderModuleVk::ShaderModuleVk(Device& device, const ShaderModuleCreateInfo& createInfo)
    : device_(device), shaderStageFlags_(createInfo.shaderStageFlags)
{
    PLUGIN_ASSERT(createInfo.spvData.size() > 0);
    PLUGIN_ASSERT(createInfo.shaderStageFlags & (ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT |
                                                    ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT |
                                                    ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT));

    bool valid = false;
    if (createInfo.reflectionData.IsValid()) {
        valid = true;
        pipelineLayout_ = createInfo.reflectionData.GetPipelineLayout();

        constants_ = createInfo.reflectionData.GetSpecializationConstants();
        sscv_.constants = constants_;

        if (shaderStageFlags_ == ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT) {
            vertexInputAttributeDescriptions_ = createInfo.reflectionData.GetInputDescriptions();
            for (const auto& attrib : vertexInputAttributeDescriptions_) {
                VertexInputDeclaration::VertexInputBindingDescription bindingDesc;
                bindingDesc.binding = attrib.binding;
                bindingDesc.stride = GpuProgramUtil::FormatByteSize(attrib.format);
                bindingDesc.vertexInputRate = VertexInputRate::CORE_VERTEX_INPUT_RATE_VERTEX;
                vertexInputBindingDescriptions_.push_back(bindingDesc);
            }
            vidv_.bindingDescriptions = vertexInputBindingDescriptions_;
            vidv_.attributeDescriptions = vertexInputAttributeDescriptions_;
        } else if (shaderStageFlags_ == ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT) {
        } else if (shaderStageFlags_ == ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT) {
            const Math::UVec3 tgs = createInfo.reflectionData.GetLocalSize();
            stg_.x = tgs[0u];
            stg_.y = tgs[1u];
            stg_.z = tgs[2u];
        } else {
            PLUGIN_LOG_E("invalid shader stage flags for module creation");
            valid = false;
        }
    }

    // NOTE: sorting not needed?

    if (valid) {
        const VkDevice vkDevice = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
        plat_.shaderModule = CreateShaderModule(vkDevice, createInfo.spvData);
    } else {
        PLUGIN_LOG_E("invalid vulkan shader module");
    }
}

ShaderModuleVk::~ShaderModuleVk()
{
    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
    if (plat_.shaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, // device
            plat_.shaderModule,       // shaderModule
            nullptr);                 // pAllocator
    }
}

ShaderStageFlags ShaderModuleVk::GetShaderStageFlags() const
{
    return shaderStageFlags_;
}

const ShaderModulePlatformData& ShaderModuleVk::GetPlatformData() const
{
    return plat_;
}

const PipelineLayout& ShaderModuleVk::GetPipelineLayout() const
{
    return pipelineLayout_;
}

ShaderSpecializationConstantView ShaderModuleVk::GetSpecilization() const
{
    return sscv_;
}

VertexInputDeclarationView ShaderModuleVk::GetVertexInputDeclaration() const
{
    return vidv_;
}

ShaderThreadGroup ShaderModuleVk::GetThreadGroupSize() const
{
    return stg_;
}
RENDER_END_NAMESPACE()
