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

#include <test_framework.h>

#include <meta/api/animation.h>
#include <meta/api/property/property_event_handler.h>
#include <meta/api/util.h>
#include <meta/base/namespace.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation_controller.h>
#include <meta/interface/property/construct_property.h>

#include "helpers/animation_test_base.h"
#include "helpers/serialisation_utils.h"
#include "helpers/testing_objects.h"

META_BEGIN_NAMESPACE()

namespace UTest {

class API_AnimationControllerTest : public API_AnimationTestBase {
protected:
    void SetUp() override
    {
        API_AnimationTestBase::SetUp();

        property_ = META_NS::ConstructProperty<float>("prop", 0.f);
        sutAnimationCtrl_ = META_NS::GetAnimationController();
        ASSERT_THAT(sutAnimationCtrl_, testing::NotNull());
    }

    void TearDown() override
    {
        auto count = GetValue(sutAnimationCtrl_->Count());

        sutAnimationCtrl_->Clear();

        count = GetValue(sutAnimationCtrl_->Count());

        ASSERT_EQ(sutAnimationCtrl_->GetRunning().size(), 0);
        API_AnimationTestBase::TearDown();
    }

    META_NS::KeyframeAnimation<float> CreateAnimation()
    {
        auto animation = META_NS::KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation));
        animation.SetProperty(property_);
        animation.SetFrom(0.f);
        animation.SetTo(10.f);
        animation.SetDuration(ANIMATION_LENGTH);
        animation.SetController(sutAnimationCtrl_);
        return animation;
    }

    static constexpr auto ANIMATION_LENGTH = TimeSpan::Milliseconds(100);

    META_NS::Property<float> property_;
    META_NS::IAnimationController::Ptr sutAnimationCtrl_;
};

/**
 * @tc.name: RunningAnimationListContainsStartedAnimations
 * @tc.desc: Tests for Running Animation List Contains Started Animations. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AnimationControllerTest, RunningAnimationListContainsStartedAnimations, testing::ext::TestSize.Level1)
{
    // given
    auto anim = CreateAnimation();

    // when
    anim.Start();

    // expected
    ASSERT_EQ(anim.GetRunning(), true);
    const auto runningAnimations = sutAnimationCtrl_->GetRunning();
    ASSERT_THAT(runningAnimations.size(), testing::Eq(1));
    EXPECT_THAT(runningAnimations[0].lock(), testing::Eq(anim));
}

/**
 * @tc.name: RunningAnimationListIsEmptyWhenAllAnimationsWereStopped
 * @tc.desc: Tests for Running Animation List Is Empty When All Animations Were Stopped. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(
    API_AnimationControllerTest, RunningAnimationListIsEmptyWhenAllAnimationsWereStopped, testing::ext::TestSize.Level1)
{
    // given
    auto anim = CreateAnimation();

    // when
    anim.Start();
    anim.Stop();

    // expected
    ASSERT_EQ(anim.GetRunning(), false);
    EXPECT_TRUE(sutAnimationCtrl_->GetRunning().empty());
}

/**
 * @tc.name: RunningAnimationListContainsResumedAnimations
 * @tc.desc: Tests for Running Animation List Contains Resumed Animations. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AnimationControllerTest, RunningAnimationListContainsResumedAnimations, testing::ext::TestSize.Level1)
{
    // given
    auto anim = CreateAnimation();

    // when
    anim.Start();
    anim.Pause();
    anim.Start();

    // expected
    ASSERT_EQ(anim.GetRunning(), true);
    const auto runningAnimations = sutAnimationCtrl_->GetRunning();
    ASSERT_THAT(runningAnimations.size(), testing::Eq(1));
    EXPECT_THAT(runningAnimations[0].lock(), testing::Eq(anim));
}

/**
 * @tc.name: RunningAnimationListIsEmptyWhenAllAnimationsWereFinished
 * @tc.desc: Tests for Running Animation List Is Empty When All Animations Were Finished. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AnimationControllerTest, RunningAnimationListIsEmptyWhenAllAnimationsWereFinished,
    testing::ext::TestSize.Level1)
{
    // given
    auto anim = CreateAnimation();

    // when
    anim.Start();
    StepAnimationController(0);
    StepAnimationController(ANIMATION_LENGTH.ToMilliseconds());

    // expected
    ASSERT_EQ(anim.GetRunning(), false);
    ASSERT_EQ(anim.GetProgress(), 1.f);
    EXPECT_TRUE(sutAnimationCtrl_->GetRunning().empty());
}

/**
 * @tc.name: AnimationControllerUpdatesControllerPropOfAddedAnimation
 * @tc.desc: Tests for Animation Controller Updates Controller Prop Of Added Animation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AnimationControllerTest, AnimationControllerUpdatesControllerPropOfAddedAnimation,
    testing::ext::TestSize.Level1)
{
    // given
    auto anim = CreateAnimation();
    anim.SetController({});
    ASSERT_TRUE(anim.GetController().expired());

    // when
    sutAnimationCtrl_->AddAnimation(anim);

    // expected
    ASSERT_FALSE(anim.GetController().expired());
    EXPECT_THAT(anim.GetController().lock(), testing::Eq(sutAnimationCtrl_));
}

/**
 * @tc.name: Serialization
 * @tc.desc: Tests for Serialization. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_AnimationControllerTest, Serialization, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    static constexpr auto ImplicitPropertyName = "ImplicitProperty";
    static constexpr auto KeyframePropertyName = "KeyframeProperty";

    {
        auto controller = GetObjectRegistry().Create<IAnimationController>(META_NS::ClassId::AnimationController);

        auto implicitProp = ConstructProperty<float>(ImplicitPropertyName, 0);
        auto keyframeProp = ConstructProperty<float>(KeyframePropertyName, 0);

        Object object(CreateInstance(ClassId::Object));
        Metadata(object).AddProperty(implicitProp);
        Metadata(object).AddProperty(keyframeProp);

        auto implicitAnim = PropertyAnimation(CreateInstance(ClassId::PropertyAnimation))
                                .SetProperty(implicitProp)
                                .SetController(controller);
        auto keyframeAnim = SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
        keyframeAnim.SetController(controller);
        keyframeAnim.Add(KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation))
                             .SetFrom(0)
                             .SetTo(10)
                             .SetProperty(keyframeProp)
                             .SetDuration(TimeSpan::Milliseconds(100)));
        keyframeAnim.Add(KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation))
                             .SetFrom(10)
                             .SetTo(0)
                             .SetProperty(keyframeProp)
                             .SetDuration(TimeSpan::Milliseconds(100)));

        AttachmentContainer(object).Attach(implicitAnim);
        AttachmentContainer(object).Attach(keyframeAnim);
        AttachmentContainer(object).Attach(controller);

        ASSERT_TRUE(ser.Export(object));
        ser.Dump("app://animation_ser.ui");
    }

    auto obj = ser.Import();
    ASSERT_TRUE(obj);
    Object imported(obj);
    ASSERT_TRUE(imported);

    auto controllers =
        AttachmentContainer(imported).GetAttachments({ TypeId(META_NS::InterfaceId::IAnimationController) });
    ASSERT_EQ(controllers.size(), 1);

    auto importedController = interface_pointer_cast<IAnimationController>(controllers[0]);
    EXPECT_EQ(importedController->Count()->GetValue(), 2);

    auto animations = importedController->GetAnimations();
    ASSERT_EQ(animations.size(), 2);

    for (const auto& anim : animations) {
        auto animation = anim.lock();
        ASSERT_TRUE(animation);
        auto controller = animation->Controller()->GetValue().lock();
        EXPECT_EQ(controller, importedController);
    }

    // Check that we got the same animation hierarchy back
    PropertyAnimation implicitAnim(animations[0].lock());
    SequentialAnimation seqAnim(animations[1].lock());
    ASSERT_TRUE(implicitAnim);
    ASSERT_TRUE(seqAnim);
    auto children = seqAnim.GetAnimations();
    ASSERT_EQ(children.size(), 2);
    KeyframeAnimation<float> segment1(children[0]);
    ASSERT_TRUE(segment1);
    EXPECT_TRUE(segment1.GetValid());

    // Start the sequential animation and check that it works
    seqAnim.Start();
    importedController->Step(GetTestClock());

    EXPECT_TRUE(seqAnim.GetRunning());
    EXPECT_EQ(seqAnim.GetProgress(), 0);
    auto progress = seqAnim.GetProgress();

    for (auto i = 0; i < 5; i++) {
        IncrementClockTime(TimeSpan::Milliseconds(10));
        importedController->Step(GetTestClock());
        auto current = seqAnim.GetProgress();
        EXPECT_GT(current, progress);
        progress = current;
    }
}

} // namespace UTest

META_END_NAMESPACE()
