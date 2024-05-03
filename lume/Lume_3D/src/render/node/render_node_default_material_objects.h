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

#ifndef CORE__RENDER__NODE__RENDER_NODE_DEFAULT_MATERIAL_OBJECTS_H
#define CORE__RENDER__NODE__RENDER_NODE_DEFAULT_MATERIAL_OBJECTS_H

#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultMaterial;

class RenderNodeDefaultMaterialObjects final : public RENDER_NS::IRenderNode {
public:
    RenderNodeDefaultMaterialObjects() = default;
    ~RenderNodeDefaultMaterialObjects() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "a25b27ea-a4ff-4b64-87c7-a7865cccfd92" };
    static constexpr char const* const TYPE_NAME = "RenderNodeDefaultMaterialObjects";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    struct ObjectCounts {
        uint32_t maxMeshCount { 0u };
        uint32_t maxSubmeshCount { 0u };
        uint32_t maxSkinCount { 0u };
        uint32_t maxMaterialCount { 0u };
    };
    struct UboHandles {
        RENDER_NS::RenderHandleReference mat;
        RENDER_NS::RenderHandleReference matTransform;
        RENDER_NS::RenderHandleReference userMat;
        RENDER_NS::RenderHandleReference mesh;
        RENDER_NS::RenderHandleReference submeshSkin;
    };

    void UpdateBuffers(const IRenderDataStoreDefaultMaterial& dataStoreMaterial);
    void UpdateMeshBuffer(const IRenderDataStoreDefaultMaterial& dataStoreMaterial);
    void UpdateSkinBuffer(const IRenderDataStoreDefaultMaterial& dataStoreMaterial);
    void UpdateMaterialBuffers(const IRenderDataStoreDefaultMaterial& dataStoreMaterial);
    void ProcessBuffers(const ObjectCounts& objectCounts);

    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    SceneRenderDataStores stores_;
    ObjectCounts objectCounts_;
    UboHandles ubos_;
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_DEFAULT_MATERIAL_OBJECTS_H
