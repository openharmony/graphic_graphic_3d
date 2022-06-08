/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef RENDER_RENDERER_H
#define RENDER_RENDERER_H

#include <mutex>

#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <core/threading/intf_thread_pool.h>
#include <render/intf_renderer.h>
#include <render/namespace.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>
#include <render/util/intf_render_util.h>

RENDER_BEGIN_NAMESPACE()
class Device;
class GpuResourceManager;
class IRenderContext;
class ShaderManager;
class PipelineRenderNodes;
class RenderPipeline;
class RenderBackend;
class RenderFrameSync;
class RenderGraph;
class PipelineInitializerBase;
class RenderNodeGraphManager;
class RenderDataStoreManager;
class RenderUtil;
struct RenderNodeGraphNodeStore;

/**
Renderer.
Implementation of default renderer.
Renderer renderers a render node graph with input data.
*/
class Renderer final : public IRenderer {
public:
    explicit Renderer(IRenderContext& context);
    ~Renderer();

    void RenderFrame(const BASE_NS::array_view<const RenderHandleReference> renderNodeGraphs) override;
    void RenderDeferred(const BASE_NS::array_view<const RenderHandleReference> renderNodeGraphs) override;
    void RenderDeferredFrame() override;

    void InitNodeGraph(RenderHandle renderNodeGraphHandle);

private:
    // same render node graphs needs to be removed before calling this
    void RenderFrameImpl(const BASE_NS::array_view<const RenderHandle> renderNodeGraphs);

    void ExecuteRenderNodes(const BASE_NS::array_view<const RenderHandle> renderNodeGraphInputs,
        const BASE_NS::array_view<RenderNodeGraphNodeStore*> renderNodeGraphNodeStores);
    void ExecuteRenderBackend(const BASE_NS::array_view<const RenderHandle> renderNodeGraphInputs,
        const BASE_NS::array_view<RenderNodeGraphNodeStore*> renderNodeGraphNodeStores);

    BASE_NS::vector<RenderHandle> GatherInputs(const BASE_NS::array_view<const RenderHandle> renderNodeGraphInputList);
    void RemapBackBufferHandle(const IRenderDataStoreManager& renderData);

    void ProcessTimeStampEnd();
    void Tick();

    CORE_NS::IThreadPool::Ptr threadPool_;

    IRenderContext& renderContext_;
    Device& device_;
    GpuResourceManager& gpuResourceMgr_;
    ShaderManager& shaderMgr_;
    RenderNodeGraphManager& renderNodeGraphMgr_;
    RenderDataStoreManager& renderDataStoreMgr_;
    RenderUtil& renderUtil_;

    BASE_NS::unique_ptr<RenderGraph> renderGraph_;
    BASE_NS::unique_ptr<RenderBackend> renderBackend_;
    BASE_NS::unique_ptr<RenderFrameSync> renderFrameSync_;
    CORE_NS::IParallelTaskQueue::Ptr parallelQueue_;
    CORE_NS::ISequentialTaskQueue::Ptr sequentialQueue_;
    RenderingConfiguration renderConfig_;

    RenderHandleReference defaultStagingRng_;
    // deferred creation (only if using NodeGraphBackBufferConfiguration BackBufferType::GPU_IMAGE_BUFFER_COPY)
    RenderHandleReference defaultBackBufferGpuBufferRng_;

    // mutex for calling RenderFrame()
    mutable std::mutex renderMutex_;

    // mutex for deferred render node graph list
    mutable std::mutex deferredMutex_;
    BASE_NS::vector<RenderHandleReference> deferredRenderNodeGraphs_;

    RenderTimings::Times frameTimes_;
    uint64_t firstTime_ { ~0u };
    uint64_t previousFrameTime_ { ~0u };
    uint64_t deltaTime_ { 1 };
};
RENDER_END_NAMESPACE()

#endif // RENDER_RENDERER_H
