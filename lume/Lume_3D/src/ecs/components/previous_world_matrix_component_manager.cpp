/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"
#include "ecs/components/previous_world_matrix_component.h"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;
using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class PreviousWorldMatrixComponentManager final
    : public BaseManager<PreviousWorldMatrixComponent, IPreviousWorldMatrixComponentManager> {
    BEGIN_PROPERTY(PreviousWorldMatrixComponent, ComponentMetadata)
#include <3d/ecs/components/world_matrix_component.h>
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit PreviousWorldMatrixComponentManager(IEcs& ecs)
        : BaseManager<PreviousWorldMatrixComponent, IPreviousWorldMatrixComponentManager>(
              ecs, CORE_NS::GetName<PreviousWorldMatrixComponent>())
    {}

    ~PreviousWorldMatrixComponentManager() = default;

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

IComponentManager* IPreviousWorldMatrixComponentManagerInstance(IEcs& ecs)
{
    return new PreviousWorldMatrixComponentManager(ecs);
}

void IPreviousWorldMatrixComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<PreviousWorldMatrixComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
