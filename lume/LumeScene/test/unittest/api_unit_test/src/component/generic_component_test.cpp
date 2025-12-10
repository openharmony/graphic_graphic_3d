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

#include <scene/ext/intf_component.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/interface/intf_transform.h>

#include <3d/ecs/components/transform_component.h>

#include <meta/interface/intf_object.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginGenericComponentTest : public ScenePluginTest {
public:
    META_NS::IProperty::Ptr GetProperty(IGenericComponent::Ptr component, BASE_NS::string propName) const
    {
        const auto ecsObj = interface_cast<IEcsObjectAccess>(component)->GetEcsObject();
        const auto evManager = ecsObj->GetEngineValueManager();
        return META_NS::PropertyFromEngineValue(propName, evManager->GetEngineValue(propName));
    }
};

/**
 * @tc.name: ReadFromUnregisteredComponent
 * @tc.desc: Tests for Read From Unregistered Component. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginGenericComponentTest, ReadFromUnregisteredComponent, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    scene->GetInternalScene()->UnregisterComponent(CORE3D_NS::ITransformComponentManager::UID);
    const auto node = scene->CreateNode("//test").GetResult();

    const auto att = interface_cast<META_NS::IAttach>(node);
    ASSERT_TRUE(att);
    EXPECT_TRUE(att->GetAttachments<ITransform>().empty());
    const auto component = att->GetAttachmentContainer()->FindByName<IGenericComponent>("TransformComponent");
    ASSERT_TRUE(component);

    EXPECT_TRUE(interface_cast<IComponent>(component)->PopulateAllProperties());
    const auto prop = GetProperty(component, "TransformComponent.scale");
    ASSERT_TRUE(prop);

    const auto magicScale = BASE_NS::Math::Vec3 { 1.2345f, 2.3456f, 3.4567f };
    auto scale = BASE_NS::Math::Vec3 { magicScale };
    prop->GetValue().GetValue(scale);
    EXPECT_NE(scale, magicScale);
}

} // namespace UTest

SCENE_END_NAMESPACE()
