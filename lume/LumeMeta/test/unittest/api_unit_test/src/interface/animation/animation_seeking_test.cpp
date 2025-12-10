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
#include <meta/api/object.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/property/construct_property.h>

#include "helpers/animation_test_base.h"
#include "helpers/testing_objects.h"

META_BEGIN_NAMESPACE()

namespace UTest {

namespace {

template<typename AnimationType>
Animation CreateAnimation(const IProperty::WeakPtr& property);

template<>
Animation CreateAnimation<SequentialAnimation>(const IProperty::WeakPtr& property)
{
    return SequentialAnimation(CreateInstance(ClassId::SequentialAnimation))
        .Add(KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation)));
}

template<>
Animation CreateAnimation<ParallelAnimation>(const IProperty::WeakPtr& property)
{
    return ParallelAnimation(CreateInstance(ClassId::ParallelAnimation))
        .Add(KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation)));
}

template<>
Animation CreateAnimation<KeyframeAnimation<float>>(const IProperty::WeakPtr& property)
{
    KeyframeAnimation<float> animation(CreateInstance(ClassId::KeyframeAnimation));
    animation.SetFrom(0.0F);
    animation.SetTo(1.0F);
    animation.SetDuration(TimeSpan::Milliseconds(1000));
    animation.SetProperty(property);
    return animation;
}

} // namespace

template<typename AnimationType>
class API_AnimationSeekingTest : public API_AnimationTestBase {
public:
    API_AnimationSeekingTest()
    {
        property_ = ConstructProperty("animatedProperty", 0.0F);
        startableInterface_ = interface_pointer_cast<IStartableAnimation>(CreateAnimation<AnimationType>(property_));
        animationInterface_ = interface_pointer_cast<IAnimation>(startableInterface_);
    }

protected:
    IStartableAnimation::Ptr startableInterface_;
    IAnimation::Ptr animationInterface_;
    Property<float> property_;
};

using TestedTypes = ::testing::Types<KeyframeAnimation<float>, SequentialAnimation, ParallelAnimation>;
TYPED_TEST_SUITE(API_AnimationSeekingTest, TestedTypes);

TYPED_TEST(API_AnimationSeekingTest, SeekingSequentialAnimationDoesNotCauseAnimationStart)
{
    // given
    auto invoked = false;
    this->animationInterface_->Running().GetProperty()->OnChanged()->AddHandler(
        MakeCallback<IOnChanged>([&] { invoked = true; }));

    // when
    this->startableInterface_->Seek(0.25F);

    // expected
    const auto isRunning = this->animationInterface_->Running()->GetValue();
    EXPECT_THAT(isRunning, testing::Eq(false));
    EXPECT_THAT(invoked, testing::Eq(false));
}

TYPED_TEST(API_AnimationSeekingTest, SeekingRunningSequentialAnimationDoesNotChangeState)
{
    // given
    this->startableInterface_->Start();

    auto invoked = false;
    this->animationInterface_->Running().GetProperty()->OnChanged()->AddHandler(
        MakeCallback<IOnChanged>([&] { invoked = true; }));

    // when
    this->startableInterface_->Seek(0.25F);

    // expected
    const auto isRunning = this->animationInterface_->Running()->GetValue();
    EXPECT_THAT(isRunning, testing::Eq(true));
    EXPECT_THAT(invoked, testing::Eq(false));
}

TYPED_TEST(API_AnimationSeekingTest, SeekingTo1000ProgressFinishesAnimation)
{
    // given
    this->startableInterface_->Start();

    auto animationRunningChangeInvoke = false;
    this->animationInterface_->Running().GetProperty()->OnChanged()->AddHandler(
        MakeCallback<IOnChanged>([&] { animationRunningChangeInvoke = true; }));

    auto onFinishedInvoked = false;
    this->animationInterface_->OnFinished()->AddHandler(MakeCallback<IOnChanged>([&] { onFinishedInvoked = true; }));

    // when
    this->startableInterface_->Seek(1.0F);

    // expected
    const auto isRunning = this->animationInterface_->Running()->GetValue();
    EXPECT_THAT(isRunning, testing::Eq(false));
    EXPECT_THAT(animationRunningChangeInvoke, testing::Eq(true));
    EXPECT_THAT(onFinishedInvoked, testing::Eq(true));
}

TYPED_TEST(API_AnimationSeekingTest, SeekingToBeginningDoesNotStopAnimation)
{
    // given
    this->startableInterface_->Start();
    this->Update(TimeSpan::Milliseconds(500));

    // when
    auto animationRunningChangeInvoke = false;
    this->animationInterface_->Running().GetProperty()->OnChanged()->AddHandler(
        MakeCallback<IOnChanged>([&] { animationRunningChangeInvoke = true; }));

    this->startableInterface_->Seek(0.0F);

    // expected
    auto propertyValue = this->property_->GetValue();

    EXPECT_THAT(propertyValue, testing::Eq(0.0F));

    const auto isRunning = this->animationInterface_->Running()->GetValue();
    EXPECT_THAT(isRunning, testing::Eq(true));
    EXPECT_THAT(animationRunningChangeInvoke, testing::Eq(false));
}

} // namespace UTest
META_END_NAMESPACE()
