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

#include <meta/ext/metadata_helpers.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_derived.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_registry.h>

#include "test_framework.h"

META_BEGIN_NAMESPACE()

namespace UTest {

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

class API_BaseObjectTest : public ::testing::Test {
public:
    static void SetUpTestSuite()
    {
        RegisterObjectType<BaseObjectDerived>();
        RegisterObjectType<MetaObjectDerived>();
        RegisterObjectType<ObjectDerived>();
    }
    static void TearDownTestSuite()
    {
        UnregisterObjectType<ObjectDerived>();
        UnregisterObjectType<MetaObjectDerived>();
        UnregisterObjectType<BaseObjectDerived>();
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
            EXPECT_THAT(object->GetInterfaces(), testing::Contains(uid)) << "uid: " << BASE_NS::to_string(uid).c_str();
        } else {
            EXPECT_EQ(object->GetInterface(uid), nullptr) << "uid: " << BASE_NS::to_string(uid).c_str();
            EXPECT_THAT(object->GetInterfaces(), testing::Not(testing::Contains(uid)))
                << "uid: " << BASE_NS::to_string(uid).c_str();
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
 * @tc.name: BaseObject
 * @tc.desc: Tests for Base Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BaseObjectTest, BaseObject, testing::ext::TestSize.Level1)
{
    auto object = GetObjectRegistry().Create(META_NS::ClassId::BaseObject);
    ASSERT_NE(object, nullptr);
    CheckInterfaces(object, true, BASE_OBJECT_INTERFACES);
    CheckInterfaces(object, false, OBJECT_INTERFACES);
    EXPECT_THAT(object->GetInterfaces(), testing::SizeIs(BASE_OBJECT_INTERFACES.size()));
}

/**
 * @tc.name: Object
 * @tc.desc: Tests for Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BaseObjectTest, Object, testing::ext::TestSize.Level1)
{
    auto object = GetObjectRegistry().Create(META_NS::ClassId::Object);
    ASSERT_NE(object, nullptr);
    CheckInterfaces(object, true, BASE_OBJECT_INTERFACES);
    CheckInterfaces(object, true, OBJECT_INTERFACES);
    EXPECT_THAT(object->GetInterfaces(), testing::SizeIs(BASE_OBJECT_INTERFACES.size() + OBJECT_INTERFACES.size()));
}

/**
 * @tc.name: DeriveBaseObject
 * @tc.desc: Tests for Derive Base Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BaseObjectTest, DeriveBaseObject, testing::ext::TestSize.Level1)
{
    auto object = GetObjectRegistry().Create(ClassId::BaseObjectDerived);
    ASSERT_NE(object, nullptr);
    CheckInterfaces(object, true, BASE_OBJECT_INTERFACES);
    CheckInterfaces(object, false, OBJECT_INTERFACES);
    EXPECT_THAT(object->GetInterfaces(), testing::SizeIs(BASE_OBJECT_INTERFACES.size()));
    EXPECT_EQ(GetBaseClass(object), META_NS::ClassId::BaseObject);
}

/**
 * @tc.name: DeriveObject
 * @tc.desc: Tests for Derive Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BaseObjectTest, DeriveObject, testing::ext::TestSize.Level1)
{
    auto object = GetObjectRegistry().Create(ClassId::ObjectDerived);
    ASSERT_NE(object, nullptr);
    CheckInterfaces(object, true, BASE_OBJECT_INTERFACES);
    CheckInterfaces(object, true, OBJECT_INTERFACES);
    EXPECT_THAT(object->GetInterfaces(), testing::SizeIs(BASE_OBJECT_INTERFACES.size() + OBJECT_INTERFACES.size()));
    EXPECT_EQ(GetBaseClass(object), META_NS::ClassId::Object);
}

} // namespace UTest

META_END_NAMESPACE()
