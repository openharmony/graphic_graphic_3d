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

#include "gfx_cloud_test.h"

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/planar_reflection_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/water_ripple_component.h>
#include <3d/ecs/components/weather_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/ecs/systems/intf_weather_system.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_scene_util.h>
#include <base/math/quaternion_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/image/intf_image_loader_manager.h>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/util/intf_render_frame_util.h>
#include <render/util/intf_render_util.h>
#include <render/util/performance_data_structures.h>

// Cloud test
#include <3d/util/intf_mesh_util.h>

#include "gfx_common.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace Cloud {

struct PlaneMat {
    Entity entity;
    Entity matEntity;
};

static EntityReference LoadImage(IImageLoaderManager& imageManager, const string_view& uri,
    IGpuResourceManager& gpuResourceMgr, IRenderHandleComponentManager& handleManager, const bool mips)
{
    auto result =
        imageManager.LoadImage(uri, mips ? IImageLoaderManager::ImageLoaderFlags::IMAGE_LOADER_GENERATE_MIPS : 0);
    if (!result.success || !result.image) {
        return EntityReference {};
    }
    CORE_ASSERT(result.success);
    GpuImageDesc gpuImageDesc = gpuResourceMgr.CreateGpuImageDesc(result.image->GetImageDesc());
    RenderHandleReference backgroundImageHandle = gpuResourceMgr.Create(gpuImageDesc, std::move(result.image));

    auto imageEntity = handleManager.GetEcs().GetEntityManager().CreateReferenceCounted();
    handleManager.Create(imageEntity);
    handleManager.Write(imageEntity)->reference = move(backgroundImageHandle);
    return imageEntity;
}

void TickTest(UTest::TestResources& res, int frameCountToTick)
{
    auto* ecs = &res.GetEcs();
    auto& engine = res.GetEngine();
    auto& renderContext = res.GetRenderContext();
    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    auto rootNode = nodeSystem->CreateNode();

    auto& rdsMgr = renderContext.GetRenderDataStoreManager();
    uint32_t numBytes = res.GetWindowWidth() * res.GetWindowHeight() * 4;
    unique_ptr<ByteArray> byteArray = make_unique<ByteArray>(numBytes);

    IGpuResourceManager& gpuResourceMgr = renderContext.GetDevice().GetGpuResourceManager();
    IRenderFrameUtil& rfUtil = renderContext.GetRenderUtil().GetRenderFrameUtil();
    const RenderHandleReference outputImageHandle0 = gpuResourceMgr.GetImageHandle("TestOutputImage");

    for (int i = 0; i < frameCountToTick; i++) {
        const bool needsRender = engine.TickFrame(array_view(&ecs, 1));
        IRenderer& renderer = renderContext.GetRenderer();

        if (i == frameCountToTick - 1) {
            const IRenderFrameUtil::CopyFlags copyFlags = IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE;
            rfUtil.CopyToCpu(outputImageHandle0, copyFlags);
            renderer.RenderFrame({});
        } else {
            if (needsRender) {
                renderer.RenderFrame(res.GetGraphicsContext().GetRenderNodeGraphs(*ecs));
            }
        }
        if (i == frameCountToTick - 1) {
            const IRenderFrameUtil::FrameCopyData& data = rfUtil.GetFrameCopyData(outputImageHandle0);
            const GpuBufferDesc desc = gpuResourceMgr.GetBufferDescriptor(data.bufferHandle);
            const uint32_t minSize = Math::min(desc.byteSize, static_cast<uint32_t>(data.byteBuffer->GetData().size()));

            if (data.byteBuffer && (data.copyFlags & IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE)) {
                const uint32_t minSize =
                    Math::min(desc.byteSize, static_cast<uint32_t>(data.byteBuffer->GetData().size()));
                // map the GPU buffer memory and copy to CPU ByteArray buffer
                if (void* mem = gpuResourceMgr.MapBufferMemory(data.bufferHandle); mem) {
                    BASE_NS::CloneData(byteArray->GetData().data(), data.byteBuffer->GetData().size(), mem, minSize);
                }
            }
        }
    }

    if (byteArray) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        const char* backendName = res.GetDeviceBackendType() == DeviceBackendType::VULKAN ? "Vulkan" : "OpenGL";
        UTest::WritePng(BASE_NS::string(appDir + "/cloud" + backendName + ".png").c_str(), res.GetWindowWidth(),
            res.GetWindowHeight(), 4, byteArray->GetData().data(), res.GetWindowWidth() * 4); // 4: parm
    }
}

void CloudTest(UTest::TestResources& res)
{
    auto& ecs = res.GetEcs();
    auto& engine = res.GetEngine();

    IEntityManager& em = ecs.GetEntityManager();

    auto nodeSystem = GetSystem<INodeSystem>(ecs);
    ISceneNode* scene = nodeSystem->CreateNode();
    const Entity sceneRoot = scene->GetEntity();

    auto materialManager = GetManager<IMaterialComponentManager>(ecs);
    auto nameManager = GetManager<INameComponentManager>(ecs);
    auto uriManager = GetManager<IUriComponentManager>(ecs);
    auto evnManager = GetManager<IEnvironmentComponentManager>(ecs);

    auto& meshUtil = res.GetGraphicsContext().GetMeshUtil();
    auto& sceneUtil = res.GetGraphicsContext().GetSceneUtil();
    auto& renderContext = res.GetRenderContext();
    auto& shaderManager = res.GetRenderContext().GetDevice().GetShaderManager();
    auto& gpuResourceMgr = renderContext.GetDevice().GetGpuResourceManager();
    auto handleManager = GetManager<IRenderHandleComponentManager>(ecs);
    auto& imageManager = engine.GetImageLoaderManager();
    auto rootNode = nodeSystem->CreateNode();

    engine.GetFileManager().RegisterPath("appimages", "file://submodules/LumeCoreAssets/weather/images/", false);

    // camera component
    Entity cameraEntity;
    {
        cameraEntity = sceneUtil.CreateCamera(res.GetEcs(), Math::Vec3(0.0f, 2.75f, 3.5f),
            Math::AngleAxis((Math::DEG2RAD * -5.0f), Math::Vec3(1.0f, 0.0f, 0.0f)), 0.1f, 100.0f, 60.0f);
        ICameraComponentManager* cameraManager = GetManager<ICameraComponentManager>(res.GetEcs());
        ScopedHandle<CameraComponent> cameraComponent = cameraManager->Write(cameraEntity);
        cameraComponent->sceneFlags |= CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT;
        cameraComponent->pipelineFlags |=
            CameraComponent::PipelineFlagBits::MSAA_BIT | CameraComponent::PipelineFlagBits::ALLOW_COLOR_PRE_PASS_BIT;
        cameraComponent->renderingPipeline = CameraComponent::RenderingPipeline::FORWARD;
        cameraComponent->clearColorValue = { 1.0f, 0.0f, 0.0f, 1.0f };
        cameraComponent->layerMask = 0xFFFFFFFF;
    }
    sceneUtil.UpdateCameraViewport(
        res.GetEcs(), cameraEntity, { res.GetWindowWidth(), res.GetWindowHeight() }, true, Math::DEG2RAD * 90.0f, 1.0f);

    // render configuration component
    {
        IRenderConfigurationComponentManager* rccm = GetManager<IRenderConfigurationComponentManager>(res.GetEcs());
        rccm->Create(sceneRoot);
        ScopedHandle<RenderConfigurationComponent> renderConfiguration = rccm->Write(sceneRoot);
        renderConfiguration->renderingFlags |= RenderConfigurationComponent::CREATE_RNGS_BIT;

        IEnvironmentComponentManager* environmentManager = GetManager<IEnvironmentComponentManager>(res.GetEcs());
        if (environmentManager->GetComponentCount() == 0) {
            environmentManager->Create(sceneRoot);
        }
        renderConfiguration->environment = environmentManager->GetEntity(0);

        if (auto envDataHandle = environmentManager->Write(renderConfiguration->environment); envDataHandle) {
            EnvironmentComponent& envComponent = *envDataHandle;
            envComponent.indirectDiffuseFactor.w = 0.5f;
            envComponent.indirectSpecularFactor.w = 0.5f;
            envComponent.envMapFactor.w = 0.5f;
            envComponent.background = EnvironmentComponent::Background::SKY;
            envComponent.weather = em.GetReferenceCounted(sceneRoot);
        }
    }

    // Reflection plane
    Entity reflectionPlaneMaterial = em.Create();
    Entity reflectionPlane;
    {
        nameManager->Create(reflectionPlaneMaterial);
        nameManager->Set(reflectionPlaneMaterial, { { "ReflectionPlaneMaterial" } });
        materialManager->Create(reflectionPlaneMaterial);
        reflectionPlane =
            meshUtil.GeneratePlane(res.GetEcs(), "ReflectionPlane", reflectionPlaneMaterial, 10.0f, 10.0f);
        if (ISceneNode* node = nodeSystem->GetNode(reflectionPlane); node) {
            node->SetPosition(Math::Vec3(0.0f, 0.0f, 0.0f));
            node->SetScale(Math::Vec3(10, 0.01f, 10)); // 10: parm
        }
        sceneUtil.CreateReflectionPlaneComponent(res.GetEcs(), reflectionPlane);
        if (auto materialHandle = materialManager->Write(reflectionPlaneMaterial); materialHandle) {
            materialHandle->materialLightingFlags &= ~MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT;
            materialHandle->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor =
                Math::Vec4(0.1f, 0.1f, 0.1f, 1.0f);
            materialHandle->textures[MaterialComponent::TextureIndex::MATERIAL].factor =
                Math::Vec4(0.f, 0.01f, 0.0f, 0.3f);
        }
    }

    // Gltf model
    const vector<UTest::GltfImportInfo> files = {
        { "test://gltf/BrainStem/glTF/BrainStem.gltf", UTest::GltfImportInfo::AnimateImportedScene,
            CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL, CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL },
    };

    CORE3D_NS::ISceneNode* model;

    // Load and import all gltf files.
    for (size_t importedFileIndex = 0; importedFileIndex < files.size(); ++importedFileIndex) {
        auto const& info = files[importedFileIndex];

        auto gltf = res.GetGraphicsContext().GetGltf().LoadGLTF(info.filename);
        if (!gltf.success) {
            CORE_LOG_E("Loading '%s' resulted in errors:\n%s", info.filename, gltf.error.c_str());
            continue;
        }

        auto importer = res.GetGraphicsContext().GetGltf().CreateGLTF2Importer(res.GetEcs());
        importer->ImportGLTF(*gltf.data, info.resourceImportFlags);

        const auto& gltfImportResult = importer->GetResult();

        if (gltfImportResult.success) {
            size_t sceneIndex = gltf.data->GetDefaultSceneIndex();
            if (sceneIndex == CORE_GLTF_INVALID_INDEX && gltf.data->GetSceneCount() > 0) {
                sceneIndex = 0;
            }

            Entity importedSceneEntity;
            if (sceneIndex != CORE_GLTF_INVALID_INDEX) {
                importedSceneEntity = res.GetGraphicsContext().GetGltf().ImportGltfScene(
                    sceneIndex, *gltf.data, gltfImportResult.data, res.GetEcs(), sceneRoot, info.sceneImportFlags);
            }
            res.AppendResources(gltfImportResult.data);
            model = nodeSystem->GetNode(importedSceneEntity);
            Math::Quat rot = Math::FromEulerRad(Math::Vec3 { 0.f, 0.f, 0.f });
            model->SetName(info.filename);
            model->SetPosition(Math::Vec3 { 0.0f, 0.0f, 0.0f });
            model->SetRotation(rot);

            if (auto* wrm = GetManager<IWaterRippleComponentManager>(ecs); wrm) {
                wrm->Create(model->GetEntity());
                if (auto ripple = wrm->Write(model->GetEntity())) {
                    ripple->position.x = 0.0f;
                    ripple->position.y = 0.0f;
                }
            }
        } else {
            CORE_LOG_E("Importing of '%s' failed: %s", info.filename, gltfImportResult.error.c_str());
        }
    }

    // Weather component
    auto* wcm = GetManager<IWeatherComponentManager>(ecs);
    wcm->Create(sceneRoot);
    if (auto handle = wcm->Write(sceneRoot)) {
        // Temporary solution: Use only 2 of the required textures to reduce the size of test images
        handle->lowFrequencyImage =
            LoadImage(imageManager, "test://image/weather/HighFrequency.ktx", gpuResourceMgr, *handleManager, true);
        handle->highFrequencyImage =
            LoadImage(imageManager, "test://image/weather/HighFrequency.ktx", gpuResourceMgr, *handleManager, true);
        handle->curlNoiseImage =
            LoadImage(imageManager, "test://image/weather/curlNoise.png", gpuResourceMgr, *handleManager, false);
        handle->cirrusImage =
            LoadImage(imageManager, "test://image/weather/curlNoise.png", gpuResourceMgr, *handleManager, false);
        handle->weatherMapImage =
            LoadImage(imageManager, "test://image/weather/curlNoise.png", gpuResourceMgr, *handleManager, false);
        handle->coverage = 0.3f;
        handle->windSpeed = 0.0f;
    }
}
} // namespace Cloud
