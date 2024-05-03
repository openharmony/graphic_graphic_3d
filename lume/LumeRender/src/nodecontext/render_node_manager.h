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

#ifndef RENDER_RENDER__RENDER_NODE_MANAGER_H
#define RENDER_RENDER__RENDER_NODE_MANAGER_H

#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <core/namespace.h>
#include <render/intf_plugin.h>
#include <render/namespace.h>

CORE_BEGIN_NAMESPACE()
class IInterfaceRegister;
CORE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()
class IRenderNode;

/**
 */
class RenderNodeManager final {
public:
    RenderNodeManager() = default;
    ~RenderNodeManager() = default;

    RenderNodeManager(const RenderNodeManager&) = delete;
    RenderNodeManager& operator=(const RenderNodeManager&) = delete;

    IRenderNode* Create(char const* nodeType);
    void Destroy(char const* nodeType, IRenderNode* aInstance);

    BASE_NS::unique_ptr<IRenderNode, RENDER_NS::RenderNodeTypeInfo::DestroyRenderNodeFn> CreateRenderNode(
        char const* nodeType);

    struct RenderNodeTypeInfoFlags {
        RENDER_NS::RenderNodeTypeInfo::PluginRenderNodeBackendFlags backendFlags { 0 };
        RENDER_NS::RenderNodeTypeInfo::PluginRenderNodeClassType classType { 0 };
    };
    RenderNodeTypeInfoFlags GetRenderNodeTypeInfoFlags(char const* nodeType);

    void AddRenderNodeFactory(const RenderNodeTypeInfo& typeInfo);
    void RemoveRenderNodeFactory(const RenderNodeTypeInfo& typeInfo);

private:
    BASE_NS::unordered_map<BASE_NS::string, RENDER_NS::RenderNodeTypeInfo> factories_;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__RENDER_NODE_MANAGER_H
