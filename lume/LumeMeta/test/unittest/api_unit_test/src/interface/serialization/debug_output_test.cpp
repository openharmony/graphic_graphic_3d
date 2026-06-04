/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_importer.h>
#include <meta/interface/serialization/intf_ser_output.h>

#include "helpers/serialisation_utils.h"
#include "helpers/testing_objects.h"
#include "meta/api/object_name.h"

META_BEGIN_NAMESPACE()
namespace UTest {

namespace {

ISerOutput::Ptr CreateDebugOutput()
{
    return GetObjectRegistry().Create<ISerOutput>(ClassId::DebugOutput);
}

ISerNode::Ptr ExportAsTree(const IObject::Ptr& object)
{
    TestSerialiser ser;
    ser.Export(object);
    auto data = ser.GetData();
    MemFile file(data);
    auto importer = GetObjectRegistry().Create<IFileImporter>(ClassId::JsonImporter);
    if (!importer) {
        return {};
    }
    return importer->ImportAsTree(file);
}

}  // namespace

/**
 * @tc.name: CreateDebugOutputInstance
 * @tc.desc: Tests that DebugOutput can be created from the registry.
 * @tc.type: FUNC
 */
UNIT_TEST(API_DebugOutputTest, CreateDebugOutputInstance, testing::ext::TestSize.Level1)
{
    auto output = CreateDebugOutput();
    ASSERT_TRUE(output);
}

/**
 * @tc.name: NullTree
 * @tc.desc: Tests that Process returns empty string for null tree.
 * @tc.type: FUNC
 */
UNIT_TEST(API_DebugOutputTest, NullTree, testing::ext::TestSize.Level1)
{
    auto output = CreateDebugOutput();
    ASSERT_TRUE(output);

    auto result = output->Process(nullptr);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: SimpleObject
 * @tc.desc: Tests debug output for a simple object with a name.
 * @tc.type: FUNC
 */
UNIT_TEST(API_DebugOutputTest, SimpleObject, testing::ext::TestSize.Level1)
{
    auto output = CreateDebugOutput();
    ASSERT_TRUE(output);

    Object object(CreateInstance(ClassId::Object));
    SetName(object, "TestObj");
    auto tree = ExportAsTree(object.GetPtr<IObject>());
    ASSERT_TRUE(tree);

    auto result = output->Process(tree);
    EXPECT_FALSE(result.empty());
    // Should contain the object name somewhere in the output
    EXPECT_NE(result.find("Object"), BASE_NS::string::npos);
}

/**
 * @tc.name: ObjectWithProperties
 * @tc.desc: Tests debug output for an object with properties.
 * @tc.type: FUNC
 */
UNIT_TEST(API_DebugOutputTest, ObjectWithProperties, testing::ext::TestSize.Level1)
{
    auto output = CreateDebugOutput();
    ASSERT_TRUE(output);

    Metadata object(CreateInstance(ClassId::Object));
    SetName(object, "PropObj");
    object.AddProperty<int>("IntProp", 42);
    object.AddProperty<BASE_NS::string>("StringProp", "hello");

    auto tree = ExportAsTree(object.GetPtr<IObject>());
    ASSERT_TRUE(tree);

    auto result = output->Process(tree);
    EXPECT_FALSE(result.empty());
}

/**
 * @tc.name: MultipleProcessCalls
 * @tc.desc: Tests that DebugOutput can be created fresh for each call.
 * @tc.type: FUNC
 */
UNIT_TEST(API_DebugOutputTest, MultipleProcessCalls, testing::ext::TestSize.Level1)
{
    Object object(CreateInstance(ClassId::Object));
    SetName(object, "Obj1");
    auto tree = ExportAsTree(object.GetPtr<IObject>());
    ASSERT_TRUE(tree);

    // Each call with a fresh DebugOutput instance should work
    auto output1 = CreateDebugOutput();
    ASSERT_TRUE(output1);
    auto result1 = output1->Process(tree);
    EXPECT_FALSE(result1.empty());

    auto output2 = CreateDebugOutput();
    ASSERT_TRUE(output2);
    auto result2 = output2->Process(tree);
    EXPECT_FALSE(result2.empty());

    // Same tree should produce same output
    EXPECT_EQ(result1, result2);
}

/**
 * @tc.name: NestedObjects
 * @tc.desc: Tests debug output for nested container with children.
 * @tc.type: FUNC
 */
UNIT_TEST(API_DebugOutputTest, NestedObjects, testing::ext::TestSize.Level1)
{
    auto output = CreateDebugOutput();
    ASSERT_TRUE(output);

    auto container = CreateTestContainer<IContainer>();
    SetName(interface_pointer_cast<IObject>(container), "Parent");

    auto child = CreateTestType<IObject>("Child");
    container->Add(child);

    auto obj = interface_pointer_cast<IObject>(container);

    TestSerialiser ser;
    // Enable hierarchy serialization
    SetObjectFlags(obj, ObjectFlagBits::SERIALIZE_HIERARCHY, true);
    ser.Export(obj);
    auto data = ser.GetData();
    MemFile file(data);
    auto importer = GetObjectRegistry().Create<IFileImporter>(ClassId::JsonImporter);
    ASSERT_TRUE(importer);
    auto tree = importer->ImportAsTree(file);
    ASSERT_TRUE(tree);

    auto result = output->Process(tree);
    EXPECT_FALSE(result.empty());
    // Nested output should have indentation (tabs)
    EXPECT_NE(result.find('\t'), BASE_NS::string::npos);
}

}  // namespace UTest
META_END_NAMESPACE()
