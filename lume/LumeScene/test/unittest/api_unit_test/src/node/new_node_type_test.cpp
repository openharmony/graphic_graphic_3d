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

#include <scene/ext/node_fwd.h>
#include <scene/ext/scene_property.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>

#include "node/util.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class IMyTest : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMyTest, "e38ce662-c041-4ef9-bea1-582410994194")
public:
    META_PROPERTY(int, Prop)
    META_PROPERTY(BASE_NS::Math::Vec3, AnotherScale)
};

META_REGISTER_CLASS(NewNodeType, "cb075d98-48d2-4f12-ae21-024ddcf3ff14", META_NS::ObjectCategoryBits::NO_CATEGORY)

class NewNodeType : public META_NS::IntroduceInterfaces<NodeFwd, IMyTest> {
    META_OBJECT(NewNodeType, ClassId::NewNodeType, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IMyTest, int, Prop)
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IMyTest, BASE_NS::Math::Vec3, AnotherScale, "TransformComponent.scale")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(int, Prop)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, AnotherScale)

    bool Build(const IMetadata::Ptr& d) override
    {
        return Super::Build(d);
    }

    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override
    {
        CORE_LOG_I("hips: %s", p->GetName().c_str());
        return Super::InitDynamicProperty(p, path);
    }
};

class API_ScenePluginNewNodeTypeTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        META_NS::RegisterObjectType<NewNodeType>();
        ScenePluginTest::SetUp();
    }

    void TearDown() override
    {
        META_NS::UnregisterObjectType<NewNodeType>();
    }

protected:
};

/**
 * @tc.name: Test
 * @tc.desc: Tests for Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNewNodeTypeTest, Test, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto node = scene->CreateNode("//test", ClassId::NewNodeType).GetResult();
    ASSERT_TRUE(node);

    auto nn = interface_cast<IMyTest>(node);
    ASSERT_TRUE(nn);

    nn->Prop()->SetValue(2);
    EXPECT_EQ(nn->Prop()->GetValue(), 2);

    nn->AnotherScale()->SetValue({ 1, 2, 3 });
    UpdateScene();
    EXPECT_EQ(node->Scale()->GetValue(), (BASE_NS::Math::Vec3 { 1, 2, 3 }));
    EXPECT_EQ(node->Scale()->GetValue(), nn->AnotherScale()->GetValue());
}

/**
 * @tc.name: NewNodeTypeMetadata
 * @tc.desc: Tests for New Node Type Metadata. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNewNodeTypeTest, NewNodeTypeMetadata, testing::ext::TestSize.Level1)
{
    TestNodeMetadata(GetSceneManager(), ClassId::NewNodeType);
}

} // namespace UTest

SCENE_END_NAMESPACE()