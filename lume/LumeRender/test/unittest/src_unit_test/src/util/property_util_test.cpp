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

#include <util/property_util.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace RENDER_NS;
using namespace BASE_NS;
using namespace CORE_NS;

uint32_t counter = 0;
string NextPropertyName()
{
    return string("Property" + to_string(counter++));
}

template<typename T>
void TestPropertyType(CustomPropertyPodContainer& podContainer, const PropertyTypeDecl& propType, T data)
{
    // Set data with SetValue(...)
    {
        const string name = NextPropertyName();
        podContainer.AddOffsetProperty(name, name, podContainer.GetByteSize(), propType);

        EXPECT_EQ(propType.byteSize, sizeof(data));

        const array_view<uint8_t> dataView = array_view { reinterpret_cast<uint8_t*>(&data), sizeof(T) };
        EXPECT_TRUE(podContainer.SetValue(name, dataView));

        const T& returnedData = *reinterpret_cast<const T*>(podContainer.GetValue(name).data());
        EXPECT_EQ(returnedData, data);
    }
    // Set data alongside AddOffsetProperty(...)
    {
        const string name = NextPropertyName();
        const array_view<uint8_t> dataView = array_view { reinterpret_cast<uint8_t*>(&data), sizeof(T) };
        podContainer.AddOffsetProperty(name, name, podContainer.GetByteSize(), propType, dataView);

        const T& returnedData = *reinterpret_cast<const T*>(podContainer.GetValue(name).data());
        EXPECT_EQ(returnedData, data);
    }
}

/**
 * @tc.name: PropertyUtilTest
 * @tc.desc: Tests for CustomPropertyPodContainer and CustomPropertyPodHelper utilities.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_PropertyUtil, PropertyUtilTest, testing::ext::TestSize.Level1)
{
    CustomPropertyPodContainer podContainer = {};
    CustomPropertyPodHelper podHelper;

    // Create metadata
    podContainer.ReservePropertyCount(12 * 2);
    TestPropertyType(podContainer, PropertyType::FLOAT_T, 3.14159265f);
    TestPropertyType(podContainer, PropertyType::VEC2_T, Math::Vec2(1.2f, -3.4f));
    TestPropertyType(podContainer, PropertyType::VEC3_T, Math::Vec3(5.6f, 7.8f, -9.10f));
    TestPropertyType(podContainer, PropertyType::VEC4_T, Math::Vec4(10.11f, 12.13, 14.15, 16.17));

    TestPropertyType(podContainer, PropertyType::UINT32_T, 314159265u);
    TestPropertyType(podContainer, PropertyType::UVEC2_T, Math::UVec2(123u, 456u));
    TestPropertyType(podContainer, PropertyType::UVEC3_T, Math::UVec3(789u, 1011u, 1213u));
    TestPropertyType(podContainer, PropertyType::UVEC4_T, Math::UVec4(1415u, 1617u, 1819u, 2021u));

    TestPropertyType(podContainer, PropertyType::INT32_T, -314159265);
    TestPropertyType(podContainer, PropertyType::IVEC2_T, Math::IVec2(-123, 456));
    TestPropertyType(podContainer, PropertyType::IVEC3_T, Math::IVec3(-789, 1011, -1213));
    TestPropertyType(podContainer, PropertyType::IVEC4_T, Math::IVec4(1415, -1617, 1819, -2021));

    EXPECT_EQ(podContainer.PropertyCount(), 12 * 2);

    EXPECT_NE(podContainer.MetaData(0), nullptr);
    EXPECT_EQ(podContainer.MetaData(100), nullptr);

    podContainer.Reset();
    EXPECT_EQ(podContainer.PropertyCount(), 0);

    EXPECT_FALSE(podContainer.SetValue("NonExistent", array_view<uint8_t> {}));
    EXPECT_FALSE(podContainer.SetValue(4, array_view<uint8_t> {}));
    EXPECT_EQ(podContainer.GetValue("NonExistent").data(), nullptr);
}
