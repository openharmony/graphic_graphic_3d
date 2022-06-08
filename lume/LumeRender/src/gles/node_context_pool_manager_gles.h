/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef GLES_NODE_CONTEXT_POOL_MANAGER_GLES_H
#define GLES_NODE_CONTEXT_POOL_MANAGER_GLES_H

#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <render/namespace.h>

#include "device/gpu_resource_handle_util.h"
#include "nodecontext/node_context_pool_manager.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class DeviceGLES;
struct RenderCommandBeginRenderPass;
class GpuResourceManager;

struct LowlevelFramebufferGL {
    size_t width { 0 };
    size_t height { 0 };
    struct SubPassPair {
        // one fbo per subpass, one resolve fbo per subpass (if needed)
        uint32_t fbo { 0 };
        uint32_t resolve { 0 };
    };
    BASE_NS::vector<SubPassPair> fbos;
};
struct ContextFramebufferCacheGLES {
    // render pass hash (created in render command list) to frame buffer index
    BASE_NS::unordered_map<uint64_t, uint32_t> renderPassHashToIndex;
    // stores a frame index everytime the frame buffer is used (can be destroyed later on)
    BASE_NS::vector<uint64_t> frameBufferFrameUseIndex;

    BASE_NS::vector<LowlevelFramebufferGL> framebuffers;
};

class NodeContextPoolManagerGLES final : public NodeContextPoolManager {
public:
    NodeContextPoolManagerGLES(Device& device, GpuResourceManager& gpuResourceManager);
    ~NodeContextPoolManagerGLES();

    void BeginFrame() override;

    EngineResourceHandle GetFramebufferHandle(const RenderCommandBeginRenderPass& beginRenderPass);
    const LowlevelFramebufferGL& GetFramebuffer(const EngineResourceHandle handle) const;

    void FilterRenderPass(RenderCommandBeginRenderPass& beginRenderPass);
#if ((RENDER_VALIDATION_ENABLED == 1) || (RENDER_VULKAN_VALIDATION_ENABLED == 1))
    // not used ATM
    void SetValidationDebugName(const BASE_NS::string_view debugName) override {};
#endif
private:
    DeviceGLES& device_;
    GpuResourceManager& gpuResourceMgr_;
    uint32_t bufferingIndex_ { 0 };
    uint32_t bufferingCount_ { 0 };
    ContextFramebufferCacheGLES framebufferCache_;
    BASE_NS::vector<uint32_t> imageMap_;
    bool multisampledRenderToTexture_ = false;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__GLES__NODE_CONTEXT_POOL_MANAGER_GLES_H
