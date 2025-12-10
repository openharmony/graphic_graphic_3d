/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef SCENE_API_RESOURCE_UTILS_H
#define SCENE_API_RESOURCE_UTILS_H

#include <scene/api/resource.h>
#include <scene/ext/scene_utils.h>
#include <scene/interface/resource/types.h>

#include <meta/api/animation.h>
#include <meta/base/interface_utils.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief A helper function for retrieving all resources of given type that were imported as part of a node in a scene.
 * @param node The node whose resources to fetch.
 * @param resourceId Type of resource. If {}, return all resources, regardless of type.
 * @return A list of resources that match the input criteria.
 */
inline BASE_NS::vector<CORE_NS::IResource::Ptr> GetImportedResources(
    const INode::ConstPtr& node, const BASE_NS::Uid resourceId)
{
    if (node) {
        if (auto scene = node->GetScene()) {
            auto resources = GetResourceManager(scene);
            auto ext = GetExternalNodeAttachment(node);
            if (resources && ext) {
                BASE_NS::vector<CORE_NS::MatchingResourceId> ids;
                for (auto&& i : resources->GetResourceInfos(ext->GetSubresourceGroup())) {
                    if (i.type == resourceId || resourceId == BASE_NS::Uid {}) {
                        ids.push_back(CORE_NS::MatchingResourceId { i.id });
                    }
                }
                return resources->GetResources(ids, scene);
            }
        }
    }
    return {};
}

/**
 * @brief A helper function for retrieving a list of animations that were imported as part of a node.
 * @param node The node whose animations to fetch.
 * @return A list of animations.
 */
inline BASE_NS::vector<META_NS::Animation> GetImportedAnimations(const INode::ConstPtr& node)
{
    return META_NS::Internal::ArrayCast<META_NS::Animation>(
        GetImportedResources(node, ClassId::AnimationResource.Id().ToUid()));
}

/**
 * @brief A helper function for retrieving a list of materials that were imported as part of a node.
 * @param node The node whose materials to fetch.
 * @return A list of materials.
 */
inline BASE_NS::vector<Material> GetImportedMaterials(const INode::ConstPtr& node)
{
    BASE_NS::vector<Material> materials;
    auto mat = GetImportedResources(node, ClassId::MaterialResource.Id().ToUid());
    auto occ = GetImportedResources(node, ClassId::OcclusionMaterialResource.Id().ToUid());
    materials.reserve(mat.size() + occ.size());
    for (auto&& m : mat) {
        materials.emplace_back(m);
    }
    for (auto&& m : occ) {
        materials.emplace_back(m);
    }
    return materials;
}

SCENE_END_NAMESPACE()

#endif // SCENE_API_RESOURCE_H
