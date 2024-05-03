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

#include "pipeline_state_object_vk.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <base/util/formats.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

#include "device/gpu_program.h"
#include "device/gpu_program_util.h"
#include "device/gpu_resource_handle_util.h"
#include "util/log.h"
#include "vulkan/create_functions_vk.h"
#include "vulkan/device_vk.h"
#include "vulkan/gpu_program_vk.h"
#include "vulkan/pipeline_create_functions_vk.h"
#include "vulkan/validate_vk.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr uint32_t MAX_DYNAMIC_STATE_COUNT { 10u };

constexpr VkDescriptorSetLayoutCreateInfo EMPTY_LAYOUT_INFO {
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, // sType
    nullptr,                                             // pNext
    0U,                                                  // flags
    0U,                                                  // bindingCount
    nullptr,                                             // pBindings
};

void GetVertexInputs(const VertexInputDeclarationView& vertexInputDeclaration,
    vector<VkVertexInputBindingDescription>& vertexInputBindingDescriptions,
    vector<VkVertexInputAttributeDescription>& vertexInputAttributeDescriptions)
{
    vertexInputBindingDescriptions.resize(vertexInputDeclaration.bindingDescriptions.size());
    vertexInputAttributeDescriptions.resize(vertexInputDeclaration.attributeDescriptions.size());

    for (size_t idx = 0; idx < vertexInputBindingDescriptions.size(); ++idx) {
        const auto& bindingRef = vertexInputDeclaration.bindingDescriptions[idx];

        const VkVertexInputRate vertexInputRate = (VkVertexInputRate)bindingRef.vertexInputRate;
        vertexInputBindingDescriptions[idx] = {
            bindingRef.binding, // binding
            bindingRef.stride,  // stride
            vertexInputRate,    // inputRate
        };
    }

    for (size_t idx = 0; idx < vertexInputAttributeDescriptions.size(); ++idx) {
        const auto& attributeRef = vertexInputDeclaration.attributeDescriptions[idx];
        const VkFormat vertexInputFormat = (VkFormat)attributeRef.format;
        vertexInputAttributeDescriptions[idx] = {
            attributeRef.location, // location
            attributeRef.binding,  // binding
            vertexInputFormat,     // format
            attributeRef.offset,   // offset
        };
    }
}

struct DescriptorSetFillData {
    uint32_t descriptorSetCount { 0 };
    uint32_t pushConstantRangeCount { 0u };
    VkPushConstantRange pushConstantRanges[PipelineLayoutConstants::MAX_PUSH_CONSTANT_RANGE_COUNT];
    VkDescriptorSetLayout descriptorSetLayouts[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT] { VK_NULL_HANDLE,
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    // the layout can be coming for special descriptor sets (with e.g. platform formats and immutable samplers)
    bool descriptorSetLayoutOwnership[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT] { true, true, true, true };
};

void GetDescriptorSetFillData(const PipelineLayout& pipelineLayout,
    const LowLevelPipelineLayoutData& pipelineLayoutData, const VkDevice device,
    const VkShaderStageFlags neededShaderStageFlags, DescriptorSetFillData& ds)
{
    // NOTE: support for only one push constant
    ds.pushConstantRangeCount = (pipelineLayout.pushConstant.byteSize > 0) ? 1u : 0u;
    const LowLevelPipelineLayoutDataVk& pipelineLayoutDataVk =
        static_cast<const LowLevelPipelineLayoutDataVk&>(pipelineLayoutData);
    // uses the same temp array for all bindings in all sets
    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT];
    for (uint32_t operationIdx = 0; operationIdx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++operationIdx) {
        const auto& descRef = pipelineLayout.descriptorSetLayouts[operationIdx];
        if ((ds.descriptorSetCount >= pipelineLayout.descriptorSetCount) &&
            (descRef.set == PipelineLayoutConstants::INVALID_INDEX)) {
            continue;
        }
        ds.descriptorSetCount++;
        const uint32_t setIdx = operationIdx;
        const auto& descSetLayoutData = pipelineLayoutDataVk.descriptorSetLayouts[setIdx];
        // NOTE: we are currently only doing handling of special (immutable sampler needing) layouts
        // with the descriptor set layout coming from the real descriptor set
        if (descSetLayoutData.flags & LowLevelDescriptorSetVk::DESCRIPTOR_SET_LAYOUT_IMMUTABLE_SAMPLER_BIT) {
            PLUGIN_ASSERT(descSetLayoutData.descriptorSetLayout);
            ds.descriptorSetLayouts[setIdx] = descSetLayoutData.descriptorSetLayout;
            ds.descriptorSetLayoutOwnership[setIdx] = false; // not owned, cannot be destroyed
        } else {
            constexpr VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateFlags { 0 };
            if (descRef.set == PipelineLayoutConstants::INVALID_INDEX) {
                // provide empty layout for empty set
                VALIDATE_VK_RESULT(vkCreateDescriptorSetLayout(device, // device
                    &EMPTY_LAYOUT_INFO,                                // pCreateInfo
                    nullptr,                                           // pAllocator
                    &ds.descriptorSetLayouts[setIdx]));                // pSetLayout
            } else {
                const uint32_t bindingCount = static_cast<uint32_t>(descRef.bindings.size());
                PLUGIN_ASSERT(bindingCount <= PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT);
                for (uint32_t bindingOpIdx = 0; bindingOpIdx < bindingCount; ++bindingOpIdx) {
                    const auto& bindingRef = descRef.bindings[bindingOpIdx];
                    const VkShaderStageFlags shaderStageFlags = (VkShaderStageFlags)bindingRef.shaderStageFlags;
                    const uint32_t bindingIdx = bindingRef.binding;
                    const VkDescriptorType descriptorType = (VkDescriptorType)bindingRef.descriptorType;
                    const uint32_t descriptorCount = bindingRef.descriptorCount;

                    PLUGIN_ASSERT((shaderStageFlags & neededShaderStageFlags) > 0);
                    descriptorSetLayoutBindings[bindingOpIdx] = {
                        bindingIdx,       // binding
                        descriptorType,   // descriptorType
                        descriptorCount,  // descriptorCount
                        shaderStageFlags, // stageFlags
                        nullptr,          // pImmutableSamplers
                    };
                }

                const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
                    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, // sType
                    nullptr,                                             // pNext
                    descriptorSetLayoutCreateFlags,                      // flags
                    bindingCount,                                        // bindingCount
                    descriptorSetLayoutBindings,                         // pBindings
                };

                VALIDATE_VK_RESULT(vkCreateDescriptorSetLayout(device, // device
                    &descriptorSetLayoutCreateInfo,                    // pCreateInfo
                    nullptr,                                           // pAllocator
                    &ds.descriptorSetLayouts[setIdx]));                // pSetLayout
            }
        }
    }

    if (ds.pushConstantRangeCount == 1) {
        const VkShaderStageFlags shaderStageFlags = (VkShaderStageFlags)pipelineLayout.pushConstant.shaderStageFlags;
        PLUGIN_ASSERT((shaderStageFlags & neededShaderStageFlags) > 0);
        const uint32_t bytesize = pipelineLayout.pushConstant.byteSize;
        ds.pushConstantRanges[0] = {
            shaderStageFlags, // stageFlags
            0,                // offset
            bytesize,         // size
        };
    }
}
} // namespace

GraphicsPipelineStateObjectVk::GraphicsPipelineStateObjectVk(Device& device, const GpuShaderProgram& gpuShaderProgram,
    const GraphicsState& graphicsState, const PipelineLayout& pipelineLayout,
    const VertexInputDeclarationView& vertexInputDeclaration,
    const ShaderSpecializationConstantDataView& specializationConstants,
    const array_view<const DynamicStateEnum> dynamicStates, const RenderPassDesc& renderPassDesc,
    const array_view<const RenderPassSubpassDesc>& renderPassSubpassDescs, const uint32_t subpassIndex,
    const LowLevelRenderPassData& renderPassData, const LowLevelPipelineLayoutData& pipelineLayoutData)
    : GraphicsPipelineStateObject(), device_(device)
{
    PLUGIN_ASSERT(!renderPassSubpassDescs.empty());

    const LowLevelRenderPassDataVk& lowLevelRenderPassDataVk = (const LowLevelRenderPassDataVk&)renderPassData;

    const DeviceVk& deviceVk = (const DeviceVk&)device_;
    const DevicePlatformDataVk& devicePlatVk = (const DevicePlatformDataVk&)deviceVk.GetPlatformData();
    const VkDevice vkDevice = devicePlatVk.device;

    const GpuShaderProgramVk& program = static_cast<const GpuShaderProgramVk&>(gpuShaderProgram);
    const GpuShaderProgramPlatformDataVk& platData = program.GetPlatformData();

    vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;
    vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
    GetVertexInputs(vertexInputDeclaration, vertexInputBindingDescriptions, vertexInputAttributeDescriptions);

    constexpr VkPipelineVertexInputStateCreateFlags pipelineVertexInputStateCreateFlags { 0 };
    const VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
        nullptr,                                                   // pNext
        pipelineVertexInputStateCreateFlags,                       // flags
        (uint32_t)vertexInputBindingDescriptions.size(),           // vertexBindingDescriptionCount
        vertexInputBindingDescriptions.data(),                     // pVertexBindingDescriptions
        (uint32_t)vertexInputAttributeDescriptions.size(),         // vertexAttributeDescriptionCount
        vertexInputAttributeDescriptions.data(),                   // pVertexAttributeDescriptions
    };

    const GraphicsState::InputAssembly& inputAssembly = graphicsState.inputAssembly;

    constexpr VkPipelineInputAssemblyStateCreateFlags pipelineInputAssemblyStateCreateFlags { 0 };
    const VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, // sType
        nullptr,                                                     // pNext
        pipelineInputAssemblyStateCreateFlags,                       // flags
        (VkPrimitiveTopology)inputAssembly.primitiveTopology,        // topology
        (VkBool32)inputAssembly.enablePrimitiveRestart,              // primitiveRestartEnable
    };

    const GraphicsState::RasterizationState& rasterizationState = graphicsState.rasterizationState;

    constexpr VkPipelineRasterizationStateCreateFlags pipelineRasterizationStateCreateFlags { 0 };
    const VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, // sType
        nullptr,                                                    // Next
        pipelineRasterizationStateCreateFlags,                      // flags
        (VkBool32)rasterizationState.enableDepthClamp,              // depthClampEnable
        (VkBool32)rasterizationState.enableRasterizerDiscard,       // rasterizerDiscardEnable
        (VkPolygonMode)rasterizationState.polygonMode,              // polygonMode
        (VkCullModeFlags)rasterizationState.cullModeFlags,          // cullMode
        (VkFrontFace)rasterizationState.frontFace,                  // frontFace
        (VkBool32)rasterizationState.enableDepthBias,               // depthBiasEnable
        rasterizationState.depthBiasConstantFactor,                 // depthBiasConstantFactor
        rasterizationState.depthBiasClamp,                          // depthBiasClamp
        rasterizationState.depthBiasSlopeFactor,                    // depthBiasSlopeFactor
        rasterizationState.lineWidth,                               // lineWidth
    };

    const GraphicsState::DepthStencilState& depthStencilState = graphicsState.depthStencilState;

    const GraphicsState::StencilOpState& frontStencilOpState = depthStencilState.frontStencilOpState;
    const VkStencilOpState frontStencilOpStateVk {
        (VkStencilOp)frontStencilOpState.failOp,      // failOp
        (VkStencilOp)frontStencilOpState.passOp,      // passOp
        (VkStencilOp)frontStencilOpState.depthFailOp, // depthFailOp
        (VkCompareOp)frontStencilOpState.compareOp,   // compareOp
        frontStencilOpState.compareMask,              // compareMask
        frontStencilOpState.writeMask,                // writeMask
        frontStencilOpState.reference,                // reference
    };
    const GraphicsState::StencilOpState& backStencilOpState = depthStencilState.backStencilOpState;
    const VkStencilOpState backStencilOpStateVk {
        (VkStencilOp)backStencilOpState.failOp,      // failOp
        (VkStencilOp)backStencilOpState.passOp,      // passOp
        (VkStencilOp)backStencilOpState.depthFailOp, // depthFailOp
        (VkCompareOp)backStencilOpState.compareOp,   // compareOp
        backStencilOpState.compareMask,              // compareMask
        backStencilOpState.writeMask,                // writeMask
        backStencilOpState.reference,                // reference
    };

    constexpr VkPipelineDepthStencilStateCreateFlags pipelineDepthStencilStateCreateFlags { 0 };
    const VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo {
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, // sType
        nullptr,                                                    // pNext
        pipelineDepthStencilStateCreateFlags,                       // flags
        (VkBool32)depthStencilState.enableDepthTest,                // depthTestEnable
        (VkBool32)depthStencilState.enableDepthWrite,               // depthWriteEnable
        (VkCompareOp)depthStencilState.depthCompareOp,              // depthCompareOp
        (VkBool32)depthStencilState.enableDepthBoundsTest,          // depthBoundsTestEnable
        (VkBool32)depthStencilState.enableStencilTest,              // stencilTestEnable
        frontStencilOpStateVk,                                      // front
        backStencilOpStateVk,                                       // back
        depthStencilState.minDepthBounds,                           // minDepthBounds
        depthStencilState.maxDepthBounds,                           // maxDepthBounds
    };

    const GraphicsState::ColorBlendState& colorBlendState = graphicsState.colorBlendState;

    VkPipelineColorBlendAttachmentState
        pipelineColorBlendAttachmentStates[PipelineStateConstants::MAX_COLOR_ATTACHMENT_COUNT];
    const uint32_t colAttachmentCount = renderPassSubpassDescs[subpassIndex].colorAttachmentCount;
    PLUGIN_ASSERT(colAttachmentCount <= PipelineStateConstants::MAX_COLOR_ATTACHMENT_COUNT);
    for (size_t idx = 0; idx < colAttachmentCount; ++idx) {
        const GraphicsState::ColorBlendState::Attachment& attachmentBlendState = colorBlendState.colorAttachments[idx];

        pipelineColorBlendAttachmentStates[idx] = {
            (VkBool32)attachmentBlendState.enableBlend,                 // blendEnable
            (VkBlendFactor)attachmentBlendState.srcColorBlendFactor,    // srcColorBlendFactor
            (VkBlendFactor)attachmentBlendState.dstColorBlendFactor,    // dstColorBlendFactor
            (VkBlendOp)attachmentBlendState.colorBlendOp,               // colorBlendOp
            (VkBlendFactor)attachmentBlendState.srcAlphaBlendFactor,    // srcAlphaBlendFactor
            (VkBlendFactor)attachmentBlendState.dstAlphaBlendFactor,    // dstAlphaBlendFactor
            (VkBlendOp)attachmentBlendState.alphaBlendOp,               // alphaBlendOp
            (VkColorComponentFlags)attachmentBlendState.colorWriteMask, // colorWriteMask
        };
    }

    constexpr VkPipelineColorBlendStateCreateFlags pipelineColorBlendStateCreateFlags { 0 };
    const float* bc = colorBlendState.colorBlendConstants;
    const VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, // sType
        nullptr,                                                  // pNext
        pipelineColorBlendStateCreateFlags,                       // flags
        (VkBool32)colorBlendState.enableLogicOp,                  // logicOpEnable
        (VkLogicOp)colorBlendState.logicOp,                       // logicOp
        colAttachmentCount,                                       // attachmentCount
        pipelineColorBlendAttachmentStates,                       // pAttachments
        { bc[0u], bc[1u], bc[2u], bc[3u] },                       // blendConstants[4]
    };

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo {};
    pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

    VkDynamicState vkDynamicStates[MAX_DYNAMIC_STATE_COUNT];
    uint32_t dynamicStateCount = Math::min(MAX_DYNAMIC_STATE_COUNT, static_cast<uint32_t>(dynamicStates.size()));
    if (dynamicStateCount > 0) {
        for (uint32_t idx = 0; idx < dynamicStateCount; ++idx) {
            vkDynamicStates[idx] = (VkDynamicState)dynamicStates[idx];
        }

        constexpr VkPipelineDynamicStateCreateFlags pipelineDynamicStateCreateFlags { 0 };
        pipelineDynamicStateCreateInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, // sType
            nullptr,                                              // pNext
            pipelineDynamicStateCreateFlags,                      // flags
            dynamicStateCount,                                    // dynamicStateCount
            vkDynamicStates,                                      // pDynamicStates
        };
    }

    constexpr uint32_t maxViewportCount { 1 };
    constexpr uint32_t maxScissorCount { 1 };
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo {
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, // sType
        nullptr,                                               // pNext
        0,                                                     // flags
        maxViewportCount,                                      // viewportCount
        &lowLevelRenderPassDataVk.viewport,                    // pViewports
        maxScissorCount,                                       // scissorCount
        &lowLevelRenderPassDataVk.scissor,                     // pScissors
    };

    // reserve max
    vector<VkSpecializationMapEntry> vertexStageSpecializations;
    vector<VkSpecializationMapEntry> fragmentStageSpecializations;
    vertexStageSpecializations.reserve(specializationConstants.constants.size());
    fragmentStageSpecializations.reserve(specializationConstants.constants.size());

    uint32_t vertexDataSize = 0;
    uint32_t fragmentDataSize = 0;

    for (auto const& constant : specializationConstants.constants) {
        const auto constantSize = GpuProgramUtil::SpecializationByteSize(constant.type);
        const VkSpecializationMapEntry entry {
            static_cast<uint32_t>(constant.id), // constantID
            constant.offset,                    // offset
            constantSize                        // entry.size
        };
        if (constant.shaderStage & CORE_SHADER_STAGE_VERTEX_BIT) {
            vertexStageSpecializations.push_back(entry);
            vertexDataSize = std::max(vertexDataSize, constant.offset + constantSize);
        }
        if (constant.shaderStage & CORE_SHADER_STAGE_FRAGMENT_BIT) {
            fragmentStageSpecializations.push_back(entry);
            fragmentDataSize = std::max(fragmentDataSize, constant.offset + constantSize);
        }
    }

    const VkSpecializationInfo vertexSpecializationInfo {
        static_cast<uint32_t>(vertexStageSpecializations.size()), // mapEntryCount
        vertexStageSpecializations.data(),                        // pMapEntries
        vertexDataSize,                                           // dataSize
        specializationConstants.data.data()                       // pData
    };

    const VkSpecializationInfo fragmentSpecializationInfo {
        static_cast<uint32_t>(fragmentStageSpecializations.size()), // mapEntryCount
        fragmentStageSpecializations.data(),                        // pMapEntries
        fragmentDataSize,                                           // dataSize
        specializationConstants.data.data()                         // pData
    };

    constexpr uint32_t stageCount { 2 };
    const VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[stageCount] {
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sTypeoldHandle
            nullptr,                                             // pNextoldHandle
            0,                                                   // flags
            VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT,   // stage
            platData.vert,                                       // module
            "main",                                              // pName
            &vertexSpecializationInfo,                           // pSpecializationInfo
        },
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
            nullptr,                                             // pNext
            0,                                                   // flags
            VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, // stage
            platData.frag,                                       // module
            "main",                                              // pName
            &fragmentSpecializationInfo                          // pSpecializationInfo
        },
    };

    // NOTE: support for only one push constant
    DescriptorSetFillData ds;
    GetDescriptorSetFillData(pipelineLayout, pipelineLayoutData, vkDevice,
        VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, ds);

    const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, // sType
        nullptr,                                       // pNext
        0,                                             // flags
        ds.descriptorSetCount,                         // setLayoutCount
        ds.descriptorSetLayouts,                       // pSetLayouts
        ds.pushConstantRangeCount,                     // pushConstantRangeCount
        ds.pushConstantRanges,                         // pPushConstantRanges
    };

    VALIDATE_VK_RESULT(vkCreatePipelineLayout(vkDevice, // device
        &pipelineLayoutCreateInfo,                      // pCreateInfo,
        nullptr,                                        // pAllocator
        &plat_.pipelineLayout));                        // pPipelineLayout

    constexpr VkPipelineMultisampleStateCreateFlags pipelineMultisampleStateCreateFlags { 0 };

    VkSampleCountFlagBits sampleCountFlagBits { VK_SAMPLE_COUNT_1_BIT };
    if (renderPassSubpassDescs[subpassIndex].colorAttachmentCount > 0) {
        const auto& ref = lowLevelRenderPassDataVk.renderPassCompatibilityDesc.attachments[0];
        sampleCountFlagBits = (ref.sampleCountFlags == 0) ? VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT
                                                          : (VkSampleCountFlagBits)ref.sampleCountFlags;
    }

    VkBool32 sampleShadingEnable = VK_FALSE;
    float minSampleShading = 0.0f;

    const bool msaaEnabled =
        (VkBool32)((sampleCountFlagBits != VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT) &&
                   (sampleCountFlagBits != VkSampleCountFlagBits::VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM))
            ? true
            : false;
    if (msaaEnabled) {
        if (devicePlatVk.enabledPhysicalDeviceFeatures.sampleRateShading) {
            sampleShadingEnable = VK_TRUE;
            minSampleShading = deviceVk.GetFeatureConfigurations().minSampleShading;
        }
    }

    // NOTE: alpha to coverage
    constexpr VkBool32 alphaToCoverageEnable { false };
    constexpr VkBool32 alphaToOneEnable { false };

    const VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, // sType
        nullptr,                                                  // pNext
        pipelineMultisampleStateCreateFlags,                      // flags
        sampleCountFlagBits,                                      // rasterizationSamples
        sampleShadingEnable,                                      // sampleShadingEnable
        minSampleShading,                                         // minSampleShading
        nullptr,                                                  // pSampleMask
        alphaToCoverageEnable,                                    // alphaToCoverageEnable
        alphaToOneEnable,                                         // alphaToOneEnable
    };

    // needs nullptr if no dynamic states
    VkPipelineDynamicStateCreateInfo* ptrPipelineDynamicStateCreateInfo = nullptr;
    if (dynamicStateCount > 0) {
        ptrPipelineDynamicStateCreateInfo = &pipelineDynamicStateCreateInfo;
    }

    constexpr VkPipelineCreateFlags pipelineCreateFlags { 0 };
    const VkRenderPass renderPass = lowLevelRenderPassDataVk.renderPassCompatibility;
    const VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, // sType
        nullptr,                                         // pNext
        pipelineCreateFlags,                             // flags
        stageCount,                                      // stageCount
        pipelineShaderStageCreateInfos,                  // pStages
        &pipelineVertexInputStateCreateInfo,             // pVertexInputState
        &pipelineInputAssemblyStateCreateInfo,           // pInputAssemblyState
        nullptr,                                         // pTessellationState
        &viewportStateCreateInfo,                        // pViewportState
        &pipelineRasterizationStateCreateInfo,           // pRasterizationState
        &pipelineMultisampleStateCreateInfo,             // pMultisampleState
        &pipelineDepthStencilStateCreateInfo,            // pDepthStencilState
        &pipelineColorBlendStateCreateInfo,              // pColorBlendState
        ptrPipelineDynamicStateCreateInfo,               // pDynamicState
        plat_.pipelineLayout,                            // layout
        renderPass,                                      // renderPass
        subpassIndex,                                    // subpass
        VK_NULL_HANDLE,                                  // basePipelineHandle
        0,                                               // basePipelineIndex
    };

    VALIDATE_VK_RESULT(vkCreateGraphicsPipelines(vkDevice, // device
        devicePlatVk.pipelineCache,                        // pipelineCache
        1,                                                 // createInfoCount
        &graphicsPipelineCreateInfo,                       // pCreateInfos
        nullptr,                                           // pAllocator
        &plat_.pipeline));                                 // pPipelines

    // NOTE: direct destruction here
    for (uint32_t idx = 0; idx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++idx) {
        const auto& descRef = ds.descriptorSetLayouts[idx];
        if (descRef && ds.descriptorSetLayoutOwnership[idx]) {
            vkDestroyDescriptorSetLayout(vkDevice, // device
                descRef,                           // descriptorSetLayout
                nullptr);                          // pAllocator
        }
    }
}

GraphicsPipelineStateObjectVk::~GraphicsPipelineStateObjectVk()
{
    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
    if (plat_.pipeline) {
        vkDestroyPipeline(device, // device
            plat_.pipeline,       // pipeline
            nullptr);             // pAllocator
    }
    if (plat_.pipelineLayout) {
        vkDestroyPipelineLayout(device, // device
            plat_.pipelineLayout,       // pipelineLayout
            nullptr);                   // pAllocator
    }
}

const PipelineStateObjectPlatformDataVk& GraphicsPipelineStateObjectVk::GetPlatformData() const
{
    return plat_;
}

ComputePipelineStateObjectVk::ComputePipelineStateObjectVk(Device& device, const GpuComputeProgram& gpuComputeProgram,
    const PipelineLayout& pipelineLayout, const ShaderSpecializationConstantDataView& specializationConstants,
    const LowLevelPipelineLayoutData& pipelineLayoutData)
    : ComputePipelineStateObject(), device_(device)
{
    const DeviceVk& deviceVk = (const DeviceVk&)device_;
    const DevicePlatformDataVk& devicePlatVk = (const DevicePlatformDataVk&)deviceVk.GetPlatformData();
    const VkDevice vkDevice = devicePlatVk.device;

    const GpuComputeProgramVk& program = static_cast<const GpuComputeProgramVk&>(gpuComputeProgram);
    const auto& platData = program.GetPlatformData();
    const VkShaderModule shaderModule = platData.comp;

    vector<VkSpecializationMapEntry> computeStateSpecializations;
    computeStateSpecializations.reserve(specializationConstants.constants.size());
    uint32_t computeDataSize = 0;
    for (auto const& constant : specializationConstants.constants) {
        const auto constantSize = GpuProgramUtil::SpecializationByteSize(constant.type);
        const VkSpecializationMapEntry entry {
            static_cast<uint32_t>(constant.id), // constantID
            constant.offset,                    // offset
            constantSize                        // entry.size
        };
        if (constant.shaderStage & CORE_SHADER_STAGE_COMPUTE_BIT) {
            computeStateSpecializations.push_back(entry);
            computeDataSize = std::max(computeDataSize, constant.offset + constantSize);
        }
    }

    const VkSpecializationInfo computeSpecializationInfo {
        static_cast<uint32_t>(computeStateSpecializations.size()), // mapEntryCount
        computeStateSpecializations.data(),                        // pMapEntries
        computeDataSize,                                           // dataSize
        specializationConstants.data.data()                        // pData
    };

    const VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
        nullptr,                                             // pNext
        0,                                                   // flags
        VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT,  // stage
        shaderModule,                                        // module
        "main",                                              // pName
        &computeSpecializationInfo,                          // pSpecializationInfo
    };

    // NOTE: support for only one push constant
    DescriptorSetFillData ds;
    GetDescriptorSetFillData(
        pipelineLayout, pipelineLayoutData, vkDevice, VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT, ds);

    const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, // sType
        nullptr,                                       // pNext
        0,                                             // flags
        ds.descriptorSetCount,                         // setLayoutCount
        ds.descriptorSetLayouts,                       // pSetLayouts
        ds.pushConstantRangeCount,                     // pushConstantRangeCount
        ds.pushConstantRanges,                         // pPushConstantRanges
    };

    VALIDATE_VK_RESULT(vkCreatePipelineLayout(vkDevice, // device
        &pipelineLayoutCreateInfo,                      // pCreateInfo,
        nullptr,                                        // pAllocator
        &plat_.pipelineLayout));                        // pPipelineLayout

    constexpr VkPipelineCreateFlags pipelineCreateFlags { 0 };
    const VkComputePipelineCreateInfo computePipelineCreateInfo {
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, // sType
        nullptr,                                        // pNext
        pipelineCreateFlags,                            // flags
        pipelineShaderStageCreateInfo,                  // stage
        plat_.pipelineLayout,                           // layout
        VK_NULL_HANDLE,                                 // basePipelineHandle
        0,                                              // basePipelineIndex
    };

    VALIDATE_VK_RESULT(vkCreateComputePipelines(vkDevice, // device
        devicePlatVk.pipelineCache,                       // pipelineCache
        1,                                                // createInfoCount
        &computePipelineCreateInfo,                       // pCreateInfos
        nullptr,                                          // pAllocator
        &plat_.pipeline));                                // pPipelines

    // NOTE: direct destruction here
    for (uint32_t idx = 0; idx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++idx) {
        const auto& descRef = ds.descriptorSetLayouts[idx];
        if (descRef && ds.descriptorSetLayoutOwnership[idx]) {
            vkDestroyDescriptorSetLayout(vkDevice, // device
                descRef,                           // descriptorSetLayout
                nullptr);                          // pAllocator
        }
    }
}

ComputePipelineStateObjectVk::~ComputePipelineStateObjectVk()
{
    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
    vkDestroyPipeline(device,       // device
        plat_.pipeline,             // pipeline
        nullptr);                   // pAllocator
    vkDestroyPipelineLayout(device, // device
        plat_.pipelineLayout,       // pipelineLayout
        nullptr);                   // pAllocator
}

const PipelineStateObjectPlatformDataVk& ComputePipelineStateObjectVk::GetPlatformData() const
{
    return plat_;
}
RENDER_END_NAMESPACE()
