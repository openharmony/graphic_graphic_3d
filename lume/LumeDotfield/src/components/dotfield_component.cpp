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

#include "dotfield/ecs/components/dotfield_component.h"
#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

namespace Dotfield {
class DotfieldComponentManager final : public CORE_NS::BaseManager<DotfieldComponent, IDotfieldComponentManager> {
    BEGIN_PROPERTY(DotfieldComponent, ComponentMetadata)
#include "dotfield/ecs/components/dotfield_component.h"
    END_PROPERTY();
    const BASE_NS::array_view<const CORE_NS::Property> componentMetaData_{ ComponentMetadata,
        BASE_NS::countof(ComponentMetadata) };

public:
    DotfieldComponentManager(CORE_NS::IEcs& ecs)
        : BaseManager<DotfieldComponent, IDotfieldComponentManager>(ecs, CORE_NS::GetName<DotfieldComponent>())
    {}

    ~DotfieldComponentManager() = default;

    size_t PropertyCount() const override
    {
        return componentMetaData_.size();
    }

    const CORE_NS::Property* MetaData(size_t index) const override
    {
        if (index < componentMetaData_.size()) {
            return &componentMetaData_[index];
        }
        return nullptr;
    }

    BASE_NS::array_view<const CORE_NS::Property> MetaData() const override
    {
        return componentMetaData_;
    }
};

CORE_NS::IComponentManager* IDotfieldComponentManagerInstance(CORE_NS::IEcs& ecs)
{
    return new DotfieldComponentManager(ecs);
}

void IDotfieldComponentManagerDestroy(CORE_NS::IComponentManager* instance)
{
    delete static_cast<DotfieldComponentManager*>(instance);
}
} // namespace Dotfield
