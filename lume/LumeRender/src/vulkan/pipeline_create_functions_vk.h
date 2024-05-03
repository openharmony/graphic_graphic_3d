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

#ifndef VUKAN_PIPELINE_CREATE_FUNCTIONS_VK_H
#define VUKAN_PIPELINE_CREATE_FUNCTIONS_VK_H

#include <vulkan/vulkan_core.h>

#include <base/containers/vector.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

#include "device/device.h"
#include "util/log.h"
#include "vulkan/node_context_descriptor_set_manager_vk.h"

RENDER_BEGIN_NAMESPACE()
class DeviceVk;
struct RenderCommandBeginRenderPass;
struct RenderPassDesc;
struct RenderPassSubpassDesc;

struct LowLevelRenderPassCompatibilityDescVk final {
    struct Attachment {
        VkFormat format { VkFormat::VK_FORMAT_UNDEFINED };
        VkSampleCountFlagBits sampleCountFlags { VK_SAMPLE_COUNT_1_BIT };
        VkImageAspectFlags aspectFlags { 0u };
    };
    Attachment attachments[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT];
};

struct LowLevelRenderPassDataVk final : public LowLevelRenderPassData {
    LowLevelRenderPassCompatibilityDescVk renderPassCompatibilityDesc;

    // these are not dynamic state values
    VkViewport viewport { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    VkRect2D scissor { { 0, 0 }, { 0, 0 } };
    VkExtent2D framebufferSize { 0, 0 };
    uint32_t viewMask { 0u };

    uint64_t renderPassCompatibilityHash { 0 };
    uint64_t renderPassHash { 0 };

    VkRenderPass renderPassCompatibility { VK_NULL_HANDLE };
    VkRenderPass renderPass { VK_NULL_HANDLE };
    uint32_t subpassIndex { 0u };

    uint64_t frameBufferHash { 0 };

    VkFramebuffer framebuffer { VK_NULL_HANDLE };
};

struct LowLevelPipelineLayoutDataVk final : public LowLevelPipelineLayoutData {
    VkPipelineLayout pipelineLayout { VK_NULL_HANDLE };
    PLUGIN_STATIC_ASSERT(PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT == 4u);
    LowLevelDescriptorSetVk descriptorSetLayouts[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT];
};

// Can be used only from a single thread.
// Typically one context creates of RenderPassCreatorVk to control render pass creation.
class RenderPassCreatorVk final {
public:
    RenderPassCreatorVk() = default;
    ~RenderPassCreatorVk() = default;

    // self-contained, only uses member vector for temporary storage
    VkRenderPass CreateRenderPass(const DeviceVk& deviceVk, const RenderCommandBeginRenderPass& beginRenderPass,
        const LowLevelRenderPassDataVk& lowLevelRenderPassData);
    VkRenderPass CreateRenderPassCompatibility(const DeviceVk& deviceVk,
        const RenderCommandBeginRenderPass& beginRenderPass, const LowLevelRenderPassDataVk& lowLevelRenderPassData);
    void DestroyRenderPass(VkDevice device, VkRenderPass renderPass);

    struct RenderPassStorage1 {
        BASE_NS::vector<VkSubpassDescription> subpassDescriptions;
        BASE_NS::vector<VkSubpassDependency> subpassDependencies;
        BASE_NS::vector<VkAttachmentReference> attachmentReferences;

        BASE_NS::vector<uint32_t> multiViewMasks;
    };
    struct RenderPassStorage2 {
        BASE_NS::vector<VkSubpassDescription2KHR> subpassDescriptions;
        BASE_NS::vector<VkSubpassDependency2KHR> subpassDependencies;
        BASE_NS::vector<VkAttachmentReference2KHR> attachmentReferences;
        BASE_NS::vector<VkSubpassDescriptionDepthStencilResolveKHR> subpassDescriptionsDepthStencilResolve;
#if (RENDER_VULKAN_FSR_ENABLED == 1)
        BASE_NS::vector<VkFragmentShadingRateAttachmentInfoKHR> fragmentShadingRateAttachmentInfos;
#endif
    };

private:
    RenderPassStorage1 rps1_;
    RenderPassStorage2 rps2_;
};
RENDER_END_NAMESPACE()

#endif // VUKAN_PIPELINE_CREATE_FUNCTIONS_VK_H
