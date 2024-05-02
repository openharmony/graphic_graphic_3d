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

#ifndef CORE_RENDER_NODE_CONTEXT_POOL_MANAGER_H
#define CORE_RENDER_NODE_CONTEXT_POOL_MANAGER_H

#include <base/containers/string_view.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
/**
NodeContextPoolManager.
Creates context related platform resources per (command list / command buffer)
Recources are:
- command buffers
- framebuffers
*/
class NodeContextPoolManager {
public:
    NodeContextPoolManager() = default;
    virtual ~NodeContextPoolManager() = default;

    NodeContextPoolManager(const NodeContextPoolManager&) = delete;
    NodeContextPoolManager& operator=(const NodeContextPoolManager&) = delete;

    virtual void BeginFrame() = 0;
    virtual void BeginBackendFrame() = 0;

#if ((RENDER_VALIDATION_ENABLED == 1) || (RENDER_VULKAN_VALIDATION_ENABLED == 1))
    virtual void SetValidationDebugName(const BASE_NS::string_view debugName) = 0;
#endif
private:
};
RENDER_END_NAMESPACE()

#endif // CORE_RENDER_NODE_CONTEXT_POOL_MANAGER_H
