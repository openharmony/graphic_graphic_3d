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

#include <3d/ecs/components/render_configuration_component.h>
#include <core/property/property_types.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

CORE_BEGIN_NAMESPACE()
using CORE3D_NS::RenderConfigurationComponent;

// Extend propertysystem with the enums
DECLARE_PROPERTY_TYPE(RenderConfigurationComponent::SceneShadowType);
DECLARE_PROPERTY_TYPE(RenderConfigurationComponent::SceneShadowQuality);
DECLARE_PROPERTY_TYPE(RenderConfigurationComponent::SceneShadowSmoothness);

// Declare their metadata
ENUM_TYPE_METADATA(RenderConfigurationComponent::SceneShadowType, ENUM_VALUE(PCF, "PCF (Percentage Closer Filtering)"),
    ENUM_VALUE(VSM, "VSM (Variance Shadow Maps)"))

ENUM_TYPE_METADATA(RenderConfigurationComponent::SceneShadowQuality, ENUM_VALUE(LOW, "Low"),
    ENUM_VALUE(NORMAL, "Normal"), ENUM_VALUE(HIGH, "High"), ENUM_VALUE(ULTRA, "Ultra"))

ENUM_TYPE_METADATA(RenderConfigurationComponent::SceneShadowSmoothness, ENUM_VALUE(HARD, "Hard"),
    ENUM_VALUE(NORMAL, "Normal"), ENUM_VALUE(SOFT, "Soft"))

ENUM_TYPE_METADATA(RenderConfigurationComponent::SceneRenderingFlagBits, ENUM_VALUE(CREATE_RNGS_BIT, "Create RNGs"))
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;
using CORE_NS::PropertyFlags;

class RenderConfigurationComponentManager final
    : public BaseManager<RenderConfigurationComponent, IRenderConfigurationComponentManager> {
    BEGIN_PROPERTY(RenderConfigurationComponent, componentMetaData_)
#include <3d/ecs/components/render_configuration_component.h>
    END_PROPERTY();

public:
    explicit RenderConfigurationComponentManager(IEcs& ecs)
        : BaseManager<RenderConfigurationComponent, IRenderConfigurationComponentManager>(
              ecs, CORE_NS::GetName<RenderConfigurationComponent>(), 1U)
    {}

    ~RenderConfigurationComponentManager() = default;

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

IComponentManager* IRenderConfigurationComponentManagerInstance(IEcs& ecs)
{
    return new RenderConfigurationComponentManager(ecs);
}

void IRenderConfigurationComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<RenderConfigurationComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
