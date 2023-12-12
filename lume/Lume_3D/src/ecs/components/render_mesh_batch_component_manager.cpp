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

#include <3d/ecs/components/render_mesh_batch_component.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE_BEGIN_NAMESPACE()
using namespace CORE3D_NS;

// Extend propertysystem with the enums
DECLARE_PROPERTY_TYPE(RenderMeshBatchComponent::BatchType);

// Declare their metadata
BEGIN_ENUM(BatchTypeMetaData, RenderMeshBatchComponent::BatchType)
DECL_ENUM(RenderMeshBatchComponent::BatchType, GPU_INSTANCING, "GPU Instancing")
END_ENUM(BatchTypeMetaData, RenderMeshBatchComponent::BatchType)
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;

class RenderMeshBatchComponentManager final
    : public BaseManager<RenderMeshBatchComponent, IRenderMeshBatchComponentManager> {
    BEGIN_PROPERTY(RenderMeshBatchComponent, ComponentMetadata)
#include <3d/ecs/components/render_mesh_batch_component.h>
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    RenderMeshBatchComponentManager(IEcs& ecs)
        : BaseManager<RenderMeshBatchComponent, IRenderMeshBatchComponentManager>(
              ecs, CORE_NS::GetName<RenderMeshBatchComponent>())
    {}

    ~RenderMeshBatchComponentManager() = default;

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

IComponentManager* IRenderMeshBatchComponentManagerInstance(IEcs& ecs)
{
    return new RenderMeshBatchComponentManager(ecs);
}

void IRenderMeshBatchComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<RenderMeshBatchComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
