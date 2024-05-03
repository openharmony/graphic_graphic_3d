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

#ifndef CORE_ECS_SKINNINGSYSTEM_H
#define CORE_ECS_SKINNINGSYSTEM_H

#include <ComponentTools/component_query.h>
#include <PropertyTools/property_api_impl.h>

#include <3d/ecs/systems/intf_skinning_system.h>
#include <base/math/matrix.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>

CORE3D_BEGIN_NAMESPACE()
class INodeComponentManager;
class ISkinComponentManager;
class ISkinIbmComponentManager;
class ISkinJointsComponentManager;
class IJointMatricesComponentManager;
class IPreviousJointMatricesComponentManager;
class IWorldMatrixComponentManager;
class INodeSystem;
class ICameraComponentManager;
class ILightComponentManager;
class IMeshComponentManager;
class IRenderMeshComponentManager;
class IPicking;

struct JointMatricesComponent;

class SkinningSystem final : public ISkinningSystem {
public:
    explicit SkinningSystem(CORE_NS::IEcs& ecs);
    ~SkinningSystem() override = default;
    BASE_NS::string_view GetName() const override;
    BASE_NS::Uid GetUid() const override;
    CORE_NS::IPropertyHandle* GetProperties() override;
    const CORE_NS::IPropertyHandle* GetProperties() const override;
    void SetProperties(const CORE_NS::IPropertyHandle&) override;

    bool IsActive() const override;
    void SetActive(bool state) override;

    void Initialize() override;
    bool Update(bool frameRenderingQueued, uint64_t time, uint64_t delta) override;
    void Uninitialize() override;

    const CORE_NS::IEcs& GetECS() const override;

    void CreateInstance(CORE_NS::Entity const& skinIbmEntity, BASE_NS::array_view<const CORE_NS::Entity> const& joints,
        CORE_NS::Entity const& entity, CORE_NS::Entity const& skeleton) override;
    void CreateInstance(
        CORE_NS::Entity const& skinIbmEntity, CORE_NS::Entity const& entity, CORE_NS::Entity const& skeleton) override;
    void DestroyInstance(CORE_NS::Entity const& entity) override;

private:
    void UpdateSkin(const CORE_NS::ComponentQuery::ResultRow& row);
    void UpdateJointTransformations(bool isEnabled, const BASE_NS::array_view<CORE_NS::Entity const>& jointEntities,
        const BASE_NS::array_view<BASE_NS::Math::Mat4X4 const>& iblMatrices, JointMatricesComponent& jointMatrices,
        const BASE_NS::Math::Mat4X4& skinEntityWorldInverse);

    bool active_;
    CORE_NS::IEcs& ecs_;

    IPicking& picking_;

    ISkinComponentManager& skinManager_;
    ISkinIbmComponentManager& skinIbmManager_;
    ISkinJointsComponentManager& skinJointsManager_;
    IJointMatricesComponentManager& jointMatricesManager_;
    IPreviousJointMatricesComponentManager& previousJointMatricesManager_;
    IWorldMatrixComponentManager& worldMatrixManager_;
    INodeComponentManager& nodeManager_;
    IRenderMeshComponentManager& renderMeshManager_;
    IMeshComponentManager& meshManager_;
    INodeSystem* nodeSystem_ = nullptr;

    CORE_NS::ComponentQuery componentQuery_;

    uint32_t worldMatrixGeneration_ { 0 };
    uint32_t jointMatricesGeneration_ { 0 };
    CORE_NS::PropertyApiImpl<void> SKINNING_SYSTEM_PROPERTIES;

    CORE_NS::IThreadPool::Ptr threadPool_;
    class SkinTask;
    BASE_NS::vector<SkinTask> tasks_;
    BASE_NS::vector<CORE_NS::IThreadPool::IResult::Ptr> taskResults_;
};
CORE3D_END_NAMESPACE()

#endif // CORE_ECS_SKINNINGSYSTEM_H
