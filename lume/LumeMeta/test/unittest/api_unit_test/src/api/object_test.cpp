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
 * @tc.name: LazyInitDerived
 * @tc.desc: Tests for Lazy Init Derived. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
UNIT_TEST(API_ObjectTest, LazyInitDerived, testing::ext::TestSize.Level1)
{
    auto p1 = PropertyAnimation();
    PropertyAnimation p2;
    EXPECT_FALSE(p1);
    EXPECT_FALSE(p2);
    p2 = p1;
    EXPECT_TRUE(p1);
    EXPECT_TRUE(p2);
    EXPECT_EQ(p1, p2);

    PropertyAnimation p3;
    p1.Duration(TimeSpan::Milliseconds(42));
    p3 = p1;
    EXPECT_TRUE(p1);
    EXPECT_TRUE(p2);
    EXPECT_TRUE(p3);
    EXPECT_EQ(p1, p3);

    EXPECT_EQ(p1.TotalDuration().Get(), p2.TotalDuration().Get());
    EXPECT_EQ(p1.TotalDuration().Get(), p3.TotalDuration().Get());
}
*/

/**
 * @tc.name: Initialize
 * @tc.desc: Tests for Initialize. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
UNIT_TEST(API_ObjectTest, Initialize, testing::ext::TestSize.Level1)
{
    static constexpr auto propName = "value";
    const int value = 32;
    auto p1 = PropertyAnimation();
    PropertyAnimation p2;
    p1.MetaProperty(propName, value);
    p1.Duration(TimeSpan::Milliseconds(100));

    EXPECT_TRUE(p2.Initialize(p1));
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(p1.TotalDuration().Get(), p2.TotalDuration().Get());

    auto prop = property_cast<int>(p2.Metadata().GetProperty(propName));
    ASSERT_NE(prop, nullptr);
    EXPECT_EQ(prop->GetValue(), value);
}
*/

/**
 * @tc.name: Store
 * @tc.desc: Tests for Store. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
UNIT_TEST(API_ObjectTest, Store, testing::ext::TestSize.Level1)
{
    PropertyAnimation p1, p2;
    p1.Duration(TimeSpan::Milliseconds(100));
    p2.Duration(TimeSpan::Milliseconds(200));
    p2.Store(p1);
    EXPECT_EQ(p2.GetIObject(), p1.GetIObject());
    EXPECT_EQ(p2.Duration().Get(), p1.Duration().Get());
    // Should now point to the same object
    p2.Duration(TimeSpan::Milliseconds(300));
    EXPECT_EQ(p2.Duration().Get(), p1.Duration().Get());
    // Store to uninitialized
    PropertyAnimation p3;
    p2.Store(p3);
    EXPECT_EQ(p2.Duration().Get(), p3.Duration().Get());
    // Store uninitialized to uninitialized
    PropertyAnimation p4, p5;
    p4.Store(p5);
    EXPECT_NE(p4.GetIObject(), nullptr);
    EXPECT_EQ(p4, p5);
    p4.Duration(TimeSpan::Milliseconds(500));
    EXPECT_EQ(p4.Duration().Get(), p5.Duration().Get());
}
*/

/**
 * @tc.name: CachedInterfaces
 * @tc.desc: Tests for Cached Interfaces. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
UNIT_TEST(API_ObjectTest, CachedInterfaces, testing::ext::TestSize.Level1)
{
    // Check that also properties accessed through interfaces are updated
    // when the object reference changes, meaning that e.g. any
    // cached interface references get updated
    static constexpr auto propName = "value";
    auto p1 = PropertyAnimation();
    PropertyAnimation p2;
    PropertyAnimation p3;
    p1.MetaProperty(propName, 32);

    p1.Duration(TimeSpan::Milliseconds(100));
    p2.Duration(TimeSpan::Milliseconds(50));
    p3.Duration(TimeSpan::Milliseconds(25));
    PropertyAnimation p4(p3);

    EXPECT_NE(p1.TotalDuration().Get(), p2.TotalDuration().Get());
    EXPECT_NE(p1.TotalDuration().Get(), p3.TotalDuration().Get());

    EXPECT_EQ(p3, p4);
    EXPECT_EQ(p4.TotalDuration().Get(), p3.TotalDuration().Get());
    p3.Duration(TimeSpan::Milliseconds(15));
    EXPECT_EQ(p3.TotalDuration().Get(), TimeSpan::Milliseconds(15));
    EXPECT_EQ(p4.TotalDuration().Get(), p3.TotalDuration().Get());

    // Assign, p1 already has cached IAnimation interface, so this
    // checks that the cached interface is also updated to point
    // to the new target
    p1 = p2;
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(p1.TotalDuration().Get(), p2.TotalDuration().Get());

    // Should fail as p2 already initialized
    EXPECT_FALSE(p2.Initialize(p3));
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(p1.TotalDuration().Get(), p2.TotalDuration().Get());

    p1 = p3;
    // Should not be there anymore as object changed
    EXPECT_EQ(p1.Metadata().GetProperty(propName), nullptr);
}
*/

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
 * @tc.name: ResetIObjectDerived
 * @tc.desc: Tests for Reset Iobject Derived. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
UNIT_TEST(API_ObjectTest, ResetIObjectDerived, testing::ext::TestSize.Level1)
{
    using namespace META_NS::TimeSpanLiterals;

    PropertyAnimation p1;
    PropertyAnimation p2;
    p1.Duration(400_ms);
    p2 = p1;
    EXPECT_EQ(p1, p2);

    EXPECT_EQ(GetValue(p1.Duration()), GetValue(p2.Duration()));
    p1.ResetIObject();
    EXPECT_FALSE(p1);
    EXPECT_EQ(GetValue(p2.Duration()), 400_ms);
    p1.Duration(200_ms); // Creates a new object and sets the value
    EXPECT_EQ(GetValue(p1.Duration()), 200_ms);
    // P2 should not have anything to do with p1 anymore
    EXPECT_EQ(GetValue(p2.Duration()), 400_ms);
}
*/

/**
 * @tc.name: CopyiedUserApiObjectContainsPointerToOriginalObject
 * @tc.desc: Tests for Copyied User Api Object Contains Pointer To Original Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
UNIT_TEST(API_ObjectTest, CopyiedUserApiObjectContainsPointerToOriginalObject, testing::ext::TestSize.Level1)
{
    // given
    auto origin = PropertyAnimation();

    // when
    auto copy = origin;

    // expected
    const auto originPtr = origin.GetIObject().get();
    const auto copyPtr = copy.GetIObject().get();

    EXPECT_THAT(copy, Eq(origin));
    EXPECT_THAT(copyPtr, Eq(originPtr));
}
*/

/**
 * @tc.name: AssignmentUserApiObjectContainsPointerToOriginalObject
 * @tc.desc: Tests for Assignment User Api Object Contains Pointer To Original Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
UNIT_TEST(API_ObjectTest, AssignmentUserApiObjectContainsPointerToOriginalObject, testing::ext::TestSize.Level1)
{
    // given
    auto origin = PropertyAnimation();
    auto assigned = PropertyAnimation();

    // when
    assigned = origin;

    // expected
    const auto originPtr = origin.GetIObject().get();
    const auto assignedPtr = assigned.GetIObject().get();

    EXPECT_THAT(assigned, Eq(origin));
    EXPECT_THAT(assignedPtr, Eq(originPtr));
}
*/

/**
 * @tc.name: MoveConstructedObjectContainsPointerFromOriginAndRemovesContentFromIt
 * @tc.desc: Tests for Move Constructed Object Contains Pointer From Origin And Removes Content From It.
 * [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
UNIT_TEST(API_ObjectTest, MoveConstructedObjectContainsPointerFromOriginAndRemovesContentFromIt,
testing::ext::TestSize.Level1)
{
    // given
    auto origin = PropertyAnimation();
    auto originRawPtr = origin.GetIObject().get();

    // when
    auto moved = std::move(origin);

    // expected
    const auto isEmptyAfterMove = !static_cast<bool>(origin);
    const auto originPtr = origin.GetIObject().get();
    const auto movedPtr = moved.GetIObject().get();

    // GetIObject creates new object if it is deinitialized, so it is not nullptr
    EXPECT_THAT(isEmptyAfterMove, Eq(true));
    EXPECT_THAT(originPtr, Ne(originRawPtr));
    EXPECT_THAT(movedPtr, Eq(originRawPtr));
}
*/

/**
 * @tc.name: MoveAssignedObjectContainsPointerFromOriginAndRemovesContentFromIt
 * @tc.desc: Tests for Move Assigned Object Contains Pointer From Origin And Removes Content From It. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
UNIT_TEST(API_ObjectTest, MoveAssignedObjectContainsPointerFromOriginAndRemovesContentFromIt,
testing::ext::TestSize.Level1)
{
    // given
    auto origin = PropertyAnimation();
    auto moveAssigned = PropertyAnimation();

    auto originRawPtr = origin.GetIObject().get();

    // when
    moveAssigned = std::move(origin);

    // expected
    const auto isOriginEmptyAfterMove = !static_cast<bool>(origin);
    const auto isAssignedEmptyAfterMove = !static_cast<bool>(moveAssigned);
    const auto originPtr = origin.GetIObject().get();
    const auto movedPtr = moveAssigned.GetIObject().get();

    // GetIObject creates new object if it is deinitialized, so it is not nullptr
    EXPECT_THAT(isOriginEmptyAfterMove, Eq(true));
    EXPECT_THAT(isAssignedEmptyAfterMove, Eq(false));
    EXPECT_THAT(originPtr, Ne(originRawPtr));
    EXPECT_THAT(movedPtr, Eq(originRawPtr));
}
*/

/**
 * @tc.name: NotCompatibileMoveAssignedObjectIsEmptyAndMovedObjectIsEmptyAlso
 * @tc.desc: Tests for Not Compatibile Move Assigned Object Is Empty And Moved Object Is Empty Also. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
UNIT_TEST(API_ObjectTest, NotCompatibileMoveAssignedObjectIsEmptyAndMovedObjectIsEmptyAlso,
testing::ext::TestSize.Level1)
{
    // given
    auto sequentialAnimation = SequentialAnimation();
    auto parallelAnimation = ParallelAnimation();

    auto initialSequentialAnimationPtr = sequentialAnimation.GetIObject().get();
    auto initialParallelAnimationPtr = parallelAnimation.GetIObject().get();

    // when
    parallelAnimation = std::move(sequentialAnimation);

    // expected
    const bool isSequentialAnimationEmptyAfterMove = !static_cast<bool>(sequentialAnimation);
    const bool isParallelAnimationEmptyAfterMove = !static_cast<bool>(parallelAnimation);

    EXPECT_THAT(isSequentialAnimationEmptyAfterMove, Eq(true));
    EXPECT_THAT(isParallelAnimationEmptyAfterMove, Eq(true));
}
*/

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
