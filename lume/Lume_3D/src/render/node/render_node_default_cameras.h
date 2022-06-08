/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "b42910bb-33c4-4790-a257-3f1837415fce" };
    static constexpr char const* TYPE_NAME = "RenderNodeDefaultCameras";
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

    // get projection matrix with possible jittering
    JitterProjection GetProjectionMatrix(const RenderCamera& camera, const bool prevFrame) const;
    BASE_NS::Math::Mat4X4 GetShadowBiasMatrix(
        const IRenderDataStoreDefaultLight* dataStore, const RenderCamera& camera) const;

    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };
    CORE_NS::IFrustumUtil* frustumUtil_ { nullptr };

    SceneRenderDataStores stores_;

    RENDER_NS::RenderHandleReference resHandle_;
    uint32_t jitterIndex_ { 0u };
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_DEFAULT_CAMERAS_H
