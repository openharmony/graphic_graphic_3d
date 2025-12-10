/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include <render/datastore/intf_render_data_store_render_post_processes.h>
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
    refcnt_ptr<IRenderDataStoreRenderPostProcesses> ds = er.context->GetRenderDataStoreManager().Create(
        IRenderDataStoreRenderPostProcesses::UID, "RenderDataStoreRenderPostProcesses_My");
    ASSERT_TRUE(ds);
    ASSERT_EQ(0, ds->GetFlags());

    ds->Clear();

    {
        const array_view<const IRenderDataStoreRenderPostProcesses::PostProcessData> rpd;
        ds->AddData("data0", rpd);
        const auto getData = ds->GetData("data0");
        ASSERT_TRUE(getData.postProcesses.empty());
    }
    {
        vector<IRenderDataStoreRenderPostProcesses::PostProcessData> rpd;
        rpd.push_back({});
        ds->AddData("data1", rpd);
        {
            const auto getData = ds->GetData("data1");
            ASSERT_EQ(getData.postProcesses.size(), 1);
        }

        // re-add -> overwrites
        ds->AddData("data1", rpd);
        {
            const auto getData = ds->GetData("data1");
            ASSERT_EQ(getData.postProcesses.size(), 1);
        }
    }
    {
        vector<IRenderDataStoreRenderPostProcesses::PostProcessData> rpd;
        IRenderDataStoreRenderPostProcesses::PostProcessData ppd;
        ppd.id = 3;
        rpd.push_back(ppd);
        ppd.id = 100;
        rpd.push_back(ppd);
        ds->AddData("data2", rpd);
        const auto getData = ds->GetData("data2");
        ASSERT_EQ(getData.postProcesses.size(), 2);

        if (getData.postProcesses.size() == 2) {
            ASSERT_EQ(getData.postProcesses[0U].id, 3);
            ASSERT_EQ(getData.postProcesses[1U].id, 100);
        }
    }

    ASSERT_EQ("RenderDataStoreRenderPostProcesses", ds->GetTypeName());
    ASSERT_EQ("RenderDataStoreRenderPostProcesses_My", ds->GetName());
    ASSERT_EQ(IRenderDataStoreRenderPostProcesses::UID, ds->GetUid());
}
} // namespace

/**
 * @tc.name: RenderDataStoreRenderPostProcessesTest
 * @tc.desc: Tests for IRenderDataStoreRenderPostProcesses to store data correctly.
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreRenderPostProcesses, RenderDataStoreRenderPostProcessesTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    CreateEngineSetup(engine);
    Validate(engine);
    DestroyEngine(engine);
}
