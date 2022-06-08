/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"
#include "ecs/components/previous_joint_matrices_component.h"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class PreviousJointMatricesComponentManager final
    : public BaseManager<PreviousJointMatricesComponent, IPreviousJointMatricesComponentManager> {
    BEGIN_PROPERTY(PreviousJointMatricesComponent, ComponentMetadata)
#include "ecs/components/previous_joint_matrices_component.h"
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit PreviousJointMatricesComponentManager(IEcs& ecs)
        : BaseManager<PreviousJointMatricesComponent, IPreviousJointMatricesComponentManager>(
              ecs, CORE_NS::GetName<PreviousJointMatricesComponent>())
    {}

    ~PreviousJointMatricesComponentManager() = default;

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

IComponentManager* IPreviousJointMatricesComponentManagerInstance(IEcs& ecs)
{
    return new PreviousJointMatricesComponentManager(ecs);
}

void IPreviousJointMatricesComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<PreviousJointMatricesComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
