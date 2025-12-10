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

#include <3d/ecs/components/transform_component.h>

#include "scene/scene_component_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginTransformComponentTest : public ScenePluginComponentTest<CORE3D_NS::ITransformComponentManager> {
public:
};

/**
 * @tc.name: Members
 * @tc.desc: Tests for Members. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginTransformComponentTest, Members, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    SetComponent(node, "TransformComponent");

    TestEngineProperty<BASE_NS::Math::Vec3>("Position", { 1, 2, 3 }, nativeComponent.position);
    TestEngineProperty<BASE_NS::Math::Vec3>("Scale", { 4, 5, 6 }, nativeComponent.scale);
    TestEngineProperty<BASE_NS::Math::Quat>("Rotation", { 0.5, 0.6, 0.7, 0.8 }, nativeComponent.rotation);
}

} // namespace UTest

SCENE_END_NAMESPACE()