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
#ifndef CORE_UTIL_PICKING_H
#define CORE_UTIL_PICKING_H

#include <3d/util/intf_picking.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
class IEngine;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IJointMatricesComponentManager;
class IMeshComponentManager;
class ITransformComponentManager;
class IRenderMeshComponentManager;
class IWorldMatrixComponentManager;
struct CameraComponent;
struct MeshComponent;

class Picking : public IPicking {
public:
    Picking() = default;
    ~Picking() override = default;
    BASE_NS::Math::Vec3 ScreenToWorld(
        CORE_NS::IEcs const& ecs, CORE_NS::Entity cameraEntity, BASE_NS::Math::Vec3 screenCoordinate) const override;

    BASE_NS::Math::Vec3 WorldToScreen(
        CORE_NS::IEcs const& ecs, CORE_NS::Entity cameraEntity, BASE_NS::Math::Vec3 worldCoordinate) const override;

    BASE_NS::vector<RayCastResult> RayCast(CORE_NS::IEcs const& ecs, const BASE_NS::Math::Vec3& start,
        const BASE_NS::Math::Vec3& direction) const override;
    BASE_NS::vector<RayCastResult> RayCastFromCamera(
        CORE_NS::IEcs const& ecs, CORE_NS::Entity camera, const BASE_NS::Math::Vec2& screenPos) const override;

    MinAndMax GetWorldAABB(const BASE_NS::Math::Mat4X4& world, const BASE_NS::Math::Vec3& aabbMin,
        const BASE_NS::Math::Vec3& aabbMax) const override;

    MinAndMax GetWorldMatrixComponentAABB(CORE_NS::Entity entity, bool isRecursive, CORE_NS::IEcs& ecs) const override;

    MinAndMax GetTransformComponentAABB(CORE_NS::Entity entity, bool isRecursive, CORE_NS::IEcs& ecs) const override;

    // IInterface
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;

    void Ref() override;
    void Unref() override;

protected:
    BASE_NS::Math::Mat4X4 GetCameraViewToProjectionMatrix(const CameraComponent& cameraComponent) const;

    constexpr bool IntersectAabb(const BASE_NS::Math::Vec3 aabbMin, const BASE_NS::Math::Vec3 aabbMax,
        const BASE_NS::Math::Vec3 start, const BASE_NS::Math::Vec3 invDirection) const;

    // Calculates AABB using WorldMatrixComponent.
    void UpdateRecursiveAABB(const IRenderMeshComponentManager& renderMeshComponentManager,
        const IWorldMatrixComponentManager& worldMatrixComponentManager,
        const IJointMatricesComponentManager& jointMatricesComponentManager, const IMeshComponentManager& meshManager,
        const ISceneNode& sceneNode, bool isRecursive, MinAndMax& mamInOut) const;

    // Calculates AABB using TransformComponent.
    void UpdateRecursiveAABB(const IRenderMeshComponentManager& renderMeshComponentManager,
        const ITransformComponentManager& transformComponentManager, const IMeshComponentManager& meshManager,
        const ISceneNode& sceneNode, const BASE_NS::Math::Mat4X4& parentWorld, bool isRecursive,
        MinAndMax& mamInOut) const;

    RayCastResult HitTestNode(ISceneNode& node, const MeshComponent& mesh, const BASE_NS::Math::Mat4X4& matrix,
        const BASE_NS::Math::Vec3& start, const BASE_NS::Math::Vec3& invDir) const;
};
CORE3D_END_NAMESPACE()

#endif // CORE_UTIL_PICKING_H