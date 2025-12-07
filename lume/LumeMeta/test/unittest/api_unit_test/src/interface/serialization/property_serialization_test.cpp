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

#include <meta/api/object.h>
#include <meta/base/meta_types.h>
#include <meta/ext/any_builder.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object_fwd.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_ser_output.h>
#include <meta/interface/serialization/intf_serializable.h>

#include "helpers/serialisation_utils.h"
#include "helpers/testing_objects.h"
#include "helpers/util.h"

META_BEGIN_NAMESPACE()
namespace UTest {

/**
 * @tc.name: ExportImportWeakPtrPropertyDefault
 * @tc.desc: Tests for Export Import Weak Ptr Property Default. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, ExportImportWeakPtrPropertyDefault, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    {
        auto cont = CreateTestContainer<IContainer>();
        auto obj = CreateTestType("MyTest");
        // put the object to container so that we are able to make uri to it (to find anchor object)
        cont->Add(obj);
        auto prop1 = ConstructProperty<ITestType::WeakPtr>("test", obj);
        auto prop2 = ConstructProperty<ITestType::Ptr>("object", obj);

        auto meta = interface_cast<IMetadata>(cont);
        meta->AddProperty(prop1);
        meta->AddProperty(prop2);

        ASSERT_TRUE(ser.Export(cont));
        ser.Dump("app://test.json");
    }
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    auto p1 = obj->GetProperty<ITestType::Ptr>("object");
    auto p2 = obj->GetProperty<ITestType::WeakPtr>("test");
    ASSERT_TRUE(p1);
    ASSERT_TRUE(p2);
    ASSERT_TRUE(p2->GetValue().lock() == p1->GetValue());
}

/**
 * @tc.name: ExportImportWeakPtrPropertyValue
 * @tc.desc: Tests for Export Import Weak Ptr Property Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, ExportImportWeakPtrPropertyValue, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    {
        auto cont = CreateTestContainer<IContainer>();
        auto obj = CreateTestType("MyTest");
        // put the object to container so that we are able to make uri to it (to find anchor object)
        cont->Add(obj);
        auto prop1 = ConstructProperty<ITestType::WeakPtr>("test");
        auto prop2 = ConstructProperty<ITestType::Ptr>("object", obj);

        prop1->SetValue(obj);

        auto meta = interface_cast<IMetadata>(cont);
        meta->AddProperty(prop1);
        meta->AddProperty(prop2);

        ASSERT_TRUE(ser.Export(cont));
        ser.Dump("app://test.json");
    }
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    auto p1 = obj->GetProperty<ITestType::Ptr>("object");
    auto p2 = obj->GetProperty<ITestType::WeakPtr>("test");
    ASSERT_TRUE(p1);
    ASSERT_TRUE(p2);
    ASSERT_TRUE(p2->GetValue().lock() == p1->GetValue());
}

/**
 * @tc.name: ExportImportWeakPtrPropertyWithGlobalAnchor
 * @tc.desc: Tests for Export Import Weak Ptr Property With Global Anchor. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, DISABLED_ExportImportWeakPtrPropertyWithGlobalAnchor, testing::ext::TestSize.Level1)
{
    auto cont = CreateTestContainer<IContainer>();
    GetObjectRegistry().GetGlobalSerializationData().RegisterGlobalObject(interface_pointer_cast<IObject>(cont));
    TestSerialiser ser;
    {
        auto obj = CreateTestType("MyTest");
        // put the object to container so that we are able to make uri to it (to find anchor object)
        cont->Add(obj);
        auto prop1 = ConstructProperty<ITestType::WeakPtr>("test", obj);
        auto prop2 = ConstructProperty<ITestType::Ptr>("object", obj);

        Object object(CreateInstance(ClassId::Object));
        Metadata(object).AddProperty(prop1);
        Metadata(object).AddProperty(prop2);

        ASSERT_TRUE(ser.Export(object));
        ser.Dump("app://test.json");
    }
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);
    auto p1 = obj->GetProperty<ITestType::Ptr>("object");
    auto p2 = obj->GetProperty<ITestType::WeakPtr>("test");
    ASSERT_TRUE(p1);
    ASSERT_TRUE(p2);
    // They point to different objects because the weak ptr is serialised as ref uri, when as the ptr as object
    EXPECT_EQ(p2->GetValue().lock(), p1->GetValue());

    GetObjectRegistry().GetGlobalSerializationData().UnregisterGlobalObject(interface_pointer_cast<IObject>(cont));
}

/**
 * @tc.name: PropertyBind
 * @tc.desc: Tests for Property Bind. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, PropertyBind, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    {
        Object obj(CreateInstance(ClassId::Object));
        auto p = ConstructProperty<uint32_t>("test");
        auto source = ConstructProperty<uint32_t>("source");
        source->SetValue(2);
        EXPECT_TRUE(p->SetBind(source));
        Metadata(obj).AddProperty(p);
        Metadata(obj).AddProperty(source);
        ASSERT_TRUE(ser.Export(obj));
        ser.Dump("app://test.json");
    }
    auto obj = ser.Import<IMetadata>();
    ASSERT_TRUE(obj);

    auto p1 = obj->GetProperty<uint32_t>("test");
    auto p2 = obj->GetProperty<uint32_t>("source");
    ASSERT_TRUE(p1);
    ASSERT_TRUE(p2);
    EXPECT_EQ(p1->GetValue(), 2);
    EXPECT_EQ(p2->GetValue(), 2);
    p2->SetValue(3);
    EXPECT_EQ(p1->GetValue(), 3);
}

/**
 * @tc.name: SerialiseBind
 * @tc.desc: Tests for Serialise Bind. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_SerializationTest, SerialiseBind, testing::ext::TestSize.Level1)
{
    // serialise bind that requires resolving the bind later on when the target property owner has been serialised
    TestSerialiser ser;
    {
        Object target(CreateInstance(ClassId::Object));
        auto p1 = ConstructProperty<int>("p1");
        Metadata(target).AddProperty(p1);

        Object obj(CreateInstance(ClassId::Object));
        auto p2 = ConstructProperty<int>("p2");
        Metadata(obj).AddProperty(p2);
        AttachmentContainer(obj).Attach(target);

        ASSERT_TRUE(p2->SetBind(p1));

        p1->SetValue(2);
        EXPECT_EQ(p2->GetValue(), 2);

        ASSERT_TRUE(ser.Export(obj));
        ser.Dump("app://bind_ser_test.json");
    }

    auto nobj = ser.Import<IMetadata>();
    ASSERT_TRUE(nobj);
    auto att = interface_cast<IAttach>(nobj);
    ASSERT_TRUE(att);

    auto p2 = nobj->GetProperty<int>("p2");
    auto all = att->GetAttachments<IMetadata>();
    ASSERT_EQ(all.size(), 1);
    auto target = interface_cast<IMetadata>(all[0]);
    auto p1 = target->GetProperty<int>("p1");

    EXPECT_EQ(p2->GetValue(), 2);
    p1->SetValue(3);
    EXPECT_EQ(p2->GetValue(), 3);
}

} // namespace UTest
META_END_NAMESPACE()
