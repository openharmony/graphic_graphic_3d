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

#ifndef SCENE_TEST_SCENE_COMPONENT_TEST_H
#define SCENE_TEST_SCENE_COMPONENT_TEST_H

#include <scene/ext/intf_engine_property_init.h>

#include "scene/scene_test.h"

using BASE_NS::literals::operator""_s;

SCENE_BEGIN_NAMESPACE()
namespace UTest {

/**
 * @brief Wrap TestEngineProperty for complex types by supplying a simple equality check for one member variable.
 */
#define TEST_COMPLEX_PROP(ComplexType, propName, nativeMember, subMemberToTest, nondefaultValue)                       \
    {                                                                                                                  \
        auto complexPropObject = ComplexType {};                                                                       \
        complexPropObject.subMemberToTest = nondefaultValue;                                                           \
        auto complexPropIsEqual = [](const auto& a, const auto& b) { return a.subMemberToTest == b.subMemberToTest; }; \
        using NType = std::remove_reference<decltype(nativeMember)>::type;                                             \
        TestEngineProperty<ComplexType, NType>(propName, complexPropObject, nativeMember, complexPropIsEqual);         \
    }

template<typename ComponentManager>
class ScenePluginComponentTest : public ScenePluginTest {
public:
    using ComponentType = decltype(BASE_NS::declval<ComponentManager>().Get(CORE_NS::Entity {}));

    void SetComponent(INode::Ptr node, BASE_NS::string_view name)
    {
        if (auto att = interface_cast<META_NS::IAttach>(node)) {
            object = att->GetAttachmentContainer(true)->FindByName(name);
        }
        ASSERT_TRUE(object);
    }

    void UpdateComponentMembers()
    {
        UpdateScene();

        auto entity = interface_cast<IEcsObjectAccess>(object)->GetEcsObject()->GetEntity();
        ASSERT_TRUE(CORE_NS::EntityUtil::IsValid(entity));
        auto internal = scene->GetInternalScene();
        auto ecs = internal->GetEcsContext().GetNativeEcs();
        auto manager = CORE_NS::GetManager<ComponentManager>(*ecs);
        nativeComponent = manager->Get(entity);
    }

    template<typename Type>
    META_NS::Property<Type> GetProperty(BASE_NS::string_view name)
    {
        auto m = interface_cast<META_NS::IMetadata>(object);
        EXPECT_TRUE(m);
        auto p = m->GetProperty<Type>(name);
        EXPECT_TRUE(p);
        return p;
    }

    template<typename Type>
    META_NS::ArrayProperty<Type> GetArrayProperty(BASE_NS::string_view name)
    {
        auto m = interface_cast<META_NS::IMetadata>(object);
        EXPECT_TRUE(m);
        auto p = m->GetArrayProperty<Type>(name);
        EXPECT_TRUE(p);
        return p;
    }

    /**
     * @brief Temporarily alter the value of a property and check that the native engine value changes accordingly.
     */
    template<typename Type, typename NType>
    void TestEngineProperty(BASE_NS::string_view name, Type value, const NType& componentMember)
    {
        auto defaultIsEqual = [](const NType& a, const NType& b) { return a == b; };
        TestEngineProperty<Type, NType>(name, value, componentMember, defaultIsEqual);
    }

    /**
     * @brief Use a custom equality check for complex types lacking operator==. See TEST_COMPLEX_PROP macro for usage.
     */
    template<typename Type, typename NType>
    void TestEngineProperty(BASE_NS::string_view name, Type value, const NType& componentMember,
        std::function<bool(const NType&, const NType&)> isEqual)
    {
        auto isEq = [&](const Type& a, const Type& b) { return isEqual(static_cast<NType>(a), static_cast<NType>(b)); };
        auto p = GetProperty<Type>(name);
        ASSERT_TRUE(p);
        auto old = p->GetValue();
        auto nag = "New value of '"_s + name + "' must differ from old for TestEngineProperty to work."_s;
        EXPECT_FALSE(isEq(value, old)) << nag.c_str();

        p->SetValue(value);
        EXPECT_TRUE(isEq(p->GetValue(), value)) << BASE_NS::string(name).c_str();

        UpdateComponentMembers();

        EXPECT_TRUE(isEq(p->GetValue(), value)) << BASE_NS::string(name).c_str();
        CheckNativePropertyValue<NType>(p, static_cast<NType>(value), isEqual);
        EXPECT_TRUE(isEqual(static_cast<NType>(value), componentMember)) << BASE_NS::string(name).c_str();

        p->SetValue(old);

        UpdateComponentMembers();

        EXPECT_TRUE(isEq(p->GetValue(), old)) << BASE_NS::string(name).c_str();
        CheckNativePropertyValue<NType>(p, static_cast<NType>(old), isEqual);
        EXPECT_TRUE(isEqual(static_cast<NType>(old), componentMember)) << BASE_NS::string(name).c_str();
    }

    void TestInvalidComponentPropertyInit(CORE_NS::IInterface* component)
    {
        if (!interface_cast<IComponent>(component)) {
            return;
        }
        auto meta = interface_cast<META_NS::IMetadata>(component);
        auto init = interface_cast<IEnginePropertyInit>(component);
        ASSERT_TRUE(meta);
        ASSERT_TRUE(init);
        auto all = meta->GetProperties();
        EXPECT_FALSE(all.empty());
        for (auto&& p : all) {
            EXPECT_FALSE(init->InitDynamicProperty(p, "invalld_path"));
        }
    }

    META_NS::IObject::Ptr object;
    ComponentType nativeComponent;
};

} // namespace UTest
SCENE_END_NAMESPACE()

#endif
