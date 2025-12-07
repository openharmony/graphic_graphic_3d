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

#include <cstdint>
#include <scene/api/scene.h>
#include <scene/ext/component_util.h>
#include <scene/interface/intf_scene.h>

#include <3d/ecs/components/animation_component.h>
#include <3d/namespace.h>

#include <meta/api/animation.h>

#include "scene/scene_component_test.h"
#include "scene/scene_test.h"

META_TYPE(CORE3D_NS::AnimationComponent::PlaybackState)

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginAnimationComponentTest : public ScenePluginComponentTest<CORE3D_NS::IAnimationComponentManager> {};

/**
 * @tc.name: Members
 * @tc.desc: Tests for Members. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginAnimationComponentTest, Members, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    const auto node = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(AddComponent<CORE3D_NS::IAnimationComponentManager>(node));
    SetComponent(node, "AnimationComponent");

    auto play = CORE3D_NS::AnimationComponent::PlaybackState::PLAY;
    TestEngineProperty<CORE3D_NS::AnimationComponent::PlaybackState>("State", play, nativeComponent.state);
    TestEngineProperty<uint32_t>("RepeatCount", 5, nativeComponent.repeatCount);
    TestEngineProperty<float>("StartOffset", 1.1f, nativeComponent.startOffset);
    TestEngineProperty<float>("Duration", 2.2f, nativeComponent.duration);
    TestEngineProperty<float>("Weight", 3.3f, nativeComponent.weight);
    TestEngineProperty<float>("Speed", 4.4f, nativeComponent.speed);
}

/**
 * @tc.name: Duration
 * @tc.desc: Tests for Duration. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginAnimationComponentTest, Duration, testing::ext::TestSize.Level1)
{
    auto scene = Scene(LoadScene("test://AnimatedCube/AnimatedCube.gltf"));
    ASSERT_TRUE(scene);

    auto animations = scene.GetAnimations();
    ASSERT_FALSE(animations.empty());

    auto animation = META_NS::TimedAnimation(animations[0]);
    EXPECT_TRUE(animation.GetPtr<IEcsObjectAccess>());

    EXPECT_EQ(animation.GetDuration(), META_NS::TimeSpan::Seconds(2));
}

} // namespace UTest

SCENE_END_NAMESPACE()
