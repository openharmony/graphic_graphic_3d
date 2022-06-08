/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
struct RenderPassBeginInfo;
class GpuResourceManager;

class NodeContextDescriptorSetManagerGLES final : public NodeContextDescriptorSetManager {
public:
    explicit NodeContextDescriptorSetManagerGLES(Device& device);
    ~NodeContextDescriptorSetManagerGLES();

    void ResetAndReserve(const DescriptorCounts& descriptorCounts) override;
    void BeginFrame() override;

    RenderHandle CreateDescriptorSet(
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;
    RenderHandle CreateOneFrameDescriptorSet(
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;

    void UpdateDescriptorSetGpuHandle(const RenderHandle handle) override;

private:
    Device& device_;

    uint32_t oneFrameDescSetGeneration_ { 0u };
#if (RENDER_VALIDATION_ENABLED == 1)
    static constexpr uint32_t MAX_ONE_FRAME_GENERATION_IDX { 16u };
#endif
};
RENDER_END_NAMESPACE()

#endif // GLES_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_GLES_H
