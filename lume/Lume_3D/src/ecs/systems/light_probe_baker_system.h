/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef CORE3D_ECS_SYSTEMS_LIGHT_PROBE_BAKER_SYSTEM_H
#define CORE3D_ECS_SYSTEMS_LIGHT_PROBE_BAKER_SYSTEM_H

#include <ComponentTools/component_query.h>

#include <3d/ecs/systems/intf_light_probe_baker_system.h>
#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <3d/render/render_data_defines_3d.h>
#include <base/containers/byte_array.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <core/ecs/intf_ecs.h>
#include <core/plugin/intf_interface_helper.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/gpu_resource_desc.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
class IGpuResourceManager;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultCamera;
class IRenderHandleComponentManager;
class INodeComponentManager;
class IWorldMatrixComponentManager;
class IRenderConfigurationComponentManager;
class ICameraComponentManager;
class ILightProbeGroupComponentManager;

CORE3D_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class LightProbeBakerSystem final : public ILightProbeBakerSystem, private CORE_NS::IEcs::ComponentListener {
public:
    LightProbeBakerSystem(CORE_NS::IEcs& ecs);
    ~LightProbeBakerSystem();

    // ISystem
    BASE_NS::string_view GetName() const override;
    BASE_NS::Uid GetUid() const override;

    CORE_NS::IPropertyHandle* GetProperties() override;
    const CORE_NS::IPropertyHandle* GetProperties() const override;
    void SetProperties(const CORE_NS::IPropertyHandle&) override;

    bool IsActive() const override;
    void SetActive(bool state) override;

    void Initialize() override;
    void Uninitialize() override;
    bool Update(bool frameRenderingQueued, uint64_t time, uint64_t delta) override;

    const CORE_NS::IEcs& GetECS() const override;

    // Listener
    void OnComponentEvent(EventType type, const CORE_NS::IComponentManager& componentManager,
        BASE_NS::array_view<const CORE_NS::Entity> entities) override;

    // Bake management
    bool StartBake(BASE_NS::array_view<CORE_NS::Entity> lightProbeGroups);
    bool CancelBake();
    State GetStatus() const;

    // Rendering
    RENDER_NS::RenderHandleReference GetRenderNodeGraph() const;

private:
    bool UpdateLightProbeBake();
    bool InitializeGpuResources();
    void CollectLightProbesToBake();
    void CreatePerProbeBakingData(BASE_NS::vector<RenderCamera::LightProbeBakingData::PerProbeData>& perProbeData);
    uint64_t CreateLightProbeBakingCamera();

    struct LightProbeBakeInputs {
        BASE_NS::Math::Vec3 position{0.0f, 0.0f, 0.0f};
    };

    bool active_{true};
    CORE_NS::IEcs& ecs_;

    ILightProbeGroupComponentManager* lightProbeGroupMgr_{nullptr};
    INodeComponentManager* nodeMgr_{nullptr};
    IWorldMatrixComponentManager* worldMatrixMgr_{nullptr};
    IRenderHandleComponentManager* gpuHandleMgr_{nullptr};
    ICameraComponentManager* cameraMgr_{nullptr};
    IRenderConfigurationComponentManager* renderConfigMgr_{nullptr};

    RENDER_NS::IRenderContext* renderContext_{nullptr};
    RENDER_NS::IGpuResourceManager* gpuResourceMgr_{nullptr};

    // Access cameras set up by RenderSystem
    BASE_NS::string dataStoreScene_{"RenderDataStoreDefaultScene"};
    BASE_NS::string dataStoreCamera_{"RenderDataStoreDefaultCamera"};
    BASE_NS::refcnt_ptr<IRenderDataStoreDefaultCamera> dsCamera_;

    struct LightProbeBakeGPUResources {
        static constexpr uint32_t maxLightProbes = (1u << 16u);
        static constexpr uint32_t coeffsPerProbe = 9u;
        // Currently float4 instead of float3 buffer
        static constexpr uint32_t floatsPerCoeff = 4u;

        static constexpr uint32_t getBufferSizeBytes(uint32_t probeCount)
        {
            return probeCount * coeffsPerProbe * floatsPerCoeff * 4u;
        }

        static constexpr uint32_t cubemapAtlasSize{3072u};
        static constexpr uint32_t cubemapFaceResolution{256u};
        static constexpr uint32_t atlasProbesPerRow{2u};
        static constexpr uint32_t atlasRows{12u};
        static constexpr uint32_t atlasTotalProbes{24u};
        static_assert(atlasProbesPerRow * 6u * cubemapFaceResolution == cubemapAtlasSize);
        static_assert(atlasRows * cubemapFaceResolution == cubemapAtlasSize);
        static_assert(atlasRows * atlasProbesPerRow == atlasTotalProbes);

        RENDER_NS::RenderHandleReference cubemapAtlasTexture;
        CORE_NS::EntityReference cubemapAtlasTextureEntity;

        RENDER_NS::RenderHandleReference SHBuffer;
        CORE_NS::EntityReference SHBufferEntity;
        uint32_t SHBufferSizeProbes{0u};

        BASE_NS::unique_ptr<BASE_NS::ByteArray> SHReadbackData;
    };

    LightProbeBakeGPUResources gpuResources_;

    enum class LightProbeBakeStatusInternal {
        IDLE = 0,
        BAKE_TRIGGERED,
        BAKE_IN_PROGRESS,
        READBACK_TRIGGERED,
    };

    struct LightProbeBakeGroup {
        CORE_NS::Entity entity{};
        // Generation of component when bake started; use to discard results if group was modified
        uint32_t componentGeneration{0u};
        // Probe count when bake was started, to know how to read the results that were read back from GPU
        uint32_t probeBakeCount{0u};
    };

    struct BakeState {
        LightProbeBakeStatusInternal status{LightProbeBakeStatusInternal::IDLE};
        uint32_t iteration{0u};
        uint32_t iterationBakedProbesCount{0u};
        BASE_NS::vector<CORE_NS::Entity> lightProbeGroupEntities;
        BASE_NS::vector<LightProbeBakeInputs> lightProbeBakeInputs;
        BASE_NS::vector<LightProbeBakeGroup> bakeGroups;
        uint64_t cameraId{RenderSceneDataConstants::INVALID_ID};
        RENDER_NS::RenderHandleReference rng{};
    };

    BakeState currentBake_{};
};

class LightProbeBaker final : public CORE_NS::IInterfaceHelper<ILightProbeBaker> {
public:
    ~LightProbeBaker() override;
    void Initialize(CORE_NS::IEcs& ecs) override;
    bool StartBake(BASE_NS::array_view<CORE_NS::Entity> lightProbeGroups) override;
    bool CancelBake() override;
    ILightProbeBakerSystem::State GetStatus() const override;
    RENDER_NS::RenderHandleReference GetRenderNodeGraph() const override;

private:
    LightProbeBakerSystem* lightProbeBakerSystem_{nullptr};
};

CORE3D_END_NAMESPACE()

#endif  // CORE3D_ECS_SYSTEMS_LIGHT_PROBE_BAKER_SYSTEM_H
