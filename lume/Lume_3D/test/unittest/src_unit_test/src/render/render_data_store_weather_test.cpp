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

#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/render_data_store_weather.h>

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
 * @tc.name: SetGetWeatherTest
 * @tc.desc: Tests the functionality of WeatherSetting's set and get functions.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderDataStoreWeather, SetGetWeatherTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto& dsManager = renderContext->GetRenderDataStoreManager();
    constexpr BASE_NS::string_view dataStoreName = "DataStoreWeather0";
    auto dataStore = dsManager.Create(RenderDataStoreWeather::UID, dataStoreName.data());
    ASSERT_TRUE(dataStore);
    auto dataStoreWeather = static_cast<RenderDataStoreWeather*>(dataStore.get());

    // Get default weather settings
    auto defaultSettings = dataStoreWeather->GetWeatherSettings();

    // Create modified settings with different values
    RenderDataStoreWeather::WeatherSettings testSettings;
    testSettings.timeOfDay = 8.5f;                        // Changed from default 12.0f
    testSettings.moonBrightness = 0.8f;                   // Changed from default 0.2f
    testSettings.nightGlow = 0.1f;                        // Changed from default 0.025f
    testSettings.skyViewBrightness = 0.9f;                // Changed from default 0.7f
    testSettings.groundColor = { 0.1f, 0.2f, 0.4f, 0.f }; // Changed from default vec3(0.3f)
    testSettings.windSpeed = 2000.0f;                     // Changed from default 1000.0f
    testSettings.coverage = 0.6f;                         // Changed from default 0.3f
    testSettings.density = 0.9f;                          // Changed from default 0.7f
    testSettings.cloudBottomAltitude = 3000.0f;           // Changed from default 2000.0f
    testSettings.cloudTopAltitude = 10000.0f;
    testSettings.cloudRenderingType = RenderDataStoreWeather::CloudRenderingType::FULL; // Changed from REPROJECTED

    testSettings.rayleighScatteringBase = { 10.0f, 20.0f,
        40.0f }; // Default rayleighScatteringBase = { 5.802f, 13.558f, 33.1f }
    testSettings.ozoneAbsorptionBase = { 10.0f, 20.0f,
        40.0f };                                 // Default ozoneAbsorptionBase = { 0.650f, 1.881f, 0.085f  }
    testSettings.mieAbsorptionBase = { 13.37 };  // Default mieAbsorptionBase = { 3.996f }
    testSettings.mieScatteringBase = { 42.21 };  // Default mieScatteringBase = { 4.4f }
    testSettings.windDir = { 0.5f, 1.0f, 0.5f }; // Default windDir = { 1.0f, 0.0f, 1.0f }

    uint64_t testId = 12345;

    dataStoreWeather->SetWeatherSettings(testId, testSettings);

    auto retrievedSettings = dataStoreWeather->GetWeatherSettings();

    // Verify the retrieved settings match the test settings (not defaults)
    EXPECT_FLOAT_EQ(retrievedSettings.timeOfDay, testSettings.timeOfDay);
    EXPECT_FLOAT_EQ(retrievedSettings.moonBrightness, testSettings.moonBrightness);
    EXPECT_FLOAT_EQ(retrievedSettings.nightGlow, testSettings.nightGlow);
    EXPECT_FLOAT_EQ(retrievedSettings.skyViewBrightness, testSettings.skyViewBrightness);
    EXPECT_FLOAT_EQ(retrievedSettings.windSpeed, testSettings.windSpeed);
    EXPECT_FLOAT_EQ(retrievedSettings.coverage, testSettings.coverage);
    EXPECT_FLOAT_EQ(retrievedSettings.density, testSettings.density);
    EXPECT_EQ(retrievedSettings.cloudRenderingType, testSettings.cloudRenderingType);

    // Test vector values
    EXPECT_FLOAT_EQ(retrievedSettings.rayleighScatteringBase.x, testSettings.rayleighScatteringBase.x);
    EXPECT_FLOAT_EQ(retrievedSettings.rayleighScatteringBase.y, testSettings.rayleighScatteringBase.y);
    EXPECT_FLOAT_EQ(retrievedSettings.rayleighScatteringBase.z, testSettings.rayleighScatteringBase.z);
    EXPECT_FLOAT_EQ(retrievedSettings.ozoneAbsorptionBase.x, testSettings.ozoneAbsorptionBase.x);
    EXPECT_FLOAT_EQ(retrievedSettings.ozoneAbsorptionBase.y, testSettings.ozoneAbsorptionBase.y);
    EXPECT_FLOAT_EQ(retrievedSettings.ozoneAbsorptionBase.z, testSettings.ozoneAbsorptionBase.z);

    EXPECT_FLOAT_EQ(retrievedSettings.mieScatteringBase, testSettings.mieScatteringBase);
    EXPECT_FLOAT_EQ(retrievedSettings.mieAbsorptionBase, testSettings.mieAbsorptionBase);

    EXPECT_FLOAT_EQ(retrievedSettings.groundColor.x, testSettings.groundColor.x);
    EXPECT_FLOAT_EQ(retrievedSettings.groundColor.y, testSettings.groundColor.y);
    EXPECT_FLOAT_EQ(retrievedSettings.groundColor.z, testSettings.groundColor.z);

    EXPECT_FLOAT_EQ(retrievedSettings.windDir.x, testSettings.windDir.x);
    EXPECT_FLOAT_EQ(retrievedSettings.windDir.y, testSettings.windDir.y);
    EXPECT_FLOAT_EQ(retrievedSettings.windDir.z, testSettings.windDir.z);

    // Verify they are different from defaults
    EXPECT_NE(retrievedSettings.timeOfDay, defaultSettings.timeOfDay);
    EXPECT_NE(retrievedSettings.moonBrightness, defaultSettings.moonBrightness);
    EXPECT_NE(retrievedSettings.nightGlow, defaultSettings.nightGlow);
    EXPECT_NE(retrievedSettings.skyViewBrightness, defaultSettings.skyViewBrightness);
    EXPECT_NE(retrievedSettings.windSpeed, defaultSettings.windSpeed);
    EXPECT_NE(retrievedSettings.coverage, defaultSettings.coverage);
    EXPECT_NE(retrievedSettings.density, defaultSettings.density);
    EXPECT_NE(retrievedSettings.cloudBottomAltitude, defaultSettings.cloudBottomAltitude);
    EXPECT_NE(retrievedSettings.cloudTopAltitude, defaultSettings.cloudTopAltitude);
    EXPECT_NE(retrievedSettings.cloudRenderingType, defaultSettings.cloudRenderingType);

    // Vector comparisons with defaults
    EXPECT_NE(retrievedSettings.rayleighScatteringBase.x, defaultSettings.rayleighScatteringBase.x);
    EXPECT_NE(retrievedSettings.rayleighScatteringBase.y, defaultSettings.rayleighScatteringBase.y);
    EXPECT_NE(retrievedSettings.rayleighScatteringBase.z, defaultSettings.rayleighScatteringBase.z);
    EXPECT_NE(retrievedSettings.ozoneAbsorptionBase.x, defaultSettings.ozoneAbsorptionBase.x);
    EXPECT_NE(retrievedSettings.ozoneAbsorptionBase.y, defaultSettings.ozoneAbsorptionBase.y);
    EXPECT_NE(retrievedSettings.ozoneAbsorptionBase.z, defaultSettings.ozoneAbsorptionBase.z);

    EXPECT_NE(retrievedSettings.mieScatteringBase, defaultSettings.mieScatteringBase);
    EXPECT_NE(retrievedSettings.mieAbsorptionBase, defaultSettings.mieAbsorptionBase);

    EXPECT_NE(retrievedSettings.groundColor.x, defaultSettings.groundColor.x);
    EXPECT_NE(retrievedSettings.groundColor.y, defaultSettings.groundColor.y);
    EXPECT_NE(retrievedSettings.groundColor.z, defaultSettings.groundColor.z);
    EXPECT_NE(retrievedSettings.windDir.x, defaultSettings.windDir.x);
    EXPECT_NE(retrievedSettings.windDir.y, defaultSettings.windDir.y);
    EXPECT_NE(retrievedSettings.windDir.z, defaultSettings.windDir.z);
}
