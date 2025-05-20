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

#include <ComponentTools/component_query.h>

#include <3d/ecs/systems/intf_weather_system.h>
#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <core/ecs/intf_ecs.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/gpu_resource_desc.h>

#include "render/datastore/render_data_store_weather.h"

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
class IGpuResourceManager;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IRenderHandleComponentManager;
class INodeComponentManager;
class IMaterialComponentManager;
class IMeshComponentManager;
class IRenderMeshComponentManager;
class INameComponentManager;
class ITransformComponentManager;
class IRenderConfigurationComponentManager;
class IWaterRippleComponentManager;
class ICameraComponentManager;
class IEnvironmentComponentManager;
class IWeatherComponentManager;
CORE3D_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class WeatherSystem final : public IWeatherSystem, private CORE_NS::IEcs::ComponentListener {
public:
    /** Ripple plane object */
    struct RipplePlaneResources {
        CORE_NS::Entity entityID;
        // gpu resources
        CORE_NS::EntityReference rippleTextures[2];
        CORE_NS::EntityReference rippleVelocityTextures[2];
        CORE_NS::EntityReference rippleArgsBufferHandle;
    };

    WeatherSystem(CORE_NS::IEcs& ecs);
    ~WeatherSystem();

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

private:
    void ProcessWaterRipples();
    void UpdateCloudMaterial(CORE_NS::Entity camEntity);

    bool active_ { true };
    CORE_NS::IEcs& ecs_;
    RENDER_NS::IRenderContext& renderContext_;
    CORE3D_NS::IGraphicsContext& graphicsContext_;
    RENDER_NS::IGpuResourceManager& gpuResourceManager_;
    RENDER_NS::IRenderDataStoreManager& renderDataStoreManager_;

    CORE_NS::ComponentQuery query_;

    CORE3D_NS::IMaterialComponentManager& materialManager_;
    CORE3D_NS::IMeshComponentManager& meshManager_;
    CORE3D_NS::IRenderMeshComponentManager& renderMeshManager_;
    CORE3D_NS::IRenderHandleComponentManager& renderHandleManager_;
    CORE3D_NS::ITransformComponentManager& transformationManager_;
    CORE3D_NS::IRenderConfigurationComponentManager& renderConfigManager_;
    CORE3D_NS::ICameraComponentManager& camManager_;
    CORE3D_NS::IEnvironmentComponentManager& envManager_;
    CORE3D_NS::IWeatherComponentManager& weatherManager_;

    IWaterRippleComponentManager& rippleComponentManager_;
    uint32_t weatherGeneration_ {};
    BASE_NS::Math::Vec3 prevPlanePos_;
    BASE_NS::vector<BASE_NS::pair<CORE_NS::Entity, BASE_NS::Math::UVec2>> newRippleToAdd_;
    BASE_NS::unordered_map<CORE_NS::Entity, RipplePlaneResources> handleResources_;

    BASE_NS::refcnt_ptr<RenderDataStoreWeather> dataStoreWeather_;

    struct ObjectAndMaterial {
        CORE_NS::Entity entity;
        CORE_NS::Entity matEntity;
    };

    ObjectAndMaterial waterEffectsPlane;
    CORE_NS::Entity GetMaterialEntity(const CORE_NS::Entity& entity);
    RipplePlaneResources GetOrCreateSimulationResources(const CORE_NS::Entity& entity);

    struct CloudData {
        CORE_NS::Entity renderMeshEntity;
        CORE_NS::EntityReference matEntity;
        CORE_NS::EntityReference meshEntity;

        // will be stored to material
        CORE_NS::EntityReference cloudTexture;
        CORE_NS::EntityReference cloudPrevTexture;

        RENDER_NS::RenderHandleReference cloudTexHandle;
        RENDER_NS::RenderHandleReference cloudPrevTexHandle;

        RENDER_NS::GpuImageDesc gpuImageDesc {
            RENDER_NS::ImageType::CORE_IMAGE_TYPE_2D,
            RENDER_NS::ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
            BASE_NS::Format::BASE_FORMAT_R16G16B16A16_SFLOAT,
            RENDER_NS::ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
            RENDER_NS::ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                RENDER_NS::ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT,
            RENDER_NS::MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            0U,
            RENDER_NS::EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS,
            1U,
            1U,
            1U,
            1U,
            1U,
            RENDER_NS::SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,
            {},
        };

        RenderDataStoreWeather::CloudRenderingType cloudRenderingType {
            RenderDataStoreWeather::CloudRenderingType::REPROJECTED
        };
    };
    CloudData cloudMatData_;
};
CORE3D_END_NAMESPACE()
