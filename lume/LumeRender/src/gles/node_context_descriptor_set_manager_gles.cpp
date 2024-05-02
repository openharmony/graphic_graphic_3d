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

#include "node_context_descriptor_set_manager_gles.h"

#include <render/namespace.h>

#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
NodeContextDescriptorSetManagerGLES::NodeContextDescriptorSetManagerGLES(Device& device)
    : NodeContextDescriptorSetManager(), device_ { device }
{
    PLUGIN_UNUSED(device_);
}

NodeContextDescriptorSetManagerGLES::~NodeContextDescriptorSetManagerGLES() {}

void NodeContextDescriptorSetManagerGLES::ResetAndReserve(const DescriptorCounts& descriptorCounts)
{
    NodeContextDescriptorSetManager::ResetAndReserve(descriptorCounts);
}

void NodeContextDescriptorSetManagerGLES::BeginFrame()
{
    NodeContextDescriptorSetManager::BeginFrame();

#if (RENDER_VALIDATION_ENABLED == 1)
    oneFrameDescSetGeneration_ = (oneFrameDescSetGeneration_ + 1) % MAX_ONE_FRAME_GENERATION_IDX;
#endif
}

RenderHandle NodeContextDescriptorSetManagerGLES::CreateDescriptorSet(
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings_)
{
    RenderHandle clientHandle;
    auto& cpuDescriptorSets = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
#if (RENDER_VALIDATION_ENABLED == 1)
    if (cpuDescriptorSets.size() >= maxSets_) {
        PLUGIN_LOG_E("RENDER_VALIDATION: No more descriptor sets available");
    }
#endif
    if (cpuDescriptorSets.size() < maxSets_) {
        uint32_t dynamicOffsetCount = 0;
        CpuDescriptorSet newSet;
        newSet.bindings.reserve(descriptorSetLayoutBindings_.size());

        uint32_t bufferCount = 0;
        uint32_t imageCount = 0;
        uint32_t samplerCount = 0;
        for (const auto& refBinding : descriptorSetLayoutBindings_) {
            // NOTE: sort from 0 to n
            newSet.bindings.push_back({ refBinding, {} });
            if (IsDynamicDescriptor(refBinding.descriptorType)) {
                dynamicOffsetCount++;
            }

            const uint32_t descriptorCount = refBinding.descriptorCount;
            if (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLER) {
                samplerCount += descriptorCount;
            } else if (((refBinding.descriptorType >= CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) &&
                           (refBinding.descriptorType <= CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE)) ||
                       (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)) {
                imageCount += descriptorCount;
            } else if (((refBinding.descriptorType >= CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER) &&
                           (refBinding.descriptorType <= CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) ||
                       (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE)) {
                bufferCount += descriptorCount;
            } else {
                PLUGIN_ASSERT_MSG(false, "descriptor type not found");
            }
        }
        newSet.buffers.resize(bufferCount);
        newSet.images.resize(imageCount);
        newSet.samplers.resize(samplerCount);

        const uint32_t arrayIndex = (uint32_t)cpuDescriptorSets.size();
        cpuDescriptorSets.push_back(move(newSet));
        auto& currCpuDescriptorSet = cpuDescriptorSets[arrayIndex];
        currCpuDescriptorSet.dynamicOffsetDescriptors.resize(dynamicOffsetCount);

        // NOTE: can be used directly to index
        clientHandle = RenderHandleUtil::CreateHandle(RenderHandleType::DESCRIPTOR_SET, arrayIndex, 0);
    }
    return clientHandle;
}

RenderHandle NodeContextDescriptorSetManagerGLES::CreateOneFrameDescriptorSet(
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings_)
{
    RenderHandle clientHandle;
    auto& cpuDescriptorSets = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME];

    uint32_t dynamicOffsetCount = 0;
    CpuDescriptorSet newSet;
    newSet.bindings.reserve(descriptorSetLayoutBindings_.size());

    uint32_t bufferCount = 0;
    uint32_t imageCount = 0;
    uint32_t samplerCount = 0;
    for (const auto& refBinding : descriptorSetLayoutBindings_) {
        // NOTE: sort from 0 to n
        newSet.bindings.push_back({ refBinding, {} });
        if (IsDynamicDescriptor(refBinding.descriptorType)) {
            dynamicOffsetCount++;
        }

        const uint32_t descriptorCount = refBinding.descriptorCount;
        if (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLER) {
            samplerCount += descriptorCount;
        } else if (((refBinding.descriptorType >= CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) &&
                       (refBinding.descriptorType <= CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE)) ||
                   (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)) {
            imageCount += descriptorCount;
        } else if (((refBinding.descriptorType >= CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER) &&
                       (refBinding.descriptorType <= CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) ||
                   (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE)) {
            bufferCount += descriptorCount;
        } else {
            PLUGIN_ASSERT_MSG(false, "RENDER_VALIDATION_ENABLED: descriptor type not found");
        }
    }
    newSet.buffers.resize(bufferCount);
    newSet.images.resize(imageCount);
    newSet.samplers.resize(samplerCount);

    const uint32_t arrayIndex = static_cast<uint32_t>(cpuDescriptorSets.size());
    cpuDescriptorSets.push_back(move(newSet));

    auto& currCpuDescriptorSet = cpuDescriptorSets[arrayIndex];
    currCpuDescriptorSet.dynamicOffsetDescriptors.resize(dynamicOffsetCount);

    // NOTE: can be used directly to index
    clientHandle = RenderHandleUtil::CreateHandle(
        RenderHandleType::DESCRIPTOR_SET, arrayIndex, oneFrameDescSetGeneration_, ONE_FRAME_DESC_SET_BIT);

    return clientHandle;
}

void NodeContextDescriptorSetManagerGLES::UpdateDescriptorSetGpuHandle(const RenderHandle handle)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t oneFrameDescBit = RenderHandleUtil::GetAdditionalData(handle);
    const uint32_t descSetIdx = (oneFrameDescBit == ONE_FRAME_DESC_SET_BIT) ? DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME
                                                                            : DESCRIPTOR_SET_INDEX_TYPE_STATIC;
    auto& cpuDescriptorSets = cpuDescriptorSets_[descSetIdx];
    if (arrayIndex >= static_cast<uint32_t>(cpuDescriptorSets.size())) {
        PLUGIN_LOG_E("invalid handle in descriptor set management");
    }
    if (oneFrameDescBit == ONE_FRAME_DESC_SET_BIT) {
        const uint32_t generationIndex = RenderHandleUtil::GetGenerationIndexPart(handle);
        if (generationIndex != oneFrameDescSetGeneration_) {
            PLUGIN_LOG_E(
                "RENDER_VALIDATION: invalid one frame descriptor set handle generation. One frame descriptor sets can "
                "only be used once.");
        }
    }
#endif
}

void NodeContextDescriptorSetManagerGLES::UpdateCpuDescriptorSetPlatform(
    const DescriptorSetLayoutBindingResources& bindingResources)
{
    // no op
}
RENDER_END_NAMESPACE()
