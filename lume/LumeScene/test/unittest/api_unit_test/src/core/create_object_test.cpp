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

#include <scene/interface/intf_environment.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>

#include <3d/ecs/components/material_component.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginCreateObjectTest : public ScenePluginTest {
public:
};

/**
 * @tc.name: EnvironmentMembers
 * @tc.desc: Tests for Environment Members. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginCreateObjectTest, EnvironmentMembers, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    auto node = scene->CreateObject(ClassId::Environment).GetResult();
    ASSERT_TRUE(node);
}

/**
 * @tc.name: RenderConfigurationMembers
 * @tc.desc: Tests for Render Configuration Members. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginCreateObjectTest, RenderConfigurationMembers, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    META_NS::IObject::Ptr object = scene->CreateObject(ClassId::RenderConfiguration).GetResult();
    ASSERT_TRUE(object);

    auto m = interface_cast<META_NS::IMetadata>(object);
    ASSERT_TRUE(m);
    auto p = m->GetProperty<IEnvironment::Ptr>("Environment");
    ASSERT_TRUE(p);
}

} // namespace UTest

SCENE_END_NAMESPACE()
