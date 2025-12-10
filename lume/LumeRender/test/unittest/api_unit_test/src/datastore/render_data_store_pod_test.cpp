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

#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using CORE_NS::IEngine;
using namespace RENDER_NS;

namespace {
void Validate(const UTest::EngineResources& er)
{
    // post processing configuration pod
    {
        refcnt_ptr<IRenderDataStorePod> dataStorePod =
            er.context->GetRenderDataStoreManager().Create(IRenderDataStorePod::UID, "PostProcessDataStore");
        PostProcessConfiguration ppConf;

        ppConf.enableFlags = PostProcessConfiguration::ENABLE_BLOOM_BIT | PostProcessConfiguration::ENABLE_BLUR_BIT |
                             PostProcessConfiguration::ENABLE_FXAA_BIT | PostProcessConfiguration::ENABLE_TAA_BIT;

        ppConf.bloomConfiguration.bloomQualityType = BloomConfiguration::BloomQualityType::QUALITY_TYPE_LOW;
        ppConf.bloomConfiguration.useCompute = true;

        ppConf.blurConfiguration.blurQualityType = BlurConfiguration::QUALITY_TYPE_HIGH;
        ppConf.blurConfiguration.blurType = BlurConfiguration::TYPE_NORMAL;
        ppConf.blurConfiguration.filterSize = 10.f;

        ppConf.fxaaConfiguration.quality = FxaaConfiguration::Quality::HIGH;
        ppConf.fxaaConfiguration.sharpness = FxaaConfiguration::Sharpness::SHARP;

        ppConf.taaConfiguration.quality = TaaConfiguration::Quality::HIGH;
        ppConf.taaConfiguration.sharpness = TaaConfiguration::Sharpness::SHARP;

        const array_view<const uint8_t> dataView = { reinterpret_cast<const uint8_t*>(&ppConf), sizeof(ppConf) };
        dataStorePod->CreatePod("PostProcessConfiguration", "PostProcessConfiguration", dataView);

        // Create again to trigger set with updated values
        ppConf.bloomConfiguration.useCompute = false;
        const array_view<const uint8_t> dataView2 = { reinterpret_cast<const uint8_t*>(&ppConf), sizeof(ppConf) };
        dataStorePod->CreatePod("PostProcessConfiguration", "PostProcessConfiguration", dataView2);
    }
    {
        refcnt_ptr<IRenderDataStore> dataStore =
            er.context->GetRenderDataStoreManager().GetRenderDataStore("PostProcessDataStore");
        IRenderDataStorePod* dataStorePod = static_cast<IRenderDataStorePod*>(dataStore.get());
        ASSERT_NE(nullptr, dataStorePod);
        ASSERT_EQ("PostProcessDataStore", dataStorePod->GetName());
        ASSERT_EQ(0, dataStorePod->GetFlags());

        dataStorePod->Clear();
        dataStore = er.context->GetRenderDataStoreManager().Create(IRenderDataStorePod::UID, "PostProcessDataStore");
        ASSERT_TRUE(dataStore);

        dataStore = er.context->GetRenderDataStoreManager().Create(
            IRenderDataStoreDefaultGpuResourceDataCopy::UID, "PostProcessDataStore");
        ASSERT_FALSE(dataStore);

        auto names = dataStorePod->GetPodNames("PostProcessConfiguration");
        ASSERT_EQ(1, names.size());
        ASSERT_EQ("PostProcessConfiguration", names[0]);

        names = dataStorePod->GetPodNames("NonExistingPod");
        ASSERT_EQ(0, names.size());

        const auto ppData = dataStorePod->Get("PostProcessConfiguration");
        const PostProcessConfiguration* ppConf = reinterpret_cast<PostProcessConfiguration const*>(ppData.data());
        ASSERT_EQ(sizeof(PostProcessConfiguration), ppData.size_bytes());
        ASSERT_EQ(ppConf->bloomConfiguration.useCompute, false);

        dataStorePod->Set("NonExistingPod", {});
        dataStorePod->DestroyPod("NonExistingType", "NonExistingPod");
        dataStorePod->DestroyPod("PostProcessConfiguration", "PostProcessConfiguration");
        names = dataStorePod->GetPodNames("PostProcessConfiguration");
        ASSERT_EQ(0, names.size());
    }
    // Destruction is deferred
    ASSERT_TRUE(er.context->GetRenderDataStoreManager().GetRenderDataStore("PostProcessDataStore"));
}
} // namespace

/**
 * @tc.name: RenderDataStorePodTest
 * @tc.desc: Tests for IRenderDataStore and IRenderDataStorePod functionality. These classes are used to store
 * user-defined render data in a "pod".
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStorePod, RenderDataStorePodTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    CreateEngineSetup(engine);
    Validate(engine);
    DestroyEngine(engine);
}
