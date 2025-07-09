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
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/api/animation.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/property/property.h>

#include "TestRunner.h"
#include "helpers/animation_test_base.h"
#include "helpers/serialisation_utils.h"
#include "helpers/test_utils.h"
#include "helpers/testing_objects.h"

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

class AnimationModifierTestBase;

namespace {
struct TestDefinition {
    std::string name;
    std::function<IAnimation::Ptr(AnimationModifierTestBase&)> constructor;
    std::function<IAnimation::Ptr(AnimationModifierTestBase& test, IAnimation::Ptr)> transform = [](auto&, auto p) {
        return p;
    };
};

} // namespace

class AnimationModifierTestBase : public AnimationTestBase,
                                      public ::testing::WithParamInterface<std::tuple<TestDefinition, int>> {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
    void SetUp() override
    {
        AnimationTestBase::SetUp();
    }

    void TearDown() override
    {
        keepAliveProperties.clear();
        AnimationTestBase::TearDown();
    }

    IAnimation::Ptr GetAnimation()
    {
        return std::get<0>(GetParam()).constructor(*this);
    }

    IAnimation::Ptr Transform(IAnimation::Ptr p)
    {
        return std::get<0>(GetParam()).transform(*this, p);
    }

    BASE_NS::vector<META_NS::IProperty::Ptr> keepAliveProperties;
};

/**
 * @tc.name: ReverseImplicitAnimation
 * @tc.desc: test ReverseImplicitAnimation
 * @tc.type: FUNC
 */
HWTEST_F(AnimationModifierTestBase, ReverseImplicitAnimation, TestSize.Level1)
{
    auto animation = META_NS::PropertyAnimation(CreateInstance(ClassId::PropertyAnimation));
    constexpr auto start = 10.0f;
    constexpr auto to = 20.0f;

    auto property = META_NS::ConstructProperty<float>("test", start);

    animation.SetProperty(property);
    animation.SetDuration(META_NS::TimeSpan::Milliseconds(100));
    animation.AddModifier(META_NS::AnimationModifiers::Reverse(CreateInstance(ClassId::ReverseAnimationModifier)));

    property->SetValue(to);

    EXPECT_TRUE(animation.GetRunning());
    animation.Step(GetTestClock());

    bool finished = false;
    animation.OnFinished()->AddHandler(MakeCallback<IOnChanged>([&] { finished = true; }));

    StepAnimations({ animation }, 10, [&](uint32_t frame) {
        auto progress = animation.GetProgress();
        EXPECT_EQ(progress, frame / 10.f) << "Frame: " << frame;

        auto value = GetValue(property);
        EXPECT_EQ(value, start + (1.f - progress) * (to - start)) << "Frame: " << frame;
    });

    EXPECT_FALSE(animation.GetRunning());
    EXPECT_EQ(animation.GetProgress(), 1.f);
    EXPECT_TRUE(finished);
}

class LoopAnimationModifierTest : public AnimationModifierTestBase {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
    int GetLoopCount()
    {
        return std::get<1>(GetParam());
    }
};

class SpeedAnimationModifierTest : public AnimationModifierTestBase {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
    float GetSpeedFactor()
    {
        return std::get<1>(GetParam()) / 10.f;
    }
};

static void StartOrSetPropertyValue(IAnimation::Ptr anim)
{
    if (auto startable = interface_cast<IStartableAnimation>(anim)) {
        startable->Start();
    } else if (auto p = interface_cast<IPropertyAnimation>(anim)) {
        Property<float> prop(p->Property()->GetValue().lock());
        if (prop) {
            prop->SetValue(20.0f);
        }
    }
}

class ReverseAnimationModifierTest : public AnimationModifierTestBase {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
    int GetFrameCount() const
    {
        return std::get<1>(GetParam());
    }

    BASE_NS::vector<BASE_NS::vector<float>> CollectFrames(const META_NS::IAnimation::Ptr& anim)
    {
        const auto frames = GetFrameCount();
        const auto millisPerFrame = GetValue(anim->TotalDuration()).ToMilliseconds() / frames;
        BASE_NS::vector<BASE_NS::vector<float>> propsValues(keepAliveProperties.size(), {});

        auto recordFrame = [this](BASE_NS::vector<BASE_NS::vector<float>>& out) {
            int i = 0;
            for (const auto& prop : keepAliveProperties) {
                out[i++].push_back(GetValue<float>(prop));
            }
        };

        if (auto startable = interface_cast<IStartableAnimation>(anim)) {
            startable->Restart();
        }
        anim->Step(GetTestClock());
        recordFrame(propsValues);
        StepAnimations(
            { anim },
            frames,
            millisPerFrame,
            [&propsValues, recordFrame](int frame) {
                recordFrame(propsValues);
            });

        return propsValues;
    }
};

std::string BuildTestName(const testing::TestParamInfo<LoopAnimationModifierTest::ParamType>& info)
{
    return std::get<0>(info.param).name + "Animation" + std::to_string(info.index);
}

IAnimation::Ptr CreateKeyFrameAnimation(AnimationModifierTestBase& test)
{
    KeyframeAnimation<float> animation(CreateInstance(ClassId::KeyframeAnimation));

    auto start = 10.0f;
    float to = 20.0f;

    auto property = ConstructProperty<float>("test", 5.0f);
    test.keepAliveProperties.push_back(property);

    animation.SetFrom(start);
    animation.SetTo(to);
    animation.SetDuration(TimeSpan::Milliseconds(1000)); // 1000: param
    animation.SetProperty(property);

    return animation;
}

IAnimation::Ptr CreatePropertyAnimation(AnimationModifierTestBase& test)
{
    auto animation = PropertyAnimation(CreateInstance(ClassId::PropertyAnimation));
    auto start = 10.0f;
    float to = 20.0f;

    auto property = ConstructProperty<float>("test", start);
    test.keepAliveProperties.push_back(property);

    animation.SetProperty(property);
    animation.SetDuration(TimeSpan::Milliseconds(1000)); // 1000: param

    return animation;
}

IAnimation::Ptr CreateTrackAnimation(AnimationModifierTestBase& test)
{
    BASE_NS::vector<float> timestamps = { 0.1f, 0.3f, 0.8f, 1.f };
    BASE_NS::vector<float> keyframes = { 50.0f, 100.0f, 170.0f, 190.0f };
    const float initialValue = 10.0f;

    auto property = ConstructProperty<float>("Prop", initialValue);
    test.keepAliveProperties.push_back(property);

    BASE_NS::array_view<float> kf = keyframes;

    auto animation = TrackAnimation<float>(CreateInstance(ClassId::TrackAnimation))
                         .SetKeyframes(keyframes)
                         .SetTimestamps(timestamps)
                         .SetProperty(property)
                         .SetDuration(TimeSpan::Milliseconds(1000));

    return animation;
}

IAnimation::Ptr CreateParallelAnimation(AnimationModifierTestBase& test)
{
    auto anim1 = CreateKeyFrameAnimation(test);
    auto anim2 = CreateTrackAnimation(test);

    auto staggered = ParallelAnimation(CreateInstance(ClassId::ParallelAnimation)).Add(anim1).Add(anim2);

    return staggered;
}

IAnimation::Ptr CreateSequentialAnimation(AnimationModifierTestBase& test)
{
    auto anim1 = CreateKeyFrameAnimation(test);
    auto anim2 = CreateTrackAnimation(test);

    auto staggered = SequentialAnimation(CreateInstance(ClassId::SequentialAnimation)).Add(anim1).Add(anim2);

    return staggered;
}

IAnimation::Ptr Serialize(AnimationModifierTestBase& test, IAnimation::Ptr anim)
{
    TestSerialiser ser;

    {
        Object obj(CreateInstance(ClassId::Object));
        AttachmentContainer(obj).Attach(interface_pointer_cast<IAttachment>(anim));

        for (auto&& v : test.keepAliveProperties) {
            Metadata(obj).AddProperty(v);
        }

        ser.Export(obj);
    }
    test.keepAliveProperties.clear();

    auto meta = ser.Import<IMetadata>();
    for (auto& p : meta->GetProperties()) {
        test.keepAliveProperties.push_back(p);
    }
    if (auto att = interface_pointer_cast<IAttach>(meta)) {
        auto vec = att->GetAttachments<IAnimation>();
        return vec.empty() ? nullptr : vec[0];
    }
    return nullptr;
}

/**
 * @tc.name: ReversedAnimationIsPlayedBackward
 * @tc.desc: test ReversedAnimationIsPlayedBackward
 * @tc.type: FUNC
 */
HWTEST_P(ReverseAnimationModifierTest, ReversedAnimationIsPlayedBackward, TestSize.Level1)
{
    using ::testing::FloatNear;
    using ::testing::Pointwise;

    auto anim = GetAnimation();
    const auto forwardFrames = CollectFrames(anim);
    {
        auto reverse = AnimationModifiers::Reverse(CreateInstance(ClassId::ReverseAnimationModifier));
        auto attach = interface_pointer_cast<IAttach>(anim);
        attach->Attach(interface_pointer_cast<IObject>(reverse));
    }
    anim = Transform(anim);
    ASSERT_TRUE(anim);

    const auto reverseFrames = CollectFrames(anim);

    for (decltype(forwardFrames.size()) i = 0, end = forwardFrames.size(); i < end; ++i) {
        EXPECT_THAT(
            reverseFrames[i], Pointwise(FloatNear(0.001),
                                  decltype(forwardFrames[i]) { forwardFrames[i].rbegin(), forwardFrames[i].rend() }))
            << "i: " << i;
    }
}

static IObject::Ptr GetAnimAttachment(IAnimation::Ptr anim)
{
    auto attach = interface_pointer_cast<IAttach>(anim);
    auto vec = attach->GetAttachments<IAnimationModifier>();
    return vec.empty() ? nullptr : interface_pointer_cast<IObject>(vec[0]);
}

/**
 * @tc.name: LoopCountModifiesDuration
 * @tc.desc: test LoopCountModifiesDuration
 * @tc.type: FUNC
 */
HWTEST_P(LoopAnimationModifierTest, LoopCountModifiesDuration, TestSize.Level1)
{
    auto anim = GetAnimation();
    int loopCount = GetLoopCount();
    auto originalDuration = GetValue(anim->TotalDuration());

    {
        auto loop = AnimationModifiers::Loop(CreateInstance(ClassId::LoopAnimationModifier)).SetLoopCount(loopCount);
        auto attach = interface_pointer_cast<IAttach>(anim);
        EXPECT_TRUE(attach->Attach(interface_pointer_cast<IObject>(loop)));
    }

    anim = Transform(anim);
    ASSERT_TRUE(anim);
    AnimationModifiers::Loop loop(GetAnimAttachment(anim));
    ASSERT_TRUE(loop);

    EXPECT_EQ(GetValue(anim->TotalDuration()), loopCount * originalDuration);
    loop.SetLoopCount(loopCount * 2);
    EXPECT_EQ(GetValue(anim->TotalDuration()), loopCount * 2 * originalDuration);
    loop.SetLoopCount(loopCount);
    EXPECT_EQ(GetValue(anim->TotalDuration()), loopCount * originalDuration);

    loop.SetLoopCount(-1);
    EXPECT_EQ(GetValue(anim->TotalDuration()), TimeSpan::Infinite());
}

/**
 * @tc.name: AnimationIsLoopingRegardlessOfSpeedModificator
 * @tc.desc: test AnimationIsLoopingRegardlessOfSpeedModificator
 * @tc.type: FUNC
 */
HWTEST_P(LoopAnimationModifierTest, AnimationIsLoopingRegardlessOfSpeedModificator, TestSize.Level1)
{
    static constexpr int64_t frameStepMs = 10;
    static constexpr float speedFactor = 2.7;
    const auto expectedLoops = GetLoopCount();
    auto anim = GetAnimation();
    const auto animPlayTime = (static_cast<float>(GetValue(anim->TotalDuration()).ToMilliseconds()) / speedFactor) *
                              static_cast<float>(expectedLoops);
    const auto animTotalFrames = static_cast<int64_t>(animPlayTime) / frameStepMs;

    {
        const auto attach = interface_pointer_cast<META_NS::IAttach>(anim);
        attach->Attach(META_NS::AnimationModifiers::Loop(CreateNew).LoopIndefinitely().GetPtr());
        attach->Attach(META_NS::AnimationModifiers::Speed(CreateNew).SetSpeedFactor(speedFactor).GetPtr());
    }
    anim = Transform(anim);
    ASSERT_TRUE(anim);

    uint32_t actualLoops = 0;
    auto previousProgress = GetValue(anim->Progress());
    StartOrSetPropertyValue(anim);
    StepAnimations({ anim }, animTotalFrames, frameStepMs, [&](uint32_t frame) {
        actualLoops = actualLoops == 0 ? 1 : actualLoops;
        const auto currentProgress = GetValue(anim->Progress());
        if (previousProgress > currentProgress) {
            ++actualLoops;
        }
        previousProgress = currentProgress;
    });

    EXPECT_EQ(actualLoops, expectedLoops);
}

/**
 * @tc.name: AnimationIslooping
 * @tc.desc: test AnimationIslooping
 * @tc.type: FUNC
 */
HWTEST_P(LoopAnimationModifierTest, AnimationIslooping, TestSize.Level1)
{
    auto anim = GetAnimation();
    int loopCount = GetLoopCount();
    auto originalDuration = GetValue(anim->TotalDuration());

    // Don't test zero loopcount
    if (loopCount == 0) {
        return;
    }

    // round up
    const int frames = (originalDuration.ToMilliseconds() + 9) / 10;
    const int totalFrames = frames * loopCount;

    EXPECT_TRUE(GetValue(anim->Valid()));
    {
        auto loop =
            META_NS::AnimationModifiers::Loop(CreateInstance(ClassId::LoopAnimationModifier)).SetLoopCount(loopCount);
        auto attach = interface_pointer_cast<META_NS::IAttach>(anim);
        attach->Attach(loop.GetPtr());
    }

    anim = Transform(anim);
    ASSERT_TRUE(anim);

    EXPECT_TRUE(GetValue(anim->Valid()));

    StartOrSetPropertyValue(anim);
    anim->Step(GetTestClock());

    EXPECT_TRUE(GetValue(anim->Valid()));
    EXPECT_TRUE(GetValue(anim->Running()));

    for (int i = 0; i < loopCount; ++i) {
        const int frameOffset = i * 10;
        EXPECT_TRUE(GetValue(anim->Running()));
        auto previousProgress = 0.f;

        StepAnimations({ anim }, frames, [&](int frame) {
            frame = i * frames + frame;
            auto progress = GetValue(anim->Progress());
            if (i == (loopCount - 1) && frame == totalFrames) {
                // Last frame
                ASSERT_FALSE(GetValue(anim->Running())) << "Frame " << frame;
            } else {
                ASSERT_TRUE(GetValue(anim->Running())) << "Frame " << frame;
                ASSERT_GT(progress, previousProgress) << "Frame " << frame;
            }

            previousProgress = progress;
        });
    }

    ASSERT_FALSE(GetValue(anim->Running()));

    // Animation is stopped, it should not advance any further
    for (int i = 0; i < 10; ++i) {
        IncrementClockTime(META_NS::TimeSpan::Milliseconds(10));
        anim->Step(GetTestClock());

        EXPECT_FALSE(GetValue(anim->Running()));
        EXPECT_FLOAT_EQ(1.0f, GetValue(anim->Progress()));
    }
}

/**
 * @tc.name: DuplicateProperties
 * @tc.desc: test DuplicateProperties
 * @tc.type: FUNC
 */
HWTEST_P(LoopAnimationModifierTest, DuplicateProperties, TestSize.Level1)
{
    auto animation = GetAnimation();
    animation = Transform(animation);

    auto meta = interface_pointer_cast<META_NS::IMetadata>(animation);

    for (const auto& prop : meta->GetProperties()) {
        for (const auto& rhs : meta->GetProperties()) {
            if (prop == rhs) {
                continue;
            }

            ASSERT_NE(prop->GetName(), rhs->GetName());
        }
    }
}

/**
 * @tc.name: SpeedFactorModifiesDuration
 * @tc.desc: test SpeedFactorModifiesDuration
 * @tc.type: FUNC
 */
HWTEST_P(SpeedAnimationModifierTest, SpeedFactorModifiesDuration, TestSize.Level1)
{
    auto anim = GetAnimation();
    float speedFactor = GetSpeedFactor();
    auto originalDuration = GetValue(anim->TotalDuration());
    auto expectedDuration = originalDuration / BASE_NS::Math::abs(speedFactor);

    {
        auto speedModifier = META_NS::AnimationModifiers::Speed(CreateNew).SetSpeedFactor(speedFactor);
        auto attach = interface_pointer_cast<META_NS::IAttach>(anim);
        ASSERT_TRUE(attach->Attach(speedModifier.GetPtr()));
    }
    anim = Transform(anim);
    ASSERT_TRUE(anim);
    META_NS::AnimationModifiers::Speed speedModifier(GetAnimAttachment(anim));
    ASSERT_TRUE(speedModifier);

    EXPECT_TRUE(GetValue(anim->Valid()));

    EXPECT_EQ(GetValue(anim->TotalDuration()), expectedDuration);

    // Change speed factor, total duration should follow
    speedModifier.SetSpeedFactor(speedFactor * 2);
    EXPECT_EQ(GetValue(anim->TotalDuration()).ToMilliseconds(), (expectedDuration / 2).ToMilliseconds());
    speedModifier.SetSpeedFactor(speedFactor);
    EXPECT_EQ(GetValue(anim->TotalDuration()), expectedDuration);

    speedModifier.SetSpeedFactor(-speedFactor * 2);
    EXPECT_EQ(GetValue(anim->TotalDuration()).ToMilliseconds(), (expectedDuration / 2).ToMilliseconds());
    speedModifier.SetSpeedFactor(-speedFactor);
    EXPECT_EQ(GetValue(anim->TotalDuration()), expectedDuration);

    speedModifier.SetSpeedFactor(0);
    EXPECT_EQ(GetValue(anim->TotalDuration()), TimeSpan::Infinite());
}

/**
 * @tc.name: SpeedFactorAffectsPlayback
 * @tc.desc: test SpeedFactorAffectsPlayback
 * @tc.type: FUNC
 */
HWTEST_P(SpeedAnimationModifierTest, SpeedFactorAffectsPlayback, TestSize.Level1)
{
    auto anim = GetAnimation();
    const float speedFactor = GetSpeedFactor();
    if (speedFactor == 0.f) {
        return;
    }

    auto originalDuration = GetValue(anim->TotalDuration());
    auto expectedDuration = originalDuration / BASE_NS::Math::abs(speedFactor);

    {
        auto speedModifier = META_NS::AnimationModifiers::Speed(CreateInstance(ClassId::SpeedAnimationModifier))
                                 .SetSpeedFactor(speedFactor);

        auto attach = interface_pointer_cast<META_NS::IAttach>(anim);
        ASSERT_NE(attach, nullptr);
        attach->Attach(speedModifier.GetPtr());
    }
    anim = Transform(anim);
    ASSERT_TRUE(anim);
    META_NS::AnimationModifiers::Speed speedModifier(GetAnimAttachment(anim));
    ASSERT_TRUE(speedModifier);

    EXPECT_TRUE(GetValue(anim->Valid()));

    StartOrSetPropertyValue(anim);

    EXPECT_TRUE(GetValue(anim->Valid()));
    EXPECT_TRUE(GetValue(anim->Running()));
    anim->Step(GetTestClock());

    if (speedFactor == 0.f) {
        EXPECT_EQ(GetValue(anim->TotalDuration()), META_NS::TimeSpan::Infinite());
        EXPECT_EQ(GetValue(anim->Progress()), 0);
        return;
    }
    // Figure out how many milliseconds we must progress at each frame
    // to cover the whole animation in 10 frames
    int64_t millisPerFrame =
        BASE_NS::Math::round(static_cast<float>(GetValue(anim->TotalDuration()).ToMilliseconds()) / 10.f);
    EXPECT_EQ(millisPerFrame, (expectedDuration / 10).ToMilliseconds());

    float previousProgress = GetValue(anim->Progress());
    EXPECT_EQ(previousProgress, 0);

    StepAnimations({ anim }, 10, millisPerFrame, [millisPerFrame, &anim, &previousProgress](int frame) {
        float progress = GetValue(anim->Progress());
        EXPECT_GT(progress, previousProgress);
        EXPECT_NEAR(progress, frame / 10.f, 0.05);
        previousProgress = progress;
    });

    // Animation is stopped, it should not advance any further
    StepAnimations({ anim }, 10, millisPerFrame, [millisPerFrame, &anim, &previousProgress](int frame) {
        EXPECT_FALSE(GetValue(anim->Running()));
        EXPECT_FLOAT_EQ(1.0f, GetValue(anim->Progress()));
    });
}

INSTANTIATE_TEST_SUITE_P(LoopAnimationModifierSuite, LoopAnimationModifierTest,
    testing::Combine(
        testing::Values(TestDefinition { "Keyframe", CreateKeyFrameAnimation },
            TestDefinition { "Track", CreateTrackAnimation }, TestDefinition { "Parallel", CreateParallelAnimation },
            TestDefinition { "Sequential", CreateSequentialAnimation },
            TestDefinition { "Property", CreatePropertyAnimation },
            TestDefinition { "KeyframeSerialize", CreateKeyFrameAnimation, Serialize },
            TestDefinition { "TrackSerialize", CreateTrackAnimation, Serialize },
            TestDefinition { "ParallelSerialize", CreateParallelAnimation, Serialize },
            TestDefinition { "SequentialSerialize", CreateSequentialAnimation, Serialize },
            TestDefinition { "PropertySerialize", CreatePropertyAnimation, Serialize }),
        testing::Range(1, 10)),
    BuildTestName);

INSTANTIATE_TEST_SUITE_P(SpeedAnimationModifierSuite, SpeedAnimationModifierTest,
    testing::Combine(
        testing::Values(TestDefinition { "Keyframe", CreateKeyFrameAnimation },
            TestDefinition { "Track", CreateTrackAnimation }, TestDefinition { "Parallel", CreateParallelAnimation },
            TestDefinition { "Sequential", CreateSequentialAnimation },
            TestDefinition { "Property", CreatePropertyAnimation },
            TestDefinition { "KeyframeSerialize", CreateKeyFrameAnimation, Serialize },
            TestDefinition { "TrackSerialize", CreateTrackAnimation, Serialize },
            TestDefinition { "ParallelSerialize", CreateParallelAnimation, Serialize },
            TestDefinition { "SequentialSerialize", CreateSequentialAnimation, Serialize },
            TestDefinition { "PropertySerialize", CreatePropertyAnimation, Serialize }),
        testing::Range(-20, 20, 5)),
    BuildTestName);

INSTANTIATE_TEST_SUITE_P(ReverseAnimationModifierSuite, ReverseAnimationModifierTest,
    testing::Combine(
        testing::Values(TestDefinition { "Keyframe", CreateKeyFrameAnimation },
            TestDefinition { "Track", CreateTrackAnimation }, TestDefinition { "Parallel", CreateParallelAnimation },
            TestDefinition { "KeyframeSerialize", CreateKeyFrameAnimation, Serialize },
            TestDefinition { "TrackSerialize", CreateTrackAnimation, Serialize },
            TestDefinition { "ParallelSerialize", CreateParallelAnimation, Serialize }),
        testing::Range(10, 20, 10)),
    BuildTestName);

META_END_NAMESPACE()
