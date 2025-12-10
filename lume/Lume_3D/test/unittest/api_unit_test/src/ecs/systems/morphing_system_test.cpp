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

#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/morph_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/systems/intf_morphing_system.h>
#include <3d/render/intf_render_data_store_morph.h>
#include <3d/util/intf_mesh_util.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/datastore/intf_render_data_store_manager.h>

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
 * @tc.name: MorphingTest
 * @tc.desc: Tests for Morphing Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsMorphingSystem, MorphingTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto morphManager = GetManager<IMorphComponentManager>(*ecs);
    ASSERT_NE(nullptr, morphManager);
    auto morphingSystem = GetSystem<IMorphingSystem>(*ecs);
    ASSERT_NE(nullptr, morphingSystem);
    morphingSystem->SetActive(true);
    EXPECT_TRUE(morphingSystem->IsActive());
    EXPECT_EQ("MorphingSystem", morphingSystem->GetName());
    EXPECT_EQ(IMorphingSystem::UID, ((ISystem*)morphingSystem)->GetUid());
    EXPECT_NE(nullptr, morphingSystem->GetProperties());
    EXPECT_NE(nullptr, ((const IMorphingSystem*)morphingSystem)->GetProperties());
    EXPECT_EQ(ecs.get(), &morphingSystem->GetECS());

    Entity cube = graphicsContext->GetMeshUtil().GenerateCube(*ecs, "myCube", Entity {}, 1.0f, 1.0f, 1.0f);
    auto meshManager = GetManager<IMeshComponentManager>(*ecs);
    auto renderMeshManager = GetManager<IRenderMeshComponentManager>(*ecs);
    if (auto scopedHandle = meshManager->Write(renderMeshManager->Read(cube)->mesh); scopedHandle) {
        ASSERT_EQ(1, scopedHandle->submeshes.size());
        scopedHandle->submeshes[0].morphTargetCount = 1u;
        scopedHandle->submeshes[0].morphTargetBuffer = scopedHandle->submeshes[0].bufferAccess[0];
    }
    morphManager->Create(cube);
    if (auto scopedHandle = morphManager->Write(cube); scopedHandle) {
        scopedHandle->morphNames = { "name0", "name1", "name2" };
        scopedHandle->morphWeights = { 1.0f, 2.0f, 3.0f };
    }

    auto dsMorph = refcnt_ptr<IRenderDataStoreMorph>(
        renderContext->GetRenderDataStoreManager().GetRenderDataStore("RenderDataStoreMorph"));
    dsMorph->Clear();

    ecs->ProcessEvents();
    morphingSystem->Update(true, 1u, 1u);
    ASSERT_EQ(1, dsMorph->GetSubmeshes().size());
    EXPECT_EQ(3, dsMorph->GetSubmeshes()[0].activeTargets.size());

    dsMorph->Clear();
    morphManager->Destroy(cube);
    if (auto scopedHandle = meshManager->Write(renderMeshManager->Read(cube)->mesh); scopedHandle) {
        ASSERT_EQ(1, scopedHandle->submeshes.size());
        scopedHandle->submeshes[0].morphTargetBuffer = {};
    }

    ecs->ProcessEvents();
    morphingSystem->Update(true, 2u, 1u);
    ASSERT_EQ(1, dsMorph->GetSubmeshes().size());
    EXPECT_EQ(0, dsMorph->GetSubmeshes()[0].activeTargets.size());

    dsMorph->Clear();
}
