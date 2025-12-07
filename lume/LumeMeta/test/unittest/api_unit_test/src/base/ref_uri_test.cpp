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

#include <meta/api/object.h>
#include <meta/base/ref_uri.h>

#include "helpers/testing_objects.h"
#include "helpers/util.h"
#include "test_framework.h"

META_BEGIN_NAMESPACE()
namespace UTest {

/**
 * @tc.name: DefaultAndUidConstruction
 * @tc.desc: Tests for Default And Uid Construction. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RefUriTest, DefaultAndUidConstruction, testing::ext::TestSize.Level1)
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
 * @tc.name: StringConstruction
 * @tc.desc: Tests for String Construction. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RefUriTest, StringConstruction, testing::ext::TestSize.Level1)
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
    {
        RefUri u { "ref://this/!and/@/$that" };
        EXPECT_TRUE(u.IsValid());
        EXPECT_TRUE(!u.IsEmpty());
        EXPECT_EQ(u.ToString(), "ref://this/!and/@/$that");
        EXPECT_TRUE(u.StartsFromRoot());
        EXPECT_TRUE(u.ReferencesProperty());
        EXPECT_TRUE(!u.ReferencesObject());
        EXPECT_EQ(u.ReferencedName(), "that");
        {
            auto n = u.TakeFirstNode();
            EXPECT_EQ(n.name, "this");
            EXPECT_EQ(n.type, RefUri::Node::OBJECT);
        }
        {
            auto n = u.TakeFirstNode();
            EXPECT_EQ(n.name, "and");
            EXPECT_EQ(n.type, RefUri::Node::ATTACHMENT);
        }
        {
            auto n = u.TakeFirstNode();
            EXPECT_EQ(n.name, "@");
            EXPECT_EQ(n.type, RefUri::Node::SPECIAL);
        }
        {
            auto n = u.TakeFirstNode();
            EXPECT_EQ(n.name, "that");
            EXPECT_EQ(n.type, RefUri::Node::PROPERTY);
        }
    }
}

/**
 * @tc.name: Setters
 * @tc.desc: Tests for Setters. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RefUriTest, Setters, testing::ext::TestSize.Level1)
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
 * @tc.name: Escaping
 * @tc.desc: Tests for Escaping. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RefUriTest, Escaping, testing::ext::TestSize.Level1)
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
 * @tc.name: Context
 * @tc.desc: Tests for Context. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RefUriTest, Context, testing::ext::TestSize.Level1)
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
 * @tc.name: Canonicalization
 * @tc.desc: Tests for Canonicalization. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RefUriTest, Canonicalization, testing::ext::TestSize.Level1)
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
 * @tc.name: ObjectHierarchyTraversal
 * @tc.desc: Tests for Object Hierarchy Traversal. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RefUriTest, ObjectHierarchyTraversal, testing::ext::TestSize.Level1)
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
 * @tc.name: ObjectHierarchyTraversalWithProperty
 * @tc.desc: Tests for Object Hierarchy Traversal With Property. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RefUriTest, ObjectHierarchyTraversalWithProperty, testing::ext::TestSize.Level1)
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
 * @tc.name: ResolveWithProperties
 * @tc.desc: Tests for Resolve With Properties. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RefUriTest, ResolveWithProperties, testing::ext::TestSize.Level1)
{
    {
        Metadata obj(CreateInstance(META_NS::ClassId::Object));
        obj.AddProperty(CreateProperty<int>("prop", 0));
        EXPECT_FALSE(obj.GetPtr<IObjectInstance>()->Resolve(RefUri("ref:/$prop")));
        RefUri abs("ref:/$prop");
        abs.SetAbsoluteInterpretation(true);
        EXPECT_TRUE(obj.GetPtr<IObjectInstance>()->Resolve(abs));
    }
    {
        Metadata obj(CreateInstance(META_NS::ClassId::Object));
        obj.AddProperty(CreateProperty<IObject::Ptr>("prop", nullptr));
        EXPECT_FALSE(obj.GetPtr<IObjectInstance>()->Resolve(RefUri("ref:/$prop")));
    }
    {
        Metadata obj(CreateInstance(META_NS::ClassId::Object));
        Metadata obj2(CreateInstance(META_NS::ClassId::Object));
        auto prop = CreateProperty<IObject::Ptr>("prop", obj2.GetPtr<IObject>());
        obj.AddProperty(prop);
        EXPECT_EQ(obj.GetPtr<IObjectInstance>()->Resolve(RefUri("ref:/$prop")), obj2.GetPtr<IObject>());
        RefUri abs("ref:/$prop");
        abs.SetAbsoluteInterpretation(true);
        EXPECT_EQ(obj.GetPtr<IObjectInstance>()->Resolve(abs), interface_pointer_cast<IObject>(prop));
    }
    {
        Metadata obj(CreateInstance(META_NS::ClassId::Object));
        Metadata obj2(CreateInstance(META_NS::ClassId::Object));
        obj.AddProperty(CreateProperty<IObject::Ptr>("prop", obj2.GetPtr<IObject>()));
        obj2.AddProperty(CreateProperty<IObject::Ptr>("prop", nullptr));
        EXPECT_EQ(obj.GetPtr<IObjectInstance>()->Resolve(RefUri("ref:/$prop")), obj2.GetPtr<IObjectInstance>());
        EXPECT_FALSE(obj.GetPtr<IObjectInstance>()->Resolve(RefUri("ref:/$prop/$prop")));
    }
    {
        Metadata obj(CreateInstance(META_NS::ClassId::Object));
        Metadata obj2(CreateInstance(META_NS::ClassId::Object));
        Metadata obj3(CreateInstance(META_NS::ClassId::Object));
        obj.AddProperty(CreateProperty<IObject::Ptr>("prop", obj2.GetPtr<IObject>()));
        obj2.AddProperty(CreateProperty<IObject::Ptr>("prop", obj3.GetPtr<IObject>()));
        EXPECT_EQ(obj.GetPtr<IObjectInstance>()->Resolve(RefUri("ref:/$prop")), obj2.GetPtr<IObjectInstance>());
        EXPECT_EQ(obj.GetPtr<IObjectInstance>()->Resolve(RefUri("ref:/$prop/$prop")), obj3.GetPtr<IObjectInstance>());
    }
}

/**
 * @tc.name: IObjectResolveAndSelf
 * @tc.desc: Tests for Iobject Resolve And Self. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RefUriTest, IObjectResolveAndSelf, testing::ext::TestSize.Level1)
{
    Metadata obj(CreateInstance(META_NS::ClassId::Object));
    Metadata obj2(CreateInstance(META_NS::ClassId::Object));
    obj.AddProperty(CreateProperty<IObject::Ptr>("prop", obj2.GetPtr<IObject>()));
    EXPECT_EQ(obj.GetPtr<IObject>()->Resolve(RefUri("ref:/$prop")), obj2.GetPtr<IObject>());
    EXPECT_EQ(obj.GetPtr<IObject>()->GetSelf(), obj.GetPtr<IObject>());
    EXPECT_EQ(Resolve(obj.GetPtr<IObject>(), RefUri("ref:/$prop")), obj2.GetPtr<IObject>());
    EXPECT_EQ(GetSelf(obj.GetPtr<IObject>()), obj.GetPtr<IObject>());
}

/**
 * @tc.name: Attachment
 * @tc.desc: Tests for Attachment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RefUriTest, Attachment, testing::ext::TestSize.Level1)
{
    auto obj = CreateInstance(META_NS::ClassId::Object);
    auto att = CreateTestType();
    att->SetName("test");

    auto i = interface_cast<IAttach>(obj);
    ASSERT_TRUE(i);
    i->Attach(att);

    RefUri uri("ref:/!test/$First");
    uri.SetAbsoluteInterpretation(true);

    EXPECT_EQ(obj->Resolve(uri), interface_pointer_cast<IObject>(att->First().GetProperty()));
}

/**
 * @tc.name: Subscripting
 * @tc.desc: Tests for Subscripting. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RefUriTest, Subscripting, testing::ext::TestSize.Level1)
{
    auto aobj = CreateTestType();
    auto obj = CreateInstance(META_NS::ClassId::Object);
    auto att = CreateTestType();
    att->SetName("test");
    att->MyObjectArray()->SetValue({ nullptr, interface_pointer_cast<IObject>(aobj), nullptr });

    auto i = interface_cast<IAttach>(obj);
    ASSERT_TRUE(i);
    i->Attach(att);

    {
        RefUri uri("ref:/!test/$MyObjectArray[1]/$First");
        uri.SetAbsoluteInterpretation(true);
        EXPECT_EQ(obj->Resolve(uri), interface_pointer_cast<IObject>(aobj->First().GetProperty()));
    }
}

} // namespace UTest
META_END_NAMESPACE()
