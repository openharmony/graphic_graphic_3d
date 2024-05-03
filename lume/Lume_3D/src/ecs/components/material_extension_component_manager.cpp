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

#include <ComponentTools/base_manager.h>
#include <ComponentTools/base_manager.inl>

#include <3d/ecs/components/material_extension_component.h>
#include <core/ecs/intf_ecs.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_class_register.h>
#include <core/property/property_types.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/resource_handle.h>

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::GetInstance;
using CORE_NS::IClassRegister;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

using RENDER_NS::IGpuResourceManager;
using RENDER_NS::IRenderContext;

class MaterialExtensionComponentManager final
    : public BaseManager<MaterialExtensionComponent, IMaterialExtensionComponentManager> {
    BEGIN_PROPERTY(MaterialExtensionComponent, ComponentMetadata)
#include <3d/ecs/components/material_extension_component.h>
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };
    IGpuResourceManager& gpuResourceManager_;

public:
    explicit MaterialExtensionComponentManager(IEcs& ecs)
        : BaseManager<MaterialExtensionComponent, IMaterialExtensionComponentManager>(
              ecs, CORE_NS::GetName<MaterialExtensionComponent>()),
          gpuResourceManager_(GetInstance<IRenderContext>(
              *ecs.GetClassFactory().GetInterface<IClassRegister>(), RENDER_NS::UID_RENDER_CONTEXT)
                                  ->GetDevice()
                                  .GetGpuResourceManager())
    {}

    ~MaterialExtensionComponentManager() = default;

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

    bool Destroy(CORE_NS::Entity entity) override
    {
        if (const auto id = GetComponentId(entity); id != INVALID_COMPONENT_ID) {
            if (auto handle = Write(id); handle) {
                auto& comp = *handle;
                for (uint32_t idx = 0; idx < MaterialExtensionComponent::RESOURCE_COUNT; ++idx) {
                    comp.resources[idx] = {};
                }
            }
            return BaseManager::Destroy(entity);
        }
        return false;
    }
};

IComponentManager* IMaterialExtensionComponentManagerInstance(IEcs& ecs)
{
    return new MaterialExtensionComponentManager(ecs);
}

void IMaterialExtensionComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<MaterialExtensionComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
