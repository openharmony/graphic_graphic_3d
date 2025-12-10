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

#include <core/property/property_handle_util.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_shader_passes.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_shader_manager.h>
#include <render/property/property_types.h>

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
    IShaderManager& shaderMgr = er.context->GetDevice().GetShaderManager();

    RenderHandleReference shaderHandle0 =
        shaderMgr.GetShaderHandle("rendershaders://shader/GfxBackBufferRenderNodeTest.shader");
    RenderHandleReference shaderHandle1 =
        shaderMgr.GetShaderHandle("rendershaders://computeshader/GfxGpuBufferRenderNodeTest.shader");

    refcnt_ptr<IRenderDataStoreShaderPasses> dataStoreFs = er.context->GetRenderDataStoreManager().Create(
        IRenderDataStoreShaderPasses::UID, "RenderDataStoreShaderPasses_My");
    ASSERT_TRUE(dataStoreFs);
    ASSERT_EQ(0, dataStoreFs->GetFlags());

    dataStoreFs->Clear();

    {
        const array_view<const IRenderDataStoreShaderPasses::RenderPassData> rpd;
        dataStoreFs->AddRenderData("shader0", rpd);
        const auto getData = dataStoreFs->GetRenderData();
        ASSERT_TRUE(getData.empty());
    }
    {
        const array_view<const IRenderDataStoreShaderPasses::ComputePassData> rpd;
        dataStoreFs->AddComputeData("shader1", rpd);
        const auto getData = dataStoreFs->GetComputeData();
        ASSERT_TRUE(getData.empty());
    }
    {
        vector<IRenderDataStoreShaderPasses::RenderPassData> rpd;
        rpd.push_back({});
        dataStoreFs->AddRenderData("shader0", rpd);
        const auto getData = dataStoreFs->GetRenderData();
        ASSERT_EQ(getData.size(), 1);
    }
    {
        vector<IRenderDataStoreShaderPasses::ComputePassData> rpd;
        rpd.push_back({});
        dataStoreFs->AddComputeData("shader1", rpd);
        const auto getData = dataStoreFs->GetComputeData();
        ASSERT_EQ(getData.size(), 1);
    }
    // properties
    {
        IShaderPipelineBinder::Ptr binder = shaderMgr.CreateShaderPipelineBinder(shaderHandle0);
        ASSERT_TRUE(binder);

        // add bindings
        {
            auto* bindingProperties = binder->GetBindingProperties();
            ASSERT_TRUE(bindingProperties);
            {
                BindableImageWithHandleReference bindable;
                EXPECT_TRUE(CORE_NS::SetPropertyValue(*bindingProperties, "uImage",
                    CORE_NS::PropertyType::BINDABLE_IMAGE_WITH_HANDLE_REFERENCE_T, bindable));
                // shaderPipelineBinder_->BindImage(0U, 1U, bindable);
            }
            {
                BindableSamplerWithHandleReference bindable;
                EXPECT_TRUE(CORE_NS::SetPropertyValue(*bindingProperties, "uSampler",
                    CORE_NS::PropertyType::BINDABLE_SAMPLER_WITH_HANDLE_REFERENCE_T, bindable));
                // shaderPipelineBinder_->BindSampler(0U, 2U, bindable);
            }
            {
                BindableSamplerWithHandleReference bindable;
                EXPECT_TRUE(CORE_NS::SetPropertyValue(*bindingProperties, "uBuffer",
                    CORE_NS::PropertyType::BINDABLE_BUFFER_WITH_HANDLE_REFERENCE_T, bindable));
                // shaderPipelineBinder_->BindBuffer(0U, 3U, bindable);
            }
        }

        // custom props
        {
            auto* customProperties = binder->GetProperties();
            ASSERT_TRUE(customProperties);

            float param0 = 5.0f;
            Math::Vec2 param1 = { 2.0, 3.0 };

            EXPECT_TRUE(
                CORE_NS::SetPropertyValue(*customProperties, "param_0", CORE_NS::PropertyType::FLOAT_T, param0));
            EXPECT_TRUE(CORE_NS::SetPropertyValue(*customProperties, "param_1", CORE_NS::PropertyType::VEC2_T, param1));
            // or.. shaderPipelineBinder_->SetUniformData(0U, 0U, data);
        }
    }

    ASSERT_EQ("RenderDataStoreShaderPasses", dataStoreFs->GetTypeName());
    ASSERT_EQ("RenderDataStoreShaderPasses_My", dataStoreFs->GetName());
    ASSERT_EQ(IRenderDataStoreShaderPasses::UID, dataStoreFs->GetUid());
}
} // namespace

/**
 * @tc.name: RenderDataStoreFullscreenShaderTest
 * @tc.desc: Tests the API to implement a minimal fullscreen shader pass using IShaderManager,
 * IRenderDataStoreShaderPasses and IShaderPipelineBinder. Verifies that binding resources, setting custom properties,
 * and adding Graphics/Compute data is fine.
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreFullscreenShader, RenderDataStoreFullscreenShaderTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    CreateEngineSetup(engine);
    Validate(engine);
    DestroyEngine(engine);
}
