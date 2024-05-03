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

#include <algorithm>

#include <3d/ecs/components/render_handle_component.h>
#include <core/intf_engine.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/resource_handle.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(RENDER_NS::RenderHandle);
DECLARE_PROPERTY_TYPE(RENDER_NS::RenderHandleReference);
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::Entity;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

using RENDER_NS::RenderHandle;
using RENDER_NS::RenderHandleReference;

class RenderHandleComponentManager final : public BaseManager<RenderHandleComponent, IRenderHandleComponentManager> {
    BEGIN_PROPERTY(RenderHandleComponent, ComponentMetadata)
#include <3d/ecs/components/render_handle_component.h>
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit RenderHandleComponentManager(IEcs& ecs)
        : BaseManager<RenderHandleComponent, IRenderHandleComponentManager>(
              ecs, CORE_NS::GetName<RenderHandleComponent>())
    {}

    ~RenderHandleComponentManager() = default;

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

    RenderHandleReference GetRenderHandleReference(const CORE_NS::Entity entity) const override
    {
        if (const auto index = GetComponentId(entity); index != IComponentManager::INVALID_COMPONENT_ID) {
            return components_[index].data_.reference;
        }
        return {};
    }

    RenderHandleReference GetRenderHandleReference(const IComponentManager::ComponentId index) const override
    {
        if (index < components_.size()) {
            return components_[index].data_.reference;
        }
        return {};
    }

    RenderHandle GetRenderHandle(const CORE_NS::Entity entity) const override
    {
        if (const auto index = GetComponentId(entity); index != IComponentManager::INVALID_COMPONENT_ID) {
            return components_[index].data_.reference.GetHandle();
        }
        return {};
    }

    RenderHandle GetRenderHandle(const IComponentManager::ComponentId index) const override
    {
        if (index < components_.size()) {
            return components_[index].data_.reference.GetHandle();
        }
        return {};
    }

    Entity GetEntityWithReference(const RenderHandleReference& handle) const override
    {
        if (const auto pos = std::find_if(components_.begin(), components_.end(),
                [handle = handle.GetHandle()](
                    const BaseComponentHandle& component) { return component.data_.reference.GetHandle() == handle; });
            pos != components_.end()) {
            return pos->entity_;
        }
        return {};
    }
};

IComponentManager* IRenderHandleComponentManagerInstance(IEcs& ecs)
{
    return new RenderHandleComponentManager(ecs);
}

void IRenderHandleComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<RenderHandleComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()