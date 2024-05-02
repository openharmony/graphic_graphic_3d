/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <3d/ecs/components/local_matrix_component.h>

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

class LocalMatrixComponentManager final : public BaseManager<LocalMatrixComponent, ILocalMatrixComponentManager> {
    BEGIN_PROPERTY(LocalMatrixComponent, ComponentMetadata)
#include <3d/ecs/components/local_matrix_component.h>
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit LocalMatrixComponentManager(IEcs& ecs)
        : BaseManager<LocalMatrixComponent, ILocalMatrixComponentManager>(ecs, CORE_NS::GetName<LocalMatrixComponent>())
    {}

    ~LocalMatrixComponentManager() = default;

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

IComponentManager* ILocalMatrixComponentManagerInstance(IEcs& ecs)
{
    return new LocalMatrixComponentManager(ecs);
}

void ILocalMatrixComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<LocalMatrixComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
