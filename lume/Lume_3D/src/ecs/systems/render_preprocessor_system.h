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
#include <PropertyTools/property_api_impl.h>
#include <limits>

#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/intf_graphics_context.h>
#include <base/math/vector.h>
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
class IMeshComponentManager;
class INodeComponentManager;
class IRenderMeshComponentManager;
class ISkinComponentManager;
class IWorldMatrixComponentManager;
class IPicking;

class RenderPreprocessorSystem final : public IRenderPreprocessorSystem {
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
    void SetDataStorePointers(RENDER_NS::IRenderDataStoreManager& manager);
    void CalculateSceneBounds();
    void GatherSortData();

    CORE_NS::IEcs& ecs_;
    IGraphicsContext* graphicsContext_ { nullptr };
    RENDER_NS::IRenderContext* renderContext_ { nullptr };
    IJointMatricesComponentManager* jointMatricesManager_ { nullptr };
    ILayerComponentManager* layerManager_ { nullptr };
    IMaterialComponentManager* materialManager_ { nullptr };
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

    IRenderDataStoreDefaultCamera* dsCamera_ { nullptr };
    IRenderDataStoreDefaultLight* dsLight_ { nullptr };
    IRenderDataStoreDefaultMaterial* dsMaterial_ { nullptr };
    IRenderDataStoreDefaultScene* dsScene_ { nullptr };
    IRenderDataStoreMorph* dsMorph_ { nullptr };

    IPicking* picking_ = nullptr;

    CORE_NS::ComponentQuery renderableQuery_;
    uint32_t layerGeneration_ { 0U };
    uint32_t materialGeneration_ { 0U };
    uint32_t meshGeneration_ { 0U };
    uint32_t nodeGeneration_ { 0U };
    uint32_t renderMeshGeneration_ { 0U };
    uint32_t worldMatrixGeneration_ { 0U };
    BASE_NS::vector<MaterialProperties> materialProperties_;
    BASE_NS::vector<SortData> meshComponents_;

    BASE_NS::vector<CORE_NS::Entity> renderMeshComponents_;
    BASE_NS::array_view<const CORE_NS::Entity> renderBatchComponents_;
    BASE_NS::array_view<const CORE_NS::Entity> instancingAllowed_;
    BASE_NS::array_view<const CORE_NS::Entity> rest_;
    struct RenderMeshAaabb {
        Aabb meshAabb;
        BASE_NS::vector<Aabb> submeshAabbs;
    };
    BASE_NS::unordered_map<CORE_NS::Entity, RenderMeshAaabb> renderMeshAabbs_;
    Sphere boundingSphere_;
};
CORE3D_END_NAMESPACE()

#endif // CORE3D_ECS_RENDER_PREPROCESSOR_SYSTEM_H
