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

#include "device/device.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
CpuDescriptorSet CreateCpuDescriptorSetData(
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    uint32_t dynamicOffsetCount = 0;
    CpuDescriptorSet newSet;
    newSet.bindings.reserve(descriptorSetLayoutBindings.size());
    LowLevelDescriptorCounts descriptorCounts;
    for (const auto& refBinding : descriptorSetLayoutBindings) {
        // NOTE: sort from 0 to n
        newSet.bindings.push_back({ refBinding, {} });
        NodeContextDescriptorSetManager::IncreaseDescriptorSetCounts(refBinding, descriptorCounts, dynamicOffsetCount);
    }
    newSet.buffers.resize(descriptorCounts.bufferCount);
    newSet.images.resize(descriptorCounts.imageCount);
    newSet.samplers.resize(descriptorCounts.samplerCount);

    newSet.dynamicOffsetDescriptors.resize(dynamicOffsetCount);
    return newSet;
}
} // namespace

DescriptorSetManagerGles::DescriptorSetManagerGles(Device& device) : DescriptorSetManager(device) {}

void DescriptorSetManagerGles::BeginFrame()
{
    DescriptorSetManager::BeginFrame();
}

void DescriptorSetManagerGles::BeginBackendFrame()
{
    // handle write locking
    // handle possible destruction
    for (const auto& descriptorSet : descriptorSets_) {
        if (GlobalDescriptorSetBase* descriptorSetBase = descriptorSet.get(); descriptorSetBase) {
            bool destroyDescriptorSets = true;
            // if we have any descriptor sets in use we do not destroy the pool
            for (auto& ref : descriptorSetBase->data) {
                if (ref.renderHandleReference.GetRefCount() > 1) {
                    destroyDescriptorSets = false;
                }
                ref.frameWriteLocked = false;
            }

            if (destroyDescriptorSets) {
                if (!descriptorSetBase->data.empty()) {
                    const RenderHandle handle = descriptorSetBase->data[0U].renderHandleReference.GetHandle();
                    // set handle (index location) to be available
                    availableHandles_.push_back(handle);
                }
                nameToIndex_.erase(descriptorSetBase->name);
                *descriptorSetBase = {};
            }
        }
    }
}

void DescriptorSetManagerGles::CreateDescriptorSets(const uint32_t arrayIndex, const uint32_t descriptorSetCount,
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    PLUGIN_ASSERT((arrayIndex < descriptorSets_.size()) && (descriptorSets_[arrayIndex]));
    PLUGIN_ASSERT(descriptorSets_[arrayIndex]->data.size() == descriptorSetCount);
    if ((arrayIndex < descriptorSets_.size()) && (descriptorSets_[arrayIndex])) {
        GlobalDescriptorSetBase* cpuData = descriptorSets_[arrayIndex].get();
        for (uint32_t idx = 0; idx < descriptorSetCount; ++idx) {
            const uint32_t additionalIndex = idx;
            cpuData->data[additionalIndex].cpuDescriptorSet = CreateCpuDescriptorSetData(descriptorSetLayoutBindings);
        }
    }
}

void DescriptorSetManagerGles::UpdateDescriptorSetGpuHandle(const RenderHandle& handle)
{
    // vulkan runs the inherited method, gles does not need it
#if (RENDER_VALIDATION_ENABLED == 1)
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    PLUGIN_ASSERT(
        RenderHandleUtil::GetAdditionalData(handle) == NodeContextDescriptorSetManager::GLOBAL_DESCRIPTOR_BIT);

    if (arrayIndex >= static_cast<uint32_t>(descriptorSets_.size())) {
        PLUGIN_LOG_E("invalid handle in descriptor set management");
    }
#endif
}

void DescriptorSetManagerGles::UpdateCpuDescriptorSetPlatform(
    const DescriptorSetLayoutBindingResources& bindingResources)
{
    // no op
}

NodeContextDescriptorSetManagerGles::NodeContextDescriptorSetManagerGles(Device& device)
    : NodeContextDescriptorSetManager(device), device_ { device }
{
    PLUGIN_UNUSED(device_);
}

NodeContextDescriptorSetManagerGles::~NodeContextDescriptorSetManagerGles() = default;

void NodeContextDescriptorSetManagerGles::ResetAndReserve(const DescriptorCounts& descriptorCounts)
{
    NodeContextDescriptorSetManager::ResetAndReserve(descriptorCounts);
}

void NodeContextDescriptorSetManagerGles::BeginFrame()
{
    NodeContextDescriptorSetManager::BeginFrame();

#if (RENDER_VALIDATION_ENABLED == 1)
    oneFrameDescSetGeneration_ = (oneFrameDescSetGeneration_ + 1) % MAX_ONE_FRAME_GENERATION_IDX;
#endif
}

RenderHandle NodeContextDescriptorSetManagerGles::CreateDescriptorSet(
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    RenderHandle clientHandle;
    auto& cpuDescriptorSets = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
#if (RENDER_VALIDATION_ENABLED == 1)
    if (cpuDescriptorSets.size() >= maxSets_) {
        PLUGIN_LOG_E("RENDER_VALIDATION: No more descriptor sets available");
    }
#endif
    if (cpuDescriptorSets.size() < maxSets_) {
        const auto arrayIndex = (uint32_t)cpuDescriptorSets.size();
        cpuDescriptorSets.push_back(CreateCpuDescriptorSetData(descriptorSetLayoutBindings));

        // NOTE: can be used directly to index
        clientHandle = RenderHandleUtil::CreateHandle(RenderHandleType::DESCRIPTOR_SET, arrayIndex, 0);
    }
    return clientHandle;
}

RenderHandle NodeContextDescriptorSetManagerGles::CreateOneFrameDescriptorSet(
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    RenderHandle clientHandle;
    auto& cpuDescriptorSets = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME];

    const auto arrayIndex = static_cast<uint32_t>(cpuDescriptorSets.size());
    cpuDescriptorSets.push_back(CreateCpuDescriptorSetData(descriptorSetLayoutBindings));

    // NOTE: can be used directly to index
    clientHandle = RenderHandleUtil::CreateHandle(
        RenderHandleType::DESCRIPTOR_SET, arrayIndex, oneFrameDescSetGeneration_, ONE_FRAME_DESC_SET_BIT);

    return clientHandle;
}

void NodeContextDescriptorSetManagerGles::UpdateDescriptorSetGpuHandle(const RenderHandle handle)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t oneFrameDescBit = RenderHandleUtil::GetAdditionalData(handle);
    const uint32_t descSetIdx = (oneFrameDescBit & ONE_FRAME_DESC_SET_BIT) ? DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME
                                                                           : DESCRIPTOR_SET_INDEX_TYPE_STATIC;
    auto& cpuDescriptorSets = cpuDescriptorSets_[descSetIdx];
    if (arrayIndex >= static_cast<uint32_t>(cpuDescriptorSets.size())) {
        PLUGIN_LOG_E("invalid handle in descriptor set management");
    }
    if (oneFrameDescBit & ONE_FRAME_DESC_SET_BIT) {
        const uint32_t generationIndex = RenderHandleUtil::GetGenerationIndexPart(handle);
        if (generationIndex != oneFrameDescSetGeneration_) {
            PLUGIN_LOG_E(
                "RENDER_VALIDATION: invalid one frame descriptor set handle generation. One frame descriptor sets can "
                "only be used once.");
        }
    }
#endif
}

void NodeContextDescriptorSetManagerGles::UpdateCpuDescriptorSetPlatform(
    const DescriptorSetLayoutBindingResources& bindingResources)
{
    // no op
}

RENDER_END_NAMESPACE()
