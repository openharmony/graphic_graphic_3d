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

#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_shader_passes.h>
#include <render/intf_render_context.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace RENDER_NS;

/**
 * @tc.name: RenderDataStoreShaderPassesByNameTest
 * @tc.desc: Exercises the named (string_view name) getters and adders on
 *          IRenderDataStoreShaderPasses: missing-name lookups, no-op guards
 *          on empty inputs, and round-tripping a named block of render/compute
 *          pass data with empty shaderBinder lists.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderDataStoreShaderPasses, RenderDataStoreShaderPassesByNameTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);

    refcnt_ptr<IRenderDataStore> dataStore =
        engine.context->GetRenderDataStoreManager().Create(IRenderDataStoreShaderPasses::UID, "ShaderPassesDataStore");
    auto* ds = static_cast<IRenderDataStoreShaderPasses*>(dataStore.get());
    ASSERT_NE(nullptr, ds);

    // Missing-name lookups return empty values.
    ASSERT_TRUE(ds->GetRenderData("missing").empty());
    ASSERT_TRUE(ds->GetComputeData("missing").empty());
    ASSERT_EQ(0U, ds->GetRenderPropertyBindingInfo("missing").alignedByteSize);
    ASSERT_EQ(0U, ds->GetComputePropertyBindingInfo("missing").alignedByteSize);

    // Empty-name and empty-data inputs are no-ops (early-return guards).
    {
        IRenderDataStoreShaderPasses::RenderPassData rpData{};
        ds->AddRenderData("", {&rpData, 1});
        ds->AddRenderData("named", {});
    }
    {
        IRenderDataStoreShaderPasses::ComputePassData cpData{};
        ds->AddComputeData("", {&cpData, 1});
        ds->AddComputeData("named", {});
    }
    ASSERT_TRUE(ds->GetRenderData("named").empty());
    ASSERT_TRUE(ds->GetComputeData("named").empty());

    // Round-trip render pass data under a name. shaderBinders is intentionally
    // left empty so we hit the !shaderRef continue branch in the aligned-size loop.
    {
        IRenderDataStoreShaderPasses::RenderPassData rpData{};
        rpData.shaderBinders.resize(2);  // two null binder pointers
        ds->AddRenderData("rp1", {&rpData, 1});
        const auto out = ds->GetRenderData("rp1");
        ASSERT_EQ(1U, out.size());
        ASSERT_EQ(2U, out[0].shaderBinders.size());
        ASSERT_EQ(0U, ds->GetRenderPropertyBindingInfo("rp1").alignedByteSize);
    }
    {
        IRenderDataStoreShaderPasses::ComputePassData cpData{};
        cpData.shaderBinders.resize(2);
        ds->AddComputeData("cp1", {&cpData, 1});
        const auto out = ds->GetComputeData("cp1");
        ASSERT_EQ(1U, out.size());
        ASSERT_EQ(2U, out[0].shaderBinders.size());
        ASSERT_EQ(0U, ds->GetComputePropertyBindingInfo("cp1").alignedByteSize);
    }

    UTest::DestroyEngine(engine);
}
