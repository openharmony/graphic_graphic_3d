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

#include <base/math/vector.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_handle_util.h>
#include <core/property_tools/core_metadata.inl>
#include <core/property_tools/property_api_impl.h>
#include <core/property_tools/property_api_impl.inl>
#include <core/property_tools/property_data.h>

#include "test_framework.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

struct OtherThing {
    BASE_NS::Math::Vec4 vec4Val;
    int intArray[2];
};

struct Thing {
    int intVal;
    BASE_NS::Math::Vec2 vec2Val;
    int otherIntVal;
    BASE_NS::vector<OtherThing> others;
};

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(OtherThing)
DATA_TYPE_METADATA(OtherThing, MEMBER_PROPERTY(vec4Val, "vec4Value", 0), MEMBER_PROPERTY(intArray, "intArray", 0))

DECLARE_PROPERTY_TYPE(BASE_NS::vector<OtherThing>);

DECLARE_PROPERTY_TYPE(Thing)
DATA_TYPE_METADATA(Thing, MEMBER_PROPERTY(intVal, "intValue", 0), MEMBER_PROPERTY(otherIntVal, "otherIntValue", 0),
    MEMBER_PROPERTY(vec2Val, "vec2Value", 0), MEMBER_PROPERTY(others, "others", 0))

CORE_END_NAMESPACE()

struct Owner : public CORE_NS::IPropertyApi {
    PROPERTY_LIST(
        Thing, componentMetadata_, MEMBER_PROPERTY(intVal, "intValue", 0), MEMBER_PROPERTY(vec2Val, "vec2Value", 0))

    size_t PropertyCount() const override
    {
        return BASE_NS::countof(componentMetadata_);
    }

    const CORE_NS::Property* MetaData(size_t index) const override
    {
        if (index < BASE_NS::countof(componentMetadata_)) {
            return &componentMetadata_[index];
        }
        return nullptr;
    }

    BASE_NS::array_view<const CORE_NS::Property> MetaData() const override
    {
        return componentMetadata_;
    }

    CORE_NS::IPropertyHandle* Create() const override
    {
        return nullptr;
    }

    CORE_NS::IPropertyHandle* Clone(const CORE_NS::IPropertyHandle* srcHandle) const override
    {
        return nullptr;
    }

    void Release(CORE_NS::IPropertyHandle* handle) const override {}

    uint64_t Type() const override
    {
        return {};
    }

    struct Impl : public CORE_NS::IPropertyHandle {
        const IPropertyApi* Owner() const override
        {
            return nullptr;
        }

        size_t Size() const override
        {
            return {};
        }

        const void* RLock() const override
        {
            return nullptr;
        }

        void RUnlock() const override {}

        void* WLock() override
        {
            return nullptr;
        }

        void WUnlock() override {}
    };
};

struct ThingProperty final : public CORE_NS::PropertyApiImpl<Thing> {
    ThingProperty(Thing* data, BASE_NS::array_view<const CORE_NS::Property> props)
        : CORE_NS::PropertyApiImpl<Thing>(data, props)
    {}
};

/**
 * @tc.name: MakeScopedHandle
 * @tc.desc: Tests for Make Scoped Handle. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_PropertyHandleUtil, MakeScopedHandle, testing::ext::TestSize.Level1)
{
    Thing thing;
    thing.others.push_back(OtherThing { BASE_NS::Math::Vec4 { 0.f, 0.f, 5.f, 6.f }, {} });
    ThingProperty thingProperty(&thing, CORE_NS::PropertyType::DataType<Thing>::MetaDataFromType());

    // non-const types (writable)
    {
        auto handle = CORE_NS::MakeScopedHandle<int>(*thingProperty.GetData(), "intVal");
        EXPECT_TRUE(handle);
        if (handle) {
            *handle = 42;
        }
    }
    {
        auto handle = CORE_NS::MakeScopedHandle<BASE_NS::Math::Vec2>(*thingProperty.GetData(), "vec2Val");
        EXPECT_TRUE(handle);
        if (handle) {
            *handle = BASE_NS::Math::Vec2(1.f, 2.f);
        }
    }
    {
        auto handle = CORE_NS::MakeScopedHandle<float>(*thingProperty.GetData(), "others[0].vec4Val.x");
        EXPECT_TRUE(handle);
        if (handle) {
            *handle = 3.f;
        }
    }
    {
        auto handle = CORE_NS::MakeScopedHandle<float>(*thingProperty.GetData(), "others[0].vec4Val.y");
        EXPECT_TRUE(handle);
        if (handle) {
            *handle = 4.f;
        }
    }
    {
        auto handle = CORE_NS::MakeScopedHandle<int>(*thingProperty.GetData(), "others[0].intArray[0]");
        EXPECT_TRUE(handle);
        if (handle) {
            *handle = 1;
        }
    }
    {
        auto handle = CORE_NS::MakeScopedHandle<int>(*thingProperty.GetData(), "others[0].intArray[1]");
        EXPECT_TRUE(handle);
        if (handle) {
            *handle = 2;
        }
    }

    // unknown propertyName
    {
        auto handle = CORE_NS::MakeScopedHandle<int>(*thingProperty.GetData(), "foo");
        EXPECT_FALSE(handle);
    }

    // propertyName doesn't match propertyType (propertyType doesn't match any of the properties)
    {
        auto handle = CORE_NS::MakeScopedHandle<BASE_NS::Math::UVec2>(*thingProperty.GetData(), "vec2Val");
        EXPECT_FALSE(handle);
    }

    // propertyName doesn't match type
    {
        auto handle = CORE_NS::MakeScopedHandle<int>(*thingProperty.GetData(), "vec2Val");
        EXPECT_FALSE(handle);
    }

    // container index out of range
    {
        auto handle = CORE_NS::MakeScopedHandle<float>(*thingProperty.GetData(), "others[1].vec4Val.x");
        EXPECT_FALSE(handle);
    }
    {
        auto handle = CORE_NS::MakeScopedHandle<int>(*thingProperty.GetData(), "others[0].intArray[2]");
        EXPECT_FALSE(handle);
    }

    // const types (readonly)
    {
        auto handle = CORE_NS::MakeScopedHandle<const int>(*thingProperty.GetData(), "intVal");
        EXPECT_TRUE(handle);
        if (handle) {
            EXPECT_EQ(*handle, 42);
        }
    }
    {
        auto handle = CORE_NS::MakeScopedHandle<const BASE_NS::Math::Vec2>(*thingProperty.GetData(), "vec2Val");
        EXPECT_TRUE(handle);
        if (handle) {
            EXPECT_FLOAT_EQ(handle->x, 1.f);
            EXPECT_FLOAT_EQ(handle->y, 2.f);
        }
    }
    {
        auto handle = CORE_NS::MakeScopedHandle<const float>(*thingProperty.GetData(), "vec2Val.x");
        EXPECT_TRUE(handle);
        if (handle) {
            EXPECT_FLOAT_EQ(*handle, 1.f);
        }
    }
    {
        auto handle = CORE_NS::MakeScopedHandle<const float>(*thingProperty.GetData(), "vec2Val.y");
        EXPECT_TRUE(handle);
        if (handle) {
            EXPECT_FLOAT_EQ(*handle, 2.f);
        }
    }
    {
        auto handle = CORE_NS::MakeScopedHandle<const float>(*thingProperty.GetData(), "others[0].vec4Val.x");
        EXPECT_TRUE(handle);
        if (handle) {
            EXPECT_FLOAT_EQ(*handle, 3.f);
        }
    }
    {
        auto handle = CORE_NS::MakeScopedHandle<const float>(*thingProperty.GetData(), "others[0].vec4Val.y");
        EXPECT_TRUE(handle);
        if (handle) {
            EXPECT_FLOAT_EQ(*handle, 4.f);
        }
    }
    {
        auto handle = CORE_NS::MakeScopedHandle<int>(*thingProperty.GetData(), "others[0].intArray[0]");
        EXPECT_TRUE(handle);
        if (handle) {
            EXPECT_EQ(*handle, 1);
        }
    }
    {
        auto handle = CORE_NS::MakeScopedHandle<int>(*thingProperty.GetData(), "others[0].intArray[1]");
        EXPECT_TRUE(handle);
        if (handle) {
            EXPECT_EQ(*handle, 2);
        }
    }
}

/**
 * @tc.name: GetSetPropertyValue
 * @tc.desc: Tests for Get Set Property Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_PropertyHandleUtil, GetSetPropertyValue, testing::ext::TestSize.Level1)
{
    Thing thing;
    thing.others.push_back(OtherThing { BASE_NS::Math::Vec4 { 0.f, 0.f, 5.f, 6.f }, {} });

    ThingProperty thingProperty(&thing, CORE_NS::PropertyType::DataType<Thing>::MetaDataFromType());

    EXPECT_TRUE(CORE_NS::SetPropertyValue(thingProperty.GetData(), "vec2Val", BASE_NS::Math::Vec2(2.f, 3.f)));

    // unknown propertyName
    EXPECT_FALSE(CORE_NS::SetPropertyValue(thingProperty.GetData(), "foo", BASE_NS::Math::Vec2(1.f, 2.f)));

    // propertyName doesn't match propertyType (propertyType doesn't match any of the properties)
    EXPECT_FALSE(CORE_NS::SetPropertyValue(thingProperty.GetData(), "vec2Val", BASE_NS::Math::UVec2(2, 3)));

    // propertyName doesn't match propertyType
    EXPECT_FALSE(CORE_NS::SetPropertyValue(thingProperty.GetData(), "intVal", BASE_NS::Math::Vec2(1.f, 2.f)));

    auto vec2Val = CORE_NS::GetPropertyValue<BASE_NS::Math::Vec2>(thingProperty.GetData(), "vec2Val");
    EXPECT_EQ(vec2Val.x, 2.f);
    EXPECT_EQ(vec2Val.y, 3.f);

    EXPECT_EQ(CORE_NS::GetPropertyValue<float>(thingProperty.GetData(), "vec2Val.x"), 2.f);
    EXPECT_EQ(CORE_NS::GetPropertyValue<float>(thingProperty.GetData(), "vec2Val.y"), 3.f);

    EXPECT_TRUE(CORE_NS::SetPropertyValue(thingProperty.GetData(), "others[0].vec4Val.y", 7.f));
    EXPECT_TRUE(CORE_NS::SetPropertyValue(thingProperty.GetData(), "others[0].intArray[1]", 7));

    EXPECT_EQ(CORE_NS::GetPropertyValue<float>(thingProperty.GetData(), "others[0].vec4Val.y"), 7.f);
    EXPECT_EQ(CORE_NS::GetPropertyValue<int>(thingProperty.GetData(), "others[0].intArray[1]"), 7);

    // set/get multiple properties with same type
    EXPECT_TRUE(CORE_NS::SetPropertyValue(thingProperty.GetData(), "intVal", 1));
    EXPECT_TRUE(CORE_NS::SetPropertyValue(thingProperty.GetData(), "otherIntVal", 2));
    EXPECT_EQ(CORE_NS::GetPropertyValue<int>(thingProperty.GetData(), "intVal"), 1);
    EXPECT_EQ(CORE_NS::GetPropertyValue<int>(thingProperty.GetData(), "otherIntVal"), 2);
}

/**
 * @tc.name: FindPropertyWithHandle
 * @tc.desc: Tests for Find Property With Handle. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilPropertyToolsTest, FindPropertyWithHandle, testing::ext::TestSize.Level1)
{
    Thing thing;
    thing.others.push_back(OtherThing { BASE_NS::Math::Vec4 { 0.f, 0.f, 5.f, 6.f }, {} });
    ThingProperty thingProperty(&thing, CORE_NS::PropertyType::DataType<Thing>::MetaDataFromType());
    {
        const auto intValueProperty = CORE_NS::PropertyData::FindProperty(*thingProperty.GetData(), "intVal");
        EXPECT_TRUE((intValueProperty));
        EXPECT_EQ(intValueProperty.offset, offsetof(Thing, intVal));
    }
    {
        const auto vec2ValueProperty = CORE_NS::PropertyData::FindProperty(*thingProperty.GetData(), "vec2Val");
        EXPECT_TRUE((vec2ValueProperty));
        EXPECT_EQ(vec2ValueProperty.offset, offsetof(Thing, vec2Val));
    }
    {
        const auto othersProperty = CORE_NS::PropertyData::FindProperty(*thingProperty.GetData(), "others");
        EXPECT_TRUE((othersProperty));
        EXPECT_EQ(othersProperty.offset, offsetof(Thing, others));
    }
    {
        const auto otherProperty = CORE_NS::PropertyData::FindProperty(*thingProperty.GetData(), "others[0]");
        EXPECT_TRUE((otherProperty));
        EXPECT_EQ(otherProperty.offset + reinterpret_cast<uintptr_t>(&thing),
            reinterpret_cast<uintptr_t>(thing.others.data()));
    }
}