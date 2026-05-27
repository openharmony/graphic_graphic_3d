/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <ComponentTools/base_manager.h>
#include <ComponentTools/base_manager.inl>
#include <scene/interface/ecs/resource_component.h>

#include <core/ecs/intf_ecs.h>

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>
#include <core/resources/intf_resource.h>

// move to core
CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(CORE_NS::ResourceId);
DATA_TYPE_METADATA(CORE_NS::ResourceId, MEMBER_PROPERTY(name, "Name", 0), MEMBER_PROPERTY(group, "Group", 0))
CORE_END_NAMESPACE()

#include <meta/interface/resource/intf_resource.h>

SCENE_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class ResourcerComponentManager final : public BaseManager<ResourceComponent, IResourceComponentManager> {
    BEGIN_PROPERTY(ResourceComponent, componentMetaData_)
#include <scene/interface/ecs/resource_component.h>
    END_PROPERTY();

public:
    explicit ResourcerComponentManager(IEcs& ecs)
        : BaseManager<ResourceComponent, IResourceComponentManager>(ecs, CORE_NS::GetName<ResourceComponent>())
    {}

    ~ResourcerComponentManager() = default;

    size_t PropertyCount() const override
    {
        return BASE_NS::countof(componentMetaData_);
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < BASE_NS::countof(componentMetaData_)) {
            return &componentMetaData_[index];
        }
        return nullptr;
    }

    array_view<const Property> MetaData() const override
    {
        return componentMetaData_;
    }

    CORE_NS::Entity GetEntity(const CORE_NS::ResourceId& rid) const override
    {
        if (const auto pos = std::find_if(components_.begin(),
                components_.end(),
                [&](const BaseComponentHandle& component) {
                    return ecs_.GetEntityManager().IsAlive(component.entity_) &&
                           component.data_.resourceId.name == rid.name && component.data_.resourceId.group == rid.group;
                });
            pos != components_.end()) {
            return pos->entity_;
        }
        return {};
    }

    BASE_NS::vector<CORE_NS::Entity> GetEntities(
        const BASE_NS::array_view<const CORE_NS::MatchingResourceId> list) const override
    {
        BASE_NS::vector<CORE_NS::Entity> res;
        for (auto&& c : components_) {
            if (META_NS::IsResourceMatch(list, c.data_.resourceId)) {
                if (ecs_.GetEntityManager().IsAlive(c.entity_)) {
                    res.push_back(c.entity_);
                }
            }
        }
        return res;
    }

    BASE_NS::vector<CORE_NS::Entity> GetEntities(
        const BASE_NS::array_view<const CORE_NS::ResourceId> arr) const override
    {
        BASE_NS::vector<CORE_NS::Entity> res;
        for (auto&& id : arr) {
            auto ent = GetEntity(id);
            if (ecs_.GetEntityManager().IsAlive(ent)) {
                res.push_back(ent);
            }
        }
        return res;
    }

    bool HasGroup(BASE_NS::string_view group) const override
    {
        const auto pos =
            std::find_if(components_.begin(), components_.end(), [&](const BaseComponentHandle& component) {
                return ecs_.GetEntityManager().IsAlive(component.entity_) && component.data_.resourceId.group == group;
            });
        return pos != components_.end();
    }
};

IComponentManager* IResourceComponentManagerInstance(IEcs& ecs)
{
    return new ResourcerComponentManager(ecs);
}

void IResourceComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<ResourcerComponentManager*>(instance);
}
SCENE_END_NAMESPACE()
