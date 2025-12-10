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

#include <3d/ecs/components/light_component.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"
#include "ecs/components/layer_flag_bits_metadata.h"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

CORE_BEGIN_NAMESPACE()
using CORE3D_NS::LightComponent;

/** Extend propertysystem with the enums */
DECLARE_PROPERTY_TYPE(LightComponent::Type);
DECLARE_PROPERTY_TYPE(LightComponent::RectLight);

// Declare their metadata
ENUM_TYPE_METADATA(LightComponent::Type, ENUM_VALUE(DIRECTIONAL, "Directional"), ENUM_VALUE(POINT, "Point"),
    ENUM_VALUE(SPOT, "Spot"), ENUM_VALUE(RECT, "Rect"))

DATA_TYPE_METADATA(LightComponent::RectLight, MEMBER_PROPERTY(width, "Width", 0), MEMBER_PROPERTY(height, "Height", 0))

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
    BEGIN_PROPERTY(LightComponent, componentMetaData_)
#include <3d/ecs/components/light_component.h>
    END_PROPERTY();

public:
    explicit LightComponentManager(IEcs& ecs)
        : BaseManager<LightComponent, ILightComponentManager>(ecs, CORE_NS::GetName<LightComponent>())
    {}

    ~LightComponentManager() = default;

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

IComponentManager* ILightComponentManagerInstance(IEcs& ecs)
{
    return new LightComponentManager(ecs);
}

void ILightComponentManagerDestroy(IComponentManager* instance)
{
    static_cast<LightComponentManager*>(instance)->~LightComponentManager();
    ::operator delete(instance);
}
CORE3D_END_NAMESPACE()
