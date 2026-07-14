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
#include <3d/ecs/components/graphics_state_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/systems/intf_render_system.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/render_data_defines_3d.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/containers/vector.h>
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
    Entity light = graphicsContext->GetSceneUtil().CreateLight(*ecs, lightInfo, Math::Vec3{}, Math::Quat{});

    EXPECT_TRUE(renderSystem->Update(false, 1u, 1u));

    // only scene render node graph expected
    EXPECT_EQ(1, renderSystem->GetRenderNodeGraphs().size());

    Entity camera = graphicsContext->GetSceneUtil().CreateCamera(*ecs, Math::Vec3{}, Math::Quat{}, 0.1f, 100.0f, 75.0f);

    // Update all systems
    ecs->Update(2u, 1u);

    EXPECT_NE(0, renderSystem->GetRenderNodeGraphs().size());
}

/**
 * @tc.name: GraphicsStateEmptyRenderSlotTest
 * @tc.desc: Drives RenderSystem::HandleGraphicsStateEvents for GraphicsStateComponent entries
 *           with an empty renderSlot — both blend-disabled (opaque) and blend-enabled
 *           (translucent) — and one with a non-empty renderSlot for the if-skip arm.
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsRenderSystem, GraphicsStateEmptyRenderSlotTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;
    auto graphicsStateManager = GetManager<IGraphicsStateComponentManager>(*ecs);
    ASSERT_NE(nullptr, graphicsStateManager);

    // Empty renderSlot, blend disabled -> RENDER_SLOT_FORWARD_OPAQUE branch.
    Entity opaqueEntity = ecs->GetEntityManager().Create();
    graphicsStateManager->Create(opaqueEntity);
    if (auto handle = graphicsStateManager->Write(opaqueEntity); handle) {
        handle->renderSlot = "";
        handle->graphicsState.colorBlendState.colorAttachmentCount = 1U;
        handle->graphicsState.colorBlendState.colorAttachments[0].enableBlend = false;
    }

    // Empty renderSlot, blend enabled -> RENDER_SLOT_FORWARD_TRANSLUCENT branch.
    Entity translucentEntity = ecs->GetEntityManager().Create();
    graphicsStateManager->Create(translucentEntity);
    if (auto handle = graphicsStateManager->Write(translucentEntity); handle) {
        handle->renderSlot = "";
        handle->graphicsState.colorBlendState.colorAttachmentCount = 1U;
        handle->graphicsState.colorBlendState.colorAttachments[0].enableBlend = true;
    }

    // Non-empty renderSlot -> skip the empty-slot branch entirely.
    Entity namedEntity = ecs->GetEntityManager().Create();
    graphicsStateManager->Create(namedEntity);
    if (auto handle = graphicsStateManager->Write(namedEntity); handle) {
        handle->renderSlot = "CORE3D_RS_DM_FW_OPAQUE";
    }

    ecs->ProcessEvents();
    ecs->Update(0U, 16667U);
    ecs->ProcessEvents();

    EXPECT_TRUE(graphicsStateManager->HasComponent(opaqueEntity));
    EXPECT_TRUE(graphicsStateManager->HasComponent(translucentEntity));
    EXPECT_TRUE(graphicsStateManager->HasComponent(namedEntity));
}

/**
 * @tc.name: SetPropertiesRoundtripAndForeignOwnerEarlyReturn
 * @tc.desc: Drives RenderSystem::SetProperties for both arms of the owner-guard at L1442 —
 *           roundtrip through the system's own handle plus a foreign handle from another
 *           system that triggers the early return.
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsRenderSystem, SetPropertiesRoundtripAndForeignOwnerEarlyReturn, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto* renderSystem = GetSystem<IRenderSystem>(*ecs);
    ASSERT_NE(nullptr, renderSystem);

    IPropertyHandle* selfHandle = renderSystem->GetProperties();
    ASSERT_NE(nullptr, selfHandle);

    if (auto scoped = ScopedHandle<IRenderSystem::Properties>(selfHandle); scoped) {
        scoped->dataStorePrefix = "Local";
    }
    renderSystem->SetProperties(*selfHandle);

    auto* rpSystem = GetSystem<IRenderPreprocessorSystem>(*ecs);
    ASSERT_NE(nullptr, rpSystem);
    IPropertyHandle* foreign = rpSystem->GetProperties();
    if (foreign && foreign != selfHandle) {
        renderSystem->SetProperties(*foreign);
    }
}

/**
 * @tc.name: ColorPrePassMultiViewCountDoesNotOverflow
 * @tc.desc: The color pre-pass camera is built by copying the base camera (including its multi-view count) and
 *           refilling it from the pre-pass camera component. A base camera already holding the maximum number
 *           of multi-view children plus a pre-pass camera contributing another must not push the count past the
 *           fixed array capacity. No produced RenderCamera may report a multiViewCameraCount above the capacity.
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsRenderSystem, ColorPrePassMultiViewCountDoesNotOverflow, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto* renderSystem = GetSystem<IRenderSystem>(*ecs);
    ASSERT_NE(nullptr, renderSystem);
    renderSystem->SetActive(true);

    const auto& sceneUtil = graphicsContext->GetSceneUtil();
    auto cameraMgr = GetManager<ICameraComponentManager>(*ecs);
    ASSERT_NE(nullptr, cameraMgr);

    const auto makeMultiViewChild = [&]() {
        const Entity child = sceneUtil.CreateCamera(*ecs, Math::Vec3{}, Math::Quat{}, 0.1f, 100.0f, 75.0f);
        if (auto handle = cameraMgr->Write(child); handle) {
            handle->sceneFlags |= CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT;
            handle->pipelineFlags |= CameraComponent::PipelineFlagBits::MULTI_VIEW_ONLY_BIT;
        }
        return child;
    };

    // Fill the base camera's multi-view layer cameras to capacity.
    vector<Entity> children;
    children.reserve(RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT);
    for (uint32_t i = 0; i < RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT; ++i) {
        children.push_back(makeMultiViewChild());
    }

    // Pre-pass camera with its own multi-view child: the extra child that overflows the base camera's array.
    const Entity prePassChild = makeMultiViewChild();
    const Entity prePassCamera = sceneUtil.CreateCamera(*ecs, Math::Vec3{}, Math::Quat{}, 0.1f, 100.0f, 75.0f);
    if (auto handle = cameraMgr->Write(prePassCamera); handle) {
        handle->multiViewCameras.push_back(prePassChild);
        // Not ACTIVE_RENDER: consumed only as the base camera's pre-pass source.
    }

    // Main camera: active main camera with a color pre-pass and the full set of multi-view children.
    const Entity mainCamera = sceneUtil.CreateCamera(*ecs, Math::Vec3{}, Math::Quat{}, 0.1f, 100.0f, 75.0f);
    if (auto handle = cameraMgr->Write(mainCamera); handle) {
        handle->sceneFlags |=
            CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT | CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT;
        handle->pipelineFlags |= CameraComponent::PipelineFlagBits::ALLOW_COLOR_PRE_PASS_BIT;
        handle->multiViewCameras = children;
        handle->prePassCamera = prePassCamera;
    }

    // Runs RenderSystem::ProcessCameras, which builds the base and pre-pass RenderCameras.
    ecs->Update(2u, 1u);

    // Read the cameras back and ensure none reports a multi-view count beyond the array capacity.
    BASE_NS::string cameraDsName;
    if (auto props = ScopedHandle<const IRenderSystem::Properties>(renderSystem->GetProperties()); props) {
        cameraDsName = props->dataStoreCamera;
    }
    ASSERT_FALSE(cameraDsName.empty());
    auto& dsManager = renderContext->GetRenderDataStoreManager();
    auto* dsCamera = static_cast<IRenderDataStoreDefaultCamera*>(dsManager.GetRenderDataStore(cameraDsName).get());
    ASSERT_NE(nullptr, dsCamera);
    for (const RenderCamera& renderCamera : dsCamera->GetCameras()) {
        EXPECT_LE(renderCamera.multiViewCameraCount, RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT);
    }
}
