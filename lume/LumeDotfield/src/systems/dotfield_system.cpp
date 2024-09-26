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

#include "dotfield/ecs/systems/dotfield_system.h"

#include <PropertyTools/property_api_impl.h>
#include <PropertyTools/property_api_impl.inl>
#include <PropertyTools/property_data.h>
#include <PropertyTools/property_macros.h>
#include <cmath>
#include <random>

#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <base/containers/fixed_string.h>
#include <base/containers/unordered_map.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_class_factory.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/implementation_uids.h>

#include "dotfield/ecs/components/dotfield_component.h"
#include "render/render_data_store_default_dotfield.h"

namespace {
#include "app/shaders/common/dotfield_struct_common.h"
} // namespace

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;
using namespace Dotfield;
namespace {

class DotfieldSystem final : public IDotfieldSystem, private IEcs::ComponentListener {
public:
    DotfieldSystem(IEcs& aECS);
    ~DotfieldSystem();

    // ISystem
    string_view GetName() const override;
    Uid GetUid() const override;

    IPropertyHandle* GetProperties() override;
    const IPropertyHandle* GetProperties() const override;
    void SetProperties(const IPropertyHandle&) override;

    bool IsActive() const override;
    void SetActive(bool state) override;

    void Initialize() override;
    void Uninitialize() override;
    bool Update(bool frameRenderingQueued, uint64_t time, uint64_t delta) override;

    const IEcs& GetECS() const override;

    void OnComponentEvent(
        EventType type, const IComponentManager& componentManager, array_view<const Entity> entities) override;

private:
    struct PropertyData {
        float speed { 1.f };
    };

    BEGIN_PROPERTY(PropertyData, ComponentMetadata)
        DECL_PROPERTY2(PropertyData, speed, "Simulation speed", 0)
    END_PROPERTY();

    bool active_ { true };
    IEcs& ecs_;
    IRenderDataStoreManager& renderDataStoreManager_;
    IWorldMatrixComponentManager& worldMatrixManager_;
    IDotfieldComponentManager& dotfieldManager_;

    PropertyData props_ { 1.f };

    PropertyApiImpl<PropertyData> propertyApi_ = PropertyApiImpl<PropertyData>(&props_, ComponentMetadata);
};

DotfieldSystem::DotfieldSystem(IEcs& ecs)
    : ecs_(ecs), renderDataStoreManager_(GetInstance<IRenderContext>(
                     *ecs.GetClassFactory().GetInterface<IClassRegister>(), UID_RENDER_CONTEXT)
                                             ->GetRenderDataStoreManager()),
      worldMatrixManager_(*GetManager<IWorldMatrixComponentManager>(ecs)),
      dotfieldManager_(*GetManager<IDotfieldComponentManager>(ecs))
{
    ecs.AddListener(dotfieldManager_, *this);
}

DotfieldSystem ::~DotfieldSystem()
{
    ecs_.RemoveListener(dotfieldManager_, *this);
}

void DotfieldSystem::SetActive(bool state)
{
    active_ = state;
}

bool DotfieldSystem::IsActive() const
{
    return active_;
}

string_view DotfieldSystem::GetName() const
{
    return Dotfield::GetName(this);
}

Uid DotfieldSystem::GetUid() const
{
    return UID;
}

IPropertyHandle* DotfieldSystem::GetProperties()
{
    return propertyApi_.GetData();
}

const IPropertyHandle* DotfieldSystem::GetProperties() const
{
    return propertyApi_.GetData();
}

void DotfieldSystem::SetProperties(const IPropertyHandle& dataHandle)
{
    if (dataHandle.Owner() != &propertyApi_) {
        return;
    }
    if (auto data = ScopedHandle<const PropertyData>(&dataHandle); data) {
        props_ = *data;
    }
}

const IEcs& DotfieldSystem::GetECS() const
{
    return ecs_;
}

void DotfieldSystem::Initialize() {}

void DotfieldSystem::Uninitialize() {}

bool DotfieldSystem::Update(bool frameRenderingQueued, uint64_t time, uint64_t delta)
{
    if (!active_) {
        return false;
    }

    RenderDataStoreDefaultDotfield* dsDefaultDotfield = static_cast<RenderDataStoreDefaultDotfield*>(
        renderDataStoreManager_.GetRenderDataStore("RenderDataStoreDefaultDotfield"));
    if (dsDefaultDotfield) {
        const auto timeStep = delta * 0.000001f * props_.speed;
        dsDefaultDotfield->SetTimeStep(timeStep);
        dsDefaultDotfield->SetTime(time * 0.000001f);

        array_view<RenderDataDefaultDotfield::DotfieldPrimitive> dotfieldPrimitives =
            dsDefaultDotfield->GetDotfieldPrimitives();

        for (auto& prim : dotfieldPrimitives) {
            const auto& dfc = dotfieldManager_.Get(prim.entity);
            prim.touch = dfc.touchPosition;
            prim.touchDirection = dfc.touchDirection;
            prim.touchRadius = dfc.touchRadius;
            prim.pointScale = dfc.pointScale;
            if (worldMatrixManager_.HasComponent(prim.entity)) {
                WorldMatrixComponent wc = worldMatrixManager_.Get(prim.entity);
                prim.matrix = wc.matrix;
            }
        }
    }
    return frameRenderingQueued;
}

void DotfieldSystem::OnComponentEvent(
    EventType type, const IComponentManager& componentManager, array_view<const Entity> entities)
{
    RenderDataStoreDefaultDotfield* dsDefaultDotfield = static_cast<RenderDataStoreDefaultDotfield*>(
        renderDataStoreManager_.GetRenderDataStore("RenderDataStoreDefaultDotfield"));
    switch (type) {
        case EventType::CREATED:
            for (const auto& entity : entities) {
                if (worldMatrixManager_.HasComponent(entity)) {
                    WorldMatrixComponent wc = worldMatrixManager_.Get(entity);
                    DotfieldComponent dfc = dotfieldManager_.Get(entity);

                    RenderDataDefaultDotfield::DotfieldPrimitive prim;
                    prim.entity = entity;
                    prim.matrix = wc.matrix;
                    prim.touch = dfc.touchPosition;
                    prim.touchDirection = dfc.touchDirection;
                    prim.touchRadius = dfc.touchRadius;
                    prim.colors = { dfc.color0, dfc.color1, dfc.color2, dfc.color3 };
                    prim.size = { static_cast<uint32_t>(dfc.size.x), static_cast<uint32_t>(dfc.size.y) };
                    dsDefaultDotfield->AddDotfieldPrimitive(prim);
                }
            }
            break;

        case EventType::DESTROYED:
            for (const auto& entity : entities) {
                dsDefaultDotfield->RemoveDotfieldPrimitive(entity);
            }
            break;

        case EventType::MODIFIED:
            break;
    }
}

} // namespace

namespace Dotfield {
CORE_NS::ISystem* IDotfieldSystemInstance(IEcs& ecs)
{
    return new DotfieldSystem(ecs);
}
void IDotfieldSystemDestroy(CORE_NS::ISystem* instance)
{
    delete static_cast<DotfieldSystem*>(instance);
}
} // namespace Dotfield
