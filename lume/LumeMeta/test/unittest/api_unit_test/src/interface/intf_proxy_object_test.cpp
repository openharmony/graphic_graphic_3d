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

#include <meta/api/object.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_proxy_object.h>

#include "helpers/testing_objects.h"
#include "helpers/util.h"

META_BEGIN_NAMESPACE()
namespace UTest {

class API_ProxyObjectTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Metadata(object_).AddProperty(ConstructProperty<float>(GetObjectRegistry(), Prop1, 42.f));
        Metadata(object_).AddProperty(ConstructProperty<float>(GetObjectRegistry(), Prop2, 21.f));

        proxy_ = GetObjectRegistry().Create<IProxyObject>(ClassId::ProxyObject);
        ASSERT_TRUE(proxy_);
        ASSERT_TRUE(proxy_->SetTarget(object_));
    }
    void TearDown() override {}

protected:
    static constexpr auto Prop1 = "prop1";
    static constexpr auto Prop2 = "prop2";

    void SetPropValue(const char* propertyName, float value)
    {
        Metadata(object_).GetProperty<float>(propertyName)->SetValue(value);
    }

    Object object_ { CreateInstance(ClassId::Object) };
    IProxyObject::Ptr proxy_;
};

/**
 * @tc.name: Proxy
 * @tc.desc: Tests for Proxy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ProxyObjectTest, Proxy, testing::ext::TestSize.Level1)
{
    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    auto meta = interface_pointer_cast<IMetadata>(proxy_);
    ASSERT_TRUE(meta);

    auto prop1 = meta->GetProperty<float>(Prop1);
    auto prop2 = meta->GetProperty<float>(Prop2);

    ASSERT_TRUE(prop1);
    ASSERT_TRUE(prop2);

    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    SetPropValue(Prop1, 62.f);
    // Proxy should follow source object properties
    EXPECT_EQ(prop1->GetValue(), 62.f);

    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    // Break bind
    prop1->SetValue(2.f);
    SetPropValue(Prop1, 82.f);
    EXPECT_EQ(prop1->GetValue(), 2.f);

    ASSERT_TRUE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);
}

/**
 * @tc.name: PropertyInProxy
 * @tc.desc: Tests for Property In Proxy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ProxyObjectTest, PropertyInProxy, testing::ext::TestSize.Level1)
{
    auto meta = interface_pointer_cast<IMetadata>(proxy_);
    ASSERT_TRUE(meta);

    auto p = CreateProperty<int>("test");
    meta->AddProperty(p);

    auto prop1 = meta->GetProperty<float>(Prop1);
    auto prop2 = meta->GetProperty<float>(Prop2);

    ASSERT_TRUE(prop1);
    ASSERT_TRUE(prop2);

    EXPECT_FALSE(proxy_->GetProxyProperty("test"));
    EXPECT_TRUE(proxy_->GetProxyProperty(Prop1));

    ASSERT_TRUE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    meta->RemoveProperty(p);

    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);
}

/**
 * @tc.name: AddProperty
 * @tc.desc: Tests for Add Property. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ProxyObjectTest, AddProperty, testing::ext::TestSize.Level1)
{
    static constexpr auto prop3Name = "prop3";

    // Add a property to object, it should automatically be added to proxy
    auto prop3 = ConstructProperty<float>(GetObjectRegistry(), prop3Name, 2.f);
    Metadata(object_).AddProperty(prop3);

    auto meta = interface_pointer_cast<IMetadata>(proxy_);
    ASSERT_TRUE(meta);

    auto prop3Proxy = meta->GetProperty<float>(prop3Name);
    ASSERT_TRUE(prop3Proxy);

    prop3->SetValue(42.f);
    EXPECT_EQ(prop3Proxy->GetValue(), 42.f);
}

/**
 * @tc.name: RemoveProperty
 * @tc.desc: Tests for Remove Property. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ProxyObjectTest, RemoveProperty, testing::ext::TestSize.Level1)
{
    // Remove prop2 from our source object
    auto prop2 = Metadata(object_).GetProperty<float>(Prop2);
    ASSERT_TRUE(prop2);
    Metadata(object_).RemoveProperty(prop2);
    ASSERT_FALSE(Metadata(object_).GetProperty<float>(Prop2));

    // Property should be removed from proxy object as well
    auto meta = interface_pointer_cast<IMetadata>(proxy_);
    ASSERT_TRUE(meta);
    ASSERT_FALSE(meta->GetProperty<float>(Prop2));

    auto prop1Proxy = meta->GetProperty<float>(Prop1);
    ASSERT_TRUE(prop1Proxy);
    prop1Proxy->SetValue(100.f); // Break default bind

    // Remove property from object_, it should not be removed from proxy since
    // it has been set to a specific value
    auto prop1 = Metadata(object_).GetProperty<float>(Prop1);
    ASSERT_TRUE(prop1);
    Metadata(object_).RemoveProperty(prop2);
    prop1Proxy = meta->GetProperty<float>(Prop1);
    ASSERT_TRUE(prop1Proxy);
    EXPECT_EQ(prop1Proxy->GetValue(), 100.f);
}

/**
 * @tc.name: Dynamic
 * @tc.desc: Tests for Dynamic. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ProxyObjectTest, Dynamic, testing::ext::TestSize.Level1)
{
    proxy_->Dynamic()->SetValue(false);
    ASSERT_TRUE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);
    proxy_->Dynamic()->ResetValue();
    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy_)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    static constexpr auto prop3Name = "prop3";
    static constexpr auto prop4Name = "prop4";

    // Add a property to object, it should not be added to proxy since Dynamic=false
    auto prop3 = ConstructProperty<float>(GetObjectRegistry(), prop3Name, 2.f);
    Metadata(object_).AddProperty(prop3);

    auto meta = interface_pointer_cast<META_NS::IMetadata>(proxy_);
    ASSERT_TRUE(meta);

    EXPECT_TRUE(meta->GetProperty<float>(prop3Name));

    // Set Dynamic to false, then remove one property (it should be gone from proxy metadata).
    // Then, set dynamic back to true, the property should re-appear
    proxy_->Dynamic()->SetValue(false);
    Metadata(object_).RemoveProperty(prop3);
    EXPECT_TRUE(meta->GetProperty<float>(prop3Name));
    proxy_->Dynamic()->SetValue(true);
    EXPECT_FALSE(meta->GetProperty<float>(prop3Name));
}

/**
 * @tc.name: ProxyOfProxy
 * @tc.desc: Tests for Proxy Of Proxy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ProxyObjectTest, ProxyOfProxy, testing::ext::TestSize.Level1)
{
    // Create a secondary proxy object which proxies the proxy
    // So we have object_ <- proxy_ <- proxy2
    auto proxy2 = GetObjectRegistry().Create<IProxyObject>(ClassId::ProxyObject);
    ASSERT_TRUE(proxy2);
    ASSERT_TRUE(proxy2->SetTarget(interface_pointer_cast<IObject>(proxy_)));

    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy2)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    static constexpr auto prop3Name = "prop3";
    // Add a property to object, it should be added to proxy of proxy
    auto prop3 = ConstructProperty<float>(GetObjectRegistry(), prop3Name, 2.f);
    Metadata(object_).AddProperty(prop3);

    auto meta = interface_pointer_cast<IMetadata>(proxy2);
    ASSERT_TRUE(meta);
    auto prop3Proxy2 = meta->GetProperty<float>(prop3Name);
    ASSERT_TRUE(prop3Proxy2);

    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy2)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    prop3->SetValue(42.f);
    EXPECT_EQ(prop3Proxy2->GetValue(), 42.f);

    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy2)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    // Remove prop3 from object_, should be gone from proxy2 as well
    Metadata(object_).RemoveProperty(prop3);
    EXPECT_FALSE(meta->GetProperty<float>(prop3Name));

    ASSERT_FALSE(interface_cast<IObjectFlags>(proxy2)->GetObjectFlags() & ObjectFlagBits::SERIALIZE);

    auto p1meta = interface_pointer_cast<IMetadata>(proxy_);
    ASSERT_TRUE(p1meta);

    auto prop1 = p1meta->GetProperty<float>(Prop1);
    prop1->SetValue(55);

    auto prop1Proxy2 = meta->GetProperty<float>(Prop1);
    ASSERT_TRUE(prop1Proxy2);
    EXPECT_EQ(prop1Proxy2->GetValue(), 55);
}

/**
 * @tc.name: ProxyMember
 * @tc.desc: Tests for Proxy Member. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ProxyObjectTest, ProxyMember, testing::ext::TestSize.Level1)
{
    Object p2obj(CreateInstance(ClassId::Object));
    Metadata(p2obj).AddProperty(CreateProperty<int>("pint", 2));
    auto proxy2 = GetObjectRegistry().Create<IProxyObject>(ClassId::ProxyObject);
    proxy2->SetTarget(p2obj);
    interface_cast<IMetadata>(proxy_)->AddProperty(
        CreateProperty<IObject::Ptr>("proxy", interface_pointer_cast<IObject>(proxy2)));

    auto p1m = interface_cast<IMetadata>(proxy_);
    ASSERT_TRUE(p1m);
    {
        auto p = p1m->GetProperty<IObject::Ptr>("proxy");
        auto p2m = interface_cast<IMetadata>(p->GetValue());
        ASSERT_TRUE(p2m);
        auto pint = p2m->GetProperty<int>("pint");
        ASSERT_TRUE(pint);
        EXPECT_EQ(pint->GetValue(), 2);
    }

    // Create new proxy hierarchy and set it as target
    Object object(CreateInstance(ClassId::Object));
    auto nproxy = GetObjectRegistry().Create<IProxyObject>(ClassId::ProxyObject);
    ASSERT_TRUE(nproxy);

    Object np2obj(CreateInstance(ClassId::Object));
    Metadata(np2obj).AddProperty(CreateProperty<int>("pint", 7));
    Metadata(np2obj).AddProperty(CreateProperty<int>("pint2", 2));
    auto nproxy2 = GetObjectRegistry().Create<IProxyObject>(ClassId::ProxyObject);
    nproxy2->SetTarget(np2obj);
    interface_cast<IMetadata>(nproxy)->AddProperty(
        CreateProperty<IObject::Ptr>("proxy", interface_pointer_cast<IObject>(nproxy2)));

    ASSERT_TRUE(nproxy->SetTarget(object));
    proxy_->SetTarget(interface_pointer_cast<IObject>(nproxy));

    {
        auto p = p1m->GetProperty<IObject::Ptr>("proxy");
        auto p2m = interface_cast<IMetadata>(p->GetValue());
        ASSERT_TRUE(p2m);
        auto pint = p2m->GetProperty<int>("pint");
        ASSERT_TRUE(pint);
        EXPECT_EQ(pint->GetValue(), 7);

        auto pint2 = p2m->GetProperty<int>("pint2");
        ASSERT_TRUE(pint2);
        EXPECT_EQ(pint2->GetValue(), 2);
    }
}

/**
 * @tc.name: SetPropertyTarget
 * @tc.desc: Tests for Set Property Target. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ProxyObjectTest, SetPropertyTarget, testing::ext::TestSize.Level1)
{
    auto meta = interface_pointer_cast<IMetadata>(proxy_);
    ASSERT_TRUE(meta);

    auto prop1 = meta->GetProperty<float>(Prop1);
    ASSERT_TRUE(prop1);

    auto ep = ConstructProperty<float>(GetObjectRegistry(), Prop1, 99.f);

    EXPECT_EQ(prop1->GetValue(), 42.f);
    EXPECT_FALSE(proxy_->GetOverride(Prop1));

    EXPECT_EQ(proxy_->SetPropertyTarget(ep), prop1.GetProperty());
    EXPECT_EQ(prop1->GetValue(), 99.f);
    EXPECT_FALSE(proxy_->GetOverride(Prop1));

    prop1->SetValue(2.f);
    EXPECT_EQ(prop1->GetValue(), 2.f);
    EXPECT_TRUE(proxy_->GetOverride(Prop1));

    prop1->ResetValue();
    EXPECT_EQ(prop1->GetValue(), 99.f);

    meta->RemoveProperty(prop1);

    prop1 = meta->GetProperty<float>(Prop1);
    ASSERT_TRUE(prop1);
    EXPECT_EQ(prop1->GetValue(), 42.f);
    EXPECT_FALSE(proxy_->GetOverride(Prop1));
}

/**
 * @tc.name: InternalBindingIsScheduledForCreatingBeforeGettinProxyProperty
 * @tc.desc: Tests for Internal Binding Is Scheduled For Creating Before Gettin Proxy Property. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(
    API_ProxyObjectTest, InternalBindingIsScheduledForCreatingBeforeGettinProxyProperty, testing::ext::TestSize.Level1)
{
    proxy_->AddInternalProxy(Prop2, Prop1);

    auto objectProp1 = Metadata(object_).GetProperty<float>(Prop1);
    auto objectProp2 = Metadata(object_).GetProperty<float>(Prop2);
    auto proxyProperty = interface_cast<IMetadata>(proxy_)->GetProperty<float>(Prop1);

    auto internalProxyProp2 = interface_cast<IMetadata>(proxy_)->GetProperty<float>(Prop2);

    objectProp1->SetValue(2000.0F);
    objectProp2->SetValue(3000.0F);

    // proxy::prop2 is internal bind to proxy::prop1, proxy::prop1 is bind to object::prop1,
    // so setting object::prop2 should not be considered in proxy
    EXPECT_THAT(proxyProperty->GetValue(), testing::FloatEq(2000.0F));
    EXPECT_THAT(internalProxyProp2->GetValue(), testing::FloatEq(2000.0F));

    proxyProperty->SetValue(1000.0F);
    // proxy::prop2 is internal bind to proxy::prop1, so we should have changed value
    EXPECT_THAT(internalProxyProp2->GetValue(), testing::FloatEq(1000.0F));
}

/**
 * @tc.name: InternalBindingRemovesBindToTargetAndBindToInternalProperty
 * @tc.desc: Tests for Internal Binding Removes Bind To Target And Bind To Internal Property. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(
    API_ProxyObjectTest, InternalBindingRemovesBindToTargetAndBindToInternalProperty, testing::ext::TestSize.Level1)
{
    auto objectProp1 = Metadata(object_).GetProperty<float>(Prop1);
    auto objectProp2 = Metadata(object_).GetProperty<float>(Prop2);
    auto proxyProperty = interface_cast<IMetadata>(proxy_)->GetProperty<float>(Prop1);

    auto internalProxyProp2 = interface_cast<IMetadata>(proxy_)->GetProperty<float>(Prop2);

    proxy_->AddInternalProxy(Prop2, Prop1);

    objectProp1->SetValue(2000.0F);
    objectProp2->SetValue(3000.0F);
    // proxy::prop2 is internal bind to proxy::prop1, proxy::prop1 is bind to object::prop1,
    // so setting object::prop2 should not be considered in proxy
    EXPECT_THAT(internalProxyProp2->GetValue(), testing::FloatEq(2000.0F));

    proxyProperty->SetValue(1000.0F);
    // proxy::prop2 is internal bind to proxy::prop1, so we should have changed value
    EXPECT_THAT(internalProxyProp2->GetValue(), testing::FloatEq(1000.0F));
}

/**
 * @tc.name: InternalBindWorksAfterChangingProxyTarget
 * @tc.desc: Tests for Internal Bind Works After Changing Proxy Target. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ProxyObjectTest, InternalBindWorksAfterChangingProxyTarget, testing::ext::TestSize.Level1)
{
    proxy_->AddInternalProxy(Prop2, Prop1);

    auto proxyProperty = interface_cast<IMetadata>(proxy_)->GetProperty<float>(Prop1);
    auto internalProxyProp2 = interface_cast<IMetadata>(proxy_)->GetProperty<float>(Prop2);

    auto anotherObject = Object(CreateInstance(ClassId::Object));
    Metadata(anotherObject).AddProperty(ConstructProperty<float>(GetObjectRegistry(), Prop1, 100.F));
    Metadata(anotherObject).AddProperty(ConstructProperty<float>(GetObjectRegistry(), Prop2, 200.F));

    proxy_->SetTarget(anotherObject);

    EXPECT_THAT(internalProxyProp2->GetValue(), testing::FloatEq(100.0F));

    proxyProperty->SetValue(300.0F);

    EXPECT_THAT(internalProxyProp2->GetValue(), testing::FloatEq(300.0F));
}

/**
 * @tc.name: InternalBindIsRemovedWhenNewTargetDoesNotHaveSourceProperty
 * @tc.desc: Tests for Internal Bind Is Removed When New Target Does Not Have Source Property. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(
    API_ProxyObjectTest, InternalBindIsRemovedWhenNewTargetDoesNotHaveSourceProperty, testing::ext::TestSize.Level1)
{
    proxy_->AddInternalProxy(Prop2, Prop1);

    auto internalProxyProp2 = interface_cast<IMetadata>(proxy_)->GetProperty<float>(Prop2);

    auto anotherObject = Object(CreateInstance(ClassId::Object));
    Metadata(anotherObject).AddProperty(ConstructProperty<float>(GetObjectRegistry(), Prop2, 200.F));

    auto objectProp2 = Metadata(anotherObject).GetProperty<float>(Prop2);

    proxy_->SetTarget(anotherObject);
    EXPECT_THAT(internalProxyProp2->GetValue(), testing::FloatEq(200.0F));

    objectProp2->SetValue(800.0F);
    EXPECT_THAT(internalProxyProp2->GetValue(), testing::FloatEq(800.0F));
}

} // namespace UTest
META_END_NAMESPACE()
