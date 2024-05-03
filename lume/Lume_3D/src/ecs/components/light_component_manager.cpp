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

#include <3d/ecs/components/light_component.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"
#include "ecs/components/layer_flag_bits_metadata.h"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE_BEGIN_NAMESPACE()
/** Extend propertysystem with the enums */
DECLARE_PROPERTY_TYPE(CORE3D_NS::LightComponent::Type);

// Declare their metadata
BEGIN_ENUM(LightTypeMetaData, CORE3D_NS::LightComponent::Type)
DECL_ENUM(CORE3D_NS::LightComponent::Type, DIRECTIONAL, "Directional")
DECL_ENUM(CORE3D_NS::LightComponent::Type, POINT, "Point")
DECL_ENUM(CORE3D_NS::LightComponent::Type, SPOT, "Spot")
END_ENUM(LightTypeMetaData, CORE3D_NS::LightComponent::Type)
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;
using CORE_NS::PropertyFlags;

class LightComponentManager final : public BaseManager<LightComponent, ILightComponentManager> {
    BEGIN_PROPERTY(LightComponent, ComponentMetadata)
#include <3d/ecs/components/light_component.h>
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit LightComponentManager(IEcs& ecs)
        : BaseManager<LightComponent, ILightComponentManager>(ecs, CORE_NS::GetName<LightComponent>())
    {}

    ~LightComponentManager() = default;

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

IComponentManager* ILightComponentManagerInstance(IEcs& ecs)
{
    return new LightComponentManager(ecs);
}

void ILightComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<LightComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
