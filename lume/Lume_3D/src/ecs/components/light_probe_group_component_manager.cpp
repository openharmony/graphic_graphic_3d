/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <3d/ecs/components/light_probe_group_component.h>
#include <base/containers/array_view.h>
#include <base/containers/type_traits.h>
#include <core/property/property_types.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

CORE_BEGIN_NAMESPACE()
using CORE3D_NS::LightProbeGroupComponent;
DECLARE_PROPERTY_TYPE(LightProbeGroupComponent::LightProbe);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<LightProbeGroupComponent::LightProbe>);
DATA_TYPE_METADATA(LightProbeGroupComponent::LightProbe, MEMBER_PROPERTY(shCoefficients, "ShCoefficients", 0),
    MEMBER_PROPERTY(position, "Position", 0), MEMBER_PROPERTY(bentNormal, "BentNormal", 0),
    MEMBER_PROPERTY(ao, "AmbientOcclusion", 0))

CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class LightProbeGroupComponentManager final
    : public BaseManager<LightProbeGroupComponent, ILightProbeGroupComponentManager> {
    BEGIN_PROPERTY(LightProbeGroupComponent, properties_)
#include <3d/ecs/components/light_probe_group_component.h>
    END_PROPERTY();

public:
    explicit LightProbeGroupComponentManager(IEcs& ecs)
        : BaseManager<LightProbeGroupComponent, ILightProbeGroupComponentManager>(
              ecs, CORE_NS::GetName<LightProbeGroupComponent>(), 0U)
    {}

    ~LightProbeGroupComponentManager() = default;

    size_t PropertyCount() const override
    {
        return BASE_NS::countof(properties_);
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < BASE_NS::countof(properties_)) {
            return &properties_[index];
        }
        return nullptr;
    }

    array_view<const Property> MetaData() const override
    {
        return properties_;
    }
};

IComponentManager* ILightProbeGroupComponentManagerInstance(IEcs& ecs)
{
    return new LightProbeGroupComponentManager(ecs);
}

void ILightProbeGroupComponentManagerDestroy(IComponentManager* instance)
{
    static_cast<LightProbeGroupComponentManager*>(instance)->~LightProbeGroupComponentManager();
    ::operator delete(instance);
}
CORE3D_END_NAMESPACE()
