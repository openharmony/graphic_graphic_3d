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

#include "render_node_camera_weather.h"

#include <3d/implementation_uids.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <base/math/matrix_util.h>
#include <core/json/json.h>
#include <core/log.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "render/datastore/render_data_store_weather.h"
#include "render/shaders/common/render_post_process_structs_common.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

CORE3D_BEGIN_NAMESPACE()
struct RenderNodeCameraWeather::Settings {
    BASE_NS::Math::Vec4 params[16];
};

namespace {

constexpr uint32_t MAX_MIP_COUNT { 16U };

constexpr Math::UVec2 TRANSMITTACE_RESOLUTION { 256U, 64U };
constexpr Math::UVec2 MULTIPLE_SCATTERING_RESOLUTION { 32U, 32U };
constexpr Math::UVec2 AERIAL_PERSPECTIVE_RESOLUTION { 200U * 8, 150U * 4 }; // 32 depth slices 2d atlas
constexpr Math::UVec2 SKY_VIEW_RESOLUTION { 256U, 64U };
constexpr Math::UVec2 CUBEMAP_RESOLUTION { 256U, 256U };

constexpr ImageResourceBarrier SRC_UNDEFINED { 0, CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT, CORE_IMAGE_LAYOUT_UNDEFINED };
constexpr ImageResourceBarrier GENERAL { CORE_ACCESS_SHADER_WRITE_BIT, CORE_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    CORE_IMAGE_LAYOUT_GENERAL };
constexpr ImageResourceBarrier COMPUTE_READ { CORE_ACCESS_SHADER_READ_BIT, CORE_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    CORE_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
constexpr ImageResourceBarrier FINAL_DST { CORE_ACCESS_SHADER_READ_BIT,
    CORE_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | CORE_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    CORE_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

constexpr GpuBufferDesc UBO_DESC { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, 0U };

RenderHandleReference CreateGpuBuffers(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandleReference& handle)
{
    // Create buffer for the shader data.
    GpuBufferDesc desc = UBO_DESC;
    desc.byteSize = sizeof(RenderNodeCameraWeather::aerialPerspectiveParams_);
    return gpuResourceMgr.Create(handle, desc);
}

void UpdateBuffer(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandle& handle,
    const RenderNodeCameraWeather::AerialPerspectiveParams& shaderParameters)
{
    if (void* data = gpuResourceMgr.MapBuffer(handle); data) {
        CloneData(data, sizeof(RenderNodeCameraWeather::AerialPerspectiveParams), &shaderParameters,
            sizeof(RenderNodeCameraWeather::AerialPerspectiveParams));
        gpuResourceMgr.UnmapBuffer(handle);
    }
}

template<typename T>
RenderHandleReference CreateCloudDataUniformBuffer(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandleReference& handle)
{
    CORE_STATIC_ASSERT(sizeof(T) == PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE);
    return gpuResourceMgr.Create(
        handle, GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                    CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, sizeof(T) });
}

} // namespace

void RenderNodeCameraWeather::UpdateAerialPerspectiveParams(const Math::Mat4X4& viewProj)
{
    const Math::Mat4X4 invViewProj = Math::Inverse(viewProj);

    const Math::Vec4 A0 = invViewProj * Math::Vec4(-1.0f, -1.0f, 0.1f, 1.0f); // Top-left
    const Math::Vec4 A1 = invViewProj * Math::Vec4(-1.0f, -1.0f, 0.9f, 1.0f);
    const Math::Vec4 B0 = invViewProj * Math::Vec4(1.0f, -1.0f, 0.1f, 1.0f); // Top-right
    const Math::Vec4 B1 = invViewProj * Math::Vec4(1.0f, -1.0f, 0.9f, 1.0f);

    const Math::Vec4 C0 = invViewProj * Math::Vec4(-1.0f, 1.0f, 0.1f, 1.0f); // Bottom-left
    const Math::Vec4 C1 = invViewProj * Math::Vec4(-1.0f, 1.0f, 0.9f, 1.0f);
    const Math::Vec4 D0 = invViewProj * Math::Vec4(1.0f, 1.0f, 0.1f, 1.0f); // Bottom-right
    const Math::Vec4 D1 = invViewProj * Math::Vec4(1.0f, 1.0f, 0.9f, 1.0f);

    aerialPerspectiveParams_.frustumA =
        Math::Vec3(A1.x / A1.w, A1.y / A1.w, A1.z / A1.w) - Math::Vec3(A0.x / A0.w, A0.y / A0.w, A0.z / A0.w);
    aerialPerspectiveParams_.frustumB =
        Math::Vec3(B1.x / B1.w, B1.y / B1.w, B1.z / B1.w) - Math::Vec3(B0.x / B0.w, B0.y / B0.w, B0.z / B0.w);
    aerialPerspectiveParams_.frustumC =
        Math::Vec3(C1.x / C1.w, C1.y / C1.w, C1.z / C1.w) - Math::Vec3(C0.x / C0.w, C0.y / C0.w, C0.z / C0.w);
    aerialPerspectiveParams_.frustumD =
        Math::Vec3(D1.x / D1.w, D1.y / D1.w, D1.z / D1.w) - Math::Vec3(D0.x / D0.w, D0.y / D0.w, D0.z / D0.w);

    aerialPerspectiveParams_.sunDirElevation = settings_.sunDirElevation;
    aerialPerspectiveParams_.maxDistance = settings_.maxAerialPerspective;
    aerialPerspectiveParams_.worldScale = settings_.worldScale;
    aerialPerspectiveParams_.cameraPosition = cameraPos_;
    aerialPerspectiveParams_.rayleighScatteringBase = settings_.rayleighScatteringBase;
    aerialPerspectiveParams_.mieAbsorptionBase = settings_.mieAbsorptionBase;
    aerialPerspectiveParams_.mieScatteringBase = settings_.mieScatteringBase;
    aerialPerspectiveParams_.ozoneAbsorptionBase = settings_.ozoneAbsorptionBase;
    aerialPerspectiveParams_.shadowViewProj = sunShadowViewProjMatrix_;
}

bool RenderNodeCameraWeather::IsAtmosphericConfigChanged()
{
    bool result = false;
    result |= atmosphericConfigPrev_.rayleighScatteringBase != settings_.rayleighScatteringBase;
    result |= atmosphericConfigPrev_.ozoneAbsorptionBase != settings_.ozoneAbsorptionBase;
    result |= atmosphericConfigPrev_.mieAbsorptionBase != settings_.mieAbsorptionBase;
    result |= atmosphericConfigPrev_.mieScatteringBase != settings_.mieScatteringBase;
    result |= atmosphericConfigPrev_.groundColor != settings_.groundColor;
    result |= atmosphericConfigPrev_.skyViewBrightness != settings_.skyViewBrightness;

    return result;
}

void RenderNodeCameraWeather::UpdateAtmosphericConfig()
{
    atmosphericConfigPrev_.rayleighScatteringBase = settings_.rayleighScatteringBase;
    atmosphericConfigPrev_.ozoneAbsorptionBase = settings_.ozoneAbsorptionBase;
    atmosphericConfigPrev_.mieAbsorptionBase = settings_.mieAbsorptionBase;
    atmosphericConfigPrev_.mieScatteringBase = settings_.mieScatteringBase;
    atmosphericConfigPrev_.groundColor = settings_.groundColor;
    atmosphericConfigPrev_.skyViewBrightness = settings_.skyViewBrightness;
}

void RenderNodeCameraWeather::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    deviceBackendType_ = renderNodeContextMgr_->GetRenderContext().GetDevice().GetBackendType();

    ParseRenderNodeInputs();

    auto& shaderMgr = renderNodeContextMgr.GetShaderManager();
    const auto& renderNodeUtil = renderNodeContextMgr.GetRenderNodeUtil();
    if (!shaderMgr.IsValid(transmittanceShader_)) {
        CORE_LOG_E("RenderNodeCameraWeather needs a valid shader handle");
    }

    if (auto* classRegister = renderNodeContextMgr_->GetRenderContext().GetInterface<IClassRegister>()) {
        renderNodeSceneUtil_ = CORE_NS::GetInstance<IRenderNodeSceneUtil>(*classRegister, UID_RENDER_NODE_SCENE_UTIL);
    }
    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    if (renderNodeSceneUtil_) {
        stores_ = renderNodeSceneUtil_->GetSceneRenderDataStores(
            renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);
        dsWeatherName_ = renderNodeSceneUtil_->GetSceneRenderDataStore(stores_, RenderDataStoreWeather::TYPE_NAME);

        GetSceneUniformBuffers(stores_.dataStoreNameScene);

        const auto& cam = currentScene_.camData.camera;
        const auto camHandles = renderNodeSceneUtil_->GetSceneCameraImageHandles(
            *renderNodeContextMgr_, stores_.dataStoreNameScene, cam.name, cam);
        currentScene_.cameraEnvRadianceHandle = camHandles.radianceCubemap;
    }
    currentScene_ = {};

    INodeContextPsoManager& psoMgr = renderNodeContextMgr_->GetPsoManager();

    // Transmittance lut compute shader
    {
        skyPipelineLayouts_.transmittanceLut =
            renderNodeContextMgr.GetRenderNodeUtil().CreatePipelineLayout(transmittanceShader_);
        skyPso_.transmittanceLut =
            psoMgr.GetComputePsoHandle(transmittanceShader_, skyPipelineLayouts_.transmittanceLut, {});
    }
    // Multiple scattering lut compute shader
    {
        skyPipelineLayouts_.multipleScatteringLut =
            renderNodeContextMgr.GetRenderNodeUtil().CreatePipelineLayout(multipleScatteringShader_);
        skyPso_.multipleScatteringLut =
            psoMgr.GetComputePsoHandle(multipleScatteringShader_, skyPipelineLayouts_.multipleScatteringLut, {});
    }
    // Aerial Perspective lut compute shader
    {
        skyPipelineLayouts_.aerialPerspectiveLut =
            renderNodeContextMgr.GetRenderNodeUtil().CreatePipelineLayout(aerialPerspectiveShader_);
        skyPso_.aerialPerspectiveLut =
            psoMgr.GetComputePsoHandle(aerialPerspectiveShader_, skyPipelineLayouts_.aerialPerspectiveLut, {});

        aerialPerspectiveBuffer_ = CreateGpuBuffers(gpuResourceMgr, aerialPerspectiveBuffer_);
    }
    // Sky view lut compute shader
    {
        skyPipelineLayouts_.skyViewLut = renderNodeContextMgr.GetRenderNodeUtil().CreatePipelineLayout(skyViewShader_);
        skyPso_.skyViewLut = psoMgr.GetComputePsoHandle(skyViewShader_, skyPipelineLayouts_.skyViewLut, {});
    }
    // Sky cubemap compute shader
    {
        skyPipelineLayouts_.skyCubemap =
            renderNodeContextMgr.GetRenderNodeUtil().CreatePipelineLayout(skyCubemapShader_);
        skyPso_.skyCubemap = psoMgr.GetComputePsoHandle(skyCubemapShader_, skyPipelineLayouts_.skyCubemap, {});
    }
    // Sky cubemap mips compute shader
    {
        skyPipelineLayouts_.skyCubemapMips =
            renderNodeContextMgr.GetRenderNodeUtil().CreatePipelineLayout(skyCubemapMipsShader_);
        skyPso_.skyCubemapMips =
            psoMgr.GetComputePsoHandle(skyCubemapMipsShader_, skyPipelineLayouts_.skyCubemapMips, {});
    }
    // Cloud Volume compute shader
    {
        skyPipelineLayouts_.cloudVolume =
            renderNodeContextMgr.GetRenderNodeUtil().CreatePipelineLayout(cloudVolumeShader_);
        skyPso_.cloudVolume = psoMgr.GetComputePsoHandle(cloudVolumeShader_, skyPipelineLayouts_.cloudVolume, {});
    }
    // Cloud weatherMap generate compute shader
    {
        skyPipelineLayouts_.cloudWeatherGen =
            renderNodeContextMgr.GetRenderNodeUtil().CreatePipelineLayout(cloudWeatherGenShader_);
        skyPso_.cloudWeatherGen =
            psoMgr.GetComputePsoHandle(cloudWeatherGenShader_, skyPipelineLayouts_.cloudWeatherGen, {});
    }

    GpuImageDesc descLut { ImageType::CORE_IMAGE_TYPE_2D, ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
        Format::BASE_FORMAT_B10G11R11_UFLOAT_PACK32, ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
        ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_STORAGE_BIT,
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        0,                                                                        // ImageCreateFlags
        EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, // EngineImageCreationFlags
        TRANSMITTACE_RESOLUTION.x, TRANSMITTACE_RESOLUTION.y, 1U, 1U, 1U, SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,
        {} };

    cubeMapMipCount = static_cast<uint32_t>(std::log2(Math::max(CUBEMAP_RESOLUTION.x, CUBEMAP_RESOLUTION.y)) - 1);

    if (deviceBackendType_ != Render::DeviceBackendType::VULKAN) {
        descLut.format = BASE_FORMAT_R16G16B16A16_SFLOAT;
    }

    {
        // global name
        // NOTE: named similarly as camera targets
        const string fullCameraName = jsonInputs_.customCameraName + '_' + to_hex(jsonInputs_.customCameraId);
        const string transmittanceLutName = string(stores_.dataStoreNameScene) +
                                            DefaultMaterialCameraConstants::CAMERA_TRANSMITTANCE_LUT_PREFIX_NAME +
                                            fullCameraName;
        const string aerialPerspectiveLutName =
            string(stores_.dataStoreNameScene) +
            DefaultMaterialCameraConstants::CAMERA_AERIAL_PERSPECTIVE_LUT_PREFIX_NAME + fullCameraName;

        const string skyViewLutName = string(stores_.dataStoreNameScene) +
                                      DefaultMaterialCameraConstants::CAMERA_SKY_LUT_PREFIX_NAME + fullCameraName;

        // Transmittance image
        transmittanceLutHandle_ = gpuResourceMgr.Create(transmittanceLutName, descLut);

        // Multiple scattering image
        descLut.width = MULTIPLE_SCATTERING_RESOLUTION.x;
        descLut.height = MULTIPLE_SCATTERING_RESOLUTION.y;
        multipleScatteringLutHandle_ = gpuResourceMgr.Create(multipleScatteringLutHandle_, descLut);

        // Sky view image
        descLut.width = SKY_VIEW_RESOLUTION.x;
        descLut.height = SKY_VIEW_RESOLUTION.y;
        skyViewLutHandle_ = gpuResourceMgr.Create(skyViewLutName, descLut);

        // Aerial perspective image
        descLut.width = AERIAL_PERSPECTIVE_RESOLUTION.x;
        descLut.height = AERIAL_PERSPECTIVE_RESOLUTION.y;
        descLut.format = BASE_FORMAT_R16G16B16A16_SFLOAT;
        aerialPerspectiveLutHandle_ = gpuResourceMgr.Create(aerialPerspectiveLutName, descLut);
    }

    valid_ = true;
    clear_ = true;

    GpuSamplerDesc desc {};
    desc.minFilter = Filter::CORE_FILTER_LINEAR;
    desc.mipMapMode = Filter::CORE_FILTER_NEAREST;
    desc.magFilter = Filter::CORE_FILTER_LINEAR;
    desc.enableAnisotropy = false;
    desc.maxAnisotropy = 1;
    desc.minLod = 0;
    desc.maxLod = 10; // 10: parm
    desc.addressModeU = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT;
    desc.addressModeV = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT;
    desc.addressModeW = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT;
    volumeSampler_ = gpuResourceMgr.Create("SAMPLER_NEAREST_MIPMAP_CLAMP", desc);

    desc.minFilter = Filter::CORE_FILTER_LINEAR;
    desc.mipMapMode = Filter::CORE_FILTER_NEAREST;
    desc.magFilter = Filter::CORE_FILTER_LINEAR;
    desc.enableAnisotropy = false;
    desc.maxAnisotropy = 1;
    desc.minLod = 0;
    desc.maxLod = 0;
    desc.borderColor = Render::BorderColor::CORE_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    desc.addressModeU = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT;
    desc.addressModeV = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT;
    desc.addressModeW = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT;
    weatherSampler_ = gpuResourceMgr.Create(desc);

    GpuImageDesc imageDesc {
        ImageType::CORE_IMAGE_TYPE_3D,
        ImageViewType::CORE_IMAGE_VIEW_TYPE_3D,
        Format::BASE_FORMAT_R8_UINT,
        ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
        CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_STORAGE_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT,
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        0,
        EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS,
        32u,
        32u,
        32u,
        1u,
        1u,
        SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,
        {},
    };

    imageDesc.width = 2048; // 2048: parm
    imageDesc.height = 2048; // 2048: parm
    imageDesc.depth = 1u;
    imageDesc.format = Format::BASE_FORMAT_R32G32B32A32_SFLOAT;
    imageDesc.imageType = ImageType::CORE_IMAGE_TYPE_2D;
    imageDesc.imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
    weatherMapTexNew_ = gpuResourceMgr.Create(imageDesc);

    builtInVariables_.defSampler = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");

    skyTex_ = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");

    vector<DescriptorCounts> descriptorCounts;

    descriptorCounts.push_back(renderNodeUtil.GetDescriptorCounts(skyPipelineLayouts_.transmittanceLut));
    descriptorCounts.push_back(renderNodeUtil.GetDescriptorCounts(skyPipelineLayouts_.multipleScatteringLut));
    descriptorCounts.push_back(renderNodeUtil.GetDescriptorCounts(skyPipelineLayouts_.aerialPerspectiveLut));
    descriptorCounts.push_back(renderNodeUtil.GetDescriptorCounts(skyPipelineLayouts_.skyViewLut));
    descriptorCounts.push_back(renderNodeUtil.GetDescriptorCounts(skyPipelineLayouts_.skyCubemap));

    DescriptorCounts skyCubeMips = renderNodeUtil.GetDescriptorCounts(skyPipelineLayouts_.skyCubemapMips);
    for (uint32_t idx = 0; idx < skyCubeMips.counts.size(); idx++) {
        skyCubeMips.counts[idx].count *= MAX_MIP_COUNT * CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT;
    }
    descriptorCounts.push_back(skyCubeMips);

    descriptorCounts.push_back(renderNodeUtil.GetDescriptorCounts(skyPipelineLayouts_.cloudVolume));
    descriptorCounts.push_back(renderNodeUtil.GetDescriptorCounts(skyPipelineLayouts_.cloudWeatherGen));
    renderNodeContextMgr.GetDescriptorSetManager().ResetAndReserve(descriptorCounts);

    // Sky rendering
    skyBinder_.transmittanceLut =
        renderNodeUtil.CreatePipelineDescriptorSetBinder(skyPipelineLayouts_.transmittanceLut);
    skyBinder_.multipleScatteringLut =
        renderNodeUtil.CreatePipelineDescriptorSetBinder(skyPipelineLayouts_.multipleScatteringLut);
    skyBinder_.aerialPerspectiveLut =
        renderNodeUtil.CreatePipelineDescriptorSetBinder(skyPipelineLayouts_.aerialPerspectiveLut);
    skyBinder_.skyViewLut = renderNodeUtil.CreatePipelineDescriptorSetBinder(skyPipelineLayouts_.skyViewLut);
    skyBinder_.skyCubemap = renderNodeUtil.CreatePipelineDescriptorSetBinder(skyPipelineLayouts_.skyCubemap);
    skyBinder_.cloudVolume = renderNodeUtil.CreatePipelineDescriptorSetBinder(skyPipelineLayouts_.cloudVolume);
    skyBinder_.cloudWeatherGen = renderNodeUtil.CreatePipelineDescriptorSetBinder(skyPipelineLayouts_.cloudWeatherGen);

    skyBinder_.skyCubemapMips.resize(MAX_MIP_COUNT);
    for (uint32_t i = 0; i < MAX_MIP_COUNT; i++) {
        skyBinder_.skyCubemapMips[i] =
            renderNodeUtil.CreatePipelineDescriptorSetBinder(skyPipelineLayouts_.skyCubemapMips);
    }
    isDefaultImageInUse_ = false;
    uniformBuffer_ = CreateCloudDataUniformBuffer<Settings>(gpuResourceMgr, uniformBuffer_);

    defaultSamplers_.linearHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    defaultSamplers_.cubemapHandle =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP_SAMPLER);
}

struct RenderNodeCameraWeather::GlobalFrameData {
    Math::Vec4 timings { 0.0f, 0.0f, 0.0f, 0.0f };
};

void RenderNodeCameraWeather::PreExecuteFrame()
{
    UpdateAtmosphericConfig();
    prevTimeOfDay_ = settings_.timeOfDay;

    {
        const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        const auto* dataStoreScene = static_cast<IRenderDataStoreDefaultScene*>(
            renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
        const auto* dataStoreCamera = static_cast<IRenderDataStoreDefaultCamera*>(
            renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
        const auto* dataStoreLight = static_cast<IRenderDataStoreDefaultLight*>(
            renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));
        const auto* dataStoreWeather =
            static_cast<RenderDataStoreWeather*>(renderDataStoreMgr.GetRenderDataStore(dsWeatherName_));
        const bool validRenderDataStore = dataStoreScene && dataStoreCamera && dataStoreLight && dataStoreWeather;
        if (validRenderDataStore) {
            UpdateCurrentScene(*dataStoreScene, *dataStoreCamera, *dataStoreLight);

            settings_ = dataStoreWeather->GetWeatherSettings();
            cameraPos_ = Math::Inverse(currentScene_.camData.camera.matrices.view)[3]; // 3: index
            downscale_ = (int32_t)settings_.cloudRenderingType;

            auto cameras = dataStoreCamera->GetCameras();
            for (const auto& c : cameras) {
                if (c.shadowId == settings_.sunId) {
                    sunShadowViewProjMatrix_ = c.matrices.proj * c.matrices.view;
                    break;
                }
            }

            const auto& gpuResourceManager = renderNodeContextMgr_->GetGpuResourceManager();

            const RenderHandle shadowMapBuffer = gpuResourceManager.GetImageHandle(
                stores_.dataStoreNameScene + DefaultMaterialLightingConstants::SHADOW_DEPTH_BUFFER_NAME);
            const GpuImageDesc imageDesc = gpuResourceManager.GetImageDescriptor(shadowMapBuffer);
            shadowMapAtlasRes_ = Math::Vec2(float(imageDesc.width), float(imageDesc.height));

            const auto cameraId = to_hex(currentScene_.camData.camera.id);
            cloudTexture_ = gpuResourceManager.GetImageHandle(
                DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "CLOUD" + cameraId);
            cloudTexturePrev_ = gpuResourceManager.GetImageHandle(
                DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "CLOUD_PREV" + cameraId);
            cloudTextureDepth_ = gpuResourceManager.GetImageHandle(
                DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "CLOUD_DEPTH" + cameraId);
            cloudTexturePrevDepth_ = gpuResourceManager.GetImageHandle(
                DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "CLOUD_DEPTH_PREV" + cameraId);
            if (*reinterpret_cast<const uint32_t*>(&settings_.groundColor.w) & 1) {
                std::swap(cloudTexture_, cloudTexturePrev_);
                std::swap(cloudTextureDepth_, cloudTexturePrevDepth_);
            }
        }
    }
}

void RenderNodeCameraWeather::ExecuteFrame(IRenderCommandList& cmdList)
{
    lowFrequencyTex_ = settings_.lowFrequencyImage;
    highFrequencyTex_ = settings_.highFrequencyImage;
    curlnoiseTex_ = settings_.curlNoiseImage;
    cirrusTex_ = settings_.cirrusImage;
    weatherMapTex_ = settings_.weatherMapImage;

    fgds_ = RenderNodeSceneUtil::GetFrameGlobalDescriptorSets(*renderNodeContextMgr_, stores_, cameraName_,
        RenderNodeSceneUtil::FrameGlobalDescriptorSetFlagBits::GLOBAL_SET_0);
    if (!fgds_.valid) {
        return;
    }

    GlobalFrameData data {};
    data.timings = renderNodeContextMgr_->GetRenderNodeGraphData().renderingConfiguration.renderTimings;

    const auto sHash = BASE_NS::FNV1a32Hash(reinterpret_cast<const uint8_t*>(&settings_), sizeof(settings_));
    if (uniformHash_ != sHash) {
        uniformHash_ = sHash;
        Settings uniformSettings;
        uniformSettings.params[0U].x = settings_.nightTopAmbientColor.x;
        uniformSettings.params[0U].y = settings_.nightTopAmbientColor.y;
        uniformSettings.params[0U].z = settings_.nightTopAmbientColor.z;
        uniformSettings.params[0U].w = settings_.coverage;

        uniformSettings.params[1U] = settings_.sunDirElevation;
        uniformSettings.params[1U].w = settings_.cloudTopOffset;

        uniformSettings.params[2U].x = settings_.earthRadius;
        uniformSettings.params[2U].y = settings_.cloudBottomAltitude;
        uniformSettings.params[2U].z = settings_.cloudTopAltitude;
        uniformSettings.params[2U].w = settings_.crispiness;

        uniformSettings.params[3U].x = settings_.cloudType;
        uniformSettings.params[3U].y = settings_.nightBottomAmbientColor.x;
        uniformSettings.params[3U].z = settings_.nightBottomAmbientColor.y;
        uniformSettings.params[3U].w = settings_.nightBottomAmbientColor.z;

        uniformSettings.params[4U].x = settings_.ambientIntensityNight;
        uniformSettings.params[4U].y = settings_.curliness;
        uniformSettings.params[4U].z = settings_.maxSamples;
        uniformSettings.params[4U].w = settings_.directLightIntensityNight;

        uniformSettings.params[5U].x = settings_.windSpeed;
        uniformSettings.params[5U].y = settings_.density;
        uniformSettings.params[5U].z = settings_.ambientIntensityDay;
        uniformSettings.params[5U].w = settings_.softness;

        uniformSettings.params[6U].x = settings_.cloudHorizonFogHeight;
        uniformSettings.params[6U].y = settings_.weatherScale;
        uniformSettings.params[6U].z = settings_.directIntensityDay;
        uniformSettings.params[6U].w = settings_.horizonFogFalloff;

        uniformSettings.params[7U].x = settings_.windDir.x;
        uniformSettings.params[7U].y = settings_.windDir.y;
        uniformSettings.params[7U].z = settings_.windDir.z;
        uniformSettings.params[7U].w = (float)settings_.cloudOptimizationFlags;

        uniformSettings.params[8U].x = settings_.dayTopAmbientColor.x;
        uniformSettings.params[8U].y = settings_.dayTopAmbientColor.y;
        uniformSettings.params[8U].z = settings_.dayTopAmbientColor.z;

        uniformSettings.params[9U] = settings_.groundColor;
        uniformSettings.params[9U].w = settings_.skyViewBrightness;

        uniformSettings.params[10U] = settings_.ambientColorHueSunset;

        uniformSettings.params[11U] = settings_.directColorHueSunset;

        uniformSettings.params[12U] = settings_.sunGlareColorDay;

        uniformSettings.params[13U] = settings_.sunGlareColorSunset;

        uniformSettings.params[14U] = settings_.moonGlareColor;

        uniformSettings.params[15U].x = settings_.moonBaseColor.x;
        uniformSettings.params[15U].y = settings_.moonBaseColor.y;
        uniformSettings.params[15U].z = settings_.moonBaseColor.z;
        uniformSettings.params[15U].w = settings_.moonBrightness;

        UpdateBufferData<Settings>(
            renderNodeContextMgr_->GetGpuResourceManager(), uniformBuffer_.GetHandle(), uniformSettings);
    }

    // Transmittance and Multiscattering lut needs to be updated when related parameters are changed
    if (isFirstFrame || IsAtmosphericConfigChanged()) {
        ComputeTransmittance(cmdList);
        ComputeMultipleScattering(cmdList);
    }

    // Sky view is dependent of the camera height, therefore calculate it everyframe to render from different altitudes
    ComputeSkyView(cmdList);

    // Update cubemap when the sun position or any of the look up tables have changed
    if (isFirstFrame || prevTimeOfDay_ != settings_.timeOfDay || IsAtmosphericConfigChanged()) {
        ComputeSkyViewCubemap(cmdList);
        ComputeSkyViewCubemapMips(cmdList);

        UpdateAtmosphericConfig();
        prevTimeOfDay_ = settings_.timeOfDay;
        // For some reason OpenGL is cleaning the cubemap on the first iteration, so flag this after second frame
        isFirstFrame = frameNumber < 2; // 2: parm
        needsFullMipUpdate = true;
    } else if (needsFullMipUpdate) {
        ComputeSkyViewCubemap(cmdList);
        ComputeSkyViewCubemapMips(cmdList);
        needsFullMipUpdate = false;
    }

    frameNumber++;

    // Needs fine-tuning
    if (generateCloudWeatherMap_) {
        ComputeCloudWeatherMap(cmdList, data);
    }

    ClearCloudTargets(cmdList);
    ComputeVolumetricCloud(cmdList, data);
}

IRenderNode::ExecuteFlags RenderNodeCameraWeather::GetExecuteFlags() const
{
    if (RenderHandleUtil::IsValid(cloudTexture_) && RenderHandleUtil::IsValid(cloudTexturePrev_) &&
        RenderHandleUtil::IsValid(cloudTextureDepth_) && RenderHandleUtil::IsValid(cloudTexturePrevDepth_)) {
        return ExecuteFlagBits::EXECUTE_FLAG_BITS_DEFAULT;
    }
    return ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
}

struct CloudPcStruct {
    BASE_NS::Math::Vec4 params;
};

void RenderNodeCameraWeather::ComputeCloudWeatherMap(
    RENDER_NS::IRenderCommandList& cmdList, const GlobalFrameData& data)
{
    RENDER_DEBUG_MARKER_SCOPE(cmdList, "Compute Generate WeatherMap");
    if (!RenderHandleUtil::IsValid(cloudWeatherGenShader_)) {
        return;
    }
    cmdList.BindPipeline(skyPso_.cloudWeatherGen);
    skyBinder_.cloudWeatherGen->ClearBindings();
    // Inputs
    skyBinder_.cloudWeatherGen->BindImage(0U, 0U, { weatherMapTexNew_.GetHandle() });

    const auto descHandle0 = skyBinder_.cloudWeatherGen->GetDescriptorSetHandle(0U);
    auto bindings = skyBinder_.cloudWeatherGen->GetDescriptorSetLayoutBindingResources(0U);
    const auto descHandles = skyBinder_.cloudWeatherGen->GetDescriptorSetHandles();

    PushConstant pc { ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT, sizeof(uint32_t) };
    uint32_t step = 1u;
    Math::UVec3 resolution { 2048u, 2048u, 1u };
    int localSize = 8u;

    cmdList.UpdateDescriptorSet(descHandle0, bindings);
    cmdList.BindDescriptorSets(0U, descHandles);
    cmdList.PushConstantData(pc, arrayviewU8(step));
    cmdList.Dispatch((resolution.x / localSize), (resolution.y / localSize), 1);
}

void RenderNodeCameraWeather::ClearCloudTargets(RENDER_NS::IRenderCommandList& cmdList)
{
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const auto rextureImageDesc = gpuResourceMgr.GetImageDescriptor(cloudTexture_);

    const auto w = rextureImageDesc.width;
    const auto h = rextureImageDesc.height;
    constexpr ImageSubresourceRange imgSubresourceRange { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0,
        PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };

    // Initialize the cloud textures if their size was changed
    if (cloudPrevTexSize_.x != w || cloudPrevTexSize_.y != h) {
        cloudPrevTexSize_.x = w;
        cloudPrevTexSize_.y = h;
        static constexpr auto clearColor = RENDER_NS::ClearColorValue({ 0.0f, 0.0f, 0.0f, 0.0f });
        cmdList.ClearColorImage(cloudTexture_, clearColor, { &imgSubresourceRange, 1 });
        cmdList.ClearColorImage(cloudTexturePrev_, clearColor, { &imgSubresourceRange, 1 });
        cmdList.ClearColorImage(cloudTextureDepth_, clearColor, { &imgSubresourceRange, 1 });
        cmdList.ClearColorImage(cloudTexturePrevDepth_, clearColor, { &imgSubresourceRange, 1 });
    }
}

void RenderNodeCameraWeather::ComputeVolumetricCloud(
    RENDER_NS::IRenderCommandList& cmdList, const GlobalFrameData& data)
{
    RENDER_DEBUG_MARKER_SCOPE(cmdList, "Volumetric Cloud CAM" + to_string(currentScene_.camData.cameraIdx));
    if (!RenderHandleUtil::IsValid(cloudVolumeShader_)) {
        return;
    }

    cmdList.BindPipeline(skyPso_.cloudVolume);

    skyBinder_.cloudVolume->ClearBindings();

    uint32_t binding = 0;
    // Inputs
    skyBinder_.cloudVolume->BindSampler(1U, binding++, { builtInVariables_.defSampler });
    BindableImage bindable;
    bindable.handle = skyCubemapHandle_.GetHandle();
    bindable.samplerHandle = defaultSamplers_.cubemapHandle;
    skyBinder_.cloudVolume->BindImage(1U, binding++, bindable);

    skyBinder_.cloudVolume->BindSampler(1U, binding++, { volumeSampler_.GetHandle() });
    skyBinder_.cloudVolume->BindImage(1U, binding++, { lowFrequencyTex_ });
    skyBinder_.cloudVolume->BindImage(1U, binding++, { highFrequencyTex_ });
    skyBinder_.cloudVolume->BindImage(1U, binding++, { curlnoiseTex_ });
    skyBinder_.cloudVolume->BindSampler(1U, binding++, { weatherSampler_.GetHandle() });
    skyBinder_.cloudVolume->BindImage(1U, binding++, { cirrusTex_ });
    if (generateCloudWeatherMap_) {
        skyBinder_.cloudVolume->BindImage(1U, binding++, { weatherMapTexNew_.GetHandle() });
    } else {
        skyBinder_.cloudVolume->BindImage(1U, binding++, { weatherMapTex_ });
    }
    skyBinder_.cloudVolume->BindBuffer(1U, binding, { uniformBuffer_.GetHandle() });

    // Outputs
    BindableImage writeImg;
    BindableImage historyImg;
    BindableImage writeDepthImg;
    BindableImage historyDepthImg;
    writeImg.handle = cloudTexture_;
    historyImg.handle = cloudTexturePrev_;
    writeDepthImg.handle = cloudTextureDepth_;
    historyDepthImg.handle = cloudTexturePrevDepth_;

    skyBinder_.cloudVolume->BindImage(2U, 0U, writeImg);
    skyBinder_.cloudVolume->BindImage(2U, 1U, historyImg);
    skyBinder_.cloudVolume->BindImage(2U, 2U, { writeDepthImg });
    skyBinder_.cloudVolume->BindImage(2U, 3U, { historyDepthImg });

    const auto descHandle1 = skyBinder_.cloudVolume->GetDescriptorSetHandle(1U);
    auto bindings = skyBinder_.cloudVolume->GetDescriptorSetLayoutBindingResources(1);
    cmdList.UpdateDescriptorSet(descHandle1, bindings);

    const auto descHandle2 = skyBinder_.cloudVolume->GetDescriptorSetHandle(2U);
    bindings = skyBinder_.cloudVolume->GetDescriptorSetLayoutBindingResources(2U);
    cmdList.UpdateDescriptorSet(descHandle2, bindings);

    // Bind descriptor sets
    cmdList.BindDescriptorSet(0U, fgds_.set0);
    cmdList.BindDescriptorSet(1U, skyBinder_.cloudVolume->GetDescriptorSetHandle(1U));
    cmdList.BindDescriptorSet(2U, skyBinder_.cloudVolume->GetDescriptorSetHandle(2U));

    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const auto textureImageDesc = gpuResourceMgr.GetImageDescriptor(cloudTexture_);

    PushConstant pc { ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT, sizeof(CloudPcStruct) };
    CloudPcStruct uPc;
    uPc.params.x = data.timings.z;
    uPc.params.y = data.timings.w;
    uPc.params.z = float(textureImageDesc.width);
    uPc.params.w = float(textureImageDesc.height);

    if (textureImageDesc.width > 1u && textureImageDesc.height > 1u) {
        const Math::UVec3 targetSize = { textureImageDesc.width, textureImageDesc.height, 1 };

        const uint32_t tgcX = (targetSize.x + 8 - 1u) / 8;
        const uint32_t tgcY = (targetSize.y + 8 - 1u) / 8;
        cmdList.PushConstant(pc, reinterpret_cast<const uint8_t*>(&uPc));
        cmdList.Dispatch(tgcX, tgcY, 1);
    } else {
        CORE_LOG_ONCE_W(to_hex(uintptr_t(this)), "RenderNodeCameraWeather: dispatchResources needed");
    }
}

void RenderNodeCameraWeather::ComputeTransmittance(RENDER_NS::IRenderCommandList& cmdList)
{
    RENDER_DEBUG_MARKER_SCOPE(cmdList, "TransmittanceLut");
    if (!RenderHandleUtil::IsValid(transmittanceShader_)) {
        return;
    }
    cmdList.BindPipeline(skyPso_.transmittanceLut);

    skyBinder_.transmittanceLut->ClearBindings();
    BindableImage bindable;
    bindable.handle = transmittanceLutHandle_.GetHandle();

    skyBinder_.transmittanceLut->BindImage(0, 0, bindable);

    const auto descHandle = skyBinder_.transmittanceLut->GetDescriptorSetHandle(0);
    const auto bindings = skyBinder_.transmittanceLut->GetDescriptorSetLayoutBindingResources(0);
    cmdList.UpdateDescriptorSet(descHandle, bindings);
    const auto descHandles = skyBinder_.transmittanceLut->GetDescriptorSetHandles();
    cmdList.BindDescriptorSets(0, descHandles);

    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const auto TextureImageDesc = gpuResourceMgr.GetImageDescriptor(bindable.handle);

    struct PushConstantLocalStruct {
        Math::Vec4 rayleighScatteringBase; // .w = mieAbsorptionBase
        Math::Vec4 ozoneAbsorptionBase;    // .w = mieScatteringBase
    } uPc;

    if (TextureImageDesc.width > 1u && TextureImageDesc.height > 1u) {
        const Math::UVec3 targetSize = { TextureImageDesc.width, TextureImageDesc.height, 1 };

        const uint32_t tgcX = (targetSize.x + 8 - 1u) / 8;
        const uint32_t tgcY = (targetSize.y + 8 - 1u) / 8;

        constexpr PushConstant pc { ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT,
            sizeof(PushConstantLocalStruct) };

        uPc.rayleighScatteringBase = Math::Vec4(settings_.rayleighScatteringBase, settings_.mieAbsorptionBase);
        uPc.ozoneAbsorptionBase = Math::Vec4(settings_.ozoneAbsorptionBase, settings_.mieScatteringBase);

        cmdList.PushConstantData(pc, arrayviewU8(uPc));
        cmdList.Dispatch(tgcX, tgcY, 1);
    } else {
        CORE_LOG_W("RenderNodeCameraWeather: dispatchResources needed");
    }
}

void RenderNodeCameraWeather::ComputeMultipleScattering(RENDER_NS::IRenderCommandList& cmdList)
{
    RENDER_DEBUG_MARKER_SCOPE(cmdList, "MultiScatteringLut");
    if (!RenderHandleUtil::IsValid(multipleScatteringShader_)) {
        return;
    }
    cmdList.BindPipeline(skyPso_.multipleScatteringLut);

    skyBinder_.multipleScatteringLut->ClearBindings();
    BindableImage bindable;
    BindableImage transmittanceBindable;
    bindable.handle = multipleScatteringLutHandle_.GetHandle();
    transmittanceBindable.handle = transmittanceLutHandle_.GetHandle();
    transmittanceBindable.samplerHandle = defaultSamplers_.linearHandle;

    skyBinder_.multipleScatteringLut->BindImage(0, 0, bindable);
    skyBinder_.multipleScatteringLut->BindImage(0, 1, transmittanceBindable);

    const auto descHandle = skyBinder_.multipleScatteringLut->GetDescriptorSetHandle(0);
    const auto bindings = skyBinder_.multipleScatteringLut->GetDescriptorSetLayoutBindingResources(0);
    cmdList.UpdateDescriptorSet(descHandle, bindings);
    const auto descHandles = skyBinder_.multipleScatteringLut->GetDescriptorSetHandles();
    cmdList.BindDescriptorSets(0, descHandles);

    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const auto TextureImageDesc = gpuResourceMgr.GetImageDescriptor(bindable.handle);

    struct PushConstantLocalStruct {
        Math::Vec4 rayleighScatteringBase; // .w = mieAbsorptionBase
        Math::Vec4 ozoneAbsorptionBase;    // .w = mieScatteringBase
    } uPc;

    if (TextureImageDesc.width > 1u && TextureImageDesc.height > 1u) {
        const Math::UVec3 targetSize = { TextureImageDesc.width, TextureImageDesc.height, 1 };

        const uint32_t tgcX = targetSize.x;
        const uint32_t tgcY = targetSize.y;

        constexpr PushConstant pc { ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT,
            sizeof(PushConstantLocalStruct) };

        uPc.rayleighScatteringBase = Math::Vec4(settings_.rayleighScatteringBase, settings_.mieAbsorptionBase);
        uPc.ozoneAbsorptionBase = Math::Vec4(settings_.ozoneAbsorptionBase, settings_.mieScatteringBase);

        cmdList.PushConstantData(pc, arrayviewU8(uPc));
        cmdList.Dispatch(tgcX, tgcY, 1);
    } else {
        CORE_LOG_W("RenderNodeCameraWeather: dispatchResources needed");
    }
}

void RenderNodeCameraWeather::ComputeAerialPerspective(RENDER_NS::IRenderCommandList& cmdList)
{
    RENDER_DEBUG_MARKER_SCOPE(cmdList, "AerialPerspectiveLut");
    if (!RenderHandleUtil::IsValid(aerialPerspectiveShader_)) {
        return;
    }

    cmdList.BindPipeline(skyPso_.aerialPerspectiveLut);

    skyBinder_.aerialPerspectiveLut->ClearBindings();
    BindableImage aerialOut;
    BindableImage transmittanceBindable;
    BindableImage multipleScatteringBindable;
    BindableImage shadowMapBindable;
    BindableBuffer aerialPerspectiveBindableBuffer;
    aerialOut.handle = aerialPerspectiveLutHandle_.GetHandle();
    transmittanceBindable.handle = transmittanceLutHandle_.GetHandle();
    transmittanceBindable.samplerHandle = defaultSamplers_.linearHandle;
    multipleScatteringBindable.handle = multipleScatteringLutHandle_.GetHandle();
    multipleScatteringBindable.samplerHandle = defaultSamplers_.linearHandle;
    aerialPerspectiveBindableBuffer.handle = aerialPerspectiveBuffer_.GetHandle();

    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    auto shadowSampler =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_PCF_SHADOW_SAMPLER);
    shadowMapBindable.handle = gpuResourceMgr.GetImageHandle(
        stores_.dataStoreNameScene.c_str() + DefaultMaterialLightingConstants::SHADOW_DEPTH_BUFFER_NAME);
    shadowMapBindable.samplerHandle = shadowSampler;

    UpdateAerialPerspectiveParams(
        Math::Mat4X4(currentScene_.camData.camera.matrices.proj * currentScene_.camData.camera.matrices.view));
    UpdateBuffer(
        renderNodeContextMgr_->GetGpuResourceManager(), aerialPerspectiveBuffer_.GetHandle(), aerialPerspectiveParams_);

    skyBinder_.aerialPerspectiveLut->BindImage(0, 0, aerialOut);
    skyBinder_.aerialPerspectiveLut->BindImage(0, 1, transmittanceBindable);
    skyBinder_.aerialPerspectiveLut->BindImage(0, 2, multipleScatteringBindable); // 2: parm
    skyBinder_.aerialPerspectiveLut->BindBuffer(0, 3, aerialPerspectiveBindableBuffer); // 3: parm
    skyBinder_.aerialPerspectiveLut->BindImage(0, 4, shadowMapBindable); // 4: parm

    const auto descHandle = skyBinder_.aerialPerspectiveLut->GetDescriptorSetHandle(0);
    const auto bindings = skyBinder_.aerialPerspectiveLut->GetDescriptorSetLayoutBindingResources(0);
    cmdList.UpdateDescriptorSet(descHandle, bindings);
    const auto descHandles = skyBinder_.aerialPerspectiveLut->GetDescriptorSetHandles();
    cmdList.BindDescriptorSets(0, descHandles);

    const auto TextureImageDesc = gpuResourceMgr.GetImageDescriptor(aerialOut.handle);
    if (TextureImageDesc.width > 1u && TextureImageDesc.height > 1u) {
        const Math::UVec2 targetSize = { TextureImageDesc.width, TextureImageDesc.height };

        const uint32_t tgcX = ((targetSize.x / 8) + 16 - 1u) / 16;
        const uint32_t tgcY = ((targetSize.y / 4) + 16 - 1u) / 16;
        cmdList.Dispatch(tgcX, tgcY, 1);
    }
}

void RenderNodeCameraWeather::ComputeSkyView(RENDER_NS::IRenderCommandList& cmdList)
{
    RENDER_DEBUG_MARKER_SCOPE(cmdList, "SkyViewLut");
    if (!RenderHandleUtil::IsValid(skyViewShader_)) {
        return;
    }
    cmdList.BindPipeline(skyPso_.skyViewLut);

    skyBinder_.skyViewLut->ClearBindings();
    BindableImage bindable;
    BindableImage transmittanceBindable;
    BindableImage multipleScatteringBindable;
    bindable.handle = skyViewLutHandle_.GetHandle();
    transmittanceBindable.handle = transmittanceLutHandle_.GetHandle();
    transmittanceBindable.samplerHandle = defaultSamplers_.linearHandle;
    multipleScatteringBindable.handle = multipleScatteringLutHandle_.GetHandle();
    multipleScatteringBindable.samplerHandle = defaultSamplers_.linearHandle;

    skyBinder_.skyViewLut->BindImage(0, 0, bindable);
    skyBinder_.skyViewLut->BindImage(0, 1, transmittanceBindable);
    skyBinder_.skyViewLut->BindImage(0, 2, multipleScatteringBindable); // 2: parm

    const auto descHandle = skyBinder_.skyViewLut->GetDescriptorSetHandle(0);
    const auto bindings = skyBinder_.skyViewLut->GetDescriptorSetLayoutBindingResources(0);
    cmdList.UpdateDescriptorSet(descHandle, bindings);
    const auto descHandles = skyBinder_.skyViewLut->GetDescriptorSetHandles();
    cmdList.BindDescriptorSets(0, descHandles);

    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const auto TextureImageDesc = gpuResourceMgr.GetImageDescriptor(bindable.handle);

    struct PushConstantLocalStruct {
        Math::Vec4 camPosSunElevation;     // .xyz: camera position, .w: sun elevation
        Math::Vec4 rayleighScatteringBase; // .w = mieAbsorptionBase
        Math::Vec4 ozoneAbsorptionBase;    // .w = mieScatteringBase
        Math::Vec4 groundColor;            // .w = skyViewBrightness
    } uPc;

    if (TextureImageDesc.width > 1u && TextureImageDesc.height > 1u) {
        const Math::UVec3 targetSize = { TextureImageDesc.width, TextureImageDesc.height, 1 };

        const uint32_t tgcX = (targetSize.x + 8 - 1u) / 8;
        const uint32_t tgcY = (targetSize.y + 8 - 1u) / 8;

        constexpr PushConstant pc { ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT,
            sizeof(PushConstantLocalStruct) };

        uPc.camPosSunElevation = Math::Vec4(cameraPos_, settings_.sunDirElevation.w);
        uPc.rayleighScatteringBase = Math::Vec4(settings_.rayleighScatteringBase, settings_.mieAbsorptionBase);
        uPc.ozoneAbsorptionBase = Math::Vec4(settings_.ozoneAbsorptionBase, settings_.mieScatteringBase);
        uPc.groundColor = Math::Vec4(settings_.groundColor, settings_.skyViewBrightness);

        cmdList.PushConstantData(pc, arrayviewU8(uPc));
        cmdList.Dispatch(tgcX, tgcY, 1);
    } else {
        CORE_LOG_W("RenderNodeCameraWeather: dispatchResources needed");
    }
}

void RenderNodeCameraWeather::ComputeSkyViewCubemap(RENDER_NS::IRenderCommandList& cmdList)
{
    RENDER_DEBUG_MARKER_SCOPE(cmdList, "SkyViewCubemap");
    if (!RenderHandleUtil::IsValid(skyCubemapShader_)) {
        return;
    }

    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    cmdList.BindPipeline(skyPso_.skyCubemap);

    struct PushConstantStruct {
        Math::Vec4 sunDir;              // w: elevation
        Math::Vec4 cameraPosBrightness; // x: sky view intensity
    } uPc;

    skyBinder_.skyCubemap->ClearBindings();
    BindableImage bindable;
    BindableImage skyViewBindable;
    bindable.handle = skyCubemapHandle_.GetHandle();
    bindable.mip = 0;
    skyViewBindable.handle = skyViewLutHandle_.GetHandle();
    skyViewBindable.samplerHandle = defaultSamplers_.linearHandle;

    skyBinder_.skyCubemap->BindImage(0, 0, bindable);
    skyBinder_.skyCubemap->BindImage(0, 1, skyViewBindable);

    const auto descHandle = skyBinder_.skyCubemap->GetDescriptorSetHandle(0);
    const auto bindings = skyBinder_.skyCubemap->GetDescriptorSetLayoutBindingResources(0);
    cmdList.UpdateDescriptorSet(descHandle, bindings);
    const auto descHandles = skyBinder_.skyCubemap->GetDescriptorSetHandles();
    cmdList.BindDescriptorSets(0, descHandles);

    const auto TextureImageDesc = gpuResourceMgr.GetImageDescriptor(bindable.handle);

    if (TextureImageDesc.width > 1u && TextureImageDesc.height > 1u) {
        const Math::UVec3 targetSize = { TextureImageDesc.width, TextureImageDesc.height, 1 };
        const uint32_t tgcX = (targetSize.x + 8 - 1u) / 8;
        const uint32_t tgcY = (targetSize.y + 8 - 1u) / 8;

        constexpr PushConstant pc { ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT, sizeof(PushConstantStruct) };
        uPc.sunDir = settings_.sunDirElevation;
        uPc.cameraPosBrightness = Math::Vec4(cameraPos_, settings_.skyViewBrightness);

        cmdList.PushConstantData(pc, arrayviewU8(uPc));
        cmdList.Dispatch(tgcX, tgcY, 6); // 6: parm
    } else {
        CORE_LOG_W("RenderNodeCameraWeather: dispatchResources needed");
    }
}

void RenderNodeCameraWeather::ComputeSkyViewCubemapMips(RENDER_NS::IRenderCommandList& cmdList)
{
    RENDER_DEBUG_MARKER_SCOPE(cmdList, "SkyViewCubemapMipsCompute");
    if (!RenderHandleUtil::IsValid(skyCubemapMipsShader_)) {
        return;
    }
    const GpuImageDesc desc =
        renderNodeContextMgr_->GetGpuResourceManager().GetImageDescriptor(skyCubemapHandle_.GetHandle());
    Math::UVec2 currSize = { desc.width, desc.height };

    ImageSubresourceRange imageSubresourceRange { CORE_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,
        PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };

    cmdList.BeginDisableAutomaticBarrierPoints();

    // Initial transition: mip 0 should already be in SHADER_READ_ONLY from previous pass
    // Transition all other mips to GENERAL for writing
    for (uint32_t mipLevel = 1; mipLevel < desc.mipCount; mipLevel++) {
        imageSubresourceRange.baseMipLevel = mipLevel;
        if (isFirstFrame) {
            cmdList.CustomImageBarrier(skyCubemapHandle_.GetHandle(), SRC_UNDEFINED, GENERAL, imageSubresourceRange);
        } else {
            cmdList.CustomImageBarrier(skyCubemapHandle_.GetHandle(), FINAL_DST, GENERAL, imageSubresourceRange);
        }
    }
    cmdList.AddCustomBarrierPoint();

    for (uint32_t mipLevel = 1; mipLevel < desc.mipCount; mipLevel++) {
        // Calculating all mip levels each frame on mobile is slow
        // Lower resolution mips can be updated less often, without sacrificing visual fidelity
        if (mipLevel != 1 && !needsFullMipUpdate) {
            if (frameNumber % (mipLevel * 2U) != 0 &&
                frameNumber > 3U) { // OpenGL is cleaning the textures on first frame
                continue;
            }
            currSize.x = Math::max(1u, currSize.x / 2u);
            currSize.y = Math::max(1u, currSize.y / 2u);
        }

        // Transition both mips
        imageSubresourceRange.baseMipLevel = mipLevel - 1;
        cmdList.CustomImageBarrier(skyCubemapHandle_.GetHandle(), GENERAL, COMPUTE_READ, imageSubresourceRange);
        cmdList.AddCustomBarrierPoint();

        cmdList.BindPipeline(skyPso_.skyCubemapMips);

        auto& binder = *skyBinder_.skyCubemapMips[mipLevel];
        binder.ClearBindings();

        BindableImage bindable;
        bindable.handle = skyCubemapHandle_.GetHandle();
        bindable.mip = mipLevel - 1;
        bindable.samplerHandle = defaultSamplers_.cubemapHandle;
        binder.BindImage(0U, 0U, bindable);

        BindableImage outputBindable;
        outputBindable.handle = skyCubemapHandle_.GetHandle();
        outputBindable.mip = mipLevel;
        binder.BindImage(0U, 1U, outputBindable);

        const auto& bindings = binder.GetDescriptorSetLayoutBindingResources(0);
        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(0), bindings);
        cmdList.BindDescriptorSet(0U, binder.GetDescriptorSetHandle(0));

        const Math::UVec3 targetSize = { currSize.x, currSize.y, 1 };

        const uint32_t tgcX = (targetSize.x + 8 - 1u) / 8;
        const uint32_t tgcY = (targetSize.y + 8 - 1u) / 8;

        cmdList.Dispatch(tgcX, tgcY, 6); // 6: parm
    }

    // Final barrier: transition ALL mip levels to SHADER_READ_ONLY_OPTIMAL for graphics pipeline
    for (uint32_t mipLevel = 0; mipLevel < desc.mipCount; mipLevel++) {
        imageSubresourceRange.baseMipLevel = mipLevel;

        if (mipLevel == desc.mipCount - 1) {
            // Last mip is in GENERAL after being written to
            cmdList.CustomImageBarrier(skyCubemapHandle_.GetHandle(), GENERAL, FINAL_DST, imageSubresourceRange);
        } else {
            // All other mips (0 to N-2) are in COMPUTE_READ
            cmdList.CustomImageBarrier(skyCubemapHandle_.GetHandle(), COMPUTE_READ, FINAL_DST, imageSubresourceRange);
        }
    }

    cmdList.AddCustomBarrierPoint();
    cmdList.EndDisableAutomaticBarrierPoints();
}

void RenderNodeCameraWeather::UpdateCurrentScene(const IRenderDataStoreDefaultScene& dataStoreScene,
    const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultLight& dataStoreLight)
{
    const auto scene = dataStoreScene.GetScene();
    currentScene_.camData = RenderNodeSceneUtil::GetSceneCameraData(
        dataStoreScene, dataStoreCamera, jsonInputs_.customCameraId, jsonInputs_.customCameraName);
    const auto& cam = currentScene_.camData.camera;
    const auto camHandles = renderNodeSceneUtil_->GetSceneCameraImageHandles(
        *renderNodeContextMgr_, stores_.dataStoreNameScene, cam.name, cam);
    currentScene_.cameraEnvRadianceHandle = camHandles.radianceCubemap;

    const IRenderDataStoreDefaultLight::LightCounts lightCounts = dataStoreLight.GetLightCounts();
    currentScene_.hasShadow = ((lightCounts.dirShadow > 0) || (lightCounts.spotShadow > 0)) ? true : false;
    currentScene_.shadowTypes = dataStoreLight.GetShadowTypes();
    currentScene_.lightingFlags = dataStoreLight.GetLightingFlags();
    const uint64_t envId = currentScene_.camData.camera.environment.id;
    if (currentScene_.envId != envId) {
        currentScene_.envId = envId;

        auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        GpuImageDesc descCubemap { CORE_IMAGE_TYPE_2D, ImageViewType::CORE_IMAGE_VIEW_TYPE_CUBE,
            BASE_FORMAT_B10G11R11_UFLOAT_PACK32, CORE_IMAGE_TILING_OPTIMAL,
            CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_STORAGE_BIT,
            CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
            CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, CUBEMAP_RESOLUTION.x, CUBEMAP_RESOLUTION.y, 1U,
            cubeMapMipCount, 6U, CORE_SAMPLE_COUNT_1_BIT, ComponentMapping {} };
        // NOTE: the skyCubemapMipsHandle is created based on demand because it needs the environment id
        if (deviceBackendType_ != Render::DeviceBackendType::VULKAN) {
            descCubemap.format = BASE_FORMAT_R16G16B16A16_SFLOAT;
        }
        skyCubemapHandle_ = gpuResourceMgr.Create(
            stores_.dataStoreNameScene + DefaultMaterialSceneConstants::ENVIRONMENT_RADIANCE_CUBEMAP_PREFIX_NAME +
                to_hex(envId),
            descCubemap);
    }
}

void RenderNodeCameraWeather::GetSceneUniformBuffers(const string_view us)
{
    if (renderNodeSceneUtil_) {
        sceneBuffers_ = renderNodeSceneUtil_->GetSceneBufferHandles(*renderNodeContextMgr_, stores_.dataStoreNameScene);
    }

    string camName;
    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        camName = to_string(jsonInputs_.customCameraId);
    } else if (!(jsonInputs_.customCameraName.empty())) {
        camName = jsonInputs_.customCameraName;
    }
    if (renderNodeSceneUtil_) {
        cameraBuffers_ = renderNodeSceneUtil_->GetSceneCameraBufferHandles(
            *renderNodeContextMgr_, stores_.dataStoreNameScene, camName);
    }
}

void RenderNodeCameraWeather::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.customCameraName = parserUtil.GetStringValue(jsonVal, "customCameraName");
    jsonInputs_.customCameraId = parserUtil.GetUintValue(jsonVal, "customCameraId");

    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();

    if (deviceBackendType_ == Render::DeviceBackendType::VULKAN) {
        // transmittance lut
        transmittanceShader_ = shaderMgr.GetShaderHandle("3dshaders://computeshader/transmittance_lut.shader");
        // multiple scattering lut
        multipleScatteringShader_ =
            shaderMgr.GetShaderHandle("3dshaders://computeshader/multiple_scattering_lut.shader");
        // aerial perspective lut
        aerialPerspectiveShader_ = shaderMgr.GetShaderHandle("3dshaders://computeshader/aerial_perspective_lut.shader");
        // sky view lut
        skyViewShader_ = shaderMgr.GetShaderHandle("3dshaders://computeshader/sky_view_lut.shader");
        // sky view cubemap
        skyCubemapShader_ = shaderMgr.GetShaderHandle("3dshaders://computeshader/sky_view_cubemap.shader");
        // sky view cubemap mips
        skyCubemapMipsShader_ = shaderMgr.GetShaderHandle("3dshaders://computeshader/sky_view_cubemap_mips.shader");
    } else {
        // transmittance lut
        transmittanceShader_ = shaderMgr.GetShaderHandle("3dshaders://computeshader/transmittance_lut_gl.shader");
        // multiple scattering lut
        multipleScatteringShader_ =
            shaderMgr.GetShaderHandle("3dshaders://computeshader/multiple_scattering_lut_gl.shader");
        // sky view lut
        skyViewShader_ = shaderMgr.GetShaderHandle("3dshaders://computeshader/sky_view_lut_gl.shader");
        // sky view cubemap
        skyCubemapShader_ = shaderMgr.GetShaderHandle("3dshaders://computeshader/sky_view_cubemap_gl.shader");
        // sky view cubemap mips
        skyCubemapMipsShader_ = shaderMgr.GetShaderHandle("3dshaders://computeshader/sky_view_cubemap_mips_gl.shader");
    }
    // Cloud volume shader
    cloudVolumeShader_ = shaderMgr.GetShaderHandle("3dshaders://computeshader/cloud_volume.shader");
    // Cloud WeatherMap generation shader
    cloudWeatherGenShader_ = shaderMgr.GetShaderHandle("3dshaders://computeshader/cloud_weathermap_gen.shader");

    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        cameraName_ = to_string(jsonInputs_.customCameraId);
    } else if (!(jsonInputs_.customCameraName.empty())) {
        cameraName_ = jsonInputs_.customCameraName;
    }
}

// for plugin / factory interface
IRenderNode* RenderNodeCameraWeather::Create()
{
    return new RenderNodeCameraWeather();
}

void RenderNodeCameraWeather::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeCameraWeather*>(instance);
}
CORE3D_END_NAMESPACE()
