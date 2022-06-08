/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "skinning_system.h"

#include <PropertyTools/property_api_impl.inl>
#include <algorithm>
#include <charconv>
#include <limits>

#include <3d/ecs/components/joint_matrices_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/skin_component.h>
#include <3d/ecs/components/skin_ibm_component.h>
#include <3d/ecs/components/skin_joints_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/implementation_uids.h>
#include <3d/util/intf_picking.h>
#include <base/containers/fixed_string.h>
#include <base/math/matrix_util.h>
#include <core/ecs/intf_ecs.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_plugin_register.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>

#include "ecs/components/previous_joint_matrices_component.h"
#include "ecs/systems/node_system.h"
#include "util/string_util.h"

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace {
constexpr auto SKIN_INDEX = 0U;
constexpr auto SKIN_JOINTS_INDEX = 1U;
constexpr auto JOINT_MATS_INDEX = 2U;
constexpr auto PREV_JOINT_MATS_INDEX = 3U;
constexpr auto RENDER_MESH_INDEX = 4U;

void UpdateJointBounds(IPicking& pick, const array_view<const float>& jointBoundsData,
    const Math::Mat4X4& skinEntityWorld, JointMatricesComponent& jointMatrices)
{
    const size_t jointBoundsDataSize = jointBoundsData.size();
    const size_t boundsCount = jointBoundsDataSize / 6;

    CORE_ASSERT(jointBoundsData.size() % 6 == 0); // 6: should be multiple of 6
    CORE_ASSERT(jointMatrices.count >= boundsCount);

    constexpr float maxFloat = std::numeric_limits<float>::max();
    constexpr Math::Vec3 minDefault(maxFloat, maxFloat, maxFloat);
    constexpr Math::Vec3 maxDefault(-maxFloat, -maxFloat, -maxFloat);

    jointMatrices.jointsAabbMin = minDefault;
    jointMatrices.jointsAabbMax = maxDefault;

    for (size_t j = 0; j < jointMatrices.count; j++) {
        if (j < boundsCount) {
            // Bounds that don't have any vertices will be filled with maxFloat.
            const float* boundsData = &jointBoundsData[j * 6];
            if (*boundsData != maxFloat) {
                const Math::Vec3 min(boundsData);
                const Math::Vec3 max(boundsData + 3);
                const Math::Mat4X4& bbWorld = skinEntityWorld * jointMatrices.jointMatrices[j];
                const auto mam = pick.GetWorldAABB(bbWorld, min, max);
                // Only use bounding box if it's size is > ~zero.
                if (Math::Distance2(mam.minAABB, mam.maxAABB) > Math::EPSILON) {
                    jointMatrices.jointAabbMinArray[j] = mam.minAABB;
                    jointMatrices.jointAabbMaxArray[j] = mam.maxAABB;
                    // Update the combined min/max for all joints.
                    jointMatrices.jointsAabbMin = Math::min(jointMatrices.jointsAabbMin, mam.minAABB);
                    jointMatrices.jointsAabbMax = Math::max(jointMatrices.jointsAabbMax, mam.maxAABB);
                    continue;
                }
            }
        }

        // This joint is not referenced by any vertex or the bounding box size is zero.
        jointMatrices.jointAabbMinArray[j] = minDefault;
        jointMatrices.jointAabbMaxArray[j] = maxDefault;
    }
}

IPicking* GetPicking(IEcs& ecs)
{
    if (IEngine* engine = ecs.GetClassFactory().GetInterface<IEngine>(); engine) {
        if (auto renderContext =
                GetInstance<IRenderContext>(*engine->GetInterface<IClassRegister>(), UID_RENDER_CONTEXT);
            renderContext) {
            return GetInstance<IPicking>(*renderContext->GetInterface<IClassRegister>(), UID_PICKING);
        }
    }
    return nullptr;
}
} // namespace

SkinningSystem::SkinningSystem(IEcs& ecs)
    : active_(true), ecs_(ecs), picking_(*GetPicking(ecs)), skinManager_(*GetManager<ISkinComponentManager>(ecs)),
      skinIbmManager_(*GetManager<ISkinIbmComponentManager>(ecs)),
      skinJointsManager_(*GetManager<ISkinJointsComponentManager>(ecs)),
      jointMatricesManager_(*GetManager<IJointMatricesComponentManager>(ecs)),
      previousJointMatricesManager_(*GetManager<IPreviousJointMatricesComponentManager>(ecs)),
      worldMatrixManager_(*GetManager<IWorldMatrixComponentManager>(ecs)),
      nodeManager_(*GetManager<INodeComponentManager>(ecs)),
      renderMeshManager_(*GetManager<IRenderMeshComponentManager>(ecs)),
      meshManager_(*GetManager<IMeshComponentManager>(ecs))
{}

void SkinningSystem::SetActive(bool state)
{
    active_ = state;
}

bool SkinningSystem::IsActive() const
{
    return active_;
}

void SkinningSystem::Initialize()
{
    nodeSystem_ = GetSystem<INodeSystem>(ecs_);
    {
        const ComponentQuery::Operation operations[] = {
            { skinJointsManager_, ComponentQuery::Operation::REQUIRE },
            { jointMatricesManager_, ComponentQuery::Operation::REQUIRE },
            { previousJointMatricesManager_, ComponentQuery::Operation::OPTIONAL },
            { renderMeshManager_, ComponentQuery::Operation::OPTIONAL },
        };
        componentQuery_.SetEcsListenersEnabled(true);
        componentQuery_.SetupQuery(skinManager_, operations);
    }
}

string_view SkinningSystem::GetName() const
{
    return CORE3D_NS::GetName(this);
}

Uid SkinningSystem::GetUid() const
{
    return UID;
}

void SkinningSystem::Uninitialize()
{
    componentQuery_.SetEcsListenersEnabled(false);
}

IPropertyHandle* SkinningSystem::GetProperties()
{
    return SKINNING_SYSTEM_PROPERTIES.GetData();
}

const IPropertyHandle* SkinningSystem::GetProperties() const
{
    return SKINNING_SYSTEM_PROPERTIES.GetData();
}

void SkinningSystem::SetProperties(const IPropertyHandle&) {}

const IEcs& SkinningSystem::GetECS() const
{
    return ecs_;
}

void SkinningSystem::UpdateJointTransformations(bool isEnabled, const array_view<Entity const>& jointEntities,
    const array_view<Math::Mat4X4 const>& ibms, JointMatricesComponent& jointMatrices,
    const Math::Mat4X4& skinEntityWorldInverse)
{
    auto matrices = array_view<Math::Mat4X4>(
        jointMatrices.jointMatrices, std::min(jointMatrices.count, countof(jointMatrices.jointMatrices)));

    std::transform(jointEntities.begin(), jointEntities.end(), ibms.begin(), matrices.begin(),
        [&worldMatrixManager = worldMatrixManager_, skinEntityWorldInverse, isEnabled](
            auto const& jointEntity, auto const& ibm) {
            if (isEnabled) {
                if (const auto worldMatrixId = worldMatrixManager.GetComponentId(jointEntity);
                    worldMatrixId != IComponentManager::INVALID_COMPONENT_ID) {
                    auto const& jointGlobal = worldMatrixManager.Get(worldMatrixId).matrix;
                    auto const jointMatrix = skinEntityWorldInverse * jointGlobal * ibm;
                    return jointMatrix;
                }
            }

            return Math::Mat4X4 { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };
        });
}

void SkinningSystem::UpdateSkin(const ComponentQuery::ResultRow& row)
{
    bool isEnabled = true;
    Math::Mat4X4 skinEntityWorld(1.0f);
    Math::Mat4X4 skinEntityWorldInverse(1.0f);

    const SkinComponent skinComponent = skinManager_.Get(row.components[SKIN_INDEX]);
    if (const auto worldMatrixId = worldMatrixManager_.GetComponentId(skinComponent.skinRoot);
        worldMatrixId != IComponentManager::INVALID_COMPONENT_ID) {
        isEnabled = nodeManager_.Get(skinComponent.skinRoot).effectivelyEnabled;
        skinEntityWorld = worldMatrixManager_.Get(worldMatrixId).matrix;
        skinEntityWorldInverse = Math::Inverse(skinEntityWorld);
    }

    const auto skinIbmHandle = skinIbmManager_.Read(skinComponent.skin);
    if (!skinIbmHandle) {
        auto const onceId = to_hex(row.entity.id);
        CORE_LOG_ONCE_W(onceId.c_str(), "Invalid skin resource for entity %s", onceId.c_str());
        return;
    }

    auto const skinJointsHandle = skinJointsManager_.Read(row.components[SKIN_JOINTS_INDEX]);
    auto const jointEntities = array_view<Entity const>(
        skinJointsHandle->jointEntities, std::min(skinJointsHandle->count, countof(skinJointsHandle->jointEntities)));

    auto const& ibmMatrices = skinIbmHandle->matrices;
    if (jointEntities.size() != ibmMatrices.size()) {
        auto const onceId = to_hex(row.entity.id);
        CORE_LOG_ONCE_W(onceId.c_str(), "Entity (%zu) and description (%zu) counts don't match for entity %s",
            jointEntities.size(), ibmMatrices.size(), onceId.c_str());
        return;
    }

    auto jointMatricesHandle = jointMatricesManager_.Write(row.components[JOINT_MATS_INDEX]);
    auto& jointMatrices = *jointMatricesHandle;
    jointMatrices.count = jointEntities.size();

    UpdateJointTransformations(isEnabled, jointEntities, ibmMatrices, jointMatrices, skinEntityWorldInverse);
    if (row.IsValidComponentId(3u)) {
        if (const auto renderMeshHandle = renderMeshManager_.Read(row.components[RENDER_MESH_INDEX]);
            renderMeshHandle) {
            const RenderMeshComponent& renderMeshComponent = *renderMeshHandle;
            if (const auto meshHandle = meshManager_.Read(renderMeshComponent.mesh); meshHandle) {
                auto& mesh = *meshHandle;
                UpdateJointBounds(picking_, mesh.jointBounds, skinEntityWorld, jointMatrices);
            }
        }
    }
}

bool SkinningSystem::Update(bool frameRenderingQueued, uint64_t, uint64_t)
{
    if (!active_) {
        return false;
    }

    componentQuery_.Execute();

    // copy joint matrices if they have changed
    bool missingPrevJointMatrices = false;
    if (jointMatricesGeneration_ != jointMatricesManager_.GetGenerationCounter()) {
        jointMatricesGeneration_ = jointMatricesManager_.GetGenerationCounter();

        for (const auto& row : componentQuery_.GetResults()) {
            const bool hasPrev = row.IsValidComponentId(PREV_JOINT_MATS_INDEX);
            if (hasPrev && row.IsValidComponentId(JOINT_MATS_INDEX)) {
                auto prev = previousJointMatricesManager_.Write(row.components[PREV_JOINT_MATS_INDEX]);
                auto current = jointMatricesManager_.Read(row.components[JOINT_MATS_INDEX]);
                prev->count = current->count;
                std::copy(current->jointMatrices, current->jointMatrices + current->count, prev->jointMatrices);
            } else if (!hasPrev) {
                missingPrevJointMatrices = true;
            }
        }
    }

    if (worldMatrixGeneration_ == worldMatrixManager_.GetGenerationCounter()) {
        return false;
    }

    worldMatrixGeneration_ = worldMatrixManager_.GetGenerationCounter();

    for (const ComponentQuery::ResultRow& row : componentQuery_.GetResults()) {
        UpdateSkin(row);
    }

    if (missingPrevJointMatrices) {
        for (const auto& row : componentQuery_.GetResults()) {
            // Create missing PreviousJointMatricesComponents and initialize with current.
            if (!row.IsValidComponentId(PREV_JOINT_MATS_INDEX) && row.IsValidComponentId(JOINT_MATS_INDEX)) {
                previousJointMatricesManager_.Create(row.entity);
                auto prev = previousJointMatricesManager_.Write(row.entity);
                auto current = jointMatricesManager_.Read(row.components[JOINT_MATS_INDEX]);
                prev->count = current->count;
                std::copy(current->jointMatrices, current->jointMatrices + current->count, prev->jointMatrices);
            }
        }
    }

    return true;
}

void SkinningSystem::CreateInstance(
    Entity const& skinIbmEntity, array_view<const Entity> const& joints, Entity const& entity, Entity const& skeleton)
{
    if (!EntityUtil::IsValid(skinIbmEntity) ||
        !std::all_of(joints.begin(), joints.end(), [](const Entity& entity) { return EntityUtil::IsValid(entity); }) ||
        !EntityUtil::IsValid(entity)) {
        return;
    }
    if (const auto skinIbmHandle = skinIbmManager_.Read(skinIbmEntity); skinIbmHandle) {
        auto& skinIbm = *skinIbmHandle;
        if (skinIbm.matrices.size() != joints.size()) {
            CORE_LOG_E(
                "Skin bone count doesn't match the given joints (%zu, %zu)!", skinIbm.matrices.size(), joints.size());
            return;
        }

        // make sure the entity has the needed components
        skinManager_.Create(entity);
        skinJointsManager_.Create(entity);
        jointMatricesManager_.Create(entity);

        {
            // set the skin resource handle
            auto skinComponent = skinManager_.Get(entity);
            skinComponent.skin = skinIbmEntity;
            skinComponent.skinRoot = entity;
            skinComponent.skeleton = skeleton;
            skinManager_.Set(entity, skinComponent);
        }

        if (auto skinInstanceHandle = skinJointsManager_.Write(entity); skinInstanceHandle) {
            auto& skinInstance = *skinInstanceHandle;
            skinInstance.count = skinIbm.matrices.size();
            auto jointEntities = array_view<Entity>(skinInstance.jointEntities, skinInstance.count);
            std::copy(joints.begin(), joints.end(), jointEntities.begin());
        }
    }
}

void SkinningSystem::DestroyInstance(Entity const& entity)
{
    if (skinManager_.HasComponent(entity)) {
        skinManager_.Destroy(entity);
    }
    if (skinJointsManager_.HasComponent(entity)) {
        skinJointsManager_.Destroy(entity);
    }
    if (jointMatricesManager_.HasComponent(entity)) {
        jointMatricesManager_.Destroy(entity);
    }
}

ISystem* ISkinningSystemInstance(IEcs& ecs)
{
    return new SkinningSystem(ecs);
}

void ISkinningSystemDestroy(ISystem* instance)
{
    delete static_cast<SkinningSystem*>(instance);
}
CORE3D_END_NAMESPACE()
