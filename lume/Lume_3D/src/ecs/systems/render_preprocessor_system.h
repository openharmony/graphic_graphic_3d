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

#ifndef CORE3D_ECS_RENDER_PREPROCESSOR_SYSTEM_H
#define CORE3D_ECS_RENDER_PREPROCESSOR_SYSTEM_H

#include <ComponentTools/component_query.h>
#include <limits>

#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/intf_graphics_context.h>
#include <base/math/vector.h>
#include <core/ecs/intf_ecs.h>
#include <core/property_tools/property_api_impl.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/intf_render_context.h>

#include "property/property_handle.h"

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
class IRenderDataStoreManager;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultCamera;
class IRenderDataStoreDefaultLight;
class IRenderDataStoreDefaultMaterial;
class IRenderDataStoreDefaultScene;
class IRenderDataStoreMorph;
class IJointMatricesComponentManager;
class ILayerComponentManager;
class IMaterialComponentManager;
class IMaterialExtensionComponentManager;
class IMeshComponentManager;
class INodeComponentManager;
class IRenderMeshComponentManager;
class IRenderHandleComponentManager;
class ISkinComponentManager;
class IWorldMatrixComponentManager;
class IPicking;
struct MaterialComponent;

class RenderPreprocessorSystem final : public IRenderPreprocessorSystem, CORE_NS::IEcs::ComponentListener {
public:
    explicit RenderPreprocessorSystem(CORE_NS::IEcs& ecs);
    ~RenderPreprocessorSystem() override;
    BASE_NS::string_view GetName() const override;
    BASE_NS::Uid GetUid() const override;
    CORE_NS::IPropertyHandle* GetProperties() override;
    const CORE_NS::IPropertyHandle* GetProperties() const override;
    void SetProperties(const CORE_NS::IPropertyHandle&) override;
    bool IsActive() const override;
    void SetActive(bool state) override;

    void Initialize() override;
    void Uninitialize() override;
    bool Update(bool frameRenderingQueued, uint64_t totalTime, uint64_t deltaTime) override;

    const CORE_NS::IEcs& GetECS() const override;

    BASE_NS::array_view<const CORE_NS::Entity> GetRenderBatchMeshEntities() const;
    BASE_NS::array_view<const CORE_NS::Entity> GetInstancingAllowedEntities() const;
    BASE_NS::array_view<const CORE_NS::Entity> GetInstancingDisabledEntities() const;

    struct Aabb {
        BASE_NS::Math::Vec3 min { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max() };
        BASE_NS::Math::Vec3 max { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
            -std::numeric_limits<float>::max() };
    };

    // whole mesh
    Aabb GetRenderMeshAabb(CORE_NS::Entity renderMesh) const;
    // one for each submesh
    BASE_NS::array_view<const Aabb> GetRenderMeshAabbs(CORE_NS::Entity renderMesh) const;

    struct Sphere {
        BASE_NS::Math::Vec3 center;
        float radius {};
    };
    Sphere GetBoundingSphere() const;

    struct SortData {
        CORE_NS::IComponentManager::ComponentId renderMeshId;
        CORE_NS::Entity mesh;
        CORE_NS::Entity batch;
        CORE_NS::Entity skin;
        bool allowInstancing;
    };

    struct MaterialProperties {
        CORE_NS::Entity material;
        bool disabled;
        bool allowInstancing;
        bool shadowCaster;
    };

private:
    void OnComponentEvent(EventType type, const CORE_NS::IComponentManager& componentManager,
        BASE_NS::array_view<const CORE_NS::Entity> entities) override;
    void HandleMaterialEvents();
    void SetDataStorePointers(RENDER_NS::IRenderDataStoreManager& manager);
    void CalculateSceneBounds();
    void GatherSortData();
    void UpdateMaterialProperties();
    void UpdateSingleMaterial(CORE_NS::Entity matEntity, const MaterialComponent* materialHandle);
    MaterialProperties* GetMaterialProperties(CORE_NS::Entity matEntity);

    CORE_NS::IEcs& ecs_;
    IGraphicsContext* graphicsContext_ { nullptr };
    RENDER_NS::IRenderContext* renderContext_ { nullptr };
    IJointMatricesComponentManager* jointMatricesManager_ { nullptr };
    ILayerComponentManager* layerManager_ { nullptr };
    IMaterialComponentManager* materialManager_ { nullptr };
    IRenderHandleComponentManager* renderHandleManager_ { nullptr };
    IMeshComponentManager* meshManager_ { nullptr };
    INodeComponentManager* nodeManager_ { nullptr };
    IRenderMeshComponentManager* renderMeshManager_ { nullptr };
    ISkinComponentManager* skinManager_ { nullptr };
    IWorldMatrixComponentManager* worldMatrixManager_ { nullptr };
    bool active_ { true };

    IRenderPreprocessorSystem::Properties properties_ {
        "RenderDataStoreDefaultScene",
        "RenderDataStoreDefaultCamera",
        "RenderDataStoreDefaultLight",
        "RenderDataStoreDefaultMaterial",
        "RenderDataStoreMorph",
        "",
    };
    CORE_NS::PropertyApiImpl<IRenderPreprocessorSystem::Properties> RENDER_PREPROCESSOR_SYSTEM_PROPERTIES;

    BASE_NS::refcnt_ptr<IRenderDataStoreDefaultCamera> dsCamera_;
    BASE_NS::refcnt_ptr<IRenderDataStoreDefaultLight> dsLight_;
    BASE_NS::refcnt_ptr<IRenderDataStoreDefaultMaterial> dsMaterial_;
    BASE_NS::refcnt_ptr<IRenderDataStoreDefaultScene> dsScene_;
    BASE_NS::refcnt_ptr<IRenderDataStoreMorph> dsMorph_;

    IPicking* picking_ = nullptr;

    CORE_NS::ComponentQuery renderableQuery_;
    uint32_t jointGeneration_ { 0U };
    uint32_t layerGeneration_ { 0U };
    uint32_t materialGeneration_ { 0U };
    uint32_t meshGeneration_ { 0U };
    uint32_t nodeGeneration_ { 0U };
    uint32_t renderMeshGeneration_ { 0U };
    uint32_t worldMatrixGeneration_ { 0U };

    BASE_NS::vector<CORE_NS::Entity> materialModifiedEvents_;
    BASE_NS::vector<CORE_NS::Entity> materialDestroyedEvents_;
    BASE_NS::vector<MaterialProperties> materialProperties_;

    BASE_NS::vector<SortData> meshComponents_;

    BASE_NS::vector<CORE_NS::Entity> renderMeshComponents_;
    BASE_NS::array_view<const CORE_NS::Entity> renderBatchComponents_;
    BASE_NS::array_view<const CORE_NS::Entity> instancingAllowed_;
    BASE_NS::array_view<const CORE_NS::Entity> rest_;
    struct RenderMeshAaabb {
        CORE_NS::Entity entity;
        Aabb meshAabb;
        BASE_NS::vector<Aabb> submeshAabbs;
        bool shadowCaster { true };
    };
    BASE_NS::vector<RenderMeshAaabb> renderMeshAabbs_;
    Sphere boundingSphere_;
};
CORE3D_END_NAMESPACE()

#endif // CORE3D_ECS_RENDER_PREPROCESSOR_SYSTEM_H
