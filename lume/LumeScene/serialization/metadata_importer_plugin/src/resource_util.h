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

#ifndef SCENE_SRC_RESOURCE_UTIL_H
#define SCENE_SRC_RESOURCE_UTIL_H

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/scene_utils.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/resource/intf_resource_context.h>

#include <core/resources/intf_resource_manager.h>

#include <meta/base/memfile.h>
#include <meta/interface/resource/intf_owned_resource_groups.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/resource/intf_resource_manager_extension.h>

#include "serialization_util.h"

SCENE_BEGIN_NAMESPACE()

constexpr uint64_t IMPORTED_FROM_TEMPLATE_BIT = 128;

inline CORE_NS::ResourceIdContext UniqueResourceName(
    const CORE_NS::IResourceManager::Ptr& resources, const CORE_NS::ResourceIdContext& id)
{
    std::size_t index{};
    auto nid = id;
    while (resources->GetResourceInfo(nid).id.IsValid()) {
        nid.id.name = id.id.name + " (" + BASE_NS::to_string(++index) + ")";
    }
    return nid;
}

inline void SetPrimaryGroupOnly(const IScene::Ptr& scene, BASE_NS::string_view primary)
{
    if (auto iScene = scene->GetInternalScene()) {
        if (auto c = iScene->GetContext()) {
            if (auto res = interface_pointer_cast<META_NS::IOwnedResourceGroups>(c->GetResources())) {
                auto handle = res->GetGroupHandle(primary, scene);
                auto bundle = iScene->GetResourceGroups();
                bundle = ResourceGroupBundle({handle});
                iScene->SetResourceGroups(BASE_NS::move(bundle));
            }
        }
    }
}

ResourceGroupBundle StringToResourceGroupBundle(
    const IScene::Ptr& scene, const BASE_NS::vector<BASE_NS::string>& groups);

bool CreateObjectResourceOptions(const CORE_NS::IResource::ConstPtr& p, const CORE_NS::ResourceManagerPtr& rm,
    const CORE_NS::ResourceContextPtr& context, CORE_NS::IResourceOptions& options);

void ApplyObjectResourceOptions(const CORE_NS::ResourceId& id, const CORE_NS::IResource::Ptr& res,
    CORE_NS::IResourceOptions& options, const CORE_NS::ResourceManagerPtr& rm,
    const CORE_NS::ResourceContextPtr& context);

BASE_NS::vector<CORE_NS::ResourceId> GetAlreadyExistingResources(
    const BASE_NS::array_view<const CORE_NS::ResourceInfo>& infos, CORE_NS::IResourceManager::Ptr& dest,
    const CORE_NS::ResourceContextPtr& context);

inline void CopyResourceInfos(const BASE_NS::array_view<const CORE_NS::ResourceInfo>& infos,
    META_NS::IResourceManagerExtension& destExt, const CORE_NS::ResourceContextPtr& context,
    BASE_NS::vector<CORE_NS::ResourceId>& out)
{
    for (auto&& i : infos) {
        META_NS::ResourceData d{i};
        d.options = i.options->Clone();
        auto destI = destExt.GetResources({i.id}, context);
        if (!destI.empty()) {
            out.push_back(destI.front().id);
            d.object = destI.front().object;
        }
        destExt.AddResources({d}, context);
    }
}

inline BASE_NS::vector<CORE_NS::ResourceId> CopyResourceInfos(
    const BASE_NS::array_view<const CORE_NS::ResourceInfo>& infos, CORE_NS::IResourceManager::Ptr& dest,
    const CORE_NS::ResourceContextPtr& context)
{
    BASE_NS::vector<CORE_NS::ResourceId> ret;
    if (auto destExt = interface_cast<META_NS::IResourceManagerExtension>(dest)) {
        CopyResourceInfos(infos, *destExt, context, ret);
    }
    return ret;
}

inline BASE_NS::vector<CORE_NS::ResourceId> CopyResourceInfos(
    const BASE_NS::shared_ptr<const CORE_NS::IResourceManager>& source, CORE_NS::IResourceManager::Ptr& dest,
    const BASE_NS::array_view<const CORE_NS::MatchingResourceId>& selection, const CORE_NS::ResourceContextPtr& context)
{
    if (source && dest) {
        return CopyResourceInfos(source->GetResourceInfos(selection, context), dest, context);
    }
    return {};
}

inline BASE_NS::vector<CORE_NS::ResourceId> CopyResourceInfos(
    const BASE_NS::shared_ptr<const CORE_NS::IResourceManager>& source, CORE_NS::IResourceManager::Ptr& dest,
    const CORE_NS::ResourceContextPtr& context)
{
    return CopyResourceInfos(source, dest, {CORE_NS::MatchingResourceId()}, context);
}

SCENE_END_NAMESPACE()

#endif