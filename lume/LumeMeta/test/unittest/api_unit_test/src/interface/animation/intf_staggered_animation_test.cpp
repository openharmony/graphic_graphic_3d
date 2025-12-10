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
#include <bitset>
#include <limits>
#include <numeric>
#include <test_framework.h>

#include <meta/api/animation.h>
#include <meta/api/util.h>
#include <meta/interface/intf_containable.h>

#include "helpers/animation_test_base.h"
#include "helpers/serialisation_utils.h"
#include "helpers/test_utils.h"
#include "helpers/testing_objects.h"
#include "helpers/util.h"

using META_NS::CreateInstance;

META_BEGIN_NAMESPACE()
namespace UTest {

class API_StaggeredAnimationTest : public API_AnimationTestBase {
protected:
    void SetUp() override
    {
        API_AnimationTestBase::SetUp();

        t1_ = META_NS::GetObjectRegistry().Create<ITestType>(ClassId::TestType);
        r1_ = META_NS::GetObjectRegistry().Create<ITestType>(ClassId::TestType);

        ASSERT_NE(t1_, nullptr);
    }

    template<class A, class T>
    void SetProperties(A& target, const IProperty::Ptr& property, T from, T to, TimeSpan duration)
    {
        target.SetProperty(property);
        target.SetFrom(from);
        target.SetTo(to);
        target.SetDuration(duration);
    }

    ITestType::Ptr t1_;
    ITestType::Ptr r1_;
};

/**
 * @tc.name: CreateParallel
 * @tc.desc: Tests for Create Parallel. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, CreateParallel, testing::ext::TestSize.Level1)
{
    auto anim = CreateObjectInstance<IParallelAnimation>();

    ASSERT_TRUE(anim);
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IObject.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IAnimation.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IStaggeredAnimation.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IParallelAnimation.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IStartableAnimation.Id().ToUid()));
}

/**
 * @tc.name: CreateSequential
 * @tc.desc: Tests for Create Sequential. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, CreateSequential, testing::ext::TestSize.Level1)
{
    auto anim = CreateObjectInstance<ISequentialAnimation>();

    ASSERT_TRUE(anim);
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IObject.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IAnimation.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IStaggeredAnimation.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::ISequentialAnimation.Id().ToUid()));
    EXPECT_TRUE(anim.GetInterface(META_NS::InterfaceId::IStartableAnimation.Id().ToUid()));
}

/**
 * @tc.name: ParallelSingleChild
 * @tc.desc: Tests for Parallel Single Child. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, ParallelSingleChild, testing::ext::TestSize.Level1)
{
    auto staggered = CreateObjectInstance<IParallelAnimation>();
    auto anim1 = CreateObjectInstance<IKeyframeAnimation, BASE_NS::Math::Vec3>();

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to { 100, 50, 0 };

    SetProperties(anim1, t1_->Vec3Property1(), from, to, TimeSpan::Milliseconds(100));

    Container(staggered).Add(Object(anim1));
    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(staggered.GetRunning());
    EXPECT_TRUE(anim1.GetRunning());
    EXPECT_EQ(staggered.GetTotalDuration(), TimeSpan::Milliseconds(100));

    BASE_NS::Math::Vec3 previous = from;
    float previousProgress = 0.f;

    StepAnimations({ staggered }, 10, 10, [&](uint32_t frame) {
        auto progress = staggered.GetProgress();
        // Check that progress of ParallelAnimation moves forward
        EXPECT_GT(progress, previousProgress);
        previousProgress = progress;

        bool running = anim1.GetRunning();
        EXPECT_EQ(running, frame < 10) << "Frame: " << frame;

        // Check animations that are still running are proceeding
        auto pos = GetValue(t1_->Vec3Property1());
        EXPECT_GT(pos, previous) << "Frame: " << frame;
        previous = pos;
    });

    EXPECT_FALSE(staggered.GetRunning());
    EXPECT_EQ(staggered.GetProgress(), 1.f);
    EXPECT_EQ(anim1.GetProgress(), 1.f);
    EXPECT_EQ(GetValue(t1_->Vec3Property1()), to);
}

/**
 * @tc.name: Parallel
 * @tc.desc: Tests for Parallel. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, Parallel, testing::ext::TestSize.Level1)
{
    auto staggered = CreateObjectInstance<IParallelAnimation>();
    auto anim1 = CreateObjectInstance<IKeyframeAnimation, BASE_NS::Math::Vec3>();
    auto anim2 = CreateObjectInstance<IKeyframeAnimation, BASE_NS::Math::Vec3>();
    auto anim3 = CreateObjectInstance<IKeyframeAnimation, BASE_NS::Math::Vec3>();

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };

    SetProperties(anim1, t1_->Vec3Property1(), from, to[0], TimeSpan::Milliseconds(100));
    SetProperties(anim2, t1_->Vec3Property2(), from, to[1], TimeSpan::Milliseconds(50));
    SetProperties(anim3, t1_->Vec3Property3(), from, to[2], TimeSpan::Milliseconds(100));

    auto c = Container(staggered);
    c.Add(anim1);
    c.Add(anim2);
    c.Add(anim3);
    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(staggered.GetRunning());
    EXPECT_TRUE(anim1.GetRunning());
    EXPECT_TRUE(anim2.GetRunning());
    EXPECT_TRUE(anim3.GetRunning());
    EXPECT_EQ(staggered.GetTotalDuration(), TimeSpan::Milliseconds(100));

    BASE_NS::Math::Vec3 previous[3] = { from, from, from };
    float previousProgress = 0.f;

    StepAnimations({ staggered }, 10, 10, [&](uint32_t frame) {
        auto progress = staggered.GetProgress();
        // Check that progress of ParallelAnimation moves forward
        EXPECT_GT(progress, previousProgress);
        previousProgress = progress;

        std::bitset<3> currentAnimsRunning;
        currentAnimsRunning[0] = anim1.GetRunning();
        currentAnimsRunning[1] = anim2.GetRunning();
        currentAnimsRunning[2] = anim3.GetRunning();
        // First animation finishes at 50ms, the rest at 100ms
        size_t expectedRunning = frame > 4 ? frame > 9 ? 0 : 2 : 3;
        EXPECT_EQ(currentAnimsRunning.count(), expectedRunning) << "Frame: " << frame;

        // Check animations that are still running are proceeding
        if (currentAnimsRunning[0]) {
            auto pos = GetValue(t1_->Vec3Property1());
            EXPECT_GT(pos, previous[0]) << "Frame: " << frame;
            previous[0] = pos;
        }
        if (currentAnimsRunning[1]) {
            auto rot = GetValue(t1_->Vec3Property2());
            EXPECT_GT(rot, previous[1]) << "Frame: " << frame;
            previous[1] = rot;
        }
        if (currentAnimsRunning[0]) {
            auto sca = GetValue(t1_->Vec3Property3());
            EXPECT_GT(sca, previous[2]) << "Frame: " << frame;
            previous[2] = sca;
        }
    });

    EXPECT_FALSE(staggered.GetRunning());
    EXPECT_EQ(staggered.GetProgress(), 1.f);
    EXPECT_EQ(anim1.GetProgress(), 1.f);
    EXPECT_EQ(anim2.GetProgress(), 1.f);
    EXPECT_EQ(anim3.GetProgress(), 1.f);
    EXPECT_EQ(GetValue(t1_->Vec3Property1()), to[0]);
    EXPECT_EQ(GetValue(t1_->Vec3Property2()), to[1]);
    EXPECT_EQ(GetValue(t1_->Vec3Property3()), to[2]);
}

/**
 * @tc.name: ParallelStopsItsChildrenWhenNotRunning
 * @tc.desc: Tests for Parallel Stops Its Children When Not Running. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, ParallelStopsItsChildrenWhenNotRunning, testing::ext::TestSize.Level1)
{
    auto staggered = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto childAnim = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    SetProperties(childAnim, t1_->Vec3Property1(), BASE_NS::Math::Vec3 { 0, 0, 0 }, BASE_NS::Math::Vec3 { 100, 50, 0 },
        TimeSpan::Milliseconds(100));

    Container(staggered).Add(childAnim);
    staggered.Start();
    StepAnimations({ staggered }, 10, 10, [](uint32_t /*frame*/) {});
    staggered.Stop();

    EXPECT_FALSE(staggered.GetRunning());
    EXPECT_FALSE(childAnim.GetRunning());
}

/**
 * @tc.name: ParallelSerialization
 * @tc.desc: Tests for Parallel Serialization. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, ParallelSerialization, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };
    {
        auto staggered = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
        auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
        auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
        auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

        SetProperties(anim1, t1_->Vec3Property1(), from, to[0], TimeSpan::Milliseconds(100));
        SetProperties(anim2, t1_->Vec3Property2(), from, to[1], TimeSpan::Milliseconds(50));
        SetProperties(anim3, t1_->Vec3Property3(), from, to[2], TimeSpan::Milliseconds(100));

        auto c = Container(staggered);

        c.Add(anim1);
        c.Add(anim2);
        c.Add(anim3);

        Metadata obj(CreateInstance(ClassId::Object));
        obj.AddProperty(t1_->Vec3Property1());
        obj.AddProperty(t1_->Vec3Property2());
        obj.AddProperty(t1_->Vec3Property3());
        AttachmentContainer(obj).Attach(staggered);
        ASSERT_TRUE(ser.Export(Object(obj)));
        ser.Dump("app://test.json");
    }

    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);

    auto att = interface_cast<IAttach>(obj);
    ASSERT_TRUE(att);
    auto attvec = att->GetAttachments<IAnimation>();
    ASSERT_EQ(attvec.size(), 1);
    ParallelAnimation staggered(attvec.front());
    ASSERT_TRUE(staggered);

    auto anims = staggered.GetAnimations();
    ASSERT_EQ(anims.size(), 3);
    KeyframeAnimation<BASE_NS::Math::Vec3> anim1(anims[0]);
    ASSERT_TRUE(anim1);
    KeyframeAnimation<BASE_NS::Math::Vec3> anim2(anims[1]);
    ASSERT_TRUE(anim2);
    KeyframeAnimation<BASE_NS::Math::Vec3> anim3(anims[2]);
    ASSERT_TRUE(anim3);
    Property<BASE_NS::Math::Vec3> prop1(anim1.GetProperty().lock());
    ASSERT_TRUE(prop1);
    Property<BASE_NS::Math::Vec3> prop2(anim2.GetProperty().lock());
    ASSERT_TRUE(prop2);
    Property<BASE_NS::Math::Vec3> prop3(anim3.GetProperty().lock());
    ASSERT_TRUE(prop3);

    staggered.Start();
    staggered.Step(GetTestClock());

    BASE_NS::Math::Vec3 previous[3] = { from, from, from };
    float previousProgress = 0.f;

    StepAnimations({ staggered }, 10, 10, [&](uint32_t frame) {
        auto progress = staggered.GetProgress();
        // Check that progress of ParallelAnimation moves forward
        EXPECT_GT(progress, previousProgress);
        previousProgress = progress;

        std::bitset<3> currentAnimsRunning;
        currentAnimsRunning[0] = anim1.GetRunning();
        currentAnimsRunning[1] = anim2.GetRunning();
        currentAnimsRunning[2] = anim3.GetRunning();
        // First animation finishes at 50ms, the rest at 100ms
        size_t expectedRunning = frame > 4 ? frame > 9 ? 0 : 2 : 3;
        EXPECT_EQ(currentAnimsRunning.count(), expectedRunning) << "Frame: " << frame;

        // Check animations that are still running are proceeding
        if (currentAnimsRunning[0]) {
            auto pos = prop1->GetValue();
            EXPECT_GT(pos, previous[0]) << "Frame: " << frame;
            previous[0] = pos;
        }
        if (currentAnimsRunning[1]) {
            auto rot = prop2->GetValue();
            EXPECT_GT(rot, previous[1]) << "Frame: " << frame;
            previous[1] = rot;
        }
        if (currentAnimsRunning[0]) {
            auto sca = prop3->GetValue();
            EXPECT_GT(sca, previous[2]) << "Frame: " << frame;
            previous[2] = sca;
        }
    });

    EXPECT_FALSE(staggered.GetRunning());
    EXPECT_EQ(staggered.GetProgress(), 1.f);
    EXPECT_EQ(anim1.GetProgress(), 1.f);
    EXPECT_EQ(anim2.GetProgress(), 1.f);
    EXPECT_EQ(anim3.GetProgress(), 1.f);
    EXPECT_EQ(GetValue(prop1), to[0]);
    EXPECT_EQ(GetValue(prop2), to[1]);
    EXPECT_EQ(GetValue(prop3), to[2]);
}

/**
 * @tc.name: ParallelLoop
 * @tc.desc: Tests for Parallel Loop. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, ParallelLoop, testing::ext::TestSize.Level1)
{
    auto staggered = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    const int loopCount = 3;
    auto loop =
        META_NS::AnimationModifiers::Loop(CreateInstance(ClassId::LoopAnimationModifier)).SetLoopCount(loopCount);

    AttachmentContainer(staggered).Attach(loop);

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };

    SetProperties(anim1, t1_->Vec3Property1(), from, to[0], TimeSpan::Milliseconds(100));
    SetProperties(anim2, t1_->Vec3Property2(), from, to[1], TimeSpan::Milliseconds(50));
    SetProperties(anim3, t1_->Vec3Property3(), from, to[2], TimeSpan::Milliseconds(100));

    Container c(staggered);
    c.Add(anim1);
    c.Add(anim2);
    c.Add(anim3);
    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(staggered.GetRunning());
    EXPECT_TRUE(anim1.GetRunning());
    EXPECT_TRUE(anim2.GetRunning());
    EXPECT_TRUE(anim3.GetRunning());
    EXPECT_EQ(staggered.GetTotalDuration(), TimeSpan::Milliseconds(100) * loopCount);

    for (int i = 0; i < loopCount; ++i) {
        float previousProgress = -1.f;
        auto initFrom = from - BASE_NS::Math::Vec3(1, 1, 1);
        BASE_NS::Math::Vec3 previous[3] = { initFrom, initFrom, initFrom };
        auto loop = i + 1;

        StepAnimations({ staggered }, 10, 10, [&](uint32_t frame) {
            auto progress = staggered.GetProgress();
            // Check that progress of ParallelAnimation moves forward
            EXPECT_GT(progress, previousProgress) << "Frame " << frame << ", Loop: " << loop;
            previousProgress = progress;

            std::bitset<3> currentAnimsRunning;
            currentAnimsRunning[0] = anim1.GetRunning();
            currentAnimsRunning[1] = anim2.GetRunning();
            currentAnimsRunning[2] = anim3.GetRunning();

            size_t expectedRunning = frame > 4 ? frame > 9 ? 0 : 2 : 3;
            ASSERT_EQ(currentAnimsRunning.count(), expectedRunning) << "Frame: " << frame << ", Loop: " << loop;

            // Check animations that are still running are proceeding
            if (currentAnimsRunning[0]) {
                auto pos = GetValue(t1_->Vec3Property1());
                EXPECT_GT(pos, previous[0]);
                previous[0] = pos;
            }
            if (currentAnimsRunning[1]) {
                auto rot = GetValue(t1_->Vec3Property2());
                EXPECT_GT(rot, previous[1]);
                previous[1] = rot;
            }
            if (currentAnimsRunning[0]) {
                auto sca = GetValue(t1_->Vec3Property3());
                EXPECT_GT(sca, previous[2]);
                previous[2] = sca;
            }
        });

        if (loop == loopCount) {
            // Last loop, animation should have finished
            EXPECT_FALSE(staggered.GetRunning());
        } else {
            EXPECT_TRUE(staggered.GetRunning());
            staggered.Step(GetTestClock()); // One extra step to let the animations jump back to beginning
        }
    }

    EXPECT_FLOAT_EQ(1.0f, staggered.GetProgress());

    EXPECT_FALSE(staggered.GetRunning());
    EXPECT_EQ(staggered.GetProgress(), 1.f);
    EXPECT_EQ(anim1.GetProgress(), 1.f);
    EXPECT_EQ(anim2.GetProgress(), 1.f);
    EXPECT_EQ(anim3.GetProgress(), 1.f);
    EXPECT_EQ(GetValue(t1_->Vec3Property1()), to[0]);
    EXPECT_EQ(GetValue(t1_->Vec3Property2()), to[1]);
    EXPECT_EQ(GetValue(t1_->Vec3Property3()), to[2]);
}

/**
 * @tc.name: ParallelReverse
 * @tc.desc: Tests for Parallel Reverse. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, ParallelReverse, testing::ext::TestSize.Level1)
{
    auto staggered = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto reverse = META_NS::AnimationModifiers::Reverse(CreateInstance(ClassId::ReverseAnimationModifier));

    AttachmentContainer(staggered).Attach(reverse);

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };

    SetProperties(anim1, t1_->Vec3Property1(), from, to[0], TimeSpan::Milliseconds(100));
    SetProperties(anim2, t1_->Vec3Property2(), from, to[1], TimeSpan::Milliseconds(30));
    SetProperties(anim3, t1_->Vec3Property3(), from, to[2], TimeSpan::Milliseconds(100));

    Container c(staggered);
    c.Add(anim1);
    c.Add(anim2);
    c.Add(anim3);
    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(staggered.GetRunning());
    EXPECT_TRUE(anim1.GetRunning());
    EXPECT_FALSE(anim2.GetRunning());
    EXPECT_TRUE(anim3.GetRunning());
    EXPECT_EQ(staggered.GetTotalDuration(), TimeSpan::Milliseconds(100));

    EXPECT_EQ(GetValue(t1_->Vec3Property1()), to[0]);
    EXPECT_EQ(GetValue(t1_->Vec3Property2()), from);
    EXPECT_EQ(GetValue(t1_->Vec3Property3()), to[2]);

    float previousProgress = -1.f;
    auto initFrom = to[2] + BASE_NS::Math::Vec3(1, 1, 1);
    BASE_NS::Math::Vec3 previous[3] = { initFrom, initFrom, initFrom };

    StepAnimations({ staggered }, 10, 10, [&](uint32_t frame) {
        auto progress = staggered.GetProgress();
        // Check that progress of ParallelAnimation moves forward
        EXPECT_GT(progress, previousProgress) << "Frame " << frame;
        previousProgress = progress;

        std::bitset<3> currentAnimsRunning;
        currentAnimsRunning[0] = anim1.GetRunning();
        currentAnimsRunning[1] = anim2.GetRunning();
        currentAnimsRunning[2] = anim3.GetRunning();

        size_t expectedRunning = frame > 6 ? frame > 9 ? 0 : 3 : 2;
        ASSERT_EQ(currentAnimsRunning.count(), expectedRunning) << "Frame: " << frame;

        // Check animations that are still running are proceeding
        if (currentAnimsRunning[0]) {
            auto pos = GetValue(t1_->Vec3Property1());
            EXPECT_LT(pos, previous[0]);
            previous[0] = pos;
        }
        if (currentAnimsRunning[1]) {
            auto rot = GetValue(t1_->Vec3Property2());
            EXPECT_LT(rot, previous[1]);
            previous[1] = rot;
        }
        if (currentAnimsRunning[0]) {
            auto sca = GetValue(t1_->Vec3Property3());
            EXPECT_LT(sca, previous[2]);
            previous[2] = sca;
        }
    });

    EXPECT_FLOAT_EQ(1.0f, staggered.GetProgress());

    EXPECT_FALSE(staggered.GetRunning());
    EXPECT_EQ(staggered.GetProgress(), 1.f);
    EXPECT_EQ(anim1.GetProgress(), 0.f);
    EXPECT_EQ(anim2.GetProgress(), 0.f);
    EXPECT_EQ(anim3.GetProgress(), 0.f);
    EXPECT_EQ(GetValue(t1_->Vec3Property1()), from);
    EXPECT_EQ(GetValue(t1_->Vec3Property2()), from);
    EXPECT_EQ(GetValue(t1_->Vec3Property3()), from);
}

/**
 * @tc.name: LoopPastEnd
 * @tc.desc: Tests for Loop Past End. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, LoopPastEnd, testing::ext::TestSize.Level1)
{
    META_NS::KeyframeAnimation<float> a1(CreateInstance(ClassId::KeyframeAnimation));
    META_NS::KeyframeAnimation<float> a2(CreateInstance(ClassId::KeyframeAnimation));
    auto staggered = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
    Container(staggered).Add(
        a1.SetFrom(0).SetTo(10).SetProperty(t1_->FloatProperty1()).SetDuration(TimeSpan::Milliseconds(100)));
    Container(staggered).Add(
        a2.SetFrom(10)
            .SetTo(0)
            .SetProperty(t1_->FloatProperty1())
            .SetDuration(TimeSpan::Milliseconds(100))
            .SetCurve(interface_pointer_cast<ICurve1D>(CreateInstance(ClassId::InOutCubicEasingCurve))));

    AttachmentContainer(staggered).Attach(
        META_NS::AnimationModifiers::Loop(CreateInstance(ClassId::LoopAnimationModifier)).LoopIndefinitely());

    staggered.Start();
    ASSERT_TRUE(staggered.GetRunning());

    float expectedProgress = 0.f;
    float expectedProgressStep = 20.f / 200.f; // 200ms animation with 20ms step

    // Run the animation once so that our steps hit 1.0
    for (auto loop = 1; loop <= 3; loop++) {
        staggered.Step(GetTestClock());
        StepAnimations({ staggered }, 10, 20, [&](uint32_t frame) {
            auto progress = staggered.GetProgress();
            expectedProgress += expectedProgressStep;
            expectedProgress = expectedProgress - BASE_NS::Math::floor(expectedProgress);

            EXPECT_EQ(a1.GetRunning(), frame < 5) << "Frame: " << frame << ", loop: " << loop;
            EXPECT_EQ(a2.GetRunning(), frame >= 5 && frame < 10) << "Frame: " << frame << ", loop: " << loop;
        });
    }

    staggered.Restart();
    staggered.Step(GetTestClock());
    expectedProgress = 0.f;
    expectedProgressStep = 30.f / 200.f; // 200ms animation with 30ms step

    // Run again but now so that we step past 1.0
    for (auto loop = 1; loop <= 3; loop++) {
        auto frameCount = loop == 1 ? 6 : 7;
        StepAnimations({ staggered }, frameCount, 30, [&](uint32_t frame) {
            auto progress = staggered.GetProgress();
            expectedProgress += expectedProgressStep;
            expectedProgress = expectedProgress - BASE_NS::Math::floor(expectedProgress);

            EXPECT_NEAR(progress, expectedProgress, 0.05);

            // On the seventh frame we should jump past one loop, and be at 0.05 progress or thereabouts
            bool a1running = frame < 4;
            bool a2running = !a1running && progress < 1.f;

            EXPECT_EQ(a1.GetRunning(), a1running) << "Frame: " << frame << ", loop: " << loop;
            EXPECT_EQ(a2.GetRunning(), a2running) << "Frame: " << frame << ", loop: " << loop;
        });
    }
}

/**
 * @tc.name: Sequential
 * @tc.desc: Tests for Sequential. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, Sequential, testing::ext::TestSize.Level1)
{
    auto staggered = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
    auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };

    // In total 150ms of animations
    SetProperties(anim1, t1_->Vec3Property1(), from, to[0], TimeSpan::Milliseconds(50));
    SetProperties(anim2, t1_->Vec3Property2(), from, to[1], TimeSpan::Milliseconds(100));
    SetProperties(anim3, t1_->Vec3Property3(), from, to[2], TimeSpan::Milliseconds(50));

    Container c(staggered);
    c.Add(anim1);
    c.Add(anim2);
    c.Add(anim3);

    EXPECT_EQ(staggered.GetTotalDuration(), TimeSpan::Milliseconds(200));

    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(anim1.GetRunning());
    EXPECT_TRUE(staggered.GetRunning());

    std::bitset<3> animsRun;
    float previousProgress = 0.f;

    // 20 frames at 10ms interval should finish the animation
    RunFrames(20, [&animsRun, &anim1, &anim2, &anim3, &staggered, &previousProgress](uint32_t frame) {
        auto progress = staggered.GetProgress();

        // Each frame should progress the animation
        EXPECT_GT(progress, previousProgress) << "at frame " << frame;
        previousProgress = progress;

        std::bitset<3> currentAnimsRunning;
        currentAnimsRunning[0] = anim1.GetRunning();
        currentAnimsRunning[1] = anim2.GetRunning();
        currentAnimsRunning[2] = anim3.GetRunning();

        // Only one animation should be running at a time
        EXPECT_EQ(currentAnimsRunning.count(), frame < 20 ? 1 : 0) << "at frame " << frame;

        animsRun |= currentAnimsRunning;

        // Check that total animation progress is correct for the
        // currently running animation from the sequence
        if (currentAnimsRunning[0]) {
            EXPECT_GE(progress, 0.f);
            EXPECT_LE(progress, 1.f / 4.f);
        }
        if (currentAnimsRunning[1]) {
            EXPECT_GE(progress, 1.f / 4.f);
            EXPECT_LE(progress, 3.f / 4.f);
        }
        if (currentAnimsRunning[2]) {
            EXPECT_GE(progress, 3.f / 4.f);
            EXPECT_LE(progress, 1.f);
        }
    });

    EXPECT_EQ(animsRun.count(), 3);
    EXPECT_FALSE(staggered.GetRunning());
    EXPECT_EQ(staggered.GetProgress(), 1.f);
    EXPECT_EQ(anim1.GetProgress(), 1.f);
    EXPECT_EQ(anim2.GetProgress(), 1.f);
    EXPECT_EQ(anim3.GetProgress(), 1.f);
    EXPECT_EQ(GetValue(t1_->Vec3Property1()), to[0]);
    EXPECT_EQ(GetValue(t1_->Vec3Property2()), to[1]);
    EXPECT_EQ(GetValue(t1_->Vec3Property3()), to[2]);
}

/**
 * @tc.name: SequentialSerialization
 * @tc.desc: Tests for Sequential Serialization. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, SequentialSerialization, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };
    {
        auto staggered = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
        auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
        auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
        auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

        // In total 150ms of animations
        SetProperties(anim1, t1_->Vec3Property1(), from, to[0], TimeSpan::Milliseconds(50));
        SetProperties(anim2, t1_->Vec3Property2(), from, to[1], TimeSpan::Milliseconds(100));
        SetProperties(anim3, t1_->Vec3Property3(), from, to[2], TimeSpan::Milliseconds(50));

        Container c(staggered);
        c.Add(anim1);
        c.Add(anim2);
        c.Add(anim3);

        Metadata obj(CreateInstance(ClassId::Object));
        obj.AddProperty(t1_->Vec3Property1());
        obj.AddProperty(t1_->Vec3Property2());
        obj.AddProperty(t1_->Vec3Property3());
        AttachmentContainer(obj).Attach(staggered);
        ASSERT_TRUE(ser.Export(Object(obj)));
        ser.Dump("app://test.json");
    }

    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);

    auto att = interface_cast<IAttach>(obj);
    ASSERT_TRUE(att);
    auto attvec = att->GetAttachments<IAnimation>();
    ASSERT_EQ(attvec.size(), 1);
    SequentialAnimation staggered(attvec.front());
    ASSERT_TRUE(staggered);

    auto anims = staggered.GetAnimations();
    ASSERT_EQ(anims.size(), 3);
    KeyframeAnimation<BASE_NS::Math::Vec3> anim1(anims[0]);
    ASSERT_TRUE(anim1);
    KeyframeAnimation<BASE_NS::Math::Vec3> anim2(anims[1]);
    ASSERT_TRUE(anim2);
    KeyframeAnimation<BASE_NS::Math::Vec3> anim3(anims[2]);
    ASSERT_TRUE(anim3);
    Property<BASE_NS::Math::Vec3> prop1(anim1.GetProperty().lock());
    ASSERT_TRUE(prop1);
    Property<BASE_NS::Math::Vec3> prop2(anim2.GetProperty().lock());
    ASSERT_TRUE(prop2);
    Property<BASE_NS::Math::Vec3> prop3(anim3.GetProperty().lock());
    ASSERT_TRUE(prop3);

    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(anim1.GetRunning());
    EXPECT_TRUE(staggered.GetRunning());

    std::bitset<3> animsRun;
    float previousProgress = 0.f;

    // 20 frames at 10ms interval should finish the animation
    RunFrames(20, [&animsRun, &anim1, &anim2, &anim3, &staggered, &previousProgress](uint32_t frame) {
        auto progress = staggered.GetProgress();

        // Each frame should progress the animation
        EXPECT_GT(progress, previousProgress) << "at frame " << frame;
        previousProgress = progress;

        std::bitset<3> currentAnimsRunning;
        currentAnimsRunning[0] = anim1.GetRunning();
        currentAnimsRunning[1] = anim2.GetRunning();
        currentAnimsRunning[2] = anim3.GetRunning();

        // Only one animation should be running at a time
        EXPECT_EQ(currentAnimsRunning.count(), frame < 20 ? 1 : 0) << "at frame " << frame;

        animsRun |= currentAnimsRunning;

        // Check that total animation progress is correct for the
        // currently running animation from the sequence
        if (currentAnimsRunning[0]) {
            EXPECT_GE(progress, 0.f);
            EXPECT_LE(progress, 1.f / 4.f);
        }
        if (currentAnimsRunning[1]) {
            EXPECT_GE(progress, 1.f / 4.f);
            EXPECT_LE(progress, 3.f / 4.f);
        }
        if (currentAnimsRunning[2]) {
            EXPECT_GE(progress, 3.f / 4.f);
            EXPECT_LE(progress, 1.f);
        }
    });

    EXPECT_EQ(animsRun.count(), 3);
    EXPECT_FALSE(staggered.GetRunning());
    EXPECT_EQ(staggered.GetProgress(), 1.f);
    EXPECT_EQ(anim1.GetProgress(), 1.f);
    EXPECT_EQ(anim2.GetProgress(), 1.f);
    EXPECT_EQ(anim3.GetProgress(), 1.f);
    EXPECT_EQ(GetValue(prop1), to[0]);
    EXPECT_EQ(GetValue(prop2), to[1]);
    EXPECT_EQ(GetValue(prop3), to[2]);
}

/**
 * @tc.name: SequentialWithSameAnimation
 * @tc.desc: Tests for Sequential With Same Animation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, SequentialWithSameAnimation, testing::ext::TestSize.Level1)
{
    auto staggered = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
    auto anim = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to = { 100, 50, 0 };

    SetProperties(anim, t1_->Vec3Property1(), from, to, TimeSpan::Milliseconds(50));

    Container c(staggered);
    c.Add(anim);
    c.Add(anim);
    c.Add(anim);

    EXPECT_EQ(staggered.GetTotalDuration(), TimeSpan::Milliseconds(150));

    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(anim.GetRunning());
    EXPECT_TRUE(staggered.GetRunning());

    float previousProgress = 0.f;

    // 20 frames + 10ms sleep at end of each frame -> Should be enough time
    // for all animations to run
    StepAnimations({ staggered }, 15, 10, [&](uint32_t frame) {
        EXPECT_EQ(staggered.GetRunning(), frame < 15) << "Frame: " << frame;
        auto progress = staggered.GetProgress();

        // Each frame should progress the animation
        EXPECT_GT(progress, previousProgress) << "Frame: " << frame;
        previousProgress = progress;
    });

    EXPECT_FALSE(staggered.GetRunning());
    EXPECT_EQ(staggered.GetProgress(), 1.f);
    EXPECT_EQ(anim.GetProgress(), 1.f);
}

/**
 * @tc.name: SequentialModified
 * @tc.desc: Tests for Sequential Modified. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, SequentialModified, testing::ext::TestSize.Level1)
{
    auto staggered = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
    auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    const int loopCount = 3;
    auto loop =
        META_NS::AnimationModifiers::Loop(CreateInstance(ClassId::LoopAnimationModifier)).SetLoopCount(loopCount);
    AttachmentContainer(staggered).Attach(loop);

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };

    // In total 150ms of animations
    SetProperties(anim1, t1_->Vec3Property1(), from, to[0], TimeSpan::Milliseconds(50));
    SetProperties(anim2, t1_->Vec3Property2(), from, to[1], TimeSpan::Milliseconds(50));
    SetProperties(anim3, t1_->Vec3Property3(), from, to[2], TimeSpan::Milliseconds(50));

    Container c(staggered);
    c.Add(anim1);
    c.Add(anim2);
    c.Add(anim3);
    staggered.Start();
    staggered.Step(GetTestClock());

    EXPECT_TRUE(anim1.GetRunning());
    EXPECT_TRUE(staggered.GetRunning());

    std::bitset<3> animsRun;

    for (int i = 0; i < loopCount; ++i) {
        float previousProgress = -1;
        auto loop = i + 1;

        StepAnimations({ staggered }, 15, 10, [&](uint32_t frame) {
            auto progress = staggered.GetProgress();

            // Each frame should progress the animation
            EXPECT_GT(progress, previousProgress) << "Frame: " << frame << ", Loop: " << loop;
            previousProgress = progress;

            std::bitset<3> currentAnimsRunning;
            currentAnimsRunning[0] = anim1.GetRunning();
            currentAnimsRunning[1] = anim2.GetRunning();
            currentAnimsRunning[2] = anim3.GetRunning();

            // Only one animation should be running at a time
            EXPECT_EQ(currentAnimsRunning.count(), frame < 15 ? 1 : 0) << "Frame: " << frame << ", Loop: " << loop;

            animsRun |= currentAnimsRunning;

            // Check that total animation progress is correct for the
            // currently running animation from the sequence
            if (currentAnimsRunning[0]) {
                EXPECT_GE(progress, 0.f);
                EXPECT_LE(progress, 1.f / 3.f + std::numeric_limits<float>::epsilon());
            }
            if (currentAnimsRunning[1]) {
                EXPECT_GE(progress, 1.f / 3.f - std::numeric_limits<float>::epsilon());
                EXPECT_LE(progress, 2.f / 3.f + std::numeric_limits<float>::epsilon());
            }
            if (currentAnimsRunning[2]) {
                EXPECT_GE(progress, 2.f / 3.f - std::numeric_limits<float>::epsilon());
                EXPECT_LE(progress, 1.f);
            }
        });
        if (loop == loopCount) {
            // Last loop, animation should have finished
            EXPECT_FALSE(staggered.GetRunning());
        } else {
            EXPECT_TRUE(staggered.GetRunning());
            staggered.Step(GetTestClock()); // One extra step to let the animations jump back to beginning
        }
    }

    EXPECT_EQ(animsRun.count(), 3);
    EXPECT_FALSE(staggered.GetRunning());
    EXPECT_EQ(staggered.GetProgress(), 1.f);
    EXPECT_EQ(anim1.GetProgress(), 1.f);
    EXPECT_EQ(anim2.GetProgress(), 1.f);
    EXPECT_EQ(anim3.GetProgress(), 1.f);
    EXPECT_EQ(GetValue(t1_->Vec3Property1()), to[0]);
    EXPECT_EQ(GetValue(t1_->Vec3Property2()), to[1]);
    EXPECT_EQ(GetValue(t1_->Vec3Property3()), to[2]);
}

/**
 * @tc.name: DurationParallel
 * @tc.desc: Tests for Duration Parallel. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, DurationParallel, testing::ext::TestSize.Level1)
{
    auto staggered = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };
    TimeSpan baseDuration = TimeSpan::Milliseconds(50);

    // Last animation has the longest duration
    std::vector<TimeSpan> duration = { baseDuration, baseDuration, baseDuration * 12 };

    SetProperties(anim1, t1_->Vec3Property1(), from, to[0], duration[0]);
    SetProperties(anim2, t1_->Vec3Property2(), from, to[1], duration[1]);
    SetProperties(anim3, t1_->Vec3Property3(), from, to[2], duration[2]);

    Container c(staggered);
    c.Add(anim1);
    c.Add(anim2);
    c.Add(anim3);

    auto staggeredDuration = *std::max_element(duration.begin(), duration.end());
    EXPECT_EQ(staggered.GetTotalDuration(), staggeredDuration);
}

/**
 * @tc.name: ParallelWithSameAnimation
 * @tc.desc: Tests for Parallel With Same Animation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, ParallelWithSameAnimation, testing::ext::TestSize.Level1)
{
    auto staggered = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto anim = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to = { 100, 50, 0 };

    SetProperties(anim, t1_->Vec3Property1(), from, to, TimeSpan::Milliseconds(50));

    Container c(staggered);
    c.Add(anim);
    c.Add(anim);
    c.Add(anim);

    EXPECT_EQ(staggered.GetAnimations().size(), 1);
    EXPECT_EQ(staggered.GetTotalDuration(), TimeSpan::Milliseconds(50));
}

/**
 * @tc.name: DurationParallelModified
 * @tc.desc: Tests for Duration Parallel Modified. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, DurationParallelModified, testing::ext::TestSize.Level1)
{
    for (int loopCount = 1; loopCount != 4; ++loopCount) {
        auto staggered = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
        auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
        auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
        auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

        auto loop =
            META_NS::AnimationModifiers::Loop(CreateInstance(ClassId::LoopAnimationModifier)).SetLoopCount(loopCount);
        AttachmentContainer(staggered).Attach(loop);

        BASE_NS::Math::Vec3 from { 0, 0, 0 };
        BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };
        TimeSpan baseDuration = TimeSpan::Milliseconds(50);

        // Last animation has the longest duration
        std::vector<TimeSpan> duration = { baseDuration, baseDuration, baseDuration * 12 };

        SetProperties(anim1, t1_->Vec3Property1(), from, to[0], duration[0]);
        SetProperties(anim2, t1_->Vec3Property2(), from, to[1], duration[1]);
        SetProperties(anim3, t1_->Vec3Property3(), from, to[2], duration[2]);

        Container c(staggered);
        c.Add(anim1);
        c.Add(anim2);
        c.Add(anim3);

        auto staggeredDuration = *std::max_element(duration.begin(), duration.end());
        EXPECT_EQ(staggered.GetTotalDuration(), staggeredDuration * loopCount);

        anim1.SetDuration(staggeredDuration - TimeSpan::Milliseconds(10));

        staggeredDuration = *std::max_element(duration.begin(), duration.end());
        EXPECT_EQ(staggered.GetTotalDuration(), staggeredDuration * loopCount);

        staggeredDuration += TimeSpan::Milliseconds(10);
        anim2.SetDuration(staggeredDuration);

        EXPECT_EQ(staggered.GetTotalDuration(), staggeredDuration * loopCount);
    }
}

/**
 * @tc.name: DurationSequential
 * @tc.desc: Tests for Duration Sequential. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, DurationSequential, testing::ext::TestSize.Level1)
{
    auto staggered = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
    auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    BASE_NS::Math::Vec3 from { 0, 0, 0 };
    BASE_NS::Math::Vec3 to[3] = { { 100, 50, 0 }, { 20, 20, 100 }, { 200, 200, 0 } };
    TimeSpan baseDuration = TimeSpan::Milliseconds(50);

    // Last animation has the longest duration
    std::vector<TimeSpan> duration = { baseDuration, baseDuration, baseDuration * 12 };

    SetProperties(anim1, t1_->Vec3Property1(), from, to[0], duration[0]);
    SetProperties(anim2, t1_->Vec3Property2(), from, to[1], duration[1]);
    SetProperties(anim3, t1_->Vec3Property3(), from, to[2], duration[2]);

    Container c(staggered);
    c.Add(anim1);
    c.Add(anim2);
    c.Add(anim3);

    auto staggeredDuration = std::accumulate(duration.begin(), duration.end(), TimeSpan::Zero());
    EXPECT_EQ(staggered.GetTotalDuration(), staggeredDuration);
}

/**
 * @tc.name: Hierarchy
 * @tc.desc: Tests for Hierarchy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, Hierarchy, testing::ext::TestSize.Level1)
{
    /*
    Create the following animation hierarchy:
    par1                    ParallelAnimation
      |
      +-- seq1              SequentialAnimation
      |     |
      |     +-- par2        ParallelAnimation
      |     |     |
      |     |     +-- a1    KeyframeAnimation
      |     |     +-- a2    KeyframeAnimation
      |     +-- a3          KeyframeAnimation
      +-- a4                KeyframeAnimation
    */

    auto p1 = ConstructProperty<float>("p1", 0);
    auto p2 = ConstructProperty<float>("p2", 0);
    auto p3 = ConstructProperty<float>("p3", 0);
    auto p4 = ConstructProperty<float>("p4", 0);

    auto seq1 = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
    auto par1 = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto par2 = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));

    auto a1 = META_NS::KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation));
    auto a2 = META_NS::KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation));
    auto a3 = META_NS::KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation));
    auto a4 = META_NS::KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation));

    float from[4] = { 0, 5, 10, 20 };
    float to[4] = { 10, 10, 25, 40 };

    SetProperties(a1, p1, from[0], to[0], TimeSpan::Milliseconds(30));
    SetProperties(a2, p2, from[1], to[1], TimeSpan::Milliseconds(30));
    SetProperties(a3, p3, from[2], to[2], TimeSpan::Milliseconds(75));
    SetProperties(a4, p4, from[3], to[3], TimeSpan::Milliseconds(150));

    par2.Add(a1).Add(a2);
    seq1.Add(a3);
    par1.Add(seq1).Add(a4);
    seq1.Add(par2);

    par1.Start();
    par1.Step(GetTestClock());

    float previousProgress = 0.f;

    StepAnimations({ par1 }, 15, 10, [&](uint32_t frame) {
        EXPECT_EQ(par1.GetRunning(), frame < 15) << "Frame " << frame;

        auto progress = par1.GetProgress();
        EXPECT_GT(progress, previousProgress) << "Frame " << frame;
        previousProgress = progress;
    });

    EXPECT_FALSE(par1.GetRunning());

    EXPECT_EQ(seq1.GetProgress(), 1.f);
    EXPECT_EQ(par1.GetProgress(), 1.f);
    EXPECT_EQ(par2.GetProgress(), 1.f);
    EXPECT_EQ(a1.GetProgress(), 1.f);
    EXPECT_EQ(a2.GetProgress(), 1.f);
    EXPECT_EQ(a3.GetProgress(), 1.f);
    EXPECT_EQ(a4.GetProgress(), 1.f);
    EXPECT_EQ(GetValue(p1), to[0]);
    EXPECT_EQ(GetValue(p2), to[1]);
    EXPECT_EQ(GetValue(p3), to[2]);
    EXPECT_EQ(GetValue(p4), to[3]);
}

/**
 * @tc.name: Reparent
 * @tc.desc: Tests for Reparent. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, Reparent, testing::ext::TestSize.Level1)
{
    auto p1 = ConstructProperty<float>("p1", 0);
    auto p2 = ConstructProperty<float>("p2", 0);
    auto seq = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
    auto par = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));

    auto a1 = META_NS::KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation));
    auto a2 = META_NS::KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation));

    auto a1cont = interface_cast<IContainable>(a1);
    auto a2cont = interface_cast<IContainable>(a2);
    ASSERT_NE(a1cont, nullptr);
    ASSERT_NE(a2cont, nullptr);

    float from[2] = { 0, 5 };
    float to[2] = { 10, 50 };
    TimeSpan duration[2] = { TimeSpan::Milliseconds(100), TimeSpan::Milliseconds(200) };

    SetProperties(a1, p1, from[0], to[0], duration[0]);
    SetProperties(a2, p2, from[1], to[1], duration[1]);

    seq.Add(a1).Add(a2);
    EXPECT_EQ(a1cont->GetParent(), seq);
    EXPECT_EQ(a2cont->GetParent(), seq);

    EXPECT_EQ(seq.GetTotalDuration(), duration[0] + duration[1]);

    seq.Start();
    seq.Step(GetTestClock());

    EXPECT_TRUE(seq.GetRunning());
    EXPECT_TRUE(a1.GetRunning());

    // Reparent the animation
    par.Add(a1);
    EXPECT_EQ(a1cont->GetParent(), par);
    EXPECT_EQ(a2cont->GetParent(), seq);

    EXPECT_TRUE(seq.GetRunning());
    EXPECT_TRUE(a1.GetRunning());
    seq.Stop();

    // Animation durations should have been updated due to hierarchy change
    EXPECT_EQ(seq.GetTotalDuration(), duration[1]);
    EXPECT_EQ(par.GetTotalDuration(), duration[0]);

    par.Start();
    EXPECT_FALSE(seq.GetRunning());
    EXPECT_TRUE(par.GetRunning());
    EXPECT_TRUE(a1.GetRunning());
}

/**
 * @tc.name: Name
 * @tc.desc: Tests for Name. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, Name, testing::ext::TestSize.Level1)
{
    ParallelAnimation a1(CreateInstance(ClassId::ParallelAnimation));
    SequentialAnimation a2(CreateInstance(ClassId::SequentialAnimation));

    Named n1(a1);
    Named n2(a2);
    EXPECT_NE(n1.GetName(), "");
    EXPECT_NE(n2.GetName(), "");

    n1.SetName("Anim1");
    n2.SetName("Anim2");

    EXPECT_EQ(n1.GetName(), "Anim1");
    EXPECT_EQ(a1.GetPtr()->GetName(), n1.GetName());
    EXPECT_EQ(n2.GetName(), "Anim2");
    EXPECT_EQ(a2.GetPtr()->GetName(), n2.GetName());
}

/**
 * @tc.name: ParallelMove
 * @tc.desc: Tests for Parallel Move. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, ParallelMove, testing::ext::TestSize.Level1)
{
    auto staggered = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    staggered.Add(anim1).Add(anim2).Add(anim3);

    auto c = META_NS::Container(staggered);
    ASSERT_TRUE(c);
    EXPECT_TRUE(c.Move(anim1, 2));

    EXPECT_TRUE(c.Move(0, 2));
    EXPECT_TRUE(c.Move(2, 1));

    c.Remove(anim1);
    c.Remove(anim2);
    c.Remove(anim3);
    EXPECT_EQ(c.GetSize(), 0);
}

/**
 * @tc.name: SequentialMove
 * @tc.desc: Tests for Sequential Move. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, SequentialMove, testing::ext::TestSize.Level1)
{
    auto staggered = META_NS::SequentialAnimation(CreateInstance(ClassId::SequentialAnimation));
    auto anim1 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim2 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));
    auto anim3 = META_NS::KeyframeAnimation<BASE_NS::Math::Vec3>(CreateInstance(ClassId::KeyframeAnimation));

    staggered.Add(anim1).Add(anim2).Add(anim3);

    auto c = META_NS::Container(staggered);
    ASSERT_TRUE(c);
    EXPECT_TRUE(c.Move(anim1, 2));

    EXPECT_TRUE(c.Move(0, 2));
    EXPECT_TRUE(c.Move(2, 1));

    c.Remove(anim1);
    c.Remove(anim2);
    c.Remove(anim3);
    EXPECT_EQ(c.GetSize(), 0);
}

/**
 * @tc.name: ParallelWithTrackAnimationSeeking
 * @tc.desc: Tests for Parallel With Track Animation Seeking. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_StaggeredAnimationTest, ParallelWithTrackAnimationSeeking, testing::ext::TestSize.Level1)
{
    static BASE_NS::vector<float> trackTimestamps = { 0.0f, 1.f };
    static BASE_NS::vector<int> trackKeyframes = { 0, 10 };

    auto property = ConstructProperty<int>("Prop");
    auto parallel = META_NS::ParallelAnimation(CreateInstance(ClassId::ParallelAnimation));
    auto anim = META_NS::TrackAnimation<int>(CreateInstance(ClassId::TrackAnimation));
    anim.SetKeyframes(trackKeyframes)
        .SetTimestamps(trackTimestamps)
        .SetProperty(property)
        .SetDuration(TimeSpan::Milliseconds(1000));

    ASSERT_TRUE(parallel);
    ASSERT_TRUE(anim);
    parallel.Add(anim);
    parallel.Start();
    parallel.Step(GetTestClock());

    EXPECT_TRUE(parallel.GetRunning());
    EXPECT_TRUE(anim.GetRunning());

    parallel.Pause();

    int count = 0;
    property->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] {
        ++count;
        EXPECT_EQ(property->GetValue(), count);
    }));

    // should do nothing
    StepAnimations({ parallel }, 10, [](uint32_t frame) {});

    parallel.Seek(0.1f);

    // should do nothing
    StepAnimations({ parallel }, 10, [](uint32_t frame) {});

    EXPECT_EQ(property->GetValue(), 1);
    EXPECT_EQ(count, 1);
    parallel.Seek(0.2f);
    EXPECT_EQ(property->GetValue(), 2);
    EXPECT_EQ(count, 2);
    parallel.Seek(0.3f);
    EXPECT_EQ(property->GetValue(), 3);
    EXPECT_EQ(count, 3);
}

} // namespace UTest

META_END_NAMESPACE()
