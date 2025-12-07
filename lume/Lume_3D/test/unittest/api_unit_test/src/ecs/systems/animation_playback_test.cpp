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
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
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
 * @tc.name: GetSetTest
 * @tc.desc: Tests for Get Set Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsAnimationPlayback, GetSetTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    auto nameManager = GetManager<INameComponentManager>(*ecs);
    auto animationSystem = GetSystem<IAnimationSystem>(*ecs);

    Entity entity = ecs->GetEntityManager().Create();
    animationManager->Create(entity);
    if (auto scopedHandle = animationManager->Write(entity); scopedHandle) {
        scopedHandle->repeatCount = 16;
        scopedHandle->speed = 2.0f;
        scopedHandle->startOffset = 0.5f;
        scopedHandle->state = AnimationComponent::PlaybackState::PLAY;
        scopedHandle->tracks = {};
        scopedHandle->weight = 1.2f;
    }
    nameManager->Create(entity);
    if (auto scopedHandle = nameManager->Write(entity); scopedHandle) {
        scopedHandle->name = "Animation";
    }

    IAnimationPlayback* playback = animationSystem->CreatePlayback(entity, {});
    ASSERT_NE(nullptr, playback);

    // Get values from animation component of entity
    {
        EXPECT_EQ("Animation", playback->GetName());
        EXPECT_EQ(0.0f, playback->GetDuration());
        EXPECT_EQ(16, playback->GetRepeatCount());
        EXPECT_EQ(2.0f, playback->GetSpeed());
        EXPECT_EQ(0.5f, playback->GetStartOffset());
        EXPECT_EQ(AnimationComponent::PlaybackState::PLAY, playback->GetPlaybackState());
        EXPECT_EQ(1.2f, playback->GetWeight());
        EXPECT_EQ(0.0f, playback->GetAnimationLength());
        EXPECT_EQ(0.0f, playback->GetTimePosition());
        EXPECT_EQ(false, playback->IsCompleted());
    }
    // Try setters
    {
        playback->SetDuration(5.3f);
        playback->SetRepeatCount(3);
        playback->SetSpeed(3.0f);
        playback->SetStartOffset(1.5f);
        playback->SetPlaybackState(AnimationComponent::PlaybackState::PAUSE);
        playback->SetWeight(2.0f);
        playback->SetTimePosition(4.0f);
    }
    // Get previously set values
    {
        EXPECT_EQ(5.3f, playback->GetDuration());
        EXPECT_EQ(3, playback->GetRepeatCount());
        EXPECT_EQ(3.0f, playback->GetSpeed());
        EXPECT_EQ(1.5f, playback->GetStartOffset());
        EXPECT_EQ(AnimationComponent::PlaybackState::PAUSE, playback->GetPlaybackState());
        EXPECT_EQ(2.0f, playback->GetWeight());
        EXPECT_EQ(4.0f, playback->GetTimePosition());
    }
    // Get defaults when entity doesn't have animation component
    {
        animationManager->Destroy(entity);
        animationManager->Gc();
        EXPECT_EQ(0.0f, playback->GetDuration());
        EXPECT_EQ(0u, playback->GetRepeatCount());
        EXPECT_EQ(0.0f, playback->GetSpeed());
        EXPECT_EQ(0.0f, playback->GetStartOffset());
        EXPECT_EQ(AnimationComponent::PlaybackState::STOP, playback->GetPlaybackState());
        EXPECT_EQ(0.0f, playback->GetWeight());
        animationManager->Create(entity);
    }
    // When destroyed playback should stop animation
    {
        animationSystem->DestroyPlayback(playback);
        if (auto scopedHandle = animationManager->Read(entity); scopedHandle) {
            EXPECT_EQ(AnimationComponent::PlaybackState::STOP, scopedHandle->state);
        }
    }
}
