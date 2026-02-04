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

#include <base/util/color.h>

#include "test_framework.h"

/**
 * @tc.name: T1SRGBToLinearConv
 * @tc.desc: Tests for T1Srgbto Linear Conv. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T1SRGBToLinearConv, testing::ext::TestSize.Level1)
{
    // should give near 0.214041144 (depending FPE)
    float result = BASE_NS::SRGBToLinearConv(0.5f);
    EXPECT_FLOAT_EQ(result, 0.214041144f);
}

/**
 * @tc.name: T1SRGBToLinearConvSmall
 * @tc.desc: Tests for T1Srgbto Linear Conv Small. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T1SRGBToLinearConvSmall, testing::ext::TestSize.Level1)
{
    float result = BASE_NS::SRGBToLinearConv(0.01f);
    EXPECT_FLOAT_EQ(result * 12.92f, 0.01f);
}

/**
 * @tc.name: T2LinearToSRGBConv
 * @tc.desc: Tests for T2Linear To Srgbconv. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T2LinearToSRGBConv, testing::ext::TestSize.Level1)
{
    float result = BASE_NS::LinearToSRGBConv(0.5f);
    EXPECT_FLOAT_EQ(result, 0.735356927f);
}

/**
 * @tc.name: T3MakeColorFromLinear
 * @tc.desc: Tests for T3Make Color From Linear. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T3MakeColorFromLinear, testing::ext::TestSize.Level1)
{
    const uint32_t srcCol { 0x00eb3425 };
    BASE_NS::Color dstCol = BASE_NS::MakeColorFromLinear(srcCol);
    EXPECT_FLOAT_EQ(dstCol.x, 0.921568632f);
    EXPECT_FLOAT_EQ(dstCol.y, 0.203921571f);
    EXPECT_FLOAT_EQ(dstCol.z, 0.145098045f);
    EXPECT_FLOAT_EQ(dstCol.w, 0.0f);
}

/**
 * @tc.name: T4MakeColorFromSRGB
 * @tc.desc: Tests for T4Make Color From Srgb. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T4MakeColorFromSRGB, testing::ext::TestSize.Level1)
{
    const uint32_t srcCol { 0x00eb3425 };
    BASE_NS::Color dstCol = BASE_NS::MakeColorFromSRGB(srcCol);
    EXPECT_FLOAT_EQ(dstCol.x, 0.830769956f);
    EXPECT_FLOAT_EQ(dstCol.y, 0.0343398005f);
    EXPECT_FLOAT_EQ(dstCol.z, 0.0185002182f);
    EXPECT_FLOAT_EQ(dstCol.w, 0.0f);
}

/**
 * @tc.name: T6FromColorToLinear
 * @tc.desc: Tests for T6From Color To Linear. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T6FromColorToLinear, testing::ext::TestSize.Level1)
{
    BASE_NS::Color srcCol { 0.921568632f, 0.203921571f, 0.145098045f, 0.0f };
    uint32_t dstCol = BASE_NS::FromColorToLinear(srcCol);
    EXPECT_EQ(dstCol, 0x00eb3425);
}

/**
 * @tc.name: T7FromColorToSRGB
 * @tc.desc: Tests for T7From Color To Srgb. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T7FromColorToSRGB, testing::ext::TestSize.Level1)
{
    BASE_NS::Color srcCol { 0.830769956f, 0.0343398005f, 0.0185002182f, 0.0f };
    uint32_t dstCol = BASE_NS::FromColorToSRGB(srcCol);
    EXPECT_EQ(dstCol, 0x00eb3325);
}
/**
 * @tc.name: T7FromColorToSRGBmaxValue
 * @tc.desc: Tests for T7From Color To Srgbmax Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T7FromColorToSRGBmaxValue, testing::ext::TestSize.Level1)
{
    BASE_NS::Color srcCol { 1.0f, 1.0f, 1.0f, 1.0f };
    uint32_t dstCol = BASE_NS::FromColorToSRGB(srcCol);
    EXPECT_EQ(dstCol, 0xffffffff);
}
/**
 * @tc.name: T7FromColorToSRGBminValue
 * @tc.desc: Tests for T7From Color To Srgbmin Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T7FromColorToSRGBminValue, testing::ext::TestSize.Level1)
{
    BASE_NS::Color srcCol { .0f, .0f, .0f, .0f };
    uint32_t dstCol = BASE_NS::FromColorToSRGB(srcCol);
    EXPECT_EQ(dstCol, 0x00000000);
}

/**
 * @tc.name: T9FromColorRGBAToLinear
 * @tc.desc: Tests for T9From Color Rgbato Linear. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T9FromColorRGBAToLinear, testing::ext::TestSize.Level1)
{
    BASE_NS::Color srcCol { 0.25f, 0.5f, 0.75f, 1.0f };
    uint32_t dstCol = BASE_NS::FromColorRGBAToLinear(srcCol);
    EXPECT_EQ(dstCol, 0xffbf7f3f); //TODO: Check if roundings are correct
}

/**
 * @tc.name: T10FromColorRGBAToSRGB
 * @tc.desc: Tests for T10From Color Rgbato Srgb. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T10FromColorRGBAToSRGB, testing::ext::TestSize.Level1)
{
    BASE_NS::Color srcCol { 0.25f, 0.5f, 0.75f, 1.0f }; // 0.75 = 223.74, 0.5 = 186, 0.25 = 135.79
    uint32_t dstCol = BASE_NS::FromColorRGBAToSRGB(srcCol);
    EXPECT_EQ(dstCol, 0xffe0bb88); //TODO: Check if roundings are correct
}

/**
 * @tc.name: T11FromColorRGBA
 * @tc.desc: Tests for T11From Color Rgba. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T11FromColorRGBA, testing::ext::TestSize.Level1)
{
    BASE_NS::Color srcCol { 0.25f, 0.5f, 0.75f, 1.0f };
    uint32_t dstCol = BASE_NS::FromColorRGBA(srcCol, 0U);
    EXPECT_EQ(dstCol, 0xffbf7f3f); // result of T9

    uint32_t dstCol2 = BASE_NS::FromColorRGBA(srcCol, 1U);
    EXPECT_EQ(dstCol2, 0xffe0bb88); // result of T10
}

/**
 * @tc.name: T12ConstructFromARGB
 * @tc.desc: Tests for T12Construct From Argb. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T12ConstructFromARGB, testing::ext::TestSize.Level1)
{
    const uint32_t srcCol { 0xAA112233 };
    BASE_NS::Color color(srcCol);
    EXPECT_FLOAT_EQ(color.x, 0x11 / 255.f); // R
    EXPECT_FLOAT_EQ(color.y, 0x22 / 255.f); // G
    EXPECT_FLOAT_EQ(color.z, 0x33 / 255.f); // B
    EXPECT_FLOAT_EQ(color.w, 0xAA / 255.f); // A
}

/**
 * @tc.name: T13OperatorPlus
 * @tc.desc: Tests for T13Operator Plus. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T13OperatorPlus, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color1(0.1f, 0.2f, 0.3f, 0.4f);
    BASE_NS::Color color2(0.5f, 0.6f, 0.7f, 0.8f);
    BASE_NS::Color result = color1 + color2;
    EXPECT_FLOAT_EQ(result.x, 0.6f);
    EXPECT_FLOAT_EQ(result.y, 0.8f);
    EXPECT_FLOAT_EQ(result.z, 1.0f);
    EXPECT_FLOAT_EQ(result.w, 1.2f);
}

/**
 * @tc.name: T14OperatorMinus
 * @tc.desc: Tests for T14Operator Minus. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T14OperatorMinus, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color1(0.5f, 0.6f, 0.7f, 0.8f);
    BASE_NS::Color color2(0.1f, 0.2f, 0.3f, 0.4f);
    BASE_NS::Color result = color1 - color2;
    EXPECT_FLOAT_EQ(result.x, 0.4f);
    EXPECT_FLOAT_EQ(result.y, 0.4f);
    EXPECT_FLOAT_EQ(result.z, 0.4f);
    EXPECT_FLOAT_EQ(result.w, 0.4f);
}

/**
 * @tc.name: T15OperatorMultiply
 * @tc.desc: Tests for T15Operator Multiply. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T15OperatorMultiply, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color1(0.1f, 0.2f, 0.3f, 0.4f);
    BASE_NS::Color color2(0.5f, 0.6f, 0.7f, 0.8f);
    BASE_NS::Color result = color1 * color2;
    EXPECT_FLOAT_EQ(result.x, 0.05f);
    EXPECT_FLOAT_EQ(result.y, 0.12f);
    EXPECT_FLOAT_EQ(result.z, 0.21f);
    EXPECT_FLOAT_EQ(result.w, 0.32f);
}

/**
 * @tc.name: T16OperatorDivision
 * @tc.desc: Tests for T16Operator Division. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T16OperatorDivision, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color1(0.1f, 0.2f, 0.3f, 0.4f);
    {
        BASE_NS::Color color2(0.0f, 0.6f, 0.5f, 0.8f);
        BASE_NS::Color result = color1 / color2;
        EXPECT_FLOAT_EQ(result.x, 0.0f); // attempt to divide by 0
        EXPECT_FLOAT_EQ(result.y, 0.3333333f);
        EXPECT_FLOAT_EQ(result.z, 0.6f);
        EXPECT_FLOAT_EQ(result.w, 0.5f);
    }
    {
        BASE_NS::Color color2(0.5f, 0.0f, 0.5f, 0.8f);
        BASE_NS::Color result = color1 / color2;
        EXPECT_FLOAT_EQ(result.x, 0.2f);
        EXPECT_FLOAT_EQ(result.y, 0.0f); // attempt to divide by 0
        EXPECT_FLOAT_EQ(result.z, 0.6f);
        EXPECT_FLOAT_EQ(result.w, 0.5f);
    }
    {
        BASE_NS::Color color2(0.5f, 0.6f, 0.0f, 0.8f);
        BASE_NS::Color result = color1 / color2;
        EXPECT_FLOAT_EQ(result.x, 0.2f);
        EXPECT_FLOAT_EQ(result.y, 0.3333333f);
        EXPECT_FLOAT_EQ(result.z, 0.0f); // attempt to divide by 0
        EXPECT_FLOAT_EQ(result.w, 0.5f);
    }
    {
        BASE_NS::Color color2(0.5f, 0.6f, 0.5f, 0.0f);
        BASE_NS::Color result = color1 / color2;
        EXPECT_FLOAT_EQ(result.x, 0.2f);
        EXPECT_FLOAT_EQ(result.y, 0.3333333f);
        EXPECT_FLOAT_EQ(result.z, 0.6f);
        EXPECT_FLOAT_EQ(result.w, 0.0f); // attempt to divide by 0
    }
}

/**
 * @tc.name: T17OperatorMultiplyScalar
 * @tc.desc: Tests for T17Operator Multiply Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T17OperatorMultiplyScalar, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color(0.1f, 0.2f, 0.3f, 0.4f);
    const float scalar = 2.5f;
    BASE_NS::Color result = color * scalar;
    EXPECT_FLOAT_EQ(result.x, 0.25f);
    EXPECT_FLOAT_EQ(result.y, 0.5f);
    EXPECT_FLOAT_EQ(result.z, 0.75f);
    EXPECT_FLOAT_EQ(result.w, 1.0f);
}

/**
 * @tc.name: T18OperatorDivisionScalar
 * @tc.desc: Tests for T18Operator Division Scalar. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T18OperatorDivisionScalar, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color(0.1f, 0.2f, 0.3f, 0.4f);
    const float scalar = 2.5f;
    {
        BASE_NS::Color result = color / scalar;
        EXPECT_FLOAT_EQ(result.x, 0.04f);
        EXPECT_FLOAT_EQ(result.y, 0.08f);
        EXPECT_FLOAT_EQ(result.z, 0.12f);
        EXPECT_FLOAT_EQ(result.w, 0.16f);
    }
    {
        BASE_NS::Color result = color / 0.f;
        EXPECT_FLOAT_EQ(result.x, 0.1f);
        EXPECT_FLOAT_EQ(result.y, 0.2f);
        EXPECT_FLOAT_EQ(result.z, 0.3f);
        EXPECT_FLOAT_EQ(result.w, 0.4f);
    }
}

/**
 * @tc.name: T19OperatorIsEqual
 * @tc.desc: Tests for T19Operator Is Equal. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T19OperatorIsEqual, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color1(0.1f, 0.2f, 0.3f, 0.4f);
    BASE_NS::Color color2(0.1f, 0.2f, 0.3f, 0.4f);
    bool result = color1 == color2;
    EXPECT_TRUE(result);
    BASE_NS::Color color3(0.5f, 0.2f, 0.3f, 0.4f);
    result = color1 == color3;
    EXPECT_FALSE(result);
    BASE_NS::Color color4(0.1f, 0.6f, 0.3f, 0.4f);
    result = color1 == color4;
    EXPECT_FALSE(result);
    BASE_NS::Color color5(0.1f, 0.2f, 0.7f, 0.4f);
    result = color1 == color5;
    EXPECT_FALSE(result);
    BASE_NS::Color color6(0.1f, 0.2f, 0.3f, 0.8f);
    result = color1 == color6;
    EXPECT_FALSE(result);
}

/**
 * @tc.name: T20OperatorIsNotEqual
 * @tc.desc: Tests for T20Operator Is Not Equal. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T20OperatorIsNotEqual, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color1(0.1f, 0.2f, 0.3f, 0.4f);
    BASE_NS::Color color2(0.1f, 0.2f, 0.3f, 0.4f);
    bool result = color1 != color2;
    EXPECT_FALSE(result);
    BASE_NS::Color color3(0.5f, 0.2f, 0.3f, 0.4f);
    result = color1 != color3;
    EXPECT_TRUE(result);
    BASE_NS::Color color4(0.1f, 0.6f, 0.3f, 0.4f);
    result = color1 != color4;
    EXPECT_TRUE(result);
    BASE_NS::Color color5(0.1f, 0.2f, 0.6f, 0.4f);
    result = color1 != color5;
    EXPECT_TRUE(result);
    BASE_NS::Color color6(0.1f, 0.2f, 0.3f, 0.7f);
    result = color1 != color6;
    EXPECT_TRUE(result);
}

/**
 * @tc.name: T21OperatorIndex
 * @tc.desc: Tests for T21Operator Index. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T21OperatorIndex, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color(0.1f, 0.2f, 0.3f, 0.4f);
    EXPECT_FLOAT_EQ(color[0], 0.1f);
    EXPECT_FLOAT_EQ(color[1], 0.2f);
    EXPECT_FLOAT_EQ(color[2], 0.3f);
    EXPECT_FLOAT_EQ(color[3], 0.4f);
    EXPECT_FLOAT_EQ(color[4], 0.1f); // out-of-bounds returns the first element

    color[1] = 0.5f;
    EXPECT_FLOAT_EQ(color.y, 0.5f);
}

/**
 * @tc.name: T22GetLerp
 * @tc.desc: Tests for T22Get Lerp. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T22GetLerp, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color1(0.1f, 0.2f, 0.3f, 0.4f);
    BASE_NS::Color color2(0.5f, 0.6f, 0.7f, 0.8f);
    float weight = 0.5f;
    BASE_NS::Color result = color1.GetLerp(color2, weight);
    EXPECT_FLOAT_EQ(result.x, 0.3f);
    EXPECT_FLOAT_EQ(result.y, 0.4f);
    EXPECT_FLOAT_EQ(result.z, 0.5f);
    EXPECT_FLOAT_EQ(result.w, 0.6f);
}

/**
 * @tc.name: T23GetRGBA32
 * @tc.desc: Tests for T23Get Rgba32. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T23GetRGBA32, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color(0.5f, 0.25f, 0.75f, 1.0f);
    uint32_t result = color.GetRgba32();
    uint32_t expected = 0x8040BFFF;
    EXPECT_EQ(result, expected);
}

/**
 * @tc.name: T24GetRGBA64
 * @tc.desc: Tests for T24Get Rgba64. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T24GetRGBA64, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color(0.5f, 0.25f, 0.75f, 1.0f);
    uint64_t result = color.GetRgba64();
    uint64_t expected = 0x80004000BFFFFFFF;
    EXPECT_EQ(result, expected);
}

/**
 * @tc.name: T25GetARGB32
 * @tc.desc: Tests for T25Get Argb32. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T25GetARGB32, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color(0.5f, 0.25f, 0.75f, 1.0f);
    uint32_t result = color.GetArgb32();
    uint32_t expected = 0xFF8040BF;
    EXPECT_EQ(result, expected);
}

/**
 * @tc.name: T26GetARGB64
 * @tc.desc: Tests for T26Get Argb64. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T26GetARGB64, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color(0.5f, 0.25f, 0.75f, 1.0f);
    uint64_t result = color.GetArgb64();
    uint64_t expected = 0xFFFF80004000BFFF;
    EXPECT_EQ(result, expected);
}

/**
 * @tc.name: T27GetGrayscaleSrgb
 * @tc.desc: Tests for T27Get Grayscale Srgb. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T27GetGrayscaleSrgb, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color(0.5f, 0.25f, 0.75f, 1.0f);
    BASE_NS::Color result = color.GetGrayscaleSrgb();
    float expectedLuminance = 0.5f * 0.2126f + 0.25f * 0.7152f + 0.75f * 0.0722f;
    EXPECT_FLOAT_EQ(result.x, expectedLuminance);
    EXPECT_FLOAT_EQ(result.y, expectedLuminance);
    EXPECT_FLOAT_EQ(result.z, expectedLuminance);
    EXPECT_FLOAT_EQ(result.w, 1.0f);
}

/**
 * @tc.name: T28GetGrayscaleLinear
 * @tc.desc: Tests for T28Get Grayscale Linear. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T28GetGrayscaleLinear, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color(0.5f, 0.25f, 0.75f, 1.0f);
    BASE_NS::Color result = color.GetGrayscaleLinear();
    float expectedLuminance = 0.5f * 0.299f + 0.25f * 0.587f + 0.75f * 0.114f;
    EXPECT_FLOAT_EQ(result.x, expectedLuminance);
    EXPECT_FLOAT_EQ(result.y, expectedLuminance);
    EXPECT_FLOAT_EQ(result.z, expectedLuminance);
    EXPECT_FLOAT_EQ(result.w, 1.0f);
}

/**
 * @tc.name: T29GetGamma
 * @tc.desc: Tests for T29Get Gamma. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T29GetGamma, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color(0.5f, 0.25f, 0.75f, 1.0f);
    BASE_NS::Color result = color.GetGamma();
    EXPECT_FLOAT_EQ(result.x, powf(0.5f, 1.0f / 2.2f));
    EXPECT_FLOAT_EQ(result.y, powf(0.25f, 1.0f / 2.2f));
    EXPECT_FLOAT_EQ(result.z, powf(0.75f, 1.0f / 2.2f));
    EXPECT_FLOAT_EQ(result.w, 1.0f);
}

/**
 * @tc.name: T30GetVec4
 * @tc.desc: Tests for T30Get Vec4. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T30GetVec4, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color(0.5f, 0.25f, 0.75f, 1.0f);
    BASE_NS::Math::Vec4 result = static_cast<BASE_NS::Math::Vec4>(color);
    EXPECT_FLOAT_EQ(result.x, color.x);
    EXPECT_FLOAT_EQ(result.y, color.y);
    EXPECT_FLOAT_EQ(result.z, color.z);
    EXPECT_FLOAT_EQ(result.w, color.w);
}

/**
 * @tc.name: T31XYZWRGBA
 * @tc.desc: Tests for T31Xyzwrgba. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T31XYZWRGBA, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color(0.1f, 0.2f, 0.3f, 0.4f);
    EXPECT_FLOAT_EQ(color.x, color.r);
    EXPECT_FLOAT_EQ(color.y, color.g);
    EXPECT_FLOAT_EQ(color.z, color.b);
    EXPECT_FLOAT_EQ(color.w, color.a);
}

/**
 * @tc.name: T32XYZWDATA
 * @tc.desc: Tests for T32Xyzwdata. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Color, T32XYZWDATA, testing::ext::TestSize.Level1)
{
    BASE_NS::Color color(0.1f, 0.2f, 0.3f, 0.4f);
    EXPECT_FLOAT_EQ(color.x, color.data[0]);
    EXPECT_FLOAT_EQ(color.y, color.data[1]);
    EXPECT_FLOAT_EQ(color.z, color.data[2]);
    EXPECT_FLOAT_EQ(color.w, color.data[3]);
}