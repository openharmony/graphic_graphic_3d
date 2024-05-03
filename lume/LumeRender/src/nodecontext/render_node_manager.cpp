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
