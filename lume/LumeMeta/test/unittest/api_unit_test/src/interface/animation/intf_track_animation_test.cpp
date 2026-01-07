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
#include <meta/ext/object_fwd.h>
#include <meta/interface/property/property.h>

#include "helpers/animation_test_base.h"
#include "helpers/serialisation_utils.h"
#include "helpers/testing_objects.h"

META_BEGIN_NAMESPACE()

namespace UTest {

class ITestKeyframeHandler : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITestKeyframeHandler, "f1b45ddb-88ce-47b2-8317-21e545b9b4cd")
public:
    virtual int Called() const = 0;
    virtual void MyFunc() = 0;
};

META_REGISTER_CLASS(KeyframeHandler, "9d6ec02e-66f1-4d57-bf06-6e1494ccc06a", META_NS::ObjectCategoryBits::APPLICATION)

class KeyframeHandler final : public IntroduceInterfaces<META_NS::ObjectFwd, ITestKeyframeHandler> {
    META_OBJECT(KeyframeHandler, ClassId::KeyframeHandler, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_FUNCTION_DATA(ITestKeyframeHandler, MyFunc)
    META_END_STATIC_DATA()

    void MyFunc() override
    {
        ++count_;
    }

    int Called() const override
    {
        return count_;
    }

    int count_ {};
};

class API_TrackAnimationTest : public API_AnimationTestBase {
protected:
    void TestBindingAnimation(bool animateSource, bool useInBetweenProperty);
};

/**
 * @tc.name: TrackAnimation
 * @tc.desc: Tests for Track Animation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TrackAnimationTest, TrackAnimation, testing::ext::TestSize.Level1)
{
    RegisterObjectType<KeyframeHandler>();
    static BASE_NS::vector<float> timestamps = { 0.1f, 0.3f, 0.8f, 1.f };
    static BASE_NS::vector<BASE_NS::Math::Vec3> keyframes = { { 50, 30, 0 }, { 100, 80, 0 }, { 170, 150, 0 },
        { 500, 500, 0 } };
    static BASE_NS::Math::Vec3 initialValue = { 10, 10, 0 };

    BASE_NS::vector<ITestKeyframeHandler::Ptr> handlerObjects;
    BASE_NS::vector<IFunction::Ptr> handlers;
    for (int i = 0; i != timestamps.size(); ++i) {
        handlerObjects.push_back(META_NS::GetObjectRegistry().Create<ITestKeyframeHandler>(ClassId::KeyframeHandler));
        handlers.push_back(interface_cast<IMetadata>(handlerObjects.back())->GetFunction("MyFunc"));
    }

    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", initialValue);
    META_NS::TrackAnimation<BASE_NS::Math::Vec3> animation(CreateInstance(META_NS::ClassId::TrackAnimation));
    animation.SetKeyframes(keyframes)
        .SetTimestamps(timestamps)
        .SetKeyframeHandlers(handlers)
        .SetProperty(property)
        .SetDuration(TimeSpan::Milliseconds(1000));

    ASSERT_TRUE(animation.GetValid());

    ASSERT_EQ(TimeSpan::Seconds(1.0f), animation.GetTotalDuration());

    animation.Start();
    // First frame
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(animation.GetRunning());
    BASE_NS::Math::Vec3 previous = initialValue;

    uint32_t steps = 0;

    // Each step steps by 10ms, so 100 steps should cover the whole 1000ms animation
    this->StepAnimations({ animation }, 100, [&](uint32_t frame) {
        const auto progress = animation.GetProgress();
        const auto value = GetValue(property);
        if (progress < timestamps[0]) {
            // First timestamp at 0.1, before that just get the value
            // EXPECT_EQ(value, initialValue) << "Frame: " << frame;
            // First timestamp at 0.1, before that just get the first keyframe
            EXPECT_EQ(value, keyframes.front()) << "Frame: " << frame;
            EXPECT_EQ(!handlerObjects[0]->Called(), 1) << "Frame: " << frame;
            EXPECT_EQ(animation.GetCurrentKeyframeIndex(), -1) << "Frame: " << frame;
        } else {
            steps++;
            BASE_NS::Math::Vec3 expected;
            BASE_NS::Math::Vec3 expectedRangeStart;
            BASE_NS::Math::Vec3 expectedRangeEnd;
            if (progress >= timestamps[3]) {
                expectedRangeStart = keyframes[2];
                expectedRangeEnd = keyframes[3];
                EXPECT_EQ(animation.GetCurrentKeyframeIndex(), 3) << "Frame: " << frame;
            } else if (progress >= timestamps[2]) {
                expectedRangeStart = keyframes[2];
                expectedRangeEnd = keyframes[3];
                EXPECT_EQ(handlerObjects[2]->Called(), 1) << "Frame: " << frame;
                EXPECT_EQ(animation.GetCurrentKeyframeIndex(), 2) << "Frame: " << frame;
            } else if (progress >= timestamps[1]) {
                expectedRangeStart = keyframes[1];
                expectedRangeEnd = keyframes[2];
                EXPECT_EQ(handlerObjects[1]->Called(), 1) << "Frame: " << frame;
                EXPECT_EQ(animation.GetCurrentKeyframeIndex(), 1) << "Frame: " << frame;
            } else if (progress >= timestamps[0]) {
                expectedRangeStart = keyframes[0];
                expectedRangeEnd = keyframes[1];
                EXPECT_EQ(handlerObjects[0]->Called(), 1) << "Frame: " << frame;
                EXPECT_EQ(animation.GetCurrentKeyframeIndex(), 0) << "Frame: " << frame;
            } else {
                // Should not end up here
                FAIL(); // << "Frame: " << frame;
            }

            for (int i = 0; i < 3; ++i) {
                EXPECT_GE(value[i], expectedRangeStart[i]) << "Frame: " << frame;
                EXPECT_LE(value[i], expectedRangeEnd[i]) << "Frame: " << frame;
            }
            EXPECT_NE(previous, value) << "Frame: " << frame;
            previous = value;
        }
    });

    EXPECT_EQ(steps, (timestamps[3] - timestamps[0]) * 100 + 1);

    EXPECT_EQ(animation.GetProgress(), 1.f);
    EXPECT_EQ(animation.GetCurrentKeyframeIndex(), 3);
    EXPECT_EQ(GetValue(property), keyframes.back());
    EXPECT_FALSE(animation.GetRunning());

    animation.Stop();

    for (const auto& h : handlerObjects) {
        EXPECT_EQ(h->Called(), 1);
    }
    EXPECT_FALSE(animation.GetRunning());

    auto value = GetValue(property);

    animation.Start();
    animation.Step(GetTestClock());

    EXPECT_TRUE(animation.GetRunning());
    EXPECT_EQ(animation.GetProgress(), 0.f);
    EXPECT_EQ(value, GetValue(property)); // Value should not change as our first keyframe is at 0.1

    // Jump past the end
    IncrementClockTime(TimeSpan::Milliseconds(2000));
    animation.Step(GetTestClock());

    EXPECT_EQ(animation.GetProgress(), 1.f);
    EXPECT_EQ(keyframes.back(), GetValue(property));

    UnregisterObjectType<KeyframeHandler>();
}

static BASE_NS::vector<float> trackTimestamps = { 0.1f, 0.5f, 1.f };
static BASE_NS::vector<BASE_NS::Math::Vec3> trackKeyframes = { { 50, 30, 0 }, { 170, 150, 0 }, { 500, 500, 0 } };
static BASE_NS::Math::Vec3 trackInitialValue = { 10, 10, 0 };

/**
 * @tc.name: AddKeyframe
 * @tc.desc: Tests for Add Keyframe. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TrackAnimationTest, AddKeyframe, testing::ext::TestSize.Level1)
{
    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", trackInitialValue);
    auto added = ConstructProperty<BASE_NS::Math::Vec3>("Prop", { 100, 80, 0 });
    auto addedValue = added->GetValueAny().Clone(true);

    META_NS::TrackAnimation<BASE_NS::Math::Vec3> animation(CreateInstance(META_NS::ClassId::TrackAnimation));
    animation.SetKeyframes(trackKeyframes)
        .SetTimestamps(trackTimestamps)
        .SetProperty(property)
        .SetDuration(TimeSpan::Milliseconds(1000));

    ASSERT_TRUE(animation.GetValid());
    ASSERT_EQ(TimeSpan::Seconds(1.0f), animation.GetTotalDuration());

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(animation.GetRunning());

    StepAnimations({ animation }, 1, 200, [&animation](uint32_t frame) {
        EXPECT_EQ(animation.GetCurrentKeyframeIndex(), 0) << frame; // 1st keyframe
    });

    // Add a keyframe that is in the middle of the current and next keyframe
    EXPECT_EQ(animation.AddKeyframe(0.3, addedValue), 1);

    StepAnimations({ animation }, 1, 100, [&animation, &property, &added](uint32_t frame) {
        EXPECT_EQ(animation.GetCurrentKeyframeIndex(), 1); // 2nd keyframe that we just added
        EXPECT_EQ(GetValue(property), GetValue(added));
    });
}

/**
 * @tc.name: AddKeyframes
 * @tc.desc: Tests for Add Keyframes. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TrackAnimationTest, AddKeyframes, testing::ext::TestSize.Level1)
{
    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", trackInitialValue);
    auto added = ConstructProperty<BASE_NS::Math::Vec3>("Prop", { 100, 80, 0 });

    META_NS::TrackAnimation<BASE_NS::Math::Vec3> animation(CreateInstance(META_NS::ClassId::TrackAnimation));
    animation.SetProperty(property).SetDuration(TimeSpan::Milliseconds(1000));

    size_t keyframes = 0;
    for (int i = 0; i < trackTimestamps.size() - 1; i++) {
        added->SetValue(trackKeyframes[i]);
        auto addedValue = added->GetValueAny().Clone(true);
        EXPECT_EQ(animation.AddKeyframe(trackTimestamps[i], addedValue), i);
        EXPECT_EQ(animation.GetValid(), ++keyframes > 1);
    }

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(animation.GetRunning());

    added->SetValue(trackKeyframes[trackTimestamps.size() - 1]);
    auto addedValue = added->GetValueAny().Clone(true);
    EXPECT_EQ(animation.AddKeyframe(trackTimestamps[trackTimestamps.size() - 1], addedValue), 2);

    EXPECT_EQ(animation.Keyframes().size(), trackKeyframes.size());
    EXPECT_EQ(animation.GetTimestamps().size(), trackTimestamps.size());

    StepAnimations({ animation }, 3, 359, [&animation, &property, &added](uint32_t frame) {
        EXPECT_EQ(animation.GetCurrentKeyframeIndex(), frame - 1) << frame;
    });
}

/**
 * @tc.name: RemoveKeyframe
 * @tc.desc: Tests for Remove Keyframe. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TrackAnimationTest, RemoveKeyframe, testing::ext::TestSize.Level1)
{
    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", trackInitialValue);
    META_NS::TrackAnimation<BASE_NS::Math::Vec3> animation(CreateInstance(META_NS::ClassId::TrackAnimation));
    animation.SetKeyframes(trackKeyframes)
        .SetTimestamps(trackTimestamps)
        .SetProperty(property)
        .SetDuration(TimeSpan::Milliseconds(1000));

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(animation.GetRunning());

    StepAnimations({ animation }, 1, 300, [&animation](uint32_t frame) {
        EXPECT_EQ(animation.GetCurrentKeyframeIndex(), 0) << frame; // 1st keyframe
    });

    EXPECT_TRUE(animation.RemoveKeyframe(1));

    EXPECT_EQ(animation.Keyframes().size(), trackKeyframes.size() - 1);
    EXPECT_EQ(animation.GetTimestamps().size(), trackTimestamps.size() - 1);

    StepAnimations({ animation }, 1, 300, [&animation](uint32_t frame) {
        // Originally this would have moved to second keyframe, but since we removed it
        // we should still be in the first keyframe
        EXPECT_EQ(animation.GetCurrentKeyframeIndex(), 0) << frame;
    });
}

/**
 * @tc.name: RemoveAllKeyframes
 * @tc.desc: Tests for Remove All Keyframes. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TrackAnimationTest, RemoveAllKeyframes, testing::ext::TestSize.Level1)
{
    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", trackInitialValue);
    META_NS::TrackAnimation<BASE_NS::Math::Vec3> animation(CreateInstance(META_NS::ClassId::TrackAnimation));
    animation.SetKeyframes(trackKeyframes)
        .SetTimestamps(trackTimestamps)
        .SetProperty(property)
        .SetDuration(TimeSpan::Milliseconds(1000));

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(animation.GetRunning());

    animation.RemoveAllKeyframes();
    EXPECT_FALSE(animation.GetRunning());
    EXPECT_FALSE(animation.GetValid());
}

/**
 * @tc.name: HandlerTest
 * @tc.desc: Tests for Handler Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TrackAnimationTest, HandlerTest, testing::ext::TestSize.Level1)
{
    RegisterObjectType<KeyframeHandler>();
    static BASE_NS::vector<float> timestamps = { 0.0f, 0.3f, 0.8f, 1.f };
    static BASE_NS::vector<BASE_NS::Math::Vec3> keyframes = { { 50, 30, 0 }, { 100, 80, 0 }, { 170, 150, 0 },
        { 500, 500, 0 } };
    static BASE_NS::Math::Vec3 initialValue = { 10, 10, 0 };

    BASE_NS::vector<ITestKeyframeHandler::Ptr> handlerObjects;
    BASE_NS::vector<IFunction::Ptr> handlers;
    for (int i = 0; i != timestamps.size(); ++i) {
        handlerObjects.push_back(META_NS::GetObjectRegistry().Create<ITestKeyframeHandler>(ClassId::KeyframeHandler));
        handlers.push_back(interface_cast<IMetadata>(handlerObjects.back())->GetFunction("MyFunc"));
    }

    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", initialValue);

    META_NS::TrackAnimation<BASE_NS::Math::Vec3> animation(CreateInstance(META_NS::ClassId::TrackAnimation));
    animation.SetKeyframes(keyframes)
        .SetKeyframeHandlers(handlers)
        .SetTimestamps(timestamps)
        .SetProperty(property)
        .SetDuration(TimeSpan::Milliseconds(1000));

    ASSERT_TRUE(animation.GetValid());

    ASSERT_EQ(TimeSpan::Seconds(1.0f), animation.GetTotalDuration());

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(animation.GetRunning());
    BASE_NS::Math::Vec3 previous = initialValue;

    // Each step steps by 10ms, so 100 steps should cover the whole 1000ms animation
    this->StepAnimations({ animation }, 100, [&](uint32_t frame) {});

    EXPECT_EQ(animation.GetProgress(), 1.f);
    EXPECT_EQ(animation.GetCurrentKeyframeIndex(), 3);
    for (const auto& h : handlerObjects) {
        EXPECT_EQ(h->Called(), 1);
    }
    UnregisterObjectType<KeyframeHandler>();
}

/**
 * @tc.name: HandlerTest2
 * @tc.desc: Tests for Handler Test2. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TrackAnimationTest, HandlerTest2, testing::ext::TestSize.Level1)
{
    RegisterObjectType<KeyframeHandler>();
    static BASE_NS::vector<float> timestamps = { 0.3f, 0.4f, 0.6f, 0.7f };
    static BASE_NS::vector<BASE_NS::Math::Vec3> keyframes = { { 50, 30, 0 }, { 100, 80, 0 }, { 170, 150, 0 },
        { 500, 500, 0 } };
    static BASE_NS::Math::Vec3 initialValue = { 10, 10, 0 };

    BASE_NS::vector<ITestKeyframeHandler::Ptr> handlerObjects;
    BASE_NS::vector<IFunction::Ptr> handlers;
    for (int i = 0; i != timestamps.size(); ++i) {
        handlerObjects.push_back(META_NS::GetObjectRegistry().Create<ITestKeyframeHandler>(ClassId::KeyframeHandler));
        handlers.push_back(interface_cast<IMetadata>(handlerObjects.back())->GetFunction("MyFunc"));
    }

    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", initialValue);

    META_NS::TrackAnimation<BASE_NS::Math::Vec3> animation(CreateInstance(META_NS::ClassId::TrackAnimation));
    animation.SetKeyframes(keyframes)
        .SetTimestamps(timestamps)
        .SetKeyframeHandlers(handlers)
        .SetProperty(property)
        .SetDuration(TimeSpan::Milliseconds(1000));

    ASSERT_TRUE(animation.GetValid());

    ASSERT_EQ(TimeSpan::Seconds(1.0f), animation.GetTotalDuration());

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(animation.GetRunning());
    // Each step steps by 10ms, so 100 steps should cover the whole 1000ms animation
    this->StepAnimations({ animation }, 100, [&](uint32_t frame) {});

    EXPECT_EQ(animation.GetProgress(), 1.f);
    EXPECT_EQ(animation.GetCurrentKeyframeIndex(), ITrackAnimation::INVALID_INDEX);
    for (const auto& h : handlerObjects) {
        EXPECT_EQ(h->Called(), 1);
    }
    UnregisterObjectType<KeyframeHandler>();
}

/**
 * @tc.name: HandlerTest3
 * @tc.desc: Tests for Handler Test3. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TrackAnimationTest, HandlerTest3, testing::ext::TestSize.Level1)
{
    RegisterObjectType<KeyframeHandler>();
    static BASE_NS::vector<float> timestamps = { 0.0f, 0.4f, 0.6f, 1.0f };
    static BASE_NS::vector<BASE_NS::Math::Vec3> keyframes = { { 50, 30, 0 }, { 100, 80, 0 }, { 170, 150, 0 },
        { 500, 500, 0 } };
    static BASE_NS::Math::Vec3 initialValue = { 10, 10, 0 };

    BASE_NS::vector<ITestKeyframeHandler::Ptr> handlerObjects;
    BASE_NS::vector<IFunction::Ptr> handlers;
    for (int i = 0; i != timestamps.size(); ++i) {
        handlerObjects.push_back(META_NS::GetObjectRegistry().Create<ITestKeyframeHandler>(ClassId::KeyframeHandler));
        handlers.push_back(interface_cast<IMetadata>(handlerObjects.back())->GetFunction("MyFunc"));
    }

    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", initialValue);

    META_NS::TrackAnimation<BASE_NS::Math::Vec3> animation(CreateInstance(META_NS::ClassId::TrackAnimation));
    animation.SetKeyframes(keyframes)
        .SetTimestamps(timestamps)
        .SetKeyframeHandlers(handlers)
        .SetProperty(property)
        .SetDuration(TimeSpan::Milliseconds(1000));

    ASSERT_TRUE(animation.GetValid());

    auto loop =
        META_NS::AnimationModifiers::Loop(CreateInstance(META_NS::ClassId::LoopAnimationModifier)).SetLoopCount(3);
    auto attach = interface_pointer_cast<META_NS::IAttach>(animation);
    attach->Attach(interface_pointer_cast<IObject>(loop));

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(animation.GetRunning());
    // Each step steps by 10ms, so 300 steps should cover the whole 1000ms animation 3 times
    this->StepAnimations(
        { animation }, 302, [&](uint32_t frame) {});

    EXPECT_EQ(animation.GetProgress(), 1.f);
    EXPECT_EQ(animation.GetCurrentKeyframeIndex(), 3);

    EXPECT_EQ(handlerObjects[0]->Called(), 3);
    EXPECT_EQ(handlerObjects[1]->Called(), 3);
    EXPECT_EQ(handlerObjects[2]->Called(), 3);
    EXPECT_EQ(handlerObjects[3]->Called(), 3);

    UnregisterObjectType<KeyframeHandler>();
}

/**
 * @tc.name: KeyframeCurves
 * @tc.desc: Tests for Keyframe Curves. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TrackAnimationTest, KeyframeCurves, testing::ext::TestSize.Level1)
{
    constexpr auto keyframeCount = 4;
    constexpr auto animationMs = 1000;
    BASE_NS::vector<float> timestamps = { 0.0f, 0.4f, 0.6f, 1.0f };
    BASE_NS::vector<BASE_NS::Math::Vec3> keyframes = { { 50, 30, 0 }, { 100, 80, 0 }, { 170, 150, 0 },
        { 500, 500, 0 } };
    BASE_NS::Math::Vec3 initialValue = { 10, 10, 0 };
    BASE_NS::vector<ICurve1D::Ptr> curves = { CreateObjectInstance<ICurve1D>(META_NS::ClassId::InOutCubicEasingCurve),
        nullptr, CreateObjectInstance<ICurve1D>(META_NS::ClassId::OutBackEasingCurve) };
    constexpr auto propertyName = "PropToAnimate";

    auto property = ConstructProperty<BASE_NS::Math::Vec3>(propertyName, initialValue);

    META_NS::TrackAnimation<BASE_NS::Math::Vec3> animation(CreateInstance(META_NS::ClassId::TrackAnimation));
    animation.SetKeyframes(keyframes)
        .SetTimestamps(timestamps)
        .SetKeyframeCurves(curves)
        .SetProperty(property)
        .SetDuration(TimeSpan::Milliseconds(animationMs));
    Metadata(animation).AddProperty(property);

    ASSERT_TRUE(animation.GetValid());

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(animation.GetRunning());
    const float step = animationMs / 100.f;

    auto checkFrame = [&](const Property<BASE_NS::Math::Vec3>& animated,
                          const META_NS::TrackAnimation<BASE_NS::Math::Vec3>& checkAnim, uint32_t frame) {
        auto linearProgress = (frame * step) / animationMs;
        uint32_t keyframe = 0;
        // Find keyframe were supposed to be in
        for (int i = timestamps.size() - 1; i >= 0; i--) {
            if (timestamps[timestamps.size() - 1] == linearProgress || timestamps[i] <= linearProgress) {
                keyframe = i;
                break;
            }
        }
        EXPECT_EQ(linearProgress, checkAnim.GetProgress()) << "Frame:" << frame;
        EXPECT_EQ(keyframe, checkAnim.GetCurrentKeyframeIndex()) << "Frame:" << frame;

        if (keyframe < timestamps.size() - 1) {
            auto startTs = timestamps[keyframe];
            auto endTs = timestamps[keyframe + 1];
            auto start = keyframes[keyframe];
            auto end = keyframes[keyframe + 1];
            auto trackProgress = (1.f / (endTs - startTs)) * (linearProgress - startTs);
            if (auto curve = curves[keyframe]) {
                trackProgress = curve->Transform(trackProgress);
            }
            auto expected = start + (end - start) * trackProgress;
            EXPECT_EQ(expected, GetValue(animated));
        }
    };

    // Run the animation we just created, checking that every frame proceeds according to expected keyframe/curve
    this->StepAnimations({ animation }, 100, step, [&](uint32_t frame) { checkFrame(property, animation, frame); });

    // Export&import our animation, run it again and check that also imported animation works as expected
    TestSerialiser serializer;
    ASSERT_TRUE(serializer.Export(animation));
    serializer.Dump("app://test.json");

    META_NS::TrackAnimation<BASE_NS::Math::Vec3> imported(serializer.Import());
    ASSERT_TRUE(imported);
    auto importedProp = Metadata(imported).GetProperty<BASE_NS::Math::Vec3>(propertyName);
    ASSERT_TRUE(importedProp);

    imported.Start();
    imported.Step(this->GetTestClock());
    EXPECT_TRUE(imported.GetRunning());

    this->StepAnimations({ imported }, 100, step, [&](uint32_t frame) { checkFrame(importedProp, imported, frame); });
    Metadata(imported).RemoveProperty(importedProp);

    Metadata(animation).RemoveProperty(property);
}

/**
 * @tc.name: PropertyOnChanged
 * @tc.desc: Tests for Property On Changed. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TrackAnimationTest, PropertyOnChanged, testing::ext::TestSize.Level1)
{
    auto property = ConstructProperty<BASE_NS::Math::Vec3>("Prop", trackInitialValue);

    int count = 0;
    property->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] {
        ++count;
        property->GetValue();
    }));

    auto animation = META_NS::TrackAnimation<BASE_NS::Math::Vec3>(CreateInstance(META_NS::ClassId::TrackAnimation))
                         .SetKeyframes(trackKeyframes)
                         .SetTimestamps(trackTimestamps)
                         .SetProperty(property)
                         .SetDuration(TimeSpan::Milliseconds(1000));

    ASSERT_TRUE(animation.GetValid());
    ASSERT_EQ(TimeSpan::Seconds(1.0f), animation.GetTotalDuration());

    animation.Start();
    StepAnimations({ animation }, 102, [&animation, &property](uint32_t frame) {

    });
    EXPECT_FALSE(animation.GetRunning());
    EXPECT_TRUE(count >= 50) << count;
}

/**
 * @tc.name: SeekingPaused
 * @tc.desc: Tests for Seeking Paused. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TrackAnimationTest, SeekingPaused, testing::ext::TestSize.Level1)
{
    auto property = ConstructProperty<int>("Prop");

    static BASE_NS::vector<float> trackTimestamps = { 0.0f, 1.f };
    static BASE_NS::vector<int> trackKeyframes = { 0, 10 };
    auto animation = META_NS::TrackAnimation<int>(CreateInstance(META_NS::ClassId::TrackAnimation))
                         .SetKeyframes(trackKeyframes)
                         .SetTimestamps(trackTimestamps)
                         .SetProperty(property)
                         .SetDuration(TimeSpan::Milliseconds(1000));

    ASSERT_TRUE(animation.GetValid());
    ASSERT_EQ(TimeSpan::Seconds(1.0f), animation.GetTotalDuration());

    animation.Start();
    StepAnimations({ animation }, 10, [](uint32_t frame) {});
    animation.Stop();
    animation.Pause();

    int count = 0;
    property->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] {
        ++count;
        EXPECT_EQ(property->GetValue(), count);
    }));

    // should do nothing
    StepAnimations({ animation }, 10, [](uint32_t frame) {});

    animation.Seek(0.1f);

    // should do nothing
    StepAnimations({ animation }, 10, [](uint32_t frame) {});

    EXPECT_EQ(property->GetValue(), 1);
    EXPECT_EQ(count, 1);
    animation.Seek(0.2f);
    EXPECT_EQ(property->GetValue(), 2);
    EXPECT_EQ(count, 2);
    animation.Seek(0.3f);
    EXPECT_EQ(property->GetValue(), 3);
    EXPECT_EQ(count, 3);
}

/**
 * @tc.name: BeyondLastFrame
 * @tc.desc: Tests for Beyond Last Frame. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TrackAnimationTest, BeyondLastFrame, testing::ext::TestSize.Level1)
{
    static BASE_NS::vector<float> timestamps = { 0.5f, 0.6f, 0.7f };
    static BASE_NS::vector<uint32_t> keyframes = { 5, 7, 10 };

    auto property = ConstructProperty<uint32_t>("Prop");

    META_NS::TrackAnimation<uint32_t> animation(CreateInstance(META_NS::ClassId::TrackAnimation));
    animation.SetKeyframes(keyframes)
        .SetTimestamps(timestamps)
        .SetProperty(property)
        .SetDuration(TimeSpan::Milliseconds(30));

    ASSERT_TRUE(animation.GetValid());

    animation.Pause();
    animation.Seek(0.1);
    EXPECT_EQ(property->GetValue(), 5);
    animation.Stop();

    animation.Start();
    animation.Step(this->GetTestClock());
    EXPECT_TRUE(animation.GetRunning());
    this->StepAnimations({ animation }, 3, [&](uint32_t frame) {});

    EXPECT_EQ(animation.GetProgress(), 1.f);
    EXPECT_EQ(property->GetValue(), 10);

    animation.Pause();
    animation.Seek(0.7);
    EXPECT_EQ(property->GetValue(), 10);
    animation.Seek(0.1);
    EXPECT_EQ(property->GetValue(), 5);
    animation.Seek(0.9);
    EXPECT_EQ(property->GetValue(), 10);
}

} // namespace UTest

META_END_NAMESPACE()
