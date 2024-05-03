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

#include "renderer.h"

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <utility>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <core/perf/intf_performance_data_manager.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>

#include "perf/cpu_perf_scope.h"

#if (RENDER_DEV_ENABLED == 1)
#include <cinttypes>
#endif

#include "datastore/render_data_store_manager.h"
#include "datastore/render_data_store_pod.h"
#include "default_engine_constants.h"
#include "device/device.h"
#include "device/gpu_resource_cache.h"
#include "device/gpu_resource_manager.h"
#include "device/gpu_resource_util.h"
#include "device/render_frame_sync.h"
#include "device/shader_manager.h"
#include "nodecontext/node_context_descriptor_set_manager.h"
#include "nodecontext/node_context_pso_manager.h"
#include "nodecontext/render_node_context_manager.h"
#include "nodecontext/render_node_graph_manager.h"
#include "nodecontext/render_node_graph_node_store.h"
#include "perf/cpu_timer.h"
#include "render_backend.h"
#include "render_context.h"
#include "render_graph.h"
#include "util/log.h"
#include "util/render_util.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
const string_view RENDER_DATA_STORE_DEFAULT_STAGING { "RenderDataStoreDefaultStaging" };

// Helper class for running lambda as a ThreadPool task.
template<typename Fn>
class FunctionTask final : public IThreadPool::ITask {
public:
    explicit FunctionTask(Fn&& func) : func_(BASE_NS::move(func)) {};

    void operator()() override
    {
        func_();
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    Fn func_;
};

template<typename Fn>
inline IThreadPool::ITask::Ptr CreateFunctionTask(Fn&& func)
{
    return IThreadPool::ITask::Ptr { new FunctionTask<Fn>(BASE_NS::move(func)) };
}

#if (RENDER_PERF_ENABLED == 1)
struct NodeTimerData {
    CpuTimer timer;
    string_view debugName;
};
#endif

struct RenderNodeExecutionParameters {
    const array_view<RenderNodeGraphNodeStore*> renderNodeGraphNodeStores;
#if (RENDER_PERF_ENABLED == 1)
    vector<NodeTimerData>& nodeTimers;
#endif
    ITaskQueue* queue;
    IRenderDataStoreManager& renderData;
    ShaderManager& shaderManager;
    RenderingConfiguration& renderConfig;
};

// Helper for Renderer::InitNodeGraph
unordered_map<string, uint32_t> InitializeRenderNodeContextData(IRenderContext& renderContext,
    RenderNodeGraphNodeStore& nodeStore, const bool enableMultiQueue, const RenderingConfiguration& renderConfig)
{
    unordered_map<string, uint32_t> renderNodeNameToIndex(nodeStore.renderNodeData.size());
    vector<ContextInitDescription> contextInitDescs(nodeStore.renderNodeData.size());
    for (size_t nodeIdx = 0; nodeIdx < nodeStore.renderNodeData.size(); ++nodeIdx) {
        const auto& renderNodeData = nodeStore.renderNodeData[nodeIdx];
        PLUGIN_ASSERT(renderNodeData.inputData);
        PLUGIN_ASSERT(renderNodeData.node);
        auto& inputData = *(renderNodeData.inputData);
        auto& nodeContextData = nodeStore.renderNodeContextData[nodeIdx];

        renderNodeNameToIndex[renderNodeData.fullName] = (uint32_t)nodeIdx;

        // reset always, dependencies are redone with new nodes
        nodeContextData.submitInfo.signalSemaphore = false;
        nodeContextData.submitInfo.waitSemaphoreCount = 0;
        nodeContextData.submitInfo.waitForSwapchainAcquireSignal = false;

        // with dynamic render node graphs, single nodes can be initialized
        // set to true when doing the renderNode->InitNode();
        if (nodeContextData.initialized) {
            continue;
        }

        auto& contextInitRef = contextInitDescs[nodeIdx];
        contextInitRef.requestedQueue = inputData.queue;

        Device& device = (Device&)renderContext.GetDevice();
        contextInitRef.requestedQueue = device.GetValidGpuQueue(contextInitRef.requestedQueue);

        ShaderManager& shaderMgr = (ShaderManager&)renderContext.GetDevice().GetShaderManager();
        GpuResourceManager& gpuResourceMgr = (GpuResourceManager&)renderContext.GetDevice().GetGpuResourceManager();
        // ordering is important
        nodeContextData.nodeContextPsoMgr = make_unique<NodeContextPsoManager>(device, shaderMgr);
        nodeContextData.nodeContextDescriptorSetMgr = device.CreateNodeContextDescriptorSetManager();
        nodeContextData.renderCommandList =
            make_unique<RenderCommandList>(renderNodeData.fullName, *nodeContextData.nodeContextDescriptorSetMgr,
                gpuResourceMgr, *nodeContextData.nodeContextPsoMgr, contextInitRef.requestedQueue, enableMultiQueue);
        nodeContextData.nodeContextPoolMgr =
            device.CreateNodeContextPoolManager(gpuResourceMgr, contextInitRef.requestedQueue);
        RenderNodeGraphData rngd = { nodeStore.renderNodeGraphName, nodeStore.renderNodeGraphDataStoreName,
            renderConfig };
        RenderNodeContextManager::CreateInfo rncmci { renderContext, rngd, *renderNodeData.inputData,
            renderNodeData.nodeName, renderNodeData.nodeJson, *nodeContextData.nodeContextDescriptorSetMgr,
            *nodeContextData.nodeContextPsoMgr, *nodeContextData.renderCommandList,
            *nodeStore.renderNodeGraphShareDataMgr };
        nodeContextData.renderNodeContextManager = make_unique<RenderNodeContextManager>(rncmci);
#if ((RENDER_VALIDATION_ENABLED == 1) || (RENDER_VULKAN_VALIDATION_ENABLED == 1))
        nodeContextData.nodeContextDescriptorSetMgr->SetValidationDebugName(renderNodeData.fullName);
        nodeContextData.nodeContextPoolMgr->SetValidationDebugName(renderNodeData.fullName);
#endif
        nodeContextData.renderBarrierList = make_unique<RenderBarrierList>(
            (contextInitRef.requestedQueue.type != GpuQueue::QueueType::UNDEFINED) ? 4u : 0u);
    }
    return renderNodeNameToIndex;
}

// Helper for Renderer::InitNodeGraph
void PatchSignaling(RenderNodeGraphNodeStore& nodeStore, const unordered_map<string, uint32_t>& renderNodeNameToIndex)
{
    PLUGIN_ASSERT(renderNodeNameToIndex.size() == nodeStore.renderNodeData.size());
    for (size_t nodeIdx = 0; nodeIdx < nodeStore.renderNodeData.size(); ++nodeIdx) {
        PLUGIN_ASSERT(nodeStore.renderNodeData[nodeIdx].inputData);
        const auto& nodeInputDataRef = *(nodeStore.renderNodeData[nodeIdx].inputData);
        auto& submitInfo = nodeStore.renderNodeContextData[nodeIdx].submitInfo;

        for (const auto& nodeNameRef : nodeInputDataRef.gpuQueueWaitForSignals.nodeNames) {
            if (const auto iter = renderNodeNameToIndex.find(nodeNameRef); iter != renderNodeNameToIndex.cend()) {
                if (submitInfo.waitSemaphoreCount < PipelineStateConstants::MAX_RENDER_NODE_GPU_WAIT_SIGNALS) {
                    const uint32_t index = iter->second;
                    // mark node to signal
                    nodeStore.renderNodeContextData[index].submitInfo.signalSemaphore = true;

                    submitInfo.waitSemaphoreNodeIndices[submitInfo.waitSemaphoreCount] = index;
                    submitInfo.waitSemaphoreCount++;
                } else {
                    PLUGIN_LOG_E("render node can wait only for (%u) render node signals",
                        PipelineStateConstants::MAX_RENDER_NODE_GPU_WAIT_SIGNALS);
                    PLUGIN_ASSERT(false);
                }
            } else {
                PLUGIN_LOG_E("invalid render node wait signal dependency");
                PLUGIN_ASSERT(false);
            }
        }
    }
}

// Helper for Renderer::RenderFrame
void BeginRenderNodeGraph(RenderNodeGraphGlobalShareDataManager* rngGlobalShareDataMgr,
    const vector<RenderNodeGraphNodeStore*>& renderNodeGraphNodeStores,
    const RenderNodeContextManager::PerFrameTimings& timings)
{
    RenderNodeGraphShareDataManager* prevRngShareDataMgr = nullptr;
    if (rngGlobalShareDataMgr) {
        rngGlobalShareDataMgr->BeginFrame();
    }
    for (const RenderNodeGraphNodeStore* renderNodeDataStore : renderNodeGraphNodeStores) {
        const uint32_t renderNodeCount = static_cast<uint32_t>(renderNodeDataStore->renderNodeContextData.size());
        auto& rngShareData = renderNodeDataStore->renderNodeGraphShareData;
        renderNodeDataStore->renderNodeGraphShareDataMgr->BeginFrame(rngGlobalShareDataMgr, prevRngShareDataMgr,
            renderNodeCount, { rngShareData.inputs, rngShareData.inputCount },
            { rngShareData.outputs, rngShareData.outputCount });
        for (uint32_t idx = 0; idx < renderNodeCount; ++idx) {
            const RenderNodeContextData& contextData = renderNodeDataStore->renderNodeContextData[idx];
            contextData.renderCommandList->BeginFrame();
            contextData.renderBarrierList->BeginFrame();
            contextData.nodeContextPoolMgr->BeginFrame();
            contextData.nodeContextDescriptorSetMgr->BeginFrame();
            contextData.renderNodeContextManager->BeginFrame(idx, timings);
        }
        prevRngShareDataMgr = renderNodeDataStore->renderNodeGraphShareDataMgr.get();
    }
}

// Helper for Renderer::RenderFrame
inline void FillRngNodeStores(array_view<const RenderHandle> inputs, RenderNodeGraphManager& renderNodeGraphMgr,
    vector<RenderNodeGraphNodeStore*>& rngNodeStores)
{
    rngNodeStores.reserve(inputs.size());
    for (auto const& input : inputs) {
        rngNodeStores.push_back(renderNodeGraphMgr.Get(input));
    }
}

// Helper for Renderer::RenderFrame
inline bool WaitForFence(const Device& device, RenderFrameSync& renderFrameSync)
{
    RENDER_CPU_PERF_SCOPE("Renderer", "Renderer", "WaitForFrameFence_Cpu");
    renderFrameSync.WaitForFrameFence();

    return device.GetDeviceStatus();
}

// Helper for Renderer::RenderFrame
inline void ProcessRenderNodeGraph(
    Device& device, RenderGraph& renderGraph, array_view<RenderNodeGraphNodeStore*> graphNodeStoreView)
{
    RENDER_CPU_PERF_SCOPE("Renderer", "Renderer", "RenderGraph_Cpu");
    renderGraph.ProcessRenderNodeGraph(device.HasSwapchain(), graphNodeStoreView);
}

// Helper for Renderer::ExecuteRenderNodes
void CreateGpuResourcesWithRenderNodes(const array_view<RenderNodeGraphNodeStore*>& renderNodeGraphNodeStores,
    IRenderDataStoreManager& renderData, ShaderManager& shaderMgr)
{
    for (size_t graphIdx = 0; graphIdx < renderNodeGraphNodeStores.size(); ++graphIdx) {
        PLUGIN_ASSERT(renderNodeGraphNodeStores[graphIdx]);

        RenderNodeGraphNodeStore const& nodeStore = *renderNodeGraphNodeStores[graphIdx];
        for (size_t nodeIdx = 0; nodeIdx < nodeStore.renderNodeData.size(); ++nodeIdx) {
            IRenderNode& renderNode = *(nodeStore.renderNodeData[nodeIdx].node);
            renderNode.PreExecuteFrame();
        }
    }
}

// Helper for Renderer::ExecuteRenderNodes
void RenderNodeExecution(RenderNodeExecutionParameters& params)
{
#if (RENDER_PERF_ENABLED == 1)
    size_t allNodeIdx = 0;
#endif
    uint64_t taskId = 0;
    for (const auto* nodeStorePtr : params.renderNodeGraphNodeStores) {
        // there shouldn't be nullptrs but let's play it safe
        PLUGIN_ASSERT(nodeStorePtr);
        if (nodeStorePtr) {
            const auto& nodeStore = *nodeStorePtr;
            for (size_t nodeIdx = 0; nodeIdx < nodeStore.renderNodeData.size(); ++nodeIdx) {
                PLUGIN_ASSERT(nodeStore.renderNodeData[nodeIdx].node);
                if (nodeStore.renderNodeData[nodeIdx].node) {
                    IRenderNode& renderNode = *(nodeStore.renderNodeData[nodeIdx].node);
                    RenderNodeContextData const& renderNodeContextData = nodeStore.renderNodeContextData[nodeIdx];
                    PLUGIN_ASSERT(renderNodeContextData.renderCommandList);
                    RenderCommandList& renderCommandList = *renderNodeContextData.renderCommandList;

                    // Do not run render node if the flag is set
                    const uint32_t flags = renderNode.GetExecuteFlags();
                    if ((renderNode.GetExecuteFlags() &
                            IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE) == 0) {
#if (RENDER_PERF_ENABLED == 1)
                        auto& timerRef = params.nodeTimers[allNodeIdx++];
                        timerRef.debugName = nodeStore.renderNodeData[nodeIdx].fullName;
                        params.queue->Submit(
                            taskId++, CreateFunctionTask([&timerRef, &renderNode, &renderCommandList]() {
                                timerRef.timer.Begin();

                                renderCommandList.BeforeRenderNodeExecuteFrame();
                                renderNode.ExecuteFrame(renderCommandList);
                                renderCommandList.AfterRenderNodeExecuteFrame();

                                timerRef.timer.End();
                            }));
#else
                        params.queue->Submit(taskId++, CreateFunctionTask([&renderCommandList, &renderNode]() {
                            renderCommandList.BeforeRenderNodeExecuteFrame();
                            renderNode.ExecuteFrame(renderCommandList);
                            renderCommandList.AfterRenderNodeExecuteFrame();
                        }));
#endif
                    }
                }
            }
        }
    }

    // Execute and wait for completion.
    params.queue->Execute();
}

// Helper for Renderer::ExecuteRenderBackend
void IterateRenderBackendNodeGraphNodeStores(const array_view<RenderNodeGraphNodeStore*>& renderNodeGraphNodeStores,
    const bool multiQueueEnabled, RenderCommandFrameData& rcfd)
{
    for (size_t graphIdx = 0; graphIdx < renderNodeGraphNodeStores.size(); ++graphIdx) {
        PLUGIN_ASSERT(renderNodeGraphNodeStores[graphIdx]);

        RenderNodeGraphNodeStore const& nodeStore = *renderNodeGraphNodeStores[graphIdx];

        unordered_map<uint32_t, uint32_t> nodeIdxToRenderCommandContextIdx;
        const uint32_t multiQueuePatchBeginIdx = (uint32_t)rcfd.renderCommandContexts.size();
        uint32_t multiQueuePatchCount = 0;
        if (multiQueueEnabled) {
            nodeIdxToRenderCommandContextIdx.reserve(nodeStore.renderNodeContextData.size());
        }

        for (size_t nodeIdx = 0; nodeIdx < nodeStore.renderNodeContextData.size(); ++nodeIdx) {
            const auto& ref = nodeStore.renderNodeContextData[nodeIdx];
            PLUGIN_ASSERT((ref.renderCommandList != nullptr) && (ref.renderBarrierList != nullptr) &&
                          (ref.nodeContextPsoMgr != nullptr) && (ref.nodeContextPoolMgr != nullptr));
            const bool valid = (ref.renderCommandList->HasValidRenderCommands()) ? true : false;
            if (valid) {
                if (multiQueueEnabled) {
                    nodeIdxToRenderCommandContextIdx[(uint32_t)nodeIdx] = (uint32_t)rcfd.renderCommandContexts.size();
                    multiQueuePatchCount++;
                }
                // get final backend node index of the first render node which uses the swapchain image
                const uint32_t backendNodeIdx = static_cast<uint32_t>(rcfd.renderCommandContexts.size());
                if ((rcfd.firstSwapchainNodeIdx > backendNodeIdx) && (ref.submitInfo.waitForSwapchainAcquireSignal)) {
                    rcfd.firstSwapchainNodeIdx = static_cast<uint32_t>(rcfd.renderCommandContexts.size());
                }
                rcfd.renderCommandContexts.push_back({ ref.renderBackendNode, ref.renderCommandList.get(),
                    ref.renderBarrierList.get(), ref.nodeContextPsoMgr.get(), ref.nodeContextDescriptorSetMgr.get(),
                    ref.nodeContextPoolMgr.get(), (uint32_t)nodeIdx, ref.submitInfo,
                    nodeStore.renderNodeData[nodeIdx].fullName });
            }
        }

        if (multiQueueEnabled) { // patch correct render command context indices
            for (uint32_t idx = multiQueuePatchBeginIdx; idx < multiQueuePatchCount; ++idx) {
                auto& ref = rcfd.renderCommandContexts[idx];
                const auto& nodeContextRef = nodeStore.renderNodeContextData[ref.renderGraphRenderNodeIndex];

                ref.submitDepencies.signalSemaphore = nodeContextRef.submitInfo.signalSemaphore;
                ref.submitDepencies.waitSemaphoreCount = nodeContextRef.submitInfo.waitSemaphoreCount;
                for (uint32_t waitIdx = 0; waitIdx < ref.submitDepencies.waitSemaphoreCount; ++waitIdx) {
                    const uint32_t currRenderNodeIdx = nodeContextRef.submitInfo.waitSemaphoreNodeIndices[waitIdx];
                    PLUGIN_ASSERT(nodeIdxToRenderCommandContextIdx.count(currRenderNodeIdx) == 1);

                    ref.submitDepencies.waitSemaphoreNodeIndices[waitIdx] =
                        nodeIdxToRenderCommandContextIdx[currRenderNodeIdx];
                }
            }
        }
    }
}

template<typename T>
inline bool IsNull(T* ptr)
{
    return ptr == nullptr;
}

inline int64_t GetTimeStampNow()
{
    using namespace std::chrono;
    using Clock = system_clock;
    return Clock::now().time_since_epoch().count();
}

void CreateDefaultRenderNodeGraphs(const Device& device, RenderNodeGraphManager& rngMgr,
    RenderHandleReference& defaultStaging, RenderHandleReference& defaultEndFrameStaging)
{
    {
        RenderNodeGraphDesc rngd;
        {
            RenderNodeDesc rnd;
            rnd.typeName = "CORE_RN_STAGING";
            rnd.nodeName = "CORE_RN_STAGING_I";
            rnd.description.queue = { GpuQueue::QueueType::GRAPHICS, 0u };
            rngd.nodes.push_back(move(rnd));
        }
#if (RENDER_VULKAN_RT_ENABLED == 1)
        if (device.GetBackendType() == DeviceBackendType::VULKAN) {
            RenderNodeDesc rnd;
            rnd.typeName = "CORE_RN_DEFAULT_ACCELERATION_STRUCTURE_STAGING";
            rnd.nodeName = "CORE_RN_DEFAULT_ACCELERATION_STRUCTURE_STAGING_I";
            rnd.description.queue = { GpuQueue::QueueType::GRAPHICS, 0u };
            rngd.nodes.push_back(move(rnd));
        }
#endif
        defaultStaging =
            rngMgr.Create(IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, rngd);
    }
    {
        RenderNodeGraphDesc rngd;
        {
            RenderNodeDesc rnd;
            rnd.typeName = "CORE_RN_END_FRAME_STAGING";
            rnd.nodeName = "CORE_RN_END_FRAME_STAGING_I";
            rnd.description.queue = { GpuQueue::QueueType::GRAPHICS, 0u };
            rngd.nodes.push_back(move(rnd));
        }
        defaultEndFrameStaging =
            rngMgr.Create(IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, rngd);
    }
}

constexpr uint32_t CHECK_RENDER_FLAGS { RenderCreateInfo::CreateInfoFlagBits::SEPARATE_RENDER_FRAME_BACKEND |
                                        RenderCreateInfo::CreateInfoFlagBits::SEPARATE_RENDER_FRAME_PRESENT };

} // namespace

Renderer::Renderer(IRenderContext& context)
    : renderContext_(context), device_(static_cast<Device&>(context.GetDevice())),
      gpuResourceMgr_(static_cast<GpuResourceManager&>(device_.GetGpuResourceManager())),
      shaderMgr_(static_cast<ShaderManager&>(device_.GetShaderManager())),
      renderNodeGraphMgr_(static_cast<RenderNodeGraphManager&>(context.GetRenderNodeGraphManager())),
      renderDataStoreMgr_(static_cast<RenderDataStoreManager&>(context.GetRenderDataStoreManager())),
      renderUtil_(static_cast<RenderUtil&>(context.GetRenderUtil()))

{
    const RenderCreateInfo rci = ((const RenderContext&)renderContext_).GetCreateInfo();
    if (rci.createFlags & RenderCreateInfo::CreateInfoFlagBits::SEPARATE_RENDER_FRAME_BACKEND) {
        separatedRendering_.separateBackend = true;
    }
    if (rci.createFlags & RenderCreateInfo::CreateInfoFlagBits::SEPARATE_RENDER_FRAME_PRESENT) {
        separatedRendering_.separatePresent = true;
    }

    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    threadPool_ = factory->CreateThreadPool(factory->GetNumberOfCores());
    parallelQueue_ = factory->CreateParallelTaskQueue(threadPool_);
    sequentialQueue_ = factory->CreateSequentialTaskQueue(threadPool_);

    renderConfig_ = { device_.GetBackendType(), RenderingConfiguration::NdcOrigin::TOP_LEFT };
#if ((RENDER_HAS_GL_BACKEND) || (RENDER_HAS_GLES_BACKEND)) && (RENDER_GL_FLIP_Y_SWAPCHAIN == 0)
    // The flag is for informative purposes only.
    if ((renderConfig_.renderBackend == DeviceBackendType::OPENGL) ||
        (renderConfig_.renderBackend == DeviceBackendType::OPENGLES)) {
        renderConfig_.ndcOrigin = RenderingConfiguration::NdcOrigin::BOTTOM_LEFT;
    }
#endif

    renderGraph_ = make_unique<RenderGraph>(gpuResourceMgr_);
    renderBackend_ = device_.CreateRenderBackend(gpuResourceMgr_, parallelQueue_);
    renderFrameSync_ = device_.CreateRenderFrameSync();
    rngGlobalShareDataMgr_ = make_unique<RenderNodeGraphGlobalShareDataManager>();

    CreateDefaultRenderNodeGraphs(device_, renderNodeGraphMgr_, defaultStagingRng_, defaultEndFrameStagingRng_);

    dsStaging_ = static_cast<IRenderDataStoreDefaultStaging*>(
        renderDataStoreMgr_.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING));
}

Renderer::~Renderer() {}

void Renderer::InitNodeGraphs(const array_view<const RenderHandle> renderNodeGraphs)
{
    const RenderNodeGraphShareDataManager* prevRngShareDataMgr = nullptr;
    for (const auto& rng : renderNodeGraphs) {
        auto renderNodeDataStore = renderNodeGraphMgr_.Get(rng);
        if (!renderNodeDataStore) {
            continue;
        }

        RenderNodeGraphNodeStore& nodeStore = *renderNodeDataStore;
        if (nodeStore.initialized) {
            continue;
        }
        nodeStore.initialized = true;

        const bool enableMultiQueue = (device_.GetGpuQueueCount() > 1);

        // serial, initialize render node context data
        auto renderNodeNameToIndex =
            InitializeRenderNodeContextData(renderContext_, nodeStore, enableMultiQueue, renderConfig_);

        if (enableMultiQueue) {
            // patch gpu queue signaling
            PatchSignaling(nodeStore, renderNodeNameToIndex);
        }

        // NOTE: needs to be called once before init. every frame called in BeginRenderNodeGraph()
        nodeStore.renderNodeGraphShareDataMgr->BeginFrame(rngGlobalShareDataMgr_.get(), prevRngShareDataMgr,
            static_cast<uint32_t>(nodeStore.renderNodeData.size()),
            { nodeStore.renderNodeGraphShareData.inputs, nodeStore.renderNodeGraphShareData.inputCount },
            { nodeStore.renderNodeGraphShareData.outputs, nodeStore.renderNodeGraphShareData.outputCount });
        prevRngShareDataMgr = nodeStore.renderNodeGraphShareDataMgr.get();

        const RenderNodeContextManager::PerFrameTimings timings { 0, 0, device_.GetFrameCount() };
        for (size_t nodeIdx = 0; nodeIdx < nodeStore.renderNodeData.size(); ++nodeIdx) {
            auto& nodeContextData = nodeStore.renderNodeContextData[nodeIdx];
            if (nodeContextData.initialized) {
                continue;
            }
            nodeContextData.initialized = true;

            // NOTE: needs to be called once before init. every frame called in BeginRenderNodeGraph()
            nodeContextData.renderNodeContextManager->BeginFrame(static_cast<uint32_t>(nodeIdx), timings);

            auto& renderNodeData = nodeStore.renderNodeData[nodeIdx];
            PLUGIN_ASSERT(renderNodeData.inputData);
            PLUGIN_ASSERT(renderNodeData.node);

            RENDER_CPU_PERF_SCOPE("Renderer", "Renderer_InitNode_Cpu", renderNodeData.fullName);
            renderNodeData.node->InitNode(*(nodeContextData.renderNodeContextManager));
        }
    }
}

// Helper for Renderer::RenderFrame
void Renderer::RemapBackBufferHandle(const IRenderDataStoreManager& renderData)
{
    const auto* dataStorePod =
        static_cast<IRenderDataStorePod*>(renderData.GetRenderDataStore(RenderDataStorePod::TYPE_NAME));
    if (dataStorePod) {
        auto const dataView = dataStorePod->Get("NodeGraphBackBufferConfiguration");
        const auto bb = reinterpret_cast<const NodeGraphBackBufferConfiguration*>(dataView.data());
        if (bb->backBufferType == NodeGraphBackBufferConfiguration::BackBufferType::SWAPCHAIN) {
            if (!device_.HasSwapchain()) {
                PLUGIN_LOG_E("Using swapchain rendering without swapchain");
            }
        } else if (bb->backBufferType == NodeGraphBackBufferConfiguration::BackBufferType::GPU_IMAGE) {
            const RenderHandle handle = gpuResourceMgr_.GetImageRawHandle(bb->backBufferName);
            if (RenderHandleUtil::IsValid(handle) && RenderHandleUtil::IsValid(bb->backBufferHandle)) {
                gpuResourceMgr_.RemapGpuImageHandle(handle, bb->backBufferHandle);
            }
        } else if (bb->backBufferType == NodeGraphBackBufferConfiguration::BackBufferType::GPU_IMAGE_BUFFER_COPY) {
            const RenderHandle handle = gpuResourceMgr_.GetImageRawHandle(bb->backBufferName);
            if (RenderHandleUtil::IsValid(handle) && RenderHandleUtil::IsValid(bb->backBufferHandle) &&
                RenderHandleUtil::IsValid(bb->gpuBufferHandle)) {
                gpuResourceMgr_.RemapGpuImageHandle(handle, bb->backBufferHandle);
            }
            // handle image to buffer copy via post frame staging
            {
                RenderHandle backbufferHandle = bb->backBufferHandle;
                if (bb->backBufferName == DefaultEngineGpuResourceConstants::CORE_DEFAULT_BACKBUFFER) {
                    // we need to use the core default backbuffer handle and not the replaced handle in this situation
                    backbufferHandle =
                        gpuResourceMgr_.GetImageHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_BACKBUFFER)
                            .GetHandle();
                }
                const GpuImageDesc desc = gpuResourceMgr_.GetImageDescriptor(backbufferHandle);
                const BufferImageCopy bic {
                    0,                                                                // bufferOffset
                    0,                                                                // bufferRowLength
                    0,                                                                // bufferImageHeight
                    ImageSubresourceLayers { CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1u }, // imageSubresource
                    Size3D { 0, 0, 0 },                                               // imageOffset
                    Size3D { desc.width, desc.height, 1u },                           // imageExtent
                };
                dsStaging_->CopyImageToBuffer(gpuResourceMgr_.Get(backbufferHandle),
                    gpuResourceMgr_.Get(bb->gpuBufferHandle), bic,
                    IRenderDataStoreDefaultStaging::ResourceCopyInfo::END_FRAME);
            }
        }
    }
}

void Renderer::RenderFrameImpl(const array_view<const RenderHandle> renderNodeGraphs)
{
    if (separatedRendering_.separateBackend || separatedRendering_.separatePresent) {
        separatedRendering_.frontMtx.lock();
    }

    Tick();
    frameTimes_.begin = GetTimeStampNow();
    RENDER_CPU_PERF_SCOPE("Renderer", "Frame", "RenderFrame");

    if (device_.GetDeviceStatus() == false) {
        ProcessTimeStampEnd();
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_E("invalid_device_status_render_frame", "invalid device for rendering");
#endif
        return;
    }
    const IRenderDataStoreManager::RenderDataStoreFlags rdsFlags = renderDataStoreMgr_.GetRenderDataStoreFlags();
    if (rdsFlags & IRenderDataStoreManager::DOUBLE_BUFFERED_RENDER_DATA_STORES) {
#if (RENDER_VALIDATION_ENABLED == 1)
        renderDataStoreMgr_.ValidateCommitFrameData();
#endif
    }
    renderDataStoreMgr_.CommitFrameData();

    device_.Activate();
    device_.FrameStart();
    renderFrameTimeData_.frameIndex = device_.GetFrameCount();

    (static_cast<GpuResourceCache&>(gpuResourceMgr_.GetGpuResourceCache())).BeginFrame(device_.GetFrameCount());

    // handle utils (needs to be called before render data store pre renders)
    renderUtil_.BeginFrame();

    // remap the default back buffer (needs to be called before render data store pre renders)
    RemapBackBufferHandle(renderDataStoreMgr_);

    renderNodeGraphMgr_.HandlePendingAllocations();
    renderDataStoreMgr_.PreRender();

    // create new shaders if any created this frame (needs to be called before render node init)
    shaderMgr_.HandlePendingAllocations();

    auto& rngInputs = renderFrameTimeData_.rngInputs;
    auto& rngNodeStores = renderFrameTimeData_.rngNodeStores;
    PLUGIN_ASSERT(rngInputs.empty());
    PLUGIN_ASSERT(rngNodeStores.empty());

    // update render node graphs with default staging
    FillRngInputs(renderNodeGraphs, rngInputs);
    const auto renderNodeGraphInputs = array_view(rngInputs.data(), rngInputs.size());

    InitNodeGraphs(renderNodeGraphInputs);
    device_.Deactivate();

    renderGraph_->BeginFrame();

    FillRngNodeStores(renderNodeGraphInputs, renderNodeGraphMgr_, rngNodeStores);
    if (std::any_of(rngNodeStores.begin(), rngNodeStores.end(), IsNull<RenderNodeGraphNodeStore>)) {
        ProcessTimeStampEnd();
        PLUGIN_LOG_W("invalid render node graphs for rendering");
        return;
    }

    // NodeContextPoolManagerGLES::BeginFrame may delete FBOs and device must be active.
    device_.Activate();

    renderFrameSync_->BeginFrame();
    // begin frame (advance ring buffers etc.)
    const RenderNodeContextManager::PerFrameTimings timings { previousFrameTime_ - firstTime_, deltaTime_,
        device_.GetFrameCount() };
    BeginRenderNodeGraph(rngGlobalShareDataMgr_.get(), rngNodeStores, timings);

    // synchronize, needed for persistantly mapped gpu buffer writing
    if (!WaitForFence(device_, *renderFrameSync_)) {
        device_.Deactivate();
        return; // possible lost device with frame fence
    }

    // gpu resource allocation and deallocation
    gpuResourceMgr_.HandlePendingAllocations();

    device_.Deactivate();

    const auto nodeStoresView = array_view<RenderNodeGraphNodeStore*>(rngNodeStores);
    ExecuteRenderNodes(renderNodeGraphInputs, nodeStoresView);

    // render graph process for all render nodes of all render graphs
    ProcessRenderNodeGraph(device_, *renderGraph_, nodeStoresView);

    renderDataStoreMgr_.PostRender();

    // set front-end index (before mutexes)
    renderStatus_.frontEndIndex = renderFrameTimeData_.frameIndex;
    if (separatedRendering_.separateBackend || separatedRendering_.separatePresent) {
        separatedRendering_.frontMtx.unlock();
    }
    if (!separatedRendering_.separateBackend) {
        RenderFrameBackendImpl();
    }
}

void Renderer::RenderFrameBackendImpl()
{
    if (separatedRendering_.separateBackend || separatedRendering_.separatePresent) {
        separatedRendering_.frontMtx.lock();
        separatedRendering_.backMtx.lock();
    }

    auto& rngInputs = renderFrameTimeData_.rngInputs;
    auto& rngNodeStores = renderFrameTimeData_.rngNodeStores;

    device_.SetLockResourceBackendAccess(true);
    renderDataStoreMgr_.PreRenderBackend();

    size_t allRenderNodeCount = 0;
    for (const auto* nodeStore : rngNodeStores) {
        PLUGIN_ASSERT(nodeStore);
        allRenderNodeCount += nodeStore->renderNodeData.size();
    }

    RenderCommandFrameData rcfd;
    PLUGIN_ASSERT(renderFrameSync_);
    rcfd.renderFrameSync = renderFrameSync_.get();
    rcfd.renderFrameUtil = &(static_cast<RenderFrameUtil&>(renderContext_.GetRenderUtil().GetRenderFrameUtil()));
    rcfd.renderCommandContexts.reserve(allRenderNodeCount);

    const bool multiQueueEnabled = (device_.GetGpuQueueCount() > 1u);
    IterateRenderBackendNodeGraphNodeStores(rngNodeStores, multiQueueEnabled, rcfd);

    // NOTE: by node graph name
    // NOTE: deprecate this
    const RenderGraph::SwapchainStates bbState = renderGraph_->GetSwapchainResourceStates();
    RenderBackendBackBufferConfiguration config;
    for (const auto& swapState : bbState.swapchains) {
        config.swapchainData.push_back({ swapState.handle, swapState.state, swapState.layout, {} });
    }
    if (!config.swapchainData.empty()) {
        // NOTE: this is a backwards compatibility for a single (default) swapchain config data
        // should be removed
        if (auto const dataStorePod = static_cast<IRenderDataStorePod const*>(
                renderDataStoreMgr_.GetRenderDataStore(RenderDataStorePod::TYPE_NAME));
            dataStorePod) {
            auto const dataView = dataStorePod->Get("NodeGraphBackBufferConfiguration");
            if (dataView.size_bytes() == sizeof(NodeGraphBackBufferConfiguration)) {
                // expects to be the first swapchain in the list
                const NodeGraphBackBufferConfiguration* bb = (const NodeGraphBackBufferConfiguration*)dataView.data();
                config.swapchainData[0U].config = *bb;
            }
        }
    }
    renderFrameTimeData_.config = config;
    renderFrameTimeData_.hasBackendWork = (!rcfd.renderCommandContexts.empty());

    device_.Activate();

    if (renderFrameTimeData_.hasBackendWork) { // do not execute backend with zero work
        device_.SetRenderBackendRunning(true);

        frameTimes_.beginBackend = GetTimeStampNow();
        renderBackend_->Render(rcfd, config);
        frameTimes_.endBackend = GetTimeStampNow();

        device_.SetRenderBackendRunning(false);
    }
    gpuResourceMgr_.EndFrame();

    if (separatedRendering_.separatePresent) {
        device_.Deactivate();
    }

    device_.SetLockResourceBackendAccess(false);

    // clear
    rngInputs.clear();
    rngNodeStores.clear();

    // set backend-end index (before mutexes)
    renderStatus_.backEndIndex = renderStatus_.frontEndIndex;
    if (separatedRendering_.separateBackend || separatedRendering_.separatePresent) {
        separatedRendering_.frontMtx.unlock();
        separatedRendering_.backMtx.unlock();
    }
    if (!separatedRendering_.separatePresent) {
        RenderFramePresentImpl();
    }
}

void Renderer::RenderFramePresentImpl()
{
    if (separatedRendering_.separatePresent) {
        separatedRendering_.backMtx.lock();
    }

    if (renderFrameTimeData_.hasBackendWork) { // do not execute backend with zero work
        if (separatedRendering_.separatePresent) {
            device_.Activate();
        }

        frameTimes_.beginBackendPresent = GetTimeStampNow();
        renderBackend_->Present(renderFrameTimeData_.config);
        frameTimes_.endBackendPresent = GetTimeStampNow();

        if (separatedRendering_.separatePresent) {
            device_.Deactivate();
        }
    }
    if (!separatedRendering_.separatePresent) {
        device_.Deactivate();
    }

    renderDataStoreMgr_.PostRenderBackend();

    renderFrameTimeData_.config = {};

    // needs to be called after render data store post render
    renderUtil_.EndFrame();

    // RenderFramePresentImpl() needs to be called every frame even thought there isn't presenting
    device_.FrameEnd();
    ProcessTimeStampEnd();

    // set presentation index (before mutexes)
    renderStatus_.presentIndex = renderStatus_.backEndIndex;
    if (separatedRendering_.separatePresent) {
        separatedRendering_.backMtx.unlock();
    }
}

uint64_t Renderer::RenderFrame(const array_view<const RenderHandleReference> renderNodeGraphs)
{
    const auto lock = std::lock_guard(renderMutex_);

    // add only unique and valid handles to list for rendering
    vector<RenderHandle> rngs;
    rngs.reserve(renderNodeGraphs.size());
    for (size_t iIdx = 0; iIdx < renderNodeGraphs.size(); ++iIdx) {
        const RenderHandle& handle = renderNodeGraphs[iIdx].GetHandle();
        bool duplicate = false;
        for (auto& ref : rngs) {
            if (ref == handle) {
                duplicate = true;
            }
        }
        if ((RenderHandleUtil::GetHandleType(handle) == RenderHandleType::RENDER_NODE_GRAPH) && (!duplicate)) {
            rngs.push_back(handle);
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        if (duplicate) {
            PLUGIN_LOG_ONCE_E("renderer_rf_duplicate_rng",
                "RENDER_VALIDATION: duplicate render node graphs are not supported (idx: %u, id: %" PRIx64,
                static_cast<uint32_t>(iIdx), handle.id);
        }
#endif
    }
    device_.SetRenderFrameRunning(true);
    // NOTE: this is the only place from where RenderFrameImpl is called
    RenderFrameImpl(rngs);
    device_.SetRenderFrameRunning(false);

    return renderStatus_.frontEndIndex;
}

uint64_t Renderer::RenderDeferred(const array_view<const RenderHandleReference> renderNodeGraphs)
{
    const auto lock = std::lock_guard(deferredMutex_);
    for (const auto& ref : renderNodeGraphs) {
        deferredRenderNodeGraphs_.push_back(ref);
    }
    return renderStatusDeferred_ + 1;
}

uint64_t Renderer::RenderDeferredFrame()
{
    deferredMutex_.lock();
    decltype(deferredRenderNodeGraphs_) renderNodeGraphs = move(deferredRenderNodeGraphs_);
    renderStatusDeferred_ = renderStatus_.frontEndIndex + 1;
    deferredMutex_.unlock();
    RenderFrame(renderNodeGraphs);

    return renderStatus_.frontEndIndex;
}

void Renderer::ExecuteRenderNodes(const array_view<const RenderHandle> renderNodeGraphInputs,
    const array_view<RenderNodeGraphNodeStore*> renderNodeGraphNodeStores)
{
#if (RENDER_PERF_ENABLED == 1)
    RENDER_CPU_PERF_BEGIN(fullExecuteCpuTimer, "Renderer", "Renderer", "ExecuteAllNodes_Cpu");

    size_t allRenderNodeCount = 0;
    for (size_t graphIdx = 0; graphIdx < renderNodeGraphNodeStores.size(); ++graphIdx) {
        allRenderNodeCount += renderNodeGraphNodeStores[graphIdx]->renderNodeData.size();
    }

    vector<NodeTimerData> nodeTimers(allRenderNodeCount);
#endif

    ITaskQueue* queue = nullptr;
    if (device_.AllowThreadedProcessing()) {
        queue = parallelQueue_.get();
    } else {
        queue = sequentialQueue_.get();
    }

    // single threaded gpu resource creation with render nodes
    CreateGpuResourcesWithRenderNodes(renderNodeGraphNodeStores, renderDataStoreMgr_, shaderMgr_);

    // lock staging data for this frame
    // NOTE: should be done with double buffering earlier
    gpuResourceMgr_.LockFrameStagingData();
    // final gpu resource allocation and deallocation before render node execute, and render graph
    device_.Activate();
    gpuResourceMgr_.HandlePendingAllocations();
    device_.Deactivate();

    // process render node graph render node share preparations
    for (auto& ref : renderNodeGraphNodeStores) {
        ref->renderNodeGraphShareDataMgr->PrepareExecuteFrame();
    }

    RenderNodeExecutionParameters params = {
        renderNodeGraphNodeStores,
#if (RENDER_PERF_ENABLED == 1)
        nodeTimers,
#endif
        queue,
        renderDataStoreMgr_,
        shaderMgr_,
        renderConfig_
    };

    // multi-threaded render node execution
    RenderNodeExecution(params);

    // Remove tasks.
    queue->Clear();

#if (RENDER_PERF_ENABLED == 1)
    RENDER_CPU_PERF_END(fullExecuteCpuTimer);

    if (auto* inst = GetInstance<IPerformanceDataManagerFactory>(UID_PERFORMANCE_FACTORY); inst) {
        if (IPerformanceDataManager* perfData = inst->Get("RenderNode"); perfData) {
            for (size_t nodeIdx = 0; nodeIdx < nodeTimers.size(); ++nodeIdx) {
                const auto& timerRef = nodeTimers[nodeIdx];
                perfData->UpdateData(timerRef.debugName, "RenderNodeExecute_Cpu", timerRef.timer.GetMicroseconds());
            }
        }
    }
#endif
}

uint64_t Renderer::RenderFrameBackend(const RenderFrameBackendInfo& info)
{
    if (separatedRendering_.separateBackend) {
        RenderFrameBackendImpl();
    } else {
        PLUGIN_LOG_E("RenderFrameBackend called separately even though render context not created as separate");
    }

    return renderStatus_.backEndIndex;
}

uint64_t Renderer::RenderFramePresent(const RenderFramePresentInfo& info)
{
    if (separatedRendering_.separatePresent) {
        RenderFramePresentImpl();
    } else {
        PLUGIN_LOG_E("RenderFramePresent called separately even though render context not created as separate");
    }

    return renderStatus_.presentIndex;
}

IRenderer::RenderStatus Renderer::GetFrameStatus() const
{
    return renderStatus_;
}

void Renderer::FillRngInputs(
    const array_view<const RenderHandle> renderNodeGraphInputList, vector<RenderHandle>& rngInputs)
{
    constexpr size_t defaultRenderNodeGraphCount = 2;
    rngInputs.reserve(renderNodeGraphInputList.size() + defaultRenderNodeGraphCount);
    rngInputs.push_back(defaultStagingRng_.GetHandle());
    rngInputs.insert(rngInputs.end(), renderNodeGraphInputList.begin().ptr(), renderNodeGraphInputList.end().ptr());
    rngInputs.push_back(defaultEndFrameStagingRng_.GetHandle());
}

void Renderer::ProcessTimeStampEnd()
{
    frameTimes_.end = GetTimeStampNow();

    int64_t finalTime = frameTimes_.begin;
    finalTime = Math::max(finalTime, frameTimes_.beginBackend);
    frameTimes_.beginBackend = finalTime;

    finalTime = Math::max(finalTime, frameTimes_.endBackend);
    frameTimes_.endBackend = finalTime;

    finalTime = Math::max(finalTime, frameTimes_.beginBackendPresent);
    frameTimes_.beginBackendPresent = finalTime;

    finalTime = Math::max(finalTime, frameTimes_.endBackendPresent);
    frameTimes_.endBackendPresent = finalTime;

    finalTime = Math::max(finalTime, frameTimes_.end);
    frameTimes_.end = finalTime;

    PLUGIN_ASSERT(frameTimes_.end >= frameTimes_.endBackend);
    PLUGIN_ASSERT(frameTimes_.endBackend >= frameTimes_.beginBackend);
    PLUGIN_ASSERT(frameTimes_.beginBackendPresent >= frameTimes_.beginBackend);
    PLUGIN_ASSERT(frameTimes_.endBackendPresent >= frameTimes_.beginBackendPresent);

    renderUtil_.SetRenderTimings(frameTimes_);
    frameTimes_ = {};
}

void Renderer::Tick()
{
    using namespace std::chrono;
    const auto currentTime =
        static_cast<uint64_t>(duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count());

    if (firstTime_ == ~0u) {
        previousFrameTime_ = firstTime_ = currentTime;
    }
    deltaTime_ = currentTime - previousFrameTime_;
    constexpr auto limitHz = duration_cast<microseconds>(duration<float, std::ratio<1, 15u>>(1)).count();
    if (deltaTime_ > limitHz) {
        deltaTime_ = limitHz; // clamp the time step to no longer than 15hz.
    }
    previousFrameTime_ = currentTime;
}
RENDER_END_NAMESPACE()
