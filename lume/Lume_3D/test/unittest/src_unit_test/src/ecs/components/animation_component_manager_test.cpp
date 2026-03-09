/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <3d/ecs/components/animation_component.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#if !defined(JSON_IMPL)
#define JSON_IMPL
#endif
#include <core/json/json.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>

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
 * @tc.name: AnimationComponent_DefaultValues
 * @tc.desc: Tests AnimationComponent default values are properly initialized [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_DefaultValues, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    // Check default values
    if (auto scopedHandle = animationManager->Read(entity); scopedHandle) {
        EXPECT_EQ(AnimationComponent::PlaybackState::STOP, scopedHandle->state);
        EXPECT_EQ(1u, scopedHandle->repeatCount);
        EXPECT_FLOAT_EQ(0.0f, scopedHandle->startOffset);
        EXPECT_FLOAT_EQ(0.0f, scopedHandle->duration);
        EXPECT_FLOAT_EQ(1.0f, scopedHandle->weight);
        EXPECT_FLOAT_EQ(1.0f, scopedHandle->speed);
        EXPECT_EQ(0u, scopedHandle->tracks.size());
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_PlaybackState_Stop
 * @tc.desc: Tests animation playback state STOP [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_PlaybackState_Stop, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        scopedHandle->state = AnimationComponent::PlaybackState::STOP;
        EXPECT_EQ(AnimationComponent::PlaybackState::STOP, scopedHandle->state);
    }

    if (auto scopedHandle = animationManager->Read(entity); scopedHandle) {
        EXPECT_EQ(AnimationComponent::PlaybackState::STOP, scopedHandle->state);
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_PlaybackState_Play
 * @tc.desc: Tests animation playback state PLAY [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_PlaybackState_Play, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        EXPECT_EQ(AnimationComponent::PlaybackState::PLAY, scopedHandle->state);
    }

    if (auto scopedHandle = animationManager->Read(entity); scopedHandle) {
        EXPECT_EQ(AnimationComponent::PlaybackState::PLAY, scopedHandle->state);
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_PlaybackState_Pause
 * @tc.desc: Tests animation playback state PAUSE [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_PlaybackState_Pause, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        scopedHandle->state = AnimationComponent::PlaybackState::PAUSE;
        EXPECT_EQ(AnimationComponent::PlaybackState::PAUSE, scopedHandle->state);
    }

    if (auto scopedHandle = animationManager->Read(entity); scopedHandle) {
        EXPECT_EQ(AnimationComponent::PlaybackState::PAUSE, scopedHandle->state);
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_PlaybackState_Transitions
 * @tc.desc: Tests animation playback state transitions [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_PlaybackState_Transitions, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        // STOP -> PLAY
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        EXPECT_EQ(AnimationComponent::PlaybackState::PLAY, scopedHandle->state);

        // PLAY -> PAUSE
        scopedHandle->state = AnimationComponent::PlaybackState::PAUSE;
        EXPECT_EQ(AnimationComponent::PlaybackState::PAUSE, scopedHandle->state);

        // PAUSE -> PLAY
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        EXPECT_EQ(AnimationComponent::PlaybackState::PLAY, scopedHandle->state);

        // PLAY -> STOP
        scopedHandle->state = AnimationComponent::PlaybackState::STOP;
        EXPECT_EQ(AnimationComponent::PlaybackState::STOP, scopedHandle->state);
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_RepeatCount
 * @tc.desc: Tests animation repeat count property [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_RepeatCount, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        // Test finite repeat counts
        scopedHandle->repeatCount = 0; // Play once
        EXPECT_EQ(0u, scopedHandle->repeatCount);

        scopedHandle->repeatCount = 1; // Repeat once
        EXPECT_EQ(1u, scopedHandle->repeatCount);

        scopedHandle->repeatCount = 10; // Repeat 10 times
        EXPECT_EQ(10u, scopedHandle->repeatCount);

        // Test infinite repeat
        scopedHandle->repeatCount = AnimationComponent::REPEAT_COUNT_INFINITE;
        EXPECT_EQ(AnimationComponent::REPEAT_COUNT_INFINITE, scopedHandle->repeatCount);
        EXPECT_EQ(~0u, scopedHandle->repeatCount);
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_StartOffset
 * @tc.desc: Tests animation start offset property [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_StartOffset, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        // Test various start offsets
        scopedHandle->startOffset = 0.0f;
        EXPECT_FLOAT_EQ(0.0f, scopedHandle->startOffset);

        scopedHandle->startOffset = 0.5f;
        EXPECT_FLOAT_EQ(0.5f, scopedHandle->startOffset);

        scopedHandle->startOffset = 1.0f;
        EXPECT_FLOAT_EQ(1.0f, scopedHandle->startOffset);

        scopedHandle->startOffset = 10.5f;
        EXPECT_FLOAT_EQ(10.5f, scopedHandle->startOffset);
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_Duration
 * @tc.desc: Tests animation duration property [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_Duration, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        // Test various durations (0 means full animation)
        scopedHandle->duration = 0.0f;
        EXPECT_FLOAT_EQ(0.0f, scopedHandle->duration);

        scopedHandle->duration = 0.5f;
        EXPECT_FLOAT_EQ(0.5f, scopedHandle->duration);

        scopedHandle->duration = 1.0f;
        EXPECT_FLOAT_EQ(1.0f, scopedHandle->duration);

        scopedHandle->duration = 60.0f; // 1 minute
        EXPECT_FLOAT_EQ(60.0f, scopedHandle->duration);
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_Weight
 * @tc.desc: Tests animation weight property [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_Weight, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        // Test various weights
        scopedHandle->weight = 0.0f;
        EXPECT_FLOAT_EQ(0.0f, scopedHandle->weight);

        scopedHandle->weight = 0.5f;
        EXPECT_FLOAT_EQ(0.5f, scopedHandle->weight);

        scopedHandle->weight = 1.0f;
        EXPECT_FLOAT_EQ(1.0f, scopedHandle->weight);

        // Test weight > 1.0 (for animation blending)
        scopedHandle->weight = 2.0f;
        EXPECT_FLOAT_EQ(2.0f, scopedHandle->weight);
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_Speed
 * @tc.desc: Tests animation speed property [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_Speed, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        // Test normal speed
        scopedHandle->speed = 1.0f;
        EXPECT_FLOAT_EQ(1.0f, scopedHandle->speed);

        // Test double speed
        scopedHandle->speed = 2.0f;
        EXPECT_FLOAT_EQ(2.0f, scopedHandle->speed);

        // Test half speed
        scopedHandle->speed = 0.5f;
        EXPECT_FLOAT_EQ(0.5f, scopedHandle->speed);

        // Test reverse speed
        scopedHandle->speed = -1.0f;
        EXPECT_FLOAT_EQ(-1.0f, scopedHandle->speed);

        // Test fast reverse
        scopedHandle->speed = -2.0f;
        EXPECT_FLOAT_EQ(-2.0f, scopedHandle->speed);

        // Test paused (zero speed)
        scopedHandle->speed = 0.0f;
        EXPECT_FLOAT_EQ(0.0f, scopedHandle->speed);
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_Tracks_Empty
 * @tc.desc: Tests animation component with no tracks [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_Tracks_Empty, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    if (auto scopedHandle = animationManager->Read(entity); scopedHandle) {
        EXPECT_EQ(0u, scopedHandle->tracks.size());
        EXPECT_TRUE(scopedHandle->tracks.empty());
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_Tracks_AddRemove
 * @tc.desc: Tests adding and removing animation tracks [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_Tracks_AddRemove, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        // Add tracks
        Entity track1 = ecs->GetEntityManager().Create();
        Entity track2 = ecs->GetEntityManager().Create();
        Entity track3 = ecs->GetEntityManager().Create();

        scopedHandle->tracks.push_back(EntityReference(track1, nullptr));
        EXPECT_EQ(1u, scopedHandle->tracks.size());

        scopedHandle->tracks.push_back(EntityReference(track2, nullptr));
        EXPECT_EQ(2u, scopedHandle->tracks.size());

        scopedHandle->tracks.push_back(EntityReference(track3, nullptr));
        EXPECT_EQ(3u, scopedHandle->tracks.size());

        EXPECT_EQ(track1, scopedHandle->tracks[0].operator Entity());
        EXPECT_EQ(track2, scopedHandle->tracks[1].operator Entity());
        EXPECT_EQ(track3, scopedHandle->tracks[2].operator Entity());

        // Remove a track
        scopedHandle->tracks.pop_back();
        EXPECT_EQ(2u, scopedHandle->tracks.size());

        // Clear all tracks
        scopedHandle->tracks.clear();
        EXPECT_EQ(0u, scopedHandle->tracks.size());

        ecs->GetEntityManager().Destroy(track1);
        ecs->GetEntityManager().Destroy(track2);
        ecs->GetEntityManager().Destroy(track3);
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_AllProperties
 * @tc.desc: Tests setting all animation component properties [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_AllProperties, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    Entity track1 = ecs->GetEntityManager().Create();
    Entity track2 = ecs->GetEntityManager().Create();

    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle->repeatCount = 5;
        scopedHandle->startOffset = 1.5f;
        scopedHandle->duration = 10.0f;
        scopedHandle->weight = 0.75f;
        scopedHandle->speed = 1.5f;
        scopedHandle->tracks.push_back(EntityReference(track1, nullptr));
        scopedHandle->tracks.push_back(EntityReference(track2, nullptr));
    }

    if (auto scopedHandle = animationManager->Read(entity); scopedHandle) {
        EXPECT_EQ(AnimationComponent::PlaybackState::PLAY, scopedHandle->state);
        EXPECT_EQ(5u, scopedHandle->repeatCount);
        EXPECT_FLOAT_EQ(1.5f, scopedHandle->startOffset);
        EXPECT_FLOAT_EQ(10.0f, scopedHandle->duration);
        EXPECT_FLOAT_EQ(0.75f, scopedHandle->weight);
        EXPECT_FLOAT_EQ(1.5f, scopedHandle->speed);
        EXPECT_EQ(2u, scopedHandle->tracks.size());
        EXPECT_EQ(track1, scopedHandle->tracks[0].operator Entity());
        EXPECT_EQ(track2, scopedHandle->tracks[1].operator Entity());
    }

    ecs->GetEntityManager().Destroy(track1);
    ecs->GetEntityManager().Destroy(track2);
    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_EntityLifecycle
 * @tc.desc: Tests AnimationComponent entity lifecycle operations [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_EntityLifecycle, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    // Create component
    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    bool hasComponent = animationManager->HasComponent(entity);
    EXPECT_TRUE(hasComponent);

    // Destroy component
    animationManager->Destroy(entity);

    hasComponent = animationManager->HasComponent(entity);
    EXPECT_FALSE(hasComponent);

    // Recreate component
    animationManager->Create(entity);

    hasComponent = animationManager->HasComponent(entity);
    EXPECT_TRUE(hasComponent);

    // Clean up entity
    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_MultipleEntities
 * @tc.desc: Tests AnimationComponent with multiple entities [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_MultipleEntities, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    const uint32_t entityCount = 5;
    BASE_NS::vector<Entity> entities;

    for (uint32_t i = 0; i < entityCount; ++i) {
        Entity entity = ecs->GetEntityManager().Create();
        animationManager->Create(entity);
        entities.push_back(entity);

        if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
            scopedHandle->state = (i % 2 == 0) ? AnimationComponent::PlaybackState::PLAY :
                                                 AnimationComponent::PlaybackState::PAUSE;
            scopedHandle->speed = static_cast<float>(i + 1) * 0.5f;
            scopedHandle->repeatCount = i + 1;
        }
    }

    for (uint32_t i = 0; i < entityCount; ++i) {
        if (auto scopedHandle = animationManager->Read(entities[i]); scopedHandle) {
            auto expectedState = (i % 2 == 0) ? AnimationComponent::PlaybackState::PLAY :
                                                AnimationComponent::PlaybackState::PAUSE;
            EXPECT_EQ(expectedState, scopedHandle->state);
            EXPECT_FLOAT_EQ(static_cast<float>(i + 1) * 0.5f, scopedHandle->speed);
            EXPECT_EQ(static_cast<uint32_t>(i + 1), scopedHandle->repeatCount);
        }
    }

    // Clean up all entities
    for (uint32_t i = 0; i < entityCount; ++i) {
        animationManager->Destroy(entities[i]);
        ecs->GetEntityManager().Destroy(entities[i]);
    }
}

/**
 * @tc.name: AnimationComponent_ReadWriteConsistency
 * @tc.desc: Tests consistency between read and write operations [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_ReadWriteConsistency, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    const int iterations = 10;

    for (int i = 0; i < iterations; ++i) {
        if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
            scopedHandle->state = (i % 3 == 0) ? AnimationComponent::PlaybackState::STOP :
                                 (i % 3 == 1) ? AnimationComponent::PlaybackState::PLAY :
                                                AnimationComponent::PlaybackState::PAUSE;
            scopedHandle->repeatCount = (i + 1) * 2;
            scopedHandle->speed = static_cast<float>(i + 1) * 0.25f;
            scopedHandle->weight = 0.5f + static_cast<float>(i) * 0.1f;
        }

        if (auto scopedHandle = animationManager->Read(entity); scopedHandle) {
            auto expectedState = (i % 3 == 0) ? AnimationComponent::PlaybackState::STOP :
                                 (i % 3 == 1) ? AnimationComponent::PlaybackState::PLAY :
                                                AnimationComponent::PlaybackState::PAUSE;
            EXPECT_EQ(expectedState, scopedHandle->state);
            EXPECT_EQ(static_cast<uint32_t>((i + 1) * 2), scopedHandle->repeatCount);
            EXPECT_FLOAT_EQ(static_cast<float>(i + 1) * 0.25f, scopedHandle->speed);
            EXPECT_FLOAT_EQ(0.5f + static_cast<float>(i) * 0.1f, scopedHandle->weight);
        }
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_Blending_Weight
 * @tc.desc: Tests animation blending with weight property [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_Blending_Weight, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    // Create two animation entities for blending
    Entity anim1 = ecs->GetEntityManager().Create();
    Entity anim2 = ecs->GetEntityManager().Create();

    animationManager->Create(anim1);
    animationManager->Create(anim2);

    if (auto scopedHandle1 = animationManager->Write(anim1); scopedHandle1) {
        scopedHandle1->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle1->weight = 0.7f; // 70% influence
    }

    if (auto scopedHandle2 = animationManager->Write(anim2); scopedHandle2) {
        scopedHandle2->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle2->weight = 0.3f; // 30% influence
    }

    if (auto scopedHandle1 = animationManager->Read(anim1); scopedHandle1) {
        EXPECT_EQ(AnimationComponent::PlaybackState::PLAY, scopedHandle1->state);
        EXPECT_FLOAT_EQ(0.7f, scopedHandle1->weight);
    }

    if (auto scopedHandle2 = animationManager->Read(anim2); scopedHandle2) {
        EXPECT_EQ(AnimationComponent::PlaybackState::PLAY, scopedHandle2->state);
        EXPECT_FLOAT_EQ(0.3f, scopedHandle2->weight);
    }

    animationManager->Destroy(anim1);
    animationManager->Destroy(anim2);
    ecs->GetEntityManager().Destroy(anim1);
    ecs->GetEntityManager().Destroy(anim2);
}

/**
 * @tc.name: AnimationComponent_Loop_InfiniteRepeat
 * @tc.desc: Tests animation infinite looping [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_Loop_InfiniteRepeat, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle->repeatCount = AnimationComponent::REPEAT_COUNT_INFINITE;
        EXPECT_EQ(AnimationComponent::REPEAT_COUNT_INFINITE, scopedHandle->repeatCount);
        EXPECT_EQ(~0u, scopedHandle->repeatCount);
    }

    if (auto scopedHandle = animationManager->Read(entity); scopedHandle) {
        EXPECT_EQ(AnimationComponent::PlaybackState::PLAY, scopedHandle->state);
        EXPECT_EQ(AnimationComponent::REPEAT_COUNT_INFINITE, scopedHandle->repeatCount);
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_ReversePlayback
 * @tc.desc: Tests reverse animation playback with negative speed [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_ReversePlayback, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle->speed = -1.0f; // Normal reverse speed
        EXPECT_FLOAT_EQ(-1.0f, scopedHandle->speed);

        scopedHandle->speed = -2.0f; // 2x reverse speed
        EXPECT_FLOAT_EQ(-2.0f, scopedHandle->speed);

        scopedHandle->speed = -0.5f; // Half reverse speed
        EXPECT_FLOAT_EQ(-0.5f, scopedHandle->speed);
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_TimeClipping
 * @tc.desc: Tests animation time clipping with start offset and duration [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_TimeClipping, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        // Skip first 2 seconds, play for 5 seconds
        scopedHandle->startOffset = 2.0f;
        scopedHandle->duration = 5.0f;

        EXPECT_FLOAT_EQ(2.0f, scopedHandle->startOffset);
        EXPECT_FLOAT_EQ(5.0f, scopedHandle->duration);
    }

    if (auto scopedHandle = animationManager->Read(entity); scopedHandle) {
        EXPECT_FLOAT_EQ(2.0f, scopedHandle->startOffset);
        EXPECT_FLOAT_EQ(5.0f, scopedHandle->duration);
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_TrackReferences
 * @tc.desc: Tests animation track entity references [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_TrackReferences, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);

    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        // Create multiple track entities
        const uint32_t trackCount = 10;
        BASE_NS::vector<Entity> tracks;

        for (uint32_t i = 0; i < trackCount; ++i) {
            Entity track = ecs->GetEntityManager().Create();
            tracks.push_back(track);
            scopedHandle->tracks.push_back(EntityReference(track, nullptr));
        }

        EXPECT_EQ(trackCount, scopedHandle->tracks.size());

        // Verify all track references
        for (uint32_t i = 0; i < trackCount; ++i) {
            EXPECT_EQ(tracks[i], scopedHandle->tracks[i].operator Entity());
        }

        // Clean up track entities
        for (uint32_t i = 0; i < trackCount; ++i) {
            ecs->GetEntityManager().Destroy(tracks[i]);
        }
    }

    animationManager->Destroy(entity);
    ecs->GetEntityManager().Destroy(entity);
}

/**
 * @tc.name: AnimationComponent_CopyBetweenEntities
 * @tc.desc: Tests copying animation properties between entities [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsAnimationComponent, AnimationComponent_CopyBetweenEntities, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    ASSERT_NE(nullptr, animationManager);

    Entity sourceEntity = ecs->GetEntityManager().Create();
    Entity targetEntity = ecs->GetEntityManager().Create();

    animationManager->Create(sourceEntity);
    animationManager->Create(targetEntity);

    // Set up source animation
    Entity track1 = ecs->GetEntityManager().Create();
    Entity track2 = ecs->GetEntityManager().Create();

    if (auto scopedHandle = animationManager->Write(sourceEntity); scopedHandle) {
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle->repeatCount = 3;
        scopedHandle->startOffset = 1.0f;
        scopedHandle->duration = 5.0f;
        scopedHandle->weight = 0.8f;
        scopedHandle->speed = 1.5f;
        scopedHandle->tracks.push_back(EntityReference(track1, nullptr));
        scopedHandle->tracks.push_back(EntityReference(track2, nullptr));
    }

    // Copy from source to target
    if (auto sourceHandle = animationManager->Read(sourceEntity); sourceHandle) {
        if (auto targetHandle = animationManager->Write(targetEntity); targetHandle) {
            targetHandle->state = sourceHandle->state;
            targetHandle->repeatCount = sourceHandle->repeatCount;
            targetHandle->startOffset = sourceHandle->startOffset;
            targetHandle->duration = sourceHandle->duration;
            targetHandle->weight = sourceHandle->weight;
            targetHandle->speed = sourceHandle->speed;
            targetHandle->tracks = sourceHandle->tracks;
        }
    }

    // Verify target matches source
    if (auto scopedHandle = animationManager->Read(targetEntity); scopedHandle) {
        EXPECT_EQ(AnimationComponent::PlaybackState::PLAY, scopedHandle->state);
        EXPECT_EQ(3u, scopedHandle->repeatCount);
        EXPECT_FLOAT_EQ(1.0f, scopedHandle->startOffset);
        EXPECT_FLOAT_EQ(5.0f, scopedHandle->duration);
        EXPECT_FLOAT_EQ(0.8f, scopedHandle->weight);
        EXPECT_FLOAT_EQ(1.5f, scopedHandle->speed);
        EXPECT_EQ(2u, scopedHandle->tracks.size());
    }

    ecs->GetEntityManager().Destroy(track1);
    ecs->GetEntityManager().Destroy(track2);
    animationManager->Destroy(sourceEntity);
    animationManager->Destroy(targetEntity);
    ecs->GetEntityManager().Destroy(sourceEntity);
    ecs->GetEntityManager().Destroy(targetEntity);
}
