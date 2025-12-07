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

// Area Light test
#include <3d/util/intf_mesh_util.h>

#include "gfx_common.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace {

struct PlaneMat {
    Entity entity;
    Entity matEntity;
};

void AreaLightTest(UTest::TestResources& res)
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
    }
    sceneUtil.UpdateCameraViewport(
        res.GetEcs(), cameraEntity, { res.GetWindowWidth(), res.GetWindowHeight() }, true, Math::DEG2RAD * 90.0f, 1.0f);

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
            node->SetScale(Math::Vec3(10, 0.01f, 10));
        }
        sceneUtil.CreateReflectionPlaneComponent(res.GetEcs(), reflectionPlane);
        if (auto materialHandle = materialManager->Write(reflectionPlaneMaterial); materialHandle) {
            materialHandle->materialLightingFlags &= ~MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT;
            materialHandle->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor =
                Math::Vec4(0.1f, 0.1f, 0.1f, 1.0f);
            materialHandle->textures[MaterialComponent::TextureIndex::MATERIAL].factor =
                Math::Vec4(0.f, 0.0f, 0.0f, 0.0f);
        }
    }

    {
        LightComponent lc;
        lc.type = LightComponent::Type::RECT;
        lc.color = { 1.0f, 0.0f, 0.0f };
        lc.intensity = 50.0f;
        lc.range = 50.0f;
        lc.rectLight.height = 2.0f;
        lc.rectLight.width = 2.0f;
        lc.rectLight.twoSided = false;

        const Math::Vec3 position = { 3.0f, 1.0f, -1.5f };
        const Math::Quat rotation = Math::Euler(0.0f, 130.0f, 0.0f);

        const Entity lightEnt = sceneUtil.CreateLight(res.GetEcs(), lc, position, rotation);
        auto node = nodeSystem->GetNode(lightEnt);
        node->SetParent(*scene);
    }

    // Gltf model
    const vector<UTest::GltfImportInfo> files = {
        { "test://gltf/FlightHelmet/FlightHelmet.gltf", UTest::GltfImportInfo::AnimateImportedScene,
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
            ISceneNode* model = nodeSystem->GetNode(importedSceneEntity);
            Math::Quat rot = Math::FromEulerRad(Math::Vec3 { 0.f, Math::DEG2RAD * 30.f, 0.f });
            model->SetScale(Math::Vec3(10.0f));
            model->SetPosition(Math::Vec3 { 0.0f, 1.0f, -3.0f });
            model->SetRotation(rot);
        } else {
            CORE_LOG_E("Importing of '%s' failed: %s", info.filename, gltfImportResult.error.c_str());
        }
    }
}
} // namespace

/**
 * @tc.name: AreaLightTestVulkan
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, AreaLightTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(800u, 500u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    AreaLightTest(res);
    res.TickTest(10);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/area_light.png").c_str(), res.GetWindowWidth(),
            res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    res.ShutdownTest();
}
