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

#ifndef SCENE_TEST_NODE_UTIL
#define SCENE_TEST_NODE_UTIL

#include <scene/interface/intf_layer.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>

#include <gmock/gmock-matchers.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

inline void TestNodeMetadata(const ISceneManager::Ptr& manager, const META_NS::ClassInfo& type)
{
    using namespace testing;
    auto scene = manager->CreateScene().GetResult();
    ASSERT_TRUE(scene);
    auto node = scene->CreateNode("//test", type).GetResult();
    auto obj = interface_cast<META_NS::IMetadata>(node);
    ASSERT_TRUE(obj);

    META_NS::MetadataInfo nameMetaInfo { META_NS::MetadataType::PROPERTY, "Name", META_NS::INamed::UID, false,
        META_NS::GetTypeId<BASE_NS::string>() };
    META_NS::MetadataInfo positionMetaInfo { META_NS::MetadataType::PROPERTY, "Position", ITransform::UID, false,
        META_NS::GetTypeId<BASE_NS::Math::Vec3>() };
    META_NS::MetadataInfo scaleMetaInfo { META_NS::MetadataType::PROPERTY, "Scale", ITransform::UID, false,
        META_NS::GetTypeId<BASE_NS::Math::Vec3>() };
    META_NS::MetadataInfo rotationMetaInfo { META_NS::MetadataType::PROPERTY, "Rotation", ITransform::UID, false,
        META_NS::GetTypeId<BASE_NS::Math::Quat>() };
    META_NS::MetadataInfo enabledMetaInfo { META_NS::MetadataType::PROPERTY, "Enabled", INode::UID, false,
        META_NS::GetTypeId<bool>() };
    META_NS::MetadataInfo layerMaskInfo { META_NS::MetadataType::PROPERTY, "LayerMask", ILayer::UID, false,
        META_NS::GetTypeId<uint64_t>() };
    META_NS::MetadataInfo onChangedMetaInfo { META_NS::MetadataType::EVENT, "OnContainerChanged",
        META_NS::IContainer::UID };

    {
        auto vec = obj->GetAllMetadatas(META_NS::MetadataType::PROPERTY);
        ASSERT_GE(vec.size(), 6);
        EXPECT_THAT(vec, Contains(nameMetaInfo).Times(1));
        EXPECT_THAT(vec, Contains(positionMetaInfo).Times(1));
        EXPECT_THAT(vec, Contains(scaleMetaInfo).Times(1));
        EXPECT_THAT(vec, Contains(rotationMetaInfo).Times(1));
        EXPECT_THAT(vec, Contains(enabledMetaInfo).Times(1));
        EXPECT_THAT(vec, Contains(layerMaskInfo).Times(1));
    }
    {
        auto vec = obj->GetAllMetadatas(META_NS::MetadataType::EVENT);
        ASSERT_GE(vec.size(), 1);
        EXPECT_THAT(vec, Contains(onChangedMetaInfo).Times(1));
    }
    {
        auto m = obj->GetMetadata(META_NS::MetadataType::PROPERTY, "Position");
        EXPECT_EQ(m, positionMetaInfo);
        auto p = obj->GetProperty("Position");
        ASSERT_TRUE(p);
        EXPECT_EQ(node->Position().GetProperty(), p);
    }
    {
        auto m = obj->GetMetadata(META_NS::MetadataType::PROPERTY, "Enabled");
        EXPECT_EQ(m, enabledMetaInfo);
        auto p = obj->GetProperty("Enabled");
        ASSERT_TRUE(p);
        EXPECT_EQ(node->Enabled().GetProperty(), p);
    }
    {
        auto m = obj->GetMetadata(META_NS::MetadataType::EVENT, "OnContainerChanged");
        EXPECT_EQ(m, onChangedMetaInfo);
        auto p = obj->GetEvent("OnContainerChanged");
        ASSERT_TRUE(p);
    }
}

} // namespace UTest
SCENE_END_NAMESPACE()

#endif // SCENE_TEST_UTIL
