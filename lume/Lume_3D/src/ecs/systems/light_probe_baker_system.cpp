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

#include "light_probe_baker_system.h"

#include <ComponentTools/component_query.h>
#include <algorithm>
#include <cfloat>
#include <cinttypes>

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/light_probe_group_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_light_probe_baker_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/ecs/systems/intf_render_system.h>
#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/render_data_defines_3d.h>
#include <base/math/matrix_util.h>
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
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>

#include "ecs/systems/render_preprocessor_system.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

CORE3D_BEGIN_NAMESPACE()
namespace {

static constexpr string_view RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY = "RenderDataStoreDefaultGpuResourceDataCopy";

// Each iteration of light probe bake feeds light probe lighting into the next bake.
// Multiple iterations result in calculating multiple bounces of diffuse light.
// Two iterations is a good minimum.
constexpr uint32_t LIGHT_PROBE_BAKE_ITERATIONS = 2u;

// Size of sub_view is clamped to end of view.
// Offset is not clamped, and size can be zero.
template <typename T>
array_view<T> sub_view(array_view<T> view, uint32_t offset, uint32_t size)
{
    if (view.empty()) {
        return view;
    }

    // Clamp size to end of view
    uint32_t view_size = static_cast<uint32_t>(view.size());
    if (offset >= view_size) {
        return array_view<T>(view.data() + view_size, static_cast<size_t>(0));
    }
    if (offset + size > view_size) {
        size = view_size - offset;
    }

    return array_view<T>(view.data() + offset, size);
}

template <typename T>
array_view<T> sub_view(vector<T>& view, uint32_t offset, uint32_t size)
{
    return sub_view(array_view<T>(view.data(), view.size()), offset, size);
}

}  // namespace

// These helpers are from render_util.cpp
namespace {

RenderNodeGraphDesc LoadRenderNodeGraph(IRenderNodeGraphLoader& rngLoader, const string_view rng)
{
    IRenderNodeGraphLoader::LoadResult lr = rngLoader.Load(rng);
    if (!lr.success) {
#if (CORE3D_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("error loading render node graph: %s - error: %s", rng.data(), lr.error.data());
#endif
    }
    return lr.desc;
}

inline json::standalone_value GetPodPostProcess(const string_view name)
{
    auto renderDataStore = json::standalone_value{json::standalone_value::object{}};
    renderDataStore["dataStoreName"] = "RenderDataStorePod";
    renderDataStore["typeName"] = "RenderDataStorePod";  // This is render data store TYPE_NAME
    renderDataStore["configurationName"] = string(name);
    return renderDataStore;
}

}  // namespace

namespace {

RenderConfigurationComponent GetRenderConfigurationComponent(
    INodeComponentManager* nodeMgr_, IRenderConfigurationComponentManager* renderConfigMgr_)
{
    for (IComponentManager::ComponentId i = 0; i < renderConfigMgr_->GetComponentCount(); i++) {
        const Entity id = renderConfigMgr_->GetEntity(i);
        if (nodeMgr_->Get(id).effectivelyEnabled) {
            return renderConfigMgr_->Get(i);
        }
    }
    return RenderConfigurationComponent{};
};

Entity FindMainCamera(ICameraComponentManager* cameraManager, const RenderConfigurationComponent& sc)
{
    Entity cameraEntity{INVALID_ENTITY};
    // Grab active main camera.
    const auto cameraCount = cameraManager->GetComponentCount();
    for (IComponentManager::ComponentId id = 0; id < cameraCount; ++id) {
        if (auto handle = cameraManager->Read(id); handle) {
            if ((handle->sceneFlags & CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT) &&
                (handle->sceneFlags & CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT)) {
                cameraEntity = cameraManager->GetEntity(id);
                break;
            }
        }
    }
#if (CORE3D_VALIDATION_ENABLED == 1)
    if (!EntityUtil::IsValid(cameraEntity)) {
        PLUGIN_LOG_ONCE_I(
            "RenderSystem::ProcessScene_no_active_main_cams", "CORE3D_VALIDATION: Main cameras are not active");
    }
#endif
    return cameraEntity;
}

}  // namespace

RenderHandleReference CreateLightProbeBakeRenderNodeGraph(IRenderNodeGraphManager& rngm, uint64_t bakeCameraId,
    const string_view renderCameraPostProcessName, const string_view sceneDataStoreName)
{
    // Light probe cubemap rasterization pass
    IRenderNodeGraphLoader& rngl = rngm.GetRenderNodeGraphLoader();
    constexpr const string_view CAM_SCENE_LIGHT_PROBE_STR = "3drendernodegraphs://core3d_rng_light_probe_cam_scene.rng";
    RenderNodeGraphDesc desc = LoadRenderNodeGraph(rngl, CAM_SCENE_LIGHT_PROBE_STR);

    // And then overriding camera parts of jsoninput
    for (auto& rnRef : desc.nodes) {
        json::standalone_value jsonVal = CORE_NS::json::parse(rnRef.nodeJson.data());
        jsonVal["customCameraId"] = bakeCameraId;
        jsonVal["customCameraName"] = "LightProbeBakingCamera";
        if (auto dataStore = jsonVal.find("renderDataStore"); dataStore) {
            if (auto config = dataStore->find("configurationName");
                config && config->is_string() && (!config->string_.empty())) {
                json::standalone_value renderDataStore = GetPodPostProcess(renderCameraPostProcessName);
                if (renderDataStore) {
                    jsonVal["renderDataStore"] = move(renderDataStore);
                }
            }
        }
        rnRef.nodeJson = to_string(jsonVal);
    }

    return rngm.Create(
        IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, desc, {}, sceneDataStoreName);
}

//////////
// LightProbeBaker system
//////////

LightProbeBakerSystem::LightProbeBakerSystem(IEcs& ecs)
    : ecs_(ecs),
      lightProbeGroupMgr_(GetManager<ILightProbeGroupComponentManager>(ecs)),
      nodeMgr_(GetManager<INodeComponentManager>(ecs)),
      worldMatrixMgr_(GetManager<IWorldMatrixComponentManager>(ecs)),
      gpuHandleMgr_(GetManager<IRenderHandleComponentManager>(ecs)),
      cameraMgr_(GetManager<ICameraComponentManager>(ecs)),
      renderConfigMgr_(GetManager<IRenderConfigurationComponentManager>(ecs))
{
    ecs.AddListener(*lightProbeGroupMgr_, *this);

    if (IEngine* engine = ecs_.GetClassFactory().GetInterface<IEngine>()) {
        if (auto* engineClassRegister = engine->GetInterface<IClassRegister>()) {
            renderContext_ = CORE3D_NS::GetInstance<IRenderContext>(*engineClassRegister, UID_RENDER_CONTEXT);
            gpuResourceMgr_ = &renderContext_->GetDevice().GetGpuResourceManager();
        }
    }
}

void LightProbeBakerSystem::Initialize()
{
    if (!renderContext_) {
        return;
    }
    if (IRenderPreprocessorSystem* rps = GetSystem<IRenderPreprocessorSystem>(ecs_); rps) {
        if (const auto in = ScopedHandle<const IRenderPreprocessorSystem::Properties>(rps->GetProperties()); in) {
            dataStoreScene_ = in->dataStoreScene;
            dataStoreCamera_ = in->dataStoreCamera;
        }
    } else {
        PLUGIN_LOG_E("DEPRECATED USAGE: RenderPreprocessorSystem not found. Add system to system graph.");
    }
    dsCamera_ = refcnt_ptr<IRenderDataStoreDefaultCamera>(
        renderContext_->GetRenderDataStoreManager().GetRenderDataStore(dataStoreCamera_));
}

void LightProbeBakerSystem::Uninitialize()
{}

LightProbeBakerSystem ::~LightProbeBakerSystem()
{
    Uninitialize();
    ecs_.RemoveListener(*lightProbeGroupMgr_, *this);
}

void LightProbeBakerSystem::SetActive(bool state)
{
    active_ = state;
}

bool LightProbeBakerSystem::IsActive() const
{
    return active_;
}

string_view LightProbeBakerSystem::GetName() const
{
    return ::GetName(this);
}

Uid LightProbeBakerSystem::GetUid() const
{
    return UID;
}

IPropertyHandle* LightProbeBakerSystem::GetProperties()
{
    return nullptr;
}

const IPropertyHandle* LightProbeBakerSystem::GetProperties() const
{
    return nullptr;
}

void LightProbeBakerSystem::SetProperties(const IPropertyHandle& dataHandle)
{}

const IEcs& LightProbeBakerSystem::GetECS() const
{
    return ecs_;
}

bool LightProbeBakerSystem::Update(bool frameRenderingQueued, uint64_t /* time */, uint64_t delta)
{
    if (!active_) {
        return false;
    }

    bool renderUpdates = UpdateLightProbeBake();
    if (currentBake_.status == LightProbeBakeStatusInternal::BAKE_IN_PROGRESS) {
        uint64_t cameraId = CreateLightProbeBakingCamera();
        if (!currentBake_.rng && cameraId != RenderSceneDataConstants::INVALID_ID) {
            currentBake_.rng = CreateLightProbeBakeRenderNodeGraph(renderContext_->GetRenderNodeGraphManager(),
                currentBake_.cameraId,
                dsCamera_->GetCamera(cameraId).postProcessName,
                dataStoreScene_);
        }
    }

    return renderUpdates;
}

void LightProbeBakerSystem::CollectLightProbesToBake()
{
    for (Entity lightProbeGroupEntity : currentBake_.lightProbeGroupEntities) {
        if (!EntityUtil::IsValid(lightProbeGroupEntity)) {
            continue;
        }
        Math::Mat4X4 worldMatrix(1.0f);
        // Clear light probe data to zero first
        if (auto lpgc = lightProbeGroupMgr_->Write(lightProbeGroupEntity)) {
            for (auto& lightProbe : lpgc->lightProbes) {
                for (uint32_t i = 0; i < countof(lightProbe.shCoefficients); ++i) {
                    lightProbe.shCoefficients[i] = Math::Vec3{0.0f, 0.0f, 0.0f};
                }
            }
        }
        // Get transformed light probe positions for baking
        if (auto lpgc = lightProbeGroupMgr_->Read(lightProbeGroupEntity)) {
            for (uint32_t probeIdx = 0; probeIdx < lpgc->lightProbes.size(); ++probeIdx) {
                Math::Vec3 probeWorldPosition =
                    BASE_NS::Math::MultiplyPoint3X4(worldMatrix, lpgc->lightProbes[probeIdx].position);
                currentBake_.lightProbeBakeInputs.push_back({probeWorldPosition});
            }
            currentBake_.bakeGroups.push_back({
                lightProbeGroupEntity,
                lightProbeGroupMgr_->GetComponentGeneration(lightProbeGroupMgr_->GetComponentId(lightProbeGroupEntity)),
                static_cast<uint32_t>(lpgc->lightProbes.size()),
            });
        }
    }
}

bool LightProbeBakerSystem::UpdateLightProbeBake()
{
    if (!lightProbeGroupMgr_ || !nodeMgr_ || !worldMatrixMgr_) {
        return false;
    }

    if (currentBake_.status == LightProbeBakeStatusInternal::BAKE_TRIGGERED) {
        // Find light probes and init light probe GPU resources ONLY when bake has just been triggered.
        CollectLightProbesToBake();
        if (!currentBake_.lightProbeBakeInputs.empty() && !currentBake_.bakeGroups.empty()) {
            currentBake_.status = LightProbeBakeStatusInternal::BAKE_IN_PROGRESS;
            if (!InitializeGpuResources()) {
                // Fail the bake
                currentBake_ = {};
                currentBake_.status = LightProbeBakeStatusInternal::IDLE;
                return false;
            }
            currentBake_.cameraId = BASE_NS::Hash(currentBake_.lightProbeGroupEntities[0].id);
        } else {
            // Nothing to bake, go back to idle
            currentBake_.status = LightProbeBakeStatusInternal::IDLE;
            return false;
        }
    } else if (currentBake_.status == LightProbeBakeStatusInternal::BAKE_IN_PROGRESS) {
        if (currentBake_.iterationBakedProbesCount == currentBake_.lightProbeBakeInputs.size()) {
            // Readback SH data
            auto& rdsMgr = renderContext_->GetRenderDataStoreManager();
            auto dataStoreDataCopy = refcnt_ptr<IRenderDataStoreDefaultGpuResourceDataCopy>(
                rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY));
            if (dataStoreDataCopy) {
                IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                dataCopy.gpuHandle = gpuResources_.SHBuffer;
                gpuResources_.SHReadbackData = make_unique<ByteArray>(
                    LightProbeBakeGPUResources::getBufferSizeBytes(gpuResources_.SHBufferSizeProbes));
                dataCopy.byteArray = gpuResources_.SHReadbackData.get();
                dataStoreDataCopy->AddCopyOperation(dataCopy);
                currentBake_.status = LightProbeBakeStatusInternal::READBACK_TRIGGERED;
            }
        }
    } else if (currentBake_.status == LightProbeBakeStatusInternal::READBACK_TRIGGERED) {
        // Readback was triggered last frame with WAIT_FOR_IDLE, therefore data should have arrived now.
        // Copy the baked data back into the corresponding light probe group components.
        Math::Vec4* bakedShCoeffs = reinterpret_cast<Math::Vec4*>(gpuResources_.SHReadbackData->GetData().data());
        for (uint32_t bakeGroupIdx = 0; bakeGroupIdx < currentBake_.bakeGroups.size(); ++bakeGroupIdx) {
            LightProbeBakeGroup const& group = currentBake_.bakeGroups[bakeGroupIdx];
            uint32_t generation =
                lightProbeGroupMgr_->GetComponentGeneration(lightProbeGroupMgr_->GetComponentId(group.entity));
            uint32_t expectedGeneration = group.componentGeneration + currentBake_.iteration;
            if (generation != expectedGeneration) {
#if (CORE3D_VALIDATION_ENABLED == 1)
                PLUGIN_LOG_E("CORE3D_VALIDATION: Light probe group changed while baking, discarding results.");
#endif
                continue;
            }
            if (auto lpgc = lightProbeGroupMgr_->Write(group.entity)) {
                if (group.probeBakeCount != lpgc->lightProbes.size()) {
#if (CORE3D_VALIDATION_ENABLED == 1)
                    // This should not happen anyway since we check for the generation
                    PLUGIN_LOG_E("CORE3D_VALIDATION: Light probe group probe count changed while baking, skipping "
                                 "results update");
#endif
                    continue;
                }
                for (uint32_t probeIdx = 0; probeIdx < group.probeBakeCount; ++probeIdx) {
                    auto& probeShCoeffs = lpgc->lightProbes[probeIdx].shCoefficients;
                    for (uint32_t coeffIdx = 0; coeffIdx < LightProbeBakeGPUResources::coeffsPerProbe; ++coeffIdx) {
                        probeShCoeffs[coeffIdx] = *bakedShCoeffs;
                        ++bakedShCoeffs;
                    }
                }
            }
        }
        if (currentBake_.iteration == LIGHT_PROBE_BAKE_ITERATIONS - 1) {
            // Finish bake
            currentBake_ = {};
            currentBake_.status = LightProbeBakeStatusInternal::IDLE;
        } else {
            // Go to next bake iteration
            currentBake_.iteration++;
            currentBake_.iterationBakedProbesCount = 0u;
            currentBake_.status = LightProbeBakeStatusInternal::BAKE_IN_PROGRESS;
        }
    }

    return true;
}

bool LightProbeBakerSystem::InitializeGpuResources()
{
    if (!gpuResourceMgr_ || !gpuHandleMgr_) {
        return false;
    }

    // Create cubemap atlas
    if (!gpuResources_.cubemapAtlasTexture) {
        GpuImageDesc atlasDesc;
        atlasDesc.imageType = ImageType::CORE_IMAGE_TYPE_2D;
        atlasDesc.imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
        atlasDesc.format = Format::BASE_FORMAT_R16G16B16A16_SFLOAT;
        atlasDesc.width = gpuResources_.cubemapAtlasSize;
        atlasDesc.height = gpuResources_.cubemapAtlasSize;
        atlasDesc.depth = 1u;
        atlasDesc.mipCount = 1u;
        atlasDesc.imageTiling = ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
        atlasDesc.usageFlags = ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                               ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
        atlasDesc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        atlasDesc.engineCreationFlags = EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;

        RenderHandleReference atlasHandle = gpuResourceMgr_->Create("LightProbeCubemapAtlas", atlasDesc);

        if (!RenderHandleUtil::IsValid(atlasHandle.GetHandle())) {
#if (CORE3D_VALIDATION_ENABLED == 1)
            PLUGIN_LOG_E("CORE3D_VALIDATION: Failed to create light probe cubemap atlas texture");
#endif
            return false;
        }

        gpuResources_.cubemapAtlasTextureEntity = ecs_.GetEntityManager().CreateReferenceCounted();
        gpuHandleMgr_->Create(gpuResources_.cubemapAtlasTextureEntity);
        gpuHandleMgr_->Write(gpuResources_.cubemapAtlasTextureEntity)->reference = atlasHandle;
        gpuResources_.cubemapAtlasTexture =
            gpuHandleMgr_->GetRenderHandleReference(gpuResources_.cubemapAtlasTextureEntity);
    }
    // Create buffer for light probe SH data
    if (!gpuResources_.SHBuffer ||
        (static_cast<uint32_t>(currentBake_.lightProbeBakeInputs.size()) != gpuResources_.SHBufferSizeProbes)) {
        GpuBufferDesc SHBufferDesc;
        SHBufferDesc.byteSize = LightProbeBakeGPUResources::getBufferSizeBytes(
            static_cast<uint32_t>(currentBake_.lightProbeBakeInputs.size()));
        SHBufferDesc.engineCreationFlags =
            EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS |
            EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER;

        // Support reading back results to CPU directly with no staging buffer
        SHBufferDesc.memoryPropertyFlags = RENDER_NS::MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                           RENDER_NS::MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_CACHED_BIT;
        SHBufferDesc.usageFlags = RENDER_NS::BufferUsageFlagBits::CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                  RENDER_NS::BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_SRC_BIT;
        RenderHandleReference SHBufferHandle = gpuResourceMgr_->Create("SHBuffer", SHBufferDesc);

        if (!RenderHandleUtil::IsValid(SHBufferHandle.GetHandle())) {
#if (CORE3D_VALIDATION_ENABLED == 1)
            PLUGIN_LOG_E("CORE3D_VALIDATION: Failed to create light probe SH buffer");
#endif
            return false;
        }

        gpuResources_.SHBufferEntity = ecs_.GetEntityManager().CreateReferenceCounted();
        gpuHandleMgr_->Create(gpuResources_.SHBufferEntity);
        gpuHandleMgr_->Write(gpuResources_.SHBufferEntity)->reference = SHBufferHandle;
        gpuResources_.SHBuffer = gpuHandleMgr_->GetRenderHandleReference(gpuResources_.SHBufferEntity);
        gpuResources_.SHBufferSizeProbes = static_cast<uint32_t>(currentBake_.lightProbeBakeInputs.size());
    }

    return true;
}

void LightProbeBakerSystem::CreatePerProbeBakingData(
    BASE_NS::vector<RenderCamera::LightProbeBakingData::PerProbeData>& perProbeData)
{
    // Every frame, bake up to maxProbesPerFrame of remaining probes to bake
    // Good limit depends on scene complexity, so probably need to adapt according to frame rate
    constexpr uint32_t maxProbesPerFrame = 2u;
    auto frameProbes =
        sub_view(currentBake_.lightProbeBakeInputs, currentBake_.iterationBakedProbesCount, maxProbesPerFrame);
    for (uint32_t i = 0; i < frameProbes.size(); ++i) {
        RenderCamera::LightProbeBakingData::PerProbeData probeBakeData;

        // Needed for writing SH results
        probeBakeData.probeIndex = currentBake_.iterationBakedProbesCount + i;

        // Position where probe cubemap will be rasterized from
        probeBakeData.probePosition = frameProbes[i].position;

        // Areas in atlas where the cubemap faces should be rasterized
        uint32_t probeAtlasOffsetX = (i % gpuResources_.atlasProbesPerRow) * 6u * gpuResources_.cubemapFaceResolution;
        uint32_t probeAtlasOffsetY = (i / gpuResources_.atlasProbesPerRow) * gpuResources_.cubemapFaceResolution;
        for (uint32_t face = 0; face < 6u; ++face) {
            auto& renderArea = probeBakeData.probeAtlasRenderAreas[face];
            renderArea.offsetX = static_cast<int32_t>(probeAtlasOffsetX + face * gpuResources_.cubemapFaceResolution);
            renderArea.offsetY = static_cast<int32_t>(probeAtlasOffsetY);
            renderArea.extentWidth = gpuResources_.cubemapFaceResolution;
            renderArea.extentHeight = gpuResources_.cubemapFaceResolution;
        }

        perProbeData.push_back(probeBakeData);
    }
}

uint64_t LightProbeBakerSystem::CreateLightProbeBakingCamera()
{
    const RenderConfigurationComponent renderConfig = GetRenderConfigurationComponent(nodeMgr_, renderConfigMgr_);

    // Find environment to use for bakes
    CORE3D_NS::RenderCamera::Environment environment{};
    if (Entity mainCameraEntity = FindMainCamera(cameraMgr_, renderConfig); EntityUtil::IsValid(mainCameraEntity)) {
        // Main camera environment
        RenderCamera mainCamera = dsCamera_->GetCamera(mainCameraEntity.id);
        environment = mainCamera.environment;
    } else {
        // Default environment
        environment = dsCamera_->GetEnvironment(RenderSceneDataConstants::INVALID_ID);
    }

    BASE_NS::vector<RenderCamera::LightProbeBakingData::PerProbeData> probeBakingData{};
    CreatePerProbeBakingData(probeBakingData);
    if (probeBakingData.empty()) {
        return RenderSceneDataConstants::INVALID_ID;
    }
    currentBake_.iterationBakedProbesCount += static_cast<uint32_t>(probeBakingData.size());

    RenderCamera camera;
    camera.id = currentBake_.cameraId;
    camera.sceneId = 0;
    camera.layerMask = LayerConstants::DEFAULT_LAYER_MASK;

    const Math::Vec3 position = {0.0f, 0.0f, 0.0f};
    const Math::Vec3 target = {1.0f, 0.0f, 0.0f};
    const Math::Vec3 upVector = {0.0f, -1.0f, 0.0f};
    camera.matrices.view = Math::LookAtRh(position, target, upVector);

    // 90-degree perspective projection for cubemap
    camera.matrices.proj = Math::PerspectiveRhZo(90.0f * BASE_NS::Math::DEG2RAD, 1.0f, 0.1f, 1000.0f);
    camera.matrices.proj[1][1] *= -1.0f;

    camera.viewport = {0.0, 0.0, 1.0, 1.0};
    camera.scissor = {0.0, 0.0, 1.0, 1.0};

    camera.renderResolution = {gpuResources_.cubemapAtlasSize, gpuResources_.cubemapAtlasSize};

    camera.colorTargets[0u] = gpuResources_.cubemapAtlasTexture;

    camera.zNear = 0.1f;
    camera.zFar = 1000.0f;
    camera.flags = RenderCamera::CAMERA_FLAG_CLEAR_COLOR_BIT | RenderCamera::CAMERA_FLAG_CLEAR_DEPTH_BIT |
                   RenderCamera::CAMERA_FLAG_LIGHT_PROBE_BAKE_BIT | RenderCamera::CAMERA_FLAG_CUSTOM_TARGETS_BIT;
    camera.renderPipelineType = RenderCamera::RenderPipelineType::LIGHT_FORWARD;
    camera.clearColorValues = {0.0f, 0.0f, 0.0f, 1.0f};
    camera.cullType = RenderCamera::CameraCullType::CAMERA_CULL_VIEW_FRUSTUM;

    camera.name = "LightProbeBakingCamera";
    camera.lightProbeBakingData.perProbeData = std::move(probeBakingData);
    camera.lightProbeBakingData.probeAtlasWidth = gpuResources_.cubemapAtlasSize;
    camera.lightProbeBakingData.probeAtlasHeight = gpuResources_.cubemapAtlasSize;

    camera.matrices.envProj = camera.matrices.proj;
    camera.postProcessName = "";
    camera.environment = environment;

    // Use radiance cubemap as the background for light probe rasterization.
    // For example, in empty scene light probe would have same SH as the offline baked SHs in the environment component.
    if (environment.radianceCubemap) {
        camera.environment.backgroundType = RenderCamera::Environment::BG_TYPE_CUBEMAP;
        camera.environment.envMap = environment.radianceCubemap;
    }

    dsCamera_->AddCamera(camera);

    return camera.id;
}

bool LightProbeBakerSystem::StartBake(BASE_NS::array_view<CORE_NS::Entity> lightProbeGroups)
{
    if (currentBake_.status == LightProbeBakeStatusInternal::IDLE) {
        currentBake_ = {};
        currentBake_.status = LightProbeBakeStatusInternal::BAKE_TRIGGERED;
        currentBake_.lightProbeGroupEntities = BASE_NS::vector<CORE_NS::Entity>(
            lightProbeGroups.data(), lightProbeGroups.data() + lightProbeGroups.size());
        return true;
    }
    // Busy with previous bake
    return false;
}

bool LightProbeBakerSystem::CancelBake()
{
    if (currentBake_.status == LightProbeBakeStatusInternal::READBACK_TRIGGERED) {
        // Right now, readback expects SHReadbackData to stay alive, therefore we cannot
        // release the data safely.
        return false;
    }
    currentBake_ = {};
    return true;
}

ILightProbeBakerSystem::State LightProbeBakerSystem::GetStatus() const
{
    State state;
    switch (currentBake_.status) {
        case LightProbeBakeStatusInternal::IDLE:
            state.status = LightProbeBakeStatus::IDLE;
            break;
        case LightProbeBakeStatusInternal::BAKE_TRIGGERED:
        case LightProbeBakeStatusInternal::BAKE_IN_PROGRESS:
            state.status = LightProbeBakeStatus::BAKE_IN_PROGRESS;
            break;
        case LightProbeBakeStatusInternal::READBACK_TRIGGERED:
            state.status = LightProbeBakeStatus::BAKE_IN_PROGRESS;
            break;
    };
    uint32_t probesBaked = currentBake_.iteration * static_cast<uint32_t>(currentBake_.lightProbeBakeInputs.size()) +
                           currentBake_.iterationBakedProbesCount;
    uint32_t totalBakes = LIGHT_PROBE_BAKE_ITERATIONS * static_cast<uint32_t>(currentBake_.lightProbeBakeInputs.size());
    state.progress = static_cast<float>(probesBaked) / static_cast<float>(Math::max(1U, totalBakes));
    return state;
}

RenderHandleReference LightProbeBakerSystem::GetRenderNodeGraph() const
{
    if (currentBake_.status == LightProbeBakeStatusInternal::BAKE_IN_PROGRESS) {
        return currentBake_.rng;
    }
    return {};
}

void LightProbeBakerSystem::OnComponentEvent(
    EventType type, const IComponentManager& componentManager, const array_view<const Entity> entities)
{}

CORE_NS::ISystem* ILightProbeBakerSystemInstance(IEcs& ecs)
{
    return new LightProbeBakerSystem(ecs);
}

void ILightProbeBakerSystemDestroy(CORE_NS::ISystem* instance)
{
    delete static_cast<LightProbeBakerSystem*>(instance);
}

//////////
// LightProbeBaker interface
//////////

constexpr Uid LIGHT_PROBE_BAKER_SYSTEM_R_DEPS[] = {
    INodeComponentManager::UID,
    IWorldMatrixComponentManager::UID,
    IRenderHandleComponentManager::UID,
    ICameraComponentManager::UID,
    IRenderConfigurationComponentManager::UID,
};
constexpr Uid LIGHT_PROBE_BAKER_SYSTEM_RW_DEPS[] = {
    ILightProbeGroupComponentManager::UID,
};

constexpr SystemTypeInfo systemInfo{
    {SystemTypeInfo::UID},
    ILightProbeBakerSystem::UID,
    CORE_NS::GetName<ILightProbeBakerSystem>().data(),
    ILightProbeBakerSystemInstance,
    ILightProbeBakerSystemDestroy,
    LIGHT_PROBE_BAKER_SYSTEM_RW_DEPS,
    LIGHT_PROBE_BAKER_SYSTEM_R_DEPS,
    {CORE3D_NS::IRenderSystem::UID},  // Run after RenderSystem, which manages camera data store
    {},                               // beforeSystem
};

void LightProbeBaker::Initialize(IEcs& ecs)
{
    GetPluginRegister().RegisterTypeInfo(systemInfo);
    lightProbeBakerSystem_ = static_cast<LightProbeBakerSystem*>(ecs.CreateSystem(systemInfo));
}

LightProbeBaker::~LightProbeBaker()
{
    GetPluginRegister().UnregisterTypeInfo(systemInfo);
}

bool LightProbeBaker::StartBake(BASE_NS::array_view<CORE_NS::Entity> lightProbeGroups)
{
    return lightProbeBakerSystem_->StartBake(lightProbeGroups);
}

bool LightProbeBaker::CancelBake()
{
    return lightProbeBakerSystem_->CancelBake();
}

ILightProbeBakerSystem::State LightProbeBaker::GetStatus() const
{
    return lightProbeBakerSystem_->GetStatus();
}

RenderHandleReference LightProbeBaker::GetRenderNodeGraph() const
{
    return lightProbeBakerSystem_->GetRenderNodeGraph();
}
CORE3D_END_NAMESPACE()
