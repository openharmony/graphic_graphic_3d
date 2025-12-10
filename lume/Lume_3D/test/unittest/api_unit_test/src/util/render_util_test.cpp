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

#include <3d/implementation_uids.h>
#include <3d/render/render_data_defines_3d.h>
#include <3d/util/intf_render_util.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_class_register.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>

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
 * @tc.name: GetRenderNodeGraphDescTest
 * @tc.desc: Tests for Get Render Node Graph Desc Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderUtil, GetRenderNodeGraphDescTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& renderUtil = graphicsContext->GetRenderUtil();

    {
        RenderCamera renderCamera;
        renderCamera.customRenderNodeGraphFile = "test://test_core3d_rng_hdrp.rng";
        renderCamera.flags = RenderCamera::CAMERA_FLAG_REFLECTION_BIT;
        auto desc = renderUtil.GetRenderNodeGraphDesc({}, renderCamera, 0u);
        EXPECT_EQ(14, desc.nodes.size());
    }
    {
        RenderScene renderScene;
        renderScene.customRenderNodeGraphFile = "test://test_core3d_rng_hdrp.rng";
        auto desc = renderUtil.GetRenderNodeGraphDesc(renderScene, 0u);
        EXPECT_EQ(14, desc.nodes.size());
    }
    {
        RenderCamera renderCamera;
        renderCamera.customPostProcessRenderNodeGraphFile = "test://test_core3d_rng_hdrp.rng";
        auto descs = renderUtil.GetRenderNodeGraphDescs({}, renderCamera, 0u);
        EXPECT_EQ(14, descs.postProcess.nodes.size());
    }
    {
        RenderCamera renderCamera;
        renderCamera.customRenderNodeGraphFile = "test://NonExistingRng.rng";
        auto desc = renderUtil.GetRenderNodeGraphDesc({}, renderCamera, 0u);
        EXPECT_EQ(0, desc.nodes.size());
    }
    {
        RenderCamera renderCamera;
        renderCamera.flags = 0u;
        renderCamera.renderPipelineType = RenderCamera::RenderPipelineType::LIGHT_FORWARD;
        auto desc = renderUtil.GetRenderNodeGraphDesc({}, renderCamera, 0u);
        EXPECT_EQ("3drendernodegraphs://core3d_rng_cam_scene_lwrp.rng", desc.renderNodeGraphUri);
    }
    {
        RenderCamera renderCamera;
        renderCamera.flags = RenderCamera::CAMERA_FLAG_MSAA_BIT;
        renderCamera.renderPipelineType = RenderCamera::RenderPipelineType::LIGHT_FORWARD;
        auto desc = renderUtil.GetRenderNodeGraphDesc({}, renderCamera, 0u);
        EXPECT_EQ("3drendernodegraphs://core3d_rng_cam_scene_lwrp_msaa.rng", desc.renderNodeGraphUri);
    }
    {
        RenderCamera renderCamera;
        renderCamera.flags = 0u;
        auto desc = renderUtil.GetRenderNodeGraphDesc({}, renderCamera, 0u);
        EXPECT_EQ("3drendernodegraphs://core3d_rng_cam_scene_hdrp.rng", desc.renderNodeGraphUri);
    }
    {
        RenderCamera renderCamera;
        renderCamera.flags = RenderCamera::CAMERA_FLAG_MSAA_BIT;
        auto desc = renderUtil.GetRenderNodeGraphDesc({}, renderCamera, 0u);
        EXPECT_EQ("3drendernodegraphs://core3d_rng_cam_scene_hdrp_msaa.rng", desc.renderNodeGraphUri);
    }
}
