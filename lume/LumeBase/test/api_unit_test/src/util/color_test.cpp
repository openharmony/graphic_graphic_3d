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

#include <gtest/gtest.h>

#include <base/util/color.h>
using namespace testing::ext;
class ColorTest : public testing::Test {
public:
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(ColorTest, SRGBToLinearConv, TestSize.Level1)
{
    // should give near 0.214041144 (depending FPE)
    float result = BASE_NS::SRGBToLinearConv(0.5f);
    EXPECT_FLOAT_EQ(result, 0.214041144f);
}

HWTEST_F(ColorTest, SRGBToLinearConvSmall, TestSize.Level1)
{
    float result = BASE_NS::SRGBToLinearConv(0.01f);
    EXPECT_FLOAT_EQ(result * 12.92f, 0.01f);
}

HWTEST_F(ColorTest, LinearToSRGBConv, TestSize.Level1)
{
    float result = BASE_NS::LinearToSRGBConv(0.5f);
    EXPECT_FLOAT_EQ(result, 0.735356927f);
}

HWTEST_F(ColorTest, MakeColorFromLinear, TestSize.Level1)
{
    const uint32_t srcCol { 0x00eb3425 };
    BASE_NS::Color dstCol = BASE_NS::MakeColorFromLinear(srcCol);
    EXPECT_FLOAT_EQ(dstCol.x, 0.921568632f);
    EXPECT_FLOAT_EQ(dstCol.y, 0.203921571f);
    EXPECT_FLOAT_EQ(dstCol.z, 0.145098045f);
    EXPECT_FLOAT_EQ(dstCol.w, 0.0f);
}

HWTEST_F(ColorTest, MakeColorFromSRGB, TestSize.Level1)
{
    const uint32_t srcCol { 0x00eb3425 };
    BASE_NS::Color dstCol = BASE_NS::MakeColorFromSRGB(srcCol);
    EXPECT_FLOAT_EQ(dstCol.x, 0.830769956f);
    EXPECT_FLOAT_EQ(dstCol.y, 0.0343398005f);
    EXPECT_FLOAT_EQ(dstCol.z, 0.0185002182f);
    EXPECT_FLOAT_EQ(dstCol.w, 0.0f);
}

HWTEST_F(ColorTest, FromColorToLinear, TestSize.Level1)
{
    BASE_NS::Color srcCol { 0.921568632f, 0.203921571f, 0.145098045f, 0.0f };
    uint32_t dstCol = BASE_NS::FromColorToLinear(srcCol);
    EXPECT_EQ(dstCol, 0x00eb3425);
}

HWTEST_F(ColorTest, FromColorToSRGB, TestSize.Level1)
{
    BASE_NS::Color srcCol { 0.830769956f, 0.0343398005f, 0.0185002182f, 0.0f };
    uint32_t dstCol = BASE_NS::FromColorToSRGB(srcCol);
    EXPECT_EQ(dstCol, 0x00eb3325);
}
HWTEST_F(ColorTest, FromColorToSRGBmaxValue, TestSize.Level1)
{
    BASE_NS::Color srcCol { 1.0f, 1.0f, 1.0f, 1.0f };
    uint32_t dstCol = BASE_NS::FromColorToSRGB(srcCol);
    EXPECT_EQ(dstCol, 0xffffffff);
}
HWTEST_F(ColorTest, FromColorToSRGBminValue, TestSize.Level1)
{
    BASE_NS::Color srcCol { .0f, .0f, .0f, .0f };
    uint32_t dstCol = BASE_NS::FromColorToSRGB(srcCol);
    EXPECT_EQ(dstCol, 0x00000000);
}

HWTEST_F(ColorTest, FromColorRGBAToLinear, TestSize.Level1)
{
    BASE_NS::Color srcCol { 0.25f, 0.5f, 0.75f, 1.0f };
    uint32_t dstCol = BASE_NS::FromColorRGBAToLinear(srcCol);
    EXPECT_EQ(dstCol, 0xffbf7f3f);
}

HWTEST_F(ColorTest, FromColorRGBAToSRGB, TestSize.Level1)
{
    BASE_NS::Color srcCol { 0.25f, 0.5f, 0.75f, 1.0f }; // 0.75 = 223.74, 0.5 = 186, 0.25 = 135.79
    uint32_t dstCol = BASE_NS::FromColorRGBAToSRGB(srcCol);
    EXPECT_EQ(dstCol, 0xffe0bb88);
}

HWTEST_F(ColorTest, FromColorRGBA, TestSize.Level1)
{
    BASE_NS::Color srcCol { 0.25f, 0.5f, 0.75f, 1.0f };
    uint32_t dstCol = BASE_NS::FromColorRGBA(srcCol, 0U);
    if (true) {
        EXPECT_EQ(dstCol, 0xffbf7f3f); // result of T9
    } else {
        // we set this to fail for long as FromColorRGBAToSRGB has a valid rounding output
        EXPECT_EQ(dstCol, 0xffe0bb88); // result of T10
    }
}