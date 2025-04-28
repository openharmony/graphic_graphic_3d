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

#include <ComponentTools/base_manager.h>
#include <ComponentTools/base_manager.inl>

#include "entity_owner_component.h"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

SCENE_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class EntityOwnerComponentManager final : public BaseManager<EntityOwnerComponent, IEntityOwnerComponentManager> {
    BEGIN_PROPERTY(EntityOwnerComponent, componentMetaData_)
#include "entity_owner_component.h"
    END_PROPERTY();

public:
    explicit EntityOwnerComponentManager(IEcs& ecs)
        : BaseManager<EntityOwnerComponent, IEntityOwnerComponentManager>(ecs, CORE_NS::GetName<EntityOwnerComponent>())
    {}

    ~EntityOwnerComponentManager() = default;

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
};

IComponentManager* IEntityOwnerComponentManagerInstance(IEcs& ecs)
{
    return new EntityOwnerComponentManager(ecs);
}

void IEntityOwnerComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<EntityOwnerComponentManager*>(instance);
}
SCENE_END_NAMESPACE()