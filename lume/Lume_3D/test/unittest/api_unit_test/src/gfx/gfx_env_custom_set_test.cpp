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

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/util/intf_scene_util.h>
#include <base/math/quaternion_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>

#include "gfx_common.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace {
EntityReference CreateGpuImageEntity(IEcs& ecs, IGpuResourceManager& gpuResMgr, string_view name,
    const ImageViewType viewType, const uint32_t layerCount)
{
    const Math::Vec4 texels[] = {
        {1.0f, 0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, 0.0f, 1.0f, 1.0f},
    };
    GpuImageDesc desc{};
    desc.width = 1U;
    desc.height = 1U;
    desc.depth = 1U;
    desc.layerCount = layerCount;
    desc.format = Format::BASE_FORMAT_R32G32B32A32_SFLOAT;
    desc.imageType = ImageType::CORE_IMAGE_TYPE_2D;
    desc.imageViewType = viewType;
    desc.imageTiling = ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
    desc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.usageFlags =
        ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (viewType == ImageViewType::CORE_IMAGE_VIEW_TYPE_CUBE) {
        desc.createFlags = ImageCreateFlagBits::CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }
    const array_view<const uint8_t> raw(reinterpret_cast<const uint8_t*>(texels), sizeof(Math::Vec4) * layerCount);
    auto rhr = gpuResMgr.Create(name, desc, raw);

    auto handleManager = GetManager<IRenderHandleComponentManager>(ecs);
    EntityReference entity = ecs.GetEntityManager().CreateReferenceCounted();
    handleManager->Create(entity);
    handleManager->Write(entity)->reference = move(rhr);
    return entity;
}

EntityReference CreateSamplerEntity(IEcs& ecs, IGpuResourceManager& gpuResMgr)
{
    auto handleManager = GetManager<IRenderHandleComponentManager>(ecs);
    const auto samplerRhr = gpuResMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    return GetOrCreateEntityReference(ecs.GetEntityManager(), *handleManager, samplerRhr);
}

void EnvCustomSetTest(UTest::TestResources& res)
{
    auto& ecs = res.GetEcs();
    auto& renderContext = res.GetRenderContext();
    auto& gpuResMgr = renderContext.GetDevice().GetGpuResourceManager();
    auto& shaderMgr = renderContext.GetDevice().GetShaderManager();

    const EntityReference cubeImage =
        CreateGpuImageEntity(ecs, gpuResMgr, "TestEnvCubeImage", ImageViewType::CORE_IMAGE_VIEW_TYPE_CUBE, 6U);
    const EntityReference image2D =
        CreateGpuImageEntity(ecs, gpuResMgr, "TestEnvImage2D", ImageViewType::CORE_IMAGE_VIEW_TYPE_2D, 1U);
    const EntityReference sampler = CreateSamplerEntity(ecs, gpuResMgr);

    shaderMgr.LoadShaderFile("test://shader/test_env.shader");
    const RenderHandleReference shaderRef = shaderMgr.GetShaderHandle("test://shader/test_env.shader");
    ASSERT_TRUE(shaderRef);

    auto handleManager = GetManager<IRenderHandleComponentManager>(ecs);
    const EntityReference shaderEntity = GetOrCreateEntityReference(ecs.GetEntityManager(), *handleManager, shaderRef);

    auto* nodeSystem = GetSystem<INodeSystem>(ecs);
    const Entity sceneRoot = nodeSystem->CreateNode()->GetEntity();

    auto* envManager = GetManager<IEnvironmentComponentManager>(ecs);
    Entity envEntity = ecs.GetEntityManager().Create();
    envManager->Create(envEntity);
    if (auto envHandle = envManager->Write(envEntity); envHandle) {
        envHandle->background = EnvironmentComponent::Background::CUBEMAP;
        envHandle->envMap = cubeImage;
        envHandle->shader = shaderEntity;
        envHandle->customResources.push_back(cubeImage);
        envHandle->customResources.push_back(image2D);
        envHandle->customResources.push_back(sampler);
    }

    auto* rccm = GetManager<IRenderConfigurationComponentManager>(ecs);
    rccm->Create(sceneRoot);
    if (auto cfg = rccm->Write(sceneRoot); cfg) {
        cfg->renderingFlags |= RenderConfigurationComponent::CREATE_RNGS_BIT;
        cfg->environment = envEntity;
    }

    auto& sceneUtil = res.GetGraphicsContext().GetSceneUtil();
    const Entity cameraEntity = sceneUtil.CreateCamera(
        ecs, Math::Vec3(0.0f, 0.0f, 3.0f), Math::AngleAxis(0.0f, Math::Vec3(1.0f, 0.0f, 0.0f)), 0.1f, 100.0f, 60.0f);
    if (auto cam = GetManager<ICameraComponentManager>(ecs)->Write(cameraEntity); cam) {
        cam->sceneFlags |= CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT;
        cam->renderingPipeline = CameraComponent::RenderingPipeline::FORWARD;
        cam->environment = envEntity;
        cam->clearColorValue = {0.0f, 0.0f, 0.0f, 1.0f};
    }
    sceneUtil.UpdateCameraViewport(
        ecs, cameraEntity, {res.GetWindowWidth(), res.GetWindowHeight()}, true, Math::DEG2RAD * 90.0f, 1.0f);
}
}  // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: EnvCustomSetVulkan
 * @tc.desc: Drives RenderNodeDefaultEnv::UpdateAndBindCustomSet by using a custom env shader
 *           whose pipeline layout declares set 3 (sampled_image x2 + sampler), with the matching
 *           EnvironmentComponent::customResources populated.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxTest, EnvCustomSetVulkan, testing::ext::TestSize.Level1)
{
    UTest::TestResources res(4U, 4U, DeviceBackendType::VULKAN);
    res.LiftTestUp(static_cast<int32_t>(res.GetWindowWidth()), static_cast<int32_t>(res.GetWindowHeight()));
    EnvCustomSetTest(res);
    res.TickTest(2);
    res.ShutdownTest();
}
#endif  // RENDER_HAS_VULKAN_BACKEND
