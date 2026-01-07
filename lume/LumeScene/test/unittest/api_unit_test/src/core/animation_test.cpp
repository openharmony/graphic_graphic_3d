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

#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>

#include <3d/ecs/components/material_component.h>

#include <meta/api/animation.h>
#include <meta/api/make_callback.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/modifiers/intf_speed.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginAnimationTest : public ScenePluginTest {
public:
protected:
    auto GetAnimationClassId() const
    {
        // ClassId::EcsAnimation
        static constexpr META_NS::ObjectId ID { "5513d745-958f-4aa6-bab7-7561cebdc3dd" };
        return ID;
    }
};

/**
 * @tc.name: Name
 * @tc.desc: Tests for Name. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginAnimationTest, Name, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://celia/Celia.gltf");
    ASSERT_TRUE(scene);
    auto anims = scene->GetAnimations().GetResult();
    ASSERT_TRUE(!anims.empty());
    auto anim = anims[0];
    auto obj = interface_cast<META_NS::IObject>(anim);
    ASSERT_TRUE(obj);
    auto named = interface_cast<META_NS::INamed>(anim);
    ASSERT_TRUE(named);

    EXPECT_EQ(obj->GetName(), named->Name()->GetValue());
    named->Name()->SetValue("hipshops");
    EXPECT_EQ(obj->GetName(), named->Name()->GetValue());
}

/**
 * @tc.name: Progress
 * @tc.desc: Tests for Progress. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginAnimationTest, Progress, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://celia/Celia.gltf");
    ASSERT_TRUE(scene);
    auto anims = scene->GetAnimations().GetResult();
    ASSERT_TRUE(!anims.empty());
    auto anim = anims[0];

    interface_cast<META_NS::IStartableAnimation>(anim)->Start();
    UpdateScene();
    EXPECT_TRUE(anim->Running()->GetValue());
    auto p1 = anim->Progress()->GetValue();
    UpdateScene(META_NS::TimeSpan::Milliseconds(50));
    WaitForUserQueue();
    auto p2 = anim->Progress()->GetValue();
    EXPECT_LT(p1, p2);
    UpdateScene(META_NS::TimeSpan::Milliseconds(50));
    WaitForUserQueue();
    auto p3 = anim->Progress()->GetValue();
    EXPECT_LT(p2, p3);
}

/**
 * @tc.name: ImportedSceneAnimationProgress
 * @tc.desc: Tests for Imported Scene Animation Progress. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginAnimationTest, ImportedSceneAnimationProgress, testing::ext::TestSize.Level1)
{
    auto celia = LoadScene("test://celia/Celia.gltf");
    ASSERT_TRUE(celia);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto node = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(node);

    auto imp = interface_cast<INodeImport>(node);
    node = imp->ImportChildScene(celia, "some").GetResult();
    ASSERT_TRUE(node);

    auto anims = scene->GetAnimations().GetResult();
    ASSERT_TRUE(!anims.empty());
    auto anim = anims[0];

    interface_cast<META_NS::IStartableAnimation>(anim)->Start();
    UpdateScene();
    EXPECT_TRUE(anim->Running()->GetValue());
    auto p1 = anim->Progress()->GetValue();
    UpdateScene(META_NS::TimeSpan::Milliseconds(50));
    WaitForUserQueue();
    auto p2 = anim->Progress()->GetValue();
    EXPECT_LT(p1, p2);
    UpdateScene(META_NS::TimeSpan::Milliseconds(50));
    WaitForUserQueue();
    auto p3 = anim->Progress()->GetValue();
    EXPECT_LT(p2, p3);
}

/**
 * @tc.name: OnFinished
 * @tc.desc: Tests for On Finished. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginAnimationTest, OnFinished, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);
    auto anims = scene->GetAnimations().GetResult();
    ASSERT_TRUE(!anims.empty());
    auto anim = anims[0];
    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);

    std::atomic<bool> finished = false;

    anim->OnFinished()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([&] { finished = true; }));

    startable->Start();

    // Run 100 frames (25ms each), the animation should finish during this time
    for (int i = 0; i < 100; i++) {
        UpdateScene(META_NS::TimeSpan::Milliseconds(25));
    }

    EXPECT_TRUE(finished);

    finished = false;
    startable->Restart();
    UpdateScene(META_NS::TimeSpan::Milliseconds(15));
    startable->Finish();
    UpdateScene();
    EXPECT_TRUE(finished);

    finished = false;
    startable->Restart();
    UpdateScene(META_NS::TimeSpan::Milliseconds(15));
    startable->Stop();
    UpdateScene();
    EXPECT_FALSE(finished);

    finished = false;
    startable->Restart();
    UpdateScene(META_NS::TimeSpan::Milliseconds(15));
    startable->Restart();
    UpdateScene();
    EXPECT_FALSE(finished);
}

/**
 * @tc.name: OnFinishedWithReverse
 * @tc.desc: Tests for On Finished With Reverse. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginAnimationTest, OnFinishedWithReverse, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);
    auto anims = scene->GetAnimations().GetResult();
    ASSERT_TRUE(!anims.empty());
    auto anim = anims[0];
    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);

    UpdateScene(META_NS::TimeSpan::Milliseconds(25));

    auto mod = META_NS::GetObjectRegistry().Create<META_NS::AnimationModifiers::ISpeed>(
        META_NS::ClassId::SpeedAnimationModifier);
    mod->SpeedFactor()->SetValue(-1);
    interface_cast<META_NS::IAttach>(anim)->Attach(mod);

    std::atomic<uint32_t> count {};

    anim->OnFinished()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([&] {
        ++count;
        CORE_LOG_W("Restarting");
        startable->Restart();
    }));
    CORE_LOG_W("Starting");
    startable->Start();

    for (int i = 0; i < 200; i++) {
        UpdateScene(META_NS::TimeSpan::Milliseconds(25));
    }

    CORE_LOG_W("Stopping");
    startable->Stop();
    EXPECT_EQ(count, 2);
}

TEST_F(API_ScenePluginAnimationTest, Remove)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);

    CORE_NS::ResourceId resource;
    META_NS::IAnimation::Ptr anim;
    {
        auto anims = scene->GetAnimations().GetResult();
        ASSERT_TRUE(!anims.empty());
        anim = anims[0];
        auto res = interface_pointer_cast<CORE_NS::IResource>(anim);
        ASSERT_TRUE(res);
        resource = res->GetResourceId();
        ASSERT_TRUE(resources->GetResource(resource));
    }
    scene->RemoveAnimation(anim).Wait();
    {
        auto anims = scene->GetAnimations().GetResult();
        ASSERT_TRUE(anims.empty());
        ASSERT_FALSE(resources->GetResource(resource));
    }
}

TEST_F(API_ScenePluginAnimationTest, Destroy)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);

    CORE_NS::ResourceId resource;
    {
        // set to loop more to test the destruction timing
        for (size_t i = 0; i != 50; ++i) {
            auto anims = scene->GetAnimations().GetResult();
            ASSERT_TRUE(!anims.empty());
            auto anim = anims[0];
            auto speedModifier = META_NS::GetObjectRegistry().Create<META_NS::AnimationModifiers::ISpeed>(
                META_NS::ClassId::SpeedAnimationModifier);
            interface_cast<META_NS::IAttach>(anim)->Attach(speedModifier);
            interface_cast<META_NS::IStartableAnimation>(anim)->Start();
            for (size_t c = 0; c != 10; ++c) {
                scene->GetInternalScene()->RunDirectlyOrInTask([&] {
                    scene->GetInternalScene()->Update(true);
                    context->GetRenderer()->GetRenderer().RenderDeferredFrame();
                });
            }
            resources->RemoveAllResources();
            interface_cast<META_NS::IAttach>(anim)->Detach(speedModifier);
        }
    }
    UpdateScene();
}

/**
 * @tc.name: Disabled
 * @tc.desc: Test GLTF imported animation state changes when the animation is disabled.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginAnimationTest, Pause, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://celia/Celia.gltf");
    ASSERT_TRUE(scene);
    auto anims = scene->GetAnimations().GetResult();
    ASSERT_TRUE(!anims.empty());
    auto anim = anims[0];
    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);

    startable->Start();
    EXPECT_EQ(META_NS::GetValue(anim->Running()), true);
    startable->Pause();
    EXPECT_EQ(META_NS::GetValue(anim->Running()), false);
    startable->Start();
    EXPECT_EQ(META_NS::GetValue(anim->Running()), true);
}

/**
 * @tc.name: Disabled
 * @tc.desc: Test GLTF imported animation state changes when the animation is disabled.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginAnimationTest, Disabled, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);
    auto anims = scene->GetAnimations().GetResult();
    ASSERT_TRUE(!anims.empty());
    auto anim = anims[0];
    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);

    auto getRunning = [&]() -> bool { return META_NS::GetValue(anim->Running()); };

    META_NS::SetValue(anim->Enabled(), false);
    META_NS::SetValue(anim->Enabled(), true);
    META_NS::SetValue(anim->Enabled(), false);
    startable->Start();
    // because of desicions made in OH 5.1, disabling an animation does not affect
    // "Running" property
    EXPECT_TRUE(getRunning());
    startable->Pause();
    EXPECT_TRUE(getRunning());
    startable->Restart();
    EXPECT_TRUE(getRunning());
    startable->Seek(0.5);
    EXPECT_TRUE(getRunning());
    startable->Stop();
    EXPECT_TRUE(getRunning());
    startable->Finish();
    // Due to legacy functionality running state is not affected when animation is disabled
    EXPECT_TRUE(getRunning());
}

/**
 * @tc.name: Loop
 * @tc.desc: Test loop for GLTF loaded animation
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginAnimationTest, Loop, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);
    auto anims = scene->GetAnimations().GetResult();
    ASSERT_TRUE(!anims.empty());
    auto anim = anims[0];
    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);

    auto loop = META_NS::AnimationModifiers::Loop(META_NS::CreateNew);
    META_NS::Animation animation(anim);
    ASSERT_TRUE(animation);
    EXPECT_TRUE(animation.AddModifier(loop));
    loop.SetLoopCount(10);
    UpdateScene();
    EXPECT_EQ(loop.GetLoopCount(), 10);
    loop.LoopIndefinitely();
    UpdateScene();
    EXPECT_EQ(loop.GetLoopCount(), -1);
    META_NS::AttachmentContainer(animation).Detach(loop);
}

/**
 * @tc.name: Attach
 * @tc.desc: Test attach/detach to an object.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginAnimationTest, Attach, testing::ext::TestSize.Level1)
{
    META_NS::Object o(META_NS::CreateNew);
    META_NS::AttachmentContainer a(o);
    ASSERT_TRUE(a);

    auto animation = META_NS::GetObjectRegistry().Create<META_NS::IAnimation>(GetAnimationClassId());
    ASSERT_TRUE(animation);

    EXPECT_TRUE(a.Attach(animation));
    EXPECT_TRUE(a.Detach(animation));
}

/**
 * @tc.name: InvalidInit
 * @tc.desc: Test invalid case EcsAnimation initialization.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginAnimationTest, InvalidInit, testing::ext::TestSize.Level1)
{
    auto animation = META_NS::GetObjectRegistry().Create<META_NS::IAnimation>(GetAnimationClassId());
    ASSERT_TRUE(animation);
    auto startable = interface_cast<META_NS::IStartableAnimation>(animation);
    ASSERT_TRUE(startable);

    auto ecso = interface_cast<IEcsObjectAccess>(animation);
    ASSERT_TRUE(ecso);
    EXPECT_FALSE(ecso->SetEcsObject({}));
    META_NS::SetValue(animation->Enabled(), true);
    startable->Seek(0.5);
}
} // namespace UTest

SCENE_END_NAMESPACE()
