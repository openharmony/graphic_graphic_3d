/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "picking.h"

#include <algorithm>
#include <cinttypes>
#include <limits>

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/joint_matrices_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <base/containers/fixed_string.h>
#include <base/math/mathf.h>
#include <base/math/matrix_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_ecs.h>
#include <core/implementation_uids.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_class_register.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/intf_property_handle.h>

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

Math::Mat4X4 Picking::GetCameraViewToProjectionMatrix(const CameraComponent& cameraComponent) const
{
    switch (cameraComponent.projection) {
        case CameraComponent::Projection::ORTHOGRAPHIC: {
            auto orthoProj = Math::OrthoRhZo(cameraComponent.xMag * -0.5f, cameraComponent.xMag * 0.5f,
                cameraComponent.yMag * -0.5f, cameraComponent.yMag * 0.5f, cameraComponent.zNear, cameraComponent.zFar);
            orthoProj[1][1] *= -1.f; // left-hand NDC while Vulkan right-handed -> flip y
            return orthoProj;
        }

        case CameraComponent::Projection::PERSPECTIVE: {
            auto persProj = Math::PerspectiveRhZo(
                cameraComponent.yFov, cameraComponent.aspect, cameraComponent.zNear, cameraComponent.zFar);
            persProj[1][1] *= -1.f; // left-hand NDC while Vulkan right-handed -> flip y
            return persProj;
        }

        case CameraComponent::Projection::CUSTOM: {
            return cameraComponent.customProjectionMatrix;
        }

        default:
            CORE_ASSERT(false);
            return Math::Mat4X4(1.0f);
    }
}

constexpr bool Picking::IntersectAabb(
    Math::Vec3 aabbMin, Math::Vec3 aabbMax, Math::Vec3 start, Math::Vec3 invDirection) const
{
    const float tx1 = (aabbMin.x - start.x) * invDirection.x;
    const float tx2 = (aabbMax.x - start.x) * invDirection.x;

    float tmin = Math::min(tx1, tx2);
    float tmax = Math::max(tx1, tx2);

    const float ty1 = (aabbMin.y - start.y) * invDirection.y;
    const float ty2 = (aabbMax.y - start.y) * invDirection.y;

    tmin = Math::max(tmin, Math::min(ty1, ty2));
    tmax = Math::min(tmax, Math::max(ty1, ty2));

    const float tz1 = (aabbMin.z - start.z) * invDirection.z;
    const float tz2 = (aabbMax.z - start.z) * invDirection.z;

    tmin = Math::max(tmin, Math::min(tz1, tz2));
    tmax = Math::min(tmax, Math::max(tz1, tz2));

    return tmax >= tmin && tmax > 0.0f;
}

// Calculates AABB using WorldMatrixComponent.
void Picking::UpdateRecursiveAABB(const IRenderMeshComponentManager& renderMeshComponentManager,
    const IWorldMatrixComponentManager& worldMatrixComponentManager,
    const IJointMatricesComponentManager& jointMatricesComponentManager, const IMeshComponentManager& meshManager,
    const ISceneNode& sceneNode, bool isRecursive, MinAndMax& mamInOut) const
{
    const Entity entity = sceneNode.GetEntity();
    if (const auto jointMatrices = jointMatricesComponentManager.Read(entity); jointMatrices) {
        // Take skinning into account.
        mamInOut.minAABB = Math::min(mamInOut.minAABB, jointMatrices->jointsAabbMin);
        mamInOut.maxAABB = Math::max(mamInOut.maxAABB, jointMatrices->jointsAabbMax);
    } else {
        const auto worldMatrixId = worldMatrixComponentManager.GetComponentId(entity);
        const auto renderMeshId = renderMeshComponentManager.GetComponentId(entity);
        if (worldMatrixId != IComponentManager::INVALID_COMPONENT_ID &&
            renderMeshId != IComponentManager::INVALID_COMPONENT_ID) {
            const Math::Mat4X4& worldMatrix = worldMatrixComponentManager.Get(worldMatrixId).matrix;

            const RenderMeshComponent rmc = renderMeshComponentManager.Get(renderMeshId);
            if (const auto meshHandle = meshManager.Read(rmc.mesh); meshHandle) {
                const MinAndMax meshMam = GetWorldAABB(worldMatrix, meshHandle->aabbMin, meshHandle->aabbMax);
                mamInOut.minAABB = Math::min(mamInOut.minAABB, meshMam.minAABB);
                mamInOut.maxAABB = Math::max(mamInOut.maxAABB, meshMam.maxAABB);
            }
        }
    }

    if (isRecursive) {
        for (ISceneNode* child : sceneNode.GetChildren()) {
            if (child) {
                UpdateRecursiveAABB(renderMeshComponentManager, worldMatrixComponentManager,
                    jointMatricesComponentManager, meshManager, *child, isRecursive, mamInOut);
            }
        }
    }
}

// Calculates AABB using TransformComponent.
void Picking::UpdateRecursiveAABB(const IRenderMeshComponentManager& renderMeshComponentManager,
    const ITransformComponentManager& transformComponentManager, const IMeshComponentManager& meshManager,
    const ISceneNode& sceneNode, const Math::Mat4X4& parentWorld, bool isRecursive, MinAndMax& mamInOut) const
{
    const Entity entity = sceneNode.GetEntity();
    Math::Mat4X4 worldMatrix = parentWorld;

    if (const auto transformId = transformComponentManager.GetComponentId(entity);
        transformId != IComponentManager::INVALID_COMPONENT_ID) {
        const TransformComponent tc = transformComponentManager.Get(transformId);
        const Math::Mat4X4 localMatrix = Math::Trs(tc.position, tc.rotation, tc.scale);
        worldMatrix = worldMatrix * localMatrix;
    }

    if (const auto renderMeshId = renderMeshComponentManager.GetComponentId(entity);
        renderMeshId != IComponentManager::INVALID_COMPONENT_ID) {
        const RenderMeshComponent rmc = renderMeshComponentManager.Get(renderMeshId);
        if (const auto meshHandle = meshManager.Read(rmc.mesh); meshHandle) {
            const MinAndMax meshMam = GetWorldAABB(worldMatrix, meshHandle->aabbMin, meshHandle->aabbMax);
            mamInOut.minAABB = Math::min(mamInOut.minAABB, meshMam.minAABB);
            mamInOut.maxAABB = Math::max(mamInOut.maxAABB, meshMam.maxAABB);
        }
    }

    // Recurse to children.
    if (isRecursive) {
        for (ISceneNode* child : sceneNode.GetChildren()) {
            if (child) {
                UpdateRecursiveAABB(renderMeshComponentManager, transformComponentManager, meshManager, *child,
                    worldMatrix, isRecursive, mamInOut);
            }
        }
    }
}

RayCastResult Picking::HitTestNode(ISceneNode& node, const MeshComponent& mesh, const Math::Mat4X4& matrix,
    const Math::Vec3& start, const Math::Vec3& invDir) const
{
    RayCastResult raycastResult;

    const MinAndMax meshMinMax = GetWorldAABB(matrix, mesh.aabbMin, mesh.aabbMax);
    if (IntersectAabb(meshMinMax.minAABB, meshMinMax.maxAABB, start, invDir)) {
        if (mesh.submeshes.size() > 1) {
            raycastResult.distance = std::numeric_limits<float>::max();

            for (auto const& submesh : mesh.submeshes) {
                const MinAndMax submeshMinMax = GetWorldAABB(matrix, submesh.aabbMin, submesh.aabbMax);
                if (IntersectAabb(submeshMinMax.minAABB, submeshMinMax.maxAABB, start, invDir)) {
                    const float distance =
                        Math::Magnitude((submeshMinMax.maxAABB + submeshMinMax.minAABB) / 2.f - start);
                    if (distance < raycastResult.distance) {
                        raycastResult.node = &node;
                        raycastResult.distance = distance;
                    }
                }
            }
        } else {
            raycastResult.distance = Math::Magnitude((meshMinMax.minAABB + meshMinMax.maxAABB) / 2.f - start);
            raycastResult.node = &node;
        }
    }

    return raycastResult;
}

Math::Vec3 Picking::ScreenToWorld(IEcs const& ecs, Entity cameraEntity, Math::Vec3 screenCoordinate) const
{
    if (!EntityUtil::IsValid(cameraEntity)) {
        return {};
    }

    auto cameraComponentManager = GetManager<ICameraComponentManager>(ecs);
    const auto cameraId = cameraComponentManager->GetComponentId(cameraEntity);
    if (cameraId == IComponentManager::INVALID_COMPONENT_ID) {
        return {};
    }

    auto worldMatrixComponentManager = GetManager<IWorldMatrixComponentManager>(ecs);
    const auto worldMatrixId = worldMatrixComponentManager->GetComponentId(cameraEntity);
    if (worldMatrixId == IComponentManager::INVALID_COMPONENT_ID) {
        return {};
    }

    const CameraComponent cameraComponent = cameraComponentManager->Get(cameraId);

    screenCoordinate.x = (screenCoordinate.x - 0.5f) * 2.f;
    screenCoordinate.y = (screenCoordinate.y - 0.5f) * 2.f;

    Math::Mat4X4 projToView = Math::Inverse(GetCameraViewToProjectionMatrix(cameraComponent));

    const WorldMatrixComponent worldMatrixComponent = worldMatrixComponentManager->Get(worldMatrixId);
    auto const& worldFromView = worldMatrixComponent.matrix;
    const auto viewCoordinate =
        (projToView * Math::Vec4(screenCoordinate.x, screenCoordinate.y, screenCoordinate.z, 1.f));
    auto worldCoordinate = worldFromView * viewCoordinate;
    worldCoordinate /= worldCoordinate.w;
    return Math::Vec3 { worldCoordinate.x, worldCoordinate.y, worldCoordinate.z };
}

Math::Vec3 Picking::WorldToScreen(IEcs const& ecs, Entity cameraEntity, Math::Vec3 worldCoordinate) const
{
    if (!EntityUtil::IsValid(cameraEntity)) {
        return {};
    }

    auto cameraComponentManager = GetManager<ICameraComponentManager>(ecs);
    const auto cameraId = cameraComponentManager->GetComponentId(cameraEntity);
    if (cameraId == IComponentManager::INVALID_COMPONENT_ID) {
        return {};
    }

    auto worldMatrixComponentManager = GetManager<IWorldMatrixComponentManager>(ecs);
    const auto worldMatrixId = worldMatrixComponentManager->GetComponentId(cameraEntity);
    if (worldMatrixId == IComponentManager::INVALID_COMPONENT_ID) {
        return {};
    }

    const CameraComponent cameraComponent = cameraComponentManager->Get(cameraId);
    Math::Mat4X4 viewToProj = GetCameraViewToProjectionMatrix(cameraComponent);

    const WorldMatrixComponent worldMatrixComponent = worldMatrixComponentManager->Get(worldMatrixId);
    auto const worldToView = Math::Inverse(worldMatrixComponent.matrix);
    const auto viewCoordinate = worldToView * Math::Vec4(worldCoordinate.x, worldCoordinate.y, worldCoordinate.z, 1.f);
    auto screenCoordinate = viewToProj * viewCoordinate;

    // Give sane results also when the point is behind the camera.
    if (screenCoordinate.w < 0.0f) {
        screenCoordinate.x *= -1.0f;
        screenCoordinate.y *= -1.0f;
        screenCoordinate.z *= -1.0f;
    }

    screenCoordinate /= screenCoordinate.w;
    screenCoordinate.x = screenCoordinate.x * 0.5f + 0.5f;
    screenCoordinate.y = screenCoordinate.y * 0.5f + 0.5f;

    return Math::Vec3 { screenCoordinate.x, screenCoordinate.y, screenCoordinate.z };
}

vector<RayCastResult> Picking::RayCast(const IEcs& ecs, const Math::Vec3& start, const Math::Vec3& direction) const
{
    vector<RayCastResult> result;

    auto nodeSystem = GetSystem<INodeSystem>(ecs);
    auto const& renderMeshComponentManager = GetManager<IRenderMeshComponentManager>(ecs);
    auto const& worldMatrixComponentManager = GetManager<IWorldMatrixComponentManager>(ecs);
    auto const& jointMatricesComponentManager = GetManager<IJointMatricesComponentManager>(ecs);
    auto const& meshComponentManager = *GetManager<IMeshComponentManager>(ecs);

    auto const invDir = Math::Vec3(1.f / direction.x, 1.f / direction.y, 1.f / direction.z);
    for (IComponentManager::ComponentId i = 0; i < renderMeshComponentManager->GetComponentCount(); i++) {
        const Entity id = renderMeshComponentManager->GetEntity(i);
        if (auto node = nodeSystem->GetNode(id); node) {
            if (const auto jointMatrices = jointMatricesComponentManager->Read(id); jointMatrices) {
                // Use the skinned aabb's.
                const auto& jointMatricesComponent = *jointMatrices;
                if (IntersectAabb(
                        jointMatricesComponent.jointsAabbMin, jointMatricesComponent.jointsAabbMax, start, invDir)) {
                    const float distance = Math::Magnitude(
                        (jointMatricesComponent.jointsAabbMax + jointMatricesComponent.jointsAabbMin) * 0.5f - start);
                    result.emplace_back(RayCastResult { node, distance });
                }
                continue;
            } else {
                if (const auto worldMatrixId = worldMatrixComponentManager->GetComponentId(id);
                    worldMatrixId != IComponentManager::INVALID_COMPONENT_ID) {
                    auto const renderMeshComponent = renderMeshComponentManager->Get(i);
                    if (const auto meshHandle = meshComponentManager.Read(renderMeshComponent.mesh); meshHandle) {
                        auto const worldMatrixComponent = worldMatrixComponentManager->Get(worldMatrixId);
                        const auto raycastResult =
                            HitTestNode(*node, *meshHandle, worldMatrixComponent.matrix, start, invDir);
                        if (raycastResult.node) {
                            result.push_back(raycastResult);
                        }
                    } else {
                        CORE_LOG_W("no mesh resource for entity %" PRIx64 ", resource %" PRIx64, id.id,
                            renderMeshComponent.mesh.id);
                        continue;
                    }
                }
            }
        }
    }

    std::sort(
        result.begin(), result.end(), [](const auto& lhs, const auto& rhs) { return (lhs.distance < rhs.distance); });

    return result;
}

vector<RayCastResult> Picking::RayCastFromCamera(IEcs const& ecs, Entity camera, const Math::Vec2& screenPos) const
{
    const auto* worldMatrixManager = GetManager<IWorldMatrixComponentManager>(ecs);
    const auto* cameraManager = GetManager<ICameraComponentManager>(ecs);
    if (!worldMatrixManager || !cameraManager) {
        return vector<RayCastResult>();
    }

    const auto wmcId = worldMatrixManager->GetComponentId(camera);
    const auto ccId = cameraManager->GetComponentId(camera);
    if (wmcId != IComponentManager::INVALID_COMPONENT_ID && ccId != IComponentManager::INVALID_COMPONENT_ID) {
        const auto worldMatrixComponent = worldMatrixManager->Get(wmcId);
        const auto cameraComponent = cameraManager->Get(ccId);
        if (cameraComponent.projection == CORE3D_NS::CameraComponent::Projection::ORTHOGRAPHIC) {
            const Math::Vec3 worldPos = ScreenToWorld(ecs, camera, Math::Vec3(screenPos.x, screenPos.y, 0.0f));
            const auto direction = worldMatrixComponent.matrix * Math::Vec4(0.0f, 0.0f, -1.0f, 0.0f);
            return RayCast(ecs, worldPos, direction);
        } else {
            // Ray origin is the camera world position.
            const Math::Vec3& rayOrigin = Math::Vec3(worldMatrixComponent.matrix.w);
            const Math::Vec3 targetPos = ScreenToWorld(ecs, camera, Math::Vec3(screenPos.x, screenPos.y, 1.0f));
            const Math::Vec3 direction = Math::Normalize(targetPos - rayOrigin);
            return RayCast(ecs, rayOrigin, direction);
        }
    }

    return vector<RayCastResult>();
}

MinAndMax Picking::GetWorldAABB(const Math::Mat4X4& world, const Math::Vec3& aabbMin, const Math::Vec3& aabbMax) const
{
    auto const aabb0 = Math::MultiplyPoint3X4(world, Math::Vec3(aabbMin.x, aabbMin.y, aabbMin.z));
    auto const aabb1 = Math::MultiplyPoint3X4(world, Math::Vec3(aabbMax.x, aabbMin.y, aabbMin.z));
    auto const aabb2 = Math::MultiplyPoint3X4(world, Math::Vec3(aabbMin.x, aabbMax.y, aabbMin.z));
    auto const aabb3 = Math::MultiplyPoint3X4(world, Math::Vec3(aabbMax.x, aabbMax.y, aabbMin.z));
    auto const aabb4 = Math::MultiplyPoint3X4(world, Math::Vec3(aabbMin.x, aabbMin.y, aabbMax.z));
    auto const aabb5 = Math::MultiplyPoint3X4(world, Math::Vec3(aabbMax.x, aabbMin.y, aabbMax.z));
    auto const aabb6 = Math::MultiplyPoint3X4(world, Math::Vec3(aabbMin.x, aabbMax.y, aabbMax.z));
    auto const aabb7 = Math::MultiplyPoint3X4(world, Math::Vec3(aabbMax.x, aabbMax.y, aabbMax.z));

    MinAndMax mam;
    mam.minAABB = Math::min(aabb0,
        Math::min(aabb1,
            Math::min(aabb2,
                Math::min(aabb3, Math::min(aabb4, Math::min(aabb4, Math::min(aabb5, Math::min(aabb6, aabb7))))))));
    mam.maxAABB = Math::max(aabb0,
        Math::max(aabb1,
            Math::max(aabb2,
                Math::max(aabb3, Math::max(aabb4, Math::max(aabb4, Math::max(aabb5, Math::max(aabb6, aabb7))))))));

    return mam;
}

MinAndMax Picking::GetWorldMatrixComponentAABB(Entity entity, bool isRecursive, IEcs& ecs) const
{
    MinAndMax mam;

    if (ISceneNode* node = GetSystem<INodeSystem>(ecs)->GetNode(entity); node) {
        auto& renderMeshComponentManager = *GetManager<IRenderMeshComponentManager>(ecs);
        auto& worldMatrixComponentManager = *GetManager<IWorldMatrixComponentManager>(ecs);
        auto& jointworldMatrixComponentManager = *GetManager<IJointMatricesComponentManager>(ecs);
        auto& meshComponentManager = *GetManager<IMeshComponentManager>(ecs);

        UpdateRecursiveAABB(renderMeshComponentManager, worldMatrixComponentManager, jointworldMatrixComponentManager,
            meshComponentManager, *node, isRecursive, mam);
    }

    return mam;
}

MinAndMax Picking::GetTransformComponentAABB(Entity entity, bool isRecursive, IEcs& ecs) const
{
    MinAndMax mam;

    if (ISceneNode* node = GetSystem<INodeSystem>(ecs)->GetNode(entity); node) {
        auto& renderMeshComponentManager = *GetManager<IRenderMeshComponentManager>(ecs);
        auto& transformComponentManager = *GetManager<ITransformComponentManager>(ecs);
        auto& meshComponentManager = *GetManager<IMeshComponentManager>(ecs);

        UpdateRecursiveAABB(renderMeshComponentManager, transformComponentManager, meshComponentManager, *node,
            Math::Mat4X4(1.0f), isRecursive, mam);
    }

    return mam;
}

const IInterface* Picking::GetInterface(const Uid& uid) const
{
    if (uid == IPicking::UID) {
        return this;
    }
    return nullptr;
}

IInterface* Picking::GetInterface(const Uid& uid)
{
    if (uid == IPicking::UID) {
        return this;
    }
    return nullptr;
}

void Picking::Ref() {}

void Picking::Unref() {}
CORE3D_END_NAMESPACE()
