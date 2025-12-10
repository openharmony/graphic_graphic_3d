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

#include <3d/render/intf_render_data_store_morph.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>

#include "render/datastore/render_data_store_morph.h"
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
 * @tc.name: AddSubmashTest
 * @tc.desc: Tests for Add Submash Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderDataStoreMorph, AddSubmashTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    constexpr BASE_NS::string_view dataStoreName = "DataStoreMorph0";
    auto dataStore = dsManager.Create(IRenderDataStoreMorph::UID, dataStoreName.data());
    ASSERT_TRUE(dataStore);
    auto dataStoreMorph = static_cast<RenderDataStoreMorph*>(dataStore.get());

    dataStoreMorph->Init({ 2u });
    {
        RenderDataMorph::Submesh submesh;
        submesh.vertexCount = 16u;
        dataStoreMorph->AddSubmesh(submesh);
    }
    {
        RenderDataMorph::Submesh submesh;
        submesh.vertexCount = 32u;
        dataStoreMorph->AddSubmesh(submesh);
    }
    auto submeshes = dataStoreMorph->GetSubmeshes();
    ASSERT_EQ(2u, submeshes.size());
    EXPECT_EQ(16u, submeshes[0].vertexCount);
    EXPECT_EQ(32u, submeshes[1].vertexCount);

    dataStoreMorph->Clear();
    EXPECT_TRUE(dataStoreMorph->GetSubmeshes().empty());
}
