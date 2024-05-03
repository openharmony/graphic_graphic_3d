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

#include <3d/ecs/components/animation_input_component.h>

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

class AnimationInputComponentManager final
    : public BaseManager<AnimationInputComponent, IAnimationInputComponentManager> {
    BEGIN_PROPERTY(AnimationInputComponent, ComponentMetadata)
#include <3d/ecs/components/animation_input_component.h>
    END_PROPERTY();
    const array_view<const Property> ComponentMetaData { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit AnimationInputComponentManager(IEcs& ecs)
        : BaseManager<AnimationInputComponent, IAnimationInputComponentManager>(
              ecs, CORE_NS::GetName<AnimationInputComponent>())
    {}

    ~AnimationInputComponentManager() = default;

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

IComponentManager* IAnimationInputComponentManagerInstance(IEcs& ecs)
{
    return new AnimationInputComponentManager(ecs);
}

void IAnimationInputComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<AnimationInputComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
