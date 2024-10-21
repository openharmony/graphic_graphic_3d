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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <meta/ext/object.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_derived.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_registry.h>

#include "src/test_runner.h"

using namespace testing::ext;

META_BEGIN_NAMESPACE()

META_REGISTER_CLASS(BaseObjectDerived, "ecc59f20-2bac-4521-8fbe-18d02cfa2ddf", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(MetaObjectDerived, "9e29e701-a3c6-439f-be85-e2d3106fcb4e", META_NS::ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(ObjectDerived, "aabee314-238c-4263-b295-8139bde9933d", META_NS::ObjectCategoryBits::NO_CATEGORY)

class BaseObjectDerived final
    : public META_NS::BaseObjectFwd<BaseObjectDerived, ClassId::BaseObjectDerived, META_NS::ClassId::BaseObject> {};

class MetaObjectDerived final
    : public META_NS::MetaObjectFwd<MetaObjectDerived, ClassId::MetaObjectDerived, META_NS::ClassId::MetaObject> {};

class ObjectDerived final : public META_NS::ObjectFwd<ObjectDerived, ClassId::ObjectDerived, META_NS::ClassId::Object> {
};

class BaseObjectTest : public testing::Test {
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
        ResetTest();
        UnregisterObjectType<ObjectDerived>();
        UnregisterObjectType<MetaObjectDerived>();
        UnregisterObjectType<BaseObjectDerived>();
    }
    void SetUp() override {}
    void TearDown() override {}
protected:
    const BASE_NS::vector<BASE_NS::Uid> BASE_OBJECT_INTERFACES { IObject::UID, IObjectInstance::UID, IObjectFlags::UID,
        IDerived::UID, ILifecycle::UID };

    const BASE_NS::vector<BASE_NS::Uid> META_OBJECT_INTERFACES { IObjectContextProvider::UID, IMetadata::UID,
        IMetadataInternal::UID };
    
    const BASE_NS::vector<BASE_NS::Uid> OBJECT_INTERFACES { IAttach::UID, IContainerQuery::UID };

    void CheckInterface(const IObject::ConstPtr& object, BASE_NS::Uid uid, bool implements)
    {
        ASSERT_NE(object, nullptr);
        if (implements) {
            EXPECT_NE(object->GetInterface(uid), nullptr) << "uid: " << BASE_NS::to_string(uid).c_str();
            EXPECT_THAT(object->GetInterfaces(), testing::Contains(uid)) << "uid: " << BASE_NS::to_string(uid).c_str();
        } else {
            EXPECT_EQ(object->GetInterface(uid), nullptr) << "uid: " << BASE_NS::to_string(uid).c_str();
            EXPECT_THAT(object->GetInterfaces(), testing::Not(testing::Contains(uid))) <<
                "uid: " << BASE_NS::to_string(uid).c_str();
        }
    }

    void CheckInterfaces(const IObject::ConstPtr& object, bool implements, const BASE_NS::vector<BASE_NS::Uid>& uids)
    {
        for (const auto& uid : uids) {
            CheckInterface(object, uid, implements);
        }
    }
};

/**
 * @tc.name: BaseObjectTest
 * @tc.desc: test BaseObject function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BaseObjectTest, BaseObject, TestSize.Level1)
{
    auto object = GetObjectRegistry().Create(META_NS::ClassId::BaseObject);
    ASSERT_NE(object, nullptr);
    CheckInterfaces(object, true, BASE_OBJECT_INTERFACES);
    CheckInterfaces(object, false, META_OBJECT_INTERFACES);
    CheckInterfaces(object, false, OBJECT_INTERFACES);
    EXPECT_THAT(object->GetInterfaces(), testing::SizeIs(BASE_OBJECT_INTERFACES.size()));
}

/**
 * @tc.name: BaseObjectTest
 * @tc.desc: test MetaObject function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BaseObjectTest, MetaObject, TestSize.Level1)
{
    auto object = GetObjectRegistry().Create(META_NS::ClassId::MetaObject);
    ASSERT_NE(object, nullptr);
    CheckInterfaces(object, true, BASE_OBJECT_INTERFACES);
    CheckInterfaces(object, true, META_OBJECT_INTERFACES);
    CheckInterfaces(object, false, OBJECT_INTERFACES);
    EXPECT_THAT(
        object->GetInterfaces(), testing::SizeIs(BASE_OBJECT_INTERFACES.size() + META_OBJECT_INTERFACES.size()));
}

/**
 * @tc.name: BaseObjectTest
 * @tc.desc: test Object function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BaseObjectTest, Object, TestSize.Level1)
{
    auto object = GetObjectRegistry().Create(META_NS::ClassId::Object);
    ASSERT_NE(object, nullptr);
    CheckInterfaces(object, true, BASE_OBJECT_INTERFACES);
    CheckInterfaces(object, true, META_OBJECT_INTERFACES);
    CheckInterfaces(object, true, OBJECT_INTERFACES);
    EXPECT_THAT(object->GetInterfaces(),
        testing::SizeIs(BASE_OBJECT_INTERFACES.size() + META_OBJECT_INTERFACES.size() + OBJECT_INTERFACES.size()));
}

/**
 * @tc.name: BaseObjectTest
 * @tc.desc: test DeriveBaseObject function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BaseObjectTest, DeriveBaseObject, TestSize.Level1)
{
    auto object = GetObjectRegistry().Create(ClassId::BaseObjectDerived);
    ASSERT_NE(object, nullptr);
    CheckInterfaces(object, true, BASE_OBJECT_INTERFACES);
    CheckInterfaces(object, false, META_OBJECT_INTERFACES);
    CheckInterfaces(object, false, OBJECT_INTERFACES);
    EXPECT_THAT(object->GetInterfaces(), testing::SizeIs(BASE_OBJECT_INTERFACES.size()));
    auto derived = interface_cast<IDerived>(object);
    ASSERT_NE(derived, nullptr);
    EXPECT_EQ(ObjectId(derived->GetSuperClassUid()), META_NS::ClassId::BaseObject);
}

/**
 * @tc.name: BaseObjectTest
 * @tc.desc: test DeriveBaseObject function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BaseObjectTest, DeriveMetaObject, TestSize.Level1)
{
    auto object = GetObjectRegistry().Create(ClassId::MetaObjectDerived);
    ASSERT_NE(object, nullptr);
    CheckInterfaces(object, true, BASE_OBJECT_INTERFACES);
    CheckInterfaces(object, true, META_OBJECT_INTERFACES);
    CheckInterfaces(object, false, OBJECT_INTERFACES);
    EXPECT_THAT(
        object->GetInterfaces(), testing::SizeIs(BASE_OBJECT_INTERFACES.size() + META_OBJECT_INTERFACES.size()));
    auto derived = interface_cast<IDerived>(object);
    ASSERT_NE(derived, nullptr);
    EXPECT_EQ(ObjectId(derived->GetSuperClassUid()), META_NS::ClassId::MetaObject);
}

/**
 * @tc.name: BaseObjectTest
 * @tc.desc: test DeriveObject function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(BaseObjectTest, DeriveObject, TestSize.Level1)
{
    auto object = GetObjectRegistry().Create(ClassId::ObjectDerived);
    ASSERT_NE(object, nullptr);
    CheckInterfaces(object, true, BASE_OBJECT_INTERFACES);
    CheckInterfaces(object, true, META_OBJECT_INTERFACES);
    CheckInterfaces(object, true, OBJECT_INTERFACES);
    EXPECT_THAT(object->GetInterfaces(),
        testing::SizeIs(BASE_OBJECT_INTERFACES.size() + META_OBJECT_INTERFACES.size() + OBJECT_INTERFACES.size()));
    auto derived = interface_cast<IDerived>(object);
    ASSERT_NE(derived, nullptr);
    EXPECT_EQ(ObjectId(derived->GetSuperClassUid()), META_NS::ClassId::Object);
}
META_END_NAMESPACE()