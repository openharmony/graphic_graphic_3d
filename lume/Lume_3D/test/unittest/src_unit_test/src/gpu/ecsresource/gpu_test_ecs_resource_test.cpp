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

#include <algorithm>

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_input_component.h>
#include <3d/ecs/components/animation_output_component.h>
#include <3d/ecs/components/animation_state_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/gltf/gltf.h>
#include <3d/util/intf_scene_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/image/intf_image_container.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>

#include "ecs/components/initial_transform_component.h"
#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
#include "util/mesh_util.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

template<typename ComponentManager>
Entity LookupResourceByUri(
    string_view uri, const IUriComponentManager& uriManager, const ComponentManager& componentManager)
{
    const auto& entityManager = uriManager.GetEcs().GetEntityManager();
    const auto uriComponents = uriManager.GetComponentCount();
    for (auto i = 0u; i < uriComponents; ++i) {
        Entity entity = uriManager.GetEntity(i);
        if (entityManager.IsAlive(uriManager.GetEntity(i))) {
            if (const auto uriHandle = uriManager.Read(i); uriHandle) {
                const bool found = uriHandle->uri == uri;
                if (found && componentManager.HasComponent(entity)) {
                    return entity;
                }
            }
        }
    }
    return {};
}

template<typename Container, typename Pred>
bool any_of(const Container& c, Pred&& p)
{
    return std::any_of(std::begin(c), std::end(c), std::forward<Pred>(p));
}

/**
 * @tc.name: testMeshResource
 * @tc.desc: Tests for Test Mesh Resource. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_ResourceTest, testMeshResource, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto components = GetPluginRegister().GetTypeInfos(ComponentManagerTypeInfo::UID);
    constexpr Uid requiredManagers[] = { IRenderHandleComponentManager::UID, IMeshComponentManager::UID,
        INameComponentManager::UID, IUriComponentManager::UID };
    for (const auto component : components) {
        const auto info = static_cast<const ComponentManagerTypeInfo*>(component);
        if (any_of(requiredManagers, [currentUid = info->uid](const auto& uid) { return uid == currentUid; })) {
            ecs->CreateComponentManager(*info);
        }
    }

    // Create mesh.
    MeshUtil meshUtil(*engine->GetInterface<IClassFactory>());
    const Entity meshEntity = meshUtil.GenerateCubeMesh(*ecs, "cube", {}, 1.0f, 1.0f, 1.0f);

    // Get mesh from resource manager.
    auto uriManager = GetManager<IUriComponentManager>(*ecs);
    auto meshManager = GetManager<IMeshComponentManager>(*ecs);

    auto meshByUri = LookupResourceByUri("cube", *uriManager, *meshManager);
    ASSERT_TRUE(meshManager->HasComponent(meshByUri));
    EXPECT_EQ(meshByUri, meshEntity);

    // Destroy mesh.
    EXPECT_TRUE(ecs->GetEntityManager().IsAlive(meshEntity));
    ecs->GetEntityManager().Destroy(meshEntity);
    EXPECT_FALSE(ecs->GetEntityManager().IsAlive(meshEntity));

    // Ensure it is gone.
    ASSERT_EQ(LookupResourceByUri("cube", *uriManager, *meshManager), Entity {});
}

/**
 * @tc.name: testGpuHandleResource
 * @tc.desc: Tests for Test Gpu Handle Resource. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_ResourceTest, testGpuHandleResource, testing::ext::TestSize.Level1)
{
    constexpr string_view imageUri = "test://image/canine_512x512.png";

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto components = GetPluginRegister().GetTypeInfos(ComponentManagerTypeInfo::UID);
    constexpr Uid requiredManagers[] = { IRenderHandleComponentManager::UID, INameComponentManager::UID,
        IUriComponentManager::UID };
    for (const auto component : components) {
        const auto info = static_cast<const ComponentManagerTypeInfo*>(component);
        if (any_of(requiredManagers, [currentUid = info->uid](const auto& uid) { return uid == currentUid; })) {
            ecs->CreateComponentManager(*info);
        }
    }
    auto systems = GetPluginRegister().GetTypeInfos(SystemTypeInfo::UID);

    auto uriManager = GetManager<IUriComponentManager>(*ecs);
    auto gpuHandleManager = GetManager<IRenderHandleComponentManager>(*ecs);

    auto* imageLoader = &(UTest::GetTestContext()->engine->GetImageLoaderManager());
    ASSERT_TRUE(imageLoader != nullptr);

    auto result = imageLoader->LoadImage(imageUri, 0);
    ASSERT_TRUE(result.success) << result.error;
    ASSERT_TRUE(result.image != nullptr);

    auto const imageContainerDesc = result.image->GetImageDesc();

    auto& gpuResourceManager = renderContext->GetDevice().GetGpuResourceManager();

    const auto& imageDesc = result.image->GetImageDesc();
    GpuImageDesc gpuDesc {
        static_cast<RENDER_NS::ImageType>(imageDesc.imageType),           // imageType
        static_cast<RENDER_NS::ImageViewType>(imageDesc.imageViewType),   //  imageViewType
        imageDesc.format,                                                 //  format
        ImageTiling::CORE_IMAGE_TILING_OPTIMAL,                           //  imageTiling
        CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT, //  usageFlags
        CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,                            //  memoryPropertyFlags
        0,                                                                //  createFlags
        0,                                                                //  engineCreationFlags
        imageDesc.width,                                                  //  width
        imageDesc.height,                                                 //  height
        imageDesc.depth,                                                  //  depth
        imageDesc.mipCount,                                               //  mipCount
        imageDesc.layerCount,                                             //  layerCount
        SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,                     //  sampleCountFlags
        {},                                                               //  componentMapping
    };
    if (imageDesc.imageFlags & IImageContainer::ImageFlags::FLAGS_CUBEMAP_BIT) {
        gpuDesc.createFlags |= ImageCreateFlagBits::CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }
    if (imageDesc.imageFlags & IImageContainer::ImageFlags::FLAGS_REQUESTING_MIPMAPS_BIT) {
        gpuDesc.usageFlags |= CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
        gpuDesc.engineCreationFlags |= CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS;
    }

    const Entity imageEntity = ecs->GetEntityManager().Create();
    gpuHandleManager->Set(imageEntity, { gpuResourceManager.Create(gpuDesc, move(result.image)) });
    uriManager->Set(imageEntity, { string(imageUri) });

    ASSERT_EQ(LookupResourceByUri(imageUri, *uriManager, *gpuHandleManager), imageEntity);

    RenderHandle const gpuImageHandle = gpuHandleManager->Get(imageEntity).reference.GetHandle();
    ASSERT_NE(gpuImageHandle, RenderHandle {});
    {
        auto const gpuImageDesc = gpuResourceManager.GetImageDescriptor(gpuResourceManager.Get(gpuImageHandle));
        ASSERT_EQ(gpuImageDesc.height, imageContainerDesc.width);
    }

    // Destroy image.
    EXPECT_TRUE(ecs->GetEntityManager().IsAlive(imageEntity));
    ecs->GetEntityManager().Destroy(imageEntity);
    EXPECT_FALSE(ecs->GetEntityManager().IsAlive(imageEntity));

    ecs->ProcessEvents();
    ecs->Update(0, 0);

    // Ensure it is gone.
    ASSERT_EQ(LookupResourceByUri(imageUri, *uriManager, *gpuHandleManager), Entity {});
}

/**
 * @tc.name: testGpuHandleResourceWeak
 * @tc.desc: Tests for Test Gpu Handle Resource Weak. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_ResourceTest, testGpuHandleResourceWeak, testing::ext::TestSize.Level1)
{
    constexpr string_view imageUri = "test://image/canine_512x512.png";

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto components = GetPluginRegister().GetTypeInfos(ComponentManagerTypeInfo::UID);
    constexpr Uid requiredManagers[] = { IRenderHandleComponentManager::UID, INameComponentManager::UID,
        IUriComponentManager::UID };
    for (const auto component : components) {
        const auto info = static_cast<const ComponentManagerTypeInfo*>(component);
        if (any_of(requiredManagers, [currentUid = info->uid](const auto& uid) { return uid == currentUid; })) {
            ecs->CreateComponentManager(*info);
        }
    }

    auto uriManager = GetManager<IUriComponentManager>(*ecs);
    auto gpuHandleManager = GetManager<IRenderHandleComponentManager>(*ecs);

    auto* imageLoader = &(UTest::GetTestContext()->engine->GetImageLoaderManager());
    ASSERT_TRUE(imageLoader != nullptr);

    auto result = imageLoader->LoadImage(imageUri, 0);
    ASSERT_TRUE(result.success) << result.error;
    ASSERT_TRUE(result.image != nullptr);

    auto const imageContainerDesc = result.image->GetImageDesc();

    auto& gpuResourceManager = renderContext->GetDevice().GetGpuResourceManager();

    const auto& imageDesc = result.image->GetImageDesc();
    GpuImageDesc gpuDesc {
        static_cast<RENDER_NS::ImageType>(imageDesc.imageType),           // imageType
        static_cast<RENDER_NS::ImageViewType>(imageDesc.imageViewType),   //  imageViewType
        imageDesc.format,                                                 //  format
        ImageTiling::CORE_IMAGE_TILING_OPTIMAL,                           //  imageTiling
        CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT, //  usageFlags
        CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,                            //  memoryPropertyFlags
        0,                                                                //  createFlags
        0,                                                                //  engineCreationFlags
        imageDesc.width,                                                  //  width
        imageDesc.height,                                                 //  height
        imageDesc.depth,                                                  //  depth
        imageDesc.mipCount,                                               //  mipCount
        imageDesc.layerCount,                                             //  layerCount
        SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,                     //  sampleCountFlags
        {},                                                               //  componentMapping
    };
    if (imageDesc.imageFlags & IImageContainer::ImageFlags::FLAGS_CUBEMAP_BIT) {
        gpuDesc.createFlags |= ImageCreateFlagBits::CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }
    if (imageDesc.imageFlags & IImageContainer::ImageFlags::FLAGS_REQUESTING_MIPMAPS_BIT) {
        gpuDesc.usageFlags |= CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
        gpuDesc.engineCreationFlags |= CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS;
    }

    const Entity imageEntity = ecs->GetEntityManager().Create();
    gpuHandleManager->Set(imageEntity, { gpuResourceManager.Create(gpuDesc, move(result.image)) });
    uriManager->Set(imageEntity, { string(imageUri) });

    ASSERT_EQ(LookupResourceByUri(imageUri, *uriManager, *gpuHandleManager), imageEntity);

    RenderHandle const gpuImageHandle = gpuHandleManager->Get(imageEntity).reference.GetHandle();
    ASSERT_NE(gpuImageHandle, RenderHandle {});
    {
        auto const gpuImageDesc = gpuResourceManager.GetImageDescriptor(gpuResourceManager.Get(gpuImageHandle));
        ASSERT_EQ(gpuImageDesc.height, imageContainerDesc.width);
    }

    // Destroy image.
    EXPECT_TRUE(ecs->GetEntityManager().IsAlive(imageEntity));
    ecs->GetEntityManager().Destroy(imageEntity);
    EXPECT_FALSE(ecs->GetEntityManager().IsAlive(imageEntity));

    ecs->ProcessEvents();

    // Ensure it is gone.
    ASSERT_EQ(LookupResourceByUri(imageUri, *uriManager, *gpuHandleManager), Entity {});
    RenderHandleReference gpuImageHandleRef = gpuResourceManager.Get(gpuImageHandle);
    gpuImageHandleRef = {};
}

/**
 * @tc.name: testAnimationResource
 * @tc.desc: Tests for Test Animation Resource. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_ResourceTest, testAnimationResource, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    constexpr Uid requiredManagers[] = {
        ITransformComponentManager::UID,
    };
    const auto components = GetPluginRegister().GetTypeInfos(ComponentManagerTypeInfo::UID);
    for (const auto component : components) {
        const auto info = static_cast<const ComponentManagerTypeInfo*>(component);
        if (any_of(requiredManagers, [currentUid = info->uid](const auto& uid) { return uid == currentUid; })) {
            ecs->CreateComponentManager(*info);
        }
    }
    constexpr string_view animationSystemName = "AnimationSystem";
    for (const auto* typeInfo : GetPluginRegister().GetTypeInfos(SystemTypeInfo::UID)) {
        if (const auto* systemTypeInfo = static_cast<const SystemTypeInfo*>(typeInfo);
            systemTypeInfo && (systemTypeInfo->typeName == animationSystemName)) {
            for (const auto component : components) {
                const auto info = static_cast<const ComponentManagerTypeInfo*>(component);
                if (any_of(systemTypeInfo->componentDependencies,
                        [currentUid = info->uid](const auto& uid) { return uid == currentUid; })) {
                    ecs->CreateComponentManager(*info);
                } else if (any_of(systemTypeInfo->readOnlyComponentDependencies,
                               [currentUid = info->uid](const auto& uid) { return uid == currentUid; })) {
                    ecs->CreateComponentManager(*info);
                }
            }
            ecs->CreateSystem(*systemTypeInfo);
            break;
        }
    }
    ecs->Initialize();
    auto& em = ecs->GetEntityManager();
    auto* animationInputManager = GetManager<IAnimationInputComponentManager>(*ecs);
    auto* animationOutputManager = GetManager<IAnimationOutputComponentManager>(*ecs);
    auto* animationTrackManager = GetManager<IAnimationTrackComponentManager>(*ecs);
    auto* animationManager = GetManager<IAnimationComponentManager>(*ecs);
    auto* transformManager = GetManager<ITransformComponentManager>(*ecs);

    // empty animation
    {
        const auto animation = em.CreateReferenceCounted();
        {
            animationManager->Create(animation);
            const auto animationHandle = animationManager->Write(animation);
            animationHandle->state = AnimationComponent::PlaybackState::PLAY;
        }
        auto* ecsPtr = ecs.get();
        engine->TickFrame(array_view(&ecsPtr, 1));
    }

    // invalid tracks
    {
        const auto target = em.CreateReferenceCounted();
        transformManager->Create(target);

        const auto timeStamps = em.CreateReferenceCounted();
        {
            animationInputManager->Create(timeStamps);
            auto timeLineHandle = animationInputManager->Write(timeStamps);
            constexpr float timeStampValues[] = { 0.f, 1.f, 2.f };
            timeLineHandle->timestamps.insert(
                timeLineHandle->timestamps.begin(), std::begin(timeStampValues), std::end(timeStampValues));
        }
        const auto positions = em.CreateReferenceCounted();
        {
            animationOutputManager->Create(positions);
            auto positionsHandle = animationOutputManager->Write(positions);
            positionsHandle->type = PropertyType::VEC3_T;
            constexpr Math::Vec3 positionValues[] = {
                { -2.f, 0.f, 0.f },
                { 0.f, 0.f, 0.f },
                { 2.f, 0.f, 0.f },

                { 0.f, 1.f, 0.f },
                { 0.f, 1.f, 0.f },
                { 2.f, 0.f, 0.f },

                { 2.f, 0.f, 0.f },
                { 0.f, 0.f, 0.f },
                { 2.f, 0.f, 0.f },
            };
            auto& data = positionsHandle->data;
            data.resize(sizeof(positionValues));
            CloneData(data.data(), data.size(), positionValues, sizeof(positionValues));
        }

        const auto emptyTrack = em.CreateReferenceCounted();
        {
            animationTrackManager->Create(emptyTrack);
        }
        const auto invalidComponentTrack = em.CreateReferenceCounted();
        {
            animationTrackManager->Create(invalidComponentTrack);
            auto trackHandle = animationTrackManager->Write(invalidComponentTrack);
            trackHandle->target = target;
            trackHandle->component = Uid { "12345678-1234-1234-1234-123456789012" };
        }
        const auto missingComponentTrack = em.CreateReferenceCounted();
        {
            animationTrackManager->Create(missingComponentTrack);
            auto trackHandle = animationTrackManager->Write(missingComponentTrack);
            trackHandle->target = target;
            trackHandle->component = IAnimationComponentManager::UID;
        }
        const auto invalidPropertyTrack = em.CreateReferenceCounted();
        {
            animationTrackManager->Create(invalidPropertyTrack);
            auto trackHandle = animationTrackManager->Write(invalidPropertyTrack);
            trackHandle->target = target;
            trackHandle->component = ITransformComponentManager::UID;
            trackHandle->property = "p";
        }
        const auto emptyInputTrack = em.CreateReferenceCounted();
        {
            animationTrackManager->Create(emptyInputTrack);
            auto trackHandle = animationTrackManager->Write(emptyInputTrack);
            trackHandle->target = target;
            trackHandle->component = ITransformComponentManager::UID;
            trackHandle->property = "position";
            trackHandle->timestamps = em.CreateReferenceCounted();
            animationInputManager->Create(trackHandle->timestamps);
        }
        const auto emptyOutputTrack = em.CreateReferenceCounted();
        {
            animationTrackManager->Create(emptyOutputTrack);
            auto trackHandle = animationTrackManager->Write(emptyOutputTrack);
            trackHandle->target = target;
            trackHandle->component = ITransformComponentManager::UID;
            trackHandle->property = "position";
            trackHandle->timestamps = timeStamps;
            trackHandle->data = em.CreateReferenceCounted();
            animationOutputManager->Create(trackHandle->data);
        }
        const auto animation = em.CreateReferenceCounted();
        {
            animationManager->Create(animation);
            const auto animationHandle = animationManager->Write(animation);
            const EntityReference tracks[] = { emptyTrack, invalidComponentTrack, missingComponentTrack,
                invalidPropertyTrack, emptyInputTrack, emptyOutputTrack };
            animationHandle->tracks.reserve(countof(tracks));
            for (const auto& track : tracks) {
                animationHandle->tracks.emplace_back(track);
            }
            animationHandle->state = AnimationComponent::PlaybackState::PLAY;
        }
        auto* ecsPtr = ecs.get();
        engine->TickFrame(array_view(&ecsPtr, 1));
    }

    // everything filled
    {
        const auto target = em.CreateReferenceCounted();
        transformManager->Create(target);

        const auto timeStamps = em.CreateReferenceCounted();
        {
            animationInputManager->Create(timeStamps);
            auto timeLineHandle = animationInputManager->Write(timeStamps);
            constexpr float timeStampValues[] = { 0.f, 1.f, 2.f };
            timeLineHandle->timestamps.insert(
                timeLineHandle->timestamps.begin(), std::begin(timeStampValues), std::end(timeStampValues));
        }
        const auto positions = em.CreateReferenceCounted();
        {
            animationOutputManager->Create(positions);
            auto positionsHandle = animationOutputManager->Write(positions);
            positionsHandle->type = PropertyType::VEC3_T;
            constexpr Math::Vec3 positionValues[] = {
                { -2.f, 0.f, 0.f },
                { 0.f, 0.f, 0.f },
                { 2.f, 0.f, 0.f },

                { 0.f, 1.f, 0.f },
                { 0.f, 1.f, 0.f },
                { 2.f, 0.f, 0.f },

                { 2.f, 0.f, 0.f },
                { 0.f, 0.f, 0.f },
                { 2.f, 0.f, 0.f },
            };
            auto& data = positionsHandle->data;
            data.resize(sizeof(positionValues));
            CloneData(data.data(), data.size(), positionValues, sizeof(positionValues));
        }
        const auto positionTrack = em.CreateReferenceCounted();
        {
            animationTrackManager->Create(positionTrack);
            auto positionTrackHandle = animationTrackManager->Write(positionTrack);
            positionTrackHandle->target = target;
            positionTrackHandle->component = ITransformComponentManager::UID;
            positionTrackHandle->property = "position";
            positionTrackHandle->interpolationMode = AnimationTrackComponent::Interpolation::SPLINE;
            positionTrackHandle->timestamps = timeStamps;
            positionTrackHandle->data = positions;
        }
        const auto animation = em.CreateReferenceCounted();
        {
            animationManager->Create(animation);
            const auto animationHandle = animationManager->Write(animation);
            const EntityReference tracks[] = { positionTrack };
            animationHandle->tracks.reserve(countof(tracks));
            for (const auto& track : tracks) {
                animationHandle->tracks.emplace_back(track);
            }
            animationHandle->state = AnimationComponent::PlaybackState::PLAY;
        }

        const auto transform0 = *transformManager->Read(target);

        auto* ecsPtr = ecs.get();
        engine->TickFrame(array_view(&ecsPtr, 1));

        const auto transform1 = *transformManager->Read(target);
        EXPECT_GT(Math::Magnitude(transform1.position - transform0.position), 0.f);
        EXPECT_EQ(transform0.rotation, transform1.rotation);
        EXPECT_EQ(transform0.scale, transform1.scale);

        {
            auto stateManager = GetManager<IAnimationStateComponentManager>(*ecs);
            ASSERT_TRUE(stateManager && stateManager->HasComponent(animation));
            const auto stateHandle = stateManager->Write(animation);
            stateHandle->time = 1.f;
        }

        engine->TickFrame(array_view(&ecsPtr, 1));

        const auto transform2 = *transformManager->Read(target);

        EXPECT_GT(Math::Magnitude(transform2.position - transform0.position), 0.f);
        EXPECT_GT(Math::Magnitude(transform2.position - transform1.position), 0.f);
        EXPECT_EQ(transform1.rotation, transform2.rotation);
        EXPECT_EQ(transform1.scale, transform2.scale);
    }
}
