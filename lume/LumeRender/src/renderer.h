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

#include "render_backend.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class GpuResourceManager;
class IRenderContext;
class IRenderDataStoreDefaultStaging;
class ShaderManager;
class PipelineRenderNodes;
class RenderPipeline;
class RenderFrameSync;
class RenderGraph;
class PipelineInitializerBase;
class RenderNodeGraphManager;
class RenderDataStoreManager;
class RenderUtil;
class RenderNodeGraphGlobalShareDataManager;
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

    uint64_t RenderFrame(const BASE_NS::array_view<const RenderHandleReference> renderNodeGraphs) override;
    uint64_t RenderDeferred(const BASE_NS::array_view<const RenderHandleReference> renderNodeGraphs) override;
    uint64_t RenderDeferredFrame() override;

    uint64_t RenderFrameBackend(const RenderFrameBackendInfo& info) override;
    uint64_t RenderFramePresent(const RenderFramePresentInfo& info) override;

    RenderStatus GetFrameStatus() const override;

private:
    void InitNodeGraphs(const BASE_NS::array_view<const RenderHandle> renderNodeGraphs);

    // same render node graphs needs to be removed before calling this
    void RenderFrameImpl(const BASE_NS::array_view<const RenderHandle> renderNodeGraphs);
    void RenderFrameBackendImpl();
    void RenderFramePresentImpl();

    void ExecuteRenderNodes(const BASE_NS::array_view<const RenderHandle> renderNodeGraphInputs,
        const BASE_NS::array_view<RenderNodeGraphNodeStore*> renderNodeGraphNodeStores);

    void FillRngInputs(const BASE_NS::array_view<const RenderHandle> renderNodeGraphInputList,
        BASE_NS::vector<RenderHandle>& rngInputs);
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
    BASE_NS::unique_ptr<RenderNodeGraphGlobalShareDataManager> rngGlobalShareDataMgr_;

    RenderHandleReference defaultStagingRng_;
    RenderHandleReference defaultEndFrameStagingRng_;
    // deferred creation (only if using NodeGraphBackBufferConfiguration BackBufferType::GPU_IMAGE_BUFFER_COPY)
    // NOTE: should be deprecated
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

    IRenderDataStoreDefaultStaging* dsStaging_ { nullptr };

    struct RenderFrameTimeData {
        uint64_t frameIndex { 0 };

        // filled before backend run
        // cleared after present run
        RenderBackendBackBufferConfiguration config;
        BASE_NS::vector<RenderHandle> rngInputs;
        BASE_NS::vector<RenderNodeGraphNodeStore*> rngNodeStores;

        // evaluated in RenderFrameBackendImpl
        // used in RenderFramePresentImpl
        bool hasBackendWork { false };
    };
    RenderFrameTimeData renderFrameTimeData_;

    struct SeparatedRenderingData {
        bool separateBackend { false };
        bool separatePresent { false };

        // this needs to be locked by RenderFrame (front-end) and RenderFrameBackend (back-end)
        std::mutex frontMtx;
        // this needs to be locked by RenderFrameBackend (back-end) and RenderFramePresent (presentation)
        std::mutex backMtx;
    };
    SeparatedRenderingData separatedRendering_;

    RenderStatus renderStatus_;
    // could be called in parallel
    uint64_t renderStatusDeferred_ { 0 };
};
RENDER_END_NAMESPACE()

#endif // RENDER_RENDERER_H
