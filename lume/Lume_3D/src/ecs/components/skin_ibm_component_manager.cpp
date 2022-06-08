/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include <3d/ecs/components/skin_ibm_component.h>

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

class SkinIbmComponentManager final : public BaseManager<SkinIbmComponent, ISkinIbmComponentManager> {
    BEGIN_PROPERTY(SkinIbmComponent, ComponentMetadata)
#include <3d/ecs/components/skin_ibm_component.h>
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit SkinIbmComponentManager(IEcs& ecs)
        : BaseManager<SkinIbmComponent, ISkinIbmComponentManager>(ecs, CORE_NS::GetName<SkinIbmComponent>())
    {}

    ~SkinIbmComponentManager() = default;

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

IComponentManager* ISkinIbmComponentManagerInstance(IEcs& ecs)
{
    return new SkinIbmComponentManager(ecs);
}

void ISkinIbmComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<SkinIbmComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()