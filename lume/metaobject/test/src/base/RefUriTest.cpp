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

#include <meta/api/object.h>
#include <meta/base/ref_uri.h>
#include <meta/interface/intf_metadata.h>

#include "src/test_runner.h"
#include "src/util.h"
#include "src/testing_objects.h"

using namespace testing::ext;

META_BEGIN_NAMESPACE()

class RefUriTest : public testing::Test {
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
 * @tc.name: RefUriTest
 * @tc.desc: test DefaultAndUidConstruction function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(RefUriTest, DefaultAndUidConstruction, TestSize.Level1)
{
    RefUri empty;
    EXPECT_TRUE(empty.IsEmpty());
    EXPECT_TRUE(empty.IsValid());
    EXPECT_TRUE(!empty.ReferencesProperty());
    EXPECT_TRUE(empty.ReferencesObject());
    EXPECT_EQ(empty.BaseObjectUid(), BASE_NS::Uid {});
    EXPECT_TRUE(!empty.StartsFromRoot());
    EXPECT_EQ(empty.RelativeUri(), empty);
    EXPECT_EQ(empty.ReferencedName(), "");
    EXPECT_EQ(empty.ToString(), "ref:/");

    BASE_NS::Uid uid { "f80a3984-7ac7-4400-8e17-13937cc11d39" };
    RefUri u1 { uid };
    EXPECT_TRUE(!u1.IsEmpty());
    EXPECT_TRUE(u1.IsValid());
    EXPECT_TRUE(!u1.ReferencesProperty());
    EXPECT_TRUE(u1.ReferencesObject());
    EXPECT_EQ(u1.BaseObjectUid(), uid);
    EXPECT_TRUE(!u1.StartsFromRoot());
    EXPECT_EQ(u1.RelativeUri(), empty);
    EXPECT_EQ(u1.ReferencedName(), "");
    EXPECT_EQ(u1.ToString(), "ref:f80a3984-7ac7-4400-8e17-13937cc11d39/");

    RefUri u2 { uid, "/test/path" };
    EXPECT_TRUE(!u2.IsEmpty());
    EXPECT_TRUE(u2.IsValid());
    EXPECT_TRUE(!u2.ReferencesProperty());
    EXPECT_TRUE(u2.ReferencesObject());
    EXPECT_EQ(u2.BaseObjectUid(), uid);
    EXPECT_TRUE(!u2.StartsFromRoot());
    EXPECT_EQ(u2.RelativeUri(), RefUri { "ref:/test/path" });
    EXPECT_EQ(u2.ReferencedName(), "path");
    EXPECT_EQ(u2.ToString(), "ref:f80a3984-7ac7-4400-8e17-13937cc11d39/test/path");
    EXPECT_EQ(u2.TakeFirstNode().name, "test");
    EXPECT_EQ(u2.TakeFirstNode().name, "path");
    EXPECT_EQ(u2, u1);

    RefUri u3 { uid, "//test/path" };
    EXPECT_TRUE(u3.StartsFromRoot());
    EXPECT_EQ(u3.ToString(), "ref:f80a3984-7ac7-4400-8e17-13937cc11d39//test/path");
    u3.SetStartsFromRoot(false);
    EXPECT_EQ(u3, RefUri(uid, "/test/path"));
}

/**
 * @tc.name: RefUriTest
 * @tc.desc: test StringConstruction function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(RefUriTest, StringConstruction, TestSize.Level1)
{
    EXPECT_EQ(RefUri { "ref:/" }, RefUri {});
    EXPECT_FALSE(RefUri("").IsValid());
    EXPECT_FALSE(RefUri("/some/path").IsValid());
    EXPECT_FALSE(RefUri("ref:///").IsValid());
    EXPECT_EQ(RefUri("ref:/.."), RefUri::ParentUri());
    EXPECT_FALSE(RefUri("ref:7ac7-4400-8e17-13937cc11d39").IsValid());
    EXPECT_FALSE(RefUri("ref:f80a3984-7ac7-4400-8e17-13937cc11d39///").IsValid());
    {
        RefUri u { "ref://" };
        EXPECT_TRUE(u.IsValid());
        EXPECT_TRUE(!u.IsEmpty());
        EXPECT_EQ(u.ToString(), "ref://");
        EXPECT_TRUE(u.StartsFromRoot());
    }
    {
        RefUri u { "ref://this/and/$that" };
        EXPECT_TRUE(u.IsValid());
        EXPECT_TRUE(!u.IsEmpty());
        EXPECT_EQ(u.ToString(), "ref://this/and/$that");
        EXPECT_TRUE(u.StartsFromRoot());
        EXPECT_TRUE(u.ReferencesProperty());
        EXPECT_TRUE(!u.ReferencesObject());
        EXPECT_EQ(u.ReferencedName(), "that");
        EXPECT_EQ(u.TakeFirstNode().name, "this");
        {
            auto n = u.TakeFirstNode();
            EXPECT_EQ(n.name, "and");
            EXPECT_EQ(n.type, RefUri::Node::OBJECT);
        }
        {
            auto n = u.TakeFirstNode();
            EXPECT_EQ(n.name, "that");
            EXPECT_EQ(n.type, RefUri::Node::PROPERTY);
        }
    }
    {
        RefUri u { "ref:f80a3984-7ac7-4400-8e17-13937cc11d39/hips/$hops/haps" };
        EXPECT_TRUE(u.IsValid());
        EXPECT_TRUE(!u.IsEmpty());
        EXPECT_EQ(u.BaseObjectUid(), BASE_NS::Uid { "f80a3984-7ac7-4400-8e17-13937cc11d39" });
        EXPECT_EQ(u.ToString(), "ref:f80a3984-7ac7-4400-8e17-13937cc11d39/hips/$hops/haps");
        EXPECT_TRUE(!u.StartsFromRoot());
        EXPECT_TRUE(!u.ReferencesProperty());
        EXPECT_TRUE(u.ReferencesObject());
        EXPECT_EQ(u.ReferencedName(), "haps");
        EXPECT_EQ(u.TakeFirstNode().name, "hips");
        {
            auto n = u.TakeFirstNode();
            EXPECT_EQ(n.name, "hops");
            EXPECT_EQ(n.type, RefUri::Node::PROPERTY);
        }
        {
            auto n = u.TakeFirstNode();
            EXPECT_EQ(n.name, "haps");
            EXPECT_EQ(n.type, RefUri::Node::OBJECT);
        }
    }
}

/**
 * @tc.name: RefUriTest
 * @tc.desc: test Setters function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(RefUriTest, Setters, TestSize.Level1)
{
    {
        RefUri u { "ref:f80a3984-7ac7-4400-8e17-13937cc11d39/hips/$hops/haps" };
        EXPECT_TRUE(u.IsValid());
        BASE_NS::Uid uid { "1ae79e0c-2701-4f19-8553-fb2dd22f6eba" };
        u.SetBaseObjectUid(uid);
        EXPECT_EQ(u.ToString(), "ref:1ae79e0c-2701-4f19-8553-fb2dd22f6eba/hips/$hops/haps");
        u.SetStartsFromRoot(true);
        EXPECT_EQ(u.ToString(), "ref:1ae79e0c-2701-4f19-8553-fb2dd22f6eba//hips/$hops/haps");
    }
    {
        RefUri u;
        u.PushPropertySegment("test");
        u.PushObjectSegment("other");
        u.PushPropertySegment("some");
        u.PushObjectSegment("path");
        EXPECT_EQ(u.ToString(), "ref:/path/$some/other/$test");
    }
}

/**
 * @tc.name: RefUriTest
 * @tc.desc: test Escaping function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(RefUriTest, Escaping, TestSize.Level1)
{
    {
        RefUri u(R"--(ref:/\/123\$456\@dd\\77)--");
        EXPECT_EQ(u.ToString(), R"--(ref:/\/123\$456\@dd\\77)--");
        EXPECT_EQ(u.TakeFirstNode().name, R"--(/123$456@dd\77)--");
    }
    {
        RefUri u(R"--(ref:/\\\\\\\\\\\\\/)--");
        EXPECT_EQ(u.ToString(), R"--(ref:/\\\\\\\\\\\\\/)--");
        EXPECT_EQ(u.TakeFirstNode().name, R"--(\\\\\\/)--");
    }
    {
        RefUri u;
        u.PushObjectSegment(R"--($/\gg@/)--");
        EXPECT_EQ(u.ToString(), R"--(ref:/\$\/\\gg\@\/)--");
    }
}

/**
 * @tc.name: RefUriTest
 * @tc.desc: test Context function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(RefUriTest, Context, TestSize.Level1)
{
    {
        RefUri u { "ref:/@Context" };
        EXPECT_TRUE(u.IsValid());
        EXPECT_TRUE(!u.IsEmpty());
        EXPECT_EQ(u.ToString(), "ref:/@Context");
        EXPECT_TRUE(!u.StartsFromRoot());
        EXPECT_TRUE(!u.ReferencesProperty());
        EXPECT_TRUE(u.ReferencesObject());
        EXPECT_EQ(u.ReferencedName(), "@Context");
        {
            auto n = u.TakeFirstNode();
            EXPECT_EQ(n.name, "@Context");
            EXPECT_EQ(n.type, RefUri::Node::SPECIAL);
        }
    }
    {
        EXPECT_EQ(RefUri::ContextUri(), RefUri("ref:/@Context"));
        RefUri u;
        u.PushObjectContextSegment();
        EXPECT_EQ(u, RefUri::ContextUri());
    }
    {
        RefUri u { "ref:1ae79e0c-2701-4f19-8553-fb2dd22f6eba/@Context/$Theme/$Prop" };
        EXPECT_EQ(u.ToString(), "ref:1ae79e0c-2701-4f19-8553-fb2dd22f6eba/@Context/$Theme/$Prop");
    }
}

/**
 * @tc.name: RefUriTest
 * @tc.desc: test Context function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(RefUriTest, Canonicalization, TestSize.Level1)
{
    EXPECT_EQ(RefUri("ref:/.").ToString(), "ref:/");
    EXPECT_EQ(RefUri("ref:/..").ToString(), "ref:/..");
    EXPECT_EQ(RefUri("ref://.").ToString(), "ref://");
    EXPECT_EQ(RefUri("ref:/some/.").ToString(), "ref:/some");
    EXPECT_EQ(RefUri("ref:/some/..").ToString(), "ref:/");
    EXPECT_EQ(RefUri("ref:/some/other/..").ToString(), "ref:/some");
    EXPECT_EQ(RefUri("ref:/1/2/../../3/../4/././5/../6/.").ToString(), "ref:/4/6");
    EXPECT_EQ(RefUri("ref:/@Theme").ToString(), "ref:/@Context/$Theme");
    EXPECT_EQ(RefUri("ref:/@Theme/./..").ToString(), "ref:/@Context");
    EXPECT_EQ(RefUri("ref:/../@Theme").ToString(), "ref:/../@Context/$Theme");
    EXPECT_EQ(RefUri("ref:/@Context/..").ToString(), "ref:/");
}

/**
 * @tc.name: RefUriTest
 * @tc.desc: test ObjectHierarchyTraversal function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(RefUriTest, ObjectHierarchyTraversal, TestSize.Level1)
{
    auto r1 = CreateTestType();
    r1->SetName("r1");
    auto r2 = CreateTestType();
    r2->SetName("r2");
    auto c1 = CreateTestContainer();
    c1->SetName("c1");
    auto c2 = CreateTestContainer();
    c2->SetName("c2");
    auto c3 = CreateTestContainer();
    c3->SetName("c3");

    interface_pointer_cast<IContainer>(c1)->Add(interface_pointer_cast<IObject>(r1));
    interface_pointer_cast<IContainer>(c2)->Add(interface_pointer_cast<IObject>(r2));
    interface_pointer_cast<IContainer>(c2)->Add(interface_pointer_cast<IObject>(c1));
    interface_pointer_cast<IContainer>(c3)->Add(interface_pointer_cast<IObject>(c2));

    auto o1 = interface_pointer_cast<IObjectInstance>(r1)->Resolve<IObjectInstance>(RefUri::ParentUri());
    ASSERT_TRUE(o1);
    EXPECT_EQ(o1->GetInstanceId(), interface_pointer_cast<IObjectInstance>(c1)->GetInstanceId());

    auto o2 = interface_pointer_cast<IObjectInstance>(c3)->Resolve<IObjectInstance>(RefUri("ref:/"));
    ASSERT_TRUE(o2);
    EXPECT_EQ(o2->GetInstanceId(), interface_pointer_cast<IObjectInstance>(c3)->GetInstanceId());

    auto o3 = interface_pointer_cast<IObjectInstance>(c3)->Resolve<IObjectInstance>(RefUri("ref:/c2/c1/r1"));
    ASSERT_TRUE(o3);
    EXPECT_EQ(o3->GetInstanceId(), interface_pointer_cast<IObjectInstance>(r1)->GetInstanceId());

    auto o4 = interface_pointer_cast<IObjectInstance>(r1)->Resolve<IObjectInstance>(RefUri("ref://"));
    ASSERT_TRUE(o4);
    EXPECT_EQ(o4->GetInstanceId(), interface_pointer_cast<IObjectInstance>(c3)->GetInstanceId());

    auto o5 = interface_pointer_cast<IObjectInstance>(r1)->Resolve<IObjectInstance>(RefUri("ref://c2/r2"));
    ASSERT_TRUE(o5);
    EXPECT_EQ(o5->GetInstanceId(), interface_pointer_cast<IObjectInstance>(r2)->GetInstanceId());
}

/**
 * @tc.name: RefUriTest
 * @tc.desc: test ObjectHierarchyTraversalWithProperty function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(RefUriTest, ObjectHierarchyTraversalWithProperty, TestSize.Level1)
{
    auto p = CreateProperty<IObject::Ptr>("prop", 0);

    auto r = CreateTestType();
    r->SetName("r");
    auto c1 = CreateTestContainer();
    c1->SetName("c1");
    auto c2 = CreateTestContainer();
    c2->SetName("c2");
    auto c3 = CreateTestContainer();
    c3->SetName("c3");

    interface_pointer_cast<IContainer>(c1)->Add(interface_pointer_cast<IObject>(r));
    p->SetValue(interface_pointer_cast<IObject>(c1));
    interface_pointer_cast<IMetadata>(c2)->AddProperty(p);
    interface_pointer_cast<IContainer>(c3)->Add(interface_pointer_cast<IObject>(c2));

    auto o = interface_cast<IObjectInstance>(c3)->Resolve<IObjectInstance>(RefUri("ref:/c2/$prop/r"));
    ASSERT_TRUE(o);
    EXPECT_EQ(o->GetInstanceId(), interface_cast<IObjectInstance>(r)->GetInstanceId());
}

/**
 * @tc.name: RefUriTest
 * @tc.desc: test ResolveWithProperties function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(RefUriTest, ResolveWithProperties, TestSize.Level1)
{
    {
        Object obj;
        obj.AddProperty(CreateProperty<int>("prop", 0));
        EXPECT_FALSE(obj.GetInterface<IObjectInstance>()->Resolve(RefUri("ref:/$prop")));
        RefUri abs("ref:/$prop");
        abs.SetAbsoluteInterpretation(true);
        EXPECT_TRUE(obj.GetInterface<IObjectInstance>()->Resolve(abs));
    }
    {
        Object obj;
        obj.AddProperty(CreateProperty<IObject::Ptr>("prop", nullptr));
        EXPECT_FALSE(obj.GetInterface<IObjectInstance>()->Resolve(RefUri("ref:/$prop")));
    }
    {
        Object obj;
        Object obj2;
        auto prop = CreateProperty<IObject::Ptr>("prop", obj2.GetIObject());
        obj.AddProperty(prop);
        EXPECT_EQ(obj.GetInterface<IObjectInstance>()->Resolve(RefUri("ref:/$prop")), obj2.GetInterfacePtr<IObject>());
        RefUri abs("ref:/$prop");
        abs.SetAbsoluteInterpretation(true);
        EXPECT_EQ(obj.GetInterface<IObjectInstance>()->Resolve(abs), interface_pointer_cast<IObject>(prop));
    }
    {
        Object obj;
        Object obj2;
        obj.AddProperty(CreateProperty<IObject::Ptr>("prop", obj2.GetIObject()));
        obj2.AddProperty(CreateProperty<IObject::Ptr>("prop", nullptr));
        EXPECT_EQ(obj.GetInterface<IObjectInstance>()->Resolve(RefUri("ref:/$prop")),
            obj2.GetInterfacePtr<IObjectInstance>());
        EXPECT_FALSE(obj.GetInterface<IObjectInstance>()->Resolve(RefUri("ref:/$prop/$prop")));
    }
    {
        Object obj;
        Object obj2;
        Object obj3;
        obj.AddProperty(CreateProperty<IObject::Ptr>("prop", obj2.GetIObject()));
        obj2.AddProperty(CreateProperty<IObject::Ptr>("prop", obj3.GetIObject()));
        EXPECT_EQ(obj.GetInterface<IObjectInstance>()->Resolve(RefUri("ref:/$prop")),
            obj2.GetInterfacePtr<IObjectInstance>());
        EXPECT_EQ(obj.GetInterface<IObjectInstance>()->Resolve(RefUri("ref:/$prop/$prop")),
            obj3.GetInterfacePtr<IObjectInstance>());
    }
}

/**
 * @tc.name: RefUriTest
 * @tc.desc: test IObjectResolveAndSelf function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(RefUriTest, IObjectResolveAndSelf, TestSize.Level1)
{
    Object obj;
    Object obj2;
    obj.AddProperty(CreateProperty<IObject::Ptr>("prop", obj2.GetIObject()));
    EXPECT_EQ(obj.GetIObject()->Resolve(RefUri("ref:/$prop")), obj2.GetIObject());
    EXPECT_EQ(obj.GetIObject()->GetSelf(), obj.GetIObject());
    EXPECT_EQ(Resolve(obj.GetIObject(), RefUri("ref:/$prop")), obj2.GetIObject());
    EXPECT_EQ(GetSelf(obj.GetIObject()), obj.GetIObject());
}
META_END_NAMESPACE()