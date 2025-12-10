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

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/systems/intf_render_system.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/math/matrix_util.h>
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
 * @tc.name: GetRenderNodeGraphsTest
 * @tc.desc: Tests for Get Render Node Graphs Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsRenderSystem, GetRenderNodeGraphsTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto renderSystem = GetSystem<IRenderSystem>(*ecs);
    ASSERT_NE(nullptr, renderSystem);
    renderSystem->SetActive(true);
    EXPECT_TRUE(renderSystem->IsActive());
    EXPECT_EQ("RenderSystem", renderSystem->GetName());
    EXPECT_EQ(IRenderSystem::UID, ((ISystem*)renderSystem)->GetUid());
    EXPECT_NE(nullptr, renderSystem->GetProperties());
    EXPECT_NE(nullptr, ((const IRenderSystem*)renderSystem)->GetProperties());
    EXPECT_EQ(ecs.get(), &renderSystem->GetECS());

    // Increment some component counter before updating render system
    LightComponent lightInfo;
    Entity light = graphicsContext->GetSceneUtil().CreateLight(*ecs, lightInfo, Math::Vec3 {}, Math::Quat {});

    EXPECT_TRUE(renderSystem->Update(false, 1u, 1u));

    // only scene render node graph expected
    EXPECT_EQ(1, renderSystem->GetRenderNodeGraphs().size());

    Entity camera =
        graphicsContext->GetSceneUtil().CreateCamera(*ecs, Math::Vec3 {}, Math::Quat {}, 0.1f, 100.0f, 75.0f);

    // Update all systems
    ecs->Update(2u, 1u);

    EXPECT_NE(0, renderSystem->GetRenderNodeGraphs().size());
}
