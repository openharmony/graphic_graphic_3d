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

#include <meta/ext/object_fwd.h>

#include "helpers/serialisation_utils.h"
#include "helpers/test_data_helpers.h"
#include "helpers/test_utils.h"
#include "helpers/util.h"

META_BEGIN_NAMESPACE()
namespace UTest {

/**
 * @tc.name: TestType
 * @tc.desc: Tests for Test Type. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationCompatTest, TestType, testing::ext::TestSize.Level1)
{
    constexpr auto path = "test://metav1/test_type.json";
    TestSerialiser ser;
    ASSERT_TRUE(ser.LoadFile(path));
    auto obj = ser.Import<ITestType>();
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->First()->GetValue(), 2);
    EXPECT_EQ(obj->Second()->GetValue(), "Hipshops");
    // old system seems to export array property value even though it is just default,
    // so this makes things not equal
    // EXPECT_TRUE(IsEqual(interface_pointer_cast<IMetadata>(CreateTestType()), obj));
}

/**
 * @tc.name: TestTypeWithAttachment
 * @tc.desc: Tests for Test Type With Attachment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationCompatTest, TestTypeWithAttachment, testing::ext::TestSize.Level1)
{
    constexpr auto path = "test://metav1/test_type_with_attachment.json";
    TestSerialiser ser;
    ASSERT_TRUE(ser.LoadFile(path));
    auto obj = ser.Import<ITestType>();
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->First()->GetValue(), 2);
    EXPECT_EQ(obj->Second()->GetValue(), "Hipshops");

    auto att = interface_cast<IAttach>(obj);
    ASSERT_TRUE(att);
    auto cont = att->GetAttachments();
    IMetadata::Ptr attobj;
    for (auto&& v : cont) {
        if (!v->GetInterface(IProperty::UID)) {
            attobj = interface_pointer_cast<IMetadata>(v);
        }
    }

    ASSERT_TRUE(attobj);
    auto p = attobj->GetProperty<BASE_NS::string>("Test");
    EXPECT_EQ(p->GetValue(), "Some");
}

/**
 * @tc.name: Bind
 * @tc.desc: Tests for Bind. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationCompatTest, Bind, testing::ext::TestSize.Level1)
{
    constexpr auto path = "test://metav1/bind.json";
    TestSerialiser ser;
    ASSERT_TRUE(ser.LoadFile(path));
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);

    auto p1 = obj->GetProperty<int>("p1");
    ASSERT_TRUE(p1);
    EXPECT_EQ(p1->GetValue(), 3);
    auto p2 = obj->GetProperty<int>("p2");
    ASSERT_TRUE(p2);
    EXPECT_EQ(p2->GetValue(), 3);

    p1->SetValue(2);
    EXPECT_EQ(p1->GetValue(), 2);
    EXPECT_EQ(p2->GetValue(), 2);
}

/**
 * @tc.name: Obj1
 * @tc.desc: Tests for Obj1. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationCompatTest, Obj1, testing::ext::TestSize.Level1)
{
    constexpr auto path = "test://metav1/obj1.json";
    TestSerialiser ser;
    ASSERT_TRUE(ser.LoadFile(path));
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);

    auto p = obj->GetProperty<IObject::Ptr>("Test");
    ASSERT_TRUE(p);
    EXPECT_FALSE(p->GetValue());
};

/**
 * @tc.name: Obj2
 * @tc.desc: Tests for Obj2. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationCompatTest, Obj2, testing::ext::TestSize.Level1)
{
    constexpr auto path = "test://metav1/obj2.json";
    TestSerialiser ser;
    ASSERT_TRUE(ser.LoadFile(path));
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);

    auto p = obj->GetProperty<IObject::Ptr>("Test");
    ASSERT_TRUE(p);
    EXPECT_TRUE(p->GetValue());
};

/**
 * @tc.name: LocalStringProperties
 * @tc.desc: Tests for Local String Properties. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationCompatTest, LocalStringProperties, testing::ext::TestSize.Level1)
{
    constexpr auto path = "test://metav1/local_strings.json";
    TestSerialiser ser;
    ASSERT_TRUE(ser.LoadFile(path));
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    auto strings = obj->GetProperty<IObject::Ptr>("strings");
    ASSERT_TRUE(strings);
    auto value = interface_cast<IMetadata>(strings->GetValue());
    ASSERT_TRUE(value);
    {
        auto lang = value->GetProperty<IObject::Ptr>("en");
        ASSERT_TRUE(lang);
        auto obj = interface_cast<IMetadata>(lang->GetValue());
        ASSERT_TRUE(obj);
        auto p = obj->GetProperty<BASE_NS::string>("Text1");
        ASSERT_TRUE(p);
        EXPECT_EQ(p->GetValue(), "Text");
    }
    {
        auto lang = value->GetProperty<IObject::Ptr>("fi");
        ASSERT_TRUE(lang);
        auto obj = interface_cast<IMetadata>(lang->GetValue());
        ASSERT_TRUE(obj);
        auto p = obj->GetProperty<BASE_NS::string>("Text1");
        ASSERT_TRUE(p);
        EXPECT_EQ(p->GetValue(), "Teksti");
    }
}

// TEST(API_SerializationCompatTest, GenerateCompat)
// {
//     {
//         TestSerialiser ser;
//         auto object = CreateTestType();
//         object->First()->Set(2);
//         object->Second()->Set("Hipshops");
//         ASSERT_TRUE(ser.Export(object));
//         ser.Dump("app://test_type.json");
//     }

//     {
//         TestSerialiser ser;
//         auto object = CreateTestType();
//         object->First()->Set(2);
//         object->Second()->Set("Hipshops");
//         Object obj;
//         obj.AddProperty(ConstructProperty<BASE_NS::string>(GetObjectRegistry(), "Test", "Some"));

//         auto att = interface_cast<IAttach>(object);
//         ASSERT_TRUE(att);
//         att->Attach(obj);

//         ASSERT_TRUE(ser.Export(object));
//         ser.Dump("app://test_type_with_attachment.json");
//     }

//     {
//         TestSerialiser ser;
//         Object object;
//         object.AddProperty(ConstructProperty<IObject::Ptr>(GetObjectRegistry(), "Test", nullptr));
//         ASSERT_TRUE(ser.Export(object));
//         ser.Dump("app://obj1.json");
//     }

//     {
//         TestSerialiser ser;
//         Object object1;
//         Object object2;
//         auto p = ConstructProperty<IObject::Ptr>(GetObjectRegistry(), "Test", nullptr);
//         p->Set(object2);
//         object1.AddProperty(p);
//         ASSERT_TRUE(ser.Export(object1));
//         ser.Dump("app://obj2.json");
//     }

//     {
//         TestSerialiser ser;
//         Object object;
//         auto p1 = ConstructProperty<int>(GetObjectRegistry(), "p1", 0);
//         auto p2 = ConstructProperty<int>(GetObjectRegistry(), "p2", 0);
//         p2->Bind(p1);
//         p1->Set(3);
//         object.AddProperty(p1);
//         object.AddProperty(p2);
//         ASSERT_TRUE(ser.Export(object));
//         ser.Dump("app://bind.json");
//     }
// }

} // namespace UTest
META_END_NAMESPACE()
