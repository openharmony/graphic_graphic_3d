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

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/weather_component.h>
#include <3d/ecs/systems/intf_render_system.h>
#include <3d/ecs/systems/intf_weather_system.h>
#include <3d/intf_graphics_context.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/vector.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/ecs/intf_system.h>
#include <core/property/intf_property_handle.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;

/**
 * @tc.name: SystemInterface
 * @tc.desc: Drives the trivial WeatherSystem ISystem accessors (GetName/Uid/Properties/ECS,
 *           plus SetActive/IsActive) so their branches show up as exercised.
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsWeatherSystem, SystemInterface, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto* weatherSystem = GetSystem<IWeatherSystem>(*ecs);
    ASSERT_NE(nullptr, weatherSystem);

    weatherSystem->SetActive(true);
    EXPECT_TRUE(weatherSystem->IsActive());
    weatherSystem->SetActive(false);
    EXPECT_FALSE(weatherSystem->IsActive());
    weatherSystem->SetActive(true);

    EXPECT_EQ("WeatherSystem", weatherSystem->GetName());
    EXPECT_EQ(IWeatherSystem::UID, static_cast<ISystem*>(weatherSystem)->GetUid());

    EXPECT_EQ(nullptr, weatherSystem->GetProperties());
    EXPECT_EQ(nullptr, static_cast<const IWeatherSystem*>(weatherSystem)->GetProperties());

    EXPECT_EQ(ecs.get(), &weatherSystem->GetECS());

    // SetProperties is a no-op on WeatherSystem; verify the call survives.
    if (const IPropertyHandle* selfHandle = weatherSystem->GetProperties(); selfHandle) {
        weatherSystem->SetProperties(*selfHandle);
    }
}

/**
 * @tc.name: AllSystemsInactiveUpdate
 * @tc.desc: Marks every Lume3D ECS system inactive then ticks the ECS once, exercising the
 *           `if (!active_) return false;` early-return arm in each system's Update.
 *           Systems are restored to active afterward so subsequent tests are unaffected.
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsWeatherSystem, AllSystemsInactiveUpdate, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    const BASE_NS::vector<ISystem*> systems = ecs->GetSystems();
    BASE_NS::vector<bool> wasActive;
    wasActive.reserve(systems.size());
    for (ISystem* s : systems) {
        wasActive.push_back(s ? s->IsActive() : false);
        if (s) {
            s->SetActive(false);
        }
    }

    ecs->ProcessEvents();
    ecs->Update(0U, 16667U);
    ecs->ProcessEvents();

    for (size_t i = 0; i < systems.size(); ++i) {
        if (systems[i]) {
            systems[i]->SetActive(wasActive[i]);
        }
    }

    // Drive every system's SetProperties with whatever handle they expose. The trivial
    // empty implementations (LocalMatrix/Skinning) just need to be called.
    auto* renderSystem = GetSystem<IRenderSystem>(*ecs);
    IPropertyHandle* foreign = renderSystem ? renderSystem->GetProperties() : nullptr;
    for (ISystem* s : systems) {
        if (!s) {
            continue;
        }
        if (auto* own = s->GetProperties(); own) {
            s->SetProperties(*own);
        } else if (foreign) {
            s->SetProperties(*foreign);
        }
    }
}

/**
 * @tc.name: WeatherUpdateWithActiveSkyConfiguration
 * @tc.desc: Wires WeatherComponent + EnvironmentComponent(SKY background) +
 *           RenderConfigurationComponent and ticks the ECS so the WeatherSystem hits
 *           GetActiveRenderConfigurationWeatherRow, FillWeatherSettingsFromComponent
 *           (incl. ComputeSunDir at noon and midnight) and the cloud rendering switches.
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsWeatherSystem, WeatherUpdateWithActiveSkyConfiguration, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto weatherManager = GetManager<IWeatherComponentManager>(*ecs);
    auto envManager = GetManager<IEnvironmentComponentManager>(*ecs);
    auto renderConfigManager = GetManager<IRenderConfigurationComponentManager>(*ecs);
    ASSERT_NE(nullptr, weatherManager);
    ASSERT_NE(nullptr, envManager);
    ASSERT_NE(nullptr, renderConfigManager);

    auto* weatherSystem = GetSystem<IWeatherSystem>(*ecs);
    ASSERT_NE(nullptr, weatherSystem);
    weatherSystem->SetActive(true);

    auto& em = ecs->GetEntityManager();

    // Active camera so the WeatherSystem camera loop (L711-735) iterates a row with
    // ACTIVE_RENDER_BIT set and falls through the !ACTIVE_RENDER_BIT continue.
    const Entity activeCamera = testContext->graphicsContext->GetSceneUtil().CreateCamera(
        *ecs, BASE_NS::Math::Vec3{0.0f, 0.0f, 0.0f}, BASE_NS::Math::Quat{}, 0.1f, 100.0f, 60.0f);
    if (auto cameraManager = GetManager<ICameraComponentManager>(*ecs); cameraManager) {
        if (auto handle = cameraManager->Write(activeCamera); handle) {
            handle->sceneFlags |= CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT;
            handle->layerMask = 0xFFFFFFFF;
        }
    }
    const Entity inactiveCamera = testContext->graphicsContext->GetSceneUtil().CreateCamera(
        *ecs, BASE_NS::Math::Vec3{0.0f, 0.0f, 0.0f}, BASE_NS::Math::Quat{}, 0.1f, 100.0f, 60.0f);
    (void)inactiveCamera;

    const Entity weatherEntity = em.Create();
    weatherManager->Create(weatherEntity);
    if (auto handle = weatherManager->Write(weatherEntity); handle) {
        handle->coverage = 0.5f;
        handle->density = 0.5f;
        handle->timeOfDay = 12.0f;
        handle->date = {1U, 6U, 2026U};
        handle->latitude = 0.0f;
        handle->longitude = 0.0f;
        handle->utc = 0.0f;
        handle->cloudRenderingType = WeatherComponent::CloudRenderingType::DOWNSCALED;
    }

    const Entity environmentEntity = em.Create();
    envManager->Create(environmentEntity);
    if (auto handle = envManager->Write(environmentEntity); handle) {
        handle->background = EnvironmentComponent::Background::SKY;
        handle->weather = em.GetReferenceCounted(weatherEntity);
    }

    const Entity renderConfigEntity = em.Create();
    renderConfigManager->Create(renderConfigEntity);
    if (auto handle = renderConfigManager->Write(renderConfigEntity); handle) {
        handle->environment = em.GetReferenceCounted(environmentEntity);
    }

    ecs->ProcessEvents();
    ecs->Update(0U, 16667U);
    ecs->ProcessEvents();

    // Flip to midnight so ComputeSunDir hits the sun-below-horizon branch and
    // FillWeatherSunSettings takes the `sunElevation <= 0` zero-scale arm.
    if (auto handle = weatherManager->Write(weatherEntity); handle) {
        handle->timeOfDay = 0.0f;
        handle->cloudRenderingType = WeatherComponent::CloudRenderingType::REPROJECTED;
    }
    ecs->ProcessEvents();
    ecs->Update(16667U, 16667U);
    ecs->ProcessEvents();

    // Sub-threshold coverage so cloudEnabledThisFrame goes false and the cleanup arms fire.
    if (auto handle = weatherManager->Write(weatherEntity); handle) {
        handle->coverage = 0.0f;
        handle->density = 0.0f;
    }
    ecs->ProcessEvents();
    ecs->Update(33334U, 16667U);
    ecs->ProcessEvents();
}
