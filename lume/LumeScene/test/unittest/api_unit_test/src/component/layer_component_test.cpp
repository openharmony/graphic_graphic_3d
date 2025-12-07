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

#include <scene/ext/component_util.h>
#include <scene/interface/intf_layer.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>

#include <3d/ecs/components/layer_component.h>

#include "scene/scene_component_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginLayerComponentTest : public ScenePluginComponentTest<CORE3D_NS::ILayerComponentManager> {
protected:
    template<class T>
    void TestILayerPropertyGetters(T* layer)
    {
        EXPECT_TRUE(layer->LayerMask());
    }

    void TestILayerFunctions(ILayer* layer)
    {
        ASSERT_TRUE(layer);
        TestILayerPropertyGetters<const ILayer>(layer);
        TestILayerPropertyGetters<ILayer>(layer);
        TestILayerPropertyGetters<const ILayer>(layer);
        TestILayerPropertyGetters<ILayer>(layer);
        TestInvalidComponentPropertyInit(layer);
    }
};

/**
 * @tc.name: Members
 * @tc.desc: Tests for Members. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginLayerComponentTest, Members, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(AddComponent<CORE3D_NS::ILayerComponentManager>(node));
    SetComponent(node, "LayerComponent");

    TestEngineProperty<uint64_t>("LayerMask", 4, nativeComponent.layerMask);
}

/**
 * @tc.name: Functions
 * @tc.desc: Tests for Functions. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginLayerComponentTest, Functions, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(AddComponent<CORE3D_NS::ILayerComponentManager>(node));
    SetComponent(node, "LayerComponent");

    TestILayerFunctions(interface_cast<ILayer>(object));
}

} // namespace UTest

SCENE_END_NAMESPACE()
