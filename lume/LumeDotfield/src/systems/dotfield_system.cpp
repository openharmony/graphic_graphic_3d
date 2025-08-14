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

#include <cmath>
#include <random>

#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
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
#include <core/property_tools/property_api_impl.h>
#include <core/property_tools/property_api_impl.inl>
#include <core/property_tools/property_data.h>
#include <core/property_tools/property_macros.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
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
    explicit DotfieldSystem(IEcs& aECS);
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

    // Listener
    void OnComponentEvent(
        EventType type, const IComponentManager& componentManager, array_view<const Entity> entities) override;

    // IDotfieldSystem

private:
    void UpdateRenderDataStore();

    struct PropertyData {
        float speed { 1.f };
    };

    PROPERTY_LIST(PropertyData, ComponentMetadata, MEMBER_PROPERTY(speed, "Simulation speed", 0))

    bool active_ { true };
    IEcs& ecs_;
    IWorldMatrixComponentManager* worldMatrixManager_ { nullptr };
    IMaterialComponentManager* materialManager_ { nullptr };
    IMeshComponentManager* meshManager_ { nullptr };
    IRenderMeshComponentManager* renderMeshManager_ { nullptr };
    IRenderHandleComponentManager* renderHandleManager_ { nullptr };
    IDotfieldComponentManager* dotfieldManager_ { nullptr };
    IRenderDataStoreManager* renderDataStoreManager_ { nullptr };
    EntityReference shader_;
    PropertyData props_ { 1.f };

    PropertyApiImpl<PropertyData> propertyApi_ = PropertyApiImpl<PropertyData>(&props_, ComponentMetadata);
    BASE_NS::refcnt_ptr<IRenderDataStoreDefaultDotfield> dsDefaultDotfield_;
};

DotfieldSystem::DotfieldSystem(IEcs& ecs)
    : ecs_(ecs), worldMatrixManager_(GetManager<IWorldMatrixComponentManager>(ecs)),
      materialManager_(GetManager<IMaterialComponentManager>(ecs)),
      meshManager_(GetManager<IMeshComponentManager>(ecs)),
      renderMeshManager_(GetManager<IRenderMeshComponentManager>(ecs)),
      renderHandleManager_(GetManager<IRenderHandleComponentManager>(ecs)),
      dotfieldManager_(GetManager<IDotfieldComponentManager>(ecs))
{
    IClassRegister* classRegister = ecs_.GetClassFactory().GetInterface<IClassRegister>();
    if (classRegister) {
        if (IRenderContext* renderContext = GetInstance<IRenderContext>(*classRegister, UID_RENDER_CONTEXT);
            renderContext) {
            renderDataStoreManager_ = &renderContext->GetRenderDataStoreManager();

            const IShaderManager& shaderMgr = renderContext->GetDevice().GetShaderManager();
            auto shaderHandle = shaderMgr.GetShaderHandle("dotfieldshaders://shader/default_material_dotfield.shader");

            shader_ = ecs.GetEntityManager().CreateReferenceCounted();
            renderHandleManager_->Create(shader_);
            renderHandleManager_->Write(shader_)->reference = shaderHandle;
        }
    }
    if (dotfieldManager_) {
        ecs_.AddListener(*dotfieldManager_, *this);
    }
}

DotfieldSystem ::~DotfieldSystem()
{
    if (dotfieldManager_) {
        ecs_.RemoveListener(*dotfieldManager_, *this);
    }
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
    if (!(worldMatrixManager_ && dotfieldManager_ && renderDataStoreManager_)) {
        return false;
    }
    if (!dsDefaultDotfield_) {
        UpdateRenderDataStore();
    }
    if (dsDefaultDotfield_) {
        // delta time from us to s
        const auto timeStep = static_cast<float>(delta) * 0.000001f * props_.speed;
        dsDefaultDotfield_->SetTimeStep(timeStep);
        dsDefaultDotfield_->SetTime(static_cast<float>(time) * 0.000001f);
        const float dsTime = dsDefaultDotfield_->GetTime();
        array_view<RenderDataDefaultDotfield::DotfieldPrimitive> dotfieldPrimitives =
            dsDefaultDotfield_->GetDotfieldPrimitives();
        const auto& bufferData = dsDefaultDotfield_->GetBufferData();
        auto buffersIt = bufferData.buffers.cbegin();
        for (auto& prim : dotfieldPrimitives) {
            const auto& dfc = dotfieldManager_->Get(prim.entity);
            prim.touch = dfc.touchPosition;
            prim.touchDirection = dfc.touchDirection;
            prim.touchRadius = dfc.touchRadius;
            prim.pointScale = dfc.pointScale;
            prim.colors = { dfc.color0, dfc.color1, dfc.color2, dfc.color3 };
            if (worldMatrixManager_->HasComponent(prim.entity)) {
                WorldMatrixComponent wc = worldMatrixManager_->Get(prim.entity);
                prim.matrix = wc.matrix;
            }
            if (auto meshHandle = meshManager_->Write(prim.entity)) {
                const auto& buffer = buffersIt->dataBuffer[bufferData.currFrameIndex];
                GpuBufferDesc desc;
                IClassRegister* classRegister = ecs_.GetClassFactory().GetInterface<IClassRegister>();
                if (classRegister) {
                    if (IRenderContext* renderContext = GetInstance<IRenderContext>(*classRegister, UID_RENDER_CONTEXT);
                        renderContext) {
                        desc = renderContext->GetDevice().GetGpuResourceManager().GetBufferDescriptor(buffer);
                    }
                }
                meshHandle->submeshes[0U].bufferAccess[MeshComponent::Submesh::DM_VB_POS].buffer =
                    GetOrCreateEntityReference(ecs_, buffersIt->dataBuffer[bufferData.currFrameIndex]);
                meshHandle->submeshes[0U].bufferAccess[MeshComponent::Submesh::DM_VB_POS].byteSize = desc.byteSize;
            }
            if (auto materialHandle = materialManager_->Write(prim.entity)) {
                materialHandle->materialShader.shader = shader_;
                BASE_NS::CloneData(
                    &materialHandle->textures[1].factor, sizeof(Math::Vec4), &prim.colors, sizeof(Math::UVec4));
                materialHandle->textures[2].factor = { dsTime, dfc.pointScale, 0.f, 0.f }; // 2: texture idx
            }
        }
    }
    return frameRenderingQueued;
}

void DotfieldSystem::OnComponentEvent(
    EventType type, const IComponentManager& componentManager, array_view<const Entity> entities)
{
    if (!(worldMatrixManager_ && dotfieldManager_ && renderDataStoreManager_)) {
        return;
    }
    UpdateRenderDataStore();
    if (!dsDefaultDotfield_) {
        return;
    }
    switch (type) {
        case EventType::CREATED:
            for (const auto& entity : entities) {
                if (worldMatrixManager_->HasComponent(entity)) {
                    WorldMatrixComponent wc = worldMatrixManager_->Get(entity);
                    DotfieldComponent dfc = dotfieldManager_->Get(entity);

                    RenderDataDefaultDotfield::DotfieldPrimitive prim;
                    prim.entity = entity;
                    prim.matrix = wc.matrix;
                    prim.touch = dfc.touchPosition;
                    prim.touchDirection = dfc.touchDirection;
                    prim.touchRadius = dfc.touchRadius;
                    prim.colors = { dfc.color0, dfc.color1, dfc.color2, dfc.color3 };
                    prim.size = { static_cast<uint32_t>(dfc.size.x), static_cast<uint32_t>(dfc.size.y) };
                    const auto index = static_cast<uint32_t>(dsDefaultDotfield_->GetDotfieldPrimitives().size());
                    dsDefaultDotfield_->AddDotfieldPrimitive(prim);

                    materialManager_->Create(entity);
                    if (auto materialHandle = materialManager_->Write(entity)) {
                        materialHandle->type = MaterialComponent::Type::CUSTOM;
                        materialHandle->materialShader.shader = shader_;
                        BASE_NS::CloneData(
                            &materialHandle->textures[1].factor, sizeof(Math::Vec4), &prim.colors, sizeof(Math::UVec4));
                        materialHandle->textures[2].factor = { 0.f, dfc.pointScale, 0.f, 0.f }; // 2: texture idx
                    }
                    meshManager_->Create(entity);
                    if (auto meshHandle = meshManager_->Write(entity)) {
                        auto& submesh = meshHandle->submeshes.emplace_back();
                        submesh.material = entity;
                        submesh.aabbMin = { -10.f, -10.f, -10.f };
                        submesh.aabbMax = { 10.f, 10.f, 10.f };
                        submesh.instanceCount = prim.size.x * prim.size.y;
                        submesh.vertexCount = 1;

                        meshHandle->aabbMin = { -10.f, -10.f, -10.f };
                        meshHandle->aabbMax = { 10.f, 10.f, 10.f };
                    }

                    renderMeshManager_->Create(entity);
                    renderMeshManager_->Write(entity)->mesh = entity;
                    renderMeshManager_->Write(entity)->customData[0] = { prim.size.x, prim.size.y, index, 0U };
                }
            }
            break;

        case EventType::DESTROYED:
            for (const auto& entity : entities) {
                dsDefaultDotfield_->RemoveDotfieldPrimitive(entity);
            }
            break;

        case EventType::MODIFIED:
            break;

        case EventType::MOVED:
            break;
    }
}

void DotfieldSystem::UpdateRenderDataStore()
{
    if (!dsDefaultDotfield_) {
        if (auto preprocessor = GetSystem<IRenderPreprocessorSystem>(ecs_)) {
            if (auto propertyHandle =
                    ScopedHandle<const IRenderPreprocessorSystem::Properties>(preprocessor->GetProperties())) {
                const auto name = propertyHandle->dataStorePrefix + "RenderDataStoreDefaultDotfield";
                dsDefaultDotfield_ = refcnt_ptr<IRenderDataStoreDefaultDotfield>(
                    renderDataStoreManager_->Create(IRenderDataStoreDefaultDotfield::UID, name.data()));
            }
        }
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
