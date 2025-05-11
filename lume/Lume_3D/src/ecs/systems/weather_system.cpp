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

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
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
static constexpr string_view RIPPLE_TEXTURE_NAME_0 { "RIPPLE_RENDER_NODE_TEXTURE_0" };
static constexpr string_view RIPPLE_TEXTURE_NAME_1 { "RIPPLE_RENDER_NODE_TEXTURE_1" };
static constexpr string_view RIPPLE_INPUT_BUFFER_NAME { "RIPPLE_RENDER_NODE_INPUTBUFFER" };

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
    auto rhr = gpuResourceMgr.Create(name, desc);

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
    IEcs& ecs, IGraphicsContext& graphicsContext, uint32_t inResolution)
{
    auto& renderContext = graphicsContext.GetRenderContext();
    auto& device = renderContext.GetDevice();
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    WeatherSystem::RipplePlaneResources mTestRes;
    // create textures for ripple simulation
    {
        mTestRes.rippleTextures[0] = CreateTargetGpuImage(ecs, gpuResourceMgr, { inResolution, inResolution },
            RIPPLE_TEXTURE_NAME_0, Format::BASE_FORMAT_R32G32B32A32_SFLOAT);
        mTestRes.rippleTextures[1] = CreateTargetGpuImage(ecs, gpuResourceMgr, { inResolution, inResolution },
            RIPPLE_TEXTURE_NAME_1, Format::BASE_FORMAT_R32G32B32A32_SFLOAT);

        GpuBufferDesc inputArgsBuffer;
        inputArgsBuffer.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        inputArgsBuffer.memoryPropertyFlags =
            CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        inputArgsBuffer.usageFlags = CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        inputArgsBuffer.byteSize = sizeof(DefaultWaterRippleDataStruct);
        mTestRes.rippleArgsBufferHandle = CreateBuffer(ecs, gpuResourceMgr, RIPPLE_INPUT_BUFFER_NAME, inputArgsBuffer);
    }

    return mTestRes;
}

// nodeEntity should have material component
WeatherSystem::RipplePlaneResources CreateSimulationResourcesForEntity(
    IEcs& ecs, IGraphicsContext& graphicsContext, const Entity& nodeEntity)
{
    WeatherSystem::RipplePlaneResources planeRes;

    auto* renderMeshCM = GetManager<IRenderMeshComponentManager>(ecs);
    auto* meshCM = GetManager<IMeshComponentManager>(ecs);
    auto* matCM = GetManager<IMaterialComponentManager>(ecs);
    auto* gpuHandleCM = GetManager<IRenderHandleComponentManager>(ecs);

    auto& entityMgr = ecs.GetEntityManager();
    auto& device = graphicsContext.GetRenderContext().GetDevice();
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    IShaderManager& shaderMgr = device.GetShaderManager();
    if (!(renderMeshCM && meshCM && matCM && gpuHandleCM)) {
        return {};
    }

    planeRes = InitSimulationResources(ecs, graphicsContext, RESOLUTION);
    planeRes.entityID = nodeEntity;

    // create bilinear sampler
    RenderHandleReference samplerRhr = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    EntityReference BilinearSampler =
        GetOrCreateEntityReference(ecs.GetEntityManager(), *gpuHandleCM, BASE_NS::move(samplerRhr));

    if (const auto rmcHandle = renderMeshCM->Read(nodeEntity); rmcHandle) {
        const RenderMeshComponent& rmc = *rmcHandle;
        if (const auto meshHandle = meshCM->Read(rmc.mesh); meshHandle) {
            const MeshComponent& meshComponent = *meshHandle;
            if (!meshComponent.submeshes.empty()) {
                if (auto matHandle = matCM->Write(meshComponent.submeshes[0].material); matHandle) {
                    MaterialComponent& matComponent = *matHandle;
                    matComponent.materialShader.shader = GetOrCreateEntityReference(entityMgr, *gpuHandleCM,
                        shaderMgr.GetShaderHandle("3dshaders://shader/ripple/custom_water_ripple.shader"));

                    // default settings fro custom_reflection_water shader.
                    matComponent.materialLightingFlags &= ~MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT;
                    matComponent.textures[MaterialComponent::TextureIndex::BASE_COLOR].factor =
                        Math::Vec4(0.1f, 0.1f, 0.25f, 0.8f);
                    // go higher for mirror
                    matComponent.textures[MaterialComponent::TextureIndex::MATERIAL].factor =
                        Math::Vec4(0.f, 0.01f, 0.0f, 0.3f);
                    matComponent.textures[MaterialComponent::TextureIndex::NORMAL].factor.x = 1.0f;
                    matComponent.textures[MaterialComponent::TextureIndex::EMISSIVE].factor = { 10.0f, 10.0f, 10.0f,
                        1.0f };

                    // set  RippleEffect texture as material component's texture.
                    matComponent.textures[MaterialComponent::TextureIndex::AO].image = planeRes.rippleTextures[0];
                    matComponent.textures[MaterialComponent::TextureIndex::AO].sampler = BASE_NS::move(BilinearSampler);
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
    : ecs_(ecs), renderContext_(*GetInstance<IRenderContext>(
                     *ecs.GetClassFactory().GetInterface<IClassRegister>(), UID_RENDER_CONTEXT)),
      graphicsContext_(
          *GetInstance<IGraphicsContext>(*renderContext_.GetInterface<IClassRegister>(), UID_GRAPHICS_CONTEXT)),
      gpuResourceManager_(renderContext_.GetDevice().GetGpuResourceManager()),
      renderDataStoreManager_(
          GetInstance<IRenderContext>(*ecs.GetClassFactory().GetInterface<IClassRegister>(), UID_RENDER_CONTEXT)
              ->GetRenderDataStoreManager()),
      materialManager_(*GetManager<IMaterialComponentManager>(ecs)),
      meshManager_(*GetManager<IMeshComponentManager>(ecs)),
      renderMeshManager_(*GetManager<IRenderMeshComponentManager>(ecs)),
      renderHandleManager_(*GetManager<IRenderHandleComponentManager>(ecs)),
      transformationManager_(*GetManager<ITransformComponentManager>(ecs)),
      renderConfigManager_(*GetManager<IRenderConfigurationComponentManager>(ecs)),
      camManager_(*GetManager<ICameraComponentManager>(ecs)),
      envManager_(*GetManager<IEnvironmentComponentManager>(ecs)),
      weatherManager_(*GetManager<IWeatherComponentManager>(ecs)),
      rippleComponentManager_(*GetManager<IWaterRippleComponentManager>(ecs))
{
    ecs.AddListener(rippleComponentManager_, *this);
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
    auto iter = handleResources_.find(entity);
    if (iter != handleResources_.end()) {
        return iter->second;
    }
    auto res = CreateSimulationResourcesForEntity(ecs_, graphicsContext_, entity);
    handleResources_.insert({ entity, res });
    return res;
}

void WeatherSystem::Initialize()
{
    const ComponentQuery::Operation operations[] = { { rippleComponentManager_, ComponentQuery::Operation::OPTIONAL } };
    query_.SetEcsListenersEnabled(true);
    query_.SetupQuery(rippleComponentManager_, operations);
    prevPlanePos_ = Math::Vec3(0.0f, 0.0f, 0.0f);

    // NOTE: data store is done deferred way in the update
}

void WeatherSystem::Uninitialize()
{
    query_.SetEcsListenersEnabled(false);
}

WeatherSystem ::~WeatherSystem()
{
    Uninitialize();
    ecs_.RemoveListener(rippleComponentManager_, *this);
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
    if (!active_) {
        return false;
    }

    if (!dataStoreWeather_) {
        string dsName;
        if (IRenderPreprocessorSystem* rpSystem = GetSystem<IRenderPreprocessorSystem>(ecs_); rpSystem) {
            if (const auto in = ScopedHandle<IRenderPreprocessorSystem::Properties>(rpSystem->GetProperties()); in) {
                dsName = in->dataStorePrefix;
            }
        }
        dsName += RenderDataStoreWeather::TYPE_NAME;
        dataStoreWeather_ = refcnt_ptr<RenderDataStoreWeather>(
            renderDataStoreManager_.Create(RenderDataStoreWeather::UID, dsName.c_str()));
    }
    if (!dataStoreWeather_) {
        return false;
    }

    query_.Execute();

    ProcessWaterRipples();

    if (const auto weatherGeneration = weatherManager_.GetGenerationCounter();
        weatherGeneration_ != weatherGeneration) {
        weatherGeneration_ = weatherGeneration;
        const auto weatherComponents = weatherManager_.GetComponentCount();
        for (auto id = 0U; id < weatherComponents; ++id) {
            auto handle = weatherManager_.Read(id);
            if (!handle) {
                continue;
            }
            RenderDataStoreWeather::WeatherSettings settings;
            settings.windSpeed = handle->windSpeed;
            settings.density = handle->density;
            settings.cloudType = handle->cloudType;
            settings.coverage = handle->coverage;
            settings.precipitation = handle->precipitation;

            settings.windDir[0] = handle->windDir.x;
            settings.windDir[1] = handle->windDir.y;
            settings.windDir[2] = handle->windDir.z;

            settings.sunPos[0] = handle->sunPos.x;
            settings.sunPos[1] = handle->sunPos.y;
            settings.sunPos[2] = handle->sunPos.z;

            settings.silverIntensity = handle->silverIntensity;
            settings.silverSpread = handle->silverSpread;
            settings.direct = handle->direct;
            settings.ambient = handle->ambient;
            settings.lightStep = handle->lightStep;
            settings.scatteringDist = handle->scatteringDist;
            settings.precipitation = handle->precipitation;

            settings.domain[0] = handle->domain[0];
            settings.domain[1] = handle->domain[1];
            settings.domain[2] = handle->domain[2];
            settings.domain[3] = handle->domain[3];
            settings.domain[4] = handle->domain[4];

            settings.anvil_bias = handle->anvilBias;
            settings.brightness = handle->brightness;
            settings.eccintricity = handle->eccintricity;

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
        }
    }

    // get largest camera resolution

    // first check if there are any weather components
    const auto wcCount = weatherManager_.GetComponentCount();
    // then check the usage with cameras
    Math::UVec2 maxRes { 0U, 0U };
    Entity camEntity;
    if (wcCount > 0) {
        bool usesWeather { false };
        if (const auto count = renderConfigManager_.GetComponentCount(); count > 0U) {
            if (const auto rcHandle = renderConfigManager_.Read(renderConfigManager_.GetEntity(0U)); rcHandle) {
                if (const auto envHandle = envManager_.Read(rcHandle->environment); envHandle) {
                    if (envHandle->weather) {
                        usesWeather = true;
                    }
                }
            }
        }
        const auto cameraCount = camManager_.GetComponentCount();
        for (IComponentManager::ComponentId id = 0; id < cameraCount; ++id) {
            ScopedHandle<const CameraComponent> handle = camManager_.Read(id);
            const CameraComponent& component = *handle;
            bool checkRes = usesWeather;
            if (auto envHandle = envManager_.Read(component.environment); envHandle) {
                if (envHandle->weather) {
                    checkRes = true;
                }
            }
            if (checkRes) {
                maxRes.x = Math::max(maxRes.x, component.renderResolution.x);
                maxRes.y = Math::max(maxRes.y, component.renderResolution.y);

                camEntity = camManager_.GetEntity(id);
            }
        }

        // possible resize
        auto& cm = cloudMatData_;

        const uint32_t resDevider = GetResDivider(cm.cloudRenderingType);
        maxRes.x = maxRes.x / resDevider;
        maxRes.y = maxRes.y / resDevider;

        const bool nonZero = (maxRes.x > 0) && (maxRes.y > 0);
        if (nonZero && ((cm.gpuImageDesc.width != maxRes.x) || (cm.gpuImageDesc.height != maxRes.y))) {
            cm.gpuImageDesc.width = maxRes.x;
            cm.gpuImageDesc.height = maxRes.y;

            cm.cloudTexHandle = renderContext_.GetDevice().GetGpuResourceManager().Create(
                DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "CLOUD", cm.gpuImageDesc);
            cm.cloudPrevTexHandle = renderContext_.GetDevice().GetGpuResourceManager().Create(
                DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "CLOUD_PREV", cm.gpuImageDesc);
            cm.cloudTexture = GetOrCreateEntityReference(ecs_, cm.cloudTexHandle);
            cm.cloudPrevTexture = GetOrCreateEntityReference(ecs_, cm.cloudPrevTexHandle);

            UpdateCloudMaterial(camEntity);
        }
    }

    return frameRenderingQueued;
}

void WeatherSystem::ProcessWaterRipples()
{
    const auto queryResult = query_.GetResults();
    if (queryResult.empty()) {
        return;
    }

    auto* nodeSystem = GetSystem<INodeSystem>(ecs_);
    // find a water plane name "water effect plane"
    if (!EntityUtil::IsValid(waterEffectsPlane.entity)) {
        if (auto find = nodeSystem->GetRootNode().LookupNodeByName("water effect plane"); find) {
            auto e = find->GetEntity();
            waterEffectsPlane.entity = e;
            waterEffectsPlane.matEntity = GetMaterialEntity(e);
        }
    }

    Math::Vec3 planeCenter;

    if (auto transComponentHandle = transformationManager_.Read(waterEffectsPlane.entity)) {
        const TransformComponent& transComponent = *transComponentHandle;
        planeCenter = transComponent.position;
        Math::Vec2 offset = Math::Vec2(planeCenter.x - prevPlanePos_.x, planeCenter.z - prevPlanePos_.z);

        const auto pickingUtil =
            GetInstance<IPicking>(*graphicsContext_.GetRenderContext().GetInterface<IClassRegister>(), UID_PICKING);
        const auto mam = pickingUtil->GetWorldMatrixComponentAABB(waterEffectsPlane.entity, false, ecs_);

        const Math::Vec3 aabbMin = mam.minAABB;
        const Math::Vec3 aabbMax = mam.maxAABB;
        const float width = Math::max(aabbMax.x - aabbMin.x, 1.0f);
        const float depth = Math::max(aabbMax.z - aabbMin.z, 1.0f);

        vec2 texelPerMeter = vec2(float(RESOLUTION), float(RESOLUTION));
        offset.x = Math::round(offset.x * texelPerMeter.x / width);
        offset.y = Math::round(offset.y * texelPerMeter.y / depth);

        dataStoreWeather_->SetWaterEffect(waterEffectsPlane.entity.id, offset);
        prevPlanePos_ = planeCenter;
    }

    const auto IntersectAabb = [this](const Math::Vec3& worldXYZ, Math::Vec2& inPlaneUv) -> bool {
        const auto pickingUtil =
            GetInstance<IPicking>(*graphicsContext_.GetRenderContext().GetInterface<IClassRegister>(), UID_PICKING);
        const auto mam = pickingUtil->GetWorldMatrixComponentAABB(waterEffectsPlane.entity, false, ecs_);

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

    for (const auto& row : queryResult) {
        const auto& entityWithRippleComponent = row.entity;

        if (EntityUtil::IsValid(entityWithRippleComponent)) {
            // get position from node system
            auto nodePtr = nodeSystem->GetNode(entityWithRippleComponent);
            Math::Vec3 nodePosition = nodePtr->GetPosition();
            auto parentNode = nodePtr->GetParent();
            while (parentNode) {
                nodePosition += parentNode->GetPosition();
                parentNode = parentNode->GetParent();
            }

            if (auto componentHandle = rippleComponentManager_.Read(row.components[0U]); componentHandle) {
                const WaterRippleComponent& rippleComponent = *componentHandle;
                constexpr bool active = true;
                if (active) {
                    Math::Vec3 rippleWorldPosition = rippleComponent.position + nodePosition;
                    Math::Vec2 inUv;
                    if (IntersectAabb(rippleWorldPosition, inUv)) {
                        const auto intersectXGrid = uint32_t((1.f - inUv.x) * RESOLUTION);
                        const auto intersectYGrid = uint32_t((1.f - inUv.y) * RESOLUTION);
                        const Math::UVec2 newPos = { intersectXGrid, intersectYGrid };

                        newRippleToAdd_.push_back({ entityWithRippleComponent, newPos });
                    }
                }
            }
        }
    }

    // update ripple input args buffer for compute shader
    DefaultWaterRippleDataStruct inputArgs;
    inputArgs.inArgs = { 0U, 0U, RESOLUTION, RESOLUTION };

    if (!newRippleToAdd_.empty()) {
        const auto newToAdd = uint32_t(newRippleToAdd_.size());
        uint32_t idx = 0U;
        for (; idx < newToAdd && idx < 8U; idx++) {
            inputArgs.newRipplePositions[idx] = newRippleToAdd_[idx].second;
        }

        newRippleToAdd_.erase(newRippleToAdd_.cbegin(), newRippleToAdd_.cbegin() + idx);
        inputArgs.inArgs.x = 1;
    }

    RipplePlaneResources res = GetOrCreateSimulationResources(waterEffectsPlane.entity);

    auto& dataStoreManager = graphicsContext_.GetRenderContext().GetRenderDataStoreManager();
    if (auto rdsDefaultStaging = refcnt_ptr<IRenderDataStoreDefaultStaging>(
            dataStoreManager.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING))) {
        const auto& inputArgsBufferHandle = renderHandleManager_.GetRenderHandleReference(res.rippleArgsBufferHandle);
        array_view<const uint8_t> data((const uint8_t*)&inputArgs, sizeof(DefaultWaterRippleDataStruct));
        const BufferCopy bufferCopy { 0, 0, sizeof(DefaultWaterRippleDataStruct) };
        rdsDefaultStaging->CopyDataToBuffer(data, inputArgsBufferHandle, bufferCopy);
    }
}

void WeatherSystem::UpdateCloudMaterial(const CORE_NS::Entity camEntity)
{
    // create only once
    if (!EntityUtil::IsValid(cloudMatData_.renderMeshEntity)) {
        IEntityManager& em = ecs_.GetEntityManager();
        cloudMatData_.meshEntity = em.CreateReferenceCounted();
        cloudMatData_.matEntity = em.CreateReferenceCounted();

        materialManager_.Create(cloudMatData_.matEntity);
        meshManager_.Create(cloudMatData_.meshEntity);

        // node for render processing
        if (auto* nodeSystem = GetSystem<INodeSystem>(ecs_); nodeSystem) {
            if (auto* cloudScene = nodeSystem->CreateNode(); cloudScene) {
                cloudMatData_.renderMeshEntity = cloudScene->GetEntity();
            }
        }
        renderMeshManager_.Create(cloudMatData_.renderMeshEntity);

        // material
        if (auto handle = materialManager_.Write(cloudMatData_.matEntity); handle) {
            IShaderManager& shaderMgr = renderContext_.GetDevice().GetShaderManager();
            const auto shader = shaderMgr.GetShaderHandle(CLOUD_SHADER_NAME);
            handle->materialShader.shader = GetOrCreateEntityReference(ecs_, shader);
            handle->textures[MaterialComponent::BASE_COLOR].image = cloudMatData_.cloudTexture;
            handle->textures[MaterialComponent::NORMAL].image = cloudMatData_.cloudPrevTexture;
            handle->materialLightingFlags = 0U; // disable all lighting

            // camera effect
            handle->cameraEntity = camEntity;
        }
        if (auto handle = meshManager_.Write(cloudMatData_.meshEntity); handle) {
            handle->submeshes.clear();
            handle->submeshes.push_back({});
            if (!handle->submeshes.empty()) {
                handle->submeshes[0U].instanceCount = 1U;
                handle->submeshes[0U].vertexCount = 3U;
                handle->submeshes[0U].material = cloudMatData_.matEntity;
            }
        }
        if (auto handle = renderMeshManager_.Write(cloudMatData_.renderMeshEntity); handle) {
            handle->mesh = cloudMatData_.meshEntity;
        }
    }
    // update material textures
    // should not come here if the textures are not changed
    if (auto handle = materialManager_.Write(cloudMatData_.matEntity); handle) {
        // flip info should be provided in the shader
        handle->textures[MaterialComponent::BASE_COLOR].image = cloudMatData_.cloudTexture;
        handle->textures[MaterialComponent::NORMAL].image = cloudMatData_.cloudPrevTexture;
    }
}

void WeatherSystem::OnComponentEvent(
    EventType type, const IComponentManager& componentManager, const array_view<const Entity> entities)
{}

CORE_NS::ISystem* IWeatherSystemInstance(IEcs& ecs)
{
    return new WeatherSystem(ecs);
}

void IWeatherSystemDestroy(CORE_NS::ISystem* instance)
{
    delete static_cast<WeatherSystem*>(instance);
}
CORE3D_END_NAMESPACE()
