/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "util/log.h"
#include "mln_log.h"
#include "node_context_pool_manager_mln.h"

#include <core/log.h>
#include <render/namespace.h>

#include "device/device.h"

RENDER_BEGIN_NAMESPACE()

NodeContextPoolManagerMln::NodeContextPoolManagerMln(
    Device& device, GpuResourceManager& gpuResourceMgr, const GpuQueue& gpuQueue)
    : device_(device), gpuResourceMgr_(gpuResourceMgr)
{
    MLN_LOG_INIT("NodeContextPoolManagerMln: initialized");
}

NodeContextPoolManagerMln::~NodeContextPoolManagerMln() = default;

void NodeContextPoolManagerMln::BeginFrame()
{
    const uint32_t bufferingCount = device_.GetCommandBufferingCount();
    if (bufferingCount > 0) {
        bufferingIndex_ = (bufferingIndex_ + 1u) % bufferingCount;
    }
}

void NodeContextPoolManagerMln::BeginBackendFrame()
{
    // Maleoon does not use command buffers or render passes in the Vulkan sense.
    // RenderTarget creation is handled by the render backend.
}

#if ((RENDER_VALIDATION_ENABLED == 1) || (RENDER_VULKAN_VALIDATION_ENABLED == 1))
void NodeContextPoolManagerMln::SetValidationDebugName(BASE_NS::string_view debugName)
{
    // No-op for Maleoon
}
#endif

RENDER_END_NAMESPACE()
