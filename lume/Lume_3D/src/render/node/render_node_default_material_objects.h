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

#ifndef CORE__RENDER__NODE__RENDER_NODE_DEFAULT_MATERIAL_OBJECTS_H
#define CORE__RENDER__NODE__RENDER_NODE_DEFAULT_MATERIAL_OBJECTS_H

#include <3d/render/intf_render_data_store_default_material.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
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
    static constexpr const char* const TYPE_NAME = "RenderNodeDefaultMaterialObjects";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

    struct MaterialHandleStruct {
        RENDER_NS::BindableImage resources[RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT];
    };

private:
    struct ObjectCounts {
        uint32_t maxMeshCount { 0u };
        uint32_t maxSubmeshCount { 0u };
        uint32_t maxSkinCount { 0u };
        uint32_t maxMaterialCount { 0u };
        uint32_t maxUniqueMaterialCount { 0u };
    };
    struct UboHandles {
        RENDER_NS::RenderHandleReference mat;
        RENDER_NS::RenderHandleReference matTransform;
        RENDER_NS::RenderHandleReference userMat;
        RENDER_NS::RenderHandleReference mesh;
        RENDER_NS::RenderHandleReference submeshSkin;
    };
    struct GlobalDescriptorSets {
        BASE_NS::vector<RENDER_NS::RenderHandleReference> handles;
        BASE_NS::vector<RENDER_NS::IDescriptorSetBinder::Ptr> descriptorSets;

        // keeps track if we need changes
        BASE_NS::vector<MaterialHandleStruct> materials;
        // force update all new descriptor sets
        bool forceUpdate { false };

        // default material set 1 descriptor set
        RENDER_NS::RenderHandleReference dmSet1;
        RENDER_NS::IDescriptorSetBinder::Ptr dmSet1Binder;

        // default material set 2 descriptor set
        // NOTE: with bindless contains global descriptor set if enabled
        RENDER_NS::RenderHandleReference dmSet2;
        RENDER_NS::IDescriptorSetBinder::Ptr dmSet2Binder;
        bool dmSet2Ready { false };
    };
    struct TlasData {
        RENDER_NS::RenderHandleReference as;
        RENDER_NS::RenderHandleReference asInstanceBuffer;
        RENDER_NS::RenderHandleReference scratch;

        RENDER_NS::AsBuildSizes asBuildSizes;
        bool createNewBuffer { false };
    };

    void UpdateMeshBuffer(const IRenderDataStoreDefaultMaterial& dataStoreMaterial);
    void UpdateSkinBuffer(const IRenderDataStoreDefaultMaterial& dataStoreMaterial);
    void UpdateMaterialBuffers(const IRenderDataStoreDefaultMaterial& dataStoreMaterial);
    void UpdateTlasBuffers(
        RENDER_NS::IRenderCommandList& cmdList, const IRenderDataStoreDefaultMaterial& dataStoreMaterial);
    void UpdateDescriptorSets(
        RENDER_NS::IRenderCommandList& cmdList, const IRenderDataStoreDefaultMaterial& dataStoreMaterial);

    void ProcessBuffers(const ObjectCounts& objectCounts);
    void ProcessGlobalBinders();
    void ProcessTlasBuffers();

    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    SceneRenderDataStores stores_;
    ObjectCounts objectCounts_;
    UboHandles ubos_;

    GlobalDescriptorSets globalDescs_;
    RENDER_NS::PipelineLayout defaultMaterialPipelineLayout_;
    MaterialHandleStruct defaultMaterialStruct_;

    bool rtEnabled_ { false };
    bool bindlessEnabled_ { false };
    TlasData tlas_;

    // helpers
    BASE_NS::vector<RENDER_NS::RenderHandle> blImageHandles_;
    BASE_NS::vector<RENDER_NS::RenderHandle> blSamplerHandles_;
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_DEFAULT_MATERIAL_OBJECTS_H
