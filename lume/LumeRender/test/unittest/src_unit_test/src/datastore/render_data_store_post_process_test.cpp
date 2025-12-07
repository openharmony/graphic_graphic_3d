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

#include <device/shader_manager.h>

#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/intf_render_data_store_post_process.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_device.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

/**
 * @tc.name: RenderDataStorePostProcessTest
 * @tc.desc: Tests for IRenderDataStorePostProcess interface by verifying that it works as expcted.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderDataStorePostProcess, RenderDataStorePostProcessTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    {
        UTest::CreateEngineSetup(engine);
    }
    refcnt_ptr<IRenderDataStore> dataStore =
        engine.context->GetRenderDataStoreManager().Create(IRenderDataStorePostProcess::UID, "PostProcessDataStore");
    IRenderDataStorePostProcess* dataStorePostProcess = static_cast<IRenderDataStorePostProcess*>(dataStore.get());
    {
        IRenderDataStorePostProcess::PostProcess::Variables vars;
        dataStorePostProcess->Create("postProcess");
        dataStorePostProcess->Create("postProcess");
        dataStorePostProcess->Create("postProcess", "ppConfig", {});
        dataStorePostProcess->Create("postProcess2", "ppConfig", {});
        ASSERT_EQ(15, dataStorePostProcess->Get("postProcess").size());
        dataStorePostProcess->Destroy("postProcess");
        ASSERT_EQ(0, dataStorePostProcess->Get("postProcess").size());
        dataStorePostProcess->Destroy("postProcess");
        ASSERT_EQ(0, dataStorePostProcess->Get("postProcess").size());
        dataStorePostProcess->Create("postProcess2", "ppConfig2", {});
        ASSERT_EQ(16, dataStorePostProcess->Get("postProcess2").size());
        dataStorePostProcess->Destroy("postProcess2", "ppConfig");
        ASSERT_EQ(15, dataStorePostProcess->Get("postProcess2").size());
        dataStorePostProcess->Set("postProcess2", "ppConfig2", vars);
        ASSERT_EQ(15, dataStorePostProcess->Get("postProcess2").size());
    }
    {
        IRenderDataStorePostProcess::PostProcess::Variables vars;
        vars.enabled = true;
        vars.factor = Math::Vec4(1.f, 2.f, 3.f, 4.f);
        dataStorePostProcess->Create("postProcess0", "ppName0", {});
        dataStorePostProcess->Set("postProcess0", "ppName0", vars);
        {
            auto realVars = dataStorePostProcess->Get("postProcess0", "ppName0");
            ASSERT_EQ("ppName0", realVars.name);
            ASSERT_EQ(vars.enabled, realVars.variables.enabled);
            ASSERT_EQ(vars.factor, realVars.variables.factor);
        }
        vars.enabled = false;
        dataStorePostProcess->Create("postProcess0", "ppName0", {});
        dataStorePostProcess->Set("postProcess0", "ppName0", vars);
        {
            auto realVars = dataStorePostProcess->Get("postProcess0", "ppName0");
            ASSERT_EQ("ppName0", realVars.name);
            ASSERT_EQ(vars.enabled, realVars.variables.enabled);
            ASSERT_EQ(vars.factor, realVars.variables.factor);
        }
        {
            dataStorePostProcess->Clear();
            auto realVars = dataStorePostProcess->Get("postProcess0", "invalid");
            ASSERT_EQ(0, realVars.name.size());
        }
    }
    {
        refcnt_ptr<IRenderDataStorePod> dataStorePod =
            engine.context->GetRenderDataStoreManager().GetRenderDataStore("RenderDataStorePod");
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

        dataStorePostProcess->Create("PostProcessConfiguration");
        ASSERT_TRUE(dataStorePostProcess->Contains("PostProcessConfiguration"));
        ASSERT_FALSE(dataStorePostProcess->Contains("NonExistingPPConf"));
        auto vals = dataStorePostProcess->Get("PostProcessConfiguration", "render_blur");
        ASSERT_EQ("render_blur", vals.name);
        ASSERT_TRUE(dataStorePostProcess->Contains("PostProcessConfiguration", "render_blur"));
        ASSERT_FALSE(dataStorePostProcess->Contains("PostProcessConfiguration", "nonExistingConf"));
    }
    {
        ASSERT_EQ("RenderDataStorePostProcess", dataStorePostProcess->GetTypeName());
        ASSERT_EQ("PostProcessDataStore", dataStorePostProcess->GetName());
        ASSERT_EQ(0, dataStorePostProcess->GetFlags());
        ASSERT_EQ(IRenderDataStorePostProcess::UID, dataStorePostProcess->GetUid());
    }
    {
        IRenderDataStorePostProcess::PostProcess::Variables vars;
        vars.userFactorIndex = 1u;
        vars.factor = { 1.f, 2.f, 3.f, 4.f };
        dataStorePostProcess->Create("testPP0", "testPP0", {});
        dataStorePostProcess->Set("testPP0", "testPP0", vars);
        {
            auto globalFactors = dataStorePostProcess->GetGlobalFactors("");
            ASSERT_EQ(0u, globalFactors.enableFlags);
        }
        {
            auto globalFactors = dataStorePostProcess->GetGlobalFactors("testPP0");
            ASSERT_EQ(vars.factor, globalFactors.userFactors[vars.userFactorIndex]);
        }
        dataStorePostProcess->Set("testPP0", { &vars, 1 });
    }
    {
        IShaderManager& shaderMgr = engine.device->GetShaderManager();
        string types[12] = { "vec4", "uvec4", "ivec4", "vec3", "uvec3", "ivec3", "vec2", "uvec2", "ivec2", "float",
            "uint", "int" };
        string values[12] = { "[0.0, 1.0, 2.0, 3.0]", "[0, 1, 2, 3]", "[0, 1, 2, 3]", "[0.0, 1.0, 2.0]", "[0, 1, 2]",
            "[0, 1, 2]", "[0.0, 1.0]", "[0, 1]", "[0, 1]", "0.4", "2", "3" };
        for (uint32_t i = 0; i < countof(types); ++i) {
            ShaderCreateData data;
            data.path = "rendershaders://shader/SinglePostProcessRenderNodeTest.shader";
            string pair = "[";
            pair += "{";
            pair += "\"type\": \"" + types[i] + "\", ";
            pair += "\"value\": " + values[i];
            pair += "}";
            pair += "]";
            string metadata = "[{\"globalFactor\": " + pair + "}, {\"customProperties\": [{\"data\":" + pair + "}]}]";
            data.materialMetadata = metadata;
            auto shaderHandle = ((ShaderManager&)shaderMgr).Create(data, { to_string(i), "" }, {});
            ASSERT_NE(RenderHandle {}, shaderHandle.GetHandle());
            IRenderDataStorePostProcess::PostProcess::Variables vars;
            vars.userFactorIndex = 1u;
            vars.factor = { 1.f, 2.f, 3.f, 4.f };
            dataStorePostProcess->Create("testPP1", "testPP1", shaderHandle);
            dataStorePostProcess->Set("testPP1", "testPP1", vars);
            auto globalFactors = dataStorePostProcess->GetGlobalFactors("testPP0");
            ASSERT_EQ(vars.factor, globalFactors.userFactors[vars.userFactorIndex]);
            dataStorePostProcess->Destroy("testPP1");
        }
    }
    {
        UTest::DestroyEngine(engine);
    }
}
