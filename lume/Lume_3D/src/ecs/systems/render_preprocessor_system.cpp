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

#include "render_preprocessor_system.h"

#include <PropertyTools/property_api_impl.inl>
#include <PropertyTools/property_macros.h>
#include <algorithm>

#include <3d/ecs/components/joint_matrices_component.h>
#include <3d/ecs/components/layer_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/skin_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/implementation_uids.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <3d/render/intf_render_data_store_morph.h>
#include <3d/util/intf_picking.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/perf/cpu_perf_scope.h>
#include <core/perf/intf_performance_data_manager.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/scoped_handle.h>
#include <render/implementation_uids.h>

#include "util/component_util_functions.h"

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(RENDER_NS::IRenderDataStoreManager*);
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using namespace RENDER_NS;
using namespace BASE_NS;
using namespace CORE_NS;

BEGIN_PROPERTY(IRenderPreprocessorSystem::Properties, ComponentMetadata)
    DECL_PROPERTY2(IRenderPreprocessorSystem::Properties, dataStoreScene, "dataStoreScene", 0)
    DECL_PROPERTY2(IRenderPreprocessorSystem::Properties, dataStoreCamera, "dataStoreCamera", 0)
    DECL_PROPERTY2(IRenderPreprocessorSystem::Properties, dataStoreLight, "dataStoreLight", 0)
    DECL_PROPERTY2(IRenderPreprocessorSystem::Properties, dataStoreMaterial, "dataStoreMaterial", 0)
    DECL_PROPERTY2(IRenderPreprocessorSystem::Properties, dataStoreMorph, "dataStoreMorph", 0)
    DECL_PROPERTY2(IRenderPreprocessorSystem::Properties, dataStorePrefix, "", 0)
END_PROPERTY();

namespace {
constexpr const auto RMC = 0U;
constexpr const auto NC = 1U;
constexpr const auto WMC = 2U;
constexpr const auto LC = 3U;
constexpr const auto JMC = 4U;
constexpr const auto SC = 5U;

struct SceneBoundingVolumeHelper {
    BASE_NS::Math::Vec3 sumOfSubmeshPoints { 0.0f, 0.0f, 0.0f };
    uint32_t submeshCount { 0 };

    BASE_NS::Math::Vec3 minAABB { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max() };
    BASE_NS::Math::Vec3 maxAABB { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max() };
};

template<typename DataStoreType>
inline auto CreateIfNeeded(IRenderDataStoreManager& manager, DataStoreType* dataStore, string_view dataStoreName)
{
    if (!dataStore || dataStore->GetName() != dataStoreName) {
        dataStore = static_cast<DataStoreType*>(manager.Create(DataStoreType::UID, dataStoreName.data()));
    }
    return dataStore;
}

vector<RenderPreprocessorSystem::MaterialProperties> GatherMaterialProperties(
    IMaterialComponentManager& materialManager)
{
    vector<RenderPreprocessorSystem::MaterialProperties> materialProperties;
    const auto materials = static_cast<IComponentManager::ComponentId>(materialManager.GetComponentCount());
    materialProperties.reserve(materials);
    for (auto id = 0U; id < materials; ++id) {
        if (auto materialHandle = materialManager.Read(id); materialHandle) {
            materialProperties.push_back({
                materialManager.GetEntity(id),
                (materialHandle->extraRenderingFlags & MaterialComponent::ExtraRenderingFlagBits::DISABLE_BIT) ==
                    MaterialComponent::ExtraRenderingFlagBits::DISABLE_BIT,
                ((materialHandle->extraRenderingFlags &
                     MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT) ==
                    MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT) ||
                    !EntityUtil::IsValid(materialHandle->materialShader.shader),
                (materialHandle->materialLightingFlags & MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT) ==
                    MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT,
            });
        }
    }
    std::sort(materialProperties.begin(), materialProperties.end(),
        [](const RenderPreprocessorSystem::MaterialProperties& lhs,
            const RenderPreprocessorSystem::MaterialProperties& rhs) { return (lhs.material.id < rhs.material.id); });
    return materialProperties;
}
} // namespace

RenderPreprocessorSystem::RenderPreprocessorSystem(IEcs& ecs)
    : ecs_(ecs), jointMatricesManager_(GetManager<IJointMatricesComponentManager>(ecs)),
      layerManager_(GetManager<ILayerComponentManager>(ecs)),
      materialManager_(GetManager<IMaterialComponentManager>(ecs)),
      meshManager_(GetManager<IMeshComponentManager>(ecs)), nodeManager_(GetManager<INodeComponentManager>(ecs)),
      renderMeshManager_(GetManager<IRenderMeshComponentManager>(ecs)),
      skinManager_(GetManager<ISkinComponentManager>(ecs)),
      worldMatrixManager_(GetManager<IWorldMatrixComponentManager>(ecs)),
      RENDER_PREPROCESSOR_SYSTEM_PROPERTIES(&properties_, array_view(ComponentMetadata))
{
    if (IEngine* engine = ecs_.GetClassFactory().GetInterface<IEngine>(); engine) {
        renderContext_ = GetInstance<IRenderContext>(*engine->GetInterface<IClassRegister>(), UID_RENDER_CONTEXT);
        if (renderContext_) {
            picking_ = GetInstance<IPicking>(*renderContext_->GetInterface<IClassRegister>(), UID_PICKING);
            graphicsContext_ =
                GetInstance<IGraphicsContext>(*renderContext_->GetInterface<IClassRegister>(), UID_GRAPHICS_CONTEXT);
        }
    }
}

RenderPreprocessorSystem::~RenderPreprocessorSystem()
{
    if (IEngine* engine = ecs_.GetClassFactory().GetInterface<IEngine>(); engine) {
        // check that render context is still alive
        if (auto renderContext =
                GetInstance<IRenderContext>(*engine->GetInterface<IClassRegister>(), UID_RENDER_CONTEXT);
            renderContext) {
            IRenderDataStoreManager& rdsMgr = renderContext->GetRenderDataStoreManager();
            if (dsScene_) {
                rdsMgr.Destroy(IRenderDataStoreDefaultScene::UID, dsScene_);
            }
            if (dsCamera_) {
                rdsMgr.Destroy(IRenderDataStoreDefaultCamera::UID, dsCamera_);
            }
            if (dsLight_) {
                rdsMgr.Destroy(IRenderDataStoreDefaultLight::UID, dsLight_);
            }
            if (dsMaterial_) {
                rdsMgr.Destroy(IRenderDataStoreDefaultMaterial::UID, dsMaterial_);
            }
            if (dsMorph_) {
                rdsMgr.Destroy(IRenderDataStoreMorph::UID, dsMorph_);
            }
        }
    }
}

void RenderPreprocessorSystem::SetActive(bool state)
{
    active_ = state;
}

bool RenderPreprocessorSystem::IsActive() const
{
    return active_;
}

string_view RenderPreprocessorSystem::GetName() const
{
    return CORE3D_NS::GetName(this);
}

Uid RenderPreprocessorSystem::GetUid() const
{
    return UID;
}

IPropertyHandle* RenderPreprocessorSystem::GetProperties()
{
    return RENDER_PREPROCESSOR_SYSTEM_PROPERTIES.GetData();
}

const IPropertyHandle* RenderPreprocessorSystem::GetProperties() const
{
    return RENDER_PREPROCESSOR_SYSTEM_PROPERTIES.GetData();
}

void RenderPreprocessorSystem::SetProperties(const IPropertyHandle& data)
{
    if (data.Owner() != &RENDER_PREPROCESSOR_SYSTEM_PROPERTIES) {
        return;
    }
    if (const auto in = ScopedHandle<const IRenderPreprocessorSystem::Properties>(&data); in) {
        properties_.dataStoreScene = in->dataStoreScene;
        properties_.dataStoreCamera = in->dataStoreCamera;
        properties_.dataStoreLight = in->dataStoreLight;
        properties_.dataStoreMaterial = in->dataStoreMaterial;
        properties_.dataStoreMorph = in->dataStoreMorph;
        if (renderContext_) {
            SetDataStorePointers(renderContext_->GetRenderDataStoreManager());
        }
    }
}

void RenderPreprocessorSystem::SetDataStorePointers(IRenderDataStoreManager& manager)
{
    // creates own data stores based on names
    dsScene_ = CreateIfNeeded(manager, dsScene_, properties_.dataStoreScene);
    dsCamera_ = CreateIfNeeded(manager, dsCamera_, properties_.dataStoreCamera);
    dsLight_ = CreateIfNeeded(manager, dsLight_, properties_.dataStoreLight);
    dsMaterial_ = CreateIfNeeded(manager, dsMaterial_, properties_.dataStoreMaterial);
    dsMorph_ = CreateIfNeeded(manager, dsMorph_, properties_.dataStoreMorph);
}

void RenderPreprocessorSystem::CalculateSceneBounds()
{
    SceneBoundingVolumeHelper helper;

    for (const auto& i : renderMeshAabbs_) {
        // the mesh aabb will have default value if all the submeshes were skipped. in the default value min > max.
        if (i.second.meshAabb.min.x < i.second.meshAabb.max.x) {
            helper.sumOfSubmeshPoints += (i.second.meshAabb.min + i.second.meshAabb.max) / 2.f;
            helper.minAABB = Math::min(helper.minAABB, i.second.meshAabb.min);
            helper.maxAABB = Math::max(helper.maxAABB, i.second.meshAabb.max);
            ++helper.submeshCount;
        }
    }

    if (helper.submeshCount == 0) {
        boundingSphere_.radius = 0.0f;
        boundingSphere_.center = Math::Vec3();
    } else {
        const auto boundingSpherePosition = helper.sumOfSubmeshPoints / static_cast<float>(helper.submeshCount);

        const float radMin = Math::Magnitude(boundingSpherePosition - helper.minAABB);
        const float radMax = Math::Magnitude(helper.maxAABB - boundingSpherePosition);
        const float boundingSphereRadius = Math::max(radMin, radMax);

        // Compensate jitter and adjust scene bounding sphere only if change in bounds is meaningful.
        if (boundingSphere_.radius > 0.0f) {
            // Calculate distance to new bounding sphere origin from current sphere.
            const float pointDistance = Math::Magnitude(boundingSpherePosition - boundingSphere_.center);
            // Calculate distance to edge of new bounding sphere from current sphere origin.
            const float sphereEdgeDistance = pointDistance + boundingSphereRadius;

            // Calculate step size for adjustment, use 10% granularity from current bounds.
            constexpr float granularityPct = 0.10f;
            const float granularity = boundingSphere_.radius * granularityPct;

            // Calculate required change of size, in order to fit new sphere inside current bounds.
            const float radDifference = sphereEdgeDistance - boundingSphere_.radius;
            const float posDifference = Math::Magnitude(boundingSpherePosition - boundingSphere_.center);
            // We need to adjust only if the change is bigger than the step size.
            if ((Math::abs(radDifference) > granularity) || (posDifference > granularity)) {
                // Calculate how many steps we need to change and in to which direction.
                const float radAmount = ceil((boundingSphereRadius - boundingSphere_.radius) / granularity);
                const int32_t posAmount = (int32_t)ceil(posDifference / granularity);
                if ((radAmount != 0.f) || (posAmount != 0)) {
                    // Update size and position of the bounds.
                    boundingSphere_.center = boundingSpherePosition;
                    boundingSphere_.radius = boundingSphere_.radius + (radAmount * granularity);
                }
            }
        } else {
            // No existing bounds, start with new values.
            boundingSphere_.radius = boundingSphereRadius;
            boundingSphere_.center = boundingSpherePosition;
        }
    }
}

void RenderPreprocessorSystem::GatherSortData()
{
    const auto& results = renderableQuery_.GetResults();
    const auto renderMeshes = static_cast<IComponentManager::ComponentId>(results.size());
    meshComponents_.clear();
    meshComponents_.reserve(renderMeshes);
    renderMeshAabbs_.clear();
    renderMeshAabbs_.reserve(renderMeshes);

    vector<bool> disabled;
    vector<bool> shadowCaster;
    for (const auto& row : results) {
        // TODO this list needs to update only when render mesh, node, or layer component have changed
        const bool effectivelyEnabled = nodeManager_->Read(row.components[NC])->effectivelyEnabled;
        const uint64_t layerMask = !row.IsValidComponentId(LC) ? LayerConstants::DEFAULT_LAYER_MASK
                                                               : layerManager_->Get(row.components[LC]).layerMask;
        if (effectivelyEnabled && (layerMask != LayerConstants::NONE_LAYER_MASK)) {
            auto renderMeshHandle = renderMeshManager_->Read(row.components[RMC]);
            // gather the submesh world aabbs
            if (const auto meshData = meshManager_->Read(renderMeshHandle->mesh); meshData) {
                // TODO render system doesn't necessarily have to read the render mesh components. preprocessor
                // could offer two lists of mesh+world, one containing the render mesh batch style meshes and second
                // containing regular meshes
                const auto renderMeshEntity = renderMeshManager_->GetEntity(row.components[RMC]);
                auto& data = renderMeshAabbs_[renderMeshEntity];
                auto& meshAabb = data.meshAabb;
                meshAabb = {};
                auto& aabbs = data.submeshAabbs;
                aabbs.clear();

                // check MaterialComponent::DISABLE_BIT and discard those, check
                // MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT and don't include them in scene bounding.
                disabled.clear();
                disabled.resize(meshData->submeshes.size());
                shadowCaster.clear();
                shadowCaster.resize(meshData->submeshes.size());
                auto submeshIdx = 0U;
                bool allowInstancing = true;
                for (const auto& submesh : meshData->submeshes) {
                    if (EntityUtil::IsValid(submesh.material)) {
                        if (auto pos = std::lower_bound(materialProperties_.cbegin(), materialProperties_.cend(),
                                submesh.material,
                                [](const MaterialProperties& element, const Entity& value) {
                                    return element.material.id < value.id;
                                });
                            (pos != materialProperties_.cend()) && (pos->material == submesh.material)) {
                            disabled[submeshIdx] = pos->disabled;
                            allowInstancing = allowInstancing && pos->allowInstancing;
                            shadowCaster[submeshIdx] = pos->shadowCaster;
                        }
                    } else {
                        // assuming MaterialComponent::materialLightingFlags default value includes
                        // SHADOW_CASTER_BIT
                        shadowCaster[submeshIdx] = true;
                    }
                    for (const auto additionalMaterial : submesh.additionalMaterials) {
                        if (EntityUtil::IsValid(additionalMaterial)) {
                            if (auto pos = std::lower_bound(materialProperties_.cbegin(), materialProperties_.cend(),
                                    additionalMaterial,
                                    [](const MaterialProperties& element, const Entity& value) {
                                        return element.material.id < value.id;
                                    });
                                (pos != materialProperties_.cend()) && (pos->material == additionalMaterial)) {
                                disabled[submeshIdx] = pos->disabled;
                                allowInstancing = allowInstancing && pos->allowInstancing;
                                shadowCaster[submeshIdx] = pos->shadowCaster;
                            }
                        } else {
                            // assuming MaterialComponent::materialLightingFlags default value includes
                            // SHADOW_CASTER_BIT
                            shadowCaster[submeshIdx] = true;
                        }
                    }
                    ++submeshIdx;
                }

                if (std::any_of(disabled.cbegin(), disabled.cend(), [](const bool disabled) { return !disabled; })) {
                    bool hasJoints = row.IsValidComponentId(JMC);
                    if (hasJoints) {
                        // TODO this needs to happen only when joint matrices have changed
                        auto jointMatricesHandle = jointMatricesManager_->Read(row.components[JMC]);
                        hasJoints = (jointMatricesHandle->count > 0U);
                        if (hasJoints) {
                            aabbs.push_back(
                                Aabb { jointMatricesHandle->jointsAabbMin, jointMatricesHandle->jointsAabbMax });
                            meshAabb.min = Math::min(meshAabb.min, jointMatricesHandle->jointsAabbMin);
                            meshAabb.max = Math::max(meshAabb.max, jointMatricesHandle->jointsAabbMax);
                        }
                    }
                    if (!hasJoints) {
                        submeshIdx = 0U;
                        const auto& world = worldMatrixManager_->Read(row.components[WMC])->matrix;
                        for (const auto& submesh : meshData->submeshes) {
                            // TODO this needs to happen only when world matrix, or mesh component have changed
                            if (disabled[submeshIdx]) {
                                aabbs.push_back({});
                            } else {
                                const MinAndMax mam = picking_->GetWorldAABB(world, submesh.aabbMin, submesh.aabbMax);
                                aabbs.push_back({ mam.minAABB, mam.maxAABB });
                                if (shadowCaster[submeshIdx]) {
                                    meshAabb.min = Math::min(meshAabb.min, mam.minAABB);
                                    meshAabb.max = Math::max(meshAabb.max, mam.maxAABB);
                                }
                            }
                            ++submeshIdx;
                        }
                    }

                    auto skin = (row.IsValidComponentId(SC)) ? skinManager_->Read(row.components[SC])->skin : Entity {};
                    meshComponents_.push_back({ row.components[RMC], renderMeshHandle->mesh,
                        renderMeshHandle->renderMeshBatch, skin, allowInstancing });
                }
            }
        }
    }
}

const IEcs& RenderPreprocessorSystem::GetECS() const
{
    return ecs_;
}

array_view<const Entity> RenderPreprocessorSystem::GetRenderBatchMeshEntities() const
{
    return renderBatchComponents_;
}

array_view<const Entity> RenderPreprocessorSystem::GetInstancingAllowedEntities() const
{
    return instancingAllowed_;
}

array_view<const Entity> RenderPreprocessorSystem::GetInstancingDisabledEntities() const
{
    return rest_;
}

RenderPreprocessorSystem::Aabb RenderPreprocessorSystem::GetRenderMeshAabb(Entity renderMesh) const
{
    if (auto pos = renderMeshAabbs_.find(renderMesh); pos != renderMeshAabbs_.cend()) {
        return pos->second.meshAabb;
    }
    return {};
}

array_view<const RenderPreprocessorSystem::Aabb> RenderPreprocessorSystem::GetRenderMeshAabbs(Entity renderMesh) const
{
    if (auto pos = renderMeshAabbs_.find(renderMesh); pos != renderMeshAabbs_.cend()) {
        return pos->second.submeshAabbs;
    }
    return {};
}

RenderPreprocessorSystem::Sphere RenderPreprocessorSystem::GetBoundingSphere() const
{
    return boundingSphere_;
}

void RenderPreprocessorSystem::Initialize()
{
    if (graphicsContext_ && renderContext_) {
        SetDataStorePointers(renderContext_->GetRenderDataStoreManager());
    }
    if (renderMeshManager_ && nodeManager_ && layerManager_) {
        const ComponentQuery::Operation operations[] = {
            { *nodeManager_, ComponentQuery::Operation::REQUIRE },
            { *worldMatrixManager_, ComponentQuery::Operation::REQUIRE },
            { *layerManager_, ComponentQuery::Operation::OPTIONAL },
            { *jointMatricesManager_, ComponentQuery::Operation::OPTIONAL },
            { *skinManager_, ComponentQuery::Operation::OPTIONAL },
        };
        renderableQuery_.SetEcsListenersEnabled(true);
        renderableQuery_.SetupQuery(*renderMeshManager_, operations, false);
    }
}

bool RenderPreprocessorSystem::Update(bool frameRenderingQueued, uint64_t totalTime, uint64_t deltaTime)
{
    if (!active_) {
        return false;
    }

    renderableQuery_.Execute();

    const auto layerGen = layerManager_->GetGenerationCounter();
    const auto materialGen = materialManager_->GetGenerationCounter();
    const auto meshGen = meshManager_->GetGenerationCounter();
    const auto nodeGen = nodeManager_->GetGenerationCounter();
    const auto renderMeshGen = renderMeshManager_->GetGenerationCounter();
    const auto worldMatrixGen = worldMatrixManager_->GetGenerationCounter();
    if ((layerGeneration_ == layerGen) && (materialGeneration_ == materialGen) && (meshGeneration_ == meshGen) &&
        (nodeGeneration_ == nodeGen) && (renderMeshGeneration_ == renderMeshGen) &&
        (worldMatrixGeneration_ == worldMatrixGen)) {
        return false;
    }

    layerGeneration_ = layerGen;
    materialGeneration_ = materialGen;
    meshGeneration_ = meshGen;
    nodeGeneration_ = nodeGen;
    renderMeshGeneration_ = renderMeshGen;
    worldMatrixGeneration_ = worldMatrixGen;

    materialProperties_ = GatherMaterialProperties(*materialManager_);

    // gather which mesh is used by each render mesh component.
    GatherSortData();

    // reorder the list so that entities with render mesh batch are first sorted by entity id
    const auto noBatchComponent = std::partition(meshComponents_.begin(), meshComponents_.end(),
        [](const SortData& value) { return EntityUtil::IsValid(value.batch); });
    std::sort(meshComponents_.begin(), noBatchComponent,
        [](const SortData& lhs, const SortData& rhs) { return lhs.renderMeshId < rhs.renderMeshId; });

    // next are meshes that can be instanced sorted by mesh id, skin id and entity id
    auto noInstancing = std::partition(
        noBatchComponent, meshComponents_.end(), [](const SortData& value) { return value.allowInstancing; });
    std::sort(noBatchComponent, noInstancing, [](const SortData& lhs, const SortData& rhs) {
        if (lhs.mesh.id < rhs.mesh.id) {
            return true;
        }
        if (lhs.mesh.id > rhs.mesh.id) {
            return false;
        }
        if (lhs.skin.id < rhs.skin.id) {
            return true;
        }
        if (lhs.skin.id > rhs.skin.id) {
            return false;
        }
        return lhs.renderMeshId < rhs.renderMeshId;
    });

    // move entities, which could be instanced but have only one instance, to the end
    {
        auto begin = reverse_iterator(noInstancing);
        auto end = reverse_iterator(noBatchComponent);
        for (auto it = begin; it != end;) {
            auto pos = std::adjacent_find(it, end, [](const SortData& lhs, const SortData& rhs) {
                return (lhs.mesh.id != rhs.mesh.id) || (lhs.skin.id != rhs.skin.id);
            });
            if (pos != end) {
                // pos points to the first element where comparison failed i.e. pos is the last of previous batch
                // and pos+1 is the first of the next batch
                ++pos;
            }
            // if the batch size is 1, move the single entry to the end
            if (const auto batchSize = pos - it; batchSize == 1U) {
                auto i = std::rotate(pos.base(), pos.base() + 1, begin.base());
                begin = reverse_iterator(i);
            }

            it = pos;
        }
        noInstancing = begin.base();
    }

    // finally sort the non instanced by entity id
    std::sort(noInstancing, meshComponents_.end(),
        [](const SortData& lhs, const SortData& rhs) { return lhs.renderMeshId < rhs.renderMeshId; });

    // list the entities with render mesh components in sorted order.
    renderMeshComponents_.clear();
    const auto meshCount = meshComponents_.size();
    renderMeshComponents_.reserve(meshCount);
    std::transform(meshComponents_.cbegin(), meshComponents_.cend(), std::back_inserter(renderMeshComponents_),
        [renderMeshManager = renderMeshManager_](
            const SortData& data) { return renderMeshManager->GetEntity(data.renderMeshId); });
    renderBatchComponents_ =
        array_view(renderMeshComponents_.data(), static_cast<size_t>(noBatchComponent - meshComponents_.begin()));
    instancingAllowed_ = array_view(renderMeshComponents_.data() + renderBatchComponents_.size(),
        static_cast<size_t>(noInstancing - noBatchComponent));
    rest_ = array_view(renderMeshComponents_.data() + renderBatchComponents_.size() + instancingAllowed_.size(),
        static_cast<size_t>(meshComponents_.end() - noInstancing));

    CalculateSceneBounds();

    return true;
}

void RenderPreprocessorSystem::Uninitialize()
{
    renderableQuery_.SetEcsListenersEnabled(false);
}

ISystem* IRenderPreprocessorSystemInstance(IEcs& ecs)
{
    return new RenderPreprocessorSystem(ecs);
}

void IRenderPreprocessorSystemDestroy(ISystem* instance)
{
    delete static_cast<RenderPreprocessorSystem*>(instance);
}
CORE3D_END_NAMESPACE()
