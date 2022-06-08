/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include <algorithm>

#include <3d/ecs/components/render_handle_component.h>
#include <core/intf_engine.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

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