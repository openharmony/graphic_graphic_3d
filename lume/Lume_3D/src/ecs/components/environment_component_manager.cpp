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

#include <3d/ecs/components/environment_component.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE_BEGIN_NAMESPACE()
/** Extend propertysystem with the enums */
DECLARE_PROPERTY_TYPE(CORE3D_NS::EnvironmentComponent::Background);

// Declare their metadata
BEGIN_ENUM(EnvironmentComponentBackgroundTypeMetaData, CORE3D_NS::EnvironmentComponent::Background)
DECL_ENUM(CORE3D_NS::EnvironmentComponent::Background, NONE, "None")
DECL_ENUM(CORE3D_NS::EnvironmentComponent::Background, IMAGE, "Image")
DECL_ENUM(CORE3D_NS::EnvironmentComponent::Background, CUBEMAP, "Cubemap")
DECL_ENUM(CORE3D_NS::EnvironmentComponent::Background, EQUIRECTANGULAR, "Equirectangular")
END_ENUM(EnvironmentComponentBackgroundTypeMetaData, CORE3D_NS::EnvironmentComponent::Background)
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class EnvironmentComponentManager final : public BaseManager<EnvironmentComponent, IEnvironmentComponentManager> {
    BEGIN_PROPERTY(EnvironmentComponent, ComponentMetadata)
#include <3d/ecs/components/environment_component.h>
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit EnvironmentComponentManager(IEcs& ecs)
        : BaseManager<EnvironmentComponent, IEnvironmentComponentManager>(ecs, CORE_NS::GetName<EnvironmentComponent>())
    {}

    ~EnvironmentComponentManager() = default;

    size_t PropertyCount() const override
    {
        return componentMetaData_.size();
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < componentMetaData_.size()) {
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
    delete static_cast<EnvironmentComponentManager*>(instance);
}

CORE3D_END_NAMESPACE()
