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

#include "render_backend_vk.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <vulkan/vulkan.h>

#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string_view.h>
#include <core/implementation_uids.h>
#include <core/perf/intf_performance_data_manager.h>
#include <core/plugin/intf_class_register.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_backend_node.h>
#include <render/vulkan/intf_device_vk.h>

#if (RENDER_PERF_ENABLED == 1)
#include "perf/gpu_query.h"
#include "perf/gpu_query_manager.h"
#include "vulkan/gpu_query_vk.h"
#endif

#include "device/gpu_acceleration_structure.h"
#include "device/gpu_buffer.h"
#include "device/gpu_image.h"
#include "device/gpu_resource_handle_util.h"
#include "device/gpu_resource_manager.h"
#include "device/gpu_sampler.h"
#include "device/pipeline_state_object.h"
#include "device/render_frame_sync.h"
#include "nodecontext/node_context_descriptor_set_manager.h"
#include "nodecontext/node_context_pool_manager.h"
#include "nodecontext/node_context_pso_manager.h"
#include "nodecontext/render_barrier_list.h"
#include "nodecontext/render_command_list.h"
#include "nodecontext/render_node_graph_node_store.h"
#include "render_backend.h"
#include "render_graph.h"
#include "util/log.h"
#include "vulkan/gpu_acceleration_structure_vk.h"
#include "vulkan/gpu_buffer_vk.h"
#include "vulkan/gpu_image_vk.h"
#include "vulkan/gpu_sampler_vk.h"
#include "vulkan/node_context_descriptor_set_manager_vk.h"
#include "vulkan/node_context_pool_manager_vk.h"
#include "vulkan/pipeline_state_object_vk.h"
#include "vulkan/render_frame_sync_vk.h"
#include "vulkan/swapchain_vk.h"
#include "vulkan/validate_vk.h"

using namespace BASE_NS;

using CORE_NS::GetInstance;
using CORE_NS::IParallelTaskQueue;
using CORE_NS::IPerformanceDataManager;
using CORE_NS::IPerformanceDataManagerFactory;
using CORE_NS::ITaskQueueFactory;
using CORE_NS::IThreadPool;
using CORE_NS::UID_TASK_QUEUE_FACTORY;

RENDER_BEGIN_NAMESPACE()
namespace {
#if (RENDER_VULKAN_RT_ENABLED == 1)
inline uint64_t GetBufferDeviceAddress(const VkDevice device, const VkBuffer buffer)
{
    const VkBufferDeviceAddressInfo addressInfo {
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, // sType
        nullptr,                                      // pNext
        buffer,                                       // buffer
    };
    return vkGetBufferDeviceAddress(device, &addressInfo);
}
#endif
} // namespace

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

#if (RENDER_DEBUG_COMMAND_MARKERS_ENABLED == 1)
namespace {
constexpr const string_view COMMAND_NAMES[] = { "Undefined", "Draw", "DrawIndirect", "Dispatch", "DispatchIndirect",
    "BindPipeline", "BeginRenderPass", "NextSubpass", "EndRenderPass", "BindVertexBuffers", "BindIndexBuffer",
    "CopyBuffer", "CopyBufferImage", "BlitImage", "BarrierPoint", "UpdateDescriptorSets", "BindDescriptorSets",
    "PushConstant", "DynamicStateViewport", "DynamicStateScissor", "DynamicStateLineWidth", "DynamicStateDepthBias",
    "DynamicStateBlendConstants", "DynamicStateDepthBounds", "DynamicStateStencil", "ExecuteBackendFramePosition",
    "WriteTimestamp", "GpuQueueTransferRelease", "GpuGueueTransferAcquire" };
} // namespace
#endif

#if (RENDER_PERF_ENABLED == 1) && (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
namespace {
static constexpr uint32_t TIME_STAMP_PER_GPU_QUERY { 2u };
}
#endif

RenderBackendVk::RenderBackendVk(
    Device& dev, GpuResourceManager& gpuResourceManager, const CORE_NS::IParallelTaskQueue::Ptr& queue)
    : RenderBackend(), device_(dev), deviceVk_(static_cast<DeviceVk&>(device_)), gpuResourceMgr_(gpuResourceManager),
      queue_(queue.get())
{
#if (RENDER_PERF_ENABLED == 1)
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
    gpuQueryMgr_ = make_unique<GpuQueryManager>();

    constexpr uint32_t maxQueryObjectCount { 512u };
    constexpr uint32_t byteSize = maxQueryObjectCount * sizeof(uint64_t) * TIME_STAMP_PER_GPU_QUERY;
    const uint32_t fullByteSize = byteSize * device_.GetCommandBufferingCount();
    const GpuBufferDesc desc {
        BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_DST_BIT,                        // usageFlags
        CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT, // memoryPropertyFlags
        0,                                                                              // engineCreationFlags
        fullByteSize,                                                                   // byteSize
    };
    perfGpuTimerData_.gpuBuffer = device_.CreateGpuBuffer(desc);
    perfGpuTimerData_.currentOffset = 0;
    perfGpuTimerData_.frameByteSize = byteSize;
    perfGpuTimerData_.fullByteSize = fullByteSize;
    { // zero initialize
        uint8_t* bufferData = static_cast<uint8_t*>(perfGpuTimerData_.gpuBuffer->Map());
        memset_s(bufferData, fullByteSize, 0, fullByteSize);
        perfGpuTimerData_.gpuBuffer->Unmap();
    }
#endif
#endif
}

void RenderBackendVk::AcquirePresentationInfo(
    RenderCommandFrameData& renderCommandFrameData, const RenderBackendBackBufferConfiguration& backBufferConfig)
{
    presentationInfo_ = {};
    if ((backBufferConfig.config.backBufferType == NodeGraphBackBufferConfiguration::BackBufferType::SWAPCHAIN) &&
        device_.HasSwapchain()) {
        presentationInfo_.useSwapchain = true;
        const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;

        const SwapchainVk* swapchain = static_cast<const SwapchainVk*>(device_.GetSwapchain());
        if (swapchain) {
            const SwapchainPlatformDataVk& platSwapchain = swapchain->GetPlatformData();
            const VkSwapchainKHR vkSwapchain = platSwapchain.swapchain;
            presentationInfo_.swapchainSemaphore = platSwapchain.swapchainImages.semaphore;

            const VkResult result = vkAcquireNextImageKHR(device, // device
                vkSwapchain,                                      // swapchin
                UINT64_MAX,                                       // timeout
                presentationInfo_.swapchainSemaphore,             // semaphore
                (VkFence) nullptr,                                // fence
                &presentationInfo_.swapchainImageIndex);          // pImageIndex

            switch (result) {
                // Success
                case VK_SUCCESS:
                case VK_TIMEOUT:
                case VK_NOT_READY:
                case VK_SUBOPTIMAL_KHR:
                    presentationInfo_.validAcquire = true;
                    break;

                // Failure
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    PLUGIN_LOG_E("vkAcquireNextImageKHR out of memory");
                    return;
                case VK_ERROR_DEVICE_LOST:
                    PLUGIN_LOG_E("vkAcquireNextImageKHR device lost");
                    return;
                case VK_ERROR_OUT_OF_DATE_KHR:
                    PLUGIN_LOG_E("vkAcquireNextImageKHR surface out of date");
                    return;
                case VK_ERROR_SURFACE_LOST_KHR:
                    PLUGIN_LOG_E("vkAcquireNextImageKHR surface lost");
                    return;

                case VK_EVENT_SET:
                case VK_EVENT_RESET:
                case VK_INCOMPLETE:
                case VK_ERROR_INITIALIZATION_FAILED:
                case VK_ERROR_MEMORY_MAP_FAILED:
                case VK_ERROR_LAYER_NOT_PRESENT:
                case VK_ERROR_EXTENSION_NOT_PRESENT:
                case VK_ERROR_FEATURE_NOT_PRESENT:
                case VK_ERROR_INCOMPATIBLE_DRIVER:
                case VK_ERROR_TOO_MANY_OBJECTS:
                case VK_ERROR_FORMAT_NOT_SUPPORTED:
                case VK_ERROR_FRAGMENTED_POOL:
                case VK_ERROR_OUT_OF_POOL_MEMORY:
                case VK_ERROR_INVALID_EXTERNAL_HANDLE:
                case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
                case VK_ERROR_VALIDATION_FAILED_EXT:
                case VK_ERROR_INVALID_SHADER_NV:
                // case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
                case VK_ERROR_FRAGMENTATION_EXT:
                case VK_ERROR_NOT_PERMITTED_EXT:
                // case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
                case VK_RESULT_MAX_ENUM:
                default:
                    PLUGIN_LOG_E("vkAcquireNextImageKHR surface lost. Device invalidated");
                    PLUGIN_ASSERT(false && "unknown result from vkAcquireNextImageKHR");
                    device_.SetDeviceStatus(false);
                    break;
            }

            PLUGIN_ASSERT(
                presentationInfo_.swapchainImageIndex < (uint32_t)platSwapchain.swapchainImages.images.size());

            // remap image to backbuffer
            const RenderHandle backBufferHandle =
                gpuResourceMgr_.GetImageRawHandle(backBufferConfig.config.backBufferName);
            const RenderHandle currentSwapchainHandle = gpuResourceMgr_.GetImageRawHandle(
                "CORE_DEFAULT_SWAPCHAIN_" + to_string(presentationInfo_.swapchainImageIndex));
            // special swapchain remapping
            gpuResourceMgr_.RenderBackendImmediateRemapGpuImageHandle(backBufferHandle, currentSwapchainHandle);
            presentationInfo_.renderGraphProcessedState = backBufferConfig.backBufferState;
            presentationInfo_.imageLayout = backBufferConfig.layout;
            if (presentationInfo_.imageLayout != ImageLayout::CORE_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
                presentationInfo_.presentationLayoutChangeNeeded = true;
                presentationInfo_.renderNodeCommandListIndex =
                    static_cast<uint32_t>(renderCommandFrameData.renderCommandContexts.size() - 1);

                const GpuImageVk* swapImage = gpuResourceMgr_.GetImage<GpuImageVk>(backBufferHandle);
                PLUGIN_ASSERT(swapImage);
                presentationInfo_.swapchainImage = swapImage->GetPlatformData().image;
            }
        }
    }

#if (RENDER_VALIDATION_ENABLED == 1)
    if ((backBufferConfig.config.backBufferType == NodeGraphBackBufferConfiguration::BackBufferType::SWAPCHAIN) &&
        (!device_.HasSwapchain())) {
        PLUGIN_LOG_E("RENDER_VALIDATION: trying to present without swapchain");
    }
#endif
}

void RenderBackendVk::Present(const RenderBackendBackBufferConfiguration& backBufferConfig)
{
    if (presentationInfo_.useSwapchain && backBufferConfig.config.present && presentationInfo_.validAcquire) {
        PLUGIN_ASSERT(!presentationInfo_.presentationLayoutChangeNeeded);
        PLUGIN_ASSERT(presentationInfo_.imageLayout == ImageLayout::CORE_IMAGE_LAYOUT_PRESENT_SRC_KHR);
#if (RENDER_PERF_ENABLED == 1)
        commonCpuTimers_.present.Begin();
#endif
        const SwapchainVk* swapchain = static_cast<const SwapchainVk*>(device_.GetSwapchain());
        if (swapchain) {
            const SwapchainPlatformDataVk& platSwapchain = swapchain->GetPlatformData();
            const VkSwapchainKHR vkSwapchain = platSwapchain.swapchain;

            PLUGIN_ASSERT(
                presentationInfo_.swapchainImageIndex < (uint32_t)platSwapchain.swapchainImages.images.size());

            // NOTE: currently waits for the last valid submission semaphore (backtraces here for valid semaphore)
            VkSemaphore waitSemaphore = VK_NULL_HANDLE;
            uint32_t waitSemaphoreCount = 0;
            if (commandBufferSubmitter_.presentationWaitSemaphore != VK_NULL_HANDLE) {
                waitSemaphore = commandBufferSubmitter_.presentationWaitSemaphore;
                waitSemaphoreCount = 1;
            }

            const VkPresentInfoKHR presentInfo {
                VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,     // sType
                nullptr,                                // pNext
                waitSemaphoreCount,                     // waitSemaphoreCount
                &waitSemaphore,                         // pWaitSemaphores
                1,                                      // swapchainCount
                &vkSwapchain,                           // pSwapchains
                &presentationInfo_.swapchainImageIndex, // pImageIndices
                nullptr                                 // pResults
            };

            const LowLevelGpuQueueVk lowLevelQueue = deviceVk_.GetPresentationGpuQueue();
            const VkResult result = vkQueuePresentKHR(lowLevelQueue.queue, // queue
                &presentInfo);                                             // pPresentInfo

            switch (result) {
                    // Success
                case VK_SUCCESS:
                    break;
                case VK_SUBOPTIMAL_KHR:
#if (RENDER_VALIDATION_ENABLED == 1)
                    PLUGIN_LOG_ONCE_W("VkQueuePresentKHR_suboptimal", "VkQueuePresentKHR suboptimal khr");
#endif
                    break;

                    // Failure
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    PLUGIN_LOG_E("vkQueuePresentKHR out of memory");
                    return;
                case VK_ERROR_DEVICE_LOST:
                    PLUGIN_LOG_E("vkQueuePresentKHR device lost");
                    return;
                case VK_ERROR_OUT_OF_DATE_KHR:
                    PLUGIN_LOG_E("vkQueuePresentKHR surface out of date");
                    return;
                case VK_ERROR_SURFACE_LOST_KHR:
                    PLUGIN_LOG_E("vkQueuePresentKHR surface lost");
                    return;

                case VK_NOT_READY:
                case VK_TIMEOUT:
                case VK_EVENT_SET:
                case VK_EVENT_RESET:
                case VK_INCOMPLETE:
                case VK_ERROR_INITIALIZATION_FAILED:
                case VK_ERROR_MEMORY_MAP_FAILED:
                case VK_ERROR_LAYER_NOT_PRESENT:
                case VK_ERROR_EXTENSION_NOT_PRESENT:
                case VK_ERROR_FEATURE_NOT_PRESENT:
                case VK_ERROR_INCOMPATIBLE_DRIVER:
                case VK_ERROR_TOO_MANY_OBJECTS:
                case VK_ERROR_FORMAT_NOT_SUPPORTED:
                case VK_ERROR_FRAGMENTED_POOL:
                case VK_ERROR_OUT_OF_POOL_MEMORY:
                case VK_ERROR_INVALID_EXTERNAL_HANDLE:
                case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
                case VK_ERROR_VALIDATION_FAILED_EXT:
                case VK_ERROR_INVALID_SHADER_NV:
                case VK_ERROR_FRAGMENTATION_EXT:
                case VK_ERROR_NOT_PERMITTED_EXT:
                case VK_RESULT_MAX_ENUM:
                default:
                    PLUGIN_LOG_E("vkQueuePresentKHR surface lost");
                    PLUGIN_ASSERT(false && "unknown result from vkQueuePresentKHR");
                    break;
            }
        }
#if (RENDER_PERF_ENABLED == 1)
        {
            commonCpuTimers_.present.End();
        }
#endif
    }
}

void RenderBackendVk::Render(
    RenderCommandFrameData& renderCommandFrameData, const RenderBackendBackBufferConfiguration& backBufferConfig)
{
    // NOTE: all command lists are validated before entering here
#if (RENDER_PERF_ENABLED == 1)
    commonCpuTimers_.full.Begin();
    commonCpuTimers_.acquire.Begin();
#endif

    commandBufferSubmitter_ = {};
    commandBufferSubmitter_.commandBuffers.resize(renderCommandFrameData.renderCommandContexts.size());

    AcquirePresentationInfo(renderCommandFrameData, backBufferConfig);
    if (presentationInfo_.useSwapchain && (!presentationInfo_.validAcquire)) {
        return;
    }

#if (RENDER_PERF_ENABLED == 1)
    commonCpuTimers_.acquire.End();

    StartFrameTimers(renderCommandFrameData);
    commonCpuTimers_.execute.Begin();
#endif

    // command list process loop/execute
    RenderProcessCommandLists(renderCommandFrameData);

#if (RENDER_PERF_ENABLED == 1)
    commonCpuTimers_.execute.End();
    commonCpuTimers_.submit.Begin();
#endif

    PLUGIN_ASSERT(renderCommandFrameData.renderCommandContexts.size() == commandBufferSubmitter_.commandBuffers.size());
    // submit vulkan command buffers
    RenderProcessSubmitCommandLists(renderCommandFrameData, backBufferConfig);

#if (RENDER_PERF_ENABLED == 1)
    commonCpuTimers_.submit.End();
    commonCpuTimers_.full.End();
    EndFrameTimers();
#endif
}

void RenderBackendVk::RenderProcessSubmitCommandLists(
    RenderCommandFrameData& renderCommandFrameData, const RenderBackendBackBufferConfiguration& backBufferConfig)
{
    // NOTE: currently backtraces to final valid command buffer semaphore
    uint32_t finalCommandBufferSubmissionIndex = ~0u;
    commandBufferSubmitter_.presentationWaitSemaphore = VK_NULL_HANDLE;
    for (int32_t cmdBufferIdx = (int32_t)commandBufferSubmitter_.commandBuffers.size() - 1; cmdBufferIdx >= 0;
         --cmdBufferIdx) {
        if ((commandBufferSubmitter_.commandBuffers[static_cast<size_t>(cmdBufferIdx)].semaphore != VK_NULL_HANDLE) &&
            (commandBufferSubmitter_.commandBuffers[static_cast<size_t>(cmdBufferIdx)].commandBuffer !=
                VK_NULL_HANDLE)) {
            finalCommandBufferSubmissionIndex = static_cast<uint32_t>(cmdBufferIdx);
            break;
        }
    }

    for (size_t cmdBufferIdx = 0; cmdBufferIdx < commandBufferSubmitter_.commandBuffers.size(); ++cmdBufferIdx) {
        const auto& cmdSubmitterRef = commandBufferSubmitter_.commandBuffers[cmdBufferIdx];
        if (cmdSubmitterRef.commandBuffer == VK_NULL_HANDLE) {
            continue;
        }

        const auto& renderContextRef = renderCommandFrameData.renderCommandContexts[cmdBufferIdx];

        uint32_t waitSemaphoreCount = 0u;
        VkSemaphore waitSemaphores[PipelineStateConstants::MAX_RENDER_NODE_GPU_WAIT_SIGNALS + 1]; // + 1 for swapchain
        VkPipelineStageFlags waitSemaphorePipelineStageFlags[PipelineStateConstants::MAX_RENDER_NODE_GPU_WAIT_SIGNALS];
        for (uint32_t waitIdx = 0; waitIdx < renderContextRef.submitDepencies.waitSemaphoreCount; ++waitIdx) {
            const uint32_t waitCmdBufferIdx = renderContextRef.submitDepencies.waitSemaphoreNodeIndices[waitIdx];
            PLUGIN_ASSERT(waitIdx < (uint32_t)commandBufferSubmitter_.commandBuffers.size());

            VkSemaphore waitSemaphore = commandBufferSubmitter_.commandBuffers[waitCmdBufferIdx].semaphore;
            if (waitSemaphore != VK_NULL_HANDLE) {
                waitSemaphores[waitSemaphoreCount] = waitSemaphore;
                waitSemaphorePipelineStageFlags[waitSemaphoreCount] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                waitSemaphoreCount++;
            }
        }

        if ((renderContextRef.submitDepencies.waitForSwapchainAcquireSignal) &&
            (presentationInfo_.swapchainSemaphore != VK_NULL_HANDLE)) {
            waitSemaphores[waitSemaphoreCount] = presentationInfo_.swapchainSemaphore;
            waitSemaphorePipelineStageFlags[waitSemaphoreCount] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            waitSemaphoreCount++;
        }

        uint32_t signalSemaphoreCount = 0u;
        VkSemaphore semaphores[] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
        VkFence fence = VK_NULL_HANDLE;
        if (finalCommandBufferSubmissionIndex == cmdBufferIdx) { // final presentation
            // add fence signaling to last submission for frame sync
            if (auto frameSync = static_cast<RenderFrameSyncVk*>(renderCommandFrameData.renderFrameSync); frameSync) {
                fence = frameSync->GetFrameFence().fence;
                frameSync->FrameFenceIsSignalled();
            }

            if (presentationInfo_.useSwapchain && backBufferConfig.config.present) {
                commandBufferSubmitter_.presentationWaitSemaphore =
                    commandBufferSubmitter_.commandBuffers[cmdBufferIdx].semaphore;
                semaphores[signalSemaphoreCount++] = commandBufferSubmitter_.presentationWaitSemaphore;
            }
            if (backBufferConfig.config.gpuSemaphoreHandle != 0) {
                semaphores[signalSemaphoreCount++] =
                    VulkanHandleCast<VkSemaphore>(backBufferConfig.config.gpuSemaphoreHandle);
            }
        } else if (renderContextRef.submitDepencies.signalSemaphore) {
            semaphores[signalSemaphoreCount++] = cmdSubmitterRef.semaphore;
        }
        PLUGIN_ASSERT(signalSemaphoreCount <= 2); // 2: no more than 2 semaphores

        const VkSubmitInfo submitInfo {
            VK_STRUCTURE_TYPE_SUBMIT_INFO,   // sType
            nullptr,                         // pNext
            waitSemaphoreCount,              // waitSemaphoreCount
            waitSemaphores,                  // pWaitSemaphores
            waitSemaphorePipelineStageFlags, // pWaitDstStageMask
            1,                               // commandBufferCount
            &cmdSubmitterRef.commandBuffer,  // pCommandBuffers
            signalSemaphoreCount,            // signalSemaphoreCount
            semaphores,                      // pSignalSemaphores
        };

        const VkQueue queue = deviceVk_.GetGpuQueue(renderContextRef.renderCommandList->GetGpuQueue()).queue;
        VALIDATE_VK_RESULT(vkQueueSubmit(queue, // queue
            1,                                  // submitCount
            &submitInfo,                        // pSubmits
            fence));                            // fence
    }
}

void RenderBackendVk::RenderProcessCommandLists(RenderCommandFrameData& renderCommandFrameData)
{
    if (queue_) {
        for (uint32_t cmdBufferIdx = 0; cmdBufferIdx < (uint32_t)renderCommandFrameData.renderCommandContexts.size();) {
            // NOTE: idx increase
            // NOTE: currently does not multi-thread dependant multi render command list render passes
            const RenderCommandContext& ref = renderCommandFrameData.renderCommandContexts[cmdBufferIdx];
            PLUGIN_ASSERT(ref.multiRenderCommandListCount > 0);
            const uint32_t rcCount = ref.multiRenderCommandListCount;
            queue_->Submit(cmdBufferIdx, FunctionTask::Create([this, cmdBufferIdx, rcCount, &renderCommandFrameData]() {
                MultiRenderCommandListDesc mrcDesc;
                mrcDesc.multiRenderCommandListCount = rcCount;
                mrcDesc.baseContext =
                    (rcCount > 1) ? &renderCommandFrameData.renderCommandContexts[cmdBufferIdx] : nullptr;

                for (uint32_t rcIdx = 0; rcIdx < rcCount; ++rcIdx) {
                    const uint32_t currIdx = cmdBufferIdx + rcIdx;
                    mrcDesc.multiRenderCommandListIndex = rcIdx;
                    RenderCommandContext& ref2 = renderCommandFrameData.renderCommandContexts[currIdx];
                    const DebugNames debugNames { ref2.debugName,
                        renderCommandFrameData.renderCommandContexts[cmdBufferIdx].debugName };
                    RenderSingleCommandList(ref2, cmdBufferIdx, mrcDesc, debugNames);
                }
            }));

            cmdBufferIdx += (rcCount > 1) ? rcCount : 1;
        }

        // Execute and wait for completion.
        queue_->Execute();
        queue_->Clear();
    } else {
        for (uint32_t cmdBufferIdx = 0; cmdBufferIdx < (uint32_t)renderCommandFrameData.renderCommandContexts.size();) {
            // NOTE: idx increase
            const RenderCommandContext& ref = renderCommandFrameData.renderCommandContexts[cmdBufferIdx];
            PLUGIN_ASSERT(ref.multiRenderCommandListCount > 0);
            const uint32_t rcCount = ref.multiRenderCommandListCount;

            MultiRenderCommandListDesc mrcDesc;
            mrcDesc.multiRenderCommandListCount = rcCount;
            mrcDesc.baseContext = (rcCount > 1) ? &renderCommandFrameData.renderCommandContexts[cmdBufferIdx] : nullptr;

            for (uint32_t rcIdx = 0; rcIdx < rcCount; ++rcIdx) {
                const uint32_t currIdx = cmdBufferIdx + rcIdx;
                mrcDesc.multiRenderCommandListIndex = rcIdx;
                RenderCommandContext& ref2 = renderCommandFrameData.renderCommandContexts[currIdx];
                const DebugNames debugNames { ref2.debugName,
                    renderCommandFrameData.renderCommandContexts[cmdBufferIdx].debugName };
                RenderSingleCommandList(ref2, cmdBufferIdx, mrcDesc, debugNames);
            }
            cmdBufferIdx += (rcCount > 1) ? rcCount : 1;
        }
    }
}

void RenderBackendVk::RenderSingleCommandList(RenderCommandContext& renderCommandCtx, const uint32_t cmdBufIdx,
    const MultiRenderCommandListDesc& multiRenderCommandListDesc, const DebugNames& debugNames)
{
    // these are validated in render graph
    const RenderCommandList& renderCommandList = *renderCommandCtx.renderCommandList;
    const RenderBarrierList& renderBarrierList = *renderCommandCtx.renderBarrierList;
    NodeContextPsoManager& nodeContextPsoMgr = *renderCommandCtx.nodeContextPsoMgr;
    NodeContextDescriptorSetManager& nodeContextDescriptorSetMgr = *renderCommandCtx.nodeContextDescriptorSetMgr;
    NodeContextPoolManager& contextPoolMgr = *renderCommandCtx.contextPoolMgr;

    ((NodeContextDescriptorSetManagerVk&)(nodeContextDescriptorSetMgr)).BeginBackendFrame();

    const array_view<const RenderCommandWithType> rcRef = renderCommandList.GetRenderCommands();

    StateCache stateCache = {}; // state cache for this render command list
    stateCache.backendNode = renderCommandCtx.renderBackendNode;

    // command buffer has been wait with a single frame fence
    const bool multiRenderCommandList =
        (multiRenderCommandListDesc.multiRenderCommandListCount > 1 && multiRenderCommandListDesc.baseContext);
    const bool beginCommandBuffer =
        (!multiRenderCommandList || (multiRenderCommandListDesc.multiRenderCommandListIndex == 0));
    const bool endCommandBuffer =
        (!multiRenderCommandList || (multiRenderCommandListDesc.multiRenderCommandListIndex ==
                                        multiRenderCommandListDesc.multiRenderCommandListCount - 1));
    const ContextCommandPoolVk* ptrCmdPool = nullptr;
    if (multiRenderCommandList) {
        ptrCmdPool = &(static_cast<NodeContextPoolManagerVk*>(multiRenderCommandListDesc.baseContext->contextPoolMgr))
                          ->GetContextCommandPool();
    } else {
        ptrCmdPool = &((NodeContextPoolManagerVk&)contextPoolMgr).GetContextCommandPool();
    }
    PLUGIN_ASSERT(ptrCmdPool);
    const LowLevelCommandBufferVk& cmdBuffer = ptrCmdPool->commandBuffer;

#if (RENDER_PERF_ENABLED == 1)
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
    const VkQueueFlags queueFlags = deviceVk_.GetGpuQueue(renderCommandList.GetGpuQueue()).queueInfo.queueFlags;
    const bool validGpuQueries = (queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) > 0;
#endif
    PLUGIN_ASSERT(timers_.count(debugNames.renderCommandBufferName) == 1);
    PerfDataSet* perfDataSet = &timers_[debugNames.renderCommandBufferName];
#endif

    if (beginCommandBuffer) {
        const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
        constexpr VkCommandPoolResetFlags commandPoolResetFlags { 0 };
        VALIDATE_VK_RESULT(vkResetCommandPool(device, // device
            ptrCmdPool->commandPool,                  // commandPool
            commandPoolResetFlags));                  // flags

        constexpr VkCommandBufferUsageFlags commandBufferUsageFlags {
            VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        const VkCommandBufferBeginInfo commandBufferBeginInfo {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, // sType
            nullptr,                                     // pNext
            commandBufferUsageFlags,                     // flags
            nullptr,                                     // pInheritanceInfo
        };

        VALIDATE_VK_RESULT(vkBeginCommandBuffer(cmdBuffer.commandBuffer, // commandBuffer
            &commandBufferBeginInfo));                                   // pBeginInfo

#if (RENDER_PERF_ENABLED == 1)
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
        if (validGpuQueries) {
            GpuQuery* gpuQuery = gpuQueryMgr_->Get(perfDataSet->gpuHandle);
            PLUGIN_ASSERT(gpuQuery);

            gpuQuery->NextQueryIndex();

            WritePerfTimeStamp(cmdBuffer, debugNames.renderCommandBufferName, 0,
                VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
        }
#endif
        perfDataSet->cpuTimer.Begin();
#endif
    }

#if (RENDER_DEBUG_MARKERS_ENABLED == 1)
    if (deviceVk_.GetDebugFunctionUtilities().vkCmdBeginDebugUtilsLabelEXT) {
        const VkDebugUtilsLabelEXT label {
            VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, // sType
            nullptr,                                 // pNext
            debugNames.renderCommandListName.data(), // pLabelName
            { 1.f, 1.f, 1.f, 1.f }                   // color[4]
        };
        deviceVk_.GetDebugFunctionUtilities().vkCmdBeginDebugUtilsLabelEXT(cmdBuffer.commandBuffer, &label);
    }
#endif

    for (const auto& ref : rcRef) {
        PLUGIN_ASSERT(ref.rc);
#if (RENDER_DEBUG_COMMAND_MARKERS_ENABLED == 1)
        if (deviceVk_.GetDebugFunctionUtilities().vkCmdBeginDebugUtilsLabelEXT) {
            const uint32_t index = (uint32_t)ref.type < countof(COMMAND_NAMES) ? (uint32_t)ref.type : 0;
            const VkDebugUtilsLabelEXT label {
                VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, // sType
                nullptr,                                 // pNext
                COMMAND_NAMES[index].data(),             // pLabelName
                { 0.87f, 0.83f, 0.29f, 1.f }             // color[4]
            };
            deviceVk_.GetDebugFunctionUtilities().vkCmdBeginDebugUtilsLabelEXT(cmdBuffer.commandBuffer, &label);
        }
#endif

        switch (ref.type) {
            case RenderCommandType::BARRIER_POINT: {
                const RenderCommandBarrierPoint& barrierPoint = *static_cast<RenderCommandBarrierPoint*>(ref.rc);

                // handle all barriers before render command that needs resource syncing
                RenderCommand(
                    barrierPoint, cmdBuffer, nodeContextPsoMgr, contextPoolMgr, stateCache, renderBarrierList);
                break;
            }
            case RenderCommandType::DRAW: {
                RenderCommand(
                    *static_cast<RenderCommandDraw*>(ref.rc), cmdBuffer, nodeContextPsoMgr, contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::DRAW_INDIRECT: {
                RenderCommand(*static_cast<RenderCommandDrawIndirect*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::DISPATCH: {
                RenderCommand(*static_cast<RenderCommandDispatch*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::DISPATCH_INDIRECT: {
                RenderCommand(*static_cast<RenderCommandDispatchIndirect*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::BIND_PIPELINE: {
                RenderCommand(*static_cast<RenderCommandBindPipeline*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::BEGIN_RENDER_PASS: {
                RenderCommand(*static_cast<RenderCommandBeginRenderPass*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::NEXT_SUBPASS: {
                RenderCommand(*static_cast<RenderCommandNextSubpass*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::END_RENDER_PASS: {
                RenderCommand(*static_cast<RenderCommandEndRenderPass*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::BIND_VERTEX_BUFFERS: {
                RenderCommand(*static_cast<RenderCommandBindVertexBuffers*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::BIND_INDEX_BUFFER: {
                RenderCommand(*static_cast<RenderCommandBindIndexBuffer*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::COPY_BUFFER: {
                RenderCommand(*static_cast<RenderCommandCopyBuffer*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::COPY_BUFFER_IMAGE: {
                RenderCommand(*static_cast<RenderCommandCopyBufferImage*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::COPY_IMAGE: {
                RenderCommand(*static_cast<RenderCommandCopyImage*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::UPDATE_DESCRIPTOR_SETS: {
                RenderCommand(*static_cast<RenderCommandUpdateDescriptorSets*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache, nodeContextDescriptorSetMgr);
                break;
            }
            case RenderCommandType::BIND_DESCRIPTOR_SETS: {
                RenderCommand(*static_cast<RenderCommandBindDescriptorSets*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache, nodeContextDescriptorSetMgr);
                break;
            }
            case RenderCommandType::PUSH_CONSTANT: {
                RenderCommand(*static_cast<RenderCommandPushConstant*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::BLIT_IMAGE: {
                RenderCommand(*static_cast<RenderCommandBlitImage*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::BUILD_ACCELERATION_STRUCTURE: {
                RenderCommand(*static_cast<RenderCommandBuildAccelerationStructure*>(ref.rc), cmdBuffer,
                    nodeContextPsoMgr, contextPoolMgr, stateCache);
                break;
            }
            // dynamic states
            case RenderCommandType::DYNAMIC_STATE_VIEWPORT: {
                RenderCommand(*static_cast<RenderCommandDynamicStateViewport*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::DYNAMIC_STATE_SCISSOR: {
                RenderCommand(*static_cast<RenderCommandDynamicStateScissor*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::DYNAMIC_STATE_LINE_WIDTH: {
                RenderCommand(*static_cast<RenderCommandDynamicStateLineWidth*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::DYNAMIC_STATE_DEPTH_BIAS: {
                RenderCommand(*static_cast<RenderCommandDynamicStateDepthBias*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::DYNAMIC_STATE_BLEND_CONSTANTS: {
                RenderCommand(*static_cast<RenderCommandDynamicStateBlendConstants*>(ref.rc), cmdBuffer,
                    nodeContextPsoMgr, contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::DYNAMIC_STATE_DEPTH_BOUNDS: {
                RenderCommand(*static_cast<RenderCommandDynamicStateDepthBounds*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::DYNAMIC_STATE_STENCIL: {
                RenderCommand(*static_cast<RenderCommandDynamicStateStencil*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::EXECUTE_BACKEND_FRAME_POSITION: {
                RenderCommand(*static_cast<RenderCommandExecuteBackendFramePosition*>(ref.rc), cmdBuffer,
                    nodeContextPsoMgr, contextPoolMgr, stateCache);
                break;
            }
            //
            case RenderCommandType::WRITE_TIMESTAMP: {
                RenderCommand(*static_cast<RenderCommandWriteTimestamp*>(ref.rc), cmdBuffer, nodeContextPsoMgr,
                    contextPoolMgr, stateCache);
                break;
            }
            case RenderCommandType::UNDEFINED:
            case RenderCommandType::GPU_QUEUE_TRANSFER_RELEASE:
            case RenderCommandType::GPU_QUEUE_TRANSFER_ACQUIRE:
            default: {
                PLUGIN_ASSERT(false && "non-valid render command");
                break;
            }
        }
#if (RENDER_DEBUG_COMMAND_MARKERS_ENABLED == 1)
        if (deviceVk_.GetDebugFunctionUtilities().vkCmdEndDebugUtilsLabelEXT) {
            deviceVk_.GetDebugFunctionUtilities().vkCmdEndDebugUtilsLabelEXT(cmdBuffer.commandBuffer);
        }
#endif
    }

    if (presentationInfo_.renderNodeCommandListIndex == cmdBufIdx) {
        RenderPresentationLayout(cmdBuffer);
    }

#if (RENDER_DEBUG_MARKERS_ENABLED == 1)
    if (deviceVk_.GetDebugFunctionUtilities().vkCmdEndDebugUtilsLabelEXT) {
        deviceVk_.GetDebugFunctionUtilities().vkCmdEndDebugUtilsLabelEXT(cmdBuffer.commandBuffer);
    }
#endif

    if (endCommandBuffer) {
#if (RENDER_PERF_ENABLED == 1)
        perfDataSet->cpuTimer.End();
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
        if (validGpuQueries) {
            WritePerfTimeStamp(cmdBuffer, debugNames.renderCommandBufferName, 1,
                VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        }
#endif
        CopyPerfTimeStamp(cmdBuffer, debugNames.renderCommandBufferName, stateCache);
#endif

        VALIDATE_VK_RESULT(vkEndCommandBuffer(cmdBuffer.commandBuffer)); // commandBuffer

        commandBufferSubmitter_.commandBuffers[cmdBufIdx] = { cmdBuffer.commandBuffer, cmdBuffer.semaphore };
    }
}

void RenderBackendVk::RenderCommand(const RenderCommandBindPipeline& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
    NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, StateCache& stateCache)
{
    const RenderHandle psoHandle = renderCmd.psoHandle;
    const VkPipelineBindPoint pipelineBindPoint = (VkPipelineBindPoint)renderCmd.pipelineBindPoint;

    stateCache.psoHandle = psoHandle;

    VkPipeline pipeline { VK_NULL_HANDLE };
    VkPipelineLayout pipelineLayout { VK_NULL_HANDLE };
    if (pipelineBindPoint == VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE) {
        const ComputePipelineStateObjectVk* pso = static_cast<const ComputePipelineStateObjectVk*>(
            psoMgr.GetComputePso(psoHandle, &stateCache.lowLevelPipelineLayoutData));
        if (pso) {
            const PipelineStateObjectPlatformDataVk& plat = pso->GetPlatformData();
            pipeline = plat.pipeline;
            pipelineLayout = plat.pipelineLayout;
        }
    } else if (pipelineBindPoint == VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS) {
        PLUGIN_ASSERT(stateCache.renderCommandBeginRenderPass != nullptr);
        if (stateCache.renderCommandBeginRenderPass) {
            uint64_t psoStateHash = stateCache.lowLevelRenderPassData.renderPassCompatibilityHash;
            if (stateCache.pipelineDescSetHash != 0) {
                HashCombine(psoStateHash, stateCache.pipelineDescSetHash);
            }
            const GraphicsPipelineStateObjectVk* pso = static_cast<const GraphicsPipelineStateObjectVk*>(
                psoMgr.GetGraphicsPso(psoHandle, stateCache.renderCommandBeginRenderPass->renderPassDesc,
                    stateCache.renderCommandBeginRenderPass->subpasses,
                    stateCache.renderCommandBeginRenderPass->subpassStartIndex, psoStateHash,
                    &stateCache.lowLevelRenderPassData, &stateCache.lowLevelPipelineLayoutData));
            if (pso) {
                const PipelineStateObjectPlatformDataVk& plat = pso->GetPlatformData();
                pipeline = plat.pipeline;
                pipelineLayout = plat.pipelineLayout;
            }
        }
    }
    PLUGIN_ASSERT(pipeline);
    PLUGIN_ASSERT(pipelineLayout);

    const bool valid = (pipeline != VK_NULL_HANDLE) ? true : false;
    if (valid) {
        stateCache.pipelineLayout = pipelineLayout;
        stateCache.lowLevelPipelineLayoutData.pipelineLayout = pipelineLayout;
        vkCmdBindPipeline(cmdBuf.commandBuffer, // commandBuffer
            pipelineBindPoint,                  // pipelineBindPoint
            pipeline);                          // pipeline
#if (RENDER_PERF_ENABLED == 1)
        stateCache.perfCounters.bindPipelineCount++;
#endif
    }
}

void RenderBackendVk::RenderCommand(const RenderCommandDraw& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
    NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache)
{
    if (renderCmd.indexCount) {
        vkCmdDrawIndexed(cmdBuf.commandBuffer, // commandBuffer
            renderCmd.indexCount,              // indexCount
            renderCmd.instanceCount,           // instanceCount
            renderCmd.firstIndex,              // firstIndex
            renderCmd.vertexOffset,            // vertexOffset
            renderCmd.firstInstance);          // firstInstance
#if (RENDER_PERF_ENABLED == 1)
        stateCache.perfCounters.drawCount++;
        stateCache.perfCounters.instanceCount += renderCmd.instanceCount;
        stateCache.perfCounters.triangleCount += renderCmd.indexCount * renderCmd.instanceCount;
#endif
    } else {
        vkCmdDraw(cmdBuf.commandBuffer, // commandBuffer
            renderCmd.vertexCount,      // vertexCount
            renderCmd.instanceCount,    // instanceCount
            renderCmd.firstVertex,      // firstVertex
            renderCmd.firstInstance);   // firstInstance
#if (RENDER_PERF_ENABLED == 1)
        stateCache.perfCounters.drawCount++;
        stateCache.perfCounters.instanceCount += renderCmd.instanceCount;
        stateCache.perfCounters.triangleCount += (renderCmd.vertexCount * 3) // 3: vertex dimension
            * renderCmd.instanceCount;
#endif
    }
}

void RenderBackendVk::RenderCommand(const RenderCommandDrawIndirect& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
    NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache)
{
    const GpuBufferVk* gpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferVk>(renderCmd.argsHandle);
    if (gpuBuffer) {
        const GpuBufferPlatformDataVk& plat = gpuBuffer->GetPlatformData();
        const VkBuffer buffer = plat.buffer;
        const VkDeviceSize offset = (VkDeviceSize)renderCmd.offset + plat.currentByteOffset;
        if (renderCmd.drawType == DrawType::DRAW_INDEXED_INDIRECT) {
            vkCmdDrawIndexedIndirect(cmdBuf.commandBuffer, // commandBuffer
                buffer,                                    // buffer
                offset,                                    // offset
                renderCmd.drawCount,                       // drawCount
                renderCmd.stride);                         // stride
        } else {
            vkCmdDrawIndirect(cmdBuf.commandBuffer, // commandBuffer
                buffer,                             // buffer
                (VkDeviceSize)renderCmd.offset,     // offset
                renderCmd.drawCount,                // drawCount
                renderCmd.stride);                  // stride
        }
#if (RENDER_PERF_ENABLED == 1)
        stateCache.perfCounters.drawIndirectCount++;
#endif
    }
}

void RenderBackendVk::RenderCommand(const RenderCommandDispatch& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
    NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache)
{
    vkCmdDispatch(cmdBuf.commandBuffer, // commandBuffer
        renderCmd.groupCountX,          // groupCountX
        renderCmd.groupCountY,          // groupCountY
        renderCmd.groupCountZ);         // groupCountZ
#if (RENDER_PERF_ENABLED == 1)
    stateCache.perfCounters.dispatchCount++;
#endif
}

void RenderBackendVk::RenderCommand(const RenderCommandDispatchIndirect& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    const StateCache& stateCache)
{
    const GpuBufferVk* gpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferVk>(renderCmd.argsHandle);
    if (gpuBuffer) {
        const GpuBufferPlatformDataVk& plat = gpuBuffer->GetPlatformData();
        const VkBuffer buffer = plat.buffer;
        const VkDeviceSize offset = (VkDeviceSize)renderCmd.offset + plat.currentByteOffset;
        vkCmdDispatchIndirect(cmdBuf.commandBuffer, // commandBuffer
            buffer,                                 // buffer
            offset);                                // offset
#if (RENDER_PERF_ENABLED == 1)
        stateCache.perfCounters.dispatchIndirectCount++;
#endif
    }
}

void RenderBackendVk::RenderCommand(const RenderCommandBeginRenderPass& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    StateCache& stateCache)
{
    PLUGIN_ASSERT(stateCache.renderCommandBeginRenderPass == nullptr);
    stateCache.renderCommandBeginRenderPass = &renderCmd;

    NodeContextPoolManagerVk& poolMgrVk = (NodeContextPoolManagerVk&)poolMgr;
    // NOTE: state cache could be optimized to store lowLevelRenderPassData in multi-rendercommandlist-case
    stateCache.lowLevelRenderPassData = poolMgrVk.GetRenderPassData(renderCmd);

    // early out for multi render command list render pass
    if (renderCmd.beginType == RenderPassBeginType::RENDER_PASS_SUBPASS_BEGIN) {
        constexpr VkSubpassContents subpassContents { VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE };
        vkCmdNextSubpass(cmdBuf.commandBuffer, // commandBuffer
            subpassContents);                  // contents

        return; // early out
    }

    const RenderPassDesc& renderPassDesc = renderCmd.renderPassDesc;

    VkClearValue clearValues[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT];
    bool hasClearValues = false;
    for (uint32_t idx = 0; idx < renderPassDesc.attachmentCount; ++idx) {
        const auto& ref = renderPassDesc.attachments[idx];
        if (ref.loadOp == AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR ||
            ref.stencilLoadOp == AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR) {
            const RenderHandle handle = renderPassDesc.attachmentHandles[idx];
            VkClearValue clearValue;
            if (RenderHandleUtil::IsDepthImage(handle)) {
                PLUGIN_STATIC_ASSERT(sizeof(clearValue.depthStencil) == sizeof(ref.clearValue.depthStencil));
                clearValue.depthStencil.depth = ref.clearValue.depthStencil.depth;
                clearValue.depthStencil.stencil = ref.clearValue.depthStencil.stencil;
            } else {
                PLUGIN_STATIC_ASSERT(sizeof(clearValue.color) == sizeof(ref.clearValue.color));
                if (!CloneData(&clearValue.color, sizeof(clearValue.color), &ref.clearValue.color,
                        sizeof(ref.clearValue.color))) {
                    PLUGIN_LOG_E("Copying of clearValue.color failed.");
                }
            }
            clearValues[idx] = clearValue;
            hasClearValues = true;
        }
    }

    // clearValueCount must be greater than the largest attachment index in renderPass that specifies a loadOp
    // (or stencilLoadOp, if the attachment has a depth/stencil format) of VK_ATTACHMENT_LOAD_OP_CLEAR
    const uint32_t clearValueCount = hasClearValues ? renderPassDesc.attachmentCount : 0;

    VkRect2D renderArea {
        { renderPassDesc.renderArea.offsetX, renderPassDesc.renderArea.offsetY },
        { renderPassDesc.renderArea.extentWidth, renderPassDesc.renderArea.extentHeight },
    };
    // render area needs to be inside frame buffer
    const auto& lowLevelData = stateCache.lowLevelRenderPassData;
    renderArea.offset.x = Math::min(renderArea.offset.x, static_cast<int32_t>(lowLevelData.framebufferSize.width));
    renderArea.offset.y = Math::min(renderArea.offset.y, static_cast<int32_t>(lowLevelData.framebufferSize.height));
    renderArea.extent.width = Math::min(renderArea.extent.width,
        static_cast<uint32_t>(static_cast<int32_t>(lowLevelData.framebufferSize.width) - renderArea.offset.x));
    renderArea.extent.height = Math::min(renderArea.extent.height,
        static_cast<uint32_t>(static_cast<int32_t>(lowLevelData.framebufferSize.height) - renderArea.offset.y));

    const VkRenderPassBeginInfo renderPassBeginInfo {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,      // sType
        nullptr,                                       // pNext
        stateCache.lowLevelRenderPassData.renderPass,  // renderPass
        stateCache.lowLevelRenderPassData.framebuffer, // framebuffer
        renderArea,                                    // renderArea
        clearValueCount,                               // clearValueCount
        clearValues,                                   // pClearValues
    };

    const VkSubpassContents subpassContents = (VkSubpassContents)renderPassDesc.subpassContents;
    vkCmdBeginRenderPass(cmdBuf.commandBuffer, // commandBuffer
        &renderPassBeginInfo,                  // pRenderPassBegin
        subpassContents);                      // contents
#if (RENDER_PERF_ENABLED == 1)
    stateCache.perfCounters.renderPassCount++;
#endif
}

void RenderBackendVk::RenderCommand(const RenderCommandNextSubpass& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
    NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache)
{
    PLUGIN_ASSERT(stateCache.renderCommandBeginRenderPass != nullptr);

    const VkSubpassContents subpassContents = (VkSubpassContents)renderCmd.subpassContents;
    vkCmdNextSubpass(cmdBuf.commandBuffer, // commandBuffer
        subpassContents);                  // contents
}

void RenderBackendVk::RenderCommand(const RenderCommandEndRenderPass& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
    NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, StateCache& stateCache)
{
    PLUGIN_ASSERT(stateCache.renderCommandBeginRenderPass != nullptr);

    // early out for multi render command list render pass
    if (renderCmd.endType == RenderPassEndType::END_SUBPASS) {
        return; // NOTE
    }

    stateCache.renderCommandBeginRenderPass = nullptr;
    stateCache.lowLevelRenderPassData = {};

    vkCmdEndRenderPass(cmdBuf.commandBuffer); // commandBuffer
}

void RenderBackendVk::RenderCommand(const RenderCommandBindVertexBuffers& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    const StateCache& stateCache)
{
    PLUGIN_ASSERT(renderCmd.vertexBufferCount > 0);
    PLUGIN_ASSERT(renderCmd.vertexBufferCount <= PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT);

    const uint32_t vertexBufferCount = renderCmd.vertexBufferCount;

    VkBuffer vertexBuffers[PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT];
    VkDeviceSize offsets[PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT];
    const GpuBufferVk* gpuBuffer = nullptr;
    RenderHandle currBufferHandle;
    for (size_t idx = 0; idx < vertexBufferCount; ++idx) {
        const VertexBuffer& currVb = renderCmd.vertexBuffers[idx];
        // our importer usually uses same GPU buffer for all vertex buffers in single primitive
        // do not re-fetch the buffer if not needed
        if (currBufferHandle.id != currVb.bufferHandle.id) {
            currBufferHandle = currVb.bufferHandle;
            gpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferVk>(currBufferHandle);
        }
        PLUGIN_ASSERT(gpuBuffer);
        if (gpuBuffer) {
            const GpuBufferPlatformDataVk& plat = gpuBuffer->GetPlatformData();
            const VkDeviceSize offset = (VkDeviceSize)currVb.bufferOffset + plat.currentByteOffset;
            vertexBuffers[idx] = plat.buffer;
            offsets[idx] = offset;
        }
    }

    vkCmdBindVertexBuffers(cmdBuf.commandBuffer, // commandBuffer
        0,                                       // firstBinding
        vertexBufferCount,                       // bindingCount
        vertexBuffers,                           // pBuffers
        offsets);                                // pOffsets
}

void RenderBackendVk::RenderCommand(const RenderCommandBindIndexBuffer& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    const StateCache& stateCache)
{
    const GpuBufferVk* gpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferVk>(renderCmd.indexBuffer.bufferHandle);

    PLUGIN_ASSERT(gpuBuffer);
    if (gpuBuffer) {
        const GpuBufferPlatformDataVk& plat = gpuBuffer->GetPlatformData();
        const VkBuffer buffer = plat.buffer;
        const VkDeviceSize offset = (VkDeviceSize)renderCmd.indexBuffer.bufferOffset + plat.currentByteOffset;
        const VkIndexType indexType = (VkIndexType)renderCmd.indexBuffer.indexType;

        vkCmdBindIndexBuffer(cmdBuf.commandBuffer, // commandBuffer
            buffer,                                // buffer
            offset,                                // offset
            indexType);                            // indexType
    }
}

void RenderBackendVk::RenderCommand(const RenderCommandBlitImage& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
    NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache)
{
    const GpuImageVk* srcImagePtr = gpuResourceMgr_.GetImage<GpuImageVk>(renderCmd.srcHandle);
    const GpuImageVk* dstImagePtr = gpuResourceMgr_.GetImage<GpuImageVk>(renderCmd.dstHandle);
    if (srcImagePtr && dstImagePtr) {
        const GpuImagePlatformDataVk& srcPlatImage = srcImagePtr->GetPlatformData();
        const GpuImagePlatformDataVk& dstPlatImage = (const GpuImagePlatformDataVk&)dstImagePtr->GetPlatformData();

        const ImageBlit& ib = renderCmd.imageBlit;
        const uint32_t srcLayerCount = (ib.srcSubresource.layerCount == PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)
                                           ? srcPlatImage.arrayLayers
                                           : ib.srcSubresource.layerCount;
        const uint32_t dstLayerCount = (ib.dstSubresource.layerCount == PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)
                                           ? dstPlatImage.arrayLayers
                                           : ib.dstSubresource.layerCount;

        const VkImageSubresourceLayers srcSubresourceLayers {
            (VkImageAspectFlags)ib.srcSubresource.imageAspectFlags, // aspectMask
            ib.srcSubresource.mipLevel,                             // mipLevel
            ib.srcSubresource.baseArrayLayer,                       // baseArrayLayer
            srcLayerCount,                                          // layerCount
        };
        const VkImageSubresourceLayers dstSubresourceLayers {
            (VkImageAspectFlags)ib.dstSubresource.imageAspectFlags, // aspectMask
            ib.dstSubresource.mipLevel,                             // mipLevel
            ib.dstSubresource.baseArrayLayer,                       // baseArrayLayer
            dstLayerCount,                                          // layerCount
        };

        const VkImageBlit imageBlit {
            srcSubresourceLayers, // srcSubresource

            { { (int32_t)ib.srcOffsets[0].width, (int32_t)ib.srcOffsets[0].height, (int32_t)ib.srcOffsets[0].depth },
                { (int32_t)ib.srcOffsets[1].width, (int32_t)ib.srcOffsets[1].height,
                    (int32_t)ib.srcOffsets[1].depth } }, // srcOffsets[2]

            dstSubresourceLayers, // dstSubresource

            { { (int32_t)ib.dstOffsets[0].width, (int32_t)ib.dstOffsets[0].height, (int32_t)ib.dstOffsets[0].depth },
                { (int32_t)ib.dstOffsets[1].width, (int32_t)ib.dstOffsets[1].height,
                    (int32_t)ib.dstOffsets[1].depth } }, // dstOffsets[2]
        };

        vkCmdBlitImage(cmdBuf.commandBuffer,         // commandBuffer
            srcPlatImage.image,                      // srcImage
            (VkImageLayout)renderCmd.srcImageLayout, // srcImageLayout,
            dstPlatImage.image,                      // dstImage
            (VkImageLayout)renderCmd.dstImageLayout, // dstImageLayout
            1,                                       // regionCount
            &imageBlit,                              // pRegions
            (VkFilter)renderCmd.filter);             // filter
    }
}

void RenderBackendVk::RenderCommand(const RenderCommandCopyBuffer& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
    NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache)
{
    const GpuBufferVk* srcGpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferVk>(renderCmd.srcHandle);
    const GpuBufferVk* dstGpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferVk>(renderCmd.dstHandle);

    PLUGIN_ASSERT(srcGpuBuffer);
    PLUGIN_ASSERT(dstGpuBuffer);

    if (srcGpuBuffer && dstGpuBuffer) {
        const VkBuffer srcBuffer = (srcGpuBuffer->GetPlatformData()).buffer;
        const VkBuffer dstBuffer = (dstGpuBuffer->GetPlatformData()).buffer;
        const VkBufferCopy bufferCopy {
            renderCmd.bufferCopy.srcOffset,
            renderCmd.bufferCopy.dstOffset,
            renderCmd.bufferCopy.size,
        };

        if (bufferCopy.size > 0) {
            vkCmdCopyBuffer(cmdBuf.commandBuffer, // commandBuffer
                srcBuffer,                        // srcBuffer
                dstBuffer,                        // dstBuffer
                1,                                // regionCount
                &bufferCopy);                     // pRegions
        }
    }
}

void RenderBackendVk::RenderCommand(const RenderCommandCopyBufferImage& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    const StateCache& stateCache)
{
    if (renderCmd.copyType == RenderCommandCopyBufferImage::CopyType::UNDEFINED) {
        PLUGIN_ASSERT(renderCmd.copyType != RenderCommandCopyBufferImage::CopyType::UNDEFINED);
        return;
    }

    const GpuBufferVk* gpuBuffer = nullptr;
    const GpuImageVk* gpuImage = nullptr;
    if (renderCmd.copyType == RenderCommandCopyBufferImage::CopyType::BUFFER_TO_IMAGE) {
        gpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferVk>(renderCmd.srcHandle);
        gpuImage = gpuResourceMgr_.GetImage<GpuImageVk>(renderCmd.dstHandle);
    } else {
        gpuImage = gpuResourceMgr_.GetImage<GpuImageVk>(renderCmd.srcHandle);
        gpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferVk>(renderCmd.dstHandle);
    }

    if (gpuBuffer && gpuImage) {
        const GpuImagePlatformDataVk& platImage = gpuImage->GetPlatformData();
        const BufferImageCopy& bufferImageCopy = renderCmd.bufferImageCopy;
        const ImageSubresourceLayers& subresourceLayer = bufferImageCopy.imageSubresource;
        const uint32_t layerCount = (subresourceLayer.layerCount == PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)
                                        ? platImage.arrayLayers
                                        : subresourceLayer.layerCount;
        const VkImageSubresourceLayers imageSubresourceLayer {
            (VkImageAspectFlags)subresourceLayer.imageAspectFlags,
            subresourceLayer.mipLevel,
            subresourceLayer.baseArrayLayer,
            layerCount,
        };
        const GpuImageDesc& imageDesc = gpuImage->GetDesc();
        // Math::min to force staying inside image
        const uint32_t mip = subresourceLayer.mipLevel;
        const VkExtent3D imageSize { imageDesc.width >> mip, imageDesc.height >> mip, imageDesc.depth };
        const Size3D& imageOffset = bufferImageCopy.imageOffset;
        const VkExtent3D imageExtent = {
            Math::min(imageSize.width - imageOffset.width, bufferImageCopy.imageExtent.width),
            Math::min(imageSize.height - imageOffset.height, bufferImageCopy.imageExtent.height),
            Math::min(imageSize.depth - imageOffset.depth, bufferImageCopy.imageExtent.depth),
        };
        const bool valid = (imageOffset.width < imageSize.width) && (imageOffset.height < imageSize.height) &&
                           (imageOffset.depth < imageSize.depth);
        const VkBufferImageCopy bufferImageCopyVk {
            bufferImageCopy.bufferOffset,
            bufferImageCopy.bufferRowLength,
            bufferImageCopy.bufferImageHeight,
            imageSubresourceLayer,
            { static_cast<int32_t>(imageOffset.width), static_cast<int32_t>(imageOffset.height),
                static_cast<int32_t>(imageOffset.depth) },
            imageExtent,
        };

        const VkBuffer buffer = (gpuBuffer->GetPlatformData()).buffer;
        const VkImage image = (gpuImage->GetPlatformData()).image;

        if (valid && renderCmd.copyType == RenderCommandCopyBufferImage::CopyType::BUFFER_TO_IMAGE) {
            vkCmdCopyBufferToImage(cmdBuf.commandBuffer,             // commandBuffer
                buffer,                                              // srcBuffer
                image,                                               // dstImage
                VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // dstImageLayout
                1,                                                   // regionCount
                &bufferImageCopyVk);                                 // pRegions
        } else if (valid && renderCmd.copyType == RenderCommandCopyBufferImage::CopyType::IMAGE_TO_BUFFER) {
            vkCmdCopyImageToBuffer(cmdBuf.commandBuffer,             // commandBuffer
                image,                                               // srcImage
                VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // srcImageLayout
                buffer,                                              // dstBuffer
                1,                                                   // regionCount
                &bufferImageCopyVk);                                 // pRegions
        }
    }
}

void RenderBackendVk::RenderCommand(const RenderCommandCopyImage& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
    NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache)
{
    const GpuImageVk* srcGpuImage = gpuResourceMgr_.GetImage<GpuImageVk>(renderCmd.srcHandle);
    const GpuImageVk* dstGpuImage = gpuResourceMgr_.GetImage<GpuImageVk>(renderCmd.dstHandle);
    if (srcGpuImage && dstGpuImage) {
        const ImageCopy& copy = renderCmd.imageCopy;
        const ImageSubresourceLayers& srcSubresourceLayer = copy.srcSubresource;
        const ImageSubresourceLayers& dstSubresourceLayer = copy.dstSubresource;

        const GpuImagePlatformDataVk& srcPlatImage = srcGpuImage->GetPlatformData();
        const GpuImagePlatformDataVk& dstPlatImage = dstGpuImage->GetPlatformData();
        const uint32_t srcLayerCount = (srcSubresourceLayer.layerCount == PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)
                                           ? srcPlatImage.arrayLayers
                                           : srcSubresourceLayer.layerCount;
        const uint32_t dstLayerCount = (dstSubresourceLayer.layerCount == PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)
                                           ? dstPlatImage.arrayLayers
                                           : dstSubresourceLayer.layerCount;

        const VkImageSubresourceLayers srcImageSubresourceLayer {
            (VkImageAspectFlags)srcSubresourceLayer.imageAspectFlags,
            srcSubresourceLayer.mipLevel,
            srcSubresourceLayer.baseArrayLayer,
            srcLayerCount,
        };
        const VkImageSubresourceLayers dstImageSubresourceLayer {
            (VkImageAspectFlags)dstSubresourceLayer.imageAspectFlags,
            dstSubresourceLayer.mipLevel,
            dstSubresourceLayer.baseArrayLayer,
            dstLayerCount,
        };

        const GpuImageDesc& srcDesc = srcGpuImage->GetDesc();
        const GpuImageDesc& dstDesc = dstGpuImage->GetDesc();

        VkExtent3D ext = { copy.extent.width, copy.extent.height, copy.extent.depth };
        ext.width = Math::min(ext.width, Math::min(srcDesc.width - copy.srcOffset.x, dstDesc.width - copy.dstOffset.x));
        ext.height =
            Math::min(ext.height, Math::min(srcDesc.height - copy.srcOffset.y, dstDesc.height - copy.dstOffset.y));
        ext.depth = Math::min(ext.depth, Math::min(srcDesc.depth - copy.srcOffset.z, dstDesc.depth - copy.dstOffset.z));

        const VkImageCopy imageCopyVk {
            srcImageSubresourceLayer,                                 // srcSubresource
            { copy.srcOffset.x, copy.srcOffset.y, copy.srcOffset.z }, // srcOffset
            dstImageSubresourceLayer,                                 // dstSubresource
            { copy.dstOffset.x, copy.dstOffset.y, copy.dstOffset.z }, // dstOffset
            ext,                                                      // extent
        };
        vkCmdCopyImage(cmdBuf.commandBuffer,                     // commandBuffer
            srcPlatImage.image,                                  // srcImage
            VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // srcImageLayout
            dstPlatImage.image,                                  // dstImage
            VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // dstImageLayout
            1,                                                   // regionCount
            &imageCopyVk);                                       // pRegions
    }
}

void RenderBackendVk::RenderCommand(const RenderCommandBarrierPoint& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
    NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache,
    const RenderBarrierList& rbl)
{
    if (!rbl.HasBarriers(renderCmd.barrierPointIndex)) {
        return;
    }

    const RenderBarrierList::BarrierPointBarriers* barrierPointBarriers =
        rbl.GetBarrierPointBarriers(renderCmd.barrierPointIndex);
    PLUGIN_ASSERT(barrierPointBarriers);
    if (!barrierPointBarriers) {
        return;
    }
    constexpr uint32_t maxBarrierCount { 8 };
    VkBufferMemoryBarrier bufferMemoryBarriers[maxBarrierCount];
    VkImageMemoryBarrier imageMemoryBarriers[maxBarrierCount];
    VkMemoryBarrier memoryBarriers[maxBarrierCount];

    // generally there is only single barrierListCount per barrier point
    // in situations with batched render passes there can be many
    // NOTE: all barrier lists could be patched to single vk command if needed
    // NOTE: Memory and pipeline barriers should be allowed in the front-end side
    const uint32_t barrierListCount = (uint32_t)barrierPointBarriers->barrierListCount;
    const RenderBarrierList::BarrierPointBarrierList* nextBarrierList = barrierPointBarriers->firstBarrierList;
#if (RENDER_VALIDATION_ENABLED == 1)
    uint32_t fullBarrierCount = 0u;
#endif
    for (uint32_t barrierListIndex = 0; barrierListIndex < barrierListCount; ++barrierListIndex) {
        if (nextBarrierList == nullptr) { // cannot be null, just a safety
            PLUGIN_ASSERT(false);
            return;
        }
        const RenderBarrierList::BarrierPointBarrierList& barrierListRef = *nextBarrierList;
        nextBarrierList = barrierListRef.nextBarrierPointBarrierList; // advance to next
        const uint32_t barrierCount = (uint32_t)barrierListRef.count;

        uint32_t bufferBarrierIdx = 0;
        uint32_t imageBarrierIdx = 0;
        uint32_t memoryBarrierIdx = 0;

        VkPipelineStageFlags srcPipelineStageMask { 0 };
        VkPipelineStageFlags dstPipelineStageMask { 0 };
        constexpr VkDependencyFlags dependencyFlags { VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT };

        for (uint32_t barrierIdx = 0; barrierIdx < barrierCount; ++barrierIdx) {
            const CommandBarrier& ref = barrierListRef.commandBarriers[barrierIdx];

            uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            if (ref.srcGpuQueue.type != ref.dstGpuQueue.type) {
                srcQueueFamilyIndex = deviceVk_.GetGpuQueue(ref.srcGpuQueue).queueInfo.queueFamilyIndex;
                dstQueueFamilyIndex = deviceVk_.GetGpuQueue(ref.dstGpuQueue).queueInfo.queueFamilyIndex;
            }

            const RenderHandle resourceHandle = ref.resourceHandle;
            const RenderHandleType handleType = RenderHandleUtil::GetHandleType(resourceHandle);

            PLUGIN_ASSERT((handleType == RenderHandleType::UNDEFINED) || (handleType == RenderHandleType::GPU_BUFFER) ||
                          (handleType == RenderHandleType::GPU_IMAGE));

            const VkAccessFlags srcAccessMask = (VkAccessFlags)(ref.src.accessFlags);
            const VkAccessFlags dstAccessMask = (VkAccessFlags)(ref.dst.accessFlags);

            srcPipelineStageMask |= (VkPipelineStageFlags)(ref.src.pipelineStageFlags);
            dstPipelineStageMask |= (VkPipelineStageFlags)(ref.dst.pipelineStageFlags);

            // NOTE: zero size buffer barriers allowed ATM
            if (handleType == RenderHandleType::GPU_BUFFER) {
                const GpuBufferVk* gpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferVk>(resourceHandle);
                PLUGIN_ASSERT(gpuBuffer);
                if (gpuBuffer) {
                    const GpuBufferPlatformDataVk& platBuffer = gpuBuffer->GetPlatformData();
                    // mapped currentByteOffset (dynamic ring buffer offset) taken into account
                    const VkDeviceSize offset = (VkDeviceSize)ref.dst.optionalByteOffset + platBuffer.currentByteOffset;
                    const VkDeviceSize size =
                        Math::min((VkDeviceSize)platBuffer.bindMemoryByteSize - ref.dst.optionalByteOffset,
                            (VkDeviceSize)ref.dst.optionalByteSize);
                    bufferMemoryBarriers[bufferBarrierIdx++] = {
                        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // sType
                        nullptr,                                 // pNext
                        srcAccessMask,                           // srcAccessMask
                        dstAccessMask,                           // dstAccessMask
                        srcQueueFamilyIndex,                     // srcQueueFamilyIndex
                        dstQueueFamilyIndex,                     // dstQueueFamilyIndex
                        platBuffer.buffer,                       // buffer
                        offset,                                  // offset
                        size,                                    // size
                    };
                }
            } else if (handleType == RenderHandleType::GPU_IMAGE) {
                const GpuImageVk* gpuImage = gpuResourceMgr_.GetImage<GpuImageVk>(resourceHandle);
                PLUGIN_ASSERT(gpuImage);
                if (gpuImage) {
                    const GpuImagePlatformDataVk& platImage = gpuImage->GetPlatformData();

                    const VkImageLayout srcImageLayout = (VkImageLayout)(ref.src.optionalImageLayout);
                    const VkImageLayout dstImageLayout = (VkImageLayout)(ref.dst.optionalImageLayout);

                    const VkImageAspectFlags imageAspectFlags =
                        (ref.dst.optionalImageSubresourceRange.imageAspectFlags == 0)
                            ? platImage.aspectFlags
                            : (VkImageAspectFlags)ref.dst.optionalImageSubresourceRange.imageAspectFlags;

                    const uint32_t levelCount = (ref.src.optionalImageSubresourceRange.levelCount ==
                                                    PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS)
                                                    ? platImage.mipLevels
                                                    : ref.src.optionalImageSubresourceRange.levelCount;
                    PLUGIN_ASSERT(levelCount <= platImage.mipLevels);

                    const uint32_t layerCount = (ref.src.optionalImageSubresourceRange.layerCount ==
                                                    PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)
                                                    ? platImage.arrayLayers
                                                    : ref.src.optionalImageSubresourceRange.layerCount;
                    PLUGIN_ASSERT(layerCount <= platImage.arrayLayers);

                    const VkImageSubresourceRange imageSubresourceRange {
                        imageAspectFlags,                                     // aspectMask
                        ref.src.optionalImageSubresourceRange.baseMipLevel,   // baseMipLevel
                        levelCount,                                           // levelCount
                        ref.src.optionalImageSubresourceRange.baseArrayLayer, // baseArrayLayer
                        layerCount,                                           // layerCount
                    };

                    imageMemoryBarriers[imageBarrierIdx++] = {
                        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // sType
                        nullptr,                                // pNext
                        srcAccessMask,                          // srcAccessMask
                        dstAccessMask,                          // dstAccessMask
                        srcImageLayout,                         // oldLayout
                        dstImageLayout,                         // newLayout
                        srcQueueFamilyIndex,                    // srcQueueFamilyIndex
                        dstQueueFamilyIndex,                    // dstQueueFamilyIndex
                        platImage.image,                        // image
                        imageSubresourceRange,                  // subresourceRange
                    };
                }
            } else {
                memoryBarriers[memoryBarrierIdx++] = {
                    VK_STRUCTURE_TYPE_MEMORY_BARRIER, // sType
                    nullptr,                          // pNext
                    srcAccessMask,                    // srcAccessMask
                    dstAccessMask,                    // dstAccessMask
                };
            }

            const bool hasBarriers = ((bufferBarrierIdx > 0) || (imageBarrierIdx > 0) || (memoryBarrierIdx > 0));
            const bool resetBarriers = ((bufferBarrierIdx >= maxBarrierCount) || (imageBarrierIdx >= maxBarrierCount) ||
                                           (memoryBarrierIdx >= maxBarrierCount) || (barrierIdx >= (barrierCount - 1)))
                                           ? true
                                           : false;

            if (hasBarriers && resetBarriers) {
#if (RENDER_VALIDATION_ENABLED == 1)
                fullBarrierCount += bufferBarrierIdx + imageBarrierIdx + memoryBarrierIdx;
#endif
                vkCmdPipelineBarrier(cmdBuf.commandBuffer, // commandBuffer
                    srcPipelineStageMask,                  // srcStageMask
                    dstPipelineStageMask,                  // dstStageMask
                    dependencyFlags,                       // dependencyFlags
                    memoryBarrierIdx,                      // memoryBarrierCount
                    memoryBarriers,                        // pMemoryBarriers
                    bufferBarrierIdx,                      // bufferMemoryBarrierCount
                    bufferMemoryBarriers,                  // pBufferMemoryBarriers
                    imageBarrierIdx,                       // imageMemoryBarrierCount
                    imageMemoryBarriers);                  // pImageMemoryBarriers

                bufferBarrierIdx = 0;
                imageBarrierIdx = 0;
                memoryBarrierIdx = 0;
            }
        }
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if (fullBarrierCount != barrierPointBarriers->fullCommandBarrierCount) {
        PLUGIN_LOG_ONCE_W("RenderBackendVk_RenderCommand_RenderCommandBarrierPoint",
            "RENDER_VALIDATION: barrier count does not match (front-end-count: %u, back-end-count: %u)",
            barrierPointBarriers->fullCommandBarrierCount, fullBarrierCount);
    }
#endif
}

void RenderBackendVk::RenderCommand(const RenderCommandUpdateDescriptorSets& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    const StateCache& stateCache, NodeContextDescriptorSetManager& ncdsm)
{
    // NOTE: update to update all at once
    NodeContextDescriptorSetManagerVk& aNcdsmVk = (NodeContextDescriptorSetManagerVk&)ncdsm;

    for (uint32_t descIdx = 0; descIdx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++descIdx) {
        const RenderHandle descHandle = renderCmd.descriptorSetHandles[descIdx];
        if (RenderHandleUtil::GetHandleType(descHandle) != RenderHandleType::DESCRIPTOR_SET) {
            continue;
        }

        // first update gpu descriptor indices
        ncdsm.UpdateDescriptorSetGpuHandle(descHandle);

        // actual vulkan descriptor set update
        const LowLevelDescriptorSetVk* descriptorSet = aNcdsmVk.GetDescriptorSet(descHandle);

        if (descriptorSet) {
            const DescriptorSetLayoutBindingResources bindingResources = ncdsm.GetCpuDescriptorSetData(descHandle);
#if (RENDER_VALIDATION_ENABLED == 1)
            // get descriptor counts
            const LowLevelDescriptorCountsVk& descriptorCounts = aNcdsmVk.GetLowLevelDescriptorCounts(descHandle);
            if ((uint32_t)bindingResources.bindings.size() > descriptorCounts.writeDescriptorCount) {
                PLUGIN_LOG_E("RENDER_VALIDATION: update descriptor set bindings exceed descriptor set bindings");
            }
#endif
            const uint32_t bindingCount = Math::min(
                (uint32_t)bindingResources.bindings.size(), PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT);
            const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;

            // max counts
            VkDescriptorBufferInfo descriptorBufferInfos[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT];
            VkDescriptorImageInfo descriptorImageInfos[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT];
            VkDescriptorImageInfo descriptorSamplerInfos[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT];
#if (RENDER_VULKAN_RT_ENABLED == 1)
            VkWriteDescriptorSetAccelerationStructureKHR
                descriptorAccelInfos[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT];
            uint32_t accelIndex = 0;
#endif
            VkWriteDescriptorSet writeDescriptorSet[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT];

            const auto& buffers = bindingResources.buffers;
            const auto& images = bindingResources.images;
            const auto& samplers = bindingResources.samplers;
            uint32_t bufferIndex = 0;
            uint32_t imageIndex = 0;
            uint32_t samplerIndex = 0;
            uint32_t writeBindIdx = 0;
            for (const auto& ref : buffers) {
                const uint32_t descriptorCount = ref.binding.descriptorCount;
                // skip, array bindings which are bound from first index, they have also descriptorCount 0
                if (descriptorCount == 0) {
                    continue;
                }
                const uint32_t arrayOffset = ref.arrayOffset;
                PLUGIN_ASSERT((arrayOffset + descriptorCount - 1) <= buffers.size());
                if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE) {
#if (RENDER_VULKAN_RT_ENABLED == 1)
                    for (uint32_t idx = 0; idx < descriptorCount; ++idx) {
                        // first is the ref, starting from 1 we use array offsets
                        const BindableBuffer& bRes =
                            (idx == 0) ? ref.resource : buffers[arrayOffset + idx - 1].resource;
                        const GpuAccelerationStructureVk* accelPtr =
                            gpuResourceMgr_.GetAccelerationStructure<GpuAccelerationStructureVk>(bRes.handle);
                        if (accelPtr) {
                            const GpuAccelerationStructurePlatformDataVk& platAccel = accelPtr->GetPlatformData();
                            descriptorAccelInfos[accelIndex + idx] = {
                                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR, // sType
                                nullptr,                                                           // pNext
                                descriptorCount,                  // accelerationStructureCount
                                &platAccel.accelerationStructure, // pAccelerationStructures
                            };
                        }
                    }
                    writeDescriptorSet[writeBindIdx++] = {
                        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,       // sType
                        &descriptorAccelInfos[accelIndex],            // pNext
                        descriptorSet->descriptorSet,                 // dstSet
                        ref.binding.binding,                          // dstBinding
                        0,                                            // dstArrayElement
                        descriptorCount,                              // descriptorCount
                        (VkDescriptorType)ref.binding.descriptorType, // descriptorType
                        nullptr,                                      // pImageInfo
                        nullptr,                                      // pBufferInfo
                        nullptr,                                      // pTexelBufferView
                    };
                    accelIndex += descriptorCount;
#endif
                } else {
                    for (uint32_t idx = 0; idx < descriptorCount; ++idx) {
                        // first is the ref, starting from 1 we use array offsets
                        const BindableBuffer& bRes =
                            (idx == 0) ? ref.resource : buffers[arrayOffset + idx - 1].resource;
                        const VkDeviceSize optionalByteOffset = (VkDeviceSize)bRes.byteOffset;
                        const GpuBufferVk* bufferPtr = gpuResourceMgr_.GetBuffer<GpuBufferVk>(bRes.handle);
                        if (bufferPtr) {
                            const GpuBufferPlatformDataVk& platBuffer = bufferPtr->GetPlatformData();
                            // takes into account dynamic ring buffers with mapping
                            const VkDeviceSize bufferMapByteOffset = (VkDeviceSize)platBuffer.currentByteOffset;
                            const VkDeviceSize byteOffset = bufferMapByteOffset + optionalByteOffset;
                            const VkDeviceSize bufferRange =
                                Math::min((VkDeviceSize)platBuffer.bindMemoryByteSize - optionalByteOffset,
                                    (VkDeviceSize)bRes.byteSize);
                            descriptorBufferInfos[bufferIndex + idx] = {
                                platBuffer.buffer, // buffer
                                byteOffset,        // offset
                                bufferRange,       // range
                            };
                        }
                    }
                    writeDescriptorSet[writeBindIdx++] = {
                        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,       // sType
                        nullptr,                                      // pNext
                        descriptorSet->descriptorSet,                 // dstSet
                        ref.binding.binding,                          // dstBinding
                        0,                                            // dstArrayElement
                        descriptorCount,                              // descriptorCount
                        (VkDescriptorType)ref.binding.descriptorType, // descriptorType
                        nullptr,                                      // pImageInfo
                        &descriptorBufferInfos[bufferIndex],          // pBufferInfo
                        nullptr,                                      // pTexelBufferView
                    };
                    bufferIndex += descriptorCount;
                }
            }
            for (const auto& ref : images) {
                const uint32_t descriptorCount = ref.binding.descriptorCount;
                // skip, array bindings which are bound from first index have also descriptorCount 0
                if (descriptorCount == 0) {
                    continue;
                }
                const VkDescriptorType descriptorType = (VkDescriptorType)ref.binding.descriptorType;
                const uint32_t arrayOffset = ref.arrayOffset;
                PLUGIN_ASSERT((arrayOffset + descriptorCount - 1) <= images.size());
                for (uint32_t idx = 0; idx < descriptorCount; ++idx) {
                    // first is the ref, starting from 1 we use array offsets
                    const BindableImage& bRes = (idx == 0) ? ref.resource : images[arrayOffset + idx - 1].resource;
                    const GpuImageVk* imagePtr = gpuResourceMgr_.GetImage<GpuImageVk>(bRes.handle);
                    if (imagePtr) {
                        VkSampler sampler = VK_NULL_HANDLE;
                        if (descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                            const GpuSamplerVk* samplerPtr =
                                gpuResourceMgr_.GetSampler<GpuSamplerVk>(bRes.samplerHandle);
                            if (samplerPtr) {
                                sampler = samplerPtr->GetPlatformData().sampler;
                            }
                        }
                        const GpuImagePlatformDataVk& platImage = imagePtr->GetPlatformData();
                        const GpuImagePlatformDataViewsVk& platImageViews = imagePtr->GetPlatformDataViews();
                        VkImageView imageView = platImage.imageView;
                        if ((bRes.mip != PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS) &&
                            (bRes.mip < platImageViews.mipImageViews.size())) {
                            imageView = platImageViews.mipImageViews[bRes.mip];
                        } else if ((bRes.layer != PipelineStateConstants::GPU_IMAGE_ALL_LAYERS) &&
                                   (bRes.layer < platImageViews.layerImageViews.size())) {
                            imageView = platImageViews.layerImageViews[bRes.layer];
                        }
                        descriptorImageInfos[imageIndex + idx] = {
                            sampler,                         // sampler
                            imageView,                       // imageView
                            (VkImageLayout)bRes.imageLayout, // imageLayout
                        };
                    }
                }
                writeDescriptorSet[writeBindIdx++] = {
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, // sType
                    nullptr,                                // pNext
                    descriptorSet->descriptorSet,           // dstSet
                    ref.binding.binding,                    // dstBinding
                    0,                                      // dstArrayElement
                    descriptorCount,                        // descriptorCount
                    descriptorType,                         // descriptorType
                    &descriptorImageInfos[imageIndex],      // pImageInfo
                    nullptr,                                // pBufferInfo
                    nullptr,                                // pTexelBufferView
                };
                imageIndex += descriptorCount;
            }
            for (const auto& ref : samplers) {
                const uint32_t descriptorCount = ref.binding.descriptorCount;
                // skip, array bindings which are bound from first index have also descriptorCount 0
                if (descriptorCount == 0) {
                    continue;
                }
                const uint32_t arrayOffset = ref.arrayOffset;
                PLUGIN_ASSERT((arrayOffset + descriptorCount - 1) <= samplers.size());
                for (uint32_t idx = 0; idx < descriptorCount; ++idx) {
                    // first is the ref, starting from 1 we use array offsets
                    const BindableSampler& bRes = (idx == 0) ? ref.resource : samplers[arrayOffset + idx - 1].resource;
                    const GpuSamplerVk* samplerPtr = gpuResourceMgr_.GetSampler<GpuSamplerVk>(bRes.handle);
                    if (samplerPtr) {
                        const GpuSamplerPlatformDataVk& platSampler = samplerPtr->GetPlatformData();
                        descriptorSamplerInfos[samplerIndex + idx] = {
                            platSampler.sampler,      // sampler
                            VK_NULL_HANDLE,           // imageView
                            VK_IMAGE_LAYOUT_UNDEFINED // imageLayout
                        };
                    }
                }
                writeDescriptorSet[writeBindIdx++] = {
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,       // sType
                    nullptr,                                      // pNext
                    descriptorSet->descriptorSet,                 // dstSet
                    ref.binding.binding,                          // dstBinding
                    0,                                            // dstArrayElement
                    descriptorCount,                              // descriptorCount
                    (VkDescriptorType)ref.binding.descriptorType, // descriptorType
                    &descriptorSamplerInfos[samplerIndex],        // pImageInfo
                    nullptr,                                      // pBufferInfo
                    nullptr,                                      // pTexelBufferView
                };
                samplerIndex += descriptorCount;
            }
            vkUpdateDescriptorSets(device, // device
                bindingCount,              // descriptorWriteCount
                writeDescriptorSet,        // pDescriptorWrites
                0,                         // descriptorCopyCount
                nullptr);                  // pDescriptorCopies
#if (RENDER_PERF_ENABLED == 1)
            stateCache.perfCounters.updateDescriptorSetCount++;
#endif
        }
    }
}

void RenderBackendVk::RenderCommand(const RenderCommandBindDescriptorSets& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    StateCache& stateCache, NodeContextDescriptorSetManager& aNcdsm)
{
    const NodeContextDescriptorSetManagerVk& aNcdsmVk = (NodeContextDescriptorSetManagerVk&)aNcdsm;

    PLUGIN_ASSERT(stateCache.psoHandle == renderCmd.psoHandle);
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(stateCache.psoHandle);
    const VkPipelineBindPoint pipelineBindPoint = (handleType == RenderHandleType::COMPUTE_PSO)
                                                      ? VK_PIPELINE_BIND_POINT_COMPUTE
                                                      : VK_PIPELINE_BIND_POINT_GRAPHICS;
    const VkPipelineLayout pipelineLayout = stateCache.pipelineLayout;

    const bool valid = (pipelineLayout != VK_NULL_HANDLE) ? true : false;
    PLUGIN_ASSERT(valid); // render command list should enforce this
    const uint32_t firstSet = renderCmd.firstSet;
    const uint32_t setCount = renderCmd.setCount;
    if (valid && (firstSet + setCount <= PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) && (setCount > 0)) {
        uint32_t combinedDynamicOffsetCount = 0;
        uint32_t dynamicOffsetDescriptorSetIndices = 0;
        uint64_t priorStatePipelineDescSetHash = stateCache.pipelineDescSetHash;

        VkDescriptorSet descriptorSets[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT];
        const uint32_t firstPlusCount = firstSet + setCount;
        for (uint32_t idx = firstSet; idx < firstPlusCount; ++idx) {
            const RenderHandle descriptorSetHandle = renderCmd.descriptorSetHandles[idx];
            if (RenderHandleUtil::GetHandleType(descriptorSetHandle) == RenderHandleType::DESCRIPTOR_SET) {
                const uint32_t dynamicDescriptorCount = aNcdsm.GetDynamicOffsetDescriptorCount(descriptorSetHandle);
                dynamicOffsetDescriptorSetIndices |= (dynamicDescriptorCount > 0) ? (1 << idx) : 0;
                combinedDynamicOffsetCount += dynamicDescriptorCount;

                const LowLevelDescriptorSetVk* descriptorSet = aNcdsmVk.GetDescriptorSet(descriptorSetHandle);
                if (descriptorSet) {
                    PLUGIN_ASSERT(descriptorSet->descriptorSet);
                    descriptorSets[idx] = descriptorSet->descriptorSet;
                    // update, copy to state cache
                    PLUGIN_ASSERT(descriptorSet->descriptorSetLayout);
                    stateCache.lowLevelPipelineLayoutData.descriptorSetLayouts[idx] = *descriptorSet;
                    const uint32_t currShift = (idx * 16u);
                    const uint64_t oldOutMask = (~(static_cast<uint64_t>(0xffff) << currShift));
                    uint64_t currHash = stateCache.pipelineDescSetHash & oldOutMask;
                    stateCache.pipelineDescSetHash = currHash | (descriptorSet->immutableSamplerBitmask);
                }
            }
        }

        PLUGIN_ASSERT(combinedDynamicOffsetCount <= PipelineLayoutConstants::MAX_DYNAMIC_DESCRIPTOR_OFFSET_COUNT);
        uint32_t dynamicOffsets[PipelineLayoutConstants::MAX_DYNAMIC_DESCRIPTOR_OFFSET_COUNT];
        uint32_t dynamicOffsetIdx = 0;
        const uint32_t userDynamicOffsetCount = renderCmd.dynamicOffsetCount;
        for (uint32_t idx = firstSet; idx < firstPlusCount; ++idx) {
            if ((1 << idx) & dynamicOffsetDescriptorSetIndices) {
                const RenderHandle descriptorSetHandle = renderCmd.descriptorSetHandles[idx];
                const DynamicOffsetDescriptors dod = aNcdsm.GetDynamicOffsetDescriptors(descriptorSetHandle);
                const size_t dodResCount = dod.resources.size();
                for (size_t dodIdx = 0; dodIdx < dodResCount; ++dodIdx) {
#if (RENDER_VALIDATION_ENABLED == 1)
                    const GpuBuffer* gpuBuffer = gpuResourceMgr_.GetBuffer(dod.resources[dodIdx]);
                    PLUGIN_UNUSED(gpuBuffer);
                    PLUGIN_ASSERT(gpuBuffer);
#endif
                    uint32_t byteOffset = 0;
                    if (dynamicOffsetIdx < userDynamicOffsetCount) {
                        byteOffset += renderCmd.dynamicOffsets[dynamicOffsetIdx];
                    }
                    dynamicOffsets[dynamicOffsetIdx++] = byteOffset;
                }
            }
        }

        if (priorStatePipelineDescSetHash == stateCache.pipelineDescSetHash) {
            vkCmdBindDescriptorSets(cmdBuf.commandBuffer, // commandBuffer
                pipelineBindPoint,                        // pipelineBindPoint
                pipelineLayout,                           // layout
                firstSet,                                 // firstSet
                setCount,                                 // descriptorSetCount
                &descriptorSets[firstSet],                // pDescriptorSets
                dynamicOffsetIdx,                         // dynamicOffsetCount
                dynamicOffsets);                          // pDynamicOffsets
#if (RENDER_PERF_ENABLED == 1)
            stateCache.perfCounters.bindDescriptorSetCount++;
#endif
        } else {
            // possible pso re-creation and bind of these sets to the new pso
            const RenderCommandBindPipeline renderCmdBindPipeline { stateCache.psoHandle,
                (PipelineBindPoint)pipelineBindPoint };
            RenderCommand(renderCmdBindPipeline, cmdBuf, psoMgr, poolMgr, stateCache);
            RenderCommand(renderCmd, cmdBuf, psoMgr, poolMgr, stateCache, aNcdsm);
        }
    }
}

void RenderBackendVk::RenderCommand(const RenderCommandPushConstant& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
    NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache)
{
    PLUGIN_ASSERT(renderCmd.pushConstant.byteSize > 0);
    PLUGIN_ASSERT(renderCmd.data);

    PLUGIN_ASSERT(stateCache.psoHandle == renderCmd.psoHandle);
    const VkPipelineLayout pipelineLayout = stateCache.pipelineLayout;

    const bool valid = ((pipelineLayout != VK_NULL_HANDLE) && (renderCmd.pushConstant.byteSize > 0)) ? true : false;
    PLUGIN_ASSERT(valid);

    if (valid) {
        const auto shaderStageFlags = static_cast<VkShaderStageFlags>(renderCmd.pushConstant.shaderStageFlags);
        vkCmdPushConstants(cmdBuf.commandBuffer, // commandBuffer
            pipelineLayout,                      // layout
            shaderStageFlags,                    // stageFlags
            0,                                   // offset
            renderCmd.pushConstant.byteSize,     // size
            static_cast<void*>(renderCmd.data)); // pValues
    }
}

void RenderBackendVk::RenderCommand(const RenderCommandBuildAccelerationStructure& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    const StateCache& stateCache)
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
    // NOTE: missing
    const GpuAccelerationStructureVk* dst =
        gpuResourceMgr_.GetAccelerationStructure<const GpuAccelerationStructureVk>(renderCmd.dstAccelerationStructure);
    const GpuBufferVk* scratchBuffer = gpuResourceMgr_.GetBuffer<const GpuBufferVk>(renderCmd.scratchBuffer);
    if (dst && scratchBuffer) {
        const DevicePlatformDataVk& devicePlat = deviceVk_.GetPlatformDataVk();
        const VkDevice device = devicePlat.device;

        const GpuAccelerationStructurePlatformDataVk& dstPlat = dst->GetPlatformData();
        const VkAccelerationStructureKHR dstAs = dstPlat.accelerationStructure;

        // scratch data with user offset
        const VkDeviceAddress scratchData { GetBufferDeviceAddress(device, scratchBuffer->GetPlatformData().buffer) +
                                            VkDeviceSize(renderCmd.scratchOffset) };

        const size_t arraySize =
            renderCmd.trianglesView.size() + renderCmd.aabbsView.size() + renderCmd.instancesView.size();
        vector<VkAccelerationStructureGeometryKHR> geometryData(arraySize);
        vector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos(arraySize);

        size_t arrayIndex = 0;
        for (const auto& trianglesRef : renderCmd.trianglesView) {
            geometryData[arrayIndex] = VkAccelerationStructureGeometryKHR {
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR, // sType
                nullptr,                                               // pNext
                VkGeometryTypeKHR::VK_GEOMETRY_TYPE_TRIANGLES_KHR,     // geometryType
                {},                                                    // geometry;
                0,                                                     // flags
            };
            uint32_t primitiveCount = 0;
            const GpuBufferVk* vb = gpuResourceMgr_.GetBuffer<const GpuBufferVk>(trianglesRef.vertexData.handle);
            const GpuBufferVk* ib = gpuResourceMgr_.GetBuffer<const GpuBufferVk>(trianglesRef.indexData.handle);
            if (vb && ib) {
                const VkDeviceOrHostAddressConstKHR vertexData { GetBufferDeviceAddress(
                    device, vb->GetPlatformData().buffer) };
                const VkDeviceOrHostAddressConstKHR indexData { GetBufferDeviceAddress(
                    device, ib->GetPlatformData().buffer) };
                VkDeviceOrHostAddressConstKHR transformData {};
                if (RenderHandleUtil::IsValid(trianglesRef.transformData.handle)) {
                    if (const GpuBufferVk* tr =
                            gpuResourceMgr_.GetBuffer<const GpuBufferVk>(trianglesRef.transformData.handle);
                        tr) {
                        transformData.deviceAddress = { GetBufferDeviceAddress(device, ib->GetPlatformData().buffer) };
                    }
                }
                primitiveCount = trianglesRef.info.indexCount / 3u; // triangles

                geometryData[arrayIndex].flags = VkGeometryFlagsKHR(renderCmd.flags);
                geometryData[arrayIndex].geometry.triangles = VkAccelerationStructureGeometryTrianglesDataKHR {
                    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR, // sType
                    nullptr,                                                              // pNext
                    VkFormat(trianglesRef.info.vertexFormat),                             // vertexFormat
                    vertexData,                                                           // vertexData
                    VkDeviceSize(trianglesRef.info.vertexStride),                         // vertexStride
                    trianglesRef.info.maxVertex,                                          // maxVertex
                    VkIndexType(trianglesRef.info.indexType),                             // indexType
                    indexData,                                                            // indexData
                    transformData,                                                        // transformData
                };
            }
            buildRangeInfos[arrayIndex] = {
                primitiveCount, // primitiveCount
                0u,             // primitiveOffset
                0u,             // firstVertex
                0u,             // transformOffset
            };
            arrayIndex++;
        }
        for (const auto& aabbsRef : renderCmd.aabbsView) {
            geometryData[arrayIndex] = VkAccelerationStructureGeometryKHR {
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR, // sType
                nullptr,                                               // pNext
                VkGeometryTypeKHR::VK_GEOMETRY_TYPE_AABBS_KHR,         // geometryType
                {},                                                    // geometry;
                0,                                                     // flags
            };
            VkDeviceOrHostAddressConstKHR deviceAddress { 0 };
            const GpuBufferVk* iPtr = gpuResourceMgr_.GetBuffer<const GpuBufferVk>(aabbsRef.data.handle);
            if (iPtr) {
                deviceAddress.deviceAddress = GetBufferDeviceAddress(device, iPtr->GetPlatformData().buffer);
            }
            geometryData[arrayIndex].flags = VkGeometryFlagsKHR(renderCmd.flags);
            geometryData[arrayIndex].geometry.aabbs = VkAccelerationStructureGeometryAabbsDataKHR {
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR, // sType
                nullptr,                                                          // pNext
                deviceAddress,                                                    // data
                aabbsRef.info.stride,                                             // stride
            };
            buildRangeInfos[arrayIndex] = {
                1u, // primitiveCount
                0u, // primitiveOffset
                0u, // firstVertex
                0u, // transformOffset
            };
            arrayIndex++;
        }
        for (const auto& instancesRef : renderCmd.instancesView) {
            geometryData[arrayIndex] = VkAccelerationStructureGeometryKHR {
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR, // sType
                nullptr,                                               // pNext
                VkGeometryTypeKHR::VK_GEOMETRY_TYPE_INSTANCES_KHR,     // geometryType
                {},                                                    // geometry;
                0,                                                     // flags
            };
            VkDeviceOrHostAddressConstKHR deviceAddress { 0 };
            const GpuBufferVk* iPtr = gpuResourceMgr_.GetBuffer<const GpuBufferVk>(instancesRef.data.handle);
            if (iPtr) {
                deviceAddress.deviceAddress = GetBufferDeviceAddress(device, iPtr->GetPlatformData().buffer);
            }
            geometryData[arrayIndex].flags = VkGeometryFlagsKHR(renderCmd.flags);
            geometryData[arrayIndex].geometry.instances = VkAccelerationStructureGeometryInstancesDataKHR {
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR, // sType
                nullptr,                                                              // pNext
                instancesRef.info.arrayOfPointers,                                    // arrayOfPointers
                deviceAddress,                                                        // data
            };
            buildRangeInfos[arrayIndex] = {
                1u, // primitiveCount
                0u, // primitiveOffset
                0u, // firstVertex
                0u, // transformOffset
            };
            arrayIndex++;
        }

        const VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo {
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR, // sType
            nullptr,                                                          // pNext
            VkAccelerationStructureTypeKHR(renderCmd.type),                   // type
            VkBuildAccelerationStructureFlagsKHR(renderCmd.flags),            // flags
            VkBuildAccelerationStructureModeKHR(renderCmd.mode),              // mode
            VK_NULL_HANDLE,                                                   // srcAccelerationStructure
            dstAs,                                                            // dstAccelerationStructure
            uint32_t(arrayIndex),                                             // geometryCount
            geometryData.data(),                                              // pGeometries
            nullptr,                                                          // ppGeometries
            scratchData,                                                      // scratchData
        };

        vector<const VkAccelerationStructureBuildRangeInfoKHR*> buildRangeInfosPtr(arrayIndex);
        for (size_t idx = 0; idx < buildRangeInfosPtr.size(); ++idx) {
            buildRangeInfosPtr[idx] = &buildRangeInfos[idx];
        }
        const DeviceVk::ExtFunctions& extFunctions = deviceVk_.GetExtFunctions();
        if (extFunctions.vkCmdBuildAccelerationStructuresKHR) {
            extFunctions.vkCmdBuildAccelerationStructuresKHR(cmdBuf.commandBuffer, // commandBuffer
                1u,                                                                // infoCount
                &buildGeometryInfo,                                                // pInfos
                buildRangeInfosPtr.data());                                        // ppBuildRangeInfos
        }
    }
#endif
}

void RenderBackendVk::RenderCommand(const RenderCommandDynamicStateViewport& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    const StateCache& stateCache)
{
    const ViewportDesc& vd = renderCmd.viewportDesc;

    const VkViewport viewport {
        vd.x,        // x
        vd.y,        // y
        vd.width,    // width
        vd.height,   // height
        vd.minDepth, // minDepth
        vd.maxDepth, // maxDepth
    };

    vkCmdSetViewport(cmdBuf.commandBuffer, // commandBuffer
        0,                                 // firstViewport
        1,                                 // viewportCount
        &viewport);                        // pViewports
}

void RenderBackendVk::RenderCommand(const RenderCommandDynamicStateScissor& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    const StateCache& stateCache)
{
    const ScissorDesc& sd = renderCmd.scissorDesc;

    const VkRect2D scissor {
        { sd.offsetX, sd.offsetY },          // offset
        { sd.extentWidth, sd.extentHeight }, // extent
    };

    vkCmdSetScissor(cmdBuf.commandBuffer, // commandBuffer
        0,                                // firstScissor
        1,                                // scissorCount
        &scissor);                        // pScissors
}

void RenderBackendVk::RenderCommand(const RenderCommandDynamicStateLineWidth& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    const StateCache& stateCache)
{
    vkCmdSetLineWidth(cmdBuf.commandBuffer, // commandBuffer
        renderCmd.lineWidth);               // lineWidth
}

void RenderBackendVk::RenderCommand(const RenderCommandDynamicStateDepthBias& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    const StateCache& stateCache)
{
    vkCmdSetDepthBias(cmdBuf.commandBuffer, // commandBuffer
        renderCmd.depthBiasConstantFactor,  // depthBiasConstantFactor
        renderCmd.depthBiasClamp,           // depthBiasClamp
        renderCmd.depthBiasSlopeFactor);    // depthBiasSlopeFactor
}

void RenderBackendVk::RenderCommand(const RenderCommandDynamicStateBlendConstants& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    const StateCache& stateCache)
{
    vkCmdSetBlendConstants(cmdBuf.commandBuffer, // commandBuffer
        renderCmd.blendConstants);               // blendConstants[4]
}

void RenderBackendVk::RenderCommand(const RenderCommandDynamicStateDepthBounds& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    const StateCache& stateCache)
{
    vkCmdSetDepthBounds(cmdBuf.commandBuffer, // commandBuffer
        renderCmd.minDepthBounds,             // minDepthBounds
        renderCmd.maxDepthBounds);            // maxDepthBounds
}

void RenderBackendVk::RenderCommand(const RenderCommandDynamicStateStencil& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    const StateCache& stateCache)
{
    const VkStencilFaceFlags stencilFaceMask = (VkStencilFaceFlags)renderCmd.faceMask;

    if (renderCmd.dynamicState == StencilDynamicState::COMPARE_MASK) {
        vkCmdSetStencilCompareMask(cmdBuf.commandBuffer, // commandBuffer
            stencilFaceMask,                             // faceMask
            renderCmd.mask);                             // compareMask
    } else if (renderCmd.dynamicState == StencilDynamicState::WRITE_MASK) {
        vkCmdSetStencilWriteMask(cmdBuf.commandBuffer, // commandBuffer
            stencilFaceMask,                           // faceMask
            renderCmd.mask);                           // writeMask
    } else if (renderCmd.dynamicState == StencilDynamicState::REFERENCE) {
        vkCmdSetStencilReference(cmdBuf.commandBuffer, // commandBuffer
            stencilFaceMask,                           // faceMask
            renderCmd.mask);                           // reference
    }
}

void RenderBackendVk::RenderCommand(const RenderCommandExecuteBackendFramePosition& renderCmd,
    const LowLevelCommandBufferVk& cmdBuf, NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr,
    const StateCache& stateCache)
{
    if (stateCache.backendNode) {
        const RenderBackendRecordingStateVk recordingState = {
            {},
            cmdBuf.commandBuffer,                              // commandBuffer
            stateCache.lowLevelRenderPassData.renderPass,      // renderPass
            stateCache.lowLevelRenderPassData.framebuffer,     // framebuffer
            stateCache.lowLevelRenderPassData.framebufferSize, // framebufferSize
            stateCache.lowLevelRenderPassData.subpassIndex,    // subpassIndex
            stateCache.pipelineLayout,                         // pipelineLayout
        };
        const ILowLevelDeviceVk& lowLevelDevice = static_cast<ILowLevelDeviceVk&>(deviceVk_.GetLowLevelDevice());
        stateCache.backendNode->ExecuteBackendFrame(lowLevelDevice, recordingState);
    }
}

void RenderBackendVk::RenderCommand(const RenderCommandWriteTimestamp& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
    NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache)
{
    PLUGIN_ASSERT_MSG(false, "not implemented");

    const VkPipelineStageFlagBits pipelineStageFlagBits = (VkPipelineStageFlagBits)renderCmd.pipelineStageFlagBits;
    const uint32_t queryIndex = renderCmd.queryIndex;
    VkQueryPool queryPool = VK_NULL_HANDLE;

    vkCmdResetQueryPool(cmdBuf.commandBuffer, // commandBuffer
        queryPool,                            // queryPool
        queryIndex,                           // firstQuery
        1);                                   // queryCount

    vkCmdWriteTimestamp(cmdBuf.commandBuffer, // commandBuffer
        pipelineStageFlagBits,                // pipelineStage
        queryPool,                            // queryPool
        queryIndex);                          // query
}

void RenderBackendVk::RenderPresentationLayout(const LowLevelCommandBufferVk& cmdBuf)
{
    PLUGIN_ASSERT(presentationInfo_.presentationLayoutChangeNeeded);
    PLUGIN_ASSERT(presentationInfo_.imageLayout != ImageLayout::CORE_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    const GpuResourceState& state = presentationInfo_.renderGraphProcessedState;
    const VkAccessFlags srcAccessMask = (VkAccessFlags)state.accessFlags;
    const VkAccessFlags dstAccessMask = (VkAccessFlags)VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
    const VkPipelineStageFlags srcStageMask =
        ((VkPipelineStageFlags)state.pipelineStageFlags) | (VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    const VkPipelineStageFlags dstStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT;
    const VkImageLayout oldLayout = (VkImageLayout)presentationInfo_.imageLayout;
    const VkImageLayout newLayout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // NOTE: queue is not currently checked (should be in the same queue as last time used)
    constexpr uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    constexpr uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    constexpr VkDependencyFlags dependencyFlags { VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT };
    constexpr VkImageSubresourceRange imageSubresourceRange {
        VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
        0,                                                // baseMipLevel
        1,                                                // levelCount
        0,                                                // baseArrayLayer
        1,                                                // layerCount
    };

    const VkImageMemoryBarrier imageMemoryBarrier {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // sType
        nullptr,                                // pNext
        srcAccessMask,                          // srcAccessMask
        dstAccessMask,                          // dstAccessMask
        oldLayout,                              // oldLayout
        newLayout,                              // newLayout
        srcQueueFamilyIndex,                    // srcQueueFamilyIndex
        dstQueueFamilyIndex,                    // dstQueueFamilyIndex
        presentationInfo_.swapchainImage,       // image
        imageSubresourceRange,                  // subresourceRange
    };

    vkCmdPipelineBarrier(cmdBuf.commandBuffer, // commandBuffer
        srcStageMask,                          // srcStageMask
        dstStageMask,                          // dstStageMask
        dependencyFlags,                       // dependencyFlags
        0,                                     // memoryBarrierCount
        nullptr,                               // pMemoryBarriers
        0,                                     // bufferMemoryBarrierCount
        nullptr,                               // pBufferMemoryBarriers
        1,                                     // imageMemoryBarrierCount
        &imageMemoryBarrier);                  // pImageMemoryBarriers

    presentationInfo_.presentationLayoutChangeNeeded = false;
    presentationInfo_.imageLayout = ImageLayout::CORE_IMAGE_LAYOUT_PRESENT_SRC_KHR;
}

#if (RENDER_PERF_ENABLED == 1)

void RenderBackendVk::StartFrameTimers(RenderCommandFrameData& renderCommandFrameData)
{
    for (const auto& renderCommandContext : renderCommandFrameData.renderCommandContexts) {
        const string_view& debugName = renderCommandContext.debugName;
        if (timers_.count(debugName) == 0) { // new timers
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
            PerfDataSet& perfDataSet = timers_[debugName];
            constexpr GpuQueryDesc desc { QueryType::CORE_QUERY_TYPE_TIMESTAMP, 0 };
            perfDataSet.gpuHandle = gpuQueryMgr_->Create(debugName, CreateGpuQueryVk(device_, desc));
            constexpr uint32_t singleQueryByteSize = sizeof(uint64_t) * TIME_STAMP_PER_GPU_QUERY;
            perfDataSet.gpuBufferOffset = (uint32_t)timers_.size() * singleQueryByteSize;
#else
            timers_.insert({ debugName, {} });
#endif
        }
    }

#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
    perfGpuTimerData_.mappedData = perfGpuTimerData_.gpuBuffer->Map();
    perfGpuTimerData_.currentOffset =
        (perfGpuTimerData_.currentOffset + perfGpuTimerData_.frameByteSize) % perfGpuTimerData_.fullByteSize;
#endif
}

void RenderBackendVk::EndFrameTimers()
{
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
    perfGpuTimerData_.gpuBuffer->Unmap();
#endif
    if (IPerformanceDataManagerFactory* globalPerfData =
            GetInstance<IPerformanceDataManagerFactory>(CORE_NS::UID_PERFORMANCE_FACTORY);
        globalPerfData) {
        IPerformanceDataManager* perfData = globalPerfData->Get("Renderer");
        perfData->UpdateData("RenderBackend", "Full_Cpu", commonCpuTimers_.full.GetMicroseconds());
        perfData->UpdateData("RenderBackend", "Acquire_Cpu", commonCpuTimers_.acquire.GetMicroseconds());
        perfData->UpdateData("RenderBackend", "Execute_Cpu", commonCpuTimers_.execute.GetMicroseconds());
        perfData->UpdateData("RenderBackend", "Submit_Cpu", commonCpuTimers_.submit.GetMicroseconds());
        perfData->UpdateData("RenderBackend", "Present_Cpu", commonCpuTimers_.present.GetMicroseconds());
    }
}

void RenderBackendVk::WritePerfTimeStamp(const LowLevelCommandBufferVk& cmdBuf, const string_view name,
    const uint32_t queryIndex, const VkPipelineStageFlagBits stageFlagBits)
{
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
    PLUGIN_ASSERT(timers_.count(name) == 1);
    const PerfDataSet* perfDataSet = &timers_[name];

    const GpuQuery* gpuQuery = gpuQueryMgr_->Get(perfDataSet->gpuHandle);
    PLUGIN_ASSERT(gpuQuery);

    const auto& platData = static_cast<const GpuQueryPlatformDataVk&>(gpuQuery->GetPlatformData());
    PLUGIN_ASSERT(platData.queryPool);

    vkCmdResetQueryPool(cmdBuf.commandBuffer, // commandBuffer
        platData.queryPool,                   // queryPool
        queryIndex,                           // firstQuery
        1);                                   // queryCount

    vkCmdWriteTimestamp(cmdBuf.commandBuffer, // commandBuffer,
        stageFlagBits,                        // pipelineStage,
        platData.queryPool,                   // queryPool,
        queryIndex);                          // query
#endif
}

namespace {
void UpdatePerfCounters(IPerformanceDataManager& perfData, const string_view name, const PerfCounters& perfCounters)
{
    perfData.UpdateData(name, "Backend_Count_Triangle", perfCounters.triangleCount);
    perfData.UpdateData(name, "Backend_Count_InstanceCount", perfCounters.instanceCount);
    perfData.UpdateData(name, "Backend_Count_Draw", perfCounters.drawCount);
    perfData.UpdateData(name, "Backend_Count_DrawIndirect", perfCounters.drawIndirectCount);
    perfData.UpdateData(name, "Backend_Count_Dispatch", perfCounters.dispatchCount);
    perfData.UpdateData(name, "Backend_Count_DispatchIndirect", perfCounters.dispatchIndirectCount);
    perfData.UpdateData(name, "Backend_Count_BindPipeline", perfCounters.bindPipelineCount);
    perfData.UpdateData(name, "Backend_Count_RenderPass", perfCounters.renderPassCount);
    perfData.UpdateData(name, "Backend_Count_UpdateDescriptorSet", perfCounters.updateDescriptorSetCount);
    perfData.UpdateData(name, "Backend_Count_BindDescriptorSet", perfCounters.bindDescriptorSetCount);
}
} // namespace

void RenderBackendVk::CopyPerfTimeStamp(
    const LowLevelCommandBufferVk& cmdBuf, const string_view name, const StateCache& cache)
{
    PLUGIN_ASSERT(timers_.count(name) == 1);
    const PerfDataSet* perfDataSet = &timers_[name];

#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
    // take data from earlier queries to cpu
    // and copy in from query to gpu buffer
    const uint32_t currentFrameByteOffset = perfGpuTimerData_.currentOffset + perfDataSet->gpuBufferOffset;
    int64_t gpuMicroSeconds = 0;
    {
        auto data = static_cast<const uint8_t*>(perfGpuTimerData_.mappedData);
        auto currentData = reinterpret_cast<const uint64_t*>(data + currentFrameByteOffset);

        const uint64_t startStamp = *currentData;
        const uint64_t endStamp = *(currentData + 1);

        const double timestampPeriod =
            static_cast<double>(static_cast<const DevicePlatformDataVk&>(device_.GetPlatformData())
                                    .physicalDeviceProperties.physicalDeviceProperties.limits.timestampPeriod);
        constexpr int64_t nanosToMicrosDivisor { 1000 };
        gpuMicroSeconds = static_cast<int64_t>((endStamp - startStamp) * timestampPeriod) / nanosToMicrosDivisor;
        constexpr int64_t maxValidMicroSecondValue { 4294967295 };
        if (gpuMicroSeconds > maxValidMicroSecondValue) {
            gpuMicroSeconds = 0;
        }
    }
#endif
    const int64_t cpuMicroSeconds = perfDataSet->cpuTimer.GetMicroseconds();

    if (IPerformanceDataManagerFactory* globalPerfData =
            GetInstance<IPerformanceDataManagerFactory>(CORE_NS::UID_PERFORMANCE_FACTORY);
        globalPerfData) {
        IPerformanceDataManager* perfData = globalPerfData->Get("RenderNode");

        perfData->UpdateData(name, "Backend_Cpu", cpuMicroSeconds);
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
        perfData->UpdateData(name, "Backend_Gpu", gpuMicroSeconds);

        const GpuQuery* gpuQuery = gpuQueryMgr_->Get(perfDataSet->gpuHandle);
        PLUGIN_ASSERT(gpuQuery);

        const auto& platData = static_cast<const GpuQueryPlatformDataVk&>(gpuQuery->GetPlatformData());

        const GpuBufferVk* gpuBuffer = static_cast<GpuBufferVk*>(perfGpuTimerData_.gpuBuffer.get());
        PLUGIN_ASSERT(gpuBuffer);
        const GpuBufferPlatformDataVk& platBuffer = gpuBuffer->GetPlatformData();

        constexpr uint32_t queryCount = 2;
        constexpr VkDeviceSize queryStride = sizeof(uint64_t);
        constexpr VkQueryResultFlags queryResultFlags =
            VkQueryResultFlagBits::VK_QUERY_RESULT_64_BIT | VkQueryResultFlagBits::VK_QUERY_RESULT_WAIT_BIT;

        vkCmdCopyQueryPoolResults(cmdBuf.commandBuffer, // commandBuffer
            platData.queryPool,                         // queryPool
            0,                                          // firstQuery
            queryCount,                                 // queryCount
            platBuffer.buffer,                          // dstBuffer
            currentFrameByteOffset,                     // dstOffset
            queryStride,                                // stride
            queryResultFlags);                          // flags
#endif
        UpdatePerfCounters(*perfData, name, cache.perfCounters);
    }
}

#endif
RENDER_END_NAMESPACE()
