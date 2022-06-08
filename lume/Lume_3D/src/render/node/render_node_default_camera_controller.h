/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE__RENDER__NODE__RENDER_NODE_DEFAULT_CAMERA_CONTROLLER_H
#define CORE__RENDER__NODE__RENDER_NODE_DEFAULT_CAMERA_CONTROLLER_H

#include <base/containers/string.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultMaterial;
class IRenderDataStoreDefaultLight;
class IRenderDataStoreDefaultScene;

class RenderNodeDefaultCameraController final : public RENDER_NS::IRenderNode {
public:
    RenderNodeDefaultCameraController() = default;
    ~RenderNodeDefaultCameraController() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "46144344-29f8-4fc1-913a-ed5f6f2e20d0" };
    static constexpr char const* TYPE_NAME = "RenderNodeDefaultCameraController";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

    struct CreatedTargets {
        RENDER_NS::RenderHandleReference color;
        RENDER_NS::RenderHandleReference depth;

        // NOTE: depending on the post processes and the output target one could re-use colorTarget as resolve
        // With hdrp and no msaa acts as a 3d scene camera rendering output
        RENDER_NS::RenderHandleReference colorResolve;

        RENDER_NS::RenderHandleReference colorMsaa;
        RENDER_NS::RenderHandleReference depthMsaa;

        // used in both DF and FW pipeline (not with light-weight)
        RENDER_NS::RenderHandleReference velocityNormal;
        // enabled with a flag with both DF and FW (not with light-weight)
        RENDER_NS::RenderHandleReference history[2u];

        RENDER_NS::RenderHandleReference baseColor;
        RENDER_NS::RenderHandleReference material;
    };

    struct CameraResourceSetup {
        BASE_NS::Math::UVec2 outResolution { 0u, 0u };
        BASE_NS::Math::UVec2 renResolution { 0u, 0u };

        BASE_NS::Format colorFormat { BASE_NS::Format::BASE_FORMAT_UNDEFINED };
        BASE_NS::Format depthFormat { BASE_NS::Format::BASE_FORMAT_UNDEFINED };

        BASE_NS::Format hdrColorFormat { BASE_NS::Format::BASE_FORMAT_R16G16B16A16_SFLOAT };
        BASE_NS::Format hdrDepthFormat { BASE_NS::Format::BASE_FORMAT_D32_SFLOAT };

        RenderCamera::Flags camFlags { 0u };
        RenderCamera::RenderPipelineType pipelineType { RenderCamera::RenderPipelineType::FORWARD };

        RENDER_NS::RenderHandle colorTarget;
        RENDER_NS::RenderHandle depthTarget;

        uint32_t historyFlipFrame { 0 };

        RENDER_NS::DeviceBackendType backendType { RENDER_NS::DeviceBackendType::VULKAN };
    };

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    static constexpr uint64_t INVALID_CAM_ID { 0xFFFFFFFFffffffff };
    struct JsonInputs {
        BASE_NS::string customCameraName;
        uint64_t customCameraId { INVALID_CAM_ID };
    };
    JsonInputs jsonInputs_;

    struct UboHandles {
        RENDER_NS::RenderHandleReference generalData;
        RENDER_NS::RenderHandleReference environment;
        RENDER_NS::RenderHandleReference postProcess;
        RENDER_NS::RenderHandleReference fog;
    };
    struct CurrentScene {
        RenderCamera camera;

        uint32_t cameraIdx { 0 };
        BASE_NS::Math::Vec4 sceneTimingData { 0.0f, 0.0f, 0.0f, 0.0f };

        BASE_NS::string customCameraName;
        uint64_t customCameraId { INVALID_CAM_ID };
        BASE_NS::string customCamRngName;
    };
    struct ConfigurationNames {
        BASE_NS::string renderDataStoreName;
        BASE_NS::string postProcessConfigurationName;
    };

    void ParseRenderNodeInputs();
    void CreateResources();
    void CreateResourceBaseTargets();
    void RegisterOutputs();

    void UpdateCurrentScene(const IRenderDataStoreDefaultScene& dataStoreScene,
        const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultLight& dataStoreLight);
    void CreateBuffers();
    void UpdateBuffers();
    void UpdateGeneralUniformBuffer();
    void UpdateEnvironmentUniformBuffer();
    void UpdateFogUniformBuffer();
    void UpdatePostProcessUniformBuffer();
    void UpdatePostProcessConfiguration();

    SceneRenderDataStores stores_;
    CameraResourceSetup camRes_;
    UboHandles uboHandles_;
    CurrentScene currentScene_;
    CreatedTargets createdTargets_;

    RENDER_NS::RenderPostProcessConfiguration currentRenderPPConfiguration_;
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_DEFAULT_CAMERA_CONTROLLER_H
