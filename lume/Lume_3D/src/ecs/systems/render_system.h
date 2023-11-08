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

#ifndef CORE_ECS_RENDERSYSTEM_H
#define CORE_ECS_RENDERSYSTEM_H

#include <ComponentTools/component_query.h>
#include <PropertyTools/property_api_impl.h>
#include <limits>

#include <3d/ecs/components/layer_defines.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/systems/intf_render_system.h>
#include <3d/render/render_data_defines_3d.h>
#include <base/math/vector.h>
#include <core/namespace.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "property/property_handle.h"

BASE_BEGIN_NAMESPACE()
namespace Math {
class Mat4X4;
} // namespace Math
BASE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
class IShaderManager;
class IRenderContext;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IEnvironmentComponentManager;
class IFogComponentManager;
class IRenderHandleComponentManager;
class INameComponentManager;
class INodeComponentManager;
class IRenderMeshBatchComponentManager;
class IRenderMeshComponentManager;
class IWorldMatrixComponentManager;
class IPreviousWorldMatrixComponentManager;
class IRenderConfigurationComponentManager;
class ICameraComponentManager;
class ILayerComponentManager;
class ILightComponentManager;
class IJointMatricesComponentManager;
class IMaterialExtensionComponentManager;
class IMaterialComponentManager;
class IMeshComponentManager;
class ISkinJointsComponentManager;
class IPlanarReflectionComponentManager;
class IPreviousJointMatricesComponentManager;
class IPostProcessComponentManager;
class IUriComponentManager;
class IRenderDataStoreDefaultCamera;
class IRenderDataStoreDefaultLight;
class IRenderDataStoreDefaultMaterial;
class IRenderDataStoreDefaultScene;

class IRenderPreprocessorSystem;

class IMesh;
class IMaterial;
class IPicking;
class IRenderUtil;
class IGraphicsContext;

struct RenderMeshComponent;
struct JointMatricesComponent;
struct PreviousJointMatricesComponent;
struct MaterialComponent;
struct WorldMatrixComponent;
struct LightComponent;
struct MinAndMax;

class RenderSystem final : public IRenderSystem {
public:
    explicit RenderSystem(CORE_NS::IEcs& ecs);
    ~RenderSystem() override;
    BASE_NS::string_view GetName() const override;
    BASE_NS::Uid GetUid() const override;
    CORE_NS::IPropertyHandle* GetProperties() override;
    const CORE_NS::IPropertyHandle* GetProperties() const override;
    void SetProperties(const CORE_NS::IPropertyHandle&) override;
    bool IsActive() const override;
    void SetActive(bool state) override;

    void Initialize() override;
    bool Update(bool frameRenderingQueued, uint64_t totalTime, uint64_t deltaTime) override;
    void Uninitialize() override;

    const CORE_NS::IEcs& GetECS() const override;

    BASE_NS::array_view<const RENDER_NS::RenderHandleReference> GetRenderNodeGraphs() const override;

    struct BatchData {
        CORE_NS::Entity entity; // node, render mesh component
        CORE_NS::Entity mesh;   // mesh component
        uint64_t layerMask { LayerConstants::DEFAULT_LAYER_MASK };
        CORE_NS::IComponentManager::ComponentId jointId { CORE_NS::IComponentManager::INVALID_COMPONENT_ID };
        CORE_NS::IComponentManager::ComponentId prevJointId { CORE_NS::IComponentManager::INVALID_COMPONENT_ID };

        BASE_NS::Math::Mat4X4 mtx;       // world matrix
        BASE_NS::Math::Mat4X4 prevWorld; // previous world matrix
    };
    using BatchDataVector = BASE_NS::vector<BatchData>;

private:
    struct SceneBoundingVolumeHelper {
        BASE_NS::Math::Vec3 sumOfSubmeshPoints { 0.0f, 0.0f, 0.0f };
        uint32_t submeshCount { 0 };

        BASE_NS::Math::Vec3 minAABB { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max() };
        BASE_NS::Math::Vec3 maxAABB { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
            -std::numeric_limits<float>::max() };
    };
    struct MeshProcessData {
        const uint64_t layerMask { 0 };
        const uint32_t batchInstanceCount { 0 };
        const CORE_NS::Entity& renderMeshEntity;
        const CORE_NS::Entity& meshEntity;
        const MeshComponent& meshComponent;
        const RenderMeshComponent& renderMeshComponent;
        const BASE_NS::Math::Mat4X4& world;
        const BASE_NS::Math::Mat4X4& prevWorld;
        SceneBoundingVolumeHelper& sceneBoundingVolume;
    };
    struct LightProcessData {
        const uint64_t layerMask { 0 };
        const CORE_NS::Entity& entity;
        const LightComponent& lightComponent;
        const BASE_NS::Math::Mat4X4& world;
        RenderScene& renderScene;
        uint32_t& spotLightIndex;
    };
    struct SkinProcessData {
        const JointMatricesComponent* const jointMatricesComponent { nullptr };
        const PreviousJointMatricesComponent* const prevJointMatricesComponent { nullptr };
    };

    void SetDataStorePointers(RENDER_NS::IRenderDataStoreManager& manager);
    // returns the instance's valid scene component
    RenderConfigurationComponent GetRenderConfigurationComponent();
    CORE_NS::Entity ProcessScene(const RenderConfigurationComponent& sc);
    void ProcessSubmesh(const MeshProcessData& mpd, const MeshComponent::Submesh& submesh, const uint32_t meshIndex,
        const uint32_t subMeshIdx, const uint32_t skinJointIndex, const MinAndMax& mam, const bool isNegative);
    void ProcessMesh(const MeshProcessData& mpd, const MinAndMax& batchMam, const SkinProcessData& spd);
    void ProcessRenderables(SceneBoundingVolumeHelper& sceneBoundingVolumeHelper);
    void ProcessBatchRenderables(SceneBoundingVolumeHelper& sceneBoundingVolumeHelper);
    void CalculateSceneBounds(const SceneBoundingVolumeHelper& sceneBoundingVolumeHelper);
    void ProcessCameras(const RenderConfigurationComponent& sceneCompnent, const CORE_NS::Entity& mainCameraEntity,
        RenderScene& renderScene);
    void ProcessLight(const LightProcessData& lightProcessData);
    void ProcessLights(RenderScene& renderScene);
    void ProcessShadowCamera(const LightProcessData lightProcessData, RenderLight& light);
    bool ProcessReflection(const CORE_NS::ComponentQuery::ResultRow& row, const RenderCamera& camera);
    bool ProcessReflections(CORE_NS::Entity cameraEntity, const RenderScene& renderScene);
    void ProcessPostProcesses();
    void FetchFullScene();
    void EvaluateMaterialModifications(const MaterialComponent& matComp);
    // calculates min max from all submeshes and does min max for inout
    struct BatchIndices {
        uint32_t submeshIndex { ~0u };
        uint32_t batchStartIndex { ~0u };
        uint32_t batchEndCount { ~0u };
    };
    // with submeshIndex == ~0u processes all submeshes
    void CombineBatchWorldMinAndMax(const BatchDataVector& batchData, const BatchIndices& batchIndices,
        const MeshComponent& mesh, MinAndMax& mam) const;

    void ProcessRenderNodeGraphs(const RenderConfigurationComponent& renderConfig, const RenderScene& renderScene);
    void DestroyRenderDataStores();
    RENDER_NS::RenderHandleReference GetCameraRenderNodeGraph(
        const RenderScene& renderScene, const RenderCamera& renderCamera);
    RENDER_NS::RenderHandleReference GetSceneRenderNodeGraph(const RenderScene& renderScene);

    struct CameraData;
    CameraData UpdateAndGetPreviousFrameCameraData(
        const CORE_NS::Entity& entity, const BASE_NS::Math::Mat4X4& view, const BASE_NS::Math::Mat4X4& proj);

    bool active_ = true;
    CORE_NS::IEcs& ecs_;

    IRenderSystem::Properties properties_;

    IRenderDataStoreDefaultCamera* dsCamera_ = nullptr;
    IRenderDataStoreDefaultLight* dsLight_ = nullptr;
    IRenderDataStoreDefaultMaterial* dsMaterial_ = nullptr;
    IRenderDataStoreDefaultScene* dsScene_ = nullptr;
    RENDER_NS::IShaderManager* shaderMgr_ = nullptr;
    RENDER_NS::IGpuResourceManager* gpuResourceMgr_ = nullptr;

    INodeComponentManager* nodeMgr_ = nullptr;
    IRenderMeshBatchComponentManager* renderMeshBatchMgr_ = nullptr;
    IRenderMeshComponentManager* renderMeshMgr_ = nullptr;
    IWorldMatrixComponentManager* worldMatrixMgr_ = nullptr;
    IPreviousWorldMatrixComponentManager* prevWorldMatrixMgr_ = nullptr;
    IRenderConfigurationComponentManager* renderConfigMgr_ = nullptr;
    ICameraComponentManager* cameraMgr_ = nullptr;
    ILightComponentManager* lightMgr_ = nullptr;
    IPlanarReflectionComponentManager* planarReflectionMgr_ = nullptr;
    IMaterialExtensionComponentManager* materialExtensionMgr_ = nullptr;
    IMaterialComponentManager* materialMgr_ = nullptr;
    IMeshComponentManager* meshMgr_ = nullptr;
    IUriComponentManager* uriMgr_ = nullptr;
    INameComponentManager* nameMgr_ = nullptr;
    IEnvironmentComponentManager* environmentMgr_ = nullptr;
    IFogComponentManager* fogMgr_ = nullptr;
    IRenderHandleComponentManager* gpuHandleMgr_ = nullptr;
    ILayerComponentManager* layerMgr_ = nullptr;

    IJointMatricesComponentManager* jointMatricesMgr_ = nullptr;
    IPreviousJointMatricesComponentManager* prevJointMatricesMgr_ = nullptr;

    IPostProcessComponentManager* postProcessMgr_ = nullptr;

    IPicking* picking_ = nullptr;

    IGraphicsContext* graphicsContext_ = nullptr;
    IRenderUtil* renderUtil_ = nullptr;
    RENDER_NS::IRenderContext* renderContext_ = nullptr;

    IRenderPreprocessorSystem* renderPreprocessorSystem_ = nullptr;

    CORE_NS::ComponentQuery lightQuery_;
    CORE_NS::ComponentQuery renderableQuery_;
    CORE_NS::ComponentQuery reflectionsQuery_;

    BASE_NS::Math::Vec3 sceneBoundingSpherePosition_ { 0.0f, 0.0f, 0.0f };
    float sceneBoundingSphereRadius_ { 0.0f };

    CORE_NS::PropertyApiImpl<IRenderSystem::Properties> RENDER_SYSTEM_PROPERTIES;

    uint64_t totalTime_ { 0u };
    uint64_t deltaTime_ { 0u };
    uint64_t frameIndex_ { 0u };

    // additionally these could be stored to somewhere else
    // though this is ECS render system and the render node graphs are owned by ECS (RS)
    // these do not add overhead if the property bits are not set (only clear per frame)
    struct RenderProcessing {
        struct AdditionalCameraContainer {
            RENDER_NS::RenderHandleReference handle;
            RenderCamera::Flags flags { 0 };
            RenderCamera::RenderPipelineType renderPipelineType { RenderCamera::RenderPipelineType::FORWARD };
            uint64_t lastFrameIndex { 0 };   // frame when used
            bool enableAutoDestroy { true }; // some might not be allowed to be destroyed (e.g. color pre-pass)
            BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> postProcessName;
        };

        // all render node graphs (scene rng is always the first and this needs to be in order)
        BASE_NS::vector<RENDER_NS::RenderHandleReference> orderedRenderNodeGraphs;
        // camera component id with generation hash
        BASE_NS::unordered_map<uint64_t, AdditionalCameraContainer> camIdToRng;

        RENDER_NS::RenderHandleReference sceneRng;
        uint64_t sceneMainCamId { 0 };

        // reset every frame (flags for rendering hints)
        uint32_t frameFlags { 0u };

        // store created pods
        BASE_NS::vector<BASE_NS::string> postProcessPods;
    };
    RenderProcessing renderProcessing_;

    struct CameraData {
        BASE_NS::Math::Mat4X4 view;
        BASE_NS::Math::Mat4X4 proj;
        uint64_t lastFrameIndex { 0 }; // frame when used
    };
    // store previous frame matrices
    BASE_NS::unordered_map<CORE_NS::Entity, CameraData> cameraData_;

    BASE_NS::unordered_map<CORE_NS::Entity, BatchDataVector> batches_;
};
CORE3D_END_NAMESPACE()

#endif // CORE_ECS_RENDERSYSTEM_H
