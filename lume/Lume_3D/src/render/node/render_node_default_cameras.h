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

#ifndef CORE__RENDER__NODE__RENDER_NODE_DEFAULT_CAMERAS_H
#define CORE__RENDER__NODE__RENDER_NODE_DEFAULT_CAMERAS_H

#include <3d/namespace.h>
#include <base/math/matrix.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"

CORE_BEGIN_NAMESPACE()
class IFrustumUtil;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultLight;

class RenderNodeDefaultCameras final : public RENDER_NS::IRenderNode {
public:
    RenderNodeDefaultCameras() = default;
    ~RenderNodeDefaultCameras() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        // there should be cameras every frame with 3D
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "b42910bb-33c4-4790-a257-3f1837415fce" };
    static constexpr char const *TYPE_NAME = "RenderNodeDefaultCameras";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    struct JitterProjection {
        BASE_NS::Math::Mat4X4 baseProj { BASE_NS::Math::IDENTITY_4X4 };
        BASE_NS::Math::Mat4X4 proj { BASE_NS::Math::IDENTITY_4X4 };
        BASE_NS::Math::Vec4 jitter { 0.0f, 0.0f, 0.0f, 0.0f };
    };

    void AddCameras(const IRenderDataStoreDefaultCamera* dsCamera, const IRenderDataStoreDefaultLight* dsLight,
        const BASE_NS::array_view<const RenderCamera> cameras, uint8_t* const data, const uint32_t cameraOffset);
    void AddEnvironments(const IRenderDataStoreDefaultCamera* dsCamera,
        const BASE_NS::array_view<const RenderCamera::Environment> environments, uint8_t* const data);
    // get projection matrix with possible jittering
    JitterProjection GetProjectionMatrix(const RenderCamera& camera, const bool prevFrame) const;
    BASE_NS::Math::Mat4X4 GetShadowBiasMatrix(
        const IRenderDataStoreDefaultLight* dataStore, const RenderCamera& camera) const;
    BASE_NS::Math::UVec4 GetMultiViewCameraIndices(
        const IRenderDataStoreDefaultCamera* rds, const RenderCamera& cam, uint32_t cameraCount);
    BASE_NS::Math::Mat4X4 ResolveViewMatrix(const RenderCamera& camera) const;

    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };
    CORE_NS::IFrustumUtil* frustumUtil_ { nullptr };

    SceneRenderDataStores stores_;

    RENDER_NS::RenderHandleReference camHandle_;
    RENDER_NS::RenderHandleReference envHandle_;
    uint32_t jitterIndex_ { 0U };

    BASE_NS::vector<RenderCamera> cubemapCameras_;
    BASE_NS::vector<BASE_NS::Math::Mat4X4> cubemapMatrices_;
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_DEFAULT_CAMERAS_H
