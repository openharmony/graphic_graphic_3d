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
#include <scene/interface/ecs/resource_component.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/resource/intf_resource_context.h>

#include <core/resources/intf_resource_manager.h>

#include <meta/base/memfile.h>
#include <meta/interface/resource/intf_owned_resource_groups.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/resource/intf_resource_manager_extension.h>

SCENE_BEGIN_NAMESPACE()

// Checks only groups in the resource manager
inline BASE_NS::string UniqueGroupName(const CORE_NS::IResourceManager::Ptr& resources, BASE_NS::string_view base,
    const CORE_NS::ResourceContextPtr& context)
{
    // try to find group name that is not in use
    std::size_t index{};
    BASE_NS::string group{base};
    while (!resources->GetResourceInfos(group, context).empty()) {
        group = BASE_NS::string(base) + " (" + BASE_NS::to_string(++index) + ")";
    }
    return group;
}
inline bool UniqueGroupInEcs(const IEcsContext& context, BASE_NS::string_view group)
{
    const auto& ecs = context.GetNativeEcs();
    const auto resManager = CORE_NS::GetManager<IResourceComponentManager>(*ecs);
    return !resManager || !resManager->HasGroup(group);
}
// Checks groups in the resource manager but also resources in ECS
inline BASE_NS::string UniqueGroupName(const IInternalScene::Ptr& scene, BASE_NS::string_view base)
{
    auto c = scene->GetContext();
    if (!c) {
        return BASE_NS::string(base);
    }
    auto resources = c->GetResources();
    if (!resources) {
        return BASE_NS::string(base);
    }
    auto& ecs = scene->GetEcsContext();
    auto associated = scene->GetResourceGroups();
    // try to find group name that is not in use
    std::size_t index{};
    BASE_NS::string group{base};
    while (associated.GetHandle(group) || !resources->GetResourceInfos(group, scene->GetScene()).empty() ||
           !UniqueGroupInEcs(ecs, group)) {
        group = BASE_NS::string(base) + " (" + BASE_NS::to_string(++index) + ")";
    }
    return group;
}
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

static bool IsEcsResourceInUse(const CORE_NS::Entity& ent, const CORE_NS::Entity& existing)
{
    return CORE_NS::EntityUtil::IsValid(ent) && ent != existing;
}

inline CORE_NS::ResourceIdContext FirstUnusedEcsResourceName(
    const IInternalScene::Ptr& scene, const CORE_NS::ResourceIdContext& id, const CORE_NS::Entity& ent)
{
    const auto resManager = CORE_NS::GetManager<IResourceComponentManager>(*scene->GetEcsContext().GetNativeEcs());
    if (!resManager) {
        return {};
    }

    std::size_t index{};
    auto nid = id;
    while (IsEcsResourceInUse(resManager->GetEntity(nid.id), ent)) {
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
inline ResourceGroupBundle StringToResourceGroupBundle(
    const IScene::Ptr& scene, const BASE_NS::vector<BASE_NS::string>& groups)
{
    ResourceGroupBundle bundle;
    if (auto res = interface_pointer_cast<META_NS::IOwnedResourceGroups>(GetResourceManager(scene))) {
        for (auto&& g : groups) {
            bundle.PushGroupHandleToBack(res->GetGroupHandle(g, scene));
        }
    }
    return bundle;
}

SCENE_END_NAMESPACE()

#endif