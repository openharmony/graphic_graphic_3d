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

#include <meta/api/animation.h>
#include <meta/api/object_name.h>

#include "helpers/serialisation_utils.h"
#include "helpers/testing_objects.h"
#include "test_framework.h"

META_BEGIN_NAMESPACE()

namespace UTest {

/**
 * @tc.name: ObjectBaseAPI
 * @tc.desc: Tests for Object Base Api. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectTest, ObjectBaseAPI, testing::ext::TestSize.Level1)
{
    META_NS::Object object(CreateObjectInstance(ClassId::Object));

    EXPECT_NE(object.GetPtr(), nullptr);
    EXPECT_NE(object.GetPtr<IObject>(), nullptr);
    EXPECT_EQ(object.GetPtr<IObject>(), object.GetPtr());
    EXPECT_EQ(interface_pointer_cast<IObject>(object), object);
    EXPECT_EQ(interface_cast<IObject>(object), object.GetPtr().get());

    META_NS::Object object2(object);
    EXPECT_EQ(object, object2);
    EXPECT_TRUE(object == object2);
    EXPECT_FALSE(object != object2);

    META_NS::Object object3(nullptr);
    EXPECT_TRUE(object != object3);
    EXPECT_FALSE(object == object3);
    EXPECT_FALSE(object3); // not initialized yet
    EXPECT_EQ(object3.GetPtr<IObject>(), nullptr);
}

/**
 * @tc.name: TypeConversion
 * @tc.desc: Tests for Type Conversion. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectTest, TypeConversion, testing::ext::TestSize.Level1)
{
    META_NS::Object o1(CreateTestType());
    META_NS::Object o2(CreateTestContainer());

    EXPECT_TRUE(o1);
    EXPECT_TRUE(o2);
    EXPECT_NE(o1.GetClassId(), o2.GetClassId());

    o1 = o2; // Changes type
    EXPECT_TRUE(o1);
    EXPECT_EQ(o1.GetPtr(), o2.GetPtr());
    EXPECT_EQ(o1.GetClassId(), o2.GetClassId());
}

/**
 * @tc.name: TypeCompatibility
 * @tc.desc: Tests for Type Compatibility. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(ObjectTest, TypeCompatibility, testing::ext::TestSize.Level1)
{
    auto parallel = ParallelAnimation(CreateNew);
    auto sequential = SequentialAnimation(CreateNew);
    ASSERT_TRUE(parallel);
    ASSERT_TRUE(sequential);
    Animation animation = parallel;
    ASSERT_TRUE(animation);

    auto parallelAsSequential = SequentialAnimation(interface_pointer_cast<CORE_NS::IInterface>(parallel));
    EXPECT_FALSE(parallelAsSequential);

    auto parallelAsAnimationAsSequential = SequentialAnimation(animation);
    EXPECT_FALSE(parallelAsAnimationAsSequential);

    auto parallelAsAnimationAsParallel = ParallelAnimation(animation);
    EXPECT_TRUE(parallelAsAnimationAsParallel);
}

/**
 * @tc.name: InvalidTypes
 * @tc.desc: Tests for Invalid Types. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
UNIT_TEST(API_ObjectTest, InvalidTypes, testing::ext::TestSize.Level1)
{
    META_NS::Object o1(interface_pointer_cast<IObject>(CreateTestType()));
    auto a1 = META_NS::PropertyAnimation().Duration(TimeSpan::Milliseconds(100));

    EXPECT_TRUE(o1);
    EXPECT_TRUE(a1);
    EXPECT_NE(o1.GetIObject()->GetClassId(), a1.GetIObject()->GetClassId());

    auto a1Ptr = a1.GetInterfacePtr<META_NS::IAnimation>();
    ASSERT_NE(a1Ptr, nullptr);

    META_NS::Object& a1Obj = a1;
    auto classIdBefore = a1Obj.GetIObject()->GetClassId();
    EXPECT_FALSE(a1Obj.Initialize(o1)); // a1Obj Object& is PropertyAnimation, so we should not be able to initialize it
                                        // with an Object of different type
    auto classIdAfter = a1Obj.GetIObject()->GetClassId();
    EXPECT_EQ(classIdBefore, classIdAfter);
    EXPECT_NE(a1.GetIObject(), nullptr);

    static constexpr auto dur = TimeSpan::Milliseconds(42);
    auto anim = PropertyAnimation().Duration(dur); // Init an animation
    Object o(anim);                                // Store a reference in Object
    PropertyAnimation anim2;
    anim2 = o; // Initialize another (valid) animation with the same object
    EXPECT_EQ(anim2.TotalDuration().Get(), dur);
    EXPECT_EQ(anim, anim2);

    SequentialAnimation anim3(o); // Init with incompatible
    EXPECT_FALSE(anim3);
    EXPECT_NE(anim, anim3);

    SequentialAnimation anim4;
    anim4 = o; // Assign with incompatible
    EXPECT_FALSE(anim4);
    EXPECT_NE(anim, anim4);
}
*/

/**
 * @tc.name: LazyInitBase
 * @tc.desc: Tests for Lazy Init Base. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectTest, LazyInitBase, testing::ext::TestSize.Level1)
{
    static constexpr auto propName = "value";
    Object o1(nullptr);
    Object o2(nullptr);

    EXPECT_FALSE(o1);
    EXPECT_FALSE(o2);
    o2 = o1; // Does nothing
    EXPECT_FALSE(o1);
    EXPECT_FALSE(o2);
    o1 = CreateObjectInstance(ClassId::Object);
    EXPECT_TRUE(o1);
    EXPECT_FALSE(o2);
    o2 = o1;
    EXPECT_TRUE(o1);
    EXPECT_TRUE(o2);

    Object o3(nullptr);
    EXPECT_FALSE(o3);
    EXPECT_TRUE(Metadata(o1).AddProperty(propName, 5));
    o3 = o1;
    EXPECT_TRUE(o1);
    EXPECT_TRUE(o2);
    EXPECT_TRUE(o3);
    EXPECT_EQ(o1, o2);
    EXPECT_EQ(o1, o3);

    auto p1 = Metadata(o1).GetProperty(propName);
    auto p2 = Metadata(o1).GetProperty(propName);
    auto p3 = Metadata(o1).GetProperty(propName);
    EXPECT_EQ(p1, p3);
    EXPECT_EQ(p2, p1);
}

/**
 * @tc.name: Instantiate
 * @tc.desc: Tests for Instantiate. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(ObjectTest, Instantiate, testing::ext::TestSize.Level1)
{
    Object o1(CreateNew);
    Object o2(nullptr);

    EXPECT_TRUE(o1);
    EXPECT_FALSE(o2);
}

/**
 * @tc.name: ResetIObject
 * @tc.desc: Tests for Reset Iobject. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectTest, ResetIObject, testing::ext::TestSize.Level1)
{
    META_NS::Object object1(CreateObjectInstance(ClassId::Object));
    META_NS::Object object2(CreateObjectInstance(ClassId::Object));
    auto iobject1 = object1.GetPtr();
    object2 = object1;
    auto iobject2 = object2.GetPtr();
    EXPECT_EQ(iobject1, iobject2);

    object1 = nullptr;
    EXPECT_FALSE(object1);
    EXPECT_NE(object1.GetPtr(), iobject2);
}

/**
 * @tc.name: ObjectName
 * @tc.desc: Tests for Object Name. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectTest, ObjectName, testing::ext::TestSize.Level1)
{
    {
        Object object(CreateObjectInstance(ClassId::Object));
        EXPECT_TRUE(SetName(object, "Test"));
        EXPECT_EQ(GetName(object), "Test");
        EXPECT_TRUE(GetObjectNameAttachment(interface_pointer_cast<IAttach>(object)));
    }
    {
        auto op = GetObjectRegistry().Create<IObjectInstance>(ClassId::BaseObject);
        EXPECT_FALSE(SetName(op, "Test"));
        EXPECT_EQ(GetName(op), op->GetInstanceId().ToString());
    }
    {
        PropertyAnimation anim(CreateObjectInstance(ClassId::PropertyAnimation));
        EXPECT_TRUE(SetName(anim, "Test"));
        EXPECT_EQ(GetName(anim), "Test");
        EXPECT_FALSE(GetObjectNameAttachment(interface_pointer_cast<IAttach>(anim)));
    }
}

/**
 * @tc.name: ObjectNameSerialise
 * @tc.desc: Tests for Object Name Serialise. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectTest, ObjectNameSerialise, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    {
        Object object(CreateObjectInstance(ClassId::Object));
        EXPECT_TRUE(SetName(object, "Test"));
        ser.Export(object);
    }
    auto in = ser.Import();
    ASSERT_TRUE(in);
    EXPECT_EQ(GetName(in), "Test");
    EXPECT_TRUE(GetObjectNameAttachment(interface_pointer_cast<IAttach>(in)));
}

} // namespace UTest

META_END_NAMESPACE()
