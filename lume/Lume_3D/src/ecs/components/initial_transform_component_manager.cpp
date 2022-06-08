/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"
#include "ecs/components/initial_transform_component.h"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(BASE_NS::vector<uint8_t>);
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class InitialTransformComponentManager final
    : public BaseManager<InitialTransformComponent, IInitialTransformComponentManager> {
    BEGIN_PROPERTY(InitialTransformComponent, ComponentMetadata)
#include "ecs/components/initial_transform_component.h"
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit InitialTransformComponentManager(IEcs& ecs)
        : BaseManager<InitialTransformComponent, IInitialTransformComponentManager>(
              ecs, CORE_NS::GetName<InitialTransformComponent>())
    {}

    ~InitialTransformComponentManager() = default;

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

IComponentManager* IInitialTransformComponentManagerInstance(IEcs& ecs)
{
    return new InitialTransformComponentManager(ecs);
}

void IInitialTransformComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<InitialTransformComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
