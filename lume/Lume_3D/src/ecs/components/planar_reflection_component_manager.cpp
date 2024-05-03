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

#include <3d/ecs/components/planar_reflection_component.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"
#include "ecs/components/layer_flag_bits_metadata.h"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE_BEGIN_NAMESPACE()
using CORE3D_NS::PlanarReflectionComponent;

// Extend propertysystem with the enums
BEGIN_ENUM(PlanarReflectionFlagBitsMetaData, PlanarReflectionComponent::FlagBits)
DECL_ENUM(PlanarReflectionComponent::FlagBits, ACTIVE_RENDER_BIT, "Active Render")
DECL_ENUM(PlanarReflectionComponent::FlagBits, MSAA_BIT, "MSAA")
END_ENUM(PlanarReflectionFlagBitsMetaData, PlanarReflectionComponent::FlagBits)
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;
using CORE_NS::PropertyFlags;

class PlanarReflectionComponentManager final
    : public BaseManager<PlanarReflectionComponent, IPlanarReflectionComponentManager> {
    BEGIN_PROPERTY(PlanarReflectionComponent, ComponentMetadata)
#include <3d/ecs/components/planar_reflection_component.h>
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit PlanarReflectionComponentManager(IEcs& ecs)
        : BaseManager<PlanarReflectionComponent, IPlanarReflectionComponentManager>(
              ecs, CORE_NS::GetName<PlanarReflectionComponent>())
    {}

    // ECS uninitialize has destroyed (Destroy()) all the components within the manager
    ~PlanarReflectionComponentManager() = default;

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

IComponentManager* IPlanarReflectionComponentManagerInstance(IEcs& ecs)
{
    return new PlanarReflectionComponentManager(ecs);
}

void IPlanarReflectionComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<PlanarReflectionComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
