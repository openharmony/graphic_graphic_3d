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

#include "pipeline_create_functions_vk.h"

#include <algorithm>
#include <vulkan/vulkan_core.h>

#include <base/containers/array_view.h>
#include <base/containers/vector.h>
#include <base/math/mathf.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

#include "nodecontext/render_command_list.h"
#include "util/log.h"
#include "vulkan/device_vk.h"
#include "vulkan/validate_vk.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
inline constexpr Size2D LocalClamp(const Size2D val, const Size2D minVal, const Size2D maxVal)
{
    return Size2D { Math::max(minVal.width, Math::min(val.width, maxVal.width)),
        Math::max(minVal.height, Math::min(val.height, maxVal.height)) };
}

inline Size2D ClampShadingRateAttachmentTexelSize(const DeviceVk& deviceVk, const Size2D val)
{
    const FragmentShadingRateProperties& fsrp = deviceVk.GetCommonDeviceProperties().fragmentShadingRateProperties;
    return LocalClamp(
        val, fsrp.minFragmentShadingRateAttachmentTexelSize, fsrp.maxFragmentShadingRateAttachmentTexelSize);
}

void CreateAttachmentDescriptions(const array_view<const RenderPassDesc::AttachmentDesc> attachments,
    const array_view<const LowLevelRenderPassCompatibilityDescVk::Attachment> compatibilityAttachmentDescs,
    const array_view<const ImageLayout> initialImageLayouts, const array_view<const ImageLayout> finalImageLayouts,
    array_view<VkAttachmentDescription> newAttachments)
{
    PLUGIN_ASSERT(attachments.size() == compatibilityAttachmentDescs.size());
    PLUGIN_ASSERT(attachments.size() <= newAttachments.size());
    PLUGIN_ASSERT(attachments.size() <= initialImageLayouts.size());
    PLUGIN_ASSERT(attachments.size() <= finalImageLayouts.size());
    auto itNewAttachments = newAttachments.begin();
    auto itInitialImageLayouts = initialImageLayouts.begin();
    auto itFinalImageLayouts = finalImageLayouts.begin();
    for (size_t idx = 0; idx < attachments.size(); ++idx) {
        const auto& attachmentRef = attachments[idx];
        const auto& compatibilityAttachmentRef = compatibilityAttachmentDescs[idx];
        const ImageLayout initialLayout = *itInitialImageLayouts++;
        const ImageLayout finalLayout = *itFinalImageLayouts++;
        PLUGIN_ASSERT(compatibilityAttachmentRef.format != VkFormat::VK_FORMAT_UNDEFINED);

        constexpr VkAttachmentDescriptionFlags attachmentDescriptionFlags { 0 };
        *itNewAttachments++ = {
            attachmentDescriptionFlags,                        // flags
            compatibilityAttachmentRef.format,                 // format
            compatibilityAttachmentRef.sampleCountFlags,       // samples
            (VkAttachmentLoadOp)attachmentRef.loadOp,          // loadOp
            (VkAttachmentStoreOp)attachmentRef.storeOp,        // storeOp
            (VkAttachmentLoadOp)attachmentRef.stencilLoadOp,   // stencilLoadOp
            (VkAttachmentStoreOp)attachmentRef.stencilStoreOp, // stencilStoreOp
            (VkImageLayout)initialLayout,                      // initialLayout
            (VkImageLayout)finalLayout,                        // finalLayout
        };
    }
}

void CreateAttachmentDescriptions2(const array_view<const RenderPassDesc::AttachmentDesc> attachments,
    const array_view<const LowLevelRenderPassCompatibilityDescVk::Attachment> compatibilityAttachmentDescs,
    const array_view<const ImageLayout> initialImageLayouts, const array_view<const ImageLayout> finalImageLayouts,
    array_view<VkAttachmentDescription2KHR> newAttachments)
{
    PLUGIN_ASSERT(attachments.size() == compatibilityAttachmentDescs.size());
    PLUGIN_ASSERT(attachments.size() <= newAttachments.size());
    PLUGIN_ASSERT(attachments.size() <= initialImageLayouts.size());
    PLUGIN_ASSERT(attachments.size() <= finalImageLayouts.size());
    auto itNewAttachments = newAttachments.begin();
    auto itInitialImageLayouts = initialImageLayouts.begin();
    auto itFinalImageLayouts = finalImageLayouts.begin();
    for (size_t idx = 0; idx < attachments.size(); ++idx) {
        const auto& attachmentRef = attachments[idx];
        const auto& compatibilityAttachmentRef = compatibilityAttachmentDescs[idx];
        const ImageLayout initialLayout = *itInitialImageLayouts++;
        const ImageLayout finalLayout = *itFinalImageLayouts++;
#if (RENDER_VALIDATION_ENABLED == 1)
        if (compatibilityAttachmentRef.format == VkFormat::VK_FORMAT_UNDEFINED) {
            PLUGIN_LOG_E("RENDER_VALIDATION: VK_FORMAT_UNDEFINED with PSO attachment descriptions");
        }
#endif

        constexpr VkAttachmentDescriptionFlags attachmentDescriptionFlags { 0 };
        *itNewAttachments++ = {
            VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR,    // sType
            nullptr,                                           // pNext
            attachmentDescriptionFlags,                        // flags
            compatibilityAttachmentRef.format,                 // format
            compatibilityAttachmentRef.sampleCountFlags,       // samples
            (VkAttachmentLoadOp)attachmentRef.loadOp,          // loadOp
            (VkAttachmentStoreOp)attachmentRef.storeOp,        // storeOp
            (VkAttachmentLoadOp)attachmentRef.stencilLoadOp,   // stencilLoadOp
            (VkAttachmentStoreOp)attachmentRef.stencilStoreOp, // stencilStoreOp
            (VkImageLayout)initialLayout,                      // initialLayout
            (VkImageLayout)finalLayout,                        // finalLayout
        };
    }
}

void CreateAttachmentDescriptionsCompatibility(const array_view<const RenderPassDesc::AttachmentDesc> attachments,
    const array_view<const LowLevelRenderPassCompatibilityDescVk::Attachment> compatibilityAttachmentDescs,
    array_view<VkAttachmentDescription> newAttachments)
{
    PLUGIN_ASSERT(attachments.size() == compatibilityAttachmentDescs.size());
    PLUGIN_ASSERT(attachments.size() <= newAttachments.size());
    auto itNewAttachments = newAttachments.begin();
    for (size_t idx = 0; idx < attachments.size(); ++idx) {
        const auto& compatibilityAttachmentRef = compatibilityAttachmentDescs[idx];
        PLUGIN_ASSERT(compatibilityAttachmentRef.format != VkFormat::VK_FORMAT_UNDEFINED);

        constexpr VkAttachmentDescriptionFlags attachmentDescriptionFlags { 0 };
        *itNewAttachments++ = {
            attachmentDescriptionFlags,                            // flags
            compatibilityAttachmentRef.format,                     // format
            compatibilityAttachmentRef.sampleCountFlags,           // samples
            VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // loadOp
            VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE, // storeOp
            VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stencilLoadOp
            VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE, // stencilStoreOp
            VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,              // initialLayout
            VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,                // finalLayout
        };
    }
}

void CreateAttachmentDescriptionsCompatibility2(const array_view<const RenderPassDesc::AttachmentDesc> attachments,
    const array_view<const LowLevelRenderPassCompatibilityDescVk::Attachment> compatibilityAttachmentDescs,
    array_view<VkAttachmentDescription2KHR> newAttachments)
{
    PLUGIN_ASSERT(attachments.size() == compatibilityAttachmentDescs.size());
    PLUGIN_ASSERT(attachments.size() <= newAttachments.size());
    auto itNewAttachments = newAttachments.begin();
    for (size_t idx = 0; idx < attachments.size(); ++idx) {
        const auto& compatibilityAttachmentRef = compatibilityAttachmentDescs[idx];
#if (RENDER_VALIDATION_ENABLED == 1)
        if (compatibilityAttachmentRef.format == VkFormat::VK_FORMAT_UNDEFINED) {
            PLUGIN_LOG_E("RENDER_VALIDATION: VK_FORMAT_UNDEFINED with PSO attachment descriptions");
        }
#endif
        constexpr VkAttachmentDescriptionFlags attachmentDescriptionFlags { 0 };
        *itNewAttachments++ = {
            VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR,        // sType
            nullptr,                                               // pNext
            attachmentDescriptionFlags,                            // flags
            compatibilityAttachmentRef.format,                     // format
            compatibilityAttachmentRef.sampleCountFlags,           // samples
            VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // loadOp
            VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE, // storeOp
            VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stencilLoadOp
            VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE, // stencilStoreOp
            VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,              // initialLayout
            VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,                // finalLayout
        };
    }
}

void CreateAttachmentReferences(const uint32_t* attachmentIndices,
    const ImageLayout* layouts, // can be null if compatibility
    const uint32_t attachmentCount, const uint32_t attachmentStartIndex, const bool createCompatibility,
    VkAttachmentReference* newAttachments)
{
    PLUGIN_ASSERT(newAttachments);
#if (RENDER_VALIDATION_ENABLED == 1)
    if (createCompatibility) {
        PLUGIN_ASSERT(layouts == nullptr);
    }
#endif
    VkImageLayout imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
    for (uint32_t idx = 0; idx < attachmentCount; ++idx) {
        const uint32_t attachmentIndex = attachmentIndices[idx];
        if (!createCompatibility) {
            imageLayout = (VkImageLayout)layouts[attachmentIndex];
        }
        newAttachments[attachmentStartIndex + idx] = {
            attachmentIndex, // attachment
            imageLayout,     // layout
        };
    }
}

void CreateAttachmentReferences2(const uint32_t* attachmentIndices,
    const ImageLayout* layouts, // null if compatibility
    const array_view<const LowLevelRenderPassCompatibilityDescVk::Attachment> compatibilityAttachmentDescs,
    const uint32_t attachmentCount, const uint32_t attachmentStartIndex, const bool createCompatibility,
    VkAttachmentReference2KHR* newAttachments)
{
    PLUGIN_ASSERT(newAttachments);
#if (RENDER_VALIDATION_ENABLED == 1)
    if (createCompatibility) {
        PLUGIN_ASSERT(layouts == nullptr);
    }
#endif
    VkImageLayout imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
    VkImageAspectFlags imageAspectFlags = 0;
    for (uint32_t idx = 0; idx < attachmentCount; ++idx) {
        const uint32_t attachmentIndex = attachmentIndices[idx];
        imageAspectFlags = compatibilityAttachmentDescs[attachmentIndex].aspectFlags;
        if (!createCompatibility) {
            imageLayout = (VkImageLayout)layouts[attachmentIndex];
        }
        newAttachments[attachmentStartIndex + idx] = {
            VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR, // sType
            nullptr,                                      // pNext
            attachmentIndex,                              // attachment
            imageLayout,                                  // layout
            imageAspectFlags,                             // aspectMask
        };
    }
}

VkRenderPass CreateRenderPassCombined(const DeviceVk& deviceVk, const RenderPassDesc& renderPassDesc,
    const LowLevelRenderPassDataVk& lowLevelRenderPassData, const array_view<const RenderPassSubpassDesc> subpassDescs,
    const RenderPassAttachmentResourceStates* subpassResourceStates,
    const RenderPassAttachmentResourceStates* inputResourceStates,
    const RenderPassImageLayouts* imageLayouts, // null when using compatibility
    const uint32_t maxAttachmentReferenceCountPerSubpass, const bool createRenderPassCompatibility,
    RenderPassCreatorVk::RenderPassStorage1& rps1)
{
    const VkDevice device = (deviceVk.GetPlatformDataVk()).device;
    const uint32_t attachmentCount = renderPassDesc.attachmentCount;
    PLUGIN_ASSERT(attachmentCount <= PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT);

    VkAttachmentDescription attachmentDescriptions[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT];
    {
        // add all attachments to attachmentDescriptions array
        const auto attachments = array_view(renderPassDesc.attachments, renderPassDesc.attachmentCount);
        const auto compatibilityAttachments =
            array_view(lowLevelRenderPassData.renderPassCompatibilityDesc.attachments, renderPassDesc.attachmentCount);
        const auto descriptions = array_view(attachmentDescriptions, countof(attachmentDescriptions));
        if (createRenderPassCompatibility) {
            CreateAttachmentDescriptionsCompatibility(attachments, compatibilityAttachments, descriptions);
        } else {
            PLUGIN_ASSERT(imageLayouts);
            const auto initialLayouts =
                array_view(imageLayouts->attachmentInitialLayouts, renderPassDesc.attachmentCount);
            const auto finalLayouts = array_view(imageLayouts->attachmentFinalLayouts, renderPassDesc.attachmentCount);
            CreateAttachmentDescriptions(
                attachments, compatibilityAttachments, initialLayouts, finalLayouts, descriptions);
        }
    }

    // resize vector for all subpass references
    const uint32_t subpassCount = renderPassDesc.subpassCount;
    auto& subpassDescriptions = rps1.subpassDescriptions;
    auto& subpassDependencies = rps1.subpassDependencies;
    auto& attachmentReferences = rps1.attachmentReferences;
    auto& multiViewMasks = rps1.multiViewMasks;
    subpassDescriptions.resize(subpassCount);
    subpassDependencies.resize(subpassCount);
    attachmentReferences.resize(subpassCount * maxAttachmentReferenceCountPerSubpass);
    multiViewMasks.clear();
    multiViewMasks.resize(subpassCount);

    bool hasMultiView = false;
    uint32_t srcSubpass = VK_SUBPASS_EXTERNAL;
    const RenderPassAttachmentResourceStates* srcResourceStates = inputResourceStates;
    for (uint32_t subpassIdx = 0; subpassIdx < subpassCount; ++subpassIdx) {
        const RenderPassSubpassDesc& subpassDesc = subpassDescs[subpassIdx];

        const uint32_t startReferenceIndex = subpassIdx * maxAttachmentReferenceCountPerSubpass;
        uint32_t referenceIndex = startReferenceIndex;

        VkAttachmentReference* inputAttachments = nullptr;
        const ImageLayout* layouts =
            (createRenderPassCompatibility) ? nullptr : subpassResourceStates[subpassIdx].layouts;
        if (subpassDesc.inputAttachmentCount > 0) {
            inputAttachments = &attachmentReferences[referenceIndex];
            CreateAttachmentReferences(subpassDesc.inputAttachmentIndices, layouts, subpassDesc.inputAttachmentCount,
                referenceIndex, createRenderPassCompatibility, attachmentReferences.data());
            referenceIndex += subpassDesc.inputAttachmentCount;
        }

        VkAttachmentReference* colorAttachments = nullptr;
        if (subpassDesc.colorAttachmentCount > 0) {
            colorAttachments = &attachmentReferences[referenceIndex];
            CreateAttachmentReferences(subpassDesc.colorAttachmentIndices, layouts, subpassDesc.colorAttachmentCount,
                referenceIndex, createRenderPassCompatibility, attachmentReferences.data());
            referenceIndex += subpassDesc.colorAttachmentCount;
        }

        VkAttachmentReference* resolveAttachments = nullptr;
        if (subpassDesc.resolveAttachmentCount > 0) {
            resolveAttachments = &attachmentReferences[referenceIndex];
            CreateAttachmentReferences(subpassDesc.resolveAttachmentIndices, layouts,
                subpassDesc.resolveAttachmentCount, referenceIndex, createRenderPassCompatibility,
                attachmentReferences.data());
            referenceIndex += subpassDesc.resolveAttachmentCount;
        }

        // NOTE: preserve attachments
        VkAttachmentReference* depthAttachment = nullptr;
        if (subpassDesc.depthAttachmentCount > 0) {
            depthAttachment = &attachmentReferences[referenceIndex];
            CreateAttachmentReferences(&subpassDesc.depthAttachmentIndex, layouts, subpassDesc.depthAttachmentCount,
                referenceIndex, createRenderPassCompatibility, attachmentReferences.data());
            referenceIndex += subpassDesc.depthAttachmentCount;
        }

        multiViewMasks[subpassIdx] = subpassDesc.viewMask;
        if (subpassDesc.viewMask > 0u) {
            hasMultiView = true;
        }

        constexpr VkSubpassDescriptionFlags subpassDescriptionFlags { 0 };
        subpassDescriptions[subpassIdx] = {
            subpassDescriptionFlags,                              // flags
            VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
            subpassDesc.inputAttachmentCount,                     // inputAttachmentCount
            inputAttachments,                                     // pInputAttachments
            subpassDesc.colorAttachmentCount,                     // colorAttachmentCount
            colorAttachments,                                     // pColorAttachments
            resolveAttachments,                                   // pResolveAttachments
            depthAttachment,                                      // pDepthStencilAttachment
            0,                                                    // preserveAttachmentCount
            nullptr,                                              // pPreserveAttachments
        };

        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dstStageMask = 0;
        VkAccessFlags srcAccessMask = 0;
        VkAccessFlags dstAccessMask = 0;
        {
            PLUGIN_ASSERT(srcResourceStates);
            const RenderPassAttachmentResourceStates& dstResourceStates = subpassResourceStates[subpassIdx];
            for (uint32_t attachmentIdx = 0; attachmentIdx < attachmentCount; ++attachmentIdx) {
                srcStageMask |= (VkPipelineStageFlagBits)srcResourceStates->states[attachmentIdx].pipelineStageFlags;
                srcAccessMask |= srcResourceStates->states[attachmentIdx].accessFlags;

                dstStageMask |= (VkPipelineStageFlagBits)dstResourceStates.states[attachmentIdx].pipelineStageFlags;
                dstAccessMask |= dstResourceStates.states[attachmentIdx].accessFlags;
            }

            // store for next subpass
            srcResourceStates = &dstResourceStates;
        }

        constexpr VkDependencyFlags dependencyFlags { VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT };
        const uint32_t dstSubpass = subpassIdx;
        subpassDependencies[subpassIdx] = {
            srcSubpass,      // srcSubpass
            subpassIdx,      // dstSubpass
            srcStageMask,    // srcStageMask
            dstStageMask,    // dstStageMask
            srcAccessMask,   // srcAccessMask
            dstAccessMask,   // dstAccessMask
            dependencyFlags, // dependencyFlags
        };
        srcSubpass = dstSubpass;
    }

    VkRenderPassMultiviewCreateInfo* pMultiviewCreateInfo = nullptr;
    VkRenderPassMultiviewCreateInfo multiViewCreateInfo;
    if (hasMultiView && deviceVk.GetCommonDeviceExtensions().multiView) {
        PLUGIN_ASSERT(renderPassDesc.subpassCount == static_cast<uint32_t>(multiViewMasks.size()));
        multiViewCreateInfo = {
            VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO, // sType
            nullptr,                                             // pNext
            renderPassDesc.subpassCount,                         // subpassCount
            multiViewMasks.data(),                               // pViewMasks
            0u,                                                  // dependencyCount
            nullptr,                                             // pViewOffsets
            0u,                                                  // correlationMaskCount
            nullptr,                                             // pCorrelationMasks
        };
        pMultiviewCreateInfo = &multiViewCreateInfo;
    }

    constexpr VkRenderPassCreateFlags renderPassCreateFlags { 0 };
    const VkRenderPassCreateInfo renderPassCreateInfo {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, // sType
        pMultiviewCreateInfo,                      // pNext
        renderPassCreateFlags,                     // flags
        attachmentCount,                           // attachmentCount
        attachmentDescriptions,                    // pAttachments
        (uint32_t)subpassDescriptions.size(),      // subpassCount
        subpassDescriptions.data(),                // pSubpasses
        (uint32_t)subpassDependencies.size(),      // dependencyCount
        subpassDependencies.data(),                // pDependencies
    };

    VkRenderPass renderPass { VK_NULL_HANDLE };
    VALIDATE_VK_RESULT(vkCreateRenderPass(device, // device
        &renderPassCreateInfo,                    // pCreateInfo
        nullptr,                                  // pAllocator
        &renderPass));                            // pRenderPass

    return renderPass;
}

VkRenderPass CreateRenderPassCombined2(const DeviceVk& deviceVk, const RenderPassDesc& renderPassDesc,
    const LowLevelRenderPassDataVk& lowLevelRenderPassData, const array_view<const RenderPassSubpassDesc> subpassDescs,
    const RenderPassAttachmentResourceStates* subpassResourceStates,
    const RenderPassAttachmentResourceStates* inputResourceStates,
    const RenderPassImageLayouts* imageLayouts, // null when using compatibility
    const uint32_t maxAttachmentReferenceCountPerSubpass, const bool createRenderPassCompatibility,
    RenderPassCreatorVk::RenderPassStorage2& rps2)
{
    const VkDevice device = (deviceVk.GetPlatformDataVk()).device;
    const uint32_t attachmentCount = renderPassDesc.attachmentCount;
    PLUGIN_ASSERT(attachmentCount <= PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT);

    VkAttachmentDescription2KHR attachmentDescriptions[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT];
    const auto compatibilityAttachments =
        array_view(lowLevelRenderPassData.renderPassCompatibilityDesc.attachments, renderPassDesc.attachmentCount);
    {
        // add all attachments to attachmentDescriptions array
        const auto attachments = array_view(renderPassDesc.attachments, renderPassDesc.attachmentCount);
        const auto descriptions = array_view(attachmentDescriptions, countof(attachmentDescriptions));
        if (createRenderPassCompatibility) {
            CreateAttachmentDescriptionsCompatibility2(attachments, compatibilityAttachments, descriptions);
        } else {
            PLUGIN_ASSERT(imageLayouts);
            const auto initialLayouts =
                array_view(imageLayouts->attachmentInitialLayouts, renderPassDesc.attachmentCount);
            const auto finalLayouts = array_view(imageLayouts->attachmentFinalLayouts, renderPassDesc.attachmentCount);
            CreateAttachmentDescriptions2(
                attachments, compatibilityAttachments, initialLayouts, finalLayouts, descriptions);
        }
    }

    // resize vector for all subpass references
    const uint32_t subpassCount = renderPassDesc.subpassCount;
    auto& subpassDescriptions = rps2.subpassDescriptions;
    auto& subpassDependencies = rps2.subpassDependencies;
    auto& subpassDescriptionsDepthStencilResolve = rps2.subpassDescriptionsDepthStencilResolve;
    auto& attachmentReferences = rps2.attachmentReferences;
    subpassDescriptions.resize(subpassCount);
    subpassDependencies.resize(subpassCount);
    subpassDescriptionsDepthStencilResolve.resize(subpassCount);
#if (RENDER_VULKAN_FSR_ENABLED == 1)
    auto& fragmentShadingRateAttachmentInfos = rps2.fragmentShadingRateAttachmentInfos;
    fragmentShadingRateAttachmentInfos.resize(subpassCount);
#endif
    attachmentReferences.resize(subpassCount * maxAttachmentReferenceCountPerSubpass);

    uint32_t srcSubpass = VK_SUBPASS_EXTERNAL;
    const RenderPassAttachmentResourceStates* srcResourceStates = inputResourceStates;
    const bool supportsMultiView = deviceVk.GetCommonDeviceExtensions().multiView;
    for (uint32_t subpassIdx = 0; subpassIdx < subpassCount; ++subpassIdx) {
        const RenderPassSubpassDesc& subpassDesc = subpassDescs[subpassIdx];

        const uint32_t startReferenceIndex = subpassIdx * maxAttachmentReferenceCountPerSubpass;
        uint32_t referenceIndex = startReferenceIndex;

        VkAttachmentReference2KHR* inputAttachments = nullptr;
        const ImageLayout* layouts =
            (createRenderPassCompatibility) ? nullptr : subpassResourceStates[subpassIdx].layouts;
        if (subpassDesc.inputAttachmentCount > 0) {
            inputAttachments = &attachmentReferences[referenceIndex];
            CreateAttachmentReferences2(subpassDesc.inputAttachmentIndices, layouts, compatibilityAttachments,
                subpassDesc.inputAttachmentCount, referenceIndex, createRenderPassCompatibility,
                attachmentReferences.data());
            referenceIndex += subpassDesc.inputAttachmentCount;
        }

        VkAttachmentReference2KHR* colorAttachments = nullptr;
        if (subpassDesc.colorAttachmentCount > 0) {
            colorAttachments = &attachmentReferences[referenceIndex];
            CreateAttachmentReferences2(subpassDesc.colorAttachmentIndices, layouts, compatibilityAttachments,
                subpassDesc.colorAttachmentCount, referenceIndex, createRenderPassCompatibility,
                attachmentReferences.data());
            referenceIndex += subpassDesc.colorAttachmentCount;
        }

        VkAttachmentReference2KHR* resolveAttachments = nullptr;
        if (subpassDesc.resolveAttachmentCount > 0) {
            resolveAttachments = &attachmentReferences[referenceIndex];
            CreateAttachmentReferences2(subpassDesc.resolveAttachmentIndices, layouts, compatibilityAttachments,
                subpassDesc.resolveAttachmentCount, referenceIndex, createRenderPassCompatibility,
                attachmentReferences.data());
            referenceIndex += subpassDesc.resolveAttachmentCount;
        }

        // for extensions
        void* pFirstExt = nullptr;
        const void** ppNext = nullptr;
#if (RENDER_VULKAN_FSR_ENABLED == 1)
        if (subpassDesc.fragmentShadingRateAttachmentCount > 0) {
            VkAttachmentReference2KHR* fragmentShadingRateAttachments = &attachmentReferences[referenceIndex];
            CreateAttachmentReferences2(&subpassDesc.fragmentShadingRateAttachmentIndex, layouts,
                compatibilityAttachments, subpassDesc.fragmentShadingRateAttachmentCount, referenceIndex,
                createRenderPassCompatibility, attachmentReferences.data());
            referenceIndex += subpassDesc.fragmentShadingRateAttachmentCount;

            VkFragmentShadingRateAttachmentInfoKHR& fragmentShadingRateAttachmentInfo =
                fragmentShadingRateAttachmentInfos[subpassIdx];
            fragmentShadingRateAttachmentInfo.sType = VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
            fragmentShadingRateAttachmentInfo.pNext = nullptr;
            const Size2D srats = ClampShadingRateAttachmentTexelSize(deviceVk, subpassDesc.shadingRateTexelSize);
            fragmentShadingRateAttachmentInfo.shadingRateAttachmentTexelSize = { srats.width, srats.height };
            fragmentShadingRateAttachmentInfo.pFragmentShadingRateAttachment = fragmentShadingRateAttachments;
            if (!pFirstExt) {
                pFirstExt = &fragmentShadingRateAttachmentInfo;
                ppNext = &(fragmentShadingRateAttachmentInfo.pNext);
            } else {
                *ppNext = &fragmentShadingRateAttachmentInfo;
                ppNext = &(fragmentShadingRateAttachmentInfo.pNext);
            }
        }
#endif

        // NOTE: preserve attachments
        VkAttachmentReference2KHR* depthAttachment = nullptr;
        if (subpassDesc.depthAttachmentCount > 0) {
            depthAttachment = &attachmentReferences[referenceIndex];
            CreateAttachmentReferences2(&subpassDesc.depthAttachmentIndex, layouts, compatibilityAttachments,
                subpassDesc.depthAttachmentCount, referenceIndex, createRenderPassCompatibility,
                attachmentReferences.data());
            referenceIndex += subpassDesc.depthAttachmentCount;
            // cannot resolve mode NONE
            if ((subpassDesc.depthResolveAttachmentCount > 0) &&
                (subpassDesc.depthResolveModeFlagBit || subpassDesc.stencilResolveModeFlagBit)) {
                VkAttachmentReference2KHR* depthResolveAttachment = nullptr;
                depthResolveAttachment = &attachmentReferences[referenceIndex];
                CreateAttachmentReferences2(&subpassDesc.depthResolveAttachmentIndex, layouts, compatibilityAttachments,
                    subpassDesc.depthResolveAttachmentCount, referenceIndex, createRenderPassCompatibility,
                    attachmentReferences.data());
                referenceIndex += subpassDesc.depthResolveAttachmentCount;
                VkSubpassDescriptionDepthStencilResolveKHR& subpassDescriptionDepthStencilResolve =
                    subpassDescriptionsDepthStencilResolve[subpassIdx];
                subpassDescriptionDepthStencilResolve.sType =
                    VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE_KHR;
                subpassDescriptionDepthStencilResolve.pNext = nullptr;
                // NOTE: acceptable values needs to be evaluated from the device
                // independent resolve not yet supported
                const VkResolveModeFlagBitsKHR depthStencilResolveMode =
                    (VkResolveModeFlagBitsKHR)subpassDesc.depthResolveModeFlagBit;
                subpassDescriptionDepthStencilResolve.depthResolveMode = depthStencilResolveMode;
                subpassDescriptionDepthStencilResolve.stencilResolveMode = depthStencilResolveMode;
                subpassDescriptionDepthStencilResolve.pDepthStencilResolveAttachment = depthResolveAttachment;

                if (!pFirstExt) {
                    pFirstExt = &subpassDescriptionDepthStencilResolve;
                    ppNext = &(subpassDescriptionDepthStencilResolve.pNext);
                } else {
                    *ppNext = &subpassDescriptionDepthStencilResolve;
                    ppNext = &(subpassDescriptionDepthStencilResolve.pNext);
                }
            }
        }

        constexpr VkSubpassDescriptionFlags subpassDescriptionFlags { 0 };
        subpassDescriptions[subpassIdx] = {
            VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2_KHR,          // sType
            pFirstExt,                                            // pNext
            subpassDescriptionFlags,                              // flags
            VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
            supportsMultiView ? subpassDesc.viewMask : 0,         // viewMask
            subpassDesc.inputAttachmentCount,                     // inputAttachmentCount
            inputAttachments,                                     // pInputAttachments
            subpassDesc.colorAttachmentCount,                     // colorAttachmentCount
            colorAttachments,                                     // pColorAttachments
            resolveAttachments,                                   // pResolveAttachments
            depthAttachment,                                      // pDepthStencilAttachment
            0,                                                    // preserveAttachmentCount
            nullptr,                                              // pPreserveAttachments
        };

        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dstStageMask = 0;
        VkAccessFlags srcAccessMask = 0;
        VkAccessFlags dstAccessMask = 0;
        {
            PLUGIN_ASSERT(srcResourceStates);
            const RenderPassAttachmentResourceStates& dstResourceStates = subpassResourceStates[subpassIdx];
            for (uint32_t attachmentIdx = 0; attachmentIdx < attachmentCount; ++attachmentIdx) {
                srcStageMask |= srcResourceStates->states[attachmentIdx].pipelineStageFlags;
                srcAccessMask |= srcResourceStates->states[attachmentIdx].accessFlags;

                dstStageMask |= dstResourceStates.states[attachmentIdx].pipelineStageFlags;
                dstAccessMask |= dstResourceStates.states[attachmentIdx].accessFlags;
            }

            // store for next subpass
            srcResourceStates = &dstResourceStates;
        }

        constexpr VkDependencyFlags dependencyFlags { VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT };
        const uint32_t dstSubpass = subpassIdx;
        subpassDependencies[subpassIdx] = {
            VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2_KHR, // sType
            nullptr,                                    // pNext
            srcSubpass,                                 // srcSubpass
            subpassIdx,                                 // dstSubpass
            srcStageMask,                               // srcStageMask
            dstStageMask,                               // dstStageMask
            srcAccessMask,                              // srcAccessMask
            dstAccessMask,                              // dstAccessMask
            dependencyFlags,                            // dependencyFlags
            0,                                          // viewOffset
        };
        srcSubpass = dstSubpass;
    }

    constexpr VkRenderPassCreateFlags renderPassCreateFlags { 0 };
    const VkRenderPassCreateInfo2KHR renderPassCreateInfo {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2_KHR, // sType
        nullptr,                                         // pNext
        renderPassCreateFlags,                           // flags
        attachmentCount,                                 // attachmentCount
        attachmentDescriptions,                          // pAttachments
        (uint32_t)subpassDescriptions.size(),            // subpassCount
        subpassDescriptions.data(),                      // pSubpasses
        (uint32_t)subpassDependencies.size(),            // dependencyCount
        subpassDependencies.data(),                      // pDependencies
        0u,                                              // correlatedViewMaskCount
        nullptr,                                         // pCorrelatedViewMasks
    };

    VkRenderPass renderPass { VK_NULL_HANDLE };
    const DeviceVk::ExtFunctions& extFunctions = deviceVk.GetExtFunctions();
    PLUGIN_ASSERT(extFunctions.vkCreateRenderPass2KHR);            // required here
    VALIDATE_VK_RESULT(extFunctions.vkCreateRenderPass2KHR(device, // device
        &renderPassCreateInfo,                                     // pCreateInfo
        nullptr,                                                   // pAllocator
        &renderPass));                                             // pRenderPass

    return renderPass;
}
} // namespace

VkRenderPass RenderPassCreatorVk::CreateRenderPass(const DeviceVk& deviceVk,
    const RenderCommandBeginRenderPass& beginRenderPass, const LowLevelRenderPassDataVk& lowLevelRenderPassData)
{
    const uint32_t subpassCount = beginRenderPass.renderPassDesc.subpassCount;
    uint32_t maxAttachmentReferenceCountPerSubpass = 0;
    for (uint32_t subpassIdx = 0; subpassIdx < subpassCount; ++subpassIdx) {
        const auto& subpassDesc = beginRenderPass.subpasses[subpassIdx];
        maxAttachmentReferenceCountPerSubpass = Math::max(maxAttachmentReferenceCountPerSubpass,
            subpassDesc.inputAttachmentCount + subpassDesc.colorAttachmentCount + subpassDesc.resolveAttachmentCount +
                subpassDesc.depthAttachmentCount + subpassDesc.depthResolveAttachmentCount +
                subpassDesc.fragmentShadingRateAttachmentCount);
    }

    const DeviceVk::CommonDeviceExtensions& deviceExtensions = deviceVk.GetCommonDeviceExtensions();
    // use renderPass2 when ever the extension is present (to make extensions work)
    if (deviceExtensions.renderPass2) {
        return CreateRenderPassCombined2(deviceVk, beginRenderPass.renderPassDesc, lowLevelRenderPassData,
            beginRenderPass.subpasses, beginRenderPass.subpassResourceStates.data(),
            &beginRenderPass.inputResourceStates, &beginRenderPass.imageLayouts, maxAttachmentReferenceCountPerSubpass,
            false, rps2_);
    } else {
        return CreateRenderPassCombined(deviceVk, beginRenderPass.renderPassDesc, lowLevelRenderPassData,
            beginRenderPass.subpasses, beginRenderPass.subpassResourceStates.data(),
            &beginRenderPass.inputResourceStates, &beginRenderPass.imageLayouts, maxAttachmentReferenceCountPerSubpass,
            false, rps1_);
    }
}

VkRenderPass RenderPassCreatorVk::CreateRenderPassCompatibility(const DeviceVk& deviceVk,
    const RenderCommandBeginRenderPass& beginRenderPass, const LowLevelRenderPassDataVk& lowLevelRenderPassData)
{
    const uint32_t subpassCount = beginRenderPass.renderPassDesc.subpassCount;
    uint32_t maxAttachmentReferenceCountPerSubpass = 0;
    for (uint32_t subpassIdx = 0; subpassIdx < subpassCount; ++subpassIdx) {
        const auto& subpassDesc = beginRenderPass.subpasses[subpassIdx];
        maxAttachmentReferenceCountPerSubpass = Math::max(maxAttachmentReferenceCountPerSubpass,
            subpassDesc.inputAttachmentCount + subpassDesc.colorAttachmentCount + subpassDesc.resolveAttachmentCount +
                subpassDesc.depthAttachmentCount + subpassDesc.depthResolveAttachmentCount +
                subpassDesc.fragmentShadingRateAttachmentCount);
    }

    const DeviceVk::CommonDeviceExtensions& deviceExtensions = deviceVk.GetCommonDeviceExtensions();
    // use renderPass2 when ever the extension is present (to make extensions work)
    if (deviceExtensions.renderPass2) {
        return CreateRenderPassCombined2(deviceVk, beginRenderPass.renderPassDesc, lowLevelRenderPassData,
            beginRenderPass.subpasses, beginRenderPass.subpassResourceStates.data(),
            &beginRenderPass.inputResourceStates, nullptr, maxAttachmentReferenceCountPerSubpass, true, rps2_);
    } else {
        return CreateRenderPassCombined(deviceVk, beginRenderPass.renderPassDesc, lowLevelRenderPassData,
            beginRenderPass.subpasses, beginRenderPass.subpassResourceStates.data(),
            &beginRenderPass.inputResourceStates, nullptr, maxAttachmentReferenceCountPerSubpass, true, rps1_);
    }
}

void RenderPassCreatorVk::DestroyRenderPass(VkDevice device, VkRenderPass renderPass)
{
    PLUGIN_ASSERT(device);
    PLUGIN_ASSERT(renderPass);

    vkDestroyRenderPass(device, // device
        renderPass,             // renderPass
        nullptr);               // pAllocator
}

RENDER_END_NAMESPACE()
