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

#include "weather_system.h"

#include <ComponentTools/component_query.h>
#include <algorithm>
#include <cfloat>
#include <cinttypes>

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/layer_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/planar_reflection_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/water_ripple_component.h>
#include <3d/ecs/components/weather_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/ecs/systems/intf_weather_system.h>
#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_picking.h>
#include <base/math/quaternion_util.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_class_factory.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/scoped_handle.h>
#include <core/property_tools/property_macros.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>

#include "3d/shaders/common/water_ripple_common.h"
#include "ecs/systems/render_preprocessor_system.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

CORE3D_BEGIN_NAMESPACE()
namespace {
constexpr uint32_t RESOLUTION { 512u };

static constexpr string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
#if (CORE3D_VALIDATION_ENABLED == 1)
static constexpr string_view RIPPLE_TEXTURE_NAME_0 { "RIPPLE_RENDER_NODE_TEXTURE_0_" };
static constexpr string_view RIPPLE_TEXTURE_NAME_1 { "RIPPLE_RENDER_NODE_TEXTURE_1_" };
#endif
static constexpr string_view RIPPLE_INPUT_BUFFER_NAME { "RIPPLE_RENDER_NODE_INPUTBUFFER" };
static constexpr uint32_t MAX_REFLECTION_CAMERAS = 16U;

// We could just have variants in one shader file, but both variants would use the same renderslot so ShaderManager
// validation complains at ShaderManager::CreateBaseShaderPathsAndHashes(...)
static constexpr string_view CLOUD_SHADER_NAME { "3dshaders://shader/clouds/core3d_dm_fw_cloud.shader" };

EntityReference CreateTargetGpuImage(IEcs& ecs, IGpuResourceManager& gpuResourceMgr, const Math::UVec2& size,
    const string_view name, BASE_NS::Format colorFormat)
{
    GpuImageDesc desc;
    desc.width = size.x;
    desc.height = size.y;
    desc.depth = 1;
    desc.format = colorFormat;
    desc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.usageFlags =
        ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_STORAGE_BIT |
        ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_DST_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
    desc.imageType = ImageType::CORE_IMAGE_TYPE_2D;
    desc.imageTiling = ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
    desc.imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
    desc.engineCreationFlags = EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;

    RenderHandleReference rhr;
    if (!name.empty()) {
        rhr = gpuResourceMgr.Create(name, desc);
    } else {
        rhr = gpuResourceMgr.Create(desc);
    }
    auto handleManager = GetManager<IRenderHandleComponentManager>(ecs);
    return GetOrCreateEntityReference(ecs.GetEntityManager(), *handleManager, rhr);
}

EntityReference CreateBuffer(
    IEcs& ecs, IGpuResourceManager& gpuResourceMgr, const string_view& name, const GpuBufferDesc& bufferDesc)
{
    auto rhr = gpuResourceMgr.Create(name, bufferDesc);
    auto handleManager = GetManager<IRenderHandleComponentManager>(ecs);
    return GetOrCreateEntityReference(ecs.GetEntityManager(), *handleManager, rhr);
}

WeatherSystem::RipplePlaneResources InitSimulationResources(
    IEcs& ecs, IGraphicsContext& graphicsContext, uint32_t inResolution, const Entity& entity)
{
    auto& renderContext = graphicsContext.GetRenderContext();
    auto& device = renderContext.GetDevice();
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    WeatherSystem::RipplePlaneResources mTestRes;
    // create textures for ripple simulation
    {
#if (CORE3D_VALIDATION_ENABLED == 1)
        mTestRes.rippleTextures[0] = CreateTargetGpuImage(ecs, gpuResourceMgr, { inResolution, inResolution },
            RIPPLE_TEXTURE_NAME_0 + "scene" + to_string(ecs.GetId()) + "_entity" + to_string(entity.id),
            Format::BASE_FORMAT_R32G32B32A32_SFLOAT);
        mTestRes.rippleTextures[1] = CreateTargetGpuImage(ecs, gpuResourceMgr, { inResolution, inResolution },
            RIPPLE_TEXTURE_NAME_1 + "scene" + to_string(ecs.GetId()) + "_entity" + to_string(entity.id),
            Format::BASE_FORMAT_R32G32B32A32_SFLOAT);
#else
        mTestRes.rippleTextures[0] = CreateTargetGpuImage(
            ecs, gpuResourceMgr, { inResolution, inResolution }, {}, Format::BASE_FORMAT_R32G32B32A32_SFLOAT);
        mTestRes.rippleTextures[1] = CreateTargetGpuImage(
            ecs, gpuResourceMgr, { inResolution, inResolution }, {}, Format::BASE_FORMAT_R32G32B32A32_SFLOAT);
#endif

        GpuBufferDesc inputArgsBuffer;
        inputArgsBuffer.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        inputArgsBuffer.memoryPropertyFlags =
            CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        inputArgsBuffer.usageFlags = CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        inputArgsBuffer.byteSize = sizeof(DefaultWaterRippleDataStruct);
        mTestRes.rippleArgsBufferHandle =
            CreateBuffer(ecs, gpuResourceMgr, RIPPLE_INPUT_BUFFER_NAME + to_string(entity.id), inputArgsBuffer);
    }

    return mTestRes;
}

// nodeEntity should have material component
WeatherSystem::RipplePlaneResources CreateSimulationResourcesForEntity(
    IEcs& ecs, IGraphicsContext& graphicsContext, const Entity& nodeEntity)
{
    WeatherSystem::RipplePlaneResources planeRes;

    const bool bindlessEnabled =
        graphicsContext.GetCreateInfo().createFlags & IGraphicsContext::CreateInfo::ENABLE_BINDLESS_PIPELINES_BIT;

    auto* renderMeshCM = GetManager<IRenderMeshComponentManager>(ecs);
    auto* meshCM = GetManager<IMeshComponentManager>(ecs);
    auto* matCM = GetManager<IMaterialComponentManager>(ecs);
    auto* gpuHandleCM = GetManager<IRenderHandleComponentManager>(ecs);

    auto& entityMgr = ecs.GetEntityManager();
    auto& device = graphicsContext.GetRenderContext().GetDevice();
    IShaderManager& shaderMgr = device.GetShaderManager();
    if (!(renderMeshCM && meshCM && matCM && gpuHandleCM)) {
        return {};
    }

    planeRes = InitSimulationResources(ecs, graphicsContext, RESOLUTION, nodeEntity);
    planeRes.entityID = nodeEntity;

    if (const auto rmcHandle = renderMeshCM->Read(nodeEntity); rmcHandle) {
        const RenderMeshComponent& rmc = *rmcHandle;
        if (const auto meshHandle = meshCM->Read(rmc.mesh); meshHandle) {
            const MeshComponent& meshComponent = *meshHandle;
            if (!meshComponent.submeshes.empty()) {
                if (auto matHandle = matCM->Write(meshComponent.submeshes[0].material); matHandle) {
                    MaterialComponent& matComponent = *matHandle;
                    const string variant = bindlessEnabled ? "Bindless" : "Default";
                    auto shader =
                        shaderMgr.GetShaderHandle("3dshaders://shader/ripple/custom_water_ripple.shader", variant);
                    matComponent.materialShader.shader = GetOrCreateEntityReference(entityMgr, *gpuHandleCM, shader);
                    // Set RippleEffect texture as material sheen texture.
                    matComponent.textures[MaterialComponent::TextureIndex::SHEEN].image =
                        planeRes.rippleTextures[planeRes.currentTextureIndex];
                }
            }
        }
    }

    return planeRes;
}

constexpr RenderDataStoreWeather::CloudRenderingType GetCloudRenderingType(
    const WeatherComponent::CloudRenderingType ct)
{
    switch (ct) {
        case WeatherComponent::CloudRenderingType::FULL:
            return RenderDataStoreWeather::CloudRenderingType::FULL;
            break;
        case WeatherComponent::CloudRenderingType::DOWNSCALED:
            return RenderDataStoreWeather::CloudRenderingType::DOWNSCALED;
            break;
        case WeatherComponent::CloudRenderingType::REPROJECTED:
            return RenderDataStoreWeather::CloudRenderingType::REPROJECTED;
            break;
        default:
            return RenderDataStoreWeather::CloudRenderingType::FULL;
    }
}
} // namespace

WeatherSystem::WeatherSystem(IEcs& ecs)
    : ecs_(ecs), materialManager_(*GetManager<IMaterialComponentManager>(ecs)),
      meshManager_(*GetManager<IMeshComponentManager>(ecs)),
      renderMeshManager_(*GetManager<IRenderMeshComponentManager>(ecs)),
      renderHandleManager_(*GetManager<IRenderHandleComponentManager>(ecs)),
      transformationManager_(*GetManager<ITransformComponentManager>(ecs)),
      renderConfigManager_(*GetManager<IRenderConfigurationComponentManager>(ecs)),
      camManager_(*GetManager<ICameraComponentManager>(ecs)),
      envManager_(*GetManager<IEnvironmentComponentManager>(ecs)),
      weatherManager_(*GetManager<IWeatherComponentManager>(ecs)),
      layerManager_(*GetManager<ILayerComponentManager>(ecs)),
      rippleComponentManager_(*GetManager<IWaterRippleComponentManager>(ecs)),
      planarReflectionManager_(*GetManager<IPlanarReflectionComponentManager>(ecs))
{
    auto classRegister = ecs.GetClassFactory().GetInterface<IClassRegister>();
    if (classRegister) {
        renderContext_ = GetInstance<IRenderContext>(*classRegister, UID_RENDER_CONTEXT);
        if (renderContext_) {
            if (auto renderContextClassRegister = renderContext_->GetInterface<IClassRegister>()) {
                graphicsContext_ = GetInstance<IGraphicsContext>(*renderContextClassRegister, UID_GRAPHICS_CONTEXT);
            }
            gpuResourceManager_ = &renderContext_->GetDevice().GetGpuResourceManager();
            renderDataStoreManager_ = &renderContext_->GetRenderDataStoreManager();
        }
    }
    ecs.AddListener(rippleComponentManager_, *this);
    ecs.AddListener(planarReflectionManager_, *this);

    Initialize();
}

Entity WeatherSystem::GetMaterialEntity(const Entity& entity)
{
    Entity materialEntity;
    if (const auto rmcHandle = renderMeshManager_.Read(entity); rmcHandle) {
        const RenderMeshComponent& rmc = *rmcHandle;
        if (const auto meshHandle = meshManager_.Read(rmc.mesh); meshHandle) {
            const MeshComponent& meshComponent = *meshHandle;
            if (!meshComponent.submeshes.empty()) {
                materialEntity = meshComponent.submeshes[0].material;
            }
        }
    }
    return materialEntity;
}

WeatherSystem::RipplePlaneResources WeatherSystem::GetOrCreateSimulationResources(const Entity& entity)
{
    if (!graphicsContext_) {
        return {};
    }
    auto iter = handleResources_.find(entity);
    if (iter != handleResources_.end()) {
        return iter->second;
    }
    auto res = CreateSimulationResourcesForEntity(ecs_, *graphicsContext_, entity);
    handleResources_.insert({ entity, res });
    return res;
}

void WeatherSystem::Initialize()
{
    const ComponentQuery::Operation operations[] = { { rippleComponentManager_, ComponentQuery::Operation::OPTIONAL } };
    query_.SetEcsListenersEnabled(true);
    query_.SetupQuery(rippleComponentManager_, operations);

    const ComponentQuery::Operation waterPlaneOps[] = {
        { planarReflectionManager_, ComponentQuery::Operation::REQUIRE },
    };

    waterPlaneQuery_.SetEcsListenersEnabled(false);
    waterPlaneQuery_.SetupQuery(planarReflectionManager_, waterPlaneOps, true);
    // NOTE: data store is done deferred way in the update
    screenPercentages_.resize(MAX_REFLECTION_CAMERAS);
}

void WeatherSystem::Uninitialize()
{
    query_.SetEcsListenersEnabled(false);
}

WeatherSystem ::~WeatherSystem()
{
    Uninitialize();
    ecs_.RemoveListener(rippleComponentManager_, *this);
    ecs_.RemoveListener(planarReflectionManager_, *this);
    for (auto& kvpair : handleResources_) {
        kvpair.second = {};
    }
}

void WeatherSystem::SetActive(bool state)
{
    active_ = state;
}

bool WeatherSystem::IsActive() const
{
    return active_;
}

string_view WeatherSystem::GetName() const
{
    return ::GetName(this);
}

Uid WeatherSystem::GetUid() const
{
    return UID;
}

IPropertyHandle* WeatherSystem::GetProperties()
{
    return nullptr;
}

const IPropertyHandle* WeatherSystem::GetProperties() const
{
    return nullptr;
}

void WeatherSystem::SetProperties(const IPropertyHandle& dataHandle) {}

const IEcs& WeatherSystem::GetECS() const
{
    return ecs_;
}

inline constexpr uint32_t GetResDivider(RenderDataStoreWeather::CloudRenderingType crt)
{
    if (crt == RenderDataStoreWeather::CloudRenderingType::REPROJECTED) {
        return 4U;
    } else if (crt == RenderDataStoreWeather::CloudRenderingType::DOWNSCALED) {
        return 2U;
    } else {
        return 1U;
    }
}

bool WeatherSystem::Update(bool frameRenderingQueued, uint64_t /* time */, uint64_t delta)
{
    if (!renderDataStoreManager_ || !gpuResourceManager_) {
        return false;
    }
    if (!active_) {
        return false;
    }

    if (!dataStoreWeather_ && renderDataStoreManager_) {
        string dsName;
        if (IRenderPreprocessorSystem* rpSystem = GetSystem<IRenderPreprocessorSystem>(ecs_); rpSystem) {
            if (const auto in = ScopedHandle<IRenderPreprocessorSystem::Properties>(rpSystem->GetProperties()); in) {
                dsName = in->dataStorePrefix;
            }
        }
        dsName += RenderDataStoreWeather::TYPE_NAME;
        dataStoreWeather_ = refcnt_ptr<RenderDataStoreWeather>(
            renderDataStoreManager_->Create(RenderDataStoreWeather::UID, dsName.c_str()));
    }
    if (!dataStoreWeather_) {
        return false;
    }

    query_.Execute();

    ProcessWaterRipples();

    const auto cameraComponentCount = camManager_.GetComponentCount();
    const auto planarCount = planarReflectionManager_.GetComponentCount();
    bool cloudEnabledThisFrame = true;
    bool weatherUpdated = false;

    if (const auto weatherGeneration = weatherManager_.GetGenerationCounter();
        weatherGeneration_ != weatherGeneration) {
        weatherGeneration_ = weatherGeneration;
        weatherUpdated = true;
        RenderDataStoreWeather::WeatherSettings settings;
        const auto weatherComponents = weatherManager_.GetComponentCount();
        for (auto id = 0U; id < weatherComponents; ++id) {
            auto handle = weatherManager_.Read(id);
            if (!handle) {
                continue;
            }
            settings.timeOfDay = handle->timeOfDay;
            settings.moonBrightness = handle->moonBrightness;
            settings.nightGlow = handle->nightGlow;

            settings.skyViewBrightness = handle->skyViewBrightness;
            settings.worldScale = handle->worldScale;
            settings.maxAerialPerspective = handle->maxAerialPerspective;
            settings.rayleighScatteringBase = handle->rayleighScatteringBase;
            settings.ozoneAbsorptionBase = handle->ozoneAbsorptionBase;
            settings.mieScatteringBase = handle->mieScatteringBase;
            settings.mieAbsorptionBase = handle->mieAbsorptionBase;
            settings.groundColor = Math::Vec4 { handle->groundColor, 0.f };

            settings.windSpeed = handle->windSpeed;
            settings.density = handle->density;
            settings.cloudType = handle->cloudType;
            settings.coverage = handle->coverage;
            settings.curliness = handle->curliness;

            settings.windDir[0] = handle->windDir.x;
            settings.windDir[1] = handle->windDir.y;
            settings.windDir[2] = handle->windDir.z; // 2: index

            float timeOfDay = handle->timeOfDay;

            float sunAngle = ((timeOfDay - 6.0f) / 24.0f) * 2.0f * Math::PI;
            vec3 sunDir = Math::Vec4(cos(sunAngle), // east-west
                sin(sunAngle),                      // up-down
                sin(sunAngle + Math::PI * 0.25f),   // north-south, offset by 45 degrees
                0.0);
            sunDir = Math::Normalize(sunDir);
            float sunElevation = Math::asin(sunDir.y);

            settings.sunDirElevation = Math::Vec4(sunDir, sunElevation);

            auto transformManager = GetManager<ITransformComponentManager>(ecs_);
            auto transformHandle = transformManager->Write(handle->directionalSun);
            if (transformHandle) {
                settings.sunId = handle->directionalSun.id;
                if (sunElevation > 0.0) {
                    vec3 sunDirection = Math::Vec3(sunDir.x, sunDir.y, sunDir.z);
                    transformHandle->rotation = Math::LookRotation(sunDirection, Math::Vec3(0.0f, 1.0f, 0.0f));
                    transformHandle->scale = Math::Vec3(1.0, 1.0, 1.0);
                } else {
                    transformHandle->scale = Math::Vec3(0.0, 0.0, 0.0);
                }
            }

            settings.directIntensityDay = handle->directIntensityDay;
            settings.cloudTopOffset = handle->cloudTopOffset;
            settings.ambientIntensityDay = handle->ambientIntensityDay;
            settings.softness = handle->softness;
            settings.curliness = handle->curliness;

            settings.earthRadius = handle->earthRadius;
            settings.cloudBottomAltitude = handle->cloudBottomAltitude;
            settings.cloudTopAltitude = handle->cloudTopAltitude;
            settings.crispiness = handle->crispiness;

            settings.weatherScale = handle->weatherScale;
            settings.cloudHorizonFogHeight = handle->cloudHorizonFogHeight;
            settings.horizonFogFalloff = handle->horizonFogFalloff;
            settings.maxSamples = handle->maxSamples;
            settings.dayTopAmbientColor = handle->dayTopAmbientColor;

            settings.ambientColorHueSunset = handle->ambientColorHueSunset;
            settings.directColorHueSunset = handle->directColorHueSunset;
            settings.sunGlareColorDay = handle->sunGlareColorDay;
            settings.sunGlareColorSunset = handle->sunGlareColorSunset;

            settings.nightTopAmbientColor = handle->nightTopAmbientColor;
            settings.ambientIntensityNight = handle->ambientIntensityNight;
            settings.directLightIntensityNight = handle->directLightIntensityNight;
            settings.nightBottomAmbientColor = handle->nightBottomAmbientColor;
            settings.moonBaseColor = handle->moonBaseColor;
            settings.moonGlareColor = handle->moonGlareColor;
            settings.cloudOptimizationFlags = handle->cloudOptimizationFlags;

            settings.cloudRenderingType = GetCloudRenderingType(handle->cloudRenderingType);

            // store
            cloudMatData_.cloudRenderingType = settings.cloudRenderingType;

            if (auto* rhm = GetManager<IRenderHandleComponentManager>(ecs_)) {
                settings.lowFrequencyImage = rhm->GetRenderHandle(handle->lowFrequencyImage);
                settings.highFrequencyImage = rhm->GetRenderHandle(handle->highFrequencyImage);
                settings.curlNoiseImage = rhm->GetRenderHandle(handle->curlNoiseImage);
                settings.cirrusImage = rhm->GetRenderHandle(handle->cirrusImage);
                settings.weatherMapImage = rhm->GetRenderHandle(handle->weatherMapImage);
            }

            dataStoreWeather_->SetWeatherSettings(weatherManager_.GetEntity(id).id, settings);
            if (settings.coverage < 0.01 || settings.density < 0.0001) {
                cloudEnabledThisFrame = false;
            }
        }
    }

    // first check if there are any weather components
    if (weatherManager_.GetComponentCount() > 0U) {
        // then check the usage with cameras
        bool usesWeather { false };
        if (const auto rcHandle = renderConfigManager_.Read(0U)) {
            if (const auto envHandle = envManager_.Read(rcHandle->environment)) {
                if (envHandle->weather) {
                    usesWeather = true;
                }
            }
        }

        vector<uint64_t> ids;
        uint32_t index = 0;
        // the low 32 bits are exposed in editor for the user.
        constexpr uint64_t usedLayersMask = 0x0000000Fffffffff;
        auto* nodeManager = GetManager<INodeComponentManager>(ecs_);
        for (IComponentManager::ComponentId cameraId = 0; cameraId < cameraComponentCount; ++cameraId) {
            const Entity cameraEntity = camManager_.GetEntity(cameraId);
            if (auto nodeHandle = nodeManager->Read(cameraEntity); !nodeHandle || !nodeHandle->effectivelyEnabled) {
                continue;
            }
            IPropertyHandle* cameraHandle = camManager_.GetData(cameraId);
            uint64_t currentLayerMask {};
            if (auto readCamera = ScopedHandle<const CameraComponent>(cameraHandle)) {
                if (!(readCamera->sceneFlags & CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT)) {
                    continue;
                }
                currentLayerMask = readCamera->layerMask;
            }
            if (currentLayerMask & LayerFlagBits::CORE_LAYER_FLAG_BIT_32) {
                continue;
            }
            const auto layerBit = (LayerFlagBits::CORE_LAYER_FLAG_BIT_63 >> index);
            if (((currentLayerMask & ~usedLayersMask) != layerBit)) {
                if (auto writeCamera = ScopedHandle<CameraComponent>(cameraHandle)) {
                    writeCamera->layerMask = (currentLayerMask & usedLayersMask) | layerBit;
                }
            }
            index++;
            ids.push_back(camManager_.GetEntity(cameraId).id);
        }
        const auto activeCameras = index;

        for (IComponentManager::ComponentId reflectionId = 0; reflectionId < planarCount; ++reflectionId) {
            const Entity reflectionEntity = planarReflectionManager_.GetEntity(reflectionId);
            if (auto nodeHandle = nodeManager->Read(reflectionEntity); !nodeHandle || !nodeHandle->effectivelyEnabled) {
                continue;
            }
            IPropertyHandle* reflectionHandle = planarReflectionManager_.GetData(reflectionId);
            uint64_t currentLayerMask {};
            if (auto readReflection = ScopedHandle<const PlanarReflectionComponent>(reflectionHandle)) {
                if (!(readReflection->additionalFlags & PlanarReflectionComponent::FlagBits::ACTIVE_RENDER_BIT)) {
                    continue;
                }
                currentLayerMask = readReflection->layerMask;
            }
            const auto layerBit = (LayerFlagBits::CORE_LAYER_FLAG_BIT_63 >> index);
            if (((currentLayerMask & ~usedLayersMask) != layerBit)) {
                if (auto writeReflection = ScopedHandle<PlanarReflectionComponent>(reflectionHandle)) {
                    writeReflection->layerMask = (currentLayerMask & usedLayersMask) | layerBit;
                }
            }
            for (const auto& cameraEntity : array_view(ids.data(), activeCameras)) {
                index++;
                // NOTE: to link this to reflection cameras the ID should be hash of reflection plane enity and camera
                // entity. so render clouds for each planar reflection from every active camera.
                ids.push_back(Hash(reflectionEntity.id, cameraEntity));
            }
        }

        // Number of cameras that render sky and clouds
        cameraCount_ = index;

        // get largest camera resolution
        Math::UVec2 maxRes { 0U, 0U };
        const auto cameraCount = camManager_.GetComponentCount();
        for (IComponentManager::ComponentId id = 0; id < cameraCount; ++id) {
            ScopedHandle<const CameraComponent> handle = camManager_.Read(id);
            const CameraComponent& component = *handle;
            bool checkRes = usesWeather;
            if (!usesWeather) {
                if (auto envHandle = envManager_.Read(component.environment); envHandle && envHandle->weather) {
                    checkRes = true;
                }
            }
            if (checkRes) {
                maxRes.x = Math::max(maxRes.x, component.renderResolution.x);
                maxRes.y = Math::max(maxRes.y, component.renderResolution.y);
            }
        }

        // possible resize
        auto& cm = cloudMatData_;

        const uint32_t resDevider = GetResDivider(cm.cloudRenderingType);
        maxRes.x = maxRes.x / resDevider;
        maxRes.y = maxRes.y / resDevider;
        bool reflectionChanged = false;

        for (IComponentManager::ComponentId id = 0; id < planarCount; ++id) {
            ScopedHandle<const PlanarReflectionComponent> handle = planarReflectionManager_.Read(id);
            const PlanarReflectionComponent& component = *handle;
            if (screenPercentages_[id] != component.screenPercentage) {
                screenPercentages_[id] = component.screenPercentage;
                reflectionChanged = true;
            }
        }

        const bool nonZero = (maxRes.x > 0U) && (maxRes.y > 0U);
        const bool sizeChanged = ((cm.gpuImageDesc.width != maxRes.x) || (cm.gpuImageDesc.height != maxRes.y));
        if (nonZero && (sizeChanged || reflectionChanged)) {
            cm.matPerCamera.resize(cameraCount_);
            cm.meshEntity.resize(cameraCount_);
            cm.renderMeshEntity.resize(cameraCount_);

            cm.cloudTexHandles.resize(cameraCount_);
            cm.cloudPrevTexHandles.resize(cameraCount_);
            cm.cloudDepthTexHandle.resize(cameraCount_);
            cm.cloudPrevDepthTexHandle.resize(cameraCount_);

            cm.cloudTexture.resize(cameraCount_);
            cm.cloudPrevTexture.resize(cameraCount_);
            cm.cloudDepthTexture.resize(cameraCount_);
            cm.cloudPrevDepthTexture.resize(cameraCount_);

            cm.gpuImageDesc.width = maxRes.x;
            cm.gpuImageDesc.height = maxRes.y;

            for (uint32_t i = 0; i < cameraCount_; i++) {
                if (i >= activeCameras) {
                    float sp = 1.0f;
                    const auto reflectionCameraIndex = (i - activeCameras);
                    if (reflectionCameraIndex < screenPercentages_.size()) {
                        sp = Math::clamp01(screenPercentages_[reflectionCameraIndex]);
                    }
                    cm.gpuImageDesc.width = uint32_t(float(maxRes.x) * sp);
                    cm.gpuImageDesc.height = uint32_t(float(maxRes.y) * sp);
                }
                // Color (current/prev)
                cm.gpuImageDesc.format = BASE_NS::Format::BASE_FORMAT_R8G8B8A8_UNORM;
                const auto cameraId = to_hex(ids[i]);
                cm.cloudTexHandles[i] = gpuResourceManager_->Create(
                    DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "CLOUD" + cameraId, cm.gpuImageDesc);
                cm.cloudPrevTexHandles[i] = gpuResourceManager_->Create(
                    DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "CLOUD_PREV" + cameraId,
                    cm.gpuImageDesc);
                cm.cloudTexture[i] = GetOrCreateEntityReference(ecs_, cm.cloudTexHandles[i]);
                cm.cloudPrevTexture[i] = GetOrCreateEntityReference(ecs_, cm.cloudPrevTexHandles[i]);

                // Depth (current/prev)
                cm.gpuImageDesc.format = BASE_NS::Format::BASE_FORMAT_R16_SFLOAT;
                cm.cloudDepthTexHandle[i] = gpuResourceManager_->Create(
                    DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "CLOUD_DEPTH" + cameraId,
                    cm.gpuImageDesc);
                cm.cloudPrevDepthTexHandle[i] = gpuResourceManager_->Create(
                    DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "CLOUD_DEPTH_PREV" + cameraId,
                    cm.gpuImageDesc);
                cm.cloudDepthTexture[i] = GetOrCreateEntityReference(ecs_, cm.cloudDepthTexHandle[i]);
                cm.cloudPrevDepthTexture[i] = GetOrCreateEntityReference(ecs_, cm.cloudPrevDepthTexHandle[i]);
                cm.gpuImageDesc.width = maxRes.x;
                cm.gpuImageDesc.height = maxRes.y;
                // Bind per camera
                UpdateCloudMaterial(i);
            }
        }
    }
    if (cloudWasEnabled_ != cloudEnabledThisFrame) {
        for (uint32_t i = 0; i < cameraCount_; i++) {
            if (auto* nodeSystem = GetSystem<INodeSystem>(ecs_)) {
                if (auto* node = nodeSystem->GetNode(cloudMatData_.renderMeshEntity[i])) {
                    node->SetEnabled(cloudEnabledThisFrame);
                }
            }
        }
        cloudWasEnabled_ = cloudEnabledThisFrame;
    }
    return frameRenderingQueued || cloudEnabledThisFrame || weatherUpdated;
}

void WeatherSystem::ProcessWaterRipples()
{
    const auto queryResult = query_.GetResults();
    auto* nodeSystem = GetSystem<INodeSystem>(ecs_);
    if (queryResult.empty() || (!nodeSystem) || (!renderContext_) || (!renderDataStoreManager_)) {
        return;
    }
    auto renderContextClassRegister = renderContext_->GetInterface<IClassRegister>();
    if (!renderContextClassRegister) {
        return;
    }
    if (!nodeSystem || !renderContext_ || !renderDataStoreManager_ || !dataStoreWeather_) {
        // If dataStoreWeather_ isn't ready, clear it if it somehow exists to prevent stale data
        if (dataStoreWeather_) {
            dataStoreWeather_->Clear();
        }
        return;
    }
    const auto pickingUtil = GetInstance<IPicking>(*renderContextClassRegister, UID_PICKING);
    if (!pickingUtil) {
        return;
    }

    dataStoreWeather_->Clear();

    for (auto& [planeEntity, planeRes] : handleResources_) {
        if (!EntityUtil::IsValid(planeEntity)) {
            CORE_LOG_W("WeatherSystem: Skipping invalid water plane entity in ProcessWaterRipples.");
            continue;
        }

        Math::Vec3 currentPlaneCenterPos { 0.0f, 0.0f, 0.0f };
        Math::Vec2 planeOffset { 0.0f, 0.0f };

        if (auto transHandle = transformationManager_.Read(planeEntity)) {
            currentPlaneCenterPos = transHandle->position;
            planeOffset = Math::Vec2(
                currentPlaneCenterPos.x - planeRes.prevPlanePos.x, currentPlaneCenterPos.z - planeRes.prevPlanePos.z);

            const auto mam = pickingUtil->GetWorldMatrixComponentAABB(planeEntity, false, ecs_);
            const Math::Vec3 aabbMin = mam.minAABB;
            const Math::Vec3 aabbMax = mam.maxAABB;
            const float width = Math::max(aabbMax.x - aabbMin.x, 1.0f);
            const float depth = Math::max(aabbMax.z - aabbMin.z, 1.0f);

            Math::Vec2 texelPerMeter =
                Math::Vec2(static_cast<float>(RESOLUTION) / width, static_cast<float>(RESOLUTION) / depth);
            planeOffset.x = Math::round(planeOffset.x * texelPerMeter.x);
            planeOffset.y = Math::round(planeOffset.y * texelPerMeter.y);

            planeRes.prevPlanePos = currentPlaneCenterPos; // Update prev position for this plane
        }

        RenderDataStoreWeather::WaterEffectData waterEffectData;
        waterEffectData.id = planeEntity.id;
        waterEffectData.planeOffset = planeOffset;
        waterEffectData.curIdx = planeRes.currentTextureIndex;
        waterEffectData.isInitialized = true;

        if (planeRes.rippleTextures[0]) {
            waterEffectData.texture0 = renderHandleManager_.GetRenderHandle(planeRes.rippleTextures[0]);
        }

        if (planeRes.rippleTextures[1]) {
            waterEffectData.texture1 = renderHandleManager_.GetRenderHandle(planeRes.rippleTextures[1]);
        }
        if (planeRes.rippleArgsBufferHandle) {
            waterEffectData.argsBuffer = renderHandleManager_.GetRenderHandle(planeRes.rippleArgsBufferHandle);
        }

        if (std::find(initializedPlaneIds_.begin(), initializedPlaneIds_.end(), planeEntity.id) ==
            initializedPlaneIds_.end()) {
            waterEffectData.isInitialized = false;
            initializedPlaneIds_.push_back(planeEntity.id);
        }

        dataStoreWeather_->SetWaterEffect(waterEffectData);

        const auto IntersectAabb = [this, renderContextClassRegister](const Math::Vec3& worldXYZ, Math::Vec2& inPlaneUv,
                                       const Entity& planeEntity) -> bool {
            const auto pickingUtil = GetInstance<IPicking>(*renderContextClassRegister, UID_PICKING);
            const auto mam = pickingUtil->GetWorldMatrixComponentAABB(planeEntity, false, ecs_);

            Math::Vec3 aabbMin = mam.minAABB;
            Math::Vec3 aabbMax = mam.maxAABB;
            constexpr float epsilon = FLT_EPSILON;

            bool xInRange = worldXYZ.x >= (aabbMin.x - epsilon) && worldXYZ.x <= (aabbMax.x + epsilon);
            bool yInRange = worldXYZ.y >= (aabbMin.y - epsilon) && worldXYZ.y <= (aabbMax.y + epsilon);
            bool zInRange = worldXYZ.z >= (aabbMin.z - epsilon) && worldXYZ.z <= (aabbMax.z + epsilon);

            if (xInRange && yInRange && zInRange) {
                inPlaneUv.x = (worldXYZ.x - aabbMin.x) / (aabbMax.x - aabbMin.x);
                inPlaneUv.y = (worldXYZ.z - aabbMin.z) / (aabbMax.z - aabbMin.z);
                return true;
            } else {
                return false;
            }
        };
        BASE_NS::vector<BASE_NS::pair<CORE_NS::Entity, BASE_NS::Math::UVec2>> newRippleToAdd;

        for (const auto& row : queryResult) {
            const auto& entityWithRippleComponent = row.entity;

            if (!EntityUtil::IsValid(entityWithRippleComponent)) {
                continue;
            }
            // get position from node system
            Math::Vec3 nodePosition;
            if (auto nodePtr = nodeSystem->GetNode(entityWithRippleComponent)) {
                nodePosition = nodePtr->GetPosition();
                auto parentNode = nodePtr->GetParent();
                while (parentNode) {
                    nodePosition += parentNode->GetPosition();
                    parentNode = parentNode->GetParent();
                }
            }
            auto componentHandle = rippleComponentManager_.Read(row.components[0U]);
            if (!componentHandle) {
                continue;
            }
            const WaterRippleComponent& rippleComponent = *componentHandle;
            Math::Vec3 rippleWorldPosition = rippleComponent.position + nodePosition;
            Math::Vec2 inUv;
            if (IntersectAabb(rippleWorldPosition, inUv, planeEntity)) {
                const auto intersectXGrid = uint32_t((1.f - inUv.x) * RESOLUTION);
                const auto intersectYGrid = uint32_t((1.f - inUv.y) * RESOLUTION);
                const Math::UVec2 newPos = { intersectXGrid, intersectYGrid };

                newRippleToAdd.push_back({ entityWithRippleComponent, newPos });
            }
        }
        DefaultWaterRippleDataStruct inputArgs {};
        inputArgs.inArgs = { 0U, planeRes.currentTextureIndex, RESOLUTION, RESOLUTION };

        if (!newRippleToAdd.empty()) {
            const auto newToAdd = static_cast<uint32_t>(newRippleToAdd.size());
            uint32_t idx = 0U;
            for (; idx < newToAdd && idx < 8U; idx++) {
                inputArgs.newRipplePositions[idx] = newRippleToAdd[idx].second;
            }
            inputArgs.inArgs.x = (idx > 0) ? 1U : 0U;
        }
        if (auto rdsDefaultStaging = refcnt_ptr<IRenderDataStoreDefaultStaging>(
                renderDataStoreManager_->GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING))) {
            if (planeRes.rippleArgsBufferHandle) {
                const auto& inputArgsBufferHandle =
                    renderHandleManager_.GetRenderHandleReference(planeRes.rippleArgsBufferHandle);
                array_view<const uint8_t> data((const uint8_t*)&inputArgs, sizeof(DefaultWaterRippleDataStruct));
                const BufferCopy bufferCopy { 0, 0, sizeof(DefaultWaterRippleDataStruct) };
                rdsDefaultStaging->CopyDataToBuffer(data, inputArgsBufferHandle, bufferCopy);
            }
        }

        planeRes.currentTextureIndex = 1 - planeRes.currentTextureIndex;

        if (const Entity matEntity = GetMaterialEntity(planeEntity); EntityUtil::IsValid(matEntity)) {
            if (auto matHandle = materialManager_.Write(matEntity); matHandle) {
                matHandle->textures[MaterialComponent::TextureIndex::SHEEN].image =
                    planeRes.rippleTextures[planeRes.currentTextureIndex];
            }
        }
    }
}

void WeatherSystem::UpdateCloudMaterial(uint32_t index)
{
    IEntityManager& em = ecs_.GetEntityManager();

    const bool bindlessEnabled =
        graphicsContext_->GetCreateInfo().createFlags & IGraphicsContext::CreateInfo::ENABLE_BINDLESS_PIPELINES_BIT;

    if (!EntityUtil::IsValid(cloudMatData_.matPerCamera[index])) {
        cloudMatData_.matPerCamera[index] = em.CreateReferenceCounted();
        materialManager_.Create(cloudMatData_.matPerCamera[index]);

        if (auto mh = materialManager_.Write(cloudMatData_.matPerCamera[index])) {
            IShaderManager& sm = renderContext_->GetDevice().GetShaderManager();
            const string_view cloudShader = bindlessEnabled ? "CLOUDS_BL" : "CLOUDS";
            mh->materialShader.shader =
                GetOrCreateEntityReference(ecs_, sm.GetShaderHandle(CLOUD_SHADER_NAME, cloudShader));

            mh->textures[MaterialComponent::NORMAL].image = cloudMatData_.cloudTexture[index];
            mh->textures[MaterialComponent::MATERIAL].image = cloudMatData_.cloudPrevTexture[index];
            mh->textures[MaterialComponent::EMISSIVE].image = cloudMatData_.cloudDepthTexture[index];
            mh->textures[MaterialComponent::AO].image = cloudMatData_.cloudPrevDepthTexture[index];

            mh->materialLightingFlags = 0U;
            mh->extraRenderingFlags |= MaterialComponent::ExtraRenderingFlagBits::CAMERA_EFFECT;
            mh->renderSort.renderSortLayer = 0;
        }
    } else {
        // If images were recreated (resize), rebind
        if (auto mh = materialManager_.Write(cloudMatData_.matPerCamera[index])) {
            mh->textures[MaterialComponent::NORMAL].image = cloudMatData_.cloudTexture[index];
            mh->textures[MaterialComponent::MATERIAL].image = cloudMatData_.cloudPrevTexture[index];
            mh->textures[MaterialComponent::EMISSIVE].image = cloudMatData_.cloudDepthTexture[index];
            mh->textures[MaterialComponent::AO].image = cloudMatData_.cloudPrevDepthTexture[index];
        }
    }

    if (!EntityUtil::IsValid(cloudMatData_.meshEntity[index])) {
        cloudMatData_.meshEntity[index] = em.CreateReferenceCounted();
        meshManager_.Create(cloudMatData_.meshEntity[index]);

        if (auto handle = meshManager_.Write(cloudMatData_.meshEntity[index])) {
            handle->submeshes.clear();
            handle->submeshes.push_back({});
            if (!handle->submeshes.empty()) {
                handle->submeshes[0U].instanceCount = 1U;
                handle->submeshes[0U].vertexCount = 3U;
                handle->submeshes[0U].material = cloudMatData_.matPerCamera[index];
            }
        }
    }

    if (!EntityUtil::IsValid(cloudMatData_.renderMeshEntity[index])) {
        if (auto* nodeSystem = GetSystem<INodeSystem>(ecs_)) {
            if (auto* node = nodeSystem->CreateNode()) {
                cloudMatData_.renderMeshEntity[index] = node->GetEntity();
                layerManager_.Create(cloudMatData_.renderMeshEntity[index]);
                auto layer = layerManager_.Write(cloudMatData_.renderMeshEntity[index]);
                layer->layerMask = LayerFlagBits::CORE_LAYER_FLAG_BIT_63 >> index;

                renderMeshManager_.Create(cloudMatData_.renderMeshEntity[index]);
                if (auto rh = renderMeshManager_.Write(cloudMatData_.renderMeshEntity[index])) {
                    rh->mesh = cloudMatData_.meshEntity[index];
                }
            }
        }
    }
}

void WeatherSystem::DestorySimulationResources(CORE_NS::Entity planeEntity)
{
    auto it = handleResources_.find(planeEntity);
    if (it == handleResources_.end()) {
        return;
    }
    auto& planeResources = it->second;
    planeResources.rippleTextures[0] = {};
    planeResources.rippleTextures[1] = {};
    planeResources.rippleArgsBufferHandle = {};
    handleResources_.erase(it);

    initializedPlaneIds_.erase(std::remove(initializedPlaneIds_.begin(), initializedPlaneIds_.end(), planeEntity.id),
        initializedPlaneIds_.end());
}

void WeatherSystem::OnComponentEvent(
    EventType type, const IComponentManager& componentManager, const array_view<const Entity> entities)
{
    IEntityManager& em = ecs_.GetEntityManager();
    auto& shaderManager = renderContext_->GetDevice().GetShaderManager();

    if (componentManager.GetUid() == planarReflectionManager_.GetUid()) {
        auto& prcm = static_cast<const IPlanarReflectionComponentManager&>(componentManager);
        PlanarReflectionComponent prc;
        for (const auto& planeEntity : entities) {
            if (auto handle = prcm.Read(planeEntity)) {
                bool waterEnabled = (handle->additionalFlags & PlanarReflectionComponent::WATER_BIT);
                if (type == EventType::CREATED || type == EventType::MODIFIED) {
                    if (waterEnabled) {
                        // A water plane was added, create resources for it.
                        if (handleResources_.find(planeEntity) == handleResources_.end()) {
                            CORE_LOG_V("WeatherSystem: Water plane entity %" PRIu64
                                       " added. Creating simulation resources.",
                                planeEntity.id);
                            if (graphicsContext_) {
                                auto res = CreateSimulationResourcesForEntity(ecs_, *graphicsContext_, planeEntity);
                                if (EntityUtil::IsValid(res.entityID)) {
                                    // Initialize its starting position
                                    if (auto transformHandle = transformationManager_.Read(planeEntity)) {
                                        res.prevPlanePos = transformHandle->position;
                                    }
                                    handleResources_.insert({ planeEntity, res });
                                }
                            }
                        }
                    } else {
                        // Change the material back to regular reflection plane if water is not enabled
                        auto it = handleResources_.find(planeEntity);
                        if (it != handleResources_.end()) {
                            if (const Entity matEntity = GetMaterialEntity(planeEntity);
                                EntityUtil::IsValid(matEntity)) {
                                if (auto matHandle = materialManager_.Write(matEntity); matHandle) {
                                    matHandle->materialShader.shader =
                                        GetOrCreateEntityReference(em, *GetManager<IRenderHandleComponentManager>(ecs_),
                                            shaderManager.GetShaderHandle(
                                                "3dshaders://shader/core3d_dm_fw_reflection_plane.shader"));
                                    matHandle->textures[MaterialComponent::TextureIndex::SHEEN].image = {};
                                    matHandle->textures[MaterialComponent::TextureIndex::SHEEN].sampler = {};
                                }
                            }
                            CORE_LOG_V("WeatherSystem: Water plane %" PRIu64
                                       " disabled.Switching to regular reflection plane material. Destroying "
                                       "simulation resources..",
                                planeEntity.id);
                            DestorySimulationResources(planeEntity);
                        }
                    }
                } else if (type == EventType::DESTROYED) {
                    auto it = handleResources_.find(planeEntity);
                    if (it != handleResources_.end()) {
                        CORE_LOG_V("WeatherSystem: Water plane entity %" PRIu64
                                   " removed.Destroying simulation resources.",
                            planeEntity.id);
                        DestorySimulationResources(planeEntity);
                    }
                }
            }
        }
    }
}

CORE_NS::ISystem* IWeatherSystemInstance(IEcs& ecs)
{
    return new WeatherSystem(ecs);
}

void IWeatherSystemDestroy(CORE_NS::ISystem* instance)
{
    delete static_cast<WeatherSystem*>(instance);
}
CORE3D_END_NAMESPACE()
