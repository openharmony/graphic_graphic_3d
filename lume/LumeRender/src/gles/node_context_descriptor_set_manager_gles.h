/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef GLES_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_GLES_H
#define GLES_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_GLES_H

#include <base/containers/array_view.h>
#include <base/containers/vector.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>
#include <render/render_data_structures.h>

#include "nodecontext/node_context_descriptor_set_manager.h"

RENDER_BEGIN_NAMESPACE()
class GpuImageGLES;

namespace Gles {
static constexpr uint32_t EXTERNAL_BIT = 0x8000000;

struct BufferType {
    uint32_t bufferId;
    uint32_t offset;
    uint32_t size;
};

struct ImageType {
    GpuImageGLES* image;
    uint32_t mode;
    uint32_t mipLevel;
};

struct SamplerType {
    uint32_t samplerId;
};

struct Bind {
    struct Resource {
        union {
            BufferType buffer { 0U, 0U, 0U };
            ImageType image;
        };
        SamplerType sampler { 0U };
    };

    DescriptorType descriptorType { CORE_DESCRIPTOR_TYPE_MAX_ENUM };
    BASE_NS::vector<Resource> resources;
};
} // namespace Gles
struct RenderPassBeginInfo;
class GpuResourceManager;

class DescriptorSetManagerGles final : public DescriptorSetManager {
public:
    explicit DescriptorSetManagerGles(Device& device);
    ~DescriptorSetManagerGles() override = default;

    void BeginFrame() override;
    void BeginBackendFrame();

    bool UpdateDescriptorSetGpuHandle(const RenderHandle& handle) override;
    void UpdateCpuDescriptorSetPlatform(const DescriptorSetLayoutBindingResources& bindingResources) override;

    void CreateDescriptorSets(const uint32_t arrayIndex, const uint32_t descriptorSetCount,
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;

    BASE_NS::array_view<const Gles::Bind> GetResources(RenderHandle handle) const;

private:
    BASE_NS::vector<BASE_NS::vector<BASE_NS::vector<Gles::Bind>>> resources_;
};

class NodeContextDescriptorSetManagerGles final : public NodeContextDescriptorSetManager {
public:
    explicit NodeContextDescriptorSetManagerGles(Device& device);
    ~NodeContextDescriptorSetManagerGles();

    void ResetAndReserve(const DescriptorCounts& descriptorCounts) override;
    void BeginFrame() override;

    RenderHandle CreateDescriptorSet(
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;
    RenderHandle CreateOneFrameDescriptorSet(
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;

    bool UpdateDescriptorSetGpuHandle(const RenderHandle handle) override;
    void UpdateCpuDescriptorSetPlatform(const DescriptorSetLayoutBindingResources& bindingResources) override;

    BASE_NS::array_view<const Gles::Bind> GetResources(RenderHandle handle) const;

private:
    Device& device_;
    // common descriptor sets and one frame descriptor sets
    BASE_NS::vector<BASE_NS::vector<Gles::Bind>>
        resources_[NodeContextDescriptorSetManager::DESCRIPTOR_SET_INDEX_TYPE_COUNT];
    uint32_t oneFrameDescSetGeneration_ { 0u };
#if (RENDER_VALIDATION_ENABLED == 1)
    static constexpr uint32_t MAX_ONE_FRAME_GENERATION_IDX { 16u };
#endif
};
RENDER_END_NAMESPACE()

#endif // GLES_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_GLES_H
