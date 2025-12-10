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
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>

#include "render/datastore/render_data_store_weather.h"
#include "render/render_node_scene_util.h"
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
    struct GlobalFrameData;

    struct BlurShaderPushConstant {
        BASE_NS::Math::Mat4X4 view;
        BASE_NS::Math::Vec4 dir[4];
    };

    struct AerialPerspectiveParams {
        BASE_NS::Math::Vec4 sunDirElevation;
        BASE_NS::Math::Vec3 frustumA;
        float maxDistance;
        BASE_NS::Math::Vec3 frustumB;
        float worldScale;
        BASE_NS::Math::Vec3 frustumC;
        float _padding0;
        BASE_NS::Math::Vec3 frustumD;
        float _padding1;
        BASE_NS::Math::Vec3 cameraPosition;
        float _padding2;
        BASE_NS::Math::Vec3 rayleighScatteringBase;
        float mieAbsorptionBase;
        BASE_NS::Math::Vec3 ozoneAbsorptionBase;
        float mieScatteringBase;
        BASE_NS::Math::Mat4X4 shadowViewProj;
    };
    AerialPerspectiveParams aerialPerspectiveParams_;

    RenderNodeCameraWeather() = default;

    ~RenderNodeCameraWeather() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;

    void PreExecuteFrame() override;

    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;

    ExecuteFlags GetExecuteFlags() const override;

    void ComputeCloudWeatherMap(RENDER_NS::IRenderCommandList& cmdList, const GlobalFrameData& data);
    void ClearCloudTargets(RENDER_NS::IRenderCommandList& cmdList);
    void ComputeVolumetricCloud(RENDER_NS::IRenderCommandList& cmdList, const GlobalFrameData& data);

    void ComputeTransmittance(RENDER_NS::IRenderCommandList& cmdList);
    void ComputeMultipleScattering(RENDER_NS::IRenderCommandList& cmdList);
    void ComputeAerialPerspective(RENDER_NS::IRenderCommandList& cmdList);
    void ComputeSkyView(RENDER_NS::IRenderCommandList& cmdList);
    void ComputeSkyViewCubemap(RENDER_NS::IRenderCommandList& cmdList);
    void ComputeSkyViewCubemapMips(RENDER_NS::IRenderCommandList& cmdList);

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

    void UpdateAerialPerspectiveParams(const BASE_NS::Math::Mat4X4& viewProj);

    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    operator RENDER_NS::IRenderNodeContextManager*() const final
    {
        return renderNodeContextMgr_;
    }

    // Json resources which might need re-fetching
    struct JsonInputs {
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
        SceneRenderCameraData camData;
        RENDER_NS::RenderHandle cameraEnvRadianceHandle;
        RENDER_NS::RenderHandle prePassColorTarget;

        bool hasShadow { false };
        CORE3D_NS::IRenderDataStoreDefaultLight::ShadowTypes shadowTypes {};
        CORE3D_NS::IRenderDataStoreDefaultLight::LightingFlags lightingFlags { 0u };

        // used the fetch the new handle for new environment
        uint64_t envId { 0xFFFFFFFFffffffffULL };
    };
    CurrentScene currentScene_;

    RENDER_NS::PostProcessConfiguration ppGlobalConfig_;
    RENDER_NS::IRenderDataStorePostProcess::PostProcess ppLocalConfig_;

    CORE3D_NS::SceneBufferHandles sceneBuffers_;
    CORE3D_NS::SceneCameraBufferHandles cameraBuffers_;
    CORE3D_NS::SceneRenderDataStores stores_;
    BASE_NS::string dsWeatherName_;
    BASE_NS::string cameraName_;

    CORE3D_NS::IRenderNodeSceneUtil* renderNodeSceneUtil_ { nullptr };

    struct UboHandles {
        // first 512 aligned is global post process
        // after (256) we have effect local data
        RENDER_NS::RenderHandleReference postProcess;
    };
    UboHandles ubos_;

    bool valid_ { false };
    bool isDefaultImageInUse_ { false };

    RENDER_NS::RenderHandle transmittanceShader_;
    RENDER_NS::RenderHandle multipleScatteringShader_;
    RENDER_NS::RenderHandle aerialPerspectiveShader_;
    RENDER_NS::RenderHandle skyViewShader_;
    RENDER_NS::RenderHandle skyCubemapShader_;
    RENDER_NS::RenderHandle skyCubemapMipsShader_;

    RENDER_NS::RenderHandle cloudVolumeShader_;
    RENDER_NS::RenderHandle cloudWeatherGenShader_;

    BASE_NS::vector<RENDER_NS::IDescriptorSetBinder::Ptr> skyViewCubemapMipsLocalBinder_;

    RenderNodeSceneUtil::FrameGlobalDescriptorSets fgds_;

    struct Binders {
        RENDER_NS::IPipelineDescriptorSetBinder::Ptr transmittanceLut;
        RENDER_NS::IPipelineDescriptorSetBinder::Ptr multipleScatteringLut;
        RENDER_NS::IPipelineDescriptorSetBinder::Ptr aerialPerspectiveLut;
        RENDER_NS::IPipelineDescriptorSetBinder::Ptr skyViewLut;
        RENDER_NS::IPipelineDescriptorSetBinder::Ptr skyCubemap;
        BASE_NS::vector<RENDER_NS::IPipelineDescriptorSetBinder::Ptr> skyCubemapMips;

        RENDER_NS::IPipelineDescriptorSetBinder::Ptr cloudVolume;
        RENDER_NS::IPipelineDescriptorSetBinder::Ptr cloudWeatherGen;
    };
    Binders skyBinder_;

    struct PipelineLayouts {
        RENDER_NS::PipelineLayout transmittanceLut;
        RENDER_NS::PipelineLayout multipleScatteringLut;
        RENDER_NS::PipelineLayout aerialPerspectiveLut;
        RENDER_NS::PipelineLayout skyViewLut;
        RENDER_NS::PipelineLayout skyCubemap;
        RENDER_NS::PipelineLayout skyCubemapMips;

        RENDER_NS::PipelineLayout cloudVolume;
        RENDER_NS::PipelineLayout cloudWeatherGen;
    };
    PipelineLayouts skyPipelineLayouts_;

    struct PSOs {
        RENDER_NS::RenderHandle transmittanceLut;
        RENDER_NS::RenderHandle multipleScatteringLut;
        RENDER_NS::RenderHandle aerialPerspectiveLut;
        RENDER_NS::RenderHandle skyViewLut;
        RENDER_NS::RenderHandle skyCubemap;
        RENDER_NS::RenderHandle skyCubemapMips;

        RENDER_NS::RenderHandle cloudVolume;
        RENDER_NS::RenderHandle cloudWeatherGen;
    };
    PSOs skyPso_;

    struct DefaultSamplers {
        RENDER_NS::RenderHandle linearHandle;
        RENDER_NS::RenderHandle cubemapHandle;
    };

    DefaultSamplers defaultSamplers_;

    RENDER_NS::RenderHandle depth_;

    RENDER_NS::RenderHandleReference uniformBuffer_;
    RENDER_NS::RenderHandleReference lights_;
    RENDER_NS::RenderHandle curlnoiseTex_;
    RENDER_NS::RenderHandle cirrusTex_;
    RENDER_NS::RenderHandle lowFrequencyTex_;
    RENDER_NS::RenderHandle highFrequencyTex_;
    RENDER_NS::RenderHandle weatherMapTex_;
    RENDER_NS::RenderHandleReference weatherMapTexNew_;
    RENDER_NS::RenderHandle skyTex_;

    RENDER_NS::RenderHandleReference transmittanceLutHandle_;
    RENDER_NS::RenderHandleReference multipleScatteringLutHandle_;
    RENDER_NS::RenderHandleReference aerialPerspectiveLutHandle_;
    RENDER_NS::RenderHandleReference skyViewLutHandle_;
    RENDER_NS::RenderHandleReference skyCubemapHandle_;

    RENDER_NS::RenderHandleReference volumeSampler_;
    RENDER_NS::RenderHandleReference weatherSampler_;

    RENDER_NS::RenderHandle cloudTexture_;
    RENDER_NS::RenderHandle cloudTexturePrev_;
    RENDER_NS::RenderHandle cloudTextureDepth_;
    RENDER_NS::RenderHandle cloudTexturePrevDepth_;

    RENDER_NS::RenderHandleReference aerialPerspectiveBuffer_;

    bool clear_ { true };
    BASE_NS::Math::UVec2 cloudPrevTexSize_ { 0U, 0U };
    bool generateCloudWeatherMap_ { false };

    int32_t downscale_ { 0 };
    uint32_t cubeMapMipCount { 0 };
    uint64_t uniformHash_ { 0 };

    RenderDataStoreWeather::WeatherSettings settings_;
    Render::DeviceBackendType deviceBackendType_;
    BASE_NS::Math::Vec4 cameraPos_;

    BASE_NS::Math::Mat4X4 sunShadowViewProjMatrix_;
    BASE_NS::Math::Vec2 shadowMapAtlasRes_;
    bool isFirstFrame = true;
    bool needsFullMipUpdate = false;

    size_t frameNumber = 0;
    float prevTimeOfDay_;

    struct AtmosphericConfig {
        BASE_NS::Math::Vec3 rayleighScatteringBase;
        BASE_NS::Math::Vec3 ozoneAbsorptionBase;
        float mieScatteringBase;
        float mieAbsorptionBase;
        BASE_NS::Math::Vec3 groundColor;
        float skyViewBrightness;
    };
    AtmosphericConfig atmosphericConfigPrev_;

    bool IsAtmosphericConfigChanged();
    void UpdateAtmosphericConfig();
};
CORE3D_END_NAMESPACE()

#endif // CORE3D_RENDER_NODE_RENDER_NODE_CAMERA_CLOUDS_H
