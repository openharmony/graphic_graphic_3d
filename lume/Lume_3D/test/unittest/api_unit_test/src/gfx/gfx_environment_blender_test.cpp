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
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_scene_util.h>
#include <base/math/quaternion_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <render/device/intf_gpu_resource_manager.h>

#include "gfx_common.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace {

struct Cubemaps {
    RenderHandleReference red;
    RenderHandleReference green;
};

Cubemaps CreateEnvironmentCubemaps(IGpuResourceManager& gpuResManager)
{
    // 6 faces
    const Math::Vec4 redData[] = {
        Math::Vec4(1.f, 0.f, 0.f, 1.f),
        Math::Vec4(0.f, 1.f, 0.f, 1.f),
        Math::Vec4(0.f, 0.f, 1.f, 1.f),
        Math::Vec4(0.f, 1.f, 1.f, 1.f),
        Math::Vec4(1.f, 0.f, 1.f, 1.f),
        Math::Vec4(1.f, 1.f, 0.f, 1.f),
    };
    const Math::Vec4 greenData[] = {
        Math::Vec4(0.f, 1.f, 1.f, 1.f),
        Math::Vec4(1.f, 0.f, 1.f, 1.f),
        Math::Vec4(1.f, 1.f, 0.f, 1.f),
        Math::Vec4(1.f, 0.f, 0.f, 1.f),
        Math::Vec4(0.f, 1.f, 0.f, 1.f),
        Math::Vec4(0.f, 0.f, 1.f, 1.f),
    };

    GpuImageDesc cubeDesc = {};
    cubeDesc.format = Format::BASE_FORMAT_R32G32B32A32_SFLOAT;
    cubeDesc.height = 1;
    cubeDesc.width = 1;
    cubeDesc.depth = 1;
    cubeDesc.layerCount = 6;
    cubeDesc.createFlags = ImageCreateFlagBits::CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    cubeDesc.imageType = ImageType::CORE_IMAGE_TYPE_2D;
    cubeDesc.imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_CUBE;
    cubeDesc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    cubeDesc.usageFlags =
        ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_DST_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;

    const array_view<const uint8_t> redRawData = arrayviewU8(redData);
    const array_view<const uint8_t> greenRawData = arrayviewU8(greenData);

    auto redRef = gpuResManager.Create("Red Cubemap", cubeDesc, redRawData);
    auto greenRef = gpuResManager.Create("Green Cubemap", cubeDesc, greenRawData);
    return { redRef, greenRef };
}

EntityReference EnvironmentBlenderTest(UTest::TestResources& res)
{
    auto& gpuResManager = res.GetRenderContext().GetDevice().GetGpuResourceManager();
    Cubemaps maps = CreateEnvironmentCubemaps(gpuResManager);

    auto& ecs = res.GetEcs();
    auto& engine = res.GetEngine();

    IEntityManager& em = ecs.GetEntityManager();

    auto nodeSystem = GetSystem<INodeSystem>(ecs);
    ISceneNode* scene = nodeSystem->CreateNode();
    const Entity sceneRoot = scene->GetEntity();

    auto* envManager = GetManager<IEnvironmentComponentManager>(ecs);
    auto* blendManager = GetManager<IDynamicEnvironmentBlenderComponentManager>(ecs);

    auto& sceneUtil = res.GetGraphicsContext().GetSceneUtil();

    EntityReference blendEntity = em.CreateReferenceCounted();
    EntityReference blendEnvEntity = em.CreateReferenceCounted();
    Entity envEntity1 = em.Create();
    Entity envEntity2 = em.Create();
    // render configuration component
    {
        IRenderConfigurationComponentManager* rccm = GetManager<IRenderConfigurationComponentManager>(res.GetEcs());
        rccm->Create(sceneRoot);
        ScopedHandle<RenderConfigurationComponent> renderConfiguration = rccm->Write(sceneRoot);
        renderConfiguration->renderingFlags |= RenderConfigurationComponent::CREATE_RNGS_BIT;

        {
            envManager->Create(envEntity1);
            if (auto envDataHandle = envManager->Write(envEntity1); envDataHandle) {
                EnvironmentComponent& envComponent = *envDataHandle;
                // Update env map type
                envComponent.background = EnvironmentComponent::Background::CUBEMAP;
                envComponent.envMap = GetOrCreateEntityReference(ecs, maps.red);
            }
        }
        {
            envManager->Create(envEntity2);
            if (auto envDataHandle = envManager->Write(envEntity2); envDataHandle) {
                EnvironmentComponent& envComponent = *envDataHandle;
                // Update env map type
                envComponent.background = EnvironmentComponent::Background::CUBEMAP;
                envComponent.envMap = GetOrCreateEntityReference(ecs, maps.green);
            }
        }
        {
            envManager->Create(blendEnvEntity);
            if (auto envDataHandle = envManager->Write(blendEnvEntity); envDataHandle) {
                EnvironmentComponent& envComponent = *envDataHandle;
                envComponent.blendEnvironments = blendEntity;
            }
            blendManager->Create(blendEntity);
            if (auto envDataHandle = blendManager->Write(blendEntity); envDataHandle) {
                DynamicEnvironmentBlenderComponent& blendComponent = *envDataHandle;
                blendComponent.environments.clear();
                blendComponent.environments.push_back(envEntity1);
                blendComponent.environments.push_back(envEntity2);
                blendComponent.entryFactor = { 0.0f, 0.0f, 0.0f, 0.0f };
                blendComponent.switchFactor = { 0.5f, 0.5f, 0.0f, 0.0f };
            }
        }
        renderConfiguration->environment = blendEnvEntity;
    }

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
        cameraComponent->environment = blendEnvEntity;
    }
    sceneUtil.UpdateCameraViewport(
        res.GetEcs(), cameraEntity, { res.GetWindowWidth(), res.GetWindowHeight() }, true, Math::DEG2RAD * 90.0f, 1.0f);
    return blendEnvEntity;
}

void Validate(array_view<uint8_t> data)
{
    for (uint32_t i = 0; i < data.size(); i += 4) {
        uint8_t r = data[i + 0];
        uint8_t g = data[i + 1];
        uint8_t b = data[i + 2];
        uint8_t a = data[i + 3];

        ASSERT_EQ(r, 186);
        ASSERT_EQ(g, 186);
        ASSERT_EQ(b, 186);
        ASSERT_EQ(a, 255);
    }
}
} // namespace

/**
 * @tc.name: EnvironmentBlenderTestVulkan
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, EnvironmentBlenderTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(4, 4, DeviceBackendType::VULKAN);
    {
        res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
        auto keepAliveRef = EnvironmentBlenderTest(res);
        res.TickTest(10);
        if (res.GetByteArray()) {
            Validate(res.GetByteArray()->GetData());

#if RENDER_SAVE_TEST_IMAGES == 1
            const BASE_NS::string appDir = res.GetEngine().GetFileManager().GetEntry("app://").name;
            UTest::WritePng(BASE_NS::string(appDir + "/EnvironmentBlenderTestVulkan.png").c_str(), res.GetWindowWidth(),
                res.GetWindowHeight(), 4, res.GetByteArray()->GetData().data(), res.GetWindowWidth() * 4);
#endif
        }
    }

    res.ShutdownTest();
}
