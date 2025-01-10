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

#include "morphing_system.h"

#include <algorithm>

#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/morph_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/render/intf_render_data_store_morph.h>
#include <core/ecs/intf_ecs.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.inl>
#include <core/property_tools/property_macros.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>

#include "property/property_handle.h"

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(RENDER_NS::IRenderDataStoreManager*);
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace {
constexpr auto MORPH_INDEX = 0U;
constexpr auto RENDER_MESH_INDEX = 1U;
constexpr auto NODE_INDEX = 2U;

PROPERTY_LIST(IMorphingSystem::Properties, ComponentMetadata, MEMBER_PROPERTY(dataStoreName, "RenderDataStoreMorph", 0))

RenderVertexBuffer GetBuffer(
    const MeshComponent::Submesh::BufferAccess& bufferAccess, const IRenderHandleComponentManager& bufferManager)
{
    if (EntityUtil::IsValid(bufferAccess.buffer)) {
        return { bufferManager.GetRenderHandleReference(bufferAccess.buffer), bufferAccess.offset,
            bufferAccess.byteSize };
    }
    return {};
}

void AddMorphSubmesh(IRenderDataStoreMorph& dataStore, const MeshComponent::Submesh& submeshDesc,
    const MeshComponent& desc, RenderDataMorph::Submesh& submesh, const IRenderHandleComponentManager& bufferManager)
{
    submesh.vertexCount = submeshDesc.vertexCount;

    submesh.vertexBuffers[0U] = GetBuffer(submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_POS], bufferManager);
    submesh.vertexBuffers[1U] = GetBuffer(submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_NOR], bufferManager);
    submesh.vertexBuffers[2U] = GetBuffer(submeshDesc.bufferAccess[MeshComponent::Submesh::DM_VB_TAN], bufferManager);

    submesh.vertexBufferCount = 3U; // 3: count of vertex buffer

    submesh.morphTargetBuffer = GetBuffer(submeshDesc.morphTargetBuffer, bufferManager);

    submesh.morphTargetCount = submeshDesc.morphTargetCount;

    // don't touch submesh.activeTargets as it's same for each submesh
    dataStore.AddSubmesh(submesh);
}
} // namespace

MorphingSystem::MorphingSystem(IEcs& ecs)
    : active_(true), ecs_(ecs), dataStore_(nullptr), nodeManager_(*GetManager<INodeComponentManager>(ecs)),
      meshManager_(*GetManager<IMeshComponentManager>(ecs)), morphManager_(*GetManager<IMorphComponentManager>(ecs)),
      renderMeshManager_(*GetManager<IRenderMeshComponentManager>(ecs)),
      gpuHandleManager_(*GetManager<IRenderHandleComponentManager>(ecs)),
      MORPHING_SYSTEM_PROPERTIES(&properties_, ComponentMetadata)
{
    if (IEngine* engine = ecs_.GetClassFactory().GetInterface<IEngine>(); engine) {
        renderContext_ = GetInstance<IRenderContext>(*engine->GetInterface<IClassRegister>(), UID_RENDER_CONTEXT);
    }
}

MorphingSystem::~MorphingSystem() {}

void MorphingSystem::SetActive(bool state)
{
    active_ = state;
}

bool MorphingSystem::IsActive() const
{
    return active_;
}

void MorphingSystem::Initialize()
{
    if (renderContext_) {
        if (IRenderPreprocessorSystem* rps = GetSystem<IRenderPreprocessorSystem>(ecs_); rps) {
            const auto in = ScopedHandle<IRenderPreprocessorSystem::Properties>(rps->GetProperties());
            properties_.dataStoreName = in->dataStoreMorph;
        } else {
            CORE_LOG_E("DEPRECATED USAGE: RenderPreprocessorSystem not found. Add system to system graph.");
        }

        SetDataStore(renderContext_->GetRenderDataStoreManager(), properties_.dataStoreName);
    }
    ecs_.AddListener(static_cast<IEcs::EntityListener&>(*this));
    ecs_.AddListener(morphManager_, *this);
    nodeQuery_.SetEcsListenersEnabled(true);
    ComponentQuery::Operation operations[] = { { renderMeshManager_, ComponentQuery::Operation::Method::REQUIRE },
        { nodeManager_, ComponentQuery::Operation::Method::REQUIRE } };
    nodeQuery_.SetupQuery(morphManager_, operations, true);
}

void MorphingSystem::Uninitialize()
{
    ecs_.RemoveListener(static_cast<IEcs::EntityListener&>(*this));
    ecs_.RemoveListener(morphManager_, *this);
    nodeQuery_.SetEcsListenersEnabled(false);
}

void MorphingSystem::SetDataStore(IRenderDataStoreManager& manager, const string_view name)
{
    if (!name.empty()) {
        dataStore_ = refcnt_ptr<IRenderDataStoreMorph>(manager.GetRenderDataStore(properties_.dataStoreName.data()));
    } else {
        dataStore_.reset();
    }
}

string_view MorphingSystem::GetName() const
{
    return CORE3D_NS::GetName(this);
}

Uid MorphingSystem::GetUid() const
{
    return UID;
}

IPropertyHandle* MorphingSystem::GetProperties()
{
    return MORPHING_SYSTEM_PROPERTIES.GetData();
}

const IPropertyHandle* MorphingSystem::GetProperties() const
{
    return MORPHING_SYSTEM_PROPERTIES.GetData();
}

void MorphingSystem::SetProperties(const IPropertyHandle& data)
{
    if (data.Owner() != &MORPHING_SYSTEM_PROPERTIES) {
        return;
    }

    if (const auto in = ScopedHandle<const IMorphingSystem::Properties>(&data); in) {
        properties_.dataStoreName = in->dataStoreName;
        if (renderContext_) {
            SetDataStore(renderContext_->GetRenderDataStoreManager(), properties_.dataStoreName);
        }
    }
}

const IEcs& MorphingSystem::GetECS() const
{
    return ecs_;
}

void MorphingSystem::OnEntityEvent(const IEntityManager::EventType type, const array_view<const Entity> entities)
{
    if (type == IEntityManager::EventType::DESTROYED) {
        // remove entities that were destroyed
        for (const auto& e : entities) {
            dirty_.erase(e);
        }
    }
}

void MorphingSystem::OnComponentEvent(const ComponentListener::EventType type,
    const IComponentManager& componentManager, const array_view<const Entity> entities)
{
    if (type == ComponentListener::EventType::DESTROYED) {
        // morph component removed..
        for (const auto& e : entities) {
            reset_.push_back(e);
            dirty_.erase(e);
        }
    }
}

bool MorphingSystem::Morph(const MeshComponent& mesh, const MorphComponent& mc, bool dirty)
{
    // update active targets here once per mesh and the same will be reused for each submesh
    auto& activeTargets = currentMorphSubmesh_.activeTargets;
    activeTargets.clear();
    const auto& weights = mc.morphWeights;
    activeTargets.reserve(weights.size());
    for (size_t ti = 0; ti < weights.size(); ti++) {
        if (weights[ti] > 0.0f) {
            activeTargets.push_back({ static_cast<uint32_t>(ti), weights[ti] });
        }
    }

    // sort according to weight (highest influence first)
    std::sort(activeTargets.begin(), activeTargets.end(),
        [](auto const& lhs, auto const& rhs) { return (lhs.weight > rhs.weight); });

    if ((!activeTargets.empty()) || (dirty)) {
        for (const auto& submesh : mesh.submeshes) {
            if (submesh.morphTargetCount > 0U) {
                AddMorphSubmesh(*dataStore_, submesh, mesh, currentMorphSubmesh_, gpuHandleManager_);
            }
        }
    }
    return !activeTargets.empty();
}

bool MorphingSystem::Update(bool frameRenderingQueued, uint64_t, uint64_t)
{
    if (!active_) {
        return false;
    }
    if (dataStore_ == nullptr) {
#if (CORE3D_VALIDATION_ENABLED == 1)
        CORE_LOG_ONCE_W("ms_data_stores_not_found", "CORE3D_VALIDATION: morphing render data store not found");
#endif
        return false;
    }

    auto generation = morphManager_.GetGenerationCounter();
    if (generation == lastGeneration_) {
        // no change to components.
        return false;
    }
    lastGeneration_ = generation;

    nodeQuery_.Execute();

    RenderDataMorph::Submesh renderData;

    // Actual processing follows..
    for (const auto& row : nodeQuery_.GetResults()) {
        if (!nodeManager_.Read(row.components[NODE_INDEX])->effectivelyEnabled) {
            continue;
        }
        const ScopedHandle<const RenderMeshComponent> renderMeshData =
            renderMeshManager_.Read(row.components[RENDER_MESH_INDEX]);
        if (const ScopedHandle<const MeshComponent> meshData = meshManager_.Read(renderMeshData->mesh)) {
            const ScopedHandle<const MorphComponent> morphData = morphManager_.Read(row.components[MORPH_INDEX]);
            dirty_[row.entity] = Morph(*meshData, *morphData, dirty_[row.entity]);
        }
    }

    // no active targets for the removed morph components
    currentMorphSubmesh_.activeTargets.clear();
    for (const auto& id : reset_) {
        // reset the render mesh of removed component..
        const ScopedHandle<const RenderMeshComponent> renderMeshData = renderMeshManager_.Read(id);
        if (!renderMeshData) {
            continue;
        }
        const ScopedHandle<const MeshComponent> meshData = meshManager_.Read(renderMeshData->mesh);
        if (!meshData) {
            continue;
        }
        for (const auto& submesh : meshData->submeshes) {
            if (submesh.morphTargetCount > 0U) {
                AddMorphSubmesh(*dataStore_, submesh, *meshData, currentMorphSubmesh_, gpuHandleManager_);
            }
        }
    }
    reset_.clear();

    return true;
}

ISystem* IMorphingSystemInstance(IEcs& ecs)
{
    return new MorphingSystem(ecs);
}
void IMorphingSystemDestroy(ISystem* instance)
{
    delete static_cast<MorphingSystem*>(instance);
}

CORE3D_END_NAMESPACE()
