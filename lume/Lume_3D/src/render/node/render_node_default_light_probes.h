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

#ifndef CORE__RENDER__NODE__RENDER_NODE_DEFAULT_LIGHT_PROBES_H
#define CORE__RENDER__NODE__RENDER_NODE_DEFAULT_LIGHT_PROBES_H

#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/pipeline_state_desc.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"
#include "render_node_default_material_render_slot.h"

CORE3D_BEGIN_NAMESPACE()

class RenderNodeDefaultLightProbes : public RenderNodeDefaultMaterialRenderSlot {
public:
    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    void PreExecuteFrame() override;

    void RenderSubmeshesProbes(RENDER_NS::IRenderCommandList& cmdList,
        const CORE3D_NS::IRenderDataStoreDefaultMaterial& dataStoreMaterial,
        const CORE3D_NS::IRenderDataStoreDefaultCamera& dataStoreCamera, const uint32_t viewMatrixIndex);

    void CreateDefaultShaderData();

    void ResetRenderSlotData(const uint32_t shaderRenderSlotId, const bool multiView);
    void UpdateCurrentScene(const CORE3D_NS::IRenderDataStoreDefaultScene& dataStoreScene,
        const CORE3D_NS::IRenderDataStoreDefaultCamera& dataStoreCamera,
        const CORE3D_NS::IRenderDataStoreDefaultLight& dataStoreLight);

    bool UpdateAndBindSet3Probes(RENDER_NS::IRenderCommandList& cmdList,
        const RenderDataDefaultMaterial::CustomResourceData& customResourceData);

    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

    static constexpr BASE_NS::Uid UID{"490441CD-5EA4-4F57-B4DC-1774F371D149"};
    static constexpr const char* const TYPE_NAME = "RenderNodeDefaultLightProbes";

protected:
    RENDER_NS::RenderHandleReference probeViewMatricesUbo_;
    BASE_NS::vector<BASE_NS::Math::Mat4X4> viewMatrices_;
};

CORE3D_END_NAMESPACE()

#endif  // CORE__RENDER__NODE__RENDER_NODE_DEFAULT_LIGHT_PROBES_H
