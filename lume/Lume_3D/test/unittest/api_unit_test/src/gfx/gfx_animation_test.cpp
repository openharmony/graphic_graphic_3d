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

#include <array>
#include <vulkan/vulkan.h>

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/post_process_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/ecs/systems/intf_morphing_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/ecs/systems/intf_render_system.h>
#include <3d/gltf/gltf.h>
#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/unique_ptr.h>
#include <base/math/quaternion_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/engine_info.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/plugin/intf_plugin_register.h>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/gles/intf_device_gles.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/util/intf_render_frame_util.h>
#include <render/util/intf_render_util.h>
#include <render/vulkan/intf_device_vk.h>

#include "gfx/gfx_common.h"
#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace {
Entity CreateLight(IEcs& ecs, const LightComponent& lightComponent, const TransformComponent& transformComponent)
{
    IEntityManager& em = ecs.GetEntityManager();

    Entity light = em.Create();

    GetManager<ITransformComponentManager>(ecs)->Set(light, transformComponent);

    auto lmm = GetManager<ILocalMatrixComponentManager>(ecs);
    lmm->Create(light);

    auto wmm = GetManager<IWorldMatrixComponentManager>(ecs);
    wmm->Create(light);

    auto ncm = GetManager<INodeComponentManager>(ecs);
    ncm->Create(light);

    auto lcm = GetManager<ILightComponentManager>(ecs);
    lcm->Set(light, lightComponent);

    return light;
}
void AnimationWithReflectionTest(UTest::TestResources& res)
{
    auto nodeSystem = GetSystem<INodeSystem>(res.GetEcs());
    auto& sceneUtil = res.GetGraphicsContext().GetSceneUtil();
    auto& meshUtil = res.GetGraphicsContext().GetMeshUtil();

    auto rootNode = nodeSystem->CreateNode();
    Entity sceneEntity = rootNode->GetEntity();
    IEntityManager& em = res.GetEcs().GetEntityManager();
    INameComponentManager* nameManager = GetManager<INameComponentManager>(res.GetEcs());
    IMaterialComponentManager* materialManager = GetManager<IMaterialComponentManager>(res.GetEcs());

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

    const vector<UTest::GltfImportInfo> files = {
        { "test://gltf/Environment/glTF/Environment.gltf", UTest::GltfImportInfo::AnimateImportedScene,
            CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL, CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL },
        { "test://gltf/BrainStem/glTF/BrainStem.gltf", UTest::GltfImportInfo::AnimateImportedScene,
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
            // Import the default scene, or first scene if there is no default scene set.
            size_t sceneIndex = gltf.data->GetDefaultSceneIndex();
            if (sceneIndex == CORE_GLTF_INVALID_INDEX && gltf.data->GetSceneCount() > 0) {
                // Use first scene.
                sceneIndex = 0;
            }

            Entity importedSceneEntity;
            if (sceneIndex != CORE_GLTF_INVALID_INDEX) {
                importedSceneEntity = res.GetGraphicsContext().GetGltf().ImportGltfScene(
                    sceneIndex, *gltf.data, gltfImportResult.data, res.GetEcs(), sceneEntity, info.sceneImportFlags);
            }

            // Assume that we always animate the 1st imported scene.
            Entity animationRoot = sceneEntity;
            if (info.target == UTest::GltfImportInfo::AnimateImportedScene &&
                EntityUtil::IsValid(importedSceneEntity)) {
                // Scenes are self contained, so animate this particular imported scene.
                animationRoot = importedSceneEntity;
            }

            // Create animation playbacks.
            if (!gltfImportResult.data.animations.empty()) {
                IAnimationSystem* animationSystem = GetSystem<IAnimationSystem>(res.GetEcs());
                if (auto animationRootNode = nodeSystem->GetNode(animationRoot); animationRootNode) {
                    for (const auto& animation : gltfImportResult.data.animations) {
                        IAnimationPlayback* playback = animationSystem->CreatePlayback(animation, *animationRootNode);
                        if (playback) {
                            playback->SetPlaybackState(AnimationComponent::PlaybackState::PLAY);
                            playback->SetRepeatCount(AnimationComponent::REPEAT_COUNT_INFINITE);
                        }
                    }
                }
            }
            res.AppendResources(gltfImportResult.data);
        } else {
            CORE_LOG_E("Importing of '%s' failed: %s", info.filename, gltfImportResult.error.c_str());
        }
    }

    // Post-init after import.
    Entity postProcessEntity;
    Entity cameraEntity;
    {
        // post process component
        {
            auto postProcessManager = GetManager<IPostProcessComponentManager>(res.GetEcs());
            postProcessEntity = em.Create();
            postProcessManager->Create(postProcessEntity);
            if (auto postProcessHandle = postProcessManager->Write(postProcessEntity); postProcessHandle) {
                postProcessHandle->enableFlags =
                    PostProcessConfiguration::ENABLE_VIGNETTE_BIT | PostProcessConfiguration::ENABLE_TONEMAP_BIT |
                    PostProcessConfiguration::ENABLE_COLOR_FRINGE_BIT | PostProcessConfiguration::ENABLE_BLOOM_BIT;
                postProcessHandle->vignetteConfiguration.coefficient = 1.0f;
                postProcessHandle->vignetteConfiguration.power = 1.0f;
                postProcessHandle->bloomConfiguration.thresholdHard = 0.9f;
                postProcessHandle->bloomConfiguration.thresholdSoft = 2.0f;
                postProcessHandle->colorFringeConfiguration.coefficient = 1.5f;
                postProcessHandle->colorFringeConfiguration.distanceCoefficient = 2.5f;
            }
        }

        // render configuration component
        {
            IRenderConfigurationComponentManager* rccm = GetManager<IRenderConfigurationComponentManager>(res.GetEcs());
            rccm->Create(sceneEntity);
            ScopedHandle<RenderConfigurationComponent> renderConfiguration = rccm->Write(sceneEntity);
            renderConfiguration->renderingFlags |= RenderConfigurationComponent::CREATE_RNGS_BIT;

            IEnvironmentComponentManager* environmentManager = GetManager<IEnvironmentComponentManager>(res.GetEcs());
            if (environmentManager->GetComponentCount() == 0) {
                environmentManager->Create(sceneEntity);
            }
            renderConfiguration->environment = environmentManager->GetEntity(0);

            if (auto envDataHandle = environmentManager->Write(renderConfiguration->environment); envDataHandle) {
                EnvironmentComponent& envComponent = *envDataHandle;
                envComponent.indirectDiffuseFactor.w = 0.5f;
                envComponent.indirectSpecularFactor.w = 0.5f;
                envComponent.envMapFactor.w = 0.5f;
            }
        }

        // camera  component
        {
            cameraEntity = sceneUtil.CreateCamera(res.GetEcs(), Math::Vec3(0.0f, 2.75f, 3.5f),
                Math::AngleAxis((Math::DEG2RAD * -25.0f), Math::Vec3(1.0f, 0.0f, 0.0f)), 0.1f, 100.0f, 60.0f);
            ICameraComponentManager* cameraManager = GetManager<ICameraComponentManager>(res.GetEcs());
            ScopedHandle<CameraComponent> cameraComponent = cameraManager->Write(cameraEntity);
            cameraComponent->sceneFlags |= CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT;
            cameraComponent->pipelineFlags |= CameraComponent::PipelineFlagBits::MSAA_BIT |
                                              CameraComponent::PipelineFlagBits::ALLOW_COLOR_PRE_PASS_BIT;
            cameraComponent->renderingPipeline = CameraComponent::RenderingPipeline::FORWARD;
            cameraComponent->clearColorValue = { 1.0f, 0.0f, 0.0f, 1.0f };
            cameraComponent->postProcess = postProcessEntity;
        }

        if (GetManager<ILightComponentManager>(res.GetEcs())->GetComponentCount() == 0) {
            // Create default light.
            LightComponent lc;
            lc.type = LightComponent::Type::DIRECTIONAL;
            lc.color = { 0.96f, 1.0f, 0.98f };
            lc.intensity = 5.0f;
            lc.shadowEnabled = true;
            sceneUtil.CreateLight(res.GetEcs(), lc, Math::Vec3(0.0f, 0.0f, 3.0f),
                Math::AngleAxis((Math::DEG2RAD * -45.0f), Math::Vec3(1.0f, 0.0f, 0.0f)));
        }
        {
            LightComponent lc;
            lc.type = LightComponent::Type::POINT;
            lc.color = { 1.0f, 0.2f, 0.2f };
            lc.intensity = 2.0f;
            lc.range = 1.0f;
            lc.spotInnerAngle = 0.785398163397448f;
            lc.spotOuterAngle = 1.570796326794897f;
            lc.shadowEnabled = false;

            TransformComponent tc;
            tc.position = { -1.0f, 1.0f, 0.0f };
            tc.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
            tc.scale = { 1.0f, 1.0f, 1.0f };

            CreateLight(res.GetEcs(), lc, tc);
        }

        {
            LightComponent lc;
            lc.type = LightComponent::Type::SPOT;
            lc.color = { 0.5f, 1.0f, 1.0f };
            lc.intensity = 6.0f;
            lc.range = 0.0f;
            lc.spotInnerAngle = Math::DEG2RAD * 45.0f;
            lc.spotOuterAngle = Math::DEG2RAD * 60.0f;
            lc.shadowEnabled = true;

            TransformComponent tc;
            tc.position = { 1.0f, 1.0f, 0.0f };
            tc.rotation = Math::Euler(-95.0f, 0.0f, 0.0f);
            tc.scale = { 1.0f, 1.0f, 1.0f };

            CreateLight(res.GetEcs(), lc, tc);
        }
    }
    sceneUtil.UpdateCameraViewport(
        res.GetEcs(), cameraEntity, { res.GetWindowWidth(), res.GetWindowHeight() }, true, Math::DEG2RAD * 75.0f, 1.0f);
}
} // namespace

/**
 * @tc.name: AnimationWithReflectionTest
 * @tc.desc: Tests for Animation With Reflection Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, AnimationWithReflectionTest, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(800u, 500u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    AnimationWithReflectionTest(res);

    res.GetEcs().SetTimeScale(15.0f);
    for (uint32_t i = 0; i < 3; ++i) {
        res.TickTestAutoRng(1);

        if (res.GetByteArray()) {
            const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
            UTest::WritePng(
                BASE_NS::string(appDir + string("./animationWithReflectionTestVulkan") + to_string(i) + ".png").c_str(),
                res.GetWindowWidth(), res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(),
                res.GetWindowWidth() * 4);
        }
    }
    res.GetEcs().SetTimeScale(1.0f);

    res.ShutdownTest();
}
