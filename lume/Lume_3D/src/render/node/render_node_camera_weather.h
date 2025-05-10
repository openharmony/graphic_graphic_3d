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

#ifndef CORE3D_RENDER_NODE_RENDER_NODE_CAMERA_CLOUDS_H
#define CORE3D_RENDER_NODE_RENDER_NODE_CAMERA_CLOUDS_H

#include <3d/render/intf_render_data_store_default_light.h>
#include <render/datastore/intf_render_data_store_post_process.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/nodecontext/intf_render_node.h>

#include "render/datastore/render_data_store_weather.h"
#include "render_node_generics.h"

RENDER_BEGIN_NAMESPACE()
struct RenderNodeGraphInputs;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultCamera;
class IRenderDataStoreDefaultScene;
CORE3D_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultLight;
class RenderDataStoreWeather;
struct WeatherComponent;
class RenderNodeCameraWeather final : public RENDER_NS::IRenderNode, public RenderNodeMixin {
public:
    struct Settings;

    struct BlurShaderPushConstant {
        BASE_NS::Math::Mat4X4 view;
        BASE_NS::Math::Vec4 dir[4];
    };

    RenderNodeCameraWeather() = default;

    ~RenderNodeCameraWeather() = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;

    void PreExecuteFrame() override;

    struct GlobalFrameData;

    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    void ExecuteReprojected(RENDER_NS::IRenderCommandList& cmdList, const GlobalFrameData& data);
    void ExecuteDownscaled(RENDER_NS::IRenderCommandList& cmdList, const GlobalFrameData& data);
    void ExecuteFull(RENDER_NS::IRenderCommandList& cmdList, const GlobalFrameData& data);
    void ExecuteGenerateClouds(RENDER_NS::IRenderCommandList& cmdList, const ExplicitPass& p, const GlobalFrameData& d);

    void ExecuteUpsampleAndPostProcess(RENDER_NS::IRenderCommandList& cmdList, RENDER_NS::RenderHandle input,
        const ExplicitPass& p, const GlobalFrameData& d);
    void ExecuteUpsampleAndReproject(RENDER_NS::IRenderCommandList& cmdList, RENDER_NS::RenderHandle input,
        RENDER_NS::RenderHandle prevInput, const ExplicitPass& p, const GlobalFrameData& d);
    void ExecuteClear(RENDER_NS::IRenderCommandList& cmdList, const ExplicitPass& p, const GlobalFrameData& d);

    template<typename T, class U>
    void RenderFullscreenTriangle(
        int firstSet, RENDER_NS::IRenderCommandList& commandList, const BlurParams<T, U>& args);

    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "c269fda6-9b07-481c-8227-a74ad7b91d2e" };
    static constexpr char const* TYPE_NAME = "RenderNodeCameraWeather";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;

    static IRenderNode* Create();

    static void Destroy(IRenderNode* instance);

private:
    static constexpr uint64_t INVALID_CAM_ID { 0xFFFFFFFFffffffff };

    void ParseRenderNodeInputs();

    void GetSceneUniformBuffers(BASE_NS::string_view us);

    void UpdateCurrentScene(const CORE3D_NS::IRenderDataStoreDefaultScene& dataStoreScene,
        const CORE3D_NS::IRenderDataStoreDefaultCamera& dataStoreCamera,
        const CORE3D_NS::IRenderDataStoreDefaultLight& dataStoreLight);

    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    operator RENDER_NS::IRenderNodeContextManager*() const final
    {
        return renderNodeContextMgr_;
    }

    // Json resources which might need re-fetching
    struct JsonInputs {
        RENDER_NS::RenderNodeGraphInputs::InputRenderPass renderPass;
        RENDER_NS::RenderNodeGraphInputs::InputResources resources;

        BASE_NS::string customCameraName;
        uint64_t customCameraId { INVALID_CAM_ID };
    };

    struct ImageData {
        RENDER_NS::RenderHandle depth;
        RENDER_NS::RenderHandle velocityNormal;
    };
    ImageData images_;

    JsonInputs jsonInputs_;

    struct BuiltInVariables {
        RENDER_NS::RenderHandle input;

        RENDER_NS::RenderHandle output;

        RENDER_NS::RenderHandle defSampler;

        // the flag for the post built-in post process
        uint32_t postProcessFlag { 0u };
    };
    BuiltInVariables builtInVariables_;

    struct CurrentScene {
        CORE3D_NS::RenderCamera camera;
        RENDER_NS::RenderHandle cameraEnvRadianceHandle;
        RENDER_NS::RenderHandle prePassColorTarget;

        bool hasShadow { false };
        CORE3D_NS::IRenderDataStoreDefaultLight::ShadowTypes shadowTypes {};
        CORE3D_NS::IRenderDataStoreDefaultLight::LightingFlags lightingFlags { 0u };
    };
    CurrentScene currentScene_;

    RENDER_NS::PostProcessConfiguration ppGlobalConfig_;
    RENDER_NS::IRenderDataStorePostProcess::PostProcess ppLocalConfig_;

    CORE3D_NS::SceneBufferHandles sceneBuffers_;
    CORE3D_NS::SceneCameraBufferHandles cameraBuffers_;
    CORE3D_NS::SceneRenderDataStores stores_;
    BASE_NS::string dsWeatherName_;

    CORE3D_NS::IRenderNodeSceneUtil* renderNodeSceneUtil_ { nullptr };

    struct UboHandles {
        // first 512 aligned is global post process
        // after (256) we have effect local data
        RENDER_NS::RenderHandleReference postProcess;
    };
    UboHandles ubos_;
    RENDER_NS::RenderHandle cameraUboHandle_;

    bool valid_ { false };
    bool isDefaultImageInUse_ { false };

    GraphicsShaderData cloudVolumeShaderData_ {};
    GraphicsShaderData downsampleShaderData_ {};
    GraphicsShaderData deubgShaderData_ {};

    GraphicsShaderData downsampleData1_ {};
    GraphicsShaderData upsampleData1_ {};
    GraphicsShaderData upsampleData2_ {};
    ComputeShaderData computeShaderData_ {};

    RENDER_NS::IPipelineDescriptorSetBinder::Ptr upsample2Binder;
    RENDER_NS::IPipelineDescriptorSetBinder::Ptr upsample1Binder;
    RENDER_NS::IPipelineDescriptorSetBinder::Ptr cloudVolumeBinder_;

    RENDER_NS::RenderHandle depth_;

    RENDER_NS::RenderHandleReference uniformBuffer_;
    RENDER_NS::RenderHandleReference lights_;
    RENDER_NS::RenderHandle curlnoiseTex_;
    RENDER_NS::RenderHandle cirrusTex_;
    RENDER_NS::RenderHandle lowFrequencyTex_;
    RENDER_NS::RenderHandle highFrequencyTex_;
    RENDER_NS::RenderHandle weatherMapTex_;
    RENDER_NS::RenderHandle skyTex_;
    RENDER_NS::RenderHandleReference volumeSampler_;
    RENDER_NS::RenderHandleReference weatherSampler_;

    RENDER_NS::RenderHandle cloudTexture_;
    RENDER_NS::RenderHandle cloudTexturePrev_;

    bool clear_ { true };
    uint64_t uniformHash_ { 0 };
    int32_t downscale_ { 0 };
    uint32_t targetIndex_ { 0 };

    RenderDataStoreWeather::WeatherSettings settings_;
};
CORE3D_END_NAMESPACE()

#endif // CORE3D_RENDER_NODE_RENDER_NODE_CAMERA_CLOUDS_H
