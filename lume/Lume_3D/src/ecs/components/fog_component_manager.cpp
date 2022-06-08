/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include <3d/ecs/components/fog_component.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class FogComponentManager final : public BaseManager<FogComponent, IFogComponentManager> {
    BEGIN_PROPERTY(FogComponent, ComponentMetadata)
#include <3d/ecs/components/fog_component.h>
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit FogComponentManager(IEcs& ecs)
        : BaseManager<FogComponent, IFogComponentManager>(ecs, CORE_NS::GetName<FogComponent>())
    {}

    ~FogComponentManager() = default;

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

IComponentManager* IFogComponentManagerInstance(IEcs& ecs)
{
    return new FogComponentManager(ecs);
}

void IFogComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<FogComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
