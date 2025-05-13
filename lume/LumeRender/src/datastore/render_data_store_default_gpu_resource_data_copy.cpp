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

#include "render_data_store_default_gpu_resource_data_copy.h"

#include <cstdint>
#include <mutex>

#include <base/containers/vector.h>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "device/gpu_resource_manager.h"
#include "device/gpu_resource_util.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
RenderDataStoreDefaultGpuResourceDataCopy::RenderDataStoreDefaultGpuResourceDataCopy(
    IRenderContext& renderContext, const string_view name)
    : device_(renderContext.GetDevice()), gpuResourceMgr_((GpuResourceManager&)device_.GetGpuResourceManager()),
      name_(name)
{}

void RenderDataStoreDefaultGpuResourceDataCopy::PostRenderBackend()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!copyData_.empty() && waitForIdle_) {
        PLUGIN_LOG_W("RENDER_PERFORMANCE_WARNING: wait for idle called for device");
        device_.WaitForIdle();
    }
    for (const auto& ref : copyData_) {
        if (ref.gpuHandle && ref.byteArray) {
            GpuResourceUtil::CopyGpuResource(device_, gpuResourceMgr_, ref.gpuHandle.GetHandle(), *ref.byteArray);
        }
    }

    copyData_.clear();
    waitForIdle_ = false;
}

void RenderDataStoreDefaultGpuResourceDataCopy::Clear()
{
    // data cannot be cleared
}

void RenderDataStoreDefaultGpuResourceDataCopy::Ref()
{
    refcnt_.fetch_add(1, std::memory_order_relaxed);
}

void RenderDataStoreDefaultGpuResourceDataCopy::Unref()
{
    if (std::atomic_fetch_sub_explicit(&refcnt_, 1, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
    }
}

int32_t RenderDataStoreDefaultGpuResourceDataCopy::GetRefCount()
{
    return refcnt_;
}

void RenderDataStoreDefaultGpuResourceDataCopy::AddCopyOperation(const GpuResourceDataCopy& copyOp)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // NOTE: one could add support for GPU image copies
    // process the image -> buffer in render node staging

    if (gpuResourceMgr_.IsGpuBuffer(copyOp.gpuHandle)) {
        if (copyOp.copyType == CopyType::WAIT_FOR_IDLE) {
            waitForIdle_ = true;
        }
        copyData_.push_back(copyOp);
    } else {
        PLUGIN_LOG_E("Copy operation only supported for GPU buffers");
    }
}

// for plugin / factory interface
refcnt_ptr<IRenderDataStore> RenderDataStoreDefaultGpuResourceDataCopy::Create(
    IRenderContext& renderContext, const char* name)
{
    return refcnt_ptr<IRenderDataStore>(new RenderDataStoreDefaultGpuResourceDataCopy(renderContext, name));
}
RENDER_END_NAMESPACE()
