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

#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/intf_text.h>
#include <text_3d/ecs/components/text_component.h>

#include "scene/scene_component_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginTextComponentTest : public ScenePluginComponentTest<TEXT3D_NS::ITextComponentManager> {
public:
};

/**
 * @tc.name: Members
 * @tc.desc: Tests for Members. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginTextComponentTest, Members, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test", ClassId::TextNode).GetResult();
    ASSERT_TRUE(node);
    SetComponent(node, "TextComponent");

    TestEngineProperty<BASE_NS::string>("Text", "some some", nativeComponent.text);
    TestEngineProperty<BASE_NS::string>("FontFamily", "Segoe UI", nativeComponent.fontFamily);
    TestEngineProperty<BASE_NS::string>("FontStyle", "Italic", nativeComponent.fontStyle);
    TestEngineProperty<float>("FontSize", 1.0f, nativeComponent.fontSize);
    TestEngineProperty<float>("Font3DThickness", 1.0f, nativeComponent.font3DThickness);
    TestEngineProperty<FontMethod>("FontMethod", FontMethod::TEXT3D, nativeComponent.fontMethod);
}

} // namespace UTest

SCENE_END_NAMESPACE()
