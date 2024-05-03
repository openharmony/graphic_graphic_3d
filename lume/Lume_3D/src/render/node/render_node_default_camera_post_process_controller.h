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

#ifndef CORE3D_RENDER__NODE__RENDER_NODE_DEFAULT_CAMERA_POST_PROCESS_CONTROLLER_H
#define CORE3D_RENDER__NODE__RENDER_NODE_DEFAULT_CAMERA_POST_PROCESS_CONTROLLER_H

#include <3d/namespace.h>
#include <3d/render/intf_render_node_scene_util.h>
#include <base/containers/string.h>
#include <base/util/uid.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/nodecontext/intf_render_node_post_process_util.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultCamera;
class IRenderDataStoreDefaultScene;

class RenderNodeDefaultCameraPostProcessController final : public RENDER_NS::IRenderNode {
public:
    RenderNodeDefaultCameraPostProcessController() = default;
    ~RenderNodeDefaultCameraPostProcessController() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "31d7e1f0-cdeb-4f1d-832c-85c710beb100" };
    static constexpr char const* TYPE_NAME = "RenderNodeDefaultCameraPostProcessController";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    static constexpr uint64_t INVALID_CAM_ID { 0xFFFFFFFFffffffff };
    struct JsonInputs {
        BASE_NS::string customCameraName;
        uint64_t customCameraId { INVALID_CAM_ID };
    };
    JsonInputs jsonInputs_;

    struct CurrentScene {
        RenderCamera camera;

        uint32_t cameraIdx { 0 };
        BASE_NS::string customCameraName;
        uint64_t customCameraId { INVALID_CAM_ID };
        uint32_t multiviewLayer { RENDER_NS::PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };

        BASE_NS::string cameraControllerRenderNodeName;
    };
    bool validPostProcessInputs_ { false };

    void RegisterOutputs();
    void ParseRenderNodeInputs();
    void UpdateCurrentScene(
        const IRenderDataStoreDefaultScene& dataStoreScene, const IRenderDataStoreDefaultCamera& dataStoreCamera);

    SceneRenderDataStores stores_;
    CurrentScene currentScene_;

    RENDER_NS::IRenderNodePostProcessUtil::Ptr rnPostProcessUtil_;
};
CORE3D_END_NAMESPACE()

#endif // CORE3D_RENDER__NODE__RENDER_NODE_DEFAULT_CAMERA_POST_PROCESS_CONTROLLER_H
