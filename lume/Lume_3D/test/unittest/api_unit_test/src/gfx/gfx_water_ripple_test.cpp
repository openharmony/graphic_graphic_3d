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

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/dynamic_environment_blender_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/planar_reflection_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/water_ripple_component.h>
#include <3d/ecs/components/weather_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_node_system.h>
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

#include "gfx_common.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace {

const uint32_t NUM_CUBES = 5;

struct PlaneMat {
    Entity entity;
    Entity matEntity;
};

void WaterRippleTest(UTest::TestResources& res)
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

    auto& meshUtil = res.GetGraphicsContext().GetMeshUtil();
    auto& sceneUtil = res.GetGraphicsContext().GetSceneUtil();
    auto& renderContext = res.GetRenderContext();
    IRenderConfigurationComponentManager* rcc = GetManager<IRenderConfigurationComponentManager>(ecs);
    rcc->Create(sceneRoot);

    const Math::Vec2 planeDims(3.0f, 1.0f);

    // camera component
    Entity cameraEntity;
    {
        cameraEntity = sceneUtil.CreateCamera(ecs, Math::Vec3(0.0f, 3.0f, 5.0f),
            Math::AngleAxis((Math::DEG2RAD * -5.0f), Math::Vec3(1.0f, 0.0f, 0.0f)), 0.1f, 100.0f, 60.0f);
        ICameraComponentManager* cameraManager = GetManager<ICameraComponentManager>(ecs);
        ScopedHandle<CameraComponent> cameraComponent = cameraManager->Write(cameraEntity);
        cameraComponent->sceneFlags |= CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT;
        cameraComponent->pipelineFlags |=
            CameraComponent::PipelineFlagBits::MSAA_BIT | CameraComponent::PipelineFlagBits::ALLOW_COLOR_PRE_PASS_BIT;
        cameraComponent->renderingPipeline = CameraComponent::RenderingPipeline::FORWARD;
        cameraComponent->clearColorValue = { 1.0f, 0.0f, 0.0f, 1.0f };
    }
    sceneUtil.UpdateCameraViewport(
        ecs, cameraEntity, { res.GetWindowWidth(), res.GetWindowHeight() }, true, Math::DEG2RAD * 90.0f, 1.0f);

    auto* wcm = GetManager<IWeatherComponentManager>(ecs);
    wcm->Create(sceneRoot);
    // Reflection plane
    Entity reflectionPlaneMaterial = em.Create();
    Entity reflectionPlane;
    {
        nameManager->Create(reflectionPlaneMaterial);
        nameManager->Set(reflectionPlaneMaterial, { { "ReflectionPlaneMaterial" } });
        materialManager->Create(reflectionPlaneMaterial);
        reflectionPlane = meshUtil.GeneratePlane(ecs, "ReflectionPlane", reflectionPlaneMaterial, 10.0f, 10.0f);
        if (ISceneNode* node = nodeSystem->GetNode(reflectionPlane); node) {
            node->SetPosition(Math::Vec3(0.0f, 0.0f, 0.0f));
            node->SetScale(Math::Vec3(planeDims.x, 0.01f, planeDims.y));
            node->SetRotation(Math::AngleAxis(10 * Math::DEG2RAD, Math::Vec3(1, 0, 0))); // 10: parm
        }
        sceneUtil.CreateReflectionPlaneComponent(ecs, reflectionPlane);

        auto prcm = GetManager<IPlanarReflectionComponentManager>(ecs);
        if (auto reflPlaneHandle = prcm->Write(reflectionPlane); reflPlaneHandle) {
            reflPlaneHandle->additionalFlags |= PlanarReflectionComponent::WATER_BIT;
        }

        if (auto materialHandle = materialManager->Write(reflectionPlaneMaterial); materialHandle) {
            materialHandle->materialLightingFlags &= ~MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT;
            materialHandle->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor =
                Math::Vec4(0.1f, 0.1f, 0.1f, 1.0f);
            // go higher for mirror
            materialHandle->textures[MaterialComponent::TextureIndex::MATERIAL].factor =
                Math::Vec4(0.f, 0.01f, 0.0f, 0.3f);
        }
    }

    // Gltf model
    const vector<UTest::GltfImportInfo> files = {
        { "test://gltf/Environment/glTF/Environment.gltf", UTest::GltfImportInfo::AnimateImportedScene,
            CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL, CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL },
    };

    // Load and import all gltf files.
    for (size_t importedFileIndex = 0; importedFileIndex < files.size(); ++importedFileIndex) {
        auto const& info = files[importedFileIndex];

        auto gltf = res.GetGraphicsContext().GetGltf().LoadGLTF(info.filename);
        if (!gltf.success) {
            CORE_LOG_E("Loading '%s' resulted in errors:\n%s", info.filename, gltf.error.c_str());
            continue;
        }

        auto importer = res.GetGraphicsContext().GetGltf().CreateGLTF2Importer(ecs);
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
                    sceneIndex, *gltf.data, gltfImportResult.data, ecs, sceneRoot, info.sceneImportFlags);
            }
            res.AppendResources(gltfImportResult.data);
        } else {
            CORE_LOG_E("Importing of '%s' failed: %s", info.filename, gltfImportResult.error.c_str());
        }
    }

    // render configuration component
    {
        IRenderConfigurationComponentManager* rccm = GetManager<IRenderConfigurationComponentManager>(ecs);
        rccm->Create(sceneRoot);
        ScopedHandle<RenderConfigurationComponent> renderConfiguration = rccm->Write(sceneRoot);
        renderConfiguration->renderingFlags |= RenderConfigurationComponent::CREATE_RNGS_BIT;

        IEnvironmentComponentManager* environmentManager = GetManager<IEnvironmentComponentManager>(ecs);
        if (environmentManager->GetComponentCount() == 0) {
            environmentManager->Create(sceneRoot);
        }
        renderConfiguration->environment = environmentManager->GetEntity(0);

        if (auto envDataHandle = environmentManager->Write(renderConfiguration->environment); envDataHandle) {
            EnvironmentComponent& envComponent = *envDataHandle;
            envComponent.indirectDiffuseFactor.w = 0.5f;
            envComponent.indirectSpecularFactor.w = 0.5f;
            envComponent.envMapFactor.w = 0.5f;
            envComponent.weather = em.GetReferenceCounted(sceneRoot);
        }
    }

    Entity whiteMaterial = em.Create();
    materialManager->Create(whiteMaterial);
    if (auto mc = materialManager->Write(whiteMaterial); mc) {
        mc->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor = Math::Vec4(1.0f);
    }

    // Make 5 cubes
    const Math::Vec3 startLoc = Math::Vec3(-6.0f, 1.0f, 0.0f);
    for (uint32_t i = 0; i < NUM_CUBES; i++) {
        auto cube =
            nodeSystem->GetNode(meshUtil.GenerateCube(ecs, "Cube" + to_string(i), whiteMaterial, 1.0f, 1.0f, 1.0f));
        cube->SetPosition(startLoc + Math::Vec3(i * 3, 0.0f, 0.0f)); // 3: parm
        if (auto* waterRippleManager = GetManager<IWaterRippleComponentManager>(ecs); waterRippleManager) {
            waterRippleManager->Create(cube->GetEntity());
            if (auto ripple = waterRippleManager->Write(cube->GetEntity())) {
                ripple->position.x = 0.f;
                ripple->position.y = 0.f;
            }
        }
    }
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

    constexpr uint32_t steps = 40;
    float cubeYPos[steps] = {};

    for (uint ii = 0; ii < steps; ii++) {
        const float t = float(ii) / steps;
        if (t < 0.5) { // 0.5: parm
            cubeYPos[ii] = Math::lerp(2.0f, -2.0f, t * 2); // 2: parm
        } else {
            cubeYPos[ii] = Math::lerp(-2.0f, 2.0f, (t - 0.5f) * 2); // 2: parm
        }
    }

    for (int i = 0; i < frameCountToTick; i++) {
        const bool needsRender = engine.TickFrame(array_view(&ecs, 1));
        IRenderer& renderer = renderContext.GetRenderer();

        if (i == frameCountToTick - 1) {
            const IRenderFrameUtil::CopyFlags copyFlags = IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE;
            rfUtil.CopyToCpu(outputImageHandle0, copyFlags);
            renderer.RenderFrame({});
        } else {
            auto nodeSystem = GetSystem<INodeSystem>(*ecs);
            auto& rootNode = nodeSystem->GetRootNode();

            for (uint32_t j = 0; j < NUM_CUBES; j++) {
                {
                    auto* node = rootNode.LookupNodeByName("Cube" + to_string(j));
                    if (node) {
                        float cubeY = cubeYPos[i % steps];
                        Math::Vec3 pos = node->GetPosition();
                        pos.y = cubeY;
                        node->SetPosition(pos);
                    }
                }
            }

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
        UTest::WritePng(BASE_NS::string(appDir + "/WaterRippleTestVulkan.png").c_str(), res.GetWindowWidth(),
            res.GetWindowHeight(), 4, byteArray->GetData().data(), res.GetWindowWidth() * 4); // 4: parm
    }
}

} // namespace

/**
 * @tc.name: WaterRippleTestVulkan
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, WaterRippleTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(800u, 500u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    WaterRippleTest(res);
    TickTest(res, 40);

    res.ShutdownTest();
}
