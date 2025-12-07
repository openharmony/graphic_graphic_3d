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

#include <meta/api/iteration.h>
#include <meta/api/object.h>
#include <meta/interface/loaders/intf_class_content_loader.h>

#include "helpers/serialisation_utils.h"
#include "helpers/test_utils.h"
#include "helpers/testing_objects.h"
#include "helpers/util.h"

META_BEGIN_NAMESPACE()
namespace UTest {

/**
 * @tc.name: Search
 * @tc.desc: Tests for Search. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContentTest, Search, testing::ext::TestSize.Level1)
{
    auto content = CreateTestType<IObject>("Content");

    ContentObject co(CreateInstance(ClassId::ContentObject));
    co.SetContent(content);
    auto c = CreateTestContainer<IContainer>();
    c->Add(co.GetPtr());
    EXPECT_FALSE(c->FindAny({ "Content", TraversalType::NO_HIERARCHY }));
    EXPECT_EQ(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }), content);
    EXPECT_EQ(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestType::UID } }), content);
    EXPECT_EQ(c->FindAny({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestType::UID } }), content);
    EXPECT_FALSE(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER, { IContainer::UID } }));

    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 2);
    EXPECT_EQ(c->FindAll({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 1);

    EXPECT_TRUE(ContainsObjectWithName(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }), "Content"));

    co.SetContentSearchable(false);
    EXPECT_FALSE(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }));
    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 1);

    co.SetContentSearchable(true);
    EXPECT_EQ(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }), content);
    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 2);
}

/**
 * @tc.name: SearchContentContainer
 * @tc.desc: Tests for Search Content Container. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContentTest, SearchContentContainer, testing::ext::TestSize.Level1)
{
    auto ccontent = CreateTestType<IObject>("Content");
    auto content = CreateTestContainer<IContainer>();
    content->Add(ccontent);

    ContentObject co(CreateInstance(ClassId::ContentObject));
    co.SetContent(interface_pointer_cast<IObject>(content));
    auto c = CreateTestContainer<IContainer>();
    c->Add(co.GetPtr());
    EXPECT_FALSE(c->FindAny({ "Content", TraversalType::NO_HIERARCHY }));
    EXPECT_EQ(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }), ccontent);
    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 3);

    co.SetContentSearchable(false);
    EXPECT_FALSE(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }));
    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 1);

    co.SetContentSearchable(true);
    EXPECT_EQ(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }), ccontent);
    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 3);
}

/**
 * @tc.name: Iteration
 * @tc.desc: Tests for Iteration. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContentTest, Iteration, testing::ext::TestSize.Level1)
{
    auto ccontent = CreateTestType<IObject>("Content");
    auto content = CreateTestContainer<IContainer>();
    content->Add(ccontent);

    ContentObject co(CreateInstance(ClassId::ContentObject));
    co.SetContent(interface_pointer_cast<IObject>(content));
    auto c = CreateTestContainer<IContainer>();
    c->Add(co.GetPtr());

    {
        BASE_NS::vector<IObject::Ptr> vec;
        EXPECT_TRUE(ForEachShared(
            c, [&](const IObject::Ptr& obj) { vec.push_back(obj); }, TraversalType::DEPTH_FIRST_PRE_ORDER));
        EXPECT_EQ(vec.size(), 3);
        EXPECT_TRUE(ContainsObjectWithName(vec, "Content"));
    }

    co.SetContentSearchable(false);

    {
        BASE_NS::vector<IObject::Ptr> vec;
        EXPECT_TRUE(ForEachShared(
            c, [&](const IObject::Ptr& obj) { vec.push_back(obj); }, TraversalType::DEPTH_FIRST_PRE_ORDER));
        EXPECT_EQ(vec.size(), 1);
        EXPECT_FALSE(ContainsObjectWithName(vec, "Content"));
    }
}

/**
 * @tc.name: Serialization
 * @tc.desc: Tests for Serialization. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContentTest, Serialization, testing::ext::TestSize.Level1)
{
    {
        TestSerialiser ser;

        ContentObject co(CreateInstance(ClassId::ContentObject));
        co.SerializeContent(false);
        co.SetContent(CreateTestType<IObject>("Content"));
        ASSERT_TRUE(ser.Export(co));

        auto imp = ser.Import<IContent>();
        ASSERT_TRUE(imp);
        EXPECT_FALSE(imp->Content()->GetValue());
    }
    {
        TestSerialiser ser;

        ContentObject co(CreateInstance(ClassId::ContentObject));
        co.SerializeContent(true);
        co.SetContent(CreateTestType<IObject>("Content"));
        ASSERT_TRUE(ser.Export(co));

        ser.Dump("app://content_test.json");

        auto imp = ser.Import<IContent>();
        ASSERT_TRUE(imp);
        ASSERT_TRUE(imp->Content()->GetValue());
        EXPECT_EQ(imp->Content()->GetValue()->GetName(), "Content");
    }
}

/**
 * @tc.name: Defaults
 * @tc.desc: Tests for Defaults. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContentTest, Defaults, testing::ext::TestSize.Level1)
{
    ContentObject co(CreateInstance(ClassId::ContentObject));
    const auto objectFlags = interface_cast<IObjectFlags>(co);
    ASSERT_TRUE(objectFlags);
    EXPECT_FALSE(objectFlags->GetObjectFlags().IsSet(META_NS::ObjectFlagBits::SERIALIZE_HIERARCHY));
}

/**
 * @tc.name: ObjectFlagsNoHierarchy
 * @tc.desc: Tests for Object Flags No Hierarchy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContentTest, ObjectFlagsNoHierarchy, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    ContentObject co(CreateInstance(ClassId::ContentObject));
    co.SetContent(CreateTestType<IObject>("Content"));

    // By default ContentObject should not export its Content (i.e. ObjectFlagBits::SERIALIZE_HIERARCHY is not set)
    ASSERT_TRUE(ser.Export(co));

    auto imported = ser.Import<IContent>();
    ASSERT_TRUE(imported);

    EXPECT_FALSE(imported->Content()->GetValue());
}

/**
 * @tc.name: ObjectFlagsHierarchy
 * @tc.desc: Tests for Object Flags Hierarchy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContentTest, ObjectFlagsHierarchy, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    ContentObject co(CreateInstance(ClassId::ContentObject));
    co.SetContent(CreateTestType<IObject>("Content"));

    auto flagsIntfPtr = interface_cast<IObjectFlags>(co);
    ASSERT_TRUE(flagsIntfPtr);
    SetObjectFlags(co.GetPtr(), ObjectFlagBits::SERIALIZE_HIERARCHY, true);

    ASSERT_TRUE(ser.Export(co));
    auto imported = ser.Import<IContent>();
    ASSERT_TRUE(imported);

    auto content = GetValue(imported->Content());
    ASSERT_TRUE(content);
    EXPECT_EQ(content->GetName(), "Content");
}

/**
 * @tc.name: ContentRequiredInterfaces
 * @tc.desc: Tests for Content Required Interfaces. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContentTest, ContentRequiredInterfaces, testing::ext::TestSize.Level1)
{
    ContentObject co(CreateInstance(ClassId::ContentObject));
    EXPECT_TRUE(RequiredInterfaces(co).SetRequiredInterfaces({ ITestType::UID }));

    auto correctType = GetObjectRegistry().Create(ClassId::TestType);
    auto wrongType = GetObjectRegistry().Create(ClassId::Object);

    EXPECT_FALSE(co.SetContent(wrongType));
    EXPECT_FALSE(co.GetContent());

    EXPECT_TRUE(co.SetContent(correctType));
    EXPECT_EQ(co.GetContent(), correctType);

    // Relax requirements
    EXPECT_TRUE(RequiredInterfaces(co).SetRequiredInterfaces({}));
    EXPECT_EQ(co.GetContent(), correctType);

    EXPECT_TRUE(co.SetContent(wrongType));
    EXPECT_EQ(co.GetContent(), wrongType);
}

} // namespace UTest
META_END_NAMESPACE()
