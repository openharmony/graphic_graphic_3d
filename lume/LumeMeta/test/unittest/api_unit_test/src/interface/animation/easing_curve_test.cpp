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

#include <meta/api/curves.h>
#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

namespace UTest {

class API_EasingCurveTest : public ::testing::TestWithParam<ClassInfo> {
public:
    API_EasingCurveTest() = default;

protected:
    void SetUp() override
    {
        curve_ = GetObjectRegistry().Create<IEasingCurve>(GetParam());
        ASSERT_TRUE(curve_);
    }

    IEasingCurve::Ptr GetCurve() const
    {
        return curve_;
    }

private:
    IEasingCurve::Ptr curve_;
};

/**
 * @tc.name: Transform
 * @tc.desc: Tests for Transform. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_EasingCurveTest, Transform, testing::ext::TestSize.Level1)
{
    auto curve = this->GetCurve();
    curve->Transform(-1);
    curve->Transform(0);
    curve->Transform(0.5);
    curve->Transform(1);
    curve->Transform(2);
}

INSTANTIATE_TEST_SUITE_P(API_EasingCurveTest, API_EasingCurveTest,
    ::testing::Values(ClassId::LinearEasingCurve, ClassId::InQuadEasingCurve, ClassId::OutQuadEasingCurve,
        ClassId::InOutQuadEasingCurve, ClassId::InCubicEasingCurve, ClassId::OutCubicEasingCurve,
        ClassId::InOutCubicEasingCurve, ClassId::InSineEasingCurve, ClassId::OutSineEasingCurve,
        ClassId::InOutSineEasingCurve, ClassId::InQuartEasingCurve, ClassId::OutQuartEasingCurve,
        ClassId::InOutQuartEasingCurve, ClassId::InQuintEasingCurve, ClassId::OutQuintEasingCurve,
        ClassId::InOutQuintEasingCurve, ClassId::InExpoEasingCurve, ClassId::OutExpoEasingCurve,
        ClassId::InOutExpoEasingCurve, ClassId::InCircEasingCurve, ClassId::OutCircEasingCurve,
        ClassId::InOutCircEasingCurve, ClassId::InBackEasingCurve, ClassId::OutBackEasingCurve,
        ClassId::InOutBackEasingCurve, ClassId::InElasticEasingCurve, ClassId::OutElasticEasingCurve,
        ClassId::InOutElasticEasingCurve, ClassId::InBounceEasingCurve, ClassId::OutBounceEasingCurve,
        ClassId::InOutBounceEasingCurve, ClassId::StepStartEasingCurve, ClassId::StepEndEasingCurve));

} // namespace UTest

META_END_NAMESPACE()
