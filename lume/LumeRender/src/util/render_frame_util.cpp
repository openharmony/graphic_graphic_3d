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

#include "util/render_frame_util.h"

#include <limits>

#include <base/containers/fixed_string.h>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>

#include "device/device.h"
#include "device/gpu_resource_manager.h"
#include "device/gpu_semaphore.h"
#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
using namespace BASE_NS;

namespace {
const string_view RENDER_DATA_STORE_DEFAULT_STAGING { "RenderDataStoreDefaultStaging" };
const string_view RENDER_DATA_STORE_DEFAULT_DATA_COPY { "RenderDataStoreDefaultGpuResourceDataCopy" };
const string_view RENDER_DATA_STORE_POD { "RenderDataStorePod" };
const string_view POD_NAME { "NodeGraphBackBufferConfiguration" };

GpuBufferDesc GetStagingBufferDesc(
    const uint32_t byteSize, const EngineBufferCreationFlags engineBufferCreatoinAdditionalFlags)
{
    return GpuBufferDesc {
        CORE_BUFFER_USAGE_TRANSFER_DST_BIT,
        CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS,
        byteSize,
        {},
    };
}
} // namespace

RenderFrameUtil::RenderFrameUtil(const IRenderContext& renderContext)
    : renderContext_(renderContext), device_(renderContext_.GetDevice())
{
    bufferedPostFrame_.resize(device_.GetDeviceConfiguration().bufferingCount);
    defaultCopyData_.byteBuffer = make_unique<ByteArray>(0U);

    const IRenderDataStoreManager& dsManager = renderContext.GetRenderDataStoreManager();
    dsStaging_ =
        static_cast<IRenderDataStoreDefaultStaging*>(dsManager.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING));
    PLUGIN_ASSERT(dsStaging_);
    dsCpuToGpuCopy_ = static_cast<IRenderDataStoreDefaultGpuResourceDataCopy*>(
        dsManager.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_DATA_COPY));
    PLUGIN_ASSERT(dsCpuToGpuCopy_);
}

void RenderFrameUtil::BeginFrame()
{
    frameHasWaitForIdle_ = false;
    {
        auto ClearAndReserve = [](const size_t count, auto& frameData) {
            frameData.copyData.clear();
            frameData.copyData.reserve(count);
        };

        const auto lock = std::lock_guard(mutex_);

        thisFrameCopiedData_.clear();
        const uint64_t frameIndex = device_.GetFrameCount();
        for (auto& ref : thisFrameSignalData_.gpuSemaphores) {
            if (ref) {
                gpuSignalDeferredDestroy_.push_back({ frameIndex, move(ref) });
            }
        }
        thisFrameSignalData_.gpuSemaphores.clear();
        thisFrameSignalData_.signalData.clear();

        // process backbuffer
        postBackBufferConfig_ = preBackBufferConfig_; // NOTE: copy
        // process copies
        const size_t count = preFrame_.copyData.size();
        auto& bufferedPostFrame = bufferedPostFrame_[bufferedIndex_];
        ClearAndReserve(count, postFrame_);
        ClearAndReserve(count, bufferedPostFrame);
        for (size_t idx = 0; idx < count; ++idx) {
            if (preFrame_.copyData[idx].copyFlags == CopyFlagBits::WAIT_FOR_IDLE) {
                postFrame_.copyData.push_back(preFrame_.copyData[idx]);
                frameHasWaitForIdle_ = true;
            } else {
                bufferedPostFrame.copyData.push_back(preFrame_.copyData[idx]);
            }
        }
        preFrame_.copyData.clear();
        // process signals
        postSignalData_ = move(preSignalData_);
        preSignalData_.clear();
    }
    // advance to next buffer position, the copies are done for the oldest data
    bufferedIndex_ = (bufferedIndex_ + 1) % bufferedPostFrame_.size();

    // process deferred destruction
    ProcessFrameSignalDeferredDestroy();

    ProcessFrameCopyData();
    ProcessFrameSignalData();
    ProcessFrameBackBufferConfiguration();
}

void RenderFrameUtil::ProcessFrameCopyData()
{
    // try to reserve
    thisFrameCopiedData_.reserve(postFrame_.copyData.size() + bufferedPostFrame_[bufferedIndex_].copyData.size());

    // wait for idle
    ProcessFrameInputCopyData(postFrame_);
    // copy everything if wait for idle has been added
    if (frameHasWaitForIdle_) {
        for (auto& ref : bufferedPostFrame_) {
            ProcessFrameInputCopyData(ref);
        }
    } else {
        ProcessFrameInputCopyData(bufferedPostFrame_[bufferedIndex_]);
    }
}

void RenderFrameUtil::ProcessFrameInputCopyData(const RenderFrameUtil::CopyData& copyData)
{
    IGpuResourceManager& gpuResourceMgr = device_.GetGpuResourceManager();
    const uint32_t count = static_cast<uint32_t>(copyData.copyData.size());
    for (uint32_t idx = 0; idx < count; ++idx) {
        const auto& dataToBeCopied = copyData.copyData[idx];
        const RenderHandleType type = dataToBeCopied.handle.GetHandleType();
        thisFrameCopiedData_.push_back({});
        auto& copyDataRef = thisFrameCopiedData_.back();
        copyDataRef = { {}, dataToBeCopied.frameIndex, dataToBeCopied.handle, dataToBeCopied.copyFlags, {} };

        const bool byteBufferCopy = ((dataToBeCopied.copyFlags & IRenderFrameUtil::GPU_BUFFER_ONLY) == 0);
        const EngineBufferCreationFlags ebcf = byteBufferCopy ? 0U : CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER;
        if (type == RenderHandleType::GPU_BUFFER) {
            const GpuBufferDesc desc = gpuResourceMgr.GetBufferDescriptor(dataToBeCopied.handle);
            const uint32_t byteSize = desc.byteSize;

            copyDataRef.bufferHandle = gpuResourceMgr.Create(GetStagingBufferDesc(byteSize, ebcf));
            // byte buffer only created if needed
            if (byteBufferCopy) {
                copyDataRef.byteBuffer = make_unique<ByteArray>(byteSize);
            }

            const BufferCopy bc {
                0,        // srcOffset
                0,        // dstOffset
                byteSize, // size
            };
            dsStaging_->CopyBufferToBuffer(dataToBeCopied.handle, copyDataRef.bufferHandle, bc,
                IRenderDataStoreDefaultStaging::ResourceCopyInfo::END_FRAME);
        } else if (type == RenderHandleType::GPU_IMAGE) {
            const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(dataToBeCopied.handle);
            const uint32_t bytesPerPixel = gpuResourceMgr.GetFormatProperties(dataToBeCopied.handle).bytesPerPixel;
            const uint32_t byteSize = desc.width * desc.height * bytesPerPixel;

            copyDataRef.bufferHandle = gpuResourceMgr.Create(GetStagingBufferDesc(byteSize, ebcf));
            if (byteBufferCopy) {
                copyDataRef.byteBuffer = make_unique<ByteArray>(byteSize);
            }

            const BufferImageCopy bic {
                0,                                                                // bufferOffset
                0,                                                                // bufferRowLength
                0,                                                                // bufferImageHeight
                ImageSubresourceLayers { CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1u }, // imageSubresource
                Size3D { 0, 0, 0 },                                               // imageOffset
                Size3D { desc.width, desc.height, 1u },                           // imageExtent
            };
            dsStaging_->CopyImageToBuffer(dataToBeCopied.handle, copyDataRef.bufferHandle, bic,
                IRenderDataStoreDefaultStaging::ResourceCopyInfo::END_FRAME);
        }

        if (copyDataRef.byteBuffer && copyDataRef.bufferHandle) {
            IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
            if (copyDataRef.copyFlags & CopyFlagBits::WAIT_FOR_IDLE) {
                dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
            }
            dataCopy.gpuHandle = copyDataRef.bufferHandle;
            dataCopy.byteArray = copyDataRef.byteBuffer.get();
            dsCpuToGpuCopy_->AddCopyOperation(dataCopy);
        }
    }
}

void RenderFrameUtil::ProcessFrameSignalData()
{
    // try to reserve
    thisFrameSignalData_.gpuSemaphores.reserve(postSignalData_.size());
    thisFrameSignalData_.signalData.reserve(postSignalData_.size());

    Device& device = (Device&)device_;
    const DeviceBackendType backendType = device.GetBackendType();
    // check the input data and create possible semaphores
    for (const auto& ref : postSignalData_) {
        SignalData sd = ref;
        sd.signalResourceType = (backendType == DeviceBackendType::VULKAN) ? SignalResourceType::GPU_SEMAPHORE
                                                                           : SignalResourceType::GPU_FENCE;
        if (ref.gpuSignalResourceHandle) { // input signal handle given
                                           // validity checked with input method
            // create a view to external handle
            thisFrameSignalData_.gpuSemaphores.push_back(device.CreateGpuSemaphoreView(ref.gpuSignalResourceHandle));
        } else {
            // create semaphore
            thisFrameSignalData_.gpuSemaphores.push_back(device.CreateGpuSemaphore());
        }
        sd.gpuSignalResourceHandle = thisFrameSignalData_.gpuSemaphores.back()->GetHandle();
        thisFrameSignalData_.signalData.push_back(sd);
    }
}

void RenderFrameUtil::ProcessFrameBackBufferConfiguration()
{
    if (postBackBufferConfig_.force) {
        const auto& rdsMgr = renderContext_.GetRenderDataStoreManager();
        if (auto* dataStorePod = static_cast<IRenderDataStorePod*>(rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_POD));
            dataStorePod) {
            NodeGraphBackBufferConfiguration ngbbc;
            ngbbc.backBufferName[postBackBufferConfig_.bbc.backBufferName.copy(
                ngbbc.backBufferName, countof(ngbbc.backBufferName) - 1)] = '\0';
            ngbbc.backBufferType =
                static_cast<NodeGraphBackBufferConfiguration::BackBufferType>(postBackBufferConfig_.bbc.backBufferType);
            ngbbc.backBufferHandle = postBackBufferConfig_.bbc.backBufferHandle.GetHandle();
            ngbbc.gpuBufferHandle = postBackBufferConfig_.bbc.gpuBufferHandle.GetHandle();
            ngbbc.present = postBackBufferConfig_.bbc.present;
            ngbbc.gpuSemaphoreHandle = postBackBufferConfig_.bbc.gpuSemaphoreHandle;

            dataStorePod->Set(POD_NAME, arrayviewU8(ngbbc));
        }
    }
}

void RenderFrameUtil::ProcessFrameSignalDeferredDestroy()
{
    // already used in previous frame
    const Device& device = (const Device&)device_;
    const uint64_t frameCount = device.GetFrameCount();
    const auto minAge = device.GetCommandBufferingCount();
    const auto ageLimit = (frameCount < minAge) ? 0 : (frameCount - minAge);

    for (auto iter = gpuSignalDeferredDestroy_.begin(); iter != gpuSignalDeferredDestroy_.end();) {
        if (iter->frameUseIndex < ageLimit) {
            iter = gpuSignalDeferredDestroy_.erase(iter);
        } else {
            ++iter;
        }
    }
}

void RenderFrameUtil::EndFrame()
{
    frameHasWaitForIdle_ = false;

    postFrame_.copyData.clear();
    if (frameHasWaitForIdle_) {
        for (auto& ref : bufferedPostFrame_) {
            ref.copyData.clear();
        }
    } else {
        bufferedPostFrame_[bufferedIndex_].copyData.clear();
    }
}

void RenderFrameUtil::CopyToCpu(const RenderHandleReference& handle, const CopyFlags flags)
{
    if (!ValidateInput(handle)) {
        return;
    }

    const auto lock = std::lock_guard(mutex_);

    const uint64_t frameIndex = device_.GetFrameCount();
    preFrame_.copyData.push_back({ frameIndex, handle, flags });
}

bool RenderFrameUtil::ValidateInput(const RenderHandleReference& handle)
{
    bool valid = true;
    const RenderHandleType type = handle.GetHandleType();
    IGpuResourceManager& gpuResourceMgr = device_.GetGpuResourceManager();
    if (type == RenderHandleType::GPU_BUFFER) {
        const GpuBufferDesc desc = gpuResourceMgr.GetBufferDescriptor(handle);
        if ((desc.usageFlags & BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_SRC_BIT) == 0) {
            valid = false;
#if (RENDER_VALIDATION_ENABLED == 1)
            const string strId = string("rfu_CopyToCpu_") + BASE_NS::to_string(handle.GetHandle().id);
            PLUGIN_LOG_ONCE_W(strId.c_str(),
                "Render frame util needs usage flag CORE_BUFFER_USAGE_TRANSFER_SRC_BIT to CPU copies. (name:%s)",
                gpuResourceMgr.GetName(handle).c_str());
#endif
        }
    } else if (type == RenderHandleType::GPU_IMAGE) {
        const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(handle);
        if ((desc.usageFlags & BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_SRC_BIT) == 0) {
            valid = false;
#if (RENDER_VALIDATION_ENABLED == 1)
            const string strId = string("rfu_CopyToCpu_") + BASE_NS::to_string(handle.GetHandle().id);
            PLUGIN_LOG_ONCE_W(strId.c_str(),
                "Render frame util needs usage flag CORE_BUFFER_USAGE_TRANSFER_SRC_BIT to CPU copies. (name:%s)",
                gpuResourceMgr.GetName(handle).c_str());
#endif
        }
    } else {
        valid = false;
#if (RENDER_VALIDATION_ENABLED == 1)
        const string strId = string("rfu_CopyToCpu_") + BASE_NS::to_string(handle.GetHandle().id);
        PLUGIN_LOG_ONCE_W(strId.c_str(), "Render frame util CopyToCpu works only on GPU buffers and images.");
#endif
    }
    return valid;
}

array_view<IRenderFrameUtil::FrameCopyData> RenderFrameUtil::GetFrameCopyData()
{
    // NOTE: not thread safe (should not be called during rendering)
    return thisFrameCopiedData_;
}

const IRenderFrameUtil::FrameCopyData& RenderFrameUtil::GetFrameCopyData(const RenderHandleReference& handle)
{
    // NOTE: not thread safe (should not be called during rendering)
    if (handle) {
        const RenderHandle rawHandle = handle.GetHandle();
        for (const auto& ref : thisFrameCopiedData_) {
            if (ref.handle.GetHandle() == rawHandle) {
                return ref;
            }
        }
    }
    return defaultCopyData_;
}

void RenderFrameUtil::SetBackBufferConfiguration(const BackBufferConfiguration& backBufferConfiguration)
{
    const auto lock = std::lock_guard(mutex_);

    preBackBufferConfig_.force = true;
    preBackBufferConfig_.bbc = backBufferConfiguration;
}

void RenderFrameUtil::AddGpuSignal(const SignalData& signalData)
{
    bool valid = true;
    if (signalData.signaled) {
        valid = false;
        PLUGIN_LOG_E("Already signalled GPU signal cannot be processed. (Not supported)");
    }
    if (signalData.gpuSignalResourceHandle) {
        const DeviceBackendType backendType = device_.GetBackendType();
        const SignalResourceType signalResourceType = signalData.signalResourceType;
        if ((backendType == DeviceBackendType::VULKAN) && (signalResourceType != SignalResourceType::GPU_SEMAPHORE)) {
            valid = false;
        } else if ((backendType != DeviceBackendType::VULKAN) &&
                   (signalResourceType != SignalResourceType::GPU_FENCE)) {
            valid = false;
        }
        if (!valid) {
            PLUGIN_LOG_E("Invalid signal type (%u) for platform (%u)", static_cast<uint32_t>(signalResourceType),
                static_cast<uint32_t>(backendType));
        }
    }

    if (valid) {
        const auto lock = std::lock_guard(mutex_);

        preSignalData_.push_back(signalData);
    }
}

BASE_NS::array_view<IRenderFrameUtil::SignalData> RenderFrameUtil::GetFrameGpuSignalData()
{
    // NOTE: not thread safe (should not be called during rendering)
    return thisFrameSignalData_.signalData;
}

IRenderFrameUtil::SignalData RenderFrameUtil::GetFrameGpuSignalData(const RenderHandleReference& handle)
{
    // NOTE: not thread safe (should not be called during rendering)
    if (handle) {
        const RenderHandle rawHandle = handle.GetHandle();
        for (const auto& ref : thisFrameSignalData_.signalData) {
            if (ref.handle.GetHandle() == rawHandle) {
                return ref;
            }
        }
    }
    return {};
}

bool RenderFrameUtil::HasGpuSignals() const
{
    return !thisFrameSignalData_.gpuSemaphores.empty();
}

array_view<unique_ptr<GpuSemaphore>> RenderFrameUtil::GetGpuSemaphores()
{
    return thisFrameSignalData_.gpuSemaphores;
}

RENDER_END_NAMESPACE()
