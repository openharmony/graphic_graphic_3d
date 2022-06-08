/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "render_node_manager.h"

#include <render/namespace.h>

#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
IRenderNode* RenderNodeManager::Create(char const* nodeType)
{
    if (auto const pos = factories_.find(nodeType); pos != factories_.end()) {
        return pos->second.createNode();
    }
    return nullptr;
}

void RenderNodeManager::Destroy(char const* nodeType, IRenderNode* aInstance)
{
    if (auto const pos = factories_.find(nodeType); pos != factories_.end()) {
        return pos->second.destroyNode(aInstance);
    }
}

unique_ptr<IRenderNode, RenderNodeTypeInfo::DestroyRenderNodeFn> RenderNodeManager::CreateRenderNode(
    char const* nodeType)
{
    if (auto const pos = factories_.find(nodeType); pos != factories_.end()) {
        return { pos->second.createNode(), pos->second.destroyNode };
    }
    return { nullptr, nullptr };
}

RenderNodeManager::RenderNodeTypeInfoFlags RenderNodeManager::GetRenderNodeTypeInfoFlags(char const* nodeType)
{
    if (auto const pos = factories_.find(nodeType); pos != factories_.end()) {
        return { pos->second.renderNodeBackendFlags, pos->second.renderNodeClassType };
    }
    return {};
}

void RenderNodeManager::AddRenderNodeFactory(const RenderNodeTypeInfo& typeInfo)
{
    if (typeInfo.createNode && typeInfo.destroyNode) {
        factories_.insert({ typeInfo.typeName, typeInfo });
    } else {
        PLUGIN_LOG_E("RenderNodeTypeInfo must provide non-null function pointers");
        PLUGIN_ASSERT(typeInfo.createNode && "createNode cannot be null");
        PLUGIN_ASSERT(typeInfo.destroyNode && "destroyNode cannot be null");
    }
}

void RenderNodeManager::RemoveRenderNodeFactory(const RenderNodeTypeInfo& typeInfo)
{
    factories_.erase(string_view(typeInfo.typeName));
}
RENDER_END_NAMESPACE()
