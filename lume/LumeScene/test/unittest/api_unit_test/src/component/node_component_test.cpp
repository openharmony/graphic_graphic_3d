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

#include <scene/interface/intf_node.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>

#include <3d/ecs/components/node_component.h>

#include "scene/scene_component_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginNodeComponentTest : public ScenePluginComponentTest<CORE3D_NS::INodeComponentManager> {
protected:
    template<class T>
    void TestINodePropertyGetters(T* node)
    {
        EXPECT_TRUE(node->Enabled());
    }

    void TestINodeFunctions(INode* node)
    {
        ASSERT_TRUE(node);
        TestINodePropertyGetters<const INode>(node);
        TestINodePropertyGetters<INode>(node);
        TestINodePropertyGetters<const INode>(node);
        TestINodePropertyGetters<INode>(node);
        TestInvalidComponentPropertyInit(node);
    }
};

/**
 * @tc.name: Members
 * @tc.desc: Tests for Members. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeComponentTest, Members, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    SetComponent(node, "NodeComponent");

    TestEngineProperty<bool>("Enabled", false, nativeComponent.enabled);
}

/**
 * @tc.name: Functions
 * @tc.desc: Tests for Functions. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeComponentTest, Functions, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    SetComponent(node, "NodeComponent");

    TestINodeFunctions(interface_cast<INode>(node));
}

} // namespace UTest

SCENE_END_NAMESPACE()
