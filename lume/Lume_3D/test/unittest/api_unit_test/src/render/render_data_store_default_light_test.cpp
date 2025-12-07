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

#include <algorithm>

#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/render_data_defines_3d.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/intf_renderer.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

/**
 * @tc.name: CreateDestroyTest
 * @tc.desc: Tests for Create Destroy Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreDefaultLight, CreateDestroyTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    static constexpr BASE_NS::string_view dataStoreName = "DataStoreDefaultLight0";

    auto dataStoreDefaultLight = dsManager.Create(IRenderDataStoreDefaultLight::UID, dataStoreName.data());
    ASSERT_TRUE(dataStoreDefaultLight);
    ASSERT_EQ("RenderDataStoreDefaultLight", dataStoreDefaultLight->GetTypeName());
    ASSERT_EQ(dataStoreName, dataStoreDefaultLight->GetName());
    ASSERT_EQ(IRenderDataStoreDefaultLight::UID, dataStoreDefaultLight->GetUid());
    ASSERT_EQ(0u, dataStoreDefaultLight->GetFlags());
    EXPECT_TRUE(dsManager.GetRenderDataStore(dataStoreName));
    // Destruction is deferred
    dataStoreDefaultLight.reset();

    // Render with no render node graph just to trigger destruction
    renderContext->GetRenderer().RenderFrame({});
    EXPECT_FALSE(dsManager.GetRenderDataStore(dataStoreName));
}

/**
 * @tc.name: GetAddLightTest
 * @tc.desc: Tests for Get Add Light Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreDefaultLight, GetAddLightTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    constexpr BASE_NS::string_view dataStoreName = "DataStoreDefaultLight0";
    auto dataStore = dsManager.Create(IRenderDataStoreDefaultLight::UID, dataStoreName.data());
    ASSERT_TRUE(dataStore);

    auto dataStoreDefaultLight = static_cast<IRenderDataStoreDefaultLight*>(dataStore.get());

    // Add lights
    {
        RenderLight light;
        light.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        light.lightUsageFlags =
            RenderLight::LIGHT_USAGE_DIRECTIONAL_LIGHT_BIT | RenderLight::LIGHT_USAGE_SHADOW_LIGHT_BIT;
        light.id = 0u;
        dataStoreDefaultLight->AddLight(light);
    }
    {
        RenderLight light;
        light.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        light.lightUsageFlags = RenderLight::LIGHT_USAGE_SPOT_LIGHT_BIT | RenderLight::LIGHT_USAGE_SHADOW_LIGHT_BIT;
        light.id = 1u;
        dataStoreDefaultLight->AddLight(light);
    }
    {
        RenderLight light;
        light.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        light.lightUsageFlags = RenderLight::LIGHT_USAGE_POINT_LIGHT_BIT | RenderLight::LIGHT_USAGE_SHADOW_LIGHT_BIT;
        light.id = 2u;
        dataStoreDefaultLight->AddLight(light);
    }
    {
        auto lights = dataStoreDefaultLight->GetLights();
        ASSERT_EQ(3u, lights.size());
        EXPECT_EQ(0u, lights[0].id);
        EXPECT_EQ(1u, lights[1].id);
        EXPECT_EQ(2u, lights[2].id);
    }
    // Add more than maximum number of light
    {
        for (uint32_t i = 0; i < DefaultMaterialLightingConstants::MAX_LIGHT_COUNT + 5; ++i) {
            RenderLight light;
            light.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            light.lightUsageFlags = RenderLight::LIGHT_USAGE_DIRECTIONAL_LIGHT_BIT;
            dataStoreDefaultLight->AddLight(light);
        }
        auto lights = dataStoreDefaultLight->GetLights();
        EXPECT_EQ(DefaultMaterialLightingConstants::MAX_LIGHT_COUNT, lights.size());
    }
    // Destruction is deferred
    dataStore.reset();

    // Render with no render node graph just to trigger destruction
    renderContext->GetRenderer().RenderFrame({});
    EXPECT_FALSE(dsManager.GetRenderDataStore(dataStoreName));
}

/**
 * @tc.name: ShadowQuaityResolutionTest
 * @tc.desc: Tests for Shadow Quaity Resolution Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreDefaultLight, ShadowQuaityResolutionTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    constexpr BASE_NS::string_view dataStoreName = "DataStoreDefaultLight0";
    auto dataStore = dsManager.Create(IRenderDataStoreDefaultLight::UID, dataStoreName.data());
    ASSERT_TRUE(dataStore);

    auto dataStoreDefaultLight = static_cast<IRenderDataStoreDefaultLight*>(dataStore.get());

    IRenderDataStoreDefaultLight::ShadowQualityResolutions resolutions;
    resolutions.low = { 100u, 100u };
    resolutions.normal = { 200u, 200u };
    resolutions.high = { 300u, 300u };
    resolutions.ultra = { 400u, 400u };
    dataStoreDefaultLight->SetShadowQualityResolutions(resolutions, 0u);

    {
        IRenderDataStoreDefaultLight::ShadowTypes shadowTypes;
        shadowTypes.shadowQuality = IRenderDataStoreDefaultLight::ShadowQuality::LOW;
        dataStoreDefaultLight->SetShadowTypes(shadowTypes, 0u);
        Math::UVec2 resolution = dataStoreDefaultLight->GetShadowQualityResolution();
        EXPECT_EQ(resolutions.low.x, resolution.x);
        EXPECT_EQ(resolutions.low.y, resolution.y);
    }
    {
        IRenderDataStoreDefaultLight::ShadowTypes shadowTypes;
        shadowTypes.shadowQuality = IRenderDataStoreDefaultLight::ShadowQuality::NORMAL;
        dataStoreDefaultLight->SetShadowTypes(shadowTypes, 0u);
        Math::UVec2 resolution = dataStoreDefaultLight->GetShadowQualityResolution();
        EXPECT_EQ(resolutions.normal.x, resolution.x);
        EXPECT_EQ(resolutions.normal.y, resolution.y);
    }
    {
        IRenderDataStoreDefaultLight::ShadowTypes shadowTypes;
        shadowTypes.shadowQuality = IRenderDataStoreDefaultLight::ShadowQuality::HIGH;
        dataStoreDefaultLight->SetShadowTypes(shadowTypes, 0u);
        Math::UVec2 resolution = dataStoreDefaultLight->GetShadowQualityResolution();
        EXPECT_EQ(resolutions.high.x, resolution.x);
        EXPECT_EQ(resolutions.high.y, resolution.y);
    }
    {
        IRenderDataStoreDefaultLight::ShadowTypes shadowTypes;
        shadowTypes.shadowQuality = IRenderDataStoreDefaultLight::ShadowQuality::ULTRA;
        dataStoreDefaultLight->SetShadowTypes(shadowTypes, 0u);
        Math::UVec2 resolution = dataStoreDefaultLight->GetShadowQualityResolution();
        EXPECT_EQ(resolutions.ultra.x, resolution.x);
        EXPECT_EQ(resolutions.ultra.y, resolution.y);
    }
    // Destruction is deferred
    dataStore.reset();
    // Render with no render node graph just to trigger destruction
    renderContext->GetRenderer().RenderFrame({});
    EXPECT_FALSE(dsManager.GetRenderDataStore(dataStoreName));
}
