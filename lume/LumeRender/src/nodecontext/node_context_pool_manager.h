/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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

#if ((RENDER_VALIDATION_ENABLED == 1) || (RENDER_VULKAN_VALIDATION_ENABLED == 1))
    virtual void SetValidationDebugName(const BASE_NS::string_view debugName) = 0;
#endif
private:
};
RENDER_END_NAMESPACE()

#endif // CORE_RENDER_NODE_CONTEXT_POOL_MANAGER_H
