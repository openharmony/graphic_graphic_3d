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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <meta/api/animation.h>
#include <meta/api/curves.h>
#include <meta/base/namespace.h>

#include "TestRunner.h"
#include "helpers/animation_test_base.h"

using namespace testing;
using namespace testing::ext;
using namespace META_NS::TimeSpanLiterals;

META_BEGIN_NAMESPACE()

class BezierCurveTest : public AnimationTestBase {
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
        curve_ = interface_cast<ICurve1D>(bezier_);
        ASSERT_NE(curve_, nullptr);
        ASSERT_NE(interface_cast<IEasingCurve>(bezier_), nullptr);
        ASSERT_NE(interface_cast<ICubicBezier>(bezier_), nullptr);
    }

    void SetControlPoints()
    {
        // Set control points to create a steeply rising curve
        BASE_NS::Math::Vec2 cp1 = { 1.f, 0.f };
        BASE_NS::Math::Vec2 cp2 = { 0.0f, 1.f };
        bezier_.SetControlPoint1({ 1.f, 0.f }).SetControlPoint2({ 0.0f, 1.f });
        ASSERT_EQ(bezier_.GetControlPoint1(), cp1);
        ASSERT_EQ(bezier_.GetControlPoint2(), cp2);
    }

    META_NS::ICurve1D* curve_ {};
    META_NS::Curves::CubicBezier bezier_ { META_NS::CreateNew };

    static constexpr float epsilon_ = 0.001f;
    static constexpr auto frames_ = 20;
    // Pre-calculated 20 cubic bezier curve values for cp1=[1.f,0.f] and cp2=[0.f, 1.f]
    static constexpr float expected_[] = { 0.000000, 0.000882, 0.003711, 0.009070, 0.017385, 0.029723, 0.047319,
        0.072904, 0.111333, 0.176019, 0.500000, 0.823981, 0.888667, 0.927096, 0.952681, 0.970277, 0.982615, 0.990930,
        0.996289, 0.999118, 1.000000 };
};

/**
 * @tc.name: BezierDefault
 * @tc.desc: test BezierDefault
 * @tc.type: FUNC
 */
HWTEST_F(BezierCurveTest, BezierDefault, TestSize.Level1)
{
    // Linear as control points are at default [0,0], [1,1]
    for (int i = 0; i < 100; i++) {
        float f = i / 100.f;
        EXPECT_NEAR(curve_->Transform(f), f, epsilon_);
    }
}

/**
 * @tc.name: BezierCurve
 * @tc.desc: test BezierCurve
 * @tc.type: FUNC
 */
HWTEST_F(BezierCurveTest, BezierCurve, TestSize.Level1)
{
    SetControlPoints();

    for (int i = 0; i <= frames_; i++) {
        float f = i / static_cast<float>(frames_);
        EXPECT_NEAR(curve_->Transform(f), expected_[i], epsilon_);
    }
}

/**
 * @tc.name: BezierAnimation
 * @tc.desc: test BezierAnimation
 * @tc.type: FUNC
 */
HWTEST_F(BezierCurveTest, BezierAnimation, TestSize.Level1)
{
    constexpr auto animationMs = 1000;
    constexpr float frameMs = animationMs / static_cast<float>(frames_);

    // Animate from 13.f->47.f
    constexpr auto start = 13.f;
    constexpr auto end = 47.f;
    constexpr auto diff = end - start;

    SetControlPoints();

    // Check that if we assign a bezier to an animation, the animated value follows the bezier as expected.
    auto property = META_NS::ConstructProperty<float>("Prop", 0.f);
    META_NS::KeyframeAnimation<float> animation(CreateNew);
    animation.SetFrom(start).SetTo(end).SetProperty(property).SetDuration(1_s).SetCurve(bezier_);

    animation.Start();
    StepAnimations({ animation }, frames_, frameMs, [&](uint32_t frame) {
        auto expected = start + expected_[frame - 1] * diff;
        EXPECT_NEAR(GetValue(property), expected, 0.1f) << "Frame: " << frame;
    });
}

META_END_NAMESPACE()
