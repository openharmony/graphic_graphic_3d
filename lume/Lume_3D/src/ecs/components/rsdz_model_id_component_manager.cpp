/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include <3d/ecs/components/rsdz_model_id_component.h>

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

class RSDZModelIdComponentManager final : public BaseManager<RSDZModelIdComponent, IRSDZModelIdComponentManager> {
    BEGIN_PROPERTY(RSDZModelIdComponent, ComponentMetadata)
#include <3d/ecs/components/rsdz_model_id_component.h>
    END_PROPERTY();
    const array_view<const Property> ComponentMetaData { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit RSDZModelIdComponentManager(IEcs& ecs)
        : BaseManager<RSDZModelIdComponent, IRSDZModelIdComponentManager>(ecs, CORE_NS::GetName<RSDZModelIdComponent>())
    {}

    ~RSDZModelIdComponentManager() = default;

    size_t PropertyCount() const override
    {
        return ComponentMetaData.size();
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < ComponentMetaData.size()) {
            return &ComponentMetaData[index];
        }
        return nullptr;
    }

    array_view<const Property> MetaData() const override
    {
        return ComponentMetaData;
    }
};

IComponentManager* IRSDZModelIdComponentManagerInstance(IEcs& ecs)
{
    return new RSDZModelIdComponentManager(ecs);
}

void IRSDZModelIdComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<RSDZModelIdComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
