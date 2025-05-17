/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Unit tests for TimeSpan
 * Author: Lauri Jaaskela
 * Create: 2022-10-07
 */

#include "TestRunner.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <meta/ext/metadata_helpers.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_derived.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_registry.h>

META_BEGIN_NAMESPACE()

using namespace ::testing;
using namespace testing::ext;

META_REGISTER_CLASS(BaseObjectDerived, "ecc59f20-2bac-4521-8fbe-18d02cfa2ddf", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(MetaObjectDerived, "9e29e701-a3c6-439f-be85-e2d3106fcb4e", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(ObjectDerived, "aabee314-238c-4263-b295-8139bde9933d", META_NS::ObjectCategoryBits::NO_CATEGORY)

class BaseObjectDerived final : public META_NS::BaseObjectFwd {
    META_OBJECT(BaseObjectDerived, ClassId::BaseObjectDerived, BaseObjectFwd)
};

class MetaObjectDerived final : public META_NS::ObjectFwd {
    META_OBJECT(MetaObjectDerived, ClassId::MetaObjectDerived, ObjectFwd)
};

class ObjectDerived final : public META_NS::ObjectFwd {
    META_OBJECT(ObjectDerived, ClassId::ObjectDerived, ObjectFwd)
};

class BaseObjectTest : public ::testing::Test {
public:
    static void SetUpTestSuite()
    {
        SetTest();
        RegisterObjectType<BaseObjectDerived>();
        RegisterObjectType<MetaObjectDerived>();
        RegisterObjectType<ObjectDerived>();
    }
    static void TearDownTestSuite()
    {
        UnregisterObjectType<ObjectDerived>();
        UnregisterObjectType<MetaObjectDerived>();
        UnregisterObjectType<BaseObjectDerived>();
        TearDownTest();
    }

    void SetUp() override {}
    void TearDown() override {}

protected:
    // ClassId::BaseObject interfaces
    const BASE_NS::vector<BASE_NS::Uid> BASE_OBJECT_INTERFACES { IObject::UID, IObjectInstance::UID, IObjectFlags::UID,
        IDerived::UID, ILifecycle::UID, IStaticMetadata::UID };
    const BASE_NS::vector<BASE_NS::Uid> OBJECT_INTERFACES { IMetadata::UID, IOwner::UID, IAttach::UID };

    void CheckInterface(const IObject::ConstPtr& object, BASE_NS::Uid uid, bool implements)
    {
        ASSERT_NE(object, nullptr);
        if (implements) {
            EXPECT_NE(object->GetInterface(uid), nullptr) << "uid: " << BASE_NS::to_string(uid).c_str();
            EXPECT_THAT(object->GetInterfaces(), Contains(uid)) << "uid: " << BASE_NS::to_string(uid).c_str();
        } else {
            EXPECT_EQ(object->GetInterface(uid), nullptr) << "uid: " << BASE_NS::to_string(uid).c_str();
            EXPECT_THAT(object->GetInterfaces(), Not(Contains(uid))) << "uid: " << BASE_NS::to_string(uid).c_str();
        }
    }

    void CheckInterfaces(const IObject::ConstPtr& object, bool implements, const BASE_NS::vector<BASE_NS::Uid>& uids)
    {
        for (const auto& uid : uids) {
            CheckInterface(object, uid, implements);
        }
    }
};

HWTEST_F(BaseObjectTest, BaseObject, TestSize.Level1)
{
    auto object = GetObjectRegistry().Create(META_NS::ClassId::BaseObject);
    ASSERT_NE(object, nullptr);
    CheckInterfaces(object, true, BASE_OBJECT_INTERFACES);
    CheckInterfaces(object, false, OBJECT_INTERFACES);
    EXPECT_THAT(object->GetInterfaces(), SizeIs(BASE_OBJECT_INTERFACES.size()));
}

HWTEST_F(BaseObjectTest, Object, TestSize.Level1)
{
    auto object = GetObjectRegistry().Create(META_NS::ClassId::Object);
    ASSERT_NE(object, nullptr);
    CheckInterfaces(object, true, BASE_OBJECT_INTERFACES);
    CheckInterfaces(object, true, OBJECT_INTERFACES);
    EXPECT_THAT(object->GetInterfaces(), SizeIs(BASE_OBJECT_INTERFACES.size() + OBJECT_INTERFACES.size()));
}

HWTEST_F(BaseObjectTest, DeriveBaseObject, TestSize.Level1)
{
    auto object = GetObjectRegistry().Create(ClassId::BaseObjectDerived);
    ASSERT_NE(object, nullptr);
    CheckInterfaces(object, true, BASE_OBJECT_INTERFACES);
    CheckInterfaces(object, false, OBJECT_INTERFACES);
    EXPECT_THAT(object->GetInterfaces(), SizeIs(BASE_OBJECT_INTERFACES.size()));
    EXPECT_EQ(GetBaseClass(object), META_NS::ClassId::BaseObject);
}

HWTEST_F(BaseObjectTest, DeriveObject, TestSize.Level1)
{
    auto object = GetObjectRegistry().Create(ClassId::ObjectDerived);
    ASSERT_NE(object, nullptr);
    CheckInterfaces(object, true, BASE_OBJECT_INTERFACES);
    CheckInterfaces(object, true, OBJECT_INTERFACES);
    EXPECT_THAT(object->GetInterfaces(), SizeIs(BASE_OBJECT_INTERFACES.size() + OBJECT_INTERFACES.size()));
    EXPECT_EQ(GetBaseClass(object), META_NS::ClassId::Object);
}

META_END_NAMESPACE()
