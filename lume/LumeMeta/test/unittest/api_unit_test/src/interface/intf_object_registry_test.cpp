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

#include <bitset>
#include <limits>
#include <test_framework.h>
#include <type_traits>

#include <base/util/uid_util.h>

#include <meta/api/animation.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/curves/intf_easing_curve.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_proxy_object.h>
#include <meta/interface/intf_task_queue_registry.h>

#include "helpers/util.h"

META_BEGIN_NAMESPACE()
namespace UTest {

class API_ObjectRegistryAPI : public ::testing::TestWithParam<ObjectCategoryBits> {};

/**
 * @tc.name: GetAllCategories
 * @tc.desc: Tests for Get All Categories. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ObjectRegistryAPI, GetAllCategories, testing::ext::TestSize.Level1)
{
    ObjectCategoryBits category = GetParam();

    auto& objectRegistry = GetObjectRegistry();
    auto categories = objectRegistry.GetAllCategories();
    EXPECT_TRUE(categories.size() > 0);

    if (category != ObjectCategoryBits::ANY) {
        // Make sure that given category is found in category list
        bool found = false;
        for (auto& c : categories) {
            if (c.category == category)
                found = true;
        }
        EXPECT_TRUE(found);
    }
}

/**
 * @tc.name: GetAllTypes
 * @tc.desc: Tests for Get All Types. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ObjectRegistryAPI, GetAllTypes, testing::ext::TestSize.Level1)
{
    auto& objectRegistry = GetObjectRegistry();
    auto types = objectRegistry.GetAllTypes(GetParam());

    // Make sure that given category contains >0 types
    EXPECT_TRUE(types.size() > 0);
}

/**
 * @tc.name: GetAllTypesAllCategories
 * @tc.desc: Tests for Get All Types All Categories. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ObjectRegistryAPI, GetAllTypesAllCategories, testing::ext::TestSize.Level1)
{
    auto& objectRegistry = GetObjectRegistry();
    auto types = objectRegistry.GetAllTypes(GetParam());
    auto allTypes = objectRegistry.GetAllTypes(ObjectCategoryBits::ANY);

    EXPECT_TRUE(types.size() > 0);
    EXPECT_TRUE(allTypes.size() > 0);

    // List of all types regardless of category should always be
    // larger than list of types for any one category.
    if (GetParam() != ObjectCategoryBits::ANY) {
        EXPECT_GT(allTypes.size(), types.size());
    } else {
        EXPECT_EQ(allTypes.size(), types.size());
    }
}

/**
 * @tc.name: Create
 * @tc.desc: Tests for Create. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ObjectRegistryAPI, Create, testing::ext::TestSize.Level1)
{
    auto& objectRegistry = GetObjectRegistry();
    auto types = objectRegistry.GetAllTypes(GetParam());

    ASSERT_TRUE(types.size() > 0);

    auto testBaseInterfaces = [](const IObject::Ptr& o) {
        if (auto instance = interface_cast<IObjectInstance>(o)) {
            EXPECT_NE(instance->GetInstanceId(), BASE_NS::Uid {});
            EXPECT_TRUE(instance->GetSelf());
            EXPECT_NE(instance->GetClassId(), BASE_NS::Uid {});
            EXPECT_FALSE(instance->GetInterfaces().empty());
            EXPECT_FALSE(instance->GetName().empty());
        }
        if (auto meta = interface_cast<IStaticMetadata>(o)) {
            ASSERT_TRUE(meta->GetStaticMetadata());
            EXPECT_NE(meta->GetStaticMetadata()->classInfo, nullptr);
        }
    };

    // Make sure that all objects of given category can be successfully created
    // and that class id of the created object matches the objectId of the type info.
    for (auto& t : types) {
        IObject::Ptr o;
        BASE_NS::weak_ptr<IObject> weak;
        CORE_NS::IInterface* oo {};
        {
            o = objectRegistry.Create(t->GetClassInfo());
            ASSERT_TRUE(o);
            EXPECT_EQ(o->GetClassId(), t->GetClassInfo());
            weak = o;
            oo = interface_cast<CORE_NS::IInterface>(o.get());
            ASSERT_TRUE(oo);
            oo->Ref(); // +1
        }
        oo->Ref();                    // +1
        EXPECT_FALSE(weak.expired()); // Still alive
        o.reset();
        oo->Unref();                 // -1
        EXPECT_TRUE(weak.expired()); // shared_ptr died
        oo->Unref();                 // -1
    }
}

INSTANTIATE_TEST_SUITE_P(API_ObjectRegistryTest, API_ObjectRegistryAPI,
    ::testing::Values(META_NS::ObjectCategoryBits::NO_CATEGORY, META_NS::ObjectCategoryBits::ANIMATION,
        META_NS::ObjectCategoryBits::INTERNAL, META_NS::ObjectCategoryBits::CURVE, META_NS::ObjectCategoryBits::ANY));

/**
 * @tc.name: Singleton
 * @tc.desc: Tests for Singleton. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, Singleton, testing::ext::TestSize.Level1)
{
    auto& objectRegistry = GetObjectRegistry();
    META_NS::IEasingCurve::WeakPtr ptr;

    {
        auto w1 = objectRegistry.Create<META_NS::IEasingCurve>(META_NS::ClassId::InBackEasingCurve);
        // Should get the same object
        auto w2 = objectRegistry.Create<META_NS::IEasingCurve>(META_NS::ClassId::InBackEasingCurve);
        EXPECT_EQ(w1.get(), w2.get());
        ptr = w2;
    }

    // Singleton object should have been destroyed since all IObject::Ptrs pointing to it went out of scope
    EXPECT_EQ(ptr.lock().get(), nullptr);

    // Create new singleton, we should get a new object now
    auto w3 = objectRegistry.Create<META_NS::IEasingCurve>(META_NS::ClassId::InBackEasingCurve);
}

/**
 * @tc.name: Global
 * @tc.desc: Tests for Global. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, Global, testing::ext::TestSize.Level1)
{
    constexpr BASE_NS::Uid uid { "11140d0f-4ec5-455c-a66d-dcd749b08194" };

    auto& objectRegistry = GetObjectRegistry();
    auto obj = objectRegistry.Create<IObjectInstance>(META_NS::ClassId::Object, { uid, true });
    ASSERT_TRUE(obj);
    auto g = objectRegistry.GetGlobalSerializationData().GetGlobalObject(obj->GetInstanceId());
    ASSERT_EQ(g, obj);
}

/**
 * @tc.name: Invalid
 * @tc.desc: Tests for Invalid. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, Invalid, testing::ext::TestSize.Level1)
{
    static constexpr BASE_NS::Uid invalid { "852f8748-e6fb-4b9b-bd49-b48d13c0d1d6" };
    auto& objectRegistry = GetObjectRegistry();

    constexpr size_t bits = std::numeric_limits<std::underlying_type_t<META_NS::ObjectCategoryBits>>::digits;

    std::bitset<bits> invalidBits;
    invalidBits.reset();
    invalidBits[bits - 2] = true; // Assume that bit at index 62 is not used in ObjectCategoryBits

    auto invalidCategory = static_cast<META_NS::ObjectCategoryBits>(invalidBits.to_ullong());

    // Invalid category
    auto types = objectRegistry.GetAllTypes(invalidCategory);
    EXPECT_EQ(types.size(), 0);

    // Invalid object uid
    auto o = objectRegistry.Create(invalid);
    EXPECT_EQ(o, nullptr);
}

META_REGISTER_CLASS(Transform, "17475c8e-5030-49ba-be6d-433d56ad1242",
    static_cast<META_NS::ObjectCategoryBits>(META_NS::ObjectCategoryBits::WIDGET | META_NS::ObjectCategoryBits::LAYOUT |
                                             META_NS::ObjectCategoryBits::CONTAINER))

class Transform final : public META_NS::ObjectFwd {
    META_OBJECT(Transform, ClassId::Transform, ObjectFwd)
};

META_REGISTER_CLASS(MyWidget, "11111c8e-5030-49ba-be6d-433d56ad1242",
    static_cast<META_NS::ObjectCategoryBits>(META_NS::ObjectCategoryBits::WIDGET))

class MyWidget final : public META_NS::ObjectFwd {
    META_OBJECT(MyWidget, ClassId::MyWidget, ObjectFwd)
};

META_REGISTER_CLASS(DeprecatedTransform, "be2ad0e4-03cf-4819-8a05-b79ed4244153",
    static_cast<META_NS::ObjectCategoryBits>(META_NS::ObjectCategoryBits::WIDGET | META_NS::ObjectCategoryBits::LAYOUT |
                                             META_NS::ObjectCategoryBits::CONTAINER |
                                             META_NS::ObjectCategoryBits::DEPRECATED))

class DeprecatedTransform final : public META_NS::ObjectFwd {
    META_OBJECT(DeprecatedTransform, ClassId::DeprecatedTransform, ObjectFwd)
public:
};

META_REGISTER_CLASS(DeprecatedMyWidget, "5d190f6d-7b23-4298-b6c1-0266b1aae6ba",
    static_cast<META_NS::ObjectCategoryBits>(META_NS::ObjectCategoryBits::WIDGET))

class DeprecatedMyWidget final : public META_NS::ObjectFwd {
    META_OBJECT(DeprecatedMyWidget, ClassId::DeprecatedMyWidget, ObjectFwd)
};

/**
 * @tc.name: GetTypesBits
 * @tc.desc: Tests for Get Types Bits. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, GetTypesBits, testing::ext::TestSize.Level1)
{
    auto& objectRegistry = GetObjectRegistry();
    objectRegistry.RegisterObjectType<Transform>();
    objectRegistry.RegisterObjectType<MyWidget>();
    objectRegistry.RegisterObjectType<DeprecatedTransform>();
    objectRegistry.RegisterObjectType<DeprecatedMyWidget>();
    // All object types which have WIDGET flag set
    auto widgetTypes = objectRegistry.GetAllTypes(META_NS::ObjectCategoryBits::WIDGET);
    // All object types which have LAYOUT flag set
    auto layoutTypes = objectRegistry.GetAllTypes(META_NS::ObjectCategoryBits::LAYOUT);
    // All object types which have WIDGET | LAYOUT flags set (most likely the same as gettin
    // a list of object types which have LAYOUT set as mostly our LAYOUTs are also WIDGETs)
    auto combo = META_NS::ObjectCategoryBits::LAYOUT | META_NS::ObjectCategoryBits::WIDGET;
    auto comboTypes = objectRegistry.GetAllTypes(static_cast<META_NS::ObjectCategoryBits>(combo));

    // From our list of WIDGET type objects, separate objects which have LAYOUT flag
    // set and which do not.
    BASE_NS::vector<IClassInfo::ConstPtr> onlyWidgetFlagSetTypes;
    BASE_NS::vector<IClassInfo::ConstPtr> bothWidgetAndLayoutFlagSetTypes;
    for (const auto& type : widgetTypes) {
        if ((type->GetClassInfo().category & META_NS::ObjectCategoryBits::LAYOUT) == 0) {
            onlyWidgetFlagSetTypes.emplace_back(type);
        } else {
            bothWidgetAndLayoutFlagSetTypes.emplace_back(type);
        }
    }

    EXPECT_TRUE(onlyWidgetFlagSetTypes.size() > 0);
    EXPECT_TRUE(layoutTypes.size() > 0);
    EXPECT_EQ(bothWidgetAndLayoutFlagSetTypes.size(), comboTypes.size());
    EXPECT_EQ(onlyWidgetFlagSetTypes.size() + layoutTypes.size(), widgetTypes.size());
    EXPECT_EQ(widgetTypes.size() - layoutTypes.size(), onlyWidgetFlagSetTypes.size());

    BASE_NS::vector<ClassInfo> comboClassesInfo;
    for (const auto& t : comboTypes) {
        comboClassesInfo.push_back(t->GetClassInfo());
    }

    BASE_NS::vector<ClassInfo> layoutClassesInfo;
    for (const auto& t : layoutTypes) {
        layoutClassesInfo.push_back(t->GetClassInfo());
    }

    EXPECT_THAT(comboClassesInfo, testing::UnorderedElementsAre(ClassId::Transform));
    EXPECT_THAT(layoutClassesInfo, testing::UnorderedElementsAre(ClassId::Transform));

    objectRegistry.UnregisterObjectType<MyWidget>();
    objectRegistry.UnregisterObjectType<Transform>();
    objectRegistry.UnregisterObjectType<DeprecatedTransform>();
    objectRegistry.UnregisterObjectType<DeprecatedMyWidget>();
}

/**
 * @tc.name: GetTypesBitsIncludingDeprecated
 * @tc.desc: Tests for Get Types Bits Including Deprecated. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, GetTypesBitsIncludingDeprecated, testing::ext::TestSize.Level1)
{
    auto& objectRegistry = GetObjectRegistry();
    objectRegistry.RegisterObjectType<Transform>();
    objectRegistry.RegisterObjectType<MyWidget>();
    objectRegistry.RegisterObjectType<DeprecatedTransform>();
    objectRegistry.RegisterObjectType<DeprecatedMyWidget>();
    // All object types which have WIDGET flag set
    auto widgetTypes = objectRegistry.GetAllTypes(META_NS::ObjectCategoryBits::WIDGET, true, false);
    // All object types which have LAYOUT flag set
    auto layoutTypes = objectRegistry.GetAllTypes(META_NS::ObjectCategoryBits::LAYOUT, true, false);
    // All object types which have WIDGET | LAYOUT flags set (most likely the same as gettin
    // a list of object types which have LAYOUT set as mostly our LAYOUTs are also WIDGETs)
    auto combo = META_NS::ObjectCategoryBits::LAYOUT | META_NS::ObjectCategoryBits::WIDGET;
    auto comboTypes = objectRegistry.GetAllTypes(static_cast<META_NS::ObjectCategoryBits>(combo), true, false);

    // From our list of WIDGET type objects, separate objects which have LAYOUT flag
    // set and which do not.
    BASE_NS::vector<IClassInfo::ConstPtr> onlyWidgetFlagSetTypes;
    BASE_NS::vector<IClassInfo::ConstPtr> bothWidgetAndLayoutFlagSetTypes;
    for (const auto& type : widgetTypes) {
        if ((type->GetClassInfo().category & META_NS::ObjectCategoryBits::LAYOUT) == 0) {
            onlyWidgetFlagSetTypes.emplace_back(type);
        } else {
            bothWidgetAndLayoutFlagSetTypes.emplace_back(type);
        }
    }

    EXPECT_TRUE(onlyWidgetFlagSetTypes.size() > 0);
    EXPECT_TRUE(layoutTypes.size() > 0);
    EXPECT_EQ(bothWidgetAndLayoutFlagSetTypes.size(), comboTypes.size());
    EXPECT_EQ(onlyWidgetFlagSetTypes.size() + layoutTypes.size(), widgetTypes.size());
    EXPECT_EQ(widgetTypes.size() - layoutTypes.size(), onlyWidgetFlagSetTypes.size());

    BASE_NS::vector<ClassInfo> comboClassesInfo;
    for (const auto& t : comboTypes) {
        comboClassesInfo.push_back(t->GetClassInfo());
    }

    BASE_NS::vector<ClassInfo> layoutClassesInfo;
    for (const auto& t : layoutTypes) {
        layoutClassesInfo.push_back(t->GetClassInfo());
    }

    EXPECT_THAT(comboClassesInfo, testing::UnorderedElementsAre(ClassId::Transform, ClassId::DeprecatedTransform));
    EXPECT_THAT(layoutClassesInfo, testing::UnorderedElementsAre(ClassId::Transform, ClassId::DeprecatedTransform));

    objectRegistry.UnregisterObjectType<MyWidget>();
    objectRegistry.UnregisterObjectType<Transform>();
    objectRegistry.UnregisterObjectType<DeprecatedTransform>();
    objectRegistry.UnregisterObjectType<DeprecatedMyWidget>();
}

/**
 * @tc.name: GetTypesBitsStrict
 * @tc.desc: Tests for Get Types Bits Strict. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, GetTypesBitsStrict, testing::ext::TestSize.Level1)
{
    auto& objectRegistry = GetObjectRegistry();
    objectRegistry.RegisterObjectType(Transform::GetFactory());
    objectRegistry.RegisterObjectType(MyWidget::GetFactory());
    objectRegistry.RegisterObjectType(DeprecatedTransform::GetFactory());
    objectRegistry.RegisterObjectType(DeprecatedMyWidget::GetFactory());

    auto widgetTypes = objectRegistry.GetAllTypes(META_NS::ObjectCategoryBits::WIDGET);
    auto animationTypes = objectRegistry.GetAllTypes(META_NS::ObjectCategoryBits::ANIMATION);
    auto combo = META_NS::ObjectCategoryBits::ANIMATION | META_NS::ObjectCategoryBits::WIDGET;
    auto comboTypes = objectRegistry.GetAllTypes(static_cast<META_NS::ObjectCategoryBits>(combo));
    // Get list of all types which have any of the given bits set
    auto comboTypesAny = objectRegistry.GetAllTypes(static_cast<META_NS::ObjectCategoryBits>(combo), false, true);

    EXPECT_GT(widgetTypes.size(), 0);
    EXPECT_GT(animationTypes.size(), 0);
    EXPECT_GT(comboTypesAny.size(), 0);
    EXPECT_EQ(comboTypes.size(), 0);
    EXPECT_EQ(widgetTypes.size() + animationTypes.size(), comboTypesAny.size());

    objectRegistry.UnregisterObjectType(MyWidget::GetFactory());
    objectRegistry.UnregisterObjectType(Transform::GetFactory());
    objectRegistry.UnregisterObjectType(DeprecatedTransform::GetFactory());
    objectRegistry.UnregisterObjectType(DeprecatedMyWidget::GetFactory());
}

/**
 * @tc.name: GetTypesBitsStrictIncludingDeprecated
 * @tc.desc: Tests for Get Types Bits Strict Including Deprecated. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, GetTypesBitsStrictIncludingDeprecated, testing::ext::TestSize.Level1)
{
    auto& objectRegistry = GetObjectRegistry();
    objectRegistry.RegisterObjectType(Transform::GetFactory());
    objectRegistry.RegisterObjectType(MyWidget::GetFactory());
    objectRegistry.RegisterObjectType(DeprecatedTransform::GetFactory());
    objectRegistry.RegisterObjectType(DeprecatedMyWidget::GetFactory());

    auto widgetTypes = objectRegistry.GetAllTypes(META_NS::ObjectCategoryBits::WIDGET, true, false);
    auto animationTypes = objectRegistry.GetAllTypes(META_NS::ObjectCategoryBits::ANIMATION, true, false);
    auto combo = META_NS::ObjectCategoryBits::ANIMATION | META_NS::ObjectCategoryBits::WIDGET;
    auto comboTypes = objectRegistry.GetAllTypes(static_cast<META_NS::ObjectCategoryBits>(combo), true, false);
    // Get list of all types which have any of the given bits set
    auto comboTypesAny = objectRegistry.GetAllTypes(static_cast<META_NS::ObjectCategoryBits>(combo), false, false);

    EXPECT_GT(widgetTypes.size(), 0);
    EXPECT_GT(animationTypes.size(), 0);
    EXPECT_GT(comboTypesAny.size(), 0);
    EXPECT_EQ(comboTypes.size(), 0);
    EXPECT_EQ(widgetTypes.size() + animationTypes.size(), comboTypesAny.size());

    objectRegistry.UnregisterObjectType(MyWidget::GetFactory());
    objectRegistry.UnregisterObjectType(Transform::GetFactory());
    objectRegistry.UnregisterObjectType(DeprecatedTransform::GetFactory());
    objectRegistry.UnregisterObjectType(DeprecatedMyWidget::GetFactory());
}

/**
 * @brief Checks which items from <find> are found from <list>
 * @param list The list which contains all of the items
 * @param find The list which contains the items to find from list
 * @return An array of find.size() which contains a boolean for each item (true if found, false otherwise)
 */
BASE_NS::vector<bool> CheckObjectList(
    const BASE_NS::vector<META_NS::IObject::Ptr>& list, const BASE_NS::vector<META_NS::IObject::Ptr>& find)
{
    BASE_NS::vector<bool> result(find.size(), false);
    size_t idx = 0;
    for (const auto& item : find) {
        for (const auto& listItem : list) {
            if (listItem == item) {
                result[idx] = true;
            }
        }
        idx++;
    }
    return result;
}

/**
 * @tc.name: GetObjectInstancesByCategory
 * @tc.desc: Tests for Get Object Instances By Category. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, GetObjectInstancesByCategory, testing::ext::TestSize.Level1)
{
    auto& objectRegistry = GetObjectRegistry();
    objectRegistry.RegisterObjectType<Transform>();
    auto widgetCountBefore = objectRegistry.GetObjectInstancesByCategory(META_NS::ObjectCategoryBits::WIDGET).size();
    auto animationCountBefore =
        objectRegistry.GetObjectInstancesByCategory(META_NS::ObjectCategoryBits::ANIMATION).size();

    // Create an animation, instance count in ANIMATION category should increase by 1
    auto animObject = objectRegistry.Create(META_NS::ClassId::KeyframeAnimation);
    auto animationCountAfter =
        objectRegistry.GetObjectInstancesByCategory(META_NS::ObjectCategoryBits::ANIMATION).size();
    auto widgetCountAfter = objectRegistry.GetObjectInstancesByCategory(META_NS::ObjectCategoryBits::WIDGET).size();
    EXPECT_EQ(animationCountAfter, animationCountBefore + 1);
    EXPECT_EQ(widgetCountAfter, widgetCountBefore);
    animationCountBefore = animationCountAfter;

    // Create a widget, instance count in WIDGET category should increase by 1
    auto widgetObject = objectRegistry.Create(ClassId::Transform);
    animationCountAfter = objectRegistry.GetObjectInstancesByCategory(META_NS::ObjectCategoryBits::ANIMATION).size();
    widgetCountAfter = objectRegistry.GetObjectInstancesByCategory(META_NS::ObjectCategoryBits::WIDGET).size();
    EXPECT_EQ(widgetCountAfter, widgetCountBefore + 1);
    EXPECT_EQ(animationCountAfter, animationCountBefore);

    // Get all instances using a bitmask, in both strictness modes
    auto combo = static_cast<META_NS::ObjectCategoryBits>(
        META_NS::ObjectCategoryBits::ANIMATION | META_NS::ObjectCategoryBits::WIDGET);
    auto instancesStrict = objectRegistry.GetObjectInstancesByCategory(combo);  // Exact category match
    auto instances = objectRegistry.GetObjectInstancesByCategory(combo, false); // Any bit
    auto animationInstances = objectRegistry.GetObjectInstancesByCategory(META_NS::ObjectCategoryBits::ANIMATION);
    auto widgetInstances = objectRegistry.GetObjectInstancesByCategory(META_NS::ObjectCategoryBits::WIDGET);

    // There should not be any objects with both ANIMATION and WIDGET category
    EXPECT_EQ(instancesStrict.size(), 0);
    // In lenient mode the returned list should contain all objects which have either WIDGET and ANIMATION category set
    EXPECT_EQ(instances.size(), widgetCountAfter + animationCountAfter);

    BASE_NS::vector<META_NS::IObject::Ptr> objects = { widgetObject, animObject };
    // ANIMATION | WIDGET bitmask in non-strict mode, should contain both objects
    auto result = CheckObjectList(instances, objects);
    EXPECT_TRUE(result[0]);
    EXPECT_TRUE(result[1]);
    result = CheckObjectList(animationInstances, objects); // Animations, should contain the animation object
    EXPECT_FALSE(result[0]);
    EXPECT_TRUE(result[1]);
    result = CheckObjectList(widgetInstances, objects); // Widgets, should contain the widget object
    EXPECT_TRUE(result[0]);
    EXPECT_FALSE(result[1]);

    objectRegistry.UnregisterObjectType<Transform>();
}

class ITestSuper : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITestSuper, "5d80a9ab-f673-49b5-b18a-f16d0db50c96")
public:
    META_PROPERTY(float, Prop1)
    META_PROPERTY(uint32_t, Prop2)
    META_EVENT(IOnChanged, OnChanged)
    virtual void MyFunc() = 0;
};

class ITestDerived : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITestDerived, "7c5842fa-87f6-482b-8838-8b842fcce64f")
public:
    META_PROPERTY(float, DerProp1)
    META_PROPERTY(uint32_t, DerProp2)
    META_EVENT(IOnChanged, OtherOnChanged)
    virtual void MyFoo(float) const = 0;
};

META_REGISTER_CLASS(TestSuper, "a5bda4e1-0bd1-4b9e-ad90-d3c9e20a7149", META_NS::ObjectCategoryBits::APPLICATION)
META_REGISTER_CLASS(TestDerived, "d838ef64-6987-49f9-9130-dc45a9619c51", META_NS::ObjectCategoryBits::APPLICATION)

class TestSuper final : public IntroduceInterfaces<META_NS::ObjectFwd, ITestSuper> {
    META_OBJECT(TestSuper, ClassId::TestSuper, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(ITestSuper, float, Prop1)
    META_STATIC_PROPERTY_DATA(ITestSuper, uint32_t, Prop2)
    META_STATIC_EVENT_DATA(ITestSuper, IOnChanged, OnChanged)
    META_STATIC_FUNCTION_DATA(ITestSuper, MyFunc)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(float, Prop1)
    META_IMPLEMENT_PROPERTY(uint32_t, Prop2)
    META_IMPLEMENT_EVENT(IOnChanged, OnChanged)

    void MyFunc() override {}

public:
    bool Build(const IMetadata::Ptr& data) override
    {
        buildParam_ = data;
        superBuildCount_++;
        return true;
    }
    void SetSuperInstance(const META_NS::IObject::Ptr& aggr, const META_NS::IObject::Ptr& super) override
    {
        ObjectFwd::SetSuperInstance(aggr, super);
        superClass_ = super.get();
        superSetSuperInstanceCount_++;
    }

    uint32_t superBuildCount_ { 0 };
    uint32_t superSetSuperInstanceCount_ { 0 };
    IObject* superClass_ { nullptr };
    IMetadata::Ptr buildParam_;
};

class TestDerived final : public IntroduceInterfaces<META_NS::ObjectFwd, ITestDerived> {
    META_OBJECT(TestDerived, ClassId::TestDerived, IntroduceInterfaces, ClassId::TestSuper)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(ITestDerived, float, DerProp1)
    META_STATIC_PROPERTY_DATA(ITestDerived, uint32_t, DerProp2)
    META_STATIC_EVENT_DATA(ITestDerived, IOnChanged, OtherOnChanged)
    META_STATIC_FUNCTION_DATA(ITestDerived, MyFoo, "float")
    META_END_STATIC_DATA()
    META_IMPLEMENT_PROPERTY(float, DerProp1)
    META_IMPLEMENT_PROPERTY(uint32_t, DerProp2)
    META_IMPLEMENT_EVENT(IOnChanged, OtherOnChanged)

    void MyFoo(float) const override {}

public:
    bool Build(const IMetadata::Ptr& data) override
    {
        buildParam_ = data;
        derivedBuildCount_++;
        return true;
    }
    void SetSuperInstance(const META_NS::IObject::Ptr& aggr, const META_NS::IObject::Ptr& super) override
    {
        ObjectFwd::SetSuperInstance(aggr, super);
        superClass_ = reinterpret_cast<TestSuper*>(super.get());
        derivedSetSuperInstanceCount_++;
    }

    uint32_t derivedBuildCount_ { 0 };
    uint32_t derivedSetSuperInstanceCount_ { 0 };
    TestSuper* superClass_ { nullptr };
    IMetadata::Ptr buildParam_;
};

/**
 * @tc.name: Build
 * @tc.desc: Tests for Build. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, Build, testing::ext::TestSize.Level1)
{
    auto& registry = GetObjectRegistry();
    registry.RegisterObjectType<TestSuper>();
    registry.RegisterObjectType<TestDerived>();

    auto object = registry.Create(ClassId::TestDerived);
    ASSERT_NE(object, nullptr);

    auto* derived = reinterpret_cast<TestDerived*>(object.get());
    ASSERT_NE(derived->superClass_, nullptr);

    // Verify that Build and SetSuperInstance functions are called once per
    // object in the "inheritance" hierarchy
    EXPECT_EQ(derived->superClass_->superBuildCount_, 1);
    EXPECT_EQ(derived->superClass_->superSetSuperInstanceCount_, 1);
    EXPECT_EQ(derived->derivedBuildCount_, 1);
    EXPECT_EQ(derived->derivedSetSuperInstanceCount_, 1);

    EXPECT_FALSE(derived->buildParam_);
    EXPECT_FALSE(derived->superClass_->buildParam_);

    registry.UnregisterObjectType<TestDerived>();
    registry.UnregisterObjectType<TestSuper>();
}

static bool Contains(const BASE_NS::vector<BASE_NS::Uid>& vec, const BASE_NS::Uid& uid)
{
    for (size_t i = 0; i != vec.size(); ++i) {
        if (vec[i] == uid) {
            return true;
        }
    }
    return false;
}

template<typename Type>
static bool ContainsName(const BASE_NS::vector<Type>& vec, const BASE_NS::string& name)
{
    for (size_t i = 0; i != vec.size(); ++i) {
        if (vec[i].name == name) {
            return true;
        }
    }
    return false;
}

/**
 * @tc.name: ConstructWithParam
 * @tc.desc: Tests for Construct With Param. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, ConstructWithParam, testing::ext::TestSize.Level1)
{
    auto& registry = GetObjectRegistry();
    RegisterObjectType<TestSuper>();
    RegisterObjectType<TestDerived>();

    auto buildData = registry.Create<IMetadata>(META_NS::ClassId::Object);

    auto object = registry.Create<IObjectInstance>(ClassId::TestDerived, buildData);
    ASSERT_NE(object, nullptr);

    auto* derived = reinterpret_cast<TestDerived*>(object.get());
    ASSERT_NE(derived->superClass_, nullptr);

    EXPECT_EQ(derived->buildParam_, buildData);

    UnregisterObjectType<TestDerived>();
    UnregisterObjectType<TestSuper>();
}

/**
 * @tc.name: ConstructWithDefaultObjectFactory
 * @tc.desc: Tests for Construct With Default Object Factory. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, ConstructWithDefaultObjectFactory, testing::ext::TestSize.Level1)
{
    auto& registry = GetObjectRegistry();
    RegisterObjectType<TestSuper>();
    RegisterObjectType<TestDerived>();

    auto buildData = registry.Create<IMetadata>(META_NS::ClassId::Object);

    auto object = registry.Create(ClassId::TestDerived, buildData);
    ASSERT_NE(object, nullptr);

    auto* derived = reinterpret_cast<TestDerived*>(object.get());
    ASSERT_NE(derived->superClass_, nullptr);

    // Verify that Build and SetSuperInstance functions are called once per
    // object in the "inheritance" hierarchy
    EXPECT_EQ(derived->superClass_->superBuildCount_, 1);
    EXPECT_EQ(derived->superClass_->superSetSuperInstanceCount_, 1);
    EXPECT_EQ(derived->derivedBuildCount_, 1);
    EXPECT_EQ(derived->derivedSetSuperInstanceCount_, 1);

    EXPECT_EQ(derived->buildParam_, buildData);
    EXPECT_EQ(derived->superClass_->buildParam_, buildData);

    {
        auto interfaces = derived->GetInterfaces();
        EXPECT_TRUE(Contains(interfaces, ITestDerived::UID));
        EXPECT_TRUE(Contains(interfaces, IObject::UID));
        EXPECT_TRUE(Contains(interfaces, IDerived::UID));
        EXPECT_TRUE(Contains(interfaces, ILifecycle::UID));
        EXPECT_TRUE(Contains(interfaces, IMetadata::UID));
        EXPECT_TRUE(Contains(interfaces, IAttach::UID));
        EXPECT_TRUE(Contains(interfaces, IStaticMetadata::UID));

        auto fac = registry.GetObjectFactory(ClassId::TestDerived);
        ASSERT_TRUE(fac);

        EXPECT_EQ(interfaces, fac->GetClassInterfaces());

        auto sm = fac->GetClassStaticMetadata();
        ASSERT_TRUE(sm);
        ASSERT_TRUE(sm->classInfo);
        EXPECT_EQ(*sm->classInfo, ClassId::TestDerived);
        auto base = GetBaseClassMeta(sm);
        ASSERT_TRUE(base);
        ASSERT_TRUE(base->classInfo);
        EXPECT_EQ(*base->classInfo, ClassId::TestSuper);
        EXPECT_TRUE(FindStaticMetadata(*sm, "DerProp1", MetadataType::PROPERTY));
        EXPECT_TRUE(FindStaticMetadata(*sm, "DerProp2", MetadataType::PROPERTY));
        EXPECT_TRUE(FindStaticMetadata(*sm, "OtherOnChanged", MetadataType::EVENT));
        EXPECT_TRUE(FindStaticMetadata(*sm, "MyFoo", MetadataType::FUNCTION));
    }

    {
        auto interfaces = derived->superClass_->GetInterfaces();
        EXPECT_TRUE(Contains(interfaces, ITestSuper::UID));
        EXPECT_TRUE(Contains(interfaces, IObject::UID));
        EXPECT_TRUE(Contains(interfaces, IDerived::UID));
        EXPECT_TRUE(Contains(interfaces, ILifecycle::UID));
        EXPECT_TRUE(Contains(interfaces, IMetadata::UID));
        EXPECT_TRUE(Contains(interfaces, IAttach::UID));
        EXPECT_TRUE(Contains(interfaces, IStaticMetadata::UID));

        auto fac = registry.GetObjectFactory(ClassId::TestSuper);
        ASSERT_TRUE(fac);

        EXPECT_EQ(interfaces, fac->GetClassInterfaces());

        auto sm = fac->GetClassStaticMetadata();
        ASSERT_TRUE(sm);
        ASSERT_TRUE(sm->classInfo);
        EXPECT_EQ(*sm->classInfo, ClassId::TestSuper);
        auto base = GetBaseClassMeta(sm);
        ASSERT_TRUE(base);
        ASSERT_TRUE(base->classInfo);
        EXPECT_EQ(*base->classInfo, META_NS::ClassId::Object);
        EXPECT_TRUE(FindStaticMetadata(*sm, "Prop1", MetadataType::PROPERTY));
        EXPECT_TRUE(FindStaticMetadata(*sm, "Prop2", MetadataType::PROPERTY));
        EXPECT_TRUE(FindStaticMetadata(*sm, "OnChanged", MetadataType::EVENT));
        EXPECT_TRUE(FindStaticMetadata(*sm, "MyFunc", MetadataType::FUNCTION));
    }

    UnregisterObjectType<TestDerived>();
    UnregisterObjectType<TestSuper>();
}

META_REGISTER_CLASS(TestFail, "abcd6e8b-36d4-44e4-802d-b2d80e50c612", META_NS::ObjectCategoryBits::APPLICATION)

class TestFail final : public META_NS::ObjectFwd {
    META_OBJECT(TestFail, ClassId::TestFail, ObjectFwd)
    using Super::Super;

    bool Build(const IMetadata::Ptr&) override
    {
        return false;
    }
};

/**
 * @tc.name: ConstructBuildReturnFalse
 * @tc.desc: Tests for Construct Build Return False. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, ConstructBuildReturnFalse, testing::ext::TestSize.Level1)
{
    RegisterObjectType<TestFail>();

    auto object = META_NS::GetObjectRegistry().Create(ClassId::TestFail);
    ASSERT_FALSE(object);

    UnregisterObjectType<TestFail>();
}

/**
 * @tc.name: GetInterface
 * @tc.desc: Tests for Get Interface. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, GetInterface, testing::ext::TestSize.Level1)
{
    auto& objectRegistry = GetObjectRegistry();
    auto types = objectRegistry.GetAllTypes(ObjectCategoryBits::ANY);

    ASSERT_FALSE(types.empty());

    for (auto& t : types) {
        auto o = objectRegistry.Create(t->GetClassInfo());
        ASSERT_TRUE(o);
        const auto* co = o.get();

        auto io = o->GetInterface(CORE_NS::IInterface::UID);
        auto cio = co->GetInterface(CORE_NS::IInterface::UID);

        ASSERT_TRUE(io);
        ASSERT_TRUE(cio);

        if (!io->GetInterface(IObject::UID)) {
            CORE_LOG_E("failing uid: %s", t->GetClassInfo().Id().ToString().c_str());
        }

        ASSERT_TRUE(io->GetInterface(IObject::UID));
        ASSERT_TRUE(cio->GetInterface(IObject::UID));
    }
}

/**
 * @tc.name: SameObjectInstanceId
 * @tc.desc: Tests for Same Object Instance Id. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, SameObjectInstanceId, testing::ext::TestSize.Level1)
{
    auto& r = GetObjectRegistry();

    InstanceId id("4aa8aaf4-c42f-4aa5-8482-fa6d8602574a");

    auto obj1 = r.Create(META_NS::ClassId::Object, IObjectRegistry::CreateInfo { id, true });
    ASSERT_TRUE(obj1);

    ASSERT_EQ(r.GetObjectInstanceByInstanceId(id), obj1);
    obj1.reset();

    auto obj2 = r.Create(META_NS::ClassId::Object, IObjectRegistry::CreateInfo { id, true });
    ASSERT_TRUE(obj2);

    // destroy some objects so that the registry "auto" disposal kicks in
    for (size_t i = 0; i != 120; ++i) {
        auto v = r.Create(META_NS::ClassId::Object);
        ASSERT_TRUE(v);
    }

    ASSERT_EQ(r.GetObjectInstanceByInstanceId(id), obj2);
}

/**
 * @tc.name: InvalidFactory
 * @tc.desc: Test invalid class factory
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, InvalidFactory, testing::ext::TestSize.Level1)
{
    auto& r = GetObjectRegistry();
    EXPECT_FALSE(r.RegisterObjectType({}));
    EXPECT_FALSE(r.UnregisterObjectType({}));
}

/**
 * @tc.name: GetAllInstances
 * @tc.desc: Test instance list getters
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, GetInstances, testing::ext::TestSize.Level1)
{
    auto& r = GetObjectRegistry();
    auto object = r.Create(META_NS::ClassId::Object);
    ASSERT_TRUE(object);
    auto all = r.GetAllObjectInstances();
    EXPECT_FALSE(all.empty());
    EXPECT_THAT(all, ::testing::Contains(object));

    all = r.GetAllSingletonObjectInstances();
    EXPECT_TRUE(all.empty());

    auto singleton = r.Create(META_NS::ClassId::FloatInterpolator);
    ASSERT_TRUE(singleton);
    all = r.GetAllSingletonObjectInstances();
    auto singletonInstanceId = interface_cast<IObjectInstance>(singleton)->GetInstanceId();
    EXPECT_FALSE(all.empty());
    EXPECT_THAT(all, ::testing::Contains(singleton));
    static constexpr auto invalidInstanceId = InstanceId { "11111111-1111-1111-1111-111111111111" };

    EXPECT_FALSE(r.GetObjectInstanceByInstanceId({}));
    EXPECT_TRUE(r.GetObjectInstanceByInstanceId(singletonInstanceId));
    EXPECT_FALSE(r.GetObjectInstanceByInstanceId(invalidInstanceId));
}

/**
 * @tc.name: GetImplementedInterface
 * @tc.desc: Test object registry IInterface implementation
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, GetImplementedInterface, testing::ext::TestSize.Level1)
{
    auto& r = GetObjectRegistry();
    r.Ref();
    r.Unref();

    EXPECT_TRUE(r.GetInterface(CORE_NS::IInterface::UID));
    EXPECT_TRUE(r.GetInterface(IObjectRegistry::UID));
    EXPECT_TRUE(r.GetInterface(ITaskQueueRegistry::UID));

    const auto& cr = r;
    EXPECT_TRUE(cr.GetInterface(CORE_NS::IInterface::UID));
    EXPECT_TRUE(cr.GetInterface(IObjectRegistry::UID));
    EXPECT_TRUE(cr.GetInterface(ITaskQueueRegistry::UID));
}

/**
 * @tc.name: HasInterpolator
 * @tc.desc: Test object registry interfaces
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, HasInterpolator, testing::ext::TestSize.Level1)
{
    auto& r = GetObjectRegistry();
    EXPECT_TRUE(r.HasInterpolator(UidFromType<float>()));
    EXPECT_FALSE(r.HasInterpolator(TypeId {}));
}

/**
 * @tc.name: GetGlobalSerializationData
 * @tc.desc: Test global serialization data APIs
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, GetGlobalSerializationData, testing::ext::TestSize.Level1)
{
    auto& r = GetObjectRegistry();
    auto& s = r.GetGlobalSerializationData();

    auto defaults = s.GetDefaultSettings();
    s.SetDefaultSettings(defaults);

    auto object = r.Create(META_NS::ClassId::Object);
    auto instanceId = interface_cast<IObjectInstance>(object)->GetInstanceId();
    ASSERT_TRUE(object);
    s.RegisterGlobalObject({});
    s.RegisterGlobalObject(object);
    EXPECT_EQ(s.GetGlobalObject(instanceId), object);
    s.UnregisterGlobalObject({});
    s.UnregisterGlobalObject(object);
    EXPECT_FALSE(s.GetGlobalObject(instanceId));
}

/**
 * @tc.name: DefaultObjectContext
 * @tc.desc: Test default object context getter
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, DefaultObjectContext, testing::ext::TestSize.Level1)
{
    auto& r = GetObjectRegistry();
    auto ctx = r.GetDefaultObjectContext();
    EXPECT_TRUE(ctx);
    EXPECT_EQ(r.GetDefaultObjectContext(), ctx);
    EXPECT_EQ(r.GetDefaultObjectContext(), ctx);
}

/**
 * @tc.name: DefaultObjectContext
 * @tc.desc: Test default and custom object context
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, ObjectContext, testing::ext::TestSize.Level1)
{
    auto& r = GetObjectRegistry();
    auto defaultCtx = r.GetDefaultObjectContext();

    auto testContext = [](const IObjectContext::Ptr& context, const IObject::Ptr& target = {}) {
        ASSERT_TRUE(context);

        EXPECT_EQ(&context->GetObjectRegistry(), &GetObjectRegistry());

        IMetadata* meta = interface_cast<IMetadata>(context);
        const IMetadata* cmeta = meta;
        EXPECT_FALSE(meta->GetProperty("invalid"));
        EXPECT_FALSE(meta->GetProperties().empty());
        EXPECT_FALSE(cmeta->GetProperty("invalid"));
        EXPECT_FALSE(cmeta->GetProperties().empty());

        auto p = ConstructProperty<int>("added", 0);

        auto proxy = interface_cast<IProxyObject>(context);
        ASSERT_TRUE(proxy);
        EXPECT_EQ(proxy->GetTarget(), target);
        EXPECT_TRUE(proxy->GetOverrides().empty());
        EXPECT_FALSE(proxy->GetOverride(""));
        EXPECT_FALSE(proxy->GetProxyProperty("invalid"));
        EXPECT_TRUE(proxy->SetPropertyTarget(p));
    };

    testContext(defaultCtx);
    auto created = r.Create<IObjectContext>(META_NS::ClassId::ObjectContext);
    testContext(created);

    auto invalidTarget = r.Create(META_NS::ClassId::Object);
    auto target = r.Create(META_NS::ClassId::ObjectContext);
    ASSERT_TRUE(invalidTarget);
    auto proxy = interface_cast<IProxyObject>(created);
    ASSERT_TRUE(proxy);
    EXPECT_FALSE(proxy->SetTarget(invalidTarget));
    testContext(created);
    EXPECT_TRUE(proxy->SetTarget(target));
    testContext(created, target);
    EXPECT_TRUE(proxy->SetTarget({}));
    testContext(created);
}

/**
 * @tc.name: GetUniqueName
 * @tc.desc: Test IObjectUtil::GetUniqueName.
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectRegistryTest, GetUniqueName, testing::ext::TestSize.Level1)
{
    auto& util = GetObjectUtil();
    BASE_NS::vector<BASE_NS::string_view> names = { "child", "child(1)", "child (2)", "child (3)", "child 2)", "(2)",
        "child (2", "xx)" };

    EXPECT_EQ(util.GetUniqueName("child", names), "child (1)");
    EXPECT_EQ(util.GetUniqueName("child1", names), "child1");
    EXPECT_EQ(util.GetUniqueName("child (2)", names), "child (1)");
    EXPECT_EQ(util.GetUniqueName("child (1)", names), "child (1)");
    EXPECT_EQ(util.GetUniqueName("child(1)", names), "child(1) (1)");
    EXPECT_EQ(util.GetUniqueName("(2)", names), "(1)");
    EXPECT_EQ(util.GetUniqueName("(1)", names), "(1)");
    EXPECT_EQ(util.GetUniqueName("child 2)", names), "child 2) (1)");
    EXPECT_EQ(util.GetUniqueName("child (2", names), "child (2 (1)");
    EXPECT_EQ(util.GetUniqueName("child (x)", names), "child (x)");
    EXPECT_EQ(util.GetUniqueName("()", names), "()");
    EXPECT_EQ(util.GetUniqueName("xx)", names), "xx) (1)");
    EXPECT_EQ(util.GetUniqueName("yy)", names), "yy)");

    // Test empty name in list
    BASE_NS::vector<BASE_NS::string_view> names2 = { "", "child", "()" };
    EXPECT_EQ(util.GetUniqueName("", names2), "(1)");
    EXPECT_EQ(util.GetUniqueName("child", names2), "child (1)");
    EXPECT_EQ(util.GetUniqueName("()", names2), "() (1)");

    // Empty list should always return the name as is
    EXPECT_EQ(util.GetUniqueName("child", {}), "child");
    EXPECT_EQ(util.GetUniqueName("", {}), "");
}

} // namespace UTest
META_END_NAMESPACE()
