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

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"
#include "ecs/components/previous_joint_matrices_component.h"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class PreviousJointMatricesComponentManager final
    : public BaseManager<PreviousJointMatricesComponent, IPreviousJointMatricesComponentManager> {
    BEGIN_PROPERTY(PreviousJointMatricesComponent, properties_)
#include "ecs/components/previous_joint_matrices_component.h"
    END_PROPERTY();

public:
    explicit PreviousJointMatricesComponentManager(IEcs& ecs)
        : BaseManager<PreviousJointMatricesComponent, IPreviousJointMatricesComponentManager>(
              ecs, CORE_NS::GetName<PreviousJointMatricesComponent>(), 0U)
    {}

    ~PreviousJointMatricesComponentManager() = default;

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

IComponentManager* IPreviousJointMatricesComponentManagerInstance(IEcs& ecs)
{
    return new PreviousJointMatricesComponentManager(ecs);
}

void IPreviousJointMatricesComponentManagerDestroy(IComponentManager* instance)
{
    static_cast<PreviousJointMatricesComponentManager*>(instance)->~PreviousJointMatricesComponentManager();
    ::operator delete(instance);
}
CORE3D_END_NAMESPACE()
