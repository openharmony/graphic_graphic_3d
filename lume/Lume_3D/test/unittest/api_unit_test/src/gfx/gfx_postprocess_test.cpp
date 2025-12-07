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
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/post_process_effect_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_scene_util.h>
#include <base/math/quaternion_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/property/property_handle_util.h>
#include <core/property_tools/core_metadata.inl>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/nodecontext/intf_render_post_process.h>

#include "gfx_common.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace {
void PostprocessEffectSsao(UTest::TestResources& res)
{
    auto& ecs = res.GetEcs();
    auto& engine = res.GetEngine();

    IEntityManager& em = ecs.GetEntityManager();

    auto nodeSystem = GetSystem<INodeSystem>(ecs);
    ISceneNode* scene = nodeSystem->CreateNode();
    const Entity sceneRoot = scene->GetEntity();

    auto materialManager = GetManager<IMaterialComponentManager>(ecs);
    auto nameManager = GetManager<INameComponentManager>(ecs);
    auto evnManager = GetManager<IEnvironmentComponentManager>(ecs);

    auto& meshUtil = res.GetGraphicsContext().GetMeshUtil();
    auto& sceneUtil = res.GetGraphicsContext().GetSceneUtil();
    auto& renderContext = res.GetRenderContext();
    auto& shaderManager = res.GetRenderContext().GetDevice().GetShaderManager();
    auto& gpuResourceMgr = renderContext.GetDevice().GetGpuResourceManager();
    auto handleManager = GetManager<IRenderHandleComponentManager>(ecs);
    auto& imageManager = engine.GetImageLoaderManager();
    auto rootNode = nodeSystem->CreateNode();

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
            envDataHandle->background = EnvironmentComponent::Background::CUBEMAP;
        }
    }

    // postprocess component
    Entity postprocessEntity;
    {
        auto* renderClassFactory = res.GetRenderContext().GetInterface<CORE_NS::IClassFactory>();
        if (renderClassFactory) {
            auto combined =
                CreateInstance<IRenderPostProcess>(*renderClassFactory, RENDER_NS::UID_RENDER_POST_PROCESS_COMBINED);
            auto combinedProps = combined->GetProperties();
            CORE_NS::SetPropertyValue(combinedProps, "enabled", true);
            CORE_NS::SetPropertyValue(combinedProps, "postProcessConfiguration.enableFlags",
                uint32_t(RENDER_NS::PostProcessConfiguration::ENABLE_TONEMAP_BIT));
            CORE_NS::SetPropertyValue(combinedProps, "postProcessConfiguration.tonemapConfiguration.tonemapType",
                RENDER_NS::TonemapConfiguration::TONEMAP_PBR_NEUTRAL);
            CORE_NS::SetPropertyValue(combinedProps, "postProcessConfiguration.tonemapConfiguration.exposure", 1.f);

            auto ssao =
                CreateInstance<IRenderPostProcess>(*renderClassFactory, RENDER_NS::UID_RENDER_POST_PROCESS_SSAO);
            CORE_NS::SetPropertyValue(ssao->GetProperties(), "enabled", true);

            auto postprocessEffectMgr = GetManager<IPostProcessEffectComponentManager>(ecs);
            postprocessEntity = ecs.GetEntityManager().Create();
            postprocessEffectMgr->Create(postprocessEntity);
            if (auto handle = postprocessEffectMgr->Write(postprocessEntity)) {
                handle->effects.push_back(combined);
                handle->effects.push_back(ssao);
            }
        }
    }
    // camera component
    Entity cameraEntity;
    {
        cameraEntity = sceneUtil.CreateCamera(res.GetEcs(), Math::Vec3(0.0f, 0.75f, 3.5f),
            Math::AngleAxis((Math::DEG2RAD * -5.0f), Math::Vec3(1.0f, 0.0f, 0.0f)), 0.1f, 100.0f, 60.0f);
        ICameraComponentManager* cameraManager = GetManager<ICameraComponentManager>(res.GetEcs());
        ScopedHandle<CameraComponent> cameraComponent = cameraManager->Write(cameraEntity);
        cameraComponent->sceneFlags |= CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT;
        cameraComponent->pipelineFlags |= CameraComponent::PipelineFlagBits::MSAA_BIT |
                                          CameraComponent::PipelineFlagBits::ALLOW_COLOR_PRE_PASS_BIT |
                                          CameraComponent::PipelineFlagBits::DEPTH_OUTPUT_BIT |
                                          CameraComponent::PipelineFlagBits::VELOCITY_OUTPUT_BIT;
        cameraComponent->renderingPipeline = CameraComponent::RenderingPipeline::FORWARD;
        cameraComponent->clearColorValue = { 1.0f, 0.0f, 0.0f, 1.0f };
        cameraComponent->postProcess = postprocessEntity;
    }
    sceneUtil.UpdateCameraViewport(
        res.GetEcs(), cameraEntity, { res.GetWindowWidth(), res.GetWindowHeight() }, true, Math::DEG2RAD * 60.0f, 1.0f);

    // objects
    {
        auto& meshUtil = res.GetGraphicsContext().GetMeshUtil();
        {
            auto material = ecs.GetEntityManager().Create();
            materialManager->Create(material);
            if (auto handle = materialManager->Write(material)) {
                handle->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor =
                    BASE_NS::Math::Vec4(0.9f, 0.7f, 0.3f, 1.f);
                handle->textures[MaterialComponent::TextureIndex::MATERIAL].factor =
                    BASE_NS::Math::Vec4(0.0f, 0.0f, 0.9f, 1.f);
            }
            auto plane = nodeSystem->GetNode(meshUtil.GeneratePlane(ecs, "plane", material, 5.0f, 5.0f));
            plane->SetPosition({ 0.0f, -1.0f, 0.0f });
            plane->SetParent(*rootNode);
        }
        {
            auto material = ecs.GetEntityManager().Create();
            materialManager->Create(material);
            if (auto handle = materialManager->Write(material)) {
                handle->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor =
                    BASE_NS::Math::Vec4(0.3f, 0.9f, 0.1f, 1.f);
                handle->textures[MaterialComponent::TextureIndex::EMISSIVE].factor =
                    BASE_NS::Math::Vec4(0.3f, 0.9f, 0.1f, 2.f);

                handle->textures[MaterialComponent::TextureIndex::MATERIAL].factor =
                    BASE_NS::Math::Vec4(0.0f, 0.0f, 0.9f, 1.f);
            }
            auto sphere = nodeSystem->GetNode(meshUtil.GenerateSphere(ecs, "sphere", material, 1.f, 32.f, 32.f));
            sphere->SetPosition({ 1.0f, 0.0f, 0.0f });
            sphere->SetParent(*rootNode);
        }
        {
            auto material = ecs.GetEntityManager().Create();
            materialManager->Create(material);
            if (auto handle = materialManager->Write(material)) {
                handle->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor =
                    BASE_NS::Math::Vec4(0.7f, 0.3f, 0.9f, 1.f);
                handle->textures[MaterialComponent::TextureIndex::MATERIAL].factor =
                    BASE_NS::Math::Vec4(0.0f, 0.0f, 0.9f, 1.f);
            }
            auto cube = nodeSystem->GetNode(meshUtil.GenerateCube(ecs, "cube", material, 1.f, 1.f, 1.f));
            cube->SetPosition({ -1.0f, -0.5f, 0.0f });
            cube->SetParent(*rootNode);
        }
    }
}
} // namespace

/**
 * @tc.name: PostprocessEffectSsaoVulkan
 * @tc.desc: Tests SSAO with simple primitives.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, PostprocessEffectSsaoVulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(800u, 500u, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    PostprocessEffectSsao(res);
    res.TickTestAutoRng(7);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/PostprocessEffectSsaoVulkan.png").c_str(), res.GetWindowWidth(),
            res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    res.ShutdownTest();
}

/**
 * @tc.name: PostprocessEffectSsaoOpenGL
 * @tc.desc: Tests SSAO with simple primitives.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, PostprocessEffectSsaoOpenGL, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(800u, 500u, UTest::GetOpenGLBackend());
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    PostprocessEffectSsao(res);
    res.TickTestAutoRng(7);

    if (res.GetByteArray()) {
        const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
        UTest::WritePng(BASE_NS::string(appDir + "/PostprocessEffectSsaoOpenGL.png").c_str(), res.GetWindowWidth(),
            res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
    }

    res.ShutdownTest();
}
