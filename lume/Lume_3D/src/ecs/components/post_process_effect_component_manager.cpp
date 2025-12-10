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

#include <3d/ecs/components/post_process_effect_component.h>
#include <render/datastore/render_data_store_render_pods.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(RENDER_NS::IRenderPostProcess::Ptr);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<RENDER_NS::IRenderPostProcess::Ptr>);
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;
using CORE_NS::PropertyFlags;

class PostProcessEffectComponentManager final
    : public BaseManager<PostProcessEffectComponent, IPostProcessEffectComponentManager> {
    BEGIN_PROPERTY(PostProcessEffectComponent, componentMetaData_)
#include <3d/ecs/components/post_process_effect_component.h>
    END_PROPERTY();

public:
    explicit PostProcessEffectComponentManager(IEcs& ecs)
        : BaseManager<PostProcessEffectComponent, IPostProcessEffectComponentManager>(
              ecs, CORE_NS::GetName<PostProcessEffectComponent>(), 0U)
    {}

    ~PostProcessEffectComponentManager() = default;

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

IComponentManager* IPostProcessEffectComponentManagerInstance(IEcs& ecs)
{
    return new PostProcessEffectComponentManager(ecs);
}

void IPostProcessEffectComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<PostProcessEffectComponentManager*>(instance);
}

CORE3D_END_NAMESPACE()
