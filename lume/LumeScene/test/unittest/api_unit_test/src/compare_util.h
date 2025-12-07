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

#ifndef SCENE_TEST_COMPARE_UTIL
#define SCENE_TEST_COMPARE_UTIL

#include <scene/interface/intf_node.h>

#include <base/containers/unordered_map.h>
#include <core/resources/intf_resource_manager.h>

#include <gtest/gtest.h>

SCENE_BEGIN_NAMESPACE()
namespace UTest {

inline void CompareTrees(INode::Ptr left, INode::Ptr right, bool ignoreRootName = false)
{
    if (!ignoreRootName) {
        EXPECT_EQ(META_NS::GetName(left), META_NS::GetName(right));
    }
    auto lChildren = left->GetChildren().GetResult();
    auto rChildren = right->GetChildren().GetResult();
    ASSERT_EQ(lChildren.size(), rChildren.size());
    for (size_t i = 0; i != lChildren.size(); ++i) {
        CompareTrees(lChildren[i], rChildren[i], false);
    }
}

inline void ExpectResources(
    CORE_NS::IResourceManager::Ptr res, const BASE_NS::vector<CORE_NS::ResourceInfo>& list, bool exact = true)
{
    auto infos = res->GetResourceInfos();
    if (exact) {
        EXPECT_EQ(infos.size(), list.size());
    }

    BASE_NS::unordered_map<CORE_NS::ResourceId, CORE_NS::ResourceInfo> map;
    for (auto&& v : infos) {
        map[v.id] = v;
    }

    for (auto&& v : list) {
        auto it = map.find(v.id);
        bool hasResource = it != map.end();
        EXPECT_TRUE(hasResource) << "missing " << v.id.ToString().c_str();
        if (hasResource) {
            EXPECT_EQ(v.path, it->second.path);
            EXPECT_EQ(v.type, it->second.type);
            EXPECT_EQ(v.optionData.size(), it->second.optionData.size());
        }
    }
}

inline void ExpectAtleastResources(
    CORE_NS::IResourceManager::Ptr res, const BASE_NS::vector<CORE_NS::ResourceInfo>& list)
{
    return ExpectResources(res, list, false);
}

inline auto ChangeGroup(BASE_NS::vector<CORE_NS::ResourceInfo> list, BASE_NS::string_view group)
{
    for (auto&& v : list) {
        v.id.group = group;
    }
    return list;
}

inline auto AddToResourceName(BASE_NS::vector<CORE_NS::ResourceInfo> list, BASE_NS::string_view add)
{
    for (auto&& v : list) {
        v.id.name += add;
    }
    return list;
}

} // namespace UTest
SCENE_END_NAMESPACE()

#endif // SCENE_TEST_UTIL
