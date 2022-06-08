/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "renderer.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <utility>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <core/perf/intf_performance_data_manager.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
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
#include "device/device.h"
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
#include "render_graph.h"
#include "util/log.h"
#include "util/render_util.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
// Helper class for running std::function as a ThreadPool task.
class FunctionTask final : public IThreadPool::ITask {
public:
    static Ptr Create(std::function<void()> func)
    {
        return Ptr { new FunctionTask(func) };
    }

    explicit FunctionTask(std::function<void()> func) : func_(func) {};

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
    std::function<void()> func_;
};

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

void ProcessShaderReload(Device& device, ShaderManager& shaderMgr, RenderNodeGraphManager& renderNodeGraphMgr,
    const array_view<const RenderHandle>& renderNodeGraphs)
{
    if (shaderMgr.HasReloadedShaders()) {
        device.WaitForIdle();
        // NOTE: would be better to force pso re-creation based on low-level handle, but cannot be done at the moment
        PLUGIN_LOG_I("RENDER_PERFORMANCE_WARNING: re-init render nodes because of reloaded shaders");
        for (const auto& ref : renderNodeGraphs) {
            RenderNodeGraphNodeStore* nodeStore = renderNodeGraphMgr.Get(ref);
            if (nodeStore) {
                nodeStore->initialized = false;
                for (auto& nodeContextRef : nodeStore->renderNodeContextData) {
                    nodeContextRef.initialized = false; // re-init all nodes
                }
            }
        }
    }
}

RenderHandleReference CreateBackBufferGpuBufferRenderNodeGraph(RenderNodeGraphManager& renderNodeGraphMgr)
{
    RenderNodeGraphDesc rngd;
    rngd.renderNodeGraphName = "CORE_RNG_BACKBUFFER_GPUBUFFER";
    RenderNodeDesc rnd;
    rnd.typeName = "CORE_RN_BACKBUFFER_GPUBUFFER";
    rnd.nodeName = "CORE_RN_BACKBUFFER_GPUBUFFER_I";
    rnd.description.queue = { GpuQueue::QueueType::GRAPHICS, 0u };
    rngd.nodes.emplace_back(move(rnd));

    return renderNodeGraphMgr.Create(IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, rngd);
}

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
        nodeContextData.renderCommandList = make_unique<RenderCommandList>(*nodeContextData.nodeContextDescriptorSetMgr,
            gpuResourceMgr, *nodeContextData.nodeContextPsoMgr, contextInitRef.requestedQueue, enableMultiQueue);
        nodeContextData.contextPoolMgr =
            device.CreateNodeContextPoolManager(gpuResourceMgr, contextInitRef.requestedQueue);
        RenderNodeGraphData rngd = { nodeStore.renderNodeGraphName, nodeStore.renderNodeGraphDataStoreName,
            renderConfig };
        RenderNodeContextManager::CreateInfo rncmci { renderContext, rngd, *renderNodeData.inputData,
            renderNodeData.nodeName, renderNodeData.nodeJson, *nodeStore.renderNodeGpuResourceMgr,
            *nodeContextData.nodeContextDescriptorSetMgr, *nodeContextData.nodeContextPsoMgr,
            *nodeContextData.renderCommandList, *nodeStore.renderNodeGraphShareDataMgr };
        nodeContextData.renderNodeContextManager = make_unique<RenderNodeContextManager>(rncmci);
#if ((RENDER_VALIDATION_ENABLED == 1) || (RENDER_VULKAN_VALIDATION_ENABLED == 1))
        nodeContextData.nodeContextDescriptorSetMgr->SetValidationDebugName(renderNodeData.fullName);
        nodeContextData.contextPoolMgr->SetValidationDebugName(renderNodeData.fullName);
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
void BeginRenderNodeGraph(const vector<RenderNodeGraphNodeStore*>& renderNodeGraphNodeStores,
    const RenderNodeContextManager::PerFrameTimings& timings)
{
    for (const RenderNodeGraphNodeStore* renderNodeDataStore : renderNodeGraphNodeStores) {
        const uint32_t renderNodeCount = static_cast<uint32_t>(renderNodeDataStore->renderNodeContextData.size());
        auto& rngShareData = renderNodeDataStore->renderNodeGraphShareData;
        renderNodeDataStore->renderNodeGraphShareDataMgr->BeginFrame(renderNodeCount,
            { rngShareData.inputs, rngShareData.inputCount }, { rngShareData.outputs, rngShareData.outputCount });
        for (uint32_t idx = 0; idx < renderNodeCount; ++idx) {
            const RenderNodeContextData& contextData = renderNodeDataStore->renderNodeContextData[idx];
            contextData.renderCommandList->BeginFrame();
            contextData.renderBarrierList->BeginFrame();
            contextData.contextPoolMgr->BeginFrame();
            contextData.nodeContextDescriptorSetMgr->BeginFrame();
            contextData.renderNodeContextManager->BeginFrame(idx, timings);
        }
    }
}

// Helper for Renderer::RenderFrame
inline vector<RenderNodeGraphNodeStore*> GetRenderNodeGraphNodeStores(
    array_view<const RenderHandle> inputs, RenderNodeGraphManager& renderNodeGraphMgr)
{
    vector<RenderNodeGraphNodeStore*> renderNodeGraphNodeStores;
    renderNodeGraphNodeStores.reserve(inputs.size());
    for (auto const& input : inputs) {
        renderNodeGraphNodeStores.emplace_back(renderNodeGraphMgr.Get(input));
    }
    return renderNodeGraphNodeStores;
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
    const Device& device, RenderGraph& renderGraph, array_view<RenderNodeGraphNodeStore*> graphNodeStoreView)
{
    RENDER_CPU_PERF_SCOPE("Renderer", "Renderer", "RenderGraph_Cpu");
    const RenderHandle backbufferHandle = device.GetBackbufferHandle();
    renderGraph.ProcessRenderNodeGraph(backbufferHandle, graphNodeStoreView);
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
        PLUGIN_ASSERT(nodeStorePtr);
        const auto& nodeStore = *nodeStorePtr;

        for (size_t nodeIdx = 0; nodeIdx < nodeStore.renderNodeData.size(); ++nodeIdx) {
            PLUGIN_ASSERT(nodeStore.renderNodeData[nodeIdx].node);
            IRenderNode& renderNode = *(nodeStore.renderNodeData[nodeIdx].node);
            RenderNodeContextData const& renderNodeContextData = nodeStore.renderNodeContextData[nodeIdx];
            RenderCommandList& renderCommandList = *renderNodeContextData.renderCommandList;

#if (RENDER_PERF_ENABLED == 1)
            auto& timerRef = params.nodeTimers[allNodeIdx];
            timerRef.debugName = nodeStore.renderNodeData[nodeIdx].fullName;
            params.queue->Submit(taskId++, FunctionTask::Create([&timerRef, &renderNode, &renderCommandList]() {
                timerRef.timer.Begin();

                renderCommandList.BeforeRenderNodeExecuteFrame();
                renderNode.ExecuteFrame(renderCommandList);
                renderCommandList.AfterRenderNodeExecuteFrame();

                timerRef.timer.End();
            }));
            allNodeIdx++;
#else
            params.queue->Submit(taskId++, FunctionTask::Create([&renderCommandList, &renderNode]() {
                renderCommandList.BeforeRenderNodeExecuteFrame();
                renderNode.ExecuteFrame(renderCommandList);
                renderCommandList.AfterRenderNodeExecuteFrame();
            }));
#endif
        }
    }

    // Execute and wait for completion.
    params.queue->Execute();
}

// Helper for Renderer::ExecuteRenderBackend
void IterateRenderBackendNodeGraphNodeStores(const array_view<RenderNodeGraphNodeStore*>& renderNodeGraphNodeStores,
    RenderCommandFrameData& rcfd, const bool& multiQueueEnabled)
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
                          (ref.nodeContextPsoMgr != nullptr) && (ref.contextPoolMgr != nullptr));
            const bool valid = (ref.renderCommandList->HasValidRenderCommands()) ? true : false;
            if (valid) {
                if (multiQueueEnabled) {
                    nodeIdxToRenderCommandContextIdx[(uint32_t)nodeIdx] = (uint32_t)rcfd.renderCommandContexts.size();
                    multiQueuePatchCount++;
                }

                rcfd.renderCommandContexts.push_back({ ref.renderBackendNode, ref.renderCommandList.get(),
                    ref.renderBarrierList.get(), ref.nodeContextPsoMgr.get(), ref.nodeContextDescriptorSetMgr.get(),
                    ref.contextPoolMgr.get(), ref.renderCommandList->HasMultiRenderCommandListSubpasses(),
                    ref.renderCommandList->GetMultiRenderCommandListSubpassCount(), (uint32_t)nodeIdx, ref.submitInfo,
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
} // namespace

Renderer::Renderer(IRenderContext& context)
    : renderContext_(context), device_(static_cast<Device&>(context.GetDevice())),
      gpuResourceMgr_(static_cast<GpuResourceManager&>(device_.GetGpuResourceManager())),
      shaderMgr_(static_cast<ShaderManager&>(device_.GetShaderManager())),
      renderNodeGraphMgr_(static_cast<RenderNodeGraphManager&>(context.GetRenderNodeGraphManager())),
      renderDataStoreMgr_(static_cast<RenderDataStoreManager&>(context.GetRenderDataStoreManager())),
      renderUtil_(static_cast<RenderUtil&>(context.GetRenderUtil()))

{
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

    { // default render node graph for staging
        RenderNodeGraphDesc rngd;
        {
            RenderNodeDesc rnd;
            rnd.typeName = "CORE_RN_STAGING";
            rnd.nodeName = "CORE_RN_STAGING_I";
            rnd.description.queue = { GpuQueue::QueueType::GRAPHICS, 0u };
            rngd.nodes.emplace_back(move(rnd));
        }
#if (RENDER_VULKAN_RT_ENABLED == 1)
        if (device_.GetBackendType() == DeviceBackendType::VULKAN) {
            RenderNodeDesc rnd;
            rnd.typeName = "CORE_RN_DEFAULT_ACCELERATION_STRUCTURE_STAGING";
            rnd.nodeName = "CORE_RN_DEFAULT_ACCELERATION_STRUCTURE_STAGING_I";
            rnd.description.queue = { GpuQueue::QueueType::GRAPHICS, 0u };
            rngd.nodes.emplace_back(move(rnd));
        }
#endif
        defaultStagingRng_ = renderNodeGraphMgr_.Create(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, rngd);
    }
}

Renderer::~Renderer() {}

void Renderer::InitNodeGraph(RenderHandle renderNodeGraphHandle)
{
    auto renderNodeDataStore = renderNodeGraphMgr_.Get(renderNodeGraphHandle);
    if (!renderNodeDataStore) {
        return;
    }

    RenderNodeGraphNodeStore& nodeStore = *renderNodeDataStore;
    if (nodeStore.initialized) {
        return;
    }
    nodeStore.initialized = true;

    // create render node graph specific managers if not created yet
    if (!nodeStore.renderNodeGpuResourceMgr) {
        nodeStore.renderNodeGpuResourceMgr = make_unique<RenderNodeGpuResourceManager>(gpuResourceMgr_);
    }

    const bool enableMultiQueue = (device_.GetGpuQueueCount() > 1);

    // serial, initialize render node context data
    auto renderNodeNameToIndex =
        InitializeRenderNodeContextData(renderContext_, nodeStore, enableMultiQueue, renderConfig_);

    if (enableMultiQueue) {
        // patch gpu queue signaling
        PatchSignaling(nodeStore, renderNodeNameToIndex);
    }

    // NOTE: needs to be called once before init. every frame called in BeginRenderNodeGraph()
    nodeStore.renderNodeGraphShareDataMgr->BeginFrame(static_cast<uint32_t>(nodeStore.renderNodeData.size()),
        { nodeStore.renderNodeGraphShareData.inputs, nodeStore.renderNodeGraphShareData.inputCount },
        { nodeStore.renderNodeGraphShareData.outputs, nodeStore.renderNodeGraphShareData.outputCount });

    for (size_t nodeIdx = 0; nodeIdx < nodeStore.renderNodeData.size(); ++nodeIdx) {
        auto& renderNodeData = nodeStore.renderNodeData[nodeIdx];
        PLUGIN_ASSERT(renderNodeData.node);
        IRenderNode& renderNode = *(renderNodeData.node);
        auto& nodeContextData = nodeStore.renderNodeContextData[nodeIdx];

        if (nodeContextData.initialized) {
            continue;
        }
        nodeContextData.initialized = true;
        // NOTE: needs to be called once before init. every frame called in BeginRenderNodeGraph()
        const RenderNodeContextManager::PerFrameTimings timings { 0, 0, device_.GetFrameCount() };
        nodeContextData.renderNodeContextManager->BeginFrame(static_cast<uint32_t>(nodeIdx), timings);

        RENDER_CPU_PERF_SCOPE("Renderer", "Renderer_InitNode_Cpu", renderNodeData.fullName);

        PLUGIN_ASSERT(nodeStore.renderNodeData[nodeIdx].inputData);
        renderNode.InitNode(*(nodeContextData.renderNodeContextManager));
    }
}

// Helper for Renderer::RenderFrame
void Renderer::RemapBackBufferHandle(const IRenderDataStoreManager& renderData)
{
    const auto* dataStorePod = static_cast<IRenderDataStorePod*>(renderData.GetRenderDataStore("RenderDataStorePod"));
    if (dataStorePod) {
        auto const dataView = dataStorePod->Get("NodeGraphBackBufferConfiguration");
        const auto bb = reinterpret_cast<const NodeGraphBackBufferConfiguration*>(dataView.data());
        if (bb->backBufferType == NodeGraphBackBufferConfiguration::BackBufferType::SWAPCHAIN) {
            PLUGIN_ASSERT(device_.HasSwapchain());
            const RenderHandle handle = gpuResourceMgr_.GetImageRawHandle(bb->backBufferName);
            if (!RenderHandleUtil::IsValid(handle)) {
                const RenderHandle backBufferHandle = gpuResourceMgr_.GetImageRawHandle(bb->backBufferName);
                const RenderHandle firstSwapchain = gpuResourceMgr_.GetImageRawHandle("CORE_DEFAULT_SWAPCHAIN_0");
                gpuResourceMgr_.RemapGpuImageHandle(backBufferHandle, firstSwapchain);
            }
        } else if (bb->backBufferType == NodeGraphBackBufferConfiguration::BackBufferType::GPU_IMAGE) {
            const RenderHandle handle = gpuResourceMgr_.GetImageRawHandle(bb->backBufferName);
            if (RenderHandleUtil::IsValid(handle) && (bb->backBufferHandle)) {
                gpuResourceMgr_.RemapGpuImageHandle(handle, bb->backBufferHandle.GetHandle());
            }
        } else if (bb->backBufferType == NodeGraphBackBufferConfiguration::BackBufferType::GPU_IMAGE_BUFFER_COPY) {
            const RenderHandle handle = gpuResourceMgr_.GetImageRawHandle(bb->backBufferName);
            if (RenderHandleUtil::IsValid(handle) && (bb->backBufferHandle) && (bb->gpuBufferHandle)) {
                gpuResourceMgr_.RemapGpuImageHandle(handle, bb->backBufferHandle.GetHandle());
            }
        }
    }
}

void Renderer::RenderFrameImpl(const array_view<const RenderHandle> renderNodeGraphs)
{
    Tick();
    frameTimes_.begin = GetTimeStampNow();
    RENDER_CPU_PERF_SCOPE("Renderer", "Frame", "RenderFrame");

    if (device_.GetDeviceStatus() == false) {
        ProcessTimeStampEnd();
        PLUGIN_LOG_ONCE_E("invalid_device_status_render_frame", "invalid device for rendering");
        return;
    }
    device_.Activate();
    device_.FrameStart();

    renderNodeGraphMgr_.HandlePendingAllocations();
    renderDataStoreMgr_.PreRender();

    ProcessShaderReload(device_, shaderMgr_, renderNodeGraphMgr_, renderNodeGraphs);
    // create new shaders if any created this frame (needs to be called before render node init)
    shaderMgr_.HandlePendingAllocations();

    // update render node graphs with default staging and possible dev gui render node graphs
    const auto renderNodeGraphInputVector = GatherInputs(renderNodeGraphs);

    const auto renderNodeGraphInputs = array_view(renderNodeGraphInputVector.data(), renderNodeGraphInputVector.size());

    for (const auto& ref : renderNodeGraphInputs) {
        InitNodeGraph(ref);
    }
    device_.Deactivate();

    renderGraph_->BeginFrame();
    renderFrameSync_->BeginFrame();

    auto graphNodeStores = GetRenderNodeGraphNodeStores(renderNodeGraphInputs, renderNodeGraphMgr_);
    if (std::any_of(graphNodeStores.begin(), graphNodeStores.end(), IsNull<RenderNodeGraphNodeStore>)) {
        ProcessTimeStampEnd();
        PLUGIN_LOG_W("invalid render node graphs for rendering");
        return;
    }

    // NOTE: by node graph name find data
    // NOTE: deprecate this
    RemapBackBufferHandle(renderDataStoreMgr_);

    // NodeContextPoolManagerGLES::BeginFrame may delete FBOs and device must be active.
    device_.Activate();

    // begin frame (advance ring buffers etc.)
    const RenderNodeContextManager::PerFrameTimings timings { previousFrameTime_ - firstTime_, deltaTime_,
        device_.GetFrameCount() };
    BeginRenderNodeGraph(graphNodeStores, timings);

    // synchronize, needed for persistantly mapped gpu buffer writing
    if (!WaitForFence(device_, *renderFrameSync_)) {
        device_.Deactivate();
        return; // possible lost device with frame fence
    }

    // gpu resource allocation and deallocation
    gpuResourceMgr_.HandlePendingAllocations();

    device_.Deactivate();

    const auto nodeStoresView = array_view<RenderNodeGraphNodeStore*>(graphNodeStores);

    ExecuteRenderNodes(renderNodeGraphInputs, nodeStoresView);

    // render graph process for all render nodes of all render graphs
    ProcessRenderNodeGraph(device_, *renderGraph_, nodeStoresView);

    device_.SetLockResourceBackendAccess(true);
    renderDataStoreMgr_.PreRenderBackend();

    ExecuteRenderBackend(renderNodeGraphInputs, nodeStoresView);

    device_.SetLockResourceBackendAccess(false);
    renderDataStoreMgr_.PostRender();

    device_.FrameEnd();
    ProcessTimeStampEnd();
}

void Renderer::RenderFrame(const array_view<const RenderHandleReference> renderNodeGraphs)
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
            rngs.emplace_back(handle);
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        if (duplicate) {
            PLUGIN_LOG_ONCE_E("renderer_rf_duplicate_rng",
                "RENDER_VALIDATION: duplicate render node graphs are not supported (idx: %u, id: %" PRIx64,
                static_cast<uint32_t>(iIdx), handle.id);
        }
#endif
    }
    RenderFrameImpl(rngs);
}

void Renderer::RenderDeferred(const array_view<const RenderHandleReference> renderNodeGraphs)
{
    const auto lock = std::lock_guard(deferredMutex_);
    for (const auto& ref : renderNodeGraphs) {
        deferredRenderNodeGraphs_.emplace_back(ref);
    }
}

void Renderer::RenderDeferredFrame()
{
    deferredMutex_.lock();
    decltype(deferredRenderNodeGraphs_) renderNodeGraphs = move(deferredRenderNodeGraphs_);
    deferredMutex_.unlock();
    RenderFrame(renderNodeGraphs);
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
    gpuResourceMgr_.LockFrameStagingData();
    // final gpu resource allocation and dealloation before low level engine resource handle lock-up
    // we do not allocate or deallocate resources after RenderNode::ExecuteFrame()
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

void Renderer::ExecuteRenderBackend(const array_view<const RenderHandle> renderNodeGraphInputs,
    const array_view<RenderNodeGraphNodeStore*> renderNodeGraphNodeStores)
{
    size_t allRenderNodeCount = 0;
    for (const auto nodeStore : renderNodeGraphNodeStores) {
        PLUGIN_ASSERT(nodeStore);
        allRenderNodeCount += nodeStore->renderNodeData.size();
    }

    RenderCommandFrameData rcfd;
    PLUGIN_ASSERT(renderFrameSync_);
    rcfd.renderFrameSync = renderFrameSync_.get();
    rcfd.renderCommandContexts.reserve(allRenderNodeCount);

    const bool multiQueueEnabled = (device_.GetGpuQueueCount() > 1u);

    IterateRenderBackendNodeGraphNodeStores(renderNodeGraphNodeStores, rcfd, multiQueueEnabled);

    // NOTE: by node graph name
    // NOTE: deprecate this
    const RenderGraph::BackbufferState bbState = renderGraph_->GetBackbufferResourceState();
    RenderBackendBackBufferConfiguration config { bbState.state, bbState.layout, {} };
    {
        auto const dataStorePod =
            static_cast<IRenderDataStorePod const*>(renderDataStoreMgr_.GetRenderDataStore("RenderDataStorePod"));
        if (dataStorePod) {
            auto const dataView = dataStorePod->Get("NodeGraphBackBufferConfiguration");
            const NodeGraphBackBufferConfiguration* bb = (const NodeGraphBackBufferConfiguration*)dataView.data();
            config.config = *bb;
        }
    }

    if (!rcfd.renderCommandContexts.empty()) { // do not execute backend with zero work
        device_.SetRenderBackendRunning(true);
        frameTimes_.beginBackend = GetTimeStampNow();
        device_.Activate();
        renderBackend_->Render(rcfd, config);
        frameTimes_.beginBackendPresent = GetTimeStampNow();
        renderBackend_->Present(config);
        device_.Deactivate();
        frameTimes_.endBackend = GetTimeStampNow();
        device_.SetRenderBackendRunning(false);
    }

    device_.Activate();
    gpuResourceMgr_.EndFrame();
    device_.Deactivate();
}

vector<RenderHandle> Renderer::GatherInputs(const array_view<const RenderHandle> renderNodeGraphInputList)
{
    vector<RenderHandle> renderNodeGraphInputsVector;
    size_t defaultRenderNodeGraphCount = 1;
#if (RENDER_DEV_ENABLED == 1)
    defaultRenderNodeGraphCount += 1;
#endif
    renderNodeGraphInputsVector.reserve(renderNodeGraphInputList.size() + defaultRenderNodeGraphCount);
    renderNodeGraphInputsVector.emplace_back(defaultStagingRng_.GetHandle());
    renderNodeGraphInputsVector.insert(renderNodeGraphInputsVector.end(), renderNodeGraphInputList.begin().ptr(),
        renderNodeGraphInputList.end().ptr());
    if (const auto* dataStorePod =
            static_cast<IRenderDataStorePod*>(renderDataStoreMgr_.GetRenderDataStore("RenderDataStorePod"));
        dataStorePod) {
        auto const dataView = dataStorePod->Get("NodeGraphBackBufferConfiguration");
        const auto bb = reinterpret_cast<const NodeGraphBackBufferConfiguration*>(dataView.data());
        if (bb->backBufferType == NodeGraphBackBufferConfiguration::BackBufferType::GPU_IMAGE_BUFFER_COPY) {
            if (!defaultBackBufferGpuBufferRng_) {
                defaultBackBufferGpuBufferRng_ = CreateBackBufferGpuBufferRenderNodeGraph(renderNodeGraphMgr_);
                // we have passed render node graph pending allocations, re-allocate
                renderNodeGraphMgr_.HandlePendingAllocations();
            }
            renderNodeGraphInputsVector.emplace_back(defaultBackBufferGpuBufferRng_.GetHandle());
        }
    }
    return renderNodeGraphInputsVector;
}

void Renderer::ProcessTimeStampEnd()
{
    frameTimes_.end = GetTimeStampNow();

    int64_t finalTime = frameTimes_.begin;
    finalTime = Math::max(finalTime, frameTimes_.beginBackend);
    frameTimes_.beginBackend = finalTime;

    finalTime = Math::max(finalTime, frameTimes_.beginBackendPresent);
    frameTimes_.beginBackendPresent = finalTime;

    finalTime = Math::max(finalTime, frameTimes_.endBackend);
    frameTimes_.endBackend = finalTime;

    finalTime = Math::max(finalTime, frameTimes_.end);
    frameTimes_.end = finalTime;

    PLUGIN_ASSERT(frameTimes_.end >= frameTimes_.endBackend);
    PLUGIN_ASSERT(frameTimes_.endBackend >= frameTimes_.beginBackendPresent);
    PLUGIN_ASSERT(frameTimes_.beginBackendPresent >= frameTimes_.beginBackend);
    PLUGIN_ASSERT(frameTimes_.beginBackend >= frameTimes_.begin);

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
