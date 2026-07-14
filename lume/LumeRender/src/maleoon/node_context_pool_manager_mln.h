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

#ifndef MALEOON_NODE_CONTEXT_POOL_MANAGER_MLN_H
#define MALEOON_NODE_CONTEXT_POOL_MANAGER_MLN_H

#include <render/namespace.h>

#include "nodecontext/node_context_pool_manager.h"

RENDER_BEGIN_NAMESPACE()

class Device;
class GpuResourceManager;
struct GpuQueue;

class NodeContextPoolManagerMln final : public NodeContextPoolManager {
public:
    NodeContextPoolManagerMln(Device& device, GpuResourceManager& gpuResourceMgr, const GpuQueue& gpuQueue);
    ~NodeContextPoolManagerMln() override;

    void BeginFrame() override;
    void BeginBackendFrame() override;

#if ((RENDER_VALIDATION_ENABLED == 1) || (RENDER_VULKAN_VALIDATION_ENABLED == 1))
    void SetValidationDebugName(BASE_NS::string_view debugName) override;
#endif

private:
    Device& device_;
    GpuResourceManager& gpuResourceMgr_;
    uint32_t bufferingIndex_{0};
};

RENDER_END_NAMESPACE()

#endif  // MALEOON_NODE_CONTEXT_POOL_MANAGER_MLN_H
