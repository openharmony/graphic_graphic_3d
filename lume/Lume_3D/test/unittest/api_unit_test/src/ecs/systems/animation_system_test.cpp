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
#include <3d/ecs/components/fog_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_handle_util.h>
#include <core/property/property_types.h>
#include <core/property_tools/core_metadata.inl>
#include <core/property_tools/property_data.h>
#include <render/device/intf_shader_manager.h>

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

/**
 * @tc.name: CreateDestroyAnimationPlaybackTest
 * @tc.desc: Tests for Create Destroy Animation Playback Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationSystem, CreateDestroyAnimationPlaybackTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);
    auto animationSystem = GetSystem<IAnimationSystem>(*ecs);
    ASSERT_NE(nullptr, animationSystem);
    animationSystem->SetActive(true);
    EXPECT_TRUE(animationSystem->IsActive());
    EXPECT_EQ("AnimationSystem", animationSystem->GetName());
    EXPECT_EQ(IAnimationSystem::UID, ((ISystem*)animationSystem)->GetUid());
    EXPECT_NE(nullptr, animationSystem->GetProperties());
    EXPECT_NE(nullptr, ((const IAnimationSystem*)animationSystem)->GetProperties());
    EXPECT_EQ(ecs.get(), &animationSystem->GetECS());

    EXPECT_EQ(0, animationSystem->GetPlaybackCount());
    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);
    IAnimationPlayback* playback = animationSystem->CreatePlayback(entity, {});
    ASSERT_NE(nullptr, playback);
    EXPECT_EQ(1, animationSystem->GetPlaybackCount());
    EXPECT_EQ(playback, animationSystem->GetPlayback(0));
    animationSystem->DestroyPlayback(playback);

    ASSERT_EQ(nullptr, animationSystem->CreatePlayback({}, {}));
}

/**
 * @tc.name: AnimationTest
 * @tc.desc: Tests for Animation Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationSystem, AnimationTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationTrackManager);
    auto animationInputManager = GetManager<IAnimationInputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationInputManager);
    auto animationSystem = GetSystem<IAnimationSystem>(*ecs);
    ASSERT_NE(nullptr, animationSystem);
    animationSystem->SetActive(true);

    EntityReference timeStamps = ecs->GetEntityManager().CreateReferenceCounted();
    animationInputManager->Create(timeStamps);
    if (auto scopedHandle = animationInputManager->Write(timeStamps); scopedHandle) {
        scopedHandle->timestamps.push_back(0.0f);
        scopedHandle->timestamps.push_back(60.0f);
    }

    Entity track = ecs->GetEntityManager().Create();
    animationTrackManager->Create(track);
    if (auto scopedHandle = animationTrackManager->Write(track); scopedHandle) {
        scopedHandle->interpolationMode = AnimationTrackComponent::Interpolation::LINEAR;
        scopedHandle->timestamps = timeStamps;
    }

    Entity animation = ecs->GetEntityManager().Create();
    animationManager->Create(animation);
    if (auto scopedHandle = animationManager->Write(animation); scopedHandle) {
        scopedHandle->duration = 59.f;
        scopedHandle->repeatCount = 1;
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle->tracks.push_back(ecs->GetEntityManager().GetReferenceCounted(track));
    }

    Entity targetEntity = ecs->GetEntityManager().Create();
    IAnimationPlayback* playback = animationSystem->CreatePlayback(animation, { &targetEntity, 1 });
    ASSERT_NE(nullptr, playback);
    EXPECT_EQ(59.0f, playback->GetDuration());
    EXPECT_EQ(60.0f, playback->GetAnimationLength());
    ecs->ProcessEvents();

    uint32_t deltaTime = 70000000u;
    ecs->Update(deltaTime, deltaTime);
    ecs->Update(deltaTime * 2, deltaTime);
    EXPECT_EQ(AnimationComponent::PlaybackState::PAUSE, playback->GetPlaybackState());
    playback->SetPlaybackState(AnimationComponent::PlaybackState::STOP);
    ecs->ProcessEvents();
    ecs->Update(deltaTime * 3, deltaTime);
    playback->SetPlaybackState(AnimationComponent::PlaybackState::PLAY);
    playback->SetRepeatCount(AnimationComponent::REPEAT_COUNT_INFINITE);
    ecs->ProcessEvents();
    ecs->Update(deltaTime * 4, deltaTime);
    ecs->Update(deltaTime * 4, deltaTime);
    EXPECT_EQ(AnimationComponent::PlaybackState::PLAY, playback->GetPlaybackState());
}

/**
 * @tc.name: AnimateFloatTest
 * @tc.desc: Tests for Animate Float Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationSystem, AnimateFloatTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationTrackManager);
    auto animationInputManager = GetManager<IAnimationInputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationInputManager);
    auto animationOutputManager = GetManager<IAnimationOutputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationOutputManager);
    auto animationSystem = GetSystem<IAnimationSystem>(*ecs);
    ASSERT_NE(nullptr, animationSystem);
    animationSystem->SetActive(true);

    EntityReference keyFrameData = ecs->GetEntityManager().CreateReferenceCounted();
    animationOutputManager->Create(keyFrameData);
    if (auto scopedHandle = animationOutputManager->Write(keyFrameData); scopedHandle) {
        scopedHandle->type = PropertyType::FLOAT_T;
        constexpr float values[] = { 0.0f, 5.0f };
        scopedHandle->data.resize(sizeof(values));
        CloneData(scopedHandle->data.data(), scopedHandle->data.size(), values, sizeof(values));
    }

    EntityReference timeStamps = ecs->GetEntityManager().CreateReferenceCounted();
    animationInputManager->Create(timeStamps);
    if (auto scopedHandle = animationInputManager->Write(timeStamps); scopedHandle) {
        scopedHandle->timestamps.push_back(0.0f);
        scopedHandle->timestamps.push_back(2.0f);
    }

    Entity targetEntity = ecs->GetEntityManager().Create();
    auto fogManager = GetManager<IFogComponentManager>(*ecs);
    ASSERT_NE(nullptr, fogManager);
    fogManager->Create(targetEntity);

    EntityReference track = ecs->GetEntityManager().CreateReferenceCounted();
    animationTrackManager->Create(track);
    if (auto scopedHandle = animationTrackManager->Write(track); scopedHandle) {
        scopedHandle->interpolationMode = AnimationTrackComponent::Interpolation::LINEAR;
        scopedHandle->timestamps = timeStamps;
        scopedHandle->data = keyFrameData;
        scopedHandle->component = IFogComponentManager::UID;
        scopedHandle->property = "startDistance";
        scopedHandle->target = ecs->GetEntityManager().GetReferenceCounted(targetEntity);
    }

    Entity animation = ecs->GetEntityManager().Create();
    animationManager->Create(animation);
    if (auto scopedHandle = animationManager->Write(animation); scopedHandle) {
        scopedHandle->duration = 1.2f;
        scopedHandle->repeatCount = 1;
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle->tracks.push_back(track);
    }

    IAnimationPlayback* playback = animationSystem->CreatePlayback(animation, { &targetEntity, 1 });
    ASSERT_NE(nullptr, playback);
    ecs->ProcessEvents();

    constexpr uint32_t sToMs = 1000000u;
    uint32_t deltaTime = 1u * sToMs;
    ecs->Update(deltaTime, deltaTime);

    if (auto scopedHandle = fogManager->Read(targetEntity); scopedHandle) {
        EXPECT_EQ(2.5f, scopedHandle->startDistance);
    }
}

/**
 * @tc.name: AnimateVec2Test
 * @tc.desc: Tests for Animate Vec2Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationSystem, AnimateVec2Test, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationTrackManager);
    auto animationInputManager = GetManager<IAnimationInputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationInputManager);
    auto animationOutputManager = GetManager<IAnimationOutputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationOutputManager);
    auto animationSystem = GetSystem<IAnimationSystem>(*ecs);
    ASSERT_NE(nullptr, animationSystem);
    animationSystem->SetActive(true);

    EntityReference keyFrameData = ecs->GetEntityManager().CreateReferenceCounted();
    animationOutputManager->Create(keyFrameData);
    if (auto scopedHandle = animationOutputManager->Write(keyFrameData); scopedHandle) {
        scopedHandle->type = PropertyType::VEC2_T;
        constexpr Math::Vec2 values[] = { { 0.0f, 0.0f }, { 5.0f, 5.0f } };
        scopedHandle->data.resize(sizeof(values));
        CloneData(scopedHandle->data.data(), scopedHandle->data.size(), values, sizeof(values));
    }

    EntityReference timeStamps = ecs->GetEntityManager().CreateReferenceCounted();
    animationInputManager->Create(timeStamps);
    if (auto scopedHandle = animationInputManager->Write(timeStamps); scopedHandle) {
        scopedHandle->timestamps.push_back(0.0f);
        scopedHandle->timestamps.push_back(2.0f);
    }

    Entity targetEntity = ecs->GetEntityManager().Create();
    auto materialManager = GetManager<IMaterialComponentManager>(*ecs);
    ASSERT_NE(nullptr, materialManager);
    materialManager->Create(targetEntity);

    EntityReference track = ecs->GetEntityManager().CreateReferenceCounted();
    animationTrackManager->Create(track);
    if (auto scopedHandle = animationTrackManager->Write(track); scopedHandle) {
        scopedHandle->interpolationMode = AnimationTrackComponent::Interpolation::STEP;
        scopedHandle->timestamps = timeStamps;
        scopedHandle->data = keyFrameData;
        scopedHandle->component = IMaterialComponentManager::UID;
        scopedHandle->property = "textures[0].transform.scale";
        scopedHandle->target = ecs->GetEntityManager().GetReferenceCounted(targetEntity);
    }

    Entity animation = ecs->GetEntityManager().Create();
    animationManager->Create(animation);
    if (auto scopedHandle = animationManager->Write(animation); scopedHandle) {
        scopedHandle->duration = 1.2f;
        scopedHandle->repeatCount = 1;
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle->tracks.push_back(track);
    }

    IAnimationPlayback* playback = animationSystem->CreatePlayback(animation, { &targetEntity, 1 });
    ASSERT_NE(nullptr, playback);
    ecs->ProcessEvents();

    constexpr uint32_t sToMs = 1000000u;
    uint32_t deltaTime = static_cast<uint32_t>(1.1f * sToMs);
    ecs->Update(deltaTime, deltaTime);

    if (auto scopedHandle = materialManager->Read(targetEntity); scopedHandle) {
        EXPECT_EQ(5u, scopedHandle->textures[0].transform.scale.x);
        EXPECT_EQ(5u, scopedHandle->textures[0].transform.scale.y);
    }
}

/**
 * @tc.name: AnimateVec3Test
 * @tc.desc: Tests for Animate Vec3Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationSystem, AnimateVec3Test, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationTrackManager);
    auto animationInputManager = GetManager<IAnimationInputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationInputManager);
    auto animationOutputManager = GetManager<IAnimationOutputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationOutputManager);
    auto animationSystem = GetSystem<IAnimationSystem>(*ecs);
    ASSERT_NE(nullptr, animationSystem);
    animationSystem->SetActive(true);

    EntityReference keyFrameData = ecs->GetEntityManager().CreateReferenceCounted();
    animationOutputManager->Create(keyFrameData);
    if (auto scopedHandle = animationOutputManager->Write(keyFrameData); scopedHandle) {
        scopedHandle->type = PropertyType::VEC3_T;
        constexpr Math::Vec3 values[] = { { 0.0f, 0.0f, 0.0f }, { 5.0f, 5.0f, 5.0f } };
        scopedHandle->data.resize(sizeof(values));
        CloneData(scopedHandle->data.data(), scopedHandle->data.size(), values, sizeof(values));
    }

    EntityReference timeStamps = ecs->GetEntityManager().CreateReferenceCounted();
    animationInputManager->Create(timeStamps);
    if (auto scopedHandle = animationInputManager->Write(timeStamps); scopedHandle) {
        scopedHandle->timestamps.push_back(0.0f);
        scopedHandle->timestamps.push_back(2.0f);
    }

    Entity targetEntity = ecs->GetEntityManager().Create();
    auto meshManager = GetManager<IMeshComponentManager>(*ecs);
    ASSERT_NE(nullptr, meshManager);
    meshManager->Create(targetEntity);

    EntityReference track = ecs->GetEntityManager().CreateReferenceCounted();
    animationTrackManager->Create(track);
    if (auto scopedHandle = animationTrackManager->Write(track); scopedHandle) {
        scopedHandle->interpolationMode = AnimationTrackComponent::Interpolation::STEP;
        scopedHandle->timestamps = timeStamps;
        scopedHandle->data = keyFrameData;
        scopedHandle->component = IMeshComponentManager::UID;
        scopedHandle->property = "aabbMin";
        scopedHandle->target = ecs->GetEntityManager().GetReferenceCounted(targetEntity);
    }

    Entity animation = ecs->GetEntityManager().Create();
    animationManager->Create(animation);
    if (auto scopedHandle = animationManager->Write(animation); scopedHandle) {
        scopedHandle->duration = 1.2f;
        scopedHandle->repeatCount = 1;
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle->tracks.push_back(track);
    }

    IAnimationPlayback* playback = animationSystem->CreatePlayback(animation, { &targetEntity, 1 });
    ASSERT_NE(nullptr, playback);
    ecs->ProcessEvents();

    constexpr uint32_t sToMs = 1000000u;
    uint32_t deltaTime = static_cast<uint32_t>(1.1f * sToMs);
    ecs->Update(deltaTime, deltaTime);

    if (auto scopedHandle = meshManager->Read(targetEntity); scopedHandle) {
        EXPECT_EQ(5.0f, scopedHandle->aabbMin.x);
        EXPECT_EQ(5.0f, scopedHandle->aabbMin.y);
        EXPECT_EQ(5.0f, scopedHandle->aabbMin.z);
    }
}

/**
 * @tc.name: AnimateBackwardsTest
 * @tc.desc: Tests for Animate Backwards Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationSystem, AnimateBackwardsTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationTrackManager);
    auto animationInputManager = GetManager<IAnimationInputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationInputManager);
    auto animationOutputManager = GetManager<IAnimationOutputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationOutputManager);
    auto animationSystem = GetSystem<IAnimationSystem>(*ecs);
    ASSERT_NE(nullptr, animationSystem);
    animationSystem->SetActive(true);

    EntityReference keyFrameData = ecs->GetEntityManager().CreateReferenceCounted();
    animationOutputManager->Create(keyFrameData);
    if (auto scopedHandle = animationOutputManager->Write(keyFrameData); scopedHandle) {
        scopedHandle->type = PropertyType::VEC4_T;
        constexpr Math::Vec4 values[] = { { 2.0f, 2.0f, 2.0f, 2.0f }, { 5.0f, 5.0f, 5.0f, 5.0f } };
        scopedHandle->data.resize(sizeof(values));
        CloneData(scopedHandle->data.data(), scopedHandle->data.size(), values, sizeof(values));
    }

    EntityReference timeStamps = ecs->GetEntityManager().CreateReferenceCounted();
    animationInputManager->Create(timeStamps);
    if (auto scopedHandle = animationInputManager->Write(timeStamps); scopedHandle) {
        scopedHandle->timestamps.push_back(0.0f);
        scopedHandle->timestamps.push_back(2.0f);
    }

    Entity targetEntity = ecs->GetEntityManager().Create();
    auto fogManager = GetManager<IFogComponentManager>(*ecs);
    ASSERT_NE(nullptr, fogManager);
    fogManager->Create(targetEntity);

    EntityReference track = ecs->GetEntityManager().CreateReferenceCounted();
    animationTrackManager->Create(track);
    if (auto scopedHandle = animationTrackManager->Write(track); scopedHandle) {
        scopedHandle->interpolationMode = AnimationTrackComponent::Interpolation::STEP;
        scopedHandle->timestamps = timeStamps;
        scopedHandle->data = keyFrameData;
        scopedHandle->component = IFogComponentManager::UID;
        scopedHandle->property = "inscatteringColor";
        scopedHandle->target = ecs->GetEntityManager().GetReferenceCounted(targetEntity);
    }

    Entity animation = ecs->GetEntityManager().Create();
    animationManager->Create(animation);
    if (auto scopedHandle = animationManager->Write(animation); scopedHandle) {
        scopedHandle->duration = 1.2f;
        scopedHandle->repeatCount = 1;
        scopedHandle->speed = -1.0f;
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle->tracks.push_back(track);
    }

    IAnimationPlayback* playback = animationSystem->CreatePlayback(animation, { &targetEntity, 1 });
    ASSERT_NE(nullptr, playback);
    ecs->ProcessEvents();

    constexpr uint32_t sToMs = 1000000u;
    uint32_t deltaTime = static_cast<uint32_t>(1.1f * sToMs);
    ecs->Update(deltaTime, deltaTime);

    if (auto scopedHandle = fogManager->Read(targetEntity); scopedHandle) {
        EXPECT_EQ(2.0f, scopedHandle->inscatteringColor.x);
        EXPECT_EQ(2.0f, scopedHandle->inscatteringColor.y);
        EXPECT_EQ(2.0f, scopedHandle->inscatteringColor.z);
        EXPECT_EQ(2.0f, scopedHandle->inscatteringColor.w);
    }
}

/**
 * @tc.name: AnimateArrayTest
 * @tc.desc: Tests for Animate Array Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationSystem, AnimateArrayTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationTrackManager);
    auto animationInputManager = GetManager<IAnimationInputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationInputManager);
    auto animationOutputManager = GetManager<IAnimationOutputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationOutputManager);
    auto animationSystem = GetSystem<IAnimationSystem>(*ecs);
    ASSERT_NE(nullptr, animationSystem);
    animationSystem->SetActive(true);

    EntityReference keyFrameData = ecs->GetEntityManager().CreateReferenceCounted();
    animationOutputManager->Create(keyFrameData);
    if (auto scopedHandle = animationOutputManager->Write(keyFrameData); scopedHandle) {
        scopedHandle->type = PropertyType::FLOAT_VECTOR_T;
        constexpr float values[] = { 0.0f, 0.0f, 0.0f, 5.0f, 5.0f, 5.0f };
        scopedHandle->data.resize(sizeof(values));
        CloneData(scopedHandle->data.data(), scopedHandle->data.size(), values, sizeof(values));
    }

    EntityReference timeStamps = ecs->GetEntityManager().CreateReferenceCounted();
    animationInputManager->Create(timeStamps);
    if (auto scopedHandle = animationInputManager->Write(timeStamps); scopedHandle) {
        scopedHandle->timestamps.push_back(0.0f);
        scopedHandle->timestamps.push_back(2.0f);
    }

    Entity targetEntities[2];
    auto meshManager = GetManager<IMeshComponentManager>(*ecs);
    for (uint32_t i = 0; i < countof(targetEntities); ++i) {
        targetEntities[i] = ecs->GetEntityManager().Create();
        ASSERT_NE(nullptr, meshManager);
        meshManager->Create(targetEntities[i]);
        if (auto scopedHandle = meshManager->Write(targetEntities[i]); scopedHandle) {
            scopedHandle->jointBounds.resize(3);
        }
    }

    EntityReference tracks[2];
    for (uint32_t i = 0; i < countof(tracks); ++i) {
        tracks[i] = ecs->GetEntityManager().CreateReferenceCounted();
        animationTrackManager->Create(tracks[i]);
        if (auto scopedHandle = animationTrackManager->Write(tracks[i]); scopedHandle) {
            if (i == 0) {
                scopedHandle->interpolationMode = AnimationTrackComponent::Interpolation::LINEAR;
            } else {
                scopedHandle->interpolationMode = AnimationTrackComponent::Interpolation::STEP;
            }
            scopedHandle->timestamps = timeStamps;
            scopedHandle->data = keyFrameData;
            scopedHandle->component = IMeshComponentManager::UID;
            scopedHandle->property = "jointBounds";
        }
    }

    Entity animation = ecs->GetEntityManager().Create();
    animationManager->Create(animation);
    if (auto scopedHandle = animationManager->Write(animation); scopedHandle) {
        scopedHandle->duration = 1.2f;
        scopedHandle->repeatCount = 1;
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        for (uint32_t i = 0; i < countof(tracks); ++i) {
            scopedHandle->tracks.push_back(tracks[i]);
        }
    }

    IAnimationPlayback* playback =
        animationSystem->CreatePlayback(animation, { targetEntities, countof(targetEntities) });
    ASSERT_NE(nullptr, playback);
    ecs->ProcessEvents();

    constexpr uint32_t sToMs = 1000000u;
    uint32_t deltaTime = static_cast<uint32_t>(1.1f * sToMs);
    ecs->Update(deltaTime, deltaTime);

    // Linear interpolation
    if (auto scopedHandle = meshManager->Read(targetEntities[0]); scopedHandle) {
        ASSERT_EQ(3, scopedHandle->jointBounds.size());
        EXPECT_EQ(2.75f, scopedHandle->jointBounds[0]);
        EXPECT_EQ(2.75f, scopedHandle->jointBounds[1]);
        EXPECT_EQ(2.75f, scopedHandle->jointBounds[2]);
    }

    // Step interpolation
    if (auto scopedHandle = meshManager->Read(targetEntities[1]); scopedHandle) {
        ASSERT_EQ(3, scopedHandle->jointBounds.size());
        EXPECT_EQ(5.0f, scopedHandle->jointBounds[0]);
        EXPECT_EQ(5.0f, scopedHandle->jointBounds[1]);
        EXPECT_EQ(5.0f, scopedHandle->jointBounds[2]);
    }
}

/**
 * @tc.name: AnimationTasksTest
 * @tc.desc: Tests for Animation Tasks Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationSystem, AnimationTasksTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationTrackManager);
    auto animationInputManager = GetManager<IAnimationInputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationInputManager);
    auto animationOutputManager = GetManager<IAnimationOutputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationOutputManager);
    auto fogManager = GetManager<IFogComponentManager>(*ecs);
    ASSERT_NE(nullptr, fogManager);
    auto animationSystem = GetSystem<IAnimationSystem>(*ecs);
    ASSERT_NE(nullptr, animationSystem);
    animationSystem->SetActive(true);

    constexpr const uint32_t numAnimations = 350u;

    vector<Entity> targetEntities;
    for (uint32_t i = 0; i < numAnimations; ++i) {
        EntityReference keyFrameData = ecs->GetEntityManager().CreateReferenceCounted();
        animationOutputManager->Create(keyFrameData);
        if (auto scopedHandle = animationOutputManager->Write(keyFrameData); scopedHandle) {
            scopedHandle->type = PropertyType::FLOAT_T;
            constexpr float values[] = { 0.0f, 5.0f };
            scopedHandle->data.resize(sizeof(values));
            CloneData(scopedHandle->data.data(), scopedHandle->data.size(), values, sizeof(values));
        }

        EntityReference timeStamps = ecs->GetEntityManager().CreateReferenceCounted();
        animationInputManager->Create(timeStamps);
        if (auto scopedHandle = animationInputManager->Write(timeStamps); scopedHandle) {
            scopedHandle->timestamps.push_back(0.0f);
            scopedHandle->timestamps.push_back(2.0f);
        }

        Entity targetEntity = ecs->GetEntityManager().Create();
        ASSERT_NE(nullptr, fogManager);
        fogManager->Create(targetEntity);
        targetEntities.push_back(targetEntity);

        EntityReference track = ecs->GetEntityManager().CreateReferenceCounted();
        animationTrackManager->Create(track);
        if (auto scopedHandle = animationTrackManager->Write(track); scopedHandle) {
            scopedHandle->interpolationMode = AnimationTrackComponent::Interpolation::LINEAR;
            scopedHandle->timestamps = timeStamps;
            scopedHandle->data = keyFrameData;
            scopedHandle->component = IFogComponentManager::UID;
            scopedHandle->property = "startDistance";
            scopedHandle->target = ecs->GetEntityManager().GetReferenceCounted(targetEntity);
        }

        Entity animation = ecs->GetEntityManager().Create();
        animationManager->Create(animation);
        if (auto scopedHandle = animationManager->Write(animation); scopedHandle) {
            scopedHandle->duration = 1.2f;
            scopedHandle->repeatCount = 1;
            scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
            scopedHandle->tracks.push_back(track);
        }

        IAnimationPlayback* playback = animationSystem->CreatePlayback(animation, { &targetEntity, 1 });
        ASSERT_NE(nullptr, playback);
    }
    ecs->ProcessEvents();

    constexpr uint32_t sToMs = 1000000u;
    uint32_t deltaTime = 1u * sToMs;
    ecs->Update(deltaTime, deltaTime);

    for (Entity targetEntity : targetEntities) {
        if (auto scopedHandle = fogManager->Read(targetEntity); scopedHandle) {
            EXPECT_EQ(2.5f, scopedHandle->startDistance);
        }
    }
}

/**
 * @tc.name: AnimateStateTest
 * @tc.desc: Tests for Animate State Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationSystem, AnimateStateTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationTrackManager);
    auto animationInputManager = GetManager<IAnimationInputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationInputManager);
    auto animationOutputManager = GetManager<IAnimationOutputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationOutputManager);
    auto animationStateManager = GetManager<IAnimationStateComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationStateManager);
    auto animationSystem = GetSystem<IAnimationSystem>(*ecs);
    ASSERT_NE(nullptr, animationSystem);
    animationSystem->SetActive(true);

    EntityReference keyFrameData = ecs->GetEntityManager().CreateReferenceCounted();
    animationOutputManager->Create(keyFrameData);
    if (auto scopedHandle = animationOutputManager->Write(keyFrameData); scopedHandle) {
        scopedHandle->type = PropertyType::FLOAT_T;
        constexpr float values[] = { 0.0f, 5.0f };
        scopedHandle->data.resize(sizeof(values));
        CloneData(scopedHandle->data.data(), scopedHandle->data.size(), values, sizeof(values));
    }

    EntityReference timeStamps = ecs->GetEntityManager().CreateReferenceCounted();
    animationInputManager->Create(timeStamps);
    if (auto scopedHandle = animationInputManager->Write(timeStamps); scopedHandle) {
        scopedHandle->timestamps.push_back(0.0f);
        scopedHandle->timestamps.push_back(2.0f);
    }

    Entity targetEntity = ecs->GetEntityManager().Create();
    auto fogManager = GetManager<IFogComponentManager>(*ecs);
    ASSERT_NE(nullptr, fogManager);
    fogManager->Create(targetEntity);

    EntityReference track = ecs->GetEntityManager().CreateReferenceCounted();
    animationTrackManager->Create(track);
    if (auto scopedHandle = animationTrackManager->Write(track); scopedHandle) {
        scopedHandle->interpolationMode = AnimationTrackComponent::Interpolation::LINEAR;
        scopedHandle->timestamps = timeStamps;
        scopedHandle->data = keyFrameData;
        scopedHandle->component = IFogComponentManager::UID;
        scopedHandle->property = "startDistance";
        scopedHandle->target = ecs->GetEntityManager().GetReferenceCounted(targetEntity);
    }

    Entity animation = ecs->GetEntityManager().Create();
    animationManager->Create(animation);
    if (auto scopedHandle = animationManager->Write(animation); scopedHandle) {
        scopedHandle->duration = 1.2f;
        scopedHandle->repeatCount = 1;
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle->tracks.push_back(track);
    }

    // Create state and start animation halfway.
    animationStateManager->Create(animation);
    animationStateManager->Write(animation)->time = 1.f;

    ecs->ProcessEvents();

    constexpr uint32_t sToMs = 1000000u;
    uint32_t deltaTime = 1u * sToMs;
    ecs->Update(deltaTime, deltaTime);

    if (auto scopedHandle = fogManager->Read(targetEntity); scopedHandle) {
        EXPECT_EQ(5.f, scopedHandle->startDistance);
    }
}

namespace {
CORE_NS::EntityReference CreateShader(CORE_NS::IEcs& ecs, RENDER_NS::IRenderContext& renderContext, BASE_NS::string uri)
{
    auto uriManager = GetManager<IUriComponentManager>(ecs);
    auto renderHandleManager = GetManager<IRenderHandleComponentManager>(ecs);
    auto& shaderMgr = renderContext.GetDevice().GetShaderManager();

    const EntityReference shader = ecs.GetEntityManager().CreateReferenceCounted();
    renderHandleManager->Create(shader);
    shaderMgr.LoadShaderFile(uri);
    renderHandleManager->Write(shader)->reference = shaderMgr.GetShaderHandle(uri);
    uriManager->Create(shader);
    uriManager->Write(shader)->uri = uri;
    return shader;
}
} // namespace
/**
 * @tc.name: AnimateCustomPropertyTest
 * @tc.desc: Tests for Animate Custom Property Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationSystem, AnimateCustomPropertyTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);
    auto animationTrackManager = GetManager<IAnimationTrackComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationTrackManager);
    auto animationInputManager = GetManager<IAnimationInputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationInputManager);
    auto animationOutputManager = GetManager<IAnimationOutputComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationOutputManager);
    auto animationSystem = GetSystem<IAnimationSystem>(*ecs);
    ASSERT_NE(nullptr, animationSystem);
    animationSystem->SetActive(true);

    EntityReference keyFrameData = ecs->GetEntityManager().CreateReferenceCounted();
    animationOutputManager->Create(keyFrameData);
    if (auto scopedHandle = animationOutputManager->Write(keyFrameData); scopedHandle) {
        scopedHandle->type = PropertyType::FLOAT_T;
        constexpr float values[] = { 0.0f, 5.0f };
        scopedHandle->data.resize(sizeof(values));
        CloneData(scopedHandle->data.data(), scopedHandle->data.size(), values, sizeof(values));
    }

    EntityReference timeStamps = ecs->GetEntityManager().CreateReferenceCounted();
    animationInputManager->Create(timeStamps);
    if (auto scopedHandle = animationInputManager->Write(timeStamps); scopedHandle) {
        scopedHandle->timestamps.push_back(0.0f);
        scopedHandle->timestamps.push_back(2.0f);
    }

    Entity targetEntity = ecs->GetEntityManager().Create();
    auto materialManager = GetManager<IMaterialComponentManager>(*ecs);
    ASSERT_NE(nullptr, materialManager);
    materialManager->Create(targetEntity);
    if (auto scopedHandle = materialManager->Write(targetEntity)) {
        scopedHandle->materialShader.shader = CreateShader(*ecs, *renderContext, "test://shader/test.shader");
    }

    EntityReference track = ecs->GetEntityManager().CreateReferenceCounted();
    animationTrackManager->Create(track);
    if (auto scopedHandle = animationTrackManager->Write(track); scopedHandle) {
        scopedHandle->interpolationMode = AnimationTrackComponent::Interpolation::LINEAR;
        scopedHandle->timestamps = timeStamps;
        scopedHandle->data = keyFrameData;
        scopedHandle->component = IMaterialComponentManager::UID;
        scopedHandle->property = "customProperties.f_";
        scopedHandle->target = ecs->GetEntityManager().GetReferenceCounted(targetEntity);
    }

    Entity animation = ecs->GetEntityManager().Create();
    animationManager->Create(animation);
    if (auto scopedHandle = animationManager->Write(animation); scopedHandle) {
        scopedHandle->duration = 1.2f;
        scopedHandle->repeatCount = 1;
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle->tracks.push_back(track);
    }

    IAnimationPlayback* playback = animationSystem->CreatePlayback(animation, { &targetEntity, 1 });
    ASSERT_NE(nullptr, playback);
    ecs->ProcessEvents();

    constexpr uint32_t sToMs = 1000000u;
    uint32_t deltaTime = 1u * sToMs;
    ecs->Update(deltaTime, deltaTime);

    if (auto scopedHandle = materialManager->Read(targetEntity); scopedHandle) {
        EXPECT_EQ(2.5f, CORE_NS::GetPropertyValue<float>(scopedHandle->customProperties, "f_"));
        EXPECT_EQ(3U, CORE_NS::GetPropertyValue<uint32_t>(scopedHandle->customProperties, "uv4_.x"));
    }
}