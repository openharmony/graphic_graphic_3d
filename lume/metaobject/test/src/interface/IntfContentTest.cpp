/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <meta/api/content_object.h>
#include <meta/api/iteration.h>
#include <meta/api/loaders/json_content_loader.h>
#include <meta/interface/loaders/intf_class_content_loader.h>

#include "src/serialisation_utils.h"
#include "src/test_runner.h"
#include "src/testing_objects.h"
#include "src/util.h"

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

class IntfContentTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        ResetTest();
    }
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: IntfContentTest
 * @tc.desc: test Search function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfContentTest, Search, TestSize.Level1)
{
    auto content = CreateTestType<IObject>("Content");

    ContentObject co;
    co.SetContent(content);
    auto c = CreateTestContainer<IContainer>();
    c->Add(co);
    EXPECT_FALSE(c->FindAny({ "Content", TraversalType::NO_HIERARCHY }));
    EXPECT_EQ(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }), content);
    EXPECT_EQ(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestType::UID } }), content);
    EXPECT_EQ(c->FindAny({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestType::UID } }), content);
    EXPECT_FALSE(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER, { IContainer::UID } }));

    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 2);
    EXPECT_EQ(c->FindAll({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 1);

    EXPECT_TRUE(ContainsObjectWithName(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }), "Content"));

    co.ContentSearchable(false);
    EXPECT_FALSE(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }));
    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 1);

    co.ContentSearchable(true);
    EXPECT_EQ(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }), content);
    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 2);
}

/**
 * @tc.name: IntfContentTest
 * @tc.desc: test SearchContentContainer function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfContentTest, SearchContentContainer, TestSize.Level1)
{
    auto ccontent = CreateTestType<IObject>("Content");
    auto content = CreateTestContainer<IContainer>();
    content->Add(ccontent);

    ContentObject co;
    co.SetContent(interface_pointer_cast<IObject>(content));
    auto c = CreateTestContainer<IContainer>();
    c->Add(co);
    EXPECT_FALSE(c->FindAny({ "Content", TraversalType::NO_HIERARCHY }));
    EXPECT_EQ(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }), ccontent);
    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 3);

    co.ContentSearchable(false);
    EXPECT_FALSE(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }));
    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 1);

    co.ContentSearchable(true);
    EXPECT_EQ(c->FindAny({ "Content", TraversalType::DEPTH_FIRST_PRE_ORDER }), ccontent);
    EXPECT_EQ(c->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER }).size(), 3);
}

/**
 * @tc.name: IntfContentTest
 * @tc.desc: test Iteration function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfContentTest, Iteration, TestSize.Level1)
{
    auto ccontent = CreateTestType<IObject>("Content");
    auto content = CreateTestContainer<IContainer>();
    content->Add(ccontent);

    ContentObject co;
    co.SetContent(interface_pointer_cast<IObject>(content));
    auto c = CreateTestContainer<IContainer>();
    c->Add(co);

    {
        BASE_NS::vector<IObject::Ptr> vec;
        EXPECT_TRUE(ForEachShared(
            c, [&](const IObject::Ptr& obj) { vec.push_back(obj); }, TraversalType::DEPTH_FIRST_PRE_ORDER));
        EXPECT_EQ(vec.size(), 3);
        EXPECT_TRUE(ContainsObjectWithName(vec, "Content"));
    }

    co.ContentSearchable(false);

    {
        BASE_NS::vector<IObject::Ptr> vec;
        EXPECT_TRUE(ForEachShared(
            c, [&](const IObject::Ptr& obj) { vec.push_back(obj); }, TraversalType::DEPTH_FIRST_PRE_ORDER));
        EXPECT_EQ(vec.size(), 1);
        EXPECT_FALSE(ContainsObjectWithName(vec, "Content"));
    }
}

/**
 * @tc.name: IntfContentTest
 * @tc.desc: test Serialization function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfContentTest, Serialization, TestSize.Level1)
{
    {
        TestSerialiser ser;

        ContentObject co;
        co.SerializeContent(false);
        co.SetContent(CreateTestType<IObject>("Content"));
        ASSERT_TRUE(ser.Export(co));

        auto imp = ser.Import<IContent>();
        ASSERT_TRUE(imp);
        EXPECT_FALSE(imp->Content()->GetValue());
    }

    {
        TestSerialiser ser;

        ContentObject co;
        co.SerializeContent(true);
        co.SetContent(CreateTestType<IObject>("Content"));
        ASSERT_TRUE(ser.Export(co));

        auto imp = ser.Import<IContent>();
        ASSERT_TRUE(imp);
        ASSERT_TRUE(imp->Content()->GetValue());
        EXPECT_EQ(imp->Content()->GetValue()->GetName(), "Content");
    }
}

/**
 * @tc.name: IntfContentTest
 * @tc.desc: test Defaults function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfContentTest, Defaults, TestSize.Level1)
{
    ContentObject co;
    const auto objectFlags = interface_cast<IObjectFlags>(co);
    ASSERT_TRUE(objectFlags);
    EXPECT_FALSE(objectFlags->GetObjectFlags().IsSet(META_NS::ObjectFlagBits::SERIALIZE_HIERARCHY));
}

/**
 * @tc.name: IntfContentTest
 * @tc.desc: test ObjectFlagsNoHierarchy function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfContentTest, ObjectFlagsNoHierarchy, TestSize.Level1)
{
    TestSerialiser ser;
    ContentObject co;
    co.SetContent(CreateTestType<IObject>("Content"));

    ASSERT_TRUE(ser.Export(co));

    auto imported = ser.Import<IContent>();
    ASSERT_TRUE(imported);

    EXPECT_FALSE(imported->Content()->GetValue());
}

/**
 * @tc.name: IntfContentTest
 * @tc.desc: test ObjectFlagsHierarchy function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfContentTest, ObjectFlagsHierarchy, TestSize.Level1)
{
    TestSerialiser ser;
    ContentObject co;
    co.SetContent(CreateTestType<IObject>("Content"));

    auto flagsIntPtr = interface_cast<IObjectFlags>(co);
    ASSERT_TRUE(flagsIntPtr);
    SetObjectFlags(co, ObjectFlagBits::SERIALIZE_HIERARCHY, true);

    ASSERT_TRUE(ser.Export(co));
    auto imported = ser.Import<IContent>();
    ASSERT_TRUE(imported);

    auto content = GetValue(imported->Content());
    ASSERT_TRUE(content);
    EXPECT_EQ(content->GetName(), "Content");
}

/**
 * @tc.name: IntfContentTest
 * @tc.desc: test Loader function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfContentTest, Loader, TestSize.Level1)
{
    auto loader = GetObjectRegistry().Create<IClassContentLoader>(ClassId::ClassContentLoader);
    ASSERT_TRUE(loader);
    loader->ClassId()->SetValue(ClassId::TestType);
    ASSERT_TRUE(loader->Create({}));

    ContentObject co;
    co.ContentLoader(loader);

    auto obj = co.Content()->GetValue();
    ASSERT_TRUE(obj);
    EXPECT_TRUE(interface_cast<ITestType>(obj));

    loader->ClassId()->SetValue(ClassId::TestContainer);

    auto cont = co.Content()->GetValue();
    ASSERT_TRUE(cont);
    EXPECT_TRUE(interface_cast<ITestContainer>(cont));
}

/**
 * @tc.name: IntfContentTest
 * @tc.desc: test ContentRequiredInterfaces function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfContentTest, ContentRequiredInterfaces, TestSize.Level1)
{
    ContentObject co;
    EXPECT_TRUE(co.SetRequiredInterfaces({ ITestType::UID }));

    auto correctType = GetObjectRegistry().Create(ClassId::TestType);
    auto wrongType = GetObjectRegistry().Create(ClassId::Object);

    EXPECT_FALSE(co.SetContent(wrongType));
    EXPECT_FALSE(co.Content()->GetValue());

    EXPECT_TRUE(co.SetContent(correctType));
    EXPECT_EQ(co.Content()->GetValue(), correctType);

    EXPECT_TRUE(co.SetRequiredInterfaces({}));
    EXPECT_EQ(co.Content()->GetValue(), correctType);

    EXPECT_TRUE(co.SetContent(wrongType));
    EXPECT_EQ(co.Content()->GetValue(), wrongType);
}

/**
 * @tc.name: IntfContentTest
 * @tc.desc: test ContentLoaderRequiredInterfaces function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfContentTest, ContentLoaderRequiredInterfaces, TestSize.Level1)
{
    ContentObject co;
    EXPECT_TRUE(co.SetRequiredInterfaces({ ITestType::UID }));

    auto correctType = ClassId::TestType;
    auto wrongType = ClassId::Object;

    auto loader = GetObjectRegistry().Create<IClassContentLoader>(ClassId::ClassContentLoader);
    ASSERT_TRUE(loader);
    co.ContentLoader()->SetValue(loader);
    EXPECT_FALSE(co.Content()->GetValue());

    loader->ClassId()->SetValue(correctType);
    EXPECT_TRUE(co.Content()->GetValue());

    loader->ClassId()->SetValue(wrongType);
    EXPECT_FALSE(co.Content()->GetValue());

    EXPECT_TRUE(co.SetRequiredInterfaces({}));
    EXPECT_TRUE(co.Content()->GetValue());

    EXPECT_TRUE(co.SetRequiredInterfaces({ ITestType::UID }));
    EXPECT_FALSE(co.Content()->GetValue());
}

META_END_NAMESPACE()