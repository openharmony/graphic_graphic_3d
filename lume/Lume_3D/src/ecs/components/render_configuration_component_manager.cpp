/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include <3d/ecs/components/render_configuration_component.h>
#include <core/property/property_types.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE_BEGIN_NAMESPACE()
using CORE3D_NS::RenderConfigurationComponent;

// Extend propertysystem with the enums
DECLARE_PROPERTY_TYPE(RenderConfigurationComponent::SceneShadowType);
DECLARE_PROPERTY_TYPE(RenderConfigurationComponent::SceneShadowQuality);
DECLARE_PROPERTY_TYPE(RenderConfigurationComponent::SceneShadowSmoothness);

// Declare their metadata
BEGIN_ENUM(SceneShadowTypeMetaData, RenderConfigurationComponent::SceneShadowType)
DECL_ENUM(RenderConfigurationComponent::SceneShadowType, PCF, "PCF (Percentage Closer Filtering)")
DECL_ENUM(RenderConfigurationComponent::SceneShadowType, VSM, "VSM (Variance Shadow Maps)")
END_ENUM(SceneShadowTypeMetaData, RenderConfigurationComponent::SceneShadowType)

BEGIN_ENUM(SceneShadowQualityMetaData, RenderConfigurationComponent::SceneShadowQuality)
DECL_ENUM(RenderConfigurationComponent::SceneShadowQuality, LOW, "Low")
DECL_ENUM(RenderConfigurationComponent::SceneShadowQuality, NORMAL, "Normal")
DECL_ENUM(RenderConfigurationComponent::SceneShadowQuality, HIGH, "High")
DECL_ENUM(RenderConfigurationComponent::SceneShadowQuality, ULTRA, "Ultra")
END_ENUM(SceneShadowQualityMetaData, RenderConfigurationComponent::SceneShadowQuality)

BEGIN_ENUM(SceneShadowSmoothnessMetaData, RenderConfigurationComponent::SceneShadowSmoothness)
DECL_ENUM(RenderConfigurationComponent::SceneShadowSmoothness, HARD, "Hard")
DECL_ENUM(RenderConfigurationComponent::SceneShadowSmoothness, NORMAL, "Normal")
DECL_ENUM(RenderConfigurationComponent::SceneShadowSmoothness, SOFT, "Soft")
END_ENUM(SceneShadowSmoothnessMetaData, RenderConfigurationComponent::SceneShadowSmoothness)

BEGIN_ENUM(SceneRenderingFlagBitsMetaData, RenderConfigurationComponent::SceneRenderingFlagBits)
DECL_ENUM(RenderConfigurationComponent::SceneRenderingFlagBits, CREATE_RNGS_BIT, "Create RNGs")
END_ENUM(SceneRenderingFlagBitsMetaData, RenderConfigurationComponent::SceneRenderingFlagBits)
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
    BEGIN_PROPERTY(RenderConfigurationComponent, ComponentMetadata)
#include <3d/ecs/components/render_configuration_component.h>
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit RenderConfigurationComponentManager(IEcs& ecs)
        : BaseManager<RenderConfigurationComponent, IRenderConfigurationComponentManager>(
              ecs, CORE_NS::GetName<RenderConfigurationComponent>())
    {}

    ~RenderConfigurationComponentManager() = default;

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

IComponentManager* IRenderConfigurationComponentManagerInstance(IEcs& ecs)
{
    return new RenderConfigurationComponentManager(ecs);
}

void IRenderConfigurationComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<RenderConfigurationComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
