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
#include <scene/interface/intf_scene.h>

#include <core/resources/intf_resource_manager.h>

#include <meta/interface/resource/intf_owned_resource_groups.h>
#include <meta/interface/resource/intf_resource.h>

#include "../ecs_component/resource_component.h"
#include "../serialization/util.h"

SCENE_BEGIN_NAMESPACE()

// Checks only groups in the resource manager
inline BASE_NS::string UniqueGroupName(const CORE_NS::IResourceManager::Ptr& resources, BASE_NS::string_view base)
{
    // try to find group name that is not in use
    std::size_t index {};
    BASE_NS::string group { base };
    while (!resources->GetResourceInfos(group).empty()) {
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
    // try to find group name that is not in use
    std::size_t index {};
    BASE_NS::string group { base };
    while (!resources->GetResourceInfos(group).empty() || !UniqueGroupInEcs(ecs, group)) {
        group = BASE_NS::string(base) + " (" + BASE_NS::to_string(++index) + ")";
    }
    return group;
}
inline CORE_NS::ResourceId UniqueResourceName(
    const CORE_NS::IResourceManager::Ptr& resources, const CORE_NS::ResourceId& id)
{
    // try to find group name that is not in use
    std::size_t index {};
    CORE_NS::ResourceId nid = id;
    while (resources->GetResourceInfo(nid).id.IsValid()) {
        nid.name = id.name + " (" + BASE_NS::to_string(++index) + ")";
    }
    return nid;
}

inline void AddGroup(const IScene::Ptr& scene, BASE_NS::string_view group)
{
    if (auto iScene = scene->GetInternalScene()) {
        if (auto c = iScene->GetContext()) {
            if (auto res = interface_pointer_cast<META_NS::IOwnedResourceGroups>(c->GetResources())) {
                auto bundle = iScene->GetResourceGroups();
                if (!bundle.GetHandle(group)) {
                    auto handle = res->GetGroupHandle(group);
                    bundle.PushGroupHandleToBack(handle);
                    iScene->SetResourceGroups(BASE_NS::move(bundle));
                }
            }
        }
    }
}
inline void ChangePrimaryGroup(const IScene::Ptr& scene, BASE_NS::string_view primary)
{
    if (auto iScene = scene->GetInternalScene()) {
        if (auto c = iScene->GetContext()) {
            if (auto res = interface_pointer_cast<META_NS::IOwnedResourceGroups>(c->GetResources())) {
                auto handle = res->GetGroupHandle(primary);
                auto bundle = iScene->GetResourceGroups();
                bundle.RemoveHandle(primary);
                bundle.PushGroupHandleToFront(handle);
                iScene->SetResourceGroups(BASE_NS::move(bundle));
            }
        }
    }
}
inline void SetPrimaryGroupOnly(const IScene::Ptr& scene, BASE_NS::string_view primary)
{
    if (auto iScene = scene->GetInternalScene()) {
        if (auto c = iScene->GetContext()) {
            if (auto res = interface_pointer_cast<META_NS::IOwnedResourceGroups>(c->GetResources())) {
                auto handle = res->GetGroupHandle(primary);
                auto bundle = iScene->GetResourceGroups();
                bundle = ResourceGroupBundle({ handle });
                iScene->SetResourceGroups(BASE_NS::move(bundle));
            }
        }
    }
}
inline ResourceGroupBundle StringToResourceGroupBundle(
    const IRenderContext::Ptr& c, const BASE_NS::vector<BASE_NS::string>& groups)
{
    ResourceGroupBundle bundle;
    if (auto res = interface_pointer_cast<META_NS::IOwnedResourceGroups>(c->GetResources())) {
        for (auto&& g : groups) {
            bundle.PushGroupHandleToBack(res->GetGroupHandle(g));
        }
    }
    return bundle;
}

inline bool CreateObjectResourceOptions(
    const CORE_NS::IResource::ConstPtr& p, const CORE_NS::ResourceManagerPtr& rm, CORE_NS::IFile& options)
{
    bool res = false;
    if (auto opts = META_NS::GetObjectRegistry().Create<META_NS::IObjectResourceOptions>(
            META_NS::ClassId::ObjectResourceOptions)) {
        auto in = interface_cast<META_NS::IMetadata>(p);
        auto out = interface_cast<META_NS::IMetadata>(opts);
        if (auto i = interface_cast<META_NS::IDerivedFromTemplate>(p)) {
            auto resource = i->GetTemplateId();
            opts->SetBaseResource(resource);
            res = resource.IsValid();
        }
        if (in && out) {
            res |= SerCloneAllToDefaultIfSet(*in, *out);
            if (res) {
                opts->Save(options, rm, nullptr);
            }
        }
    }
    return res;
}

inline void ApplyObjectResourceOptions(const CORE_NS::ResourceId& id, const CORE_NS::IResource::Ptr& res,
    CORE_NS::IFile& options, const CORE_NS::ResourceManagerPtr& rm, const CORE_NS::ResourceContextPtr& context)
{
    if (auto opts = META_NS::GetObjectRegistry().Create<META_NS::IObjectResourceOptions>(
            META_NS::ClassId::ObjectResourceOptions)) {
        opts->Load(options, rm, context);
        if (auto i = interface_cast<META_NS::IDerivedFromTemplate>(res)) {
            auto base = opts->GetBaseResource();
            if (base.IsValid()) {
                auto r = rm->GetResource(base, context);
                if (!r) {
                    CORE_LOG_W("Could not load base resource for %s", id.ToString().c_str());
                }
                if (!i->SetTemplate(r)) {
                    CORE_LOG_W("Failed to apply template for resource %s", id.ToString().c_str());
                }
            }
        }
        auto in = interface_cast<META_NS::IMetadata>(opts);
        auto out = interface_cast<META_NS::IMetadata>(res);
        if (in && out) {
            SerCopy(*in, *out);
        }
    }
}

SCENE_END_NAMESPACE()

#endif