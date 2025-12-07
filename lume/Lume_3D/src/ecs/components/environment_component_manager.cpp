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

#include <3d/ecs/components/environment_component.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

CORE_BEGIN_NAMESPACE()
using CORE3D_NS::EnvironmentComponent;
/** Extend propertysystem with the enums */
DECLARE_PROPERTY_TYPE(EnvironmentComponent::Background);

// Declare their metadata
ENUM_TYPE_METADATA(EnvironmentComponent::Background, ENUM_VALUE(NONE, "None"), ENUM_VALUE(IMAGE, "Image"),
    ENUM_VALUE(CUBEMAP, "Cubemap"), ENUM_VALUE(EQUIRECTANGULAR, "Equirectangular"), ENUM_VALUE(SKY, "Sky"))
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class EnvironmentComponentManager final : public BaseManager<EnvironmentComponent, IEnvironmentComponentManager> {
    BEGIN_PROPERTY(EnvironmentComponent, componentMetaData_)
#include <3d/ecs/components/environment_component.h>
    END_PROPERTY();

public:
    explicit EnvironmentComponentManager(IEcs& ecs)
        : BaseManager<EnvironmentComponent, IEnvironmentComponentManager>(
              ecs, CORE_NS::GetName<EnvironmentComponent>(), 2U)
    {}

    ~EnvironmentComponentManager() = default;

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

IComponentManager* IEnvironmentComponentManagerInstance(IEcs& ecs)
{
    return new EnvironmentComponentManager(ecs);
}

void IEnvironmentComponentManagerDestroy(IComponentManager* instance)
{
    static_cast<EnvironmentComponentManager*>(instance)->~EnvironmentComponentManager();
    ::operator delete(instance);
}

CORE3D_END_NAMESPACE()
