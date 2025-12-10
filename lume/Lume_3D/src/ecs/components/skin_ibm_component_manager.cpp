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

#include <3d/ecs/components/skin_ibm_component.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class SkinIbmComponentManager final : public BaseManager<SkinIbmComponent, ISkinIbmComponentManager> {
    BEGIN_PROPERTY(SkinIbmComponent, componentMetaData_)
#include <3d/ecs/components/skin_ibm_component.h>
    END_PROPERTY();

public:
    explicit SkinIbmComponentManager(IEcs& ecs)
        : BaseManager<SkinIbmComponent, ISkinIbmComponentManager>(ecs, CORE_NS::GetName<SkinIbmComponent>(), 0U)
    {}

    ~SkinIbmComponentManager() = default;

    size_t PropertyCount() const override
    {
        return BASE_NS::countof(componentMetaData_);
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < BASE_NS::countof(componentMetaData_)) {
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
    static_cast<SkinIbmComponentManager*>(instance)->~SkinIbmComponentManager();
    ::operator delete(instance);
}
CORE3D_END_NAMESPACE()