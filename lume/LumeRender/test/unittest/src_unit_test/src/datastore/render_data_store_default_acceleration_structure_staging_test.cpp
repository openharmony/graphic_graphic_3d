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

#include <datastore/render_data_store_default_acceleration_structure_staging.h>
#include <datastore/render_data_store_manager.h>

#include <render/datastore/intf_render_data_store_default_acceleration_structure_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
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
    refcnt_ptr<IRenderDataStore> dataStore = er.context->GetRenderDataStoreManager().Create(
        IRenderDataStoreDefaultAccelerationStructureStaging::UID, "AccelerationStructureDataStore");
    ASSERT_TRUE(dataStore);
    RenderDataStoreDefaultAccelerationStructureStaging* dataStoreAcc =
        static_cast<RenderDataStoreDefaultAccelerationStructureStaging*>(dataStore.get());
    ASSERT_EQ(0, dataStore->GetFlags());
    dataStoreAcc->Clear();
    dataStoreAcc->BuildAccelerationStructure({}, array_view<const AsGeometryInstancesDataWithHandleReference> {});
    dataStoreAcc->BuildAccelerationStructure({}, array_view<const AsGeometryTrianglesDataWithHandleReference> {});
    dataStoreAcc->BuildAccelerationStructure({}, array_view<const AsGeometryAabbsDataWithHandleReference> {});
    dataStoreAcc->CopyAccelerationStructureInstanceData({ {}, 0 }, {});
    bool hasStagingData = dataStoreAcc->HasStagingData();
    ASSERT_EQ(false, hasStagingData);
    AsConsumeStruct scs1 = dataStoreAcc->ConsumeStagingData();
    ASSERT_EQ(0, scs1.geometry.size());
}
} // namespace

/**
 * @tc.name: RenderDataStoreAccelerationStructureTest
 * @tc.desc: Tests IRenderDataStoreDefaultAccelerationStructureStaging with null acceleration structure buids and
 * copies.
 * @tc.type: FUNC
 */
UNIT_TEST(
    SRC_RenderDataStoreAccelerationStructure, RenderDataStoreAccelerationStructureTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);
    Validate(engine);
    UTest::DestroyEngine(engine);
}
