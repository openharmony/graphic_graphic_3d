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

#include "picking.h"

#include <algorithm>
#include <cinttypes>
#include <limits>

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/joint_matrices_component.h>
#include <3d/ecs/components/layer_component.h>
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

#include "util/scene_util.h"

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace {
MinAndMax GetWorldAABB(const Math::Mat4X4& world, const Math::Vec3& aabbMin, const Math::Vec3& aabbMax)
{
    // Based on https://gist.github.com/cmf028/81e8d3907035640ee0e3fdd69ada543f
    const auto center = (aabbMin + aabbMax) * 0.5f;
    const auto extents = aabbMax - center;

    const auto centerW = Math::MultiplyPoint3X4(world, center);

    Math::Mat3X3 absWorld;
    for (auto i = 0U; i < countof(absWorld.base); ++i) {
        absWorld.base[i].x = Math::abs(world.base[i].x);
        absWorld.base[i].y = Math::abs(world.base[i].y);
        absWorld.base[i].z = Math::abs(world.base[i].z);
    }

    Math::Vec3 extentsW;
    extentsW.x = absWorld.x.x * extents.x + absWorld.y.x * extents.y + absWorld.z.x * extents.z;
    extentsW.y = absWorld.x.y * extents.x + absWorld.y.y * extents.y + absWorld.z.y * extents.z;
    extentsW.z = absWorld.x.z * extents.x + absWorld.y.z * extents.y + absWorld.z.z * extents.z;

    return MinAndMax { centerW - extentsW, centerW + extentsW };
}

constexpr bool IntersectAabb(
    Math::Vec3 aabbMin, Math::Vec3 aabbMax, Math::Vec3 start, Math::Vec3 invDirection, float& hitDistance)
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

    hitDistance = tmin;
    // If hitDistance < 0, ray origin was inside the AABB.
    if (hitDistance < 0.0f) {
        hitDistance = tmax;
        // No hit, set distance to 0.
        if (hitDistance < 0.0f) {
            hitDistance = 0.0f;
        }
    }

    return tmax >= tmin && tmax > 0.0f;
}

bool IntersectTriangle(const Math::Vec3 triangle[3], const Math::Vec3 start, const Math::Vec3 direction,
    float& hitDistance, Math::Vec2& uv)
{
    const Math::Vec3 v0v1 = triangle[1] - triangle[0];
    const Math::Vec3 v0v2 = triangle[2] - triangle[0];

    const Math::Vec3 pvec = Math::Cross(direction, v0v2);
    const float det = Math::Dot(v0v1, pvec);

    // ray and triangle are parallel and backface culling
    if (det < Math::EPSILON) {
        hitDistance = 0.f;
        return false;
    }

    const float invDet = 1.f / det;
    const Math::Vec3 tvec = start - triangle[0];

    const float u = Math::Dot(tvec, pvec) * invDet;
    if (u < 0 || u > 1) {
        hitDistance = 0.f;
        uv = Math::Vec2(0, 0);
        return false;
    }

    const Math::Vec3 qvec = Math::Cross(tvec, v0v1);
    const float v = Math::Dot(direction, qvec) * invDet;
    if (v < 0 || u + v > 1) {
        hitDistance = 0.f;
        uv = Math::Vec2(0, 0);
        return false;
    }

    hitDistance = Math::Dot(v0v2, qvec) * invDet;
    uv = Math::Vec2(u, v);
    return true;
}

// Calculates AABB using WorldMatrixComponent.
void UpdateRecursiveAABB(const IRenderMeshComponentManager& renderMeshComponentManager,
    const IWorldMatrixComponentManager& worldMatrixComponentManager,
    const IJointMatricesComponentManager& jointMatricesComponentManager, const IMeshComponentManager& meshManager,
    const ISceneNode& sceneNode, bool isRecursive, MinAndMax& mamInOut)
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
void UpdateRecursiveAABB(const IRenderMeshComponentManager& renderMeshComponentManager,
    const ITransformComponentManager& transformComponentManager, const IMeshComponentManager& meshManager,
    const ISceneNode& sceneNode, const Math::Mat4X4& parentWorld, bool isRecursive, MinAndMax& mamInOut)
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

RayCastResult HitTestNode(ISceneNode& node, const MeshComponent& mesh, const Math::Mat4X4& matrix,
    const Math::Vec3& start, const Math::Vec3& invDir)
{
    RayCastResult raycastResult;
    const auto direction = Math::Vec3(1.f / invDir.x, 1.f / invDir.y, 1.f / invDir.z);

    const MinAndMax meshMinMax = GetWorldAABB(matrix, mesh.aabbMin, mesh.aabbMax);
    float distance = 0;
    if (IntersectAabb(meshMinMax.minAABB, meshMinMax.maxAABB, start, invDir, distance)) {
        if (mesh.submeshes.size() > 1) {
            raycastResult.centerDistance = std::numeric_limits<float>::max();

            for (auto const& submesh : mesh.submeshes) {
                const MinAndMax submeshMinMax = GetWorldAABB(matrix, submesh.aabbMin, submesh.aabbMax);
                if (IntersectAabb(submeshMinMax.minAABB, submeshMinMax.maxAABB, start, invDir, distance)) {
                    const float centerDistance =
                        Math::Magnitude((submeshMinMax.maxAABB + submeshMinMax.minAABB) / 2.f - start);
                    if (centerDistance < raycastResult.centerDistance) {
                        raycastResult.node = &node;
                        raycastResult.centerDistance = centerDistance;
                        raycastResult.distance = distance;
                        raycastResult.worldPosition = start + direction * distance;
                    }
                }
            }
        } else {
            raycastResult.centerDistance = Math::Magnitude((meshMinMax.minAABB + meshMinMax.maxAABB) / 2.f - start);
            raycastResult.distance = distance;
            raycastResult.node = &node;
            raycastResult.worldPosition = start + direction * raycastResult.distance;
        }
    }

    return raycastResult;
}

Math::Vec3 ScreenToWorld(const CameraComponent& cameraComponent, const WorldMatrixComponent& cameraWorldMatrixComponent,
    Math::Vec3 screenCoordinate)
{
    screenCoordinate.x = (screenCoordinate.x - 0.5f) * 2.f;
    screenCoordinate.y = (screenCoordinate.y - 0.5f) * 2.f;

    bool isCameraNegative = false;
    Math::Mat4X4 projToView =
        Math::Inverse(CameraMatrixUtil::CalculateProjectionMatrix(cameraComponent, isCameraNegative));

    auto const& worldFromView = cameraWorldMatrixComponent.matrix;
    const auto viewCoordinate =
        (projToView * Math::Vec4(screenCoordinate.x, screenCoordinate.y, screenCoordinate.z, 1.f));
    auto worldCoordinate = worldFromView * viewCoordinate;
    worldCoordinate /= worldCoordinate.w;
    return Math::Vec3 { worldCoordinate.x, worldCoordinate.y, worldCoordinate.z };
}

struct Ray {
    Math::Vec3 origin;
    Math::Vec3 direction;
};

Ray RayFromCamera(const CameraComponent& cameraComponent, const WorldMatrixComponent& cameraWorldMatrixComponent,
    Math::Vec2 screenCoordinate)
{
    if (cameraComponent.projection == CORE3D_NS::CameraComponent::Projection::ORTHOGRAPHIC) {
        const Math::Vec3 worldPos = CORE3D_NS::ScreenToWorld(
            cameraComponent, cameraWorldMatrixComponent, Math::Vec3(screenCoordinate.x, screenCoordinate.y, 0.0f));
        const auto direction = cameraWorldMatrixComponent.matrix * Math::Vec4(0.0f, 0.0f, -1.0f, 0.0f);
        return Ray { worldPos, direction };
    }
    // Ray origin is the camera world position.
    const Math::Vec3& rayOrigin = Math::Vec3(cameraWorldMatrixComponent.matrix.w);
    const Math::Vec3 targetPos = CORE3D_NS::ScreenToWorld(
        cameraComponent, cameraWorldMatrixComponent, Math::Vec3(screenCoordinate.x, screenCoordinate.y, 1.0f));
    const Math::Vec3 direction = Math::Normalize(targetPos - rayOrigin);
    return Ray { rayOrigin, direction };
}
} // namespace

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
    return CORE3D_NS::ScreenToWorld(
        *cameraComponentManager->Read(cameraId), worldMatrixComponentManager->Get(worldMatrixId), screenCoordinate);
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
    bool isCameraNegative = false;
    Math::Mat4X4 viewToProj = CameraMatrixUtil::CalculateProjectionMatrix(cameraComponent, isCameraNegative);

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
    float distance = 0;

    auto const invDir = DirectionVectorInverse(direction);
    for (IComponentManager::ComponentId i = 0; i < renderMeshComponentManager->GetComponentCount(); i++) {
        const Entity id = renderMeshComponentManager->GetEntity(i);
        if (auto node = nodeSystem->GetNode(id); node) {
            if (const auto jointMatrices = jointMatricesComponentManager->Read(id); jointMatrices) {
                // Use the skinned aabb's.
                const auto& jointMatricesComponent = *jointMatrices;
                if (IntersectAabb(jointMatricesComponent.jointsAabbMin, jointMatricesComponent.jointsAabbMax, start,
                        invDir, distance)) {
                    const float centerDistance = Math::Magnitude(
                        (jointMatricesComponent.jointsAabbMax + jointMatricesComponent.jointsAabbMin) * 0.5f - start);
                    const Math::Vec3 hitPosition = start + direction * distance;
                    result.push_back(RayCastResult { node, centerDistance, distance, hitPosition });
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

vector<RayCastResult> Picking::RayCast(
    const IEcs& ecs, const Math::Vec3& start, const Math::Vec3& direction, uint64_t layerMask) const
{
    vector<RayCastResult> result;

    auto nodeSystem = GetSystem<INodeSystem>(ecs);
    auto const& renderMeshComponentManager = GetManager<IRenderMeshComponentManager>(ecs);
    auto const& layerComponentManager = GetManager<ILayerComponentManager>(ecs);
    auto const& worldMatrixComponentManager = GetManager<IWorldMatrixComponentManager>(ecs);
    auto const& jointMatricesComponentManager = GetManager<IJointMatricesComponentManager>(ecs);
    auto const& meshComponentManager = *GetManager<IMeshComponentManager>(ecs);

    auto const invDir = DirectionVectorInverse(direction);
    float distance = 0;
    for (IComponentManager::ComponentId i = 0; i < renderMeshComponentManager->GetComponentCount(); i++) {
        const Entity id = renderMeshComponentManager->GetEntity(i);
        if (auto node = nodeSystem->GetNode(id); node) {
            if (layerComponentManager->Get(id).layerMask & layerMask) {
                if (const auto jointMatrices = jointMatricesComponentManager->Read(id); jointMatrices) {
                    // Use the skinned aabb's.
                    const auto& jointMatricesComponent = *jointMatrices;
                    if (IntersectAabb(jointMatricesComponent.jointsAabbMin, jointMatricesComponent.jointsAabbMax, start,
                            invDir, distance)) {
                        const float centerDistance = Math::Magnitude(
                            (jointMatricesComponent.jointsAabbMax + jointMatricesComponent.jointsAabbMin) * 0.5f -
                            start);
                        const Math::Vec3 hitPosition = start + direction * distance;
                        result.push_back(RayCastResult { node, centerDistance, distance, hitPosition });
                    }
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
                        }
                    }
                }
            }
        }
    }

    std::sort(
        result.begin(), result.end(), [](const auto& lhs, const auto& rhs) { return (lhs.distance < rhs.distance); });

    return result;
}

BASE_NS::vector<RayTriangleCastResult> Core3D::Picking::RayCast(const BASE_NS::Math::Vec3& start,
    const BASE_NS::Math::Vec3& direction, BASE_NS::array_view<const BASE_NS::Math::Vec3> triangles) const
{
    vector<RayTriangleCastResult> result;

    if (triangles.size() % 3 != 0) { // 3 ：param
        CORE_LOG_W("Number of triangles vertices not divisible by 3!");
        return result;
    }

    float distance = 0.f;
    Math::Vec2 uv;
    for (size_t ii = 0; ii < triangles.size(); ii += 3) { // 3 ：param
        if (IntersectTriangle(&triangles[ii], start, direction, distance, uv)) {
            const Math::Vec3 hitPosition = start + direction * distance;

            result.push_back(RayTriangleCastResult { distance, hitPosition, uv, static_cast<uint64_t>(ii / 3) });
        }
    }

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
        const auto cameraComponent = cameraManager->Read(ccId);
        const auto worldMatrixComponent = worldMatrixManager->Get(wmcId);
        const Ray ray = RayFromCamera(*cameraComponent, worldMatrixComponent, screenPos);
        return RayCast(ecs, ray.origin, ray.direction);
    }

    return vector<RayCastResult>();
}

vector<RayCastResult> Picking::RayCastFromCamera(
    IEcs const& ecs, Entity camera, const Math::Vec2& screenPos, uint64_t layerMask) const
{
    const auto* worldMatrixManager = GetManager<IWorldMatrixComponentManager>(ecs);
    const auto* cameraManager = GetManager<ICameraComponentManager>(ecs);
    if (!worldMatrixManager || !cameraManager) {
        return vector<RayCastResult>();
    }

    const auto wmcId = worldMatrixManager->GetComponentId(camera);
    const auto ccId = cameraManager->GetComponentId(camera);
    if (wmcId != IComponentManager::INVALID_COMPONENT_ID && ccId != IComponentManager::INVALID_COMPONENT_ID) {
        const auto cameraComponent = cameraManager->Read(ccId);
        const auto worldMatrixComponent = worldMatrixManager->Get(wmcId);
        const Ray ray = RayFromCamera(*cameraComponent, worldMatrixComponent, screenPos);
        return RayCast(ecs, ray.origin, ray.direction, layerMask);
    }

    return vector<RayCastResult>();
}

BASE_NS::vector<RayTriangleCastResult> Core3D::Picking::RayCastFromCamera(CORE_NS::IEcs const& ecs,
    CORE_NS::Entity camera, const BASE_NS::Math::Vec2& screenPos,
    BASE_NS::array_view<const BASE_NS::Math::Vec3> triangles) const
{
    const auto* worldMatrixManager = GetManager<IWorldMatrixComponentManager>(ecs);
    const auto* cameraManager = GetManager<ICameraComponentManager>(ecs);
    if (!worldMatrixManager || !cameraManager) {
        return vector<RayTriangleCastResult>();
    }

    const auto wmcId = worldMatrixManager->GetComponentId(camera);
    const auto ccId = cameraManager->GetComponentId(camera);
    if (wmcId != IComponentManager::INVALID_COMPONENT_ID && ccId != IComponentManager::INVALID_COMPONENT_ID) {
        const auto cameraComponent = cameraManager->Read(ccId);
        const auto worldMatrixComponent = worldMatrixManager->Get(wmcId);
        const Ray ray = RayFromCamera(*cameraComponent, worldMatrixComponent, screenPos);

        return RayCast(ray.origin, ray.direction, triangles);
    }

    return BASE_NS::vector<RayTriangleCastResult>();
}

MinAndMax Picking::GetWorldAABB(const Math::Mat4X4& world, const Math::Vec3& aabbMin, const Math::Vec3& aabbMax) const
{
    return CORE3D_NS::GetWorldAABB(world, aabbMin, aabbMax);
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
    if ((uid == IPicking::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

IInterface* Picking::GetInterface(const Uid& uid)
{
    if ((uid == IPicking::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

void Picking::Ref() {}

void Picking::Unref() {}
CORE3D_END_NAMESPACE()
