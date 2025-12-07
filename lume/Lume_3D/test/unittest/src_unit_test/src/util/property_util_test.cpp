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

#include <base/math/matrix.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <core/property_tools/core_metadata.inl>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
#include "util/property_util.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

/**
 * @tc.name: GetPropertyTypeDeclarationTest
 * @tc.desc: Tests for Get Property Type Declaration Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilCustomPropertyPodHelper, GetPropertyTypeDeclarationTest, testing::ext::TestSize.Level1)
{
    EXPECT_EQ(PropertyType::INT32_T, CustomPropertyPodHelper::GetPropertyTypeDeclaration("int"));
    EXPECT_EQ(PropertyType::UINT32_T, CustomPropertyPodHelper::GetPropertyTypeDeclaration("uint"));
    EXPECT_EQ(PropertyType::FLOAT_T, CustomPropertyPodHelper::GetPropertyTypeDeclaration("float"));
    EXPECT_EQ(PropertyType::BOOL_T, CustomPropertyPodHelper::GetPropertyTypeDeclaration("bool"));
    EXPECT_EQ(PropertyType::VEC2_T, CustomPropertyPodHelper::GetPropertyTypeDeclaration("vec2"));
    EXPECT_EQ(PropertyType::UVEC2_T, CustomPropertyPodHelper::GetPropertyTypeDeclaration("uvec2"));
    EXPECT_EQ(PropertyType::IVEC2_T, CustomPropertyPodHelper::GetPropertyTypeDeclaration("ivec2"));
    EXPECT_EQ(PropertyType::VEC3_T, CustomPropertyPodHelper::GetPropertyTypeDeclaration("vec3"));
    EXPECT_EQ(PropertyType::UVEC3_T, CustomPropertyPodHelper::GetPropertyTypeDeclaration("uvec3"));
    EXPECT_EQ(PropertyType::IVEC3_T, CustomPropertyPodHelper::GetPropertyTypeDeclaration("ivec3"));
    EXPECT_EQ(PropertyType::VEC4_T, CustomPropertyPodHelper::GetPropertyTypeDeclaration("vec4"));
    EXPECT_EQ(PropertyType::UVEC4_T, CustomPropertyPodHelper::GetPropertyTypeDeclaration("uvec4"));
    EXPECT_EQ(PropertyType::IVEC4_T, CustomPropertyPodHelper::GetPropertyTypeDeclaration("ivec4"));
    EXPECT_EQ(PropertyType::MAT3X3_T, CustomPropertyPodHelper::GetPropertyTypeDeclaration("mat3x3"));
    EXPECT_EQ(PropertyType::MAT4X4_T, CustomPropertyPodHelper::GetPropertyTypeDeclaration("mat4x4"));
    EXPECT_EQ(PropertyType::INVALID, CustomPropertyPodHelper::GetPropertyTypeDeclaration(""));
    EXPECT_EQ(PropertyType::INVALID, CustomPropertyPodHelper::GetPropertyTypeDeclaration("abcd"));
    EXPECT_EQ(PropertyType::INVALID, CustomPropertyPodHelper::GetPropertyTypeDeclaration("-1"));
}

class PropertySignal : public CustomPropertyWriteSignal {
public:
    PropertySignal() = default;
    ~PropertySignal() = default;

    void Signal()
    {
        ++writes_;
    }

    uint32_t writes_ { 0U };
};

/**
 * @tc.name: GetSetValueTest
 * @tc.desc: Tests for Get Set Value Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilCustomPropertyPodContainer, GetSetValueTest, testing::ext::TestSize.Level1)
{
    PropertySignal signal;
    CustomPropertyPodContainer podContainer { signal, 16u };
    EXPECT_EQ(0u, podContainer.GetByteSize());
    EXPECT_EQ(0u, podContainer.Size());
    EXPECT_EQ(0u, podContainer.PropertyCount());
    EXPECT_EQ(nullptr, podContainer.MetaData(0u));
    EXPECT_EQ(0, podContainer.Type());
    EXPECT_EQ(nullptr, podContainer.Create());
    EXPECT_EQ(nullptr, podContainer.Clone(nullptr));
    podContainer.Release(nullptr);
    podContainer.WLock();
    podContainer.WUnlock();
    EXPECT_EQ(1U, signal.writes_);

    podContainer.RLock();
    podContainer.RUnlock();

    podContainer.ReservePropertyCount(3);

    constexpr const float floatVal = 1.0f;
    podContainer.AddOffsetProperty("floatProp", "floatProp", 0u,
        CustomPropertyPodHelper::GetPropertyTypeDeclaration("float"),
        { reinterpret_cast<const uint8_t*>(&floatVal), sizeof(float) });

    constexpr const int intVal = 4;
    podContainer.AddOffsetProperty(
        "intProp0", "intProp0", 4u, CustomPropertyPodHelper::GetPropertyTypeDeclaration("int"));
    podContainer.AddOffsetProperty("intProp1", "intProp1", 8u,
        CustomPropertyPodHelper::GetPropertyTypeDeclaration("int"),
        { reinterpret_cast<const uint8_t*>(&intVal), sizeof(int) });

    // Only 3 properties are reserved so these properties will not be added
    podContainer.AddOffsetProperty(
        "intProp2", "intProp2", 12u, CustomPropertyPodHelper::GetPropertyTypeDeclaration("int"));
    podContainer.AddOffsetProperty("floatProp2", "floatProp2", 16u,
        CustomPropertyPodHelper::GetPropertyTypeDeclaration("float"),
        { reinterpret_cast<const uint8_t*>(&floatVal), sizeof(float) });

    EXPECT_EQ(floatVal, *reinterpret_cast<const float*>(podContainer.GetValue("floatProp").data()));
    EXPECT_EQ(intVal, *reinterpret_cast<const int*>(podContainer.GetValue("intProp1").data()));
    EXPECT_EQ(0, podContainer.GetValue("intProp2").size());
    EXPECT_EQ(0, podContainer.GetValue("floatProp2").size());
    EXPECT_EQ(3, podContainer.MetaData().size());
    EXPECT_NE(nullptr, podContainer.MetaData(2u));

    {
        int prop = 3;
        // set via array_view
        EXPECT_TRUE(podContainer.SetValue("intProp0", { reinterpret_cast<const uint8_t*>(&prop), sizeof(int) }));
        EXPECT_EQ(prop, *reinterpret_cast<const int*>(podContainer.GetValue("intProp0").data()));
        prop = 4;
        // set by value
        EXPECT_TRUE(podContainer.SetValue("intProp0", prop));
        EXPECT_EQ(2U, signal.writes_);
        EXPECT_EQ(prop, (podContainer.GetValue<int>("intProp0")));
        // type wrong
        EXPECT_FALSE(podContainer.SetValue("intProp0", 2.f));
        // name unknown
        EXPECT_FALSE(podContainer.SetValue("intProp2", { reinterpret_cast<const uint8_t*>(&prop), sizeof(int) }));
        EXPECT_FALSE(podContainer.SetValue("intProp2", prop));
    }
    {
        float prop = 2.0f;
        // set via array_view
        EXPECT_TRUE(podContainer.SetValue(0u, { reinterpret_cast<const uint8_t*>(&prop), sizeof(float) }));
        EXPECT_EQ(prop, *reinterpret_cast<const float*>(podContainer.GetValue("floatProp").data()));
        prop = 3.f;
        // set by value
        EXPECT_TRUE(podContainer.SetValue("floatProp", prop));
        EXPECT_EQ(3U, signal.writes_);
        EXPECT_EQ(prop, (podContainer.GetValue<float>("floatProp")));
        // type wrong
        EXPECT_FALSE(podContainer.SetValue("floatProp", 2U));
        // offset out of bounds
        EXPECT_FALSE(podContainer.SetValue(16u, { reinterpret_cast<const uint8_t*>(&prop), sizeof(float) }));
    }
}

/**
 * @tc.name: SetCustomPropertyBlobValue
 * @tc.desc: Tests for Set Custom Property Blob Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UtilCustomPropertyPodHelper, SetCustomPropertyBlobValue, testing::ext::TestSize.Level1)
{
    PropertySignal signal;
    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("int");
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        string jsonString = "5";
        auto jsonValue = json::parse(jsonString.data());
        CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        EXPECT_EQ(5, *reinterpret_cast<const int*>(podContainer.GetValue("prop").data()));
    }
    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("uint");
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        string jsonString = "5";
        auto jsonValue = json::parse(jsonString.data());
        CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        EXPECT_EQ(5u, *reinterpret_cast<const uint32_t*>(podContainer.GetValue("prop").data()));
    }
    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("float");
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        string jsonString = "5.0";
        auto jsonValue = json::parse(jsonString.data());
        CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        EXPECT_EQ(5.0, *reinterpret_cast<const float*>(podContainer.GetValue("prop").data()));
    }
    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("bool");
        auto align = CustomPropertyPodHelper::GetPropertyTypeAlignment(propertyType);
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        podContainer.AddOffsetProperty("prop2", "prop2", align, propertyType);
        {
            const auto jsonString = "true";
            auto jsonValue = json::parse(jsonString);
            CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        }
        {
            const auto jsonString = "false";
            auto jsonValue = json::parse(jsonString);
            CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, align);
        }
        const auto result = podContainer.GetValue<bool>("prop2");
        EXPECT_FALSE(result);
    }

    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("vec2");
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        string jsonString = "[5.0, 5.0]";
        auto jsonValue = json::parse(jsonString.data());
        CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        const Math::Vec2* result = reinterpret_cast<const Math::Vec2*>(podContainer.GetValue("prop").data());
        EXPECT_EQ(5.0, result->x);
        EXPECT_EQ(5.0, result->y);
    }
    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("vec3");
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        string jsonString = "[5.0, 5.0, 5.0]";
        auto jsonValue = json::parse(jsonString.data());
        CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        const Math::Vec3* result = reinterpret_cast<const Math::Vec3*>(podContainer.GetValue("prop").data());
        EXPECT_EQ(5.0, result->x);
        EXPECT_EQ(5.0, result->y);
        EXPECT_EQ(5.0, result->z);
    }
    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("vec4");
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        string jsonString = "[5.0, 5.0, 5.0, 5.0]";
        auto jsonValue = json::parse(jsonString.data());
        CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        const Math::Vec4* result = reinterpret_cast<const Math::Vec4*>(podContainer.GetValue("prop").data());
        EXPECT_EQ(5.0, result->x);
        EXPECT_EQ(5.0, result->y);
        EXPECT_EQ(5.0, result->z);
        EXPECT_EQ(5.0, result->w);
    }
    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("ivec2");
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        string jsonString = "[5, 5]";
        auto jsonValue = json::parse(jsonString.data());
        CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        const Math::IVec2* result = reinterpret_cast<const Math::IVec2*>(podContainer.GetValue("prop").data());
        EXPECT_EQ(5, result->x);
        EXPECT_EQ(5, result->y);
    }
    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("ivec3");
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        string jsonString = "[5, 5, 5]";
        auto jsonValue = json::parse(jsonString.data());
        CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        const Math::IVec3* result = reinterpret_cast<const Math::IVec3*>(podContainer.GetValue("prop").data());
        EXPECT_EQ(5, result->x);
        EXPECT_EQ(5, result->y);
        EXPECT_EQ(5, result->z);
    }
    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("ivec4");
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        string jsonString = "[5, 5, 5, 5]";
        auto jsonValue = json::parse(jsonString.data());
        CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        const Math::IVec4* result = reinterpret_cast<const Math::IVec4*>(podContainer.GetValue("prop").data());
        EXPECT_EQ(5, result->x);
        EXPECT_EQ(5, result->y);
        EXPECT_EQ(5, result->z);
        EXPECT_EQ(5, result->w);
    }
    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("uvec2");
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        string jsonString = "[5, 5]";
        auto jsonValue = json::parse(jsonString.data());
        CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        const Math::UVec2* result = reinterpret_cast<const Math::UVec2*>(podContainer.GetValue("prop").data());
        EXPECT_EQ(5u, result->x);
        EXPECT_EQ(5u, result->y);
    }
    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("uvec3");
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        string jsonString = "[5, 5, 5]";
        auto jsonValue = json::parse(jsonString.data());
        CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        const Math::UVec3* result = reinterpret_cast<const Math::UVec3*>(podContainer.GetValue("prop").data());
        EXPECT_EQ(5u, result->x);
        EXPECT_EQ(5u, result->y);
        EXPECT_EQ(5u, result->z);
    }
    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("uvec4");
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        string jsonString = "[5, 5, 5, 5]";
        auto jsonValue = json::parse(jsonString.data());
        CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        const Math::UVec4* result = reinterpret_cast<const Math::UVec4*>(podContainer.GetValue("prop").data());
        EXPECT_EQ(5u, result->x);
        EXPECT_EQ(5u, result->y);
        EXPECT_EQ(5u, result->z);
        EXPECT_EQ(5u, result->w);
    }

    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("mat3x3");
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        string jsonString = "[1, 2, 3, 4, 5, 6, 7, 8, 9]";
        auto jsonValue = json::parse(jsonString.data());
        CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        const Math::Mat3X3* result = reinterpret_cast<const Math::Mat3X3*>(podContainer.GetValue("prop").data());
        EXPECT_EQ(*result, Math::Mat3X3({ 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 }));
    }
    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("mat4x4");
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        string jsonString = "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]";
        auto jsonValue = json::parse(jsonString.data());
        CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        const Math::Mat4X4* result = reinterpret_cast<const Math::Mat4X4*>(podContainer.GetValue("prop").data());
        EXPECT_EQ(*result, Math::Mat4X4({ 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { 9, 10, 11, 12 }, { 13, 14, 15, 16 }));
    }
    {
        CustomPropertyPodContainer podContainer { signal, 16u };
        podContainer.ReservePropertyCount(1u);
        auto propertyType = CustomPropertyPodHelper::GetPropertyTypeDeclaration("-1");
        podContainer.AddOffsetProperty("prop", "prop", 0u, propertyType);
        string jsonString = "5";
        auto jsonValue = json::parse(jsonString.data());
        CustomPropertyPodHelper::SetCustomPropertyBlobValue(propertyType, &jsonValue, podContainer, 0u);
        EXPECT_EQ(0, podContainer.GetValue("prop").size());
    }
}
