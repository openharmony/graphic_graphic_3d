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
#include <3d/render/intf_render_data_store_morph.h>
#include <3d/render/intf_render_node_scene_util.h>
#include <3d/util/intf_mesh_builder.h>
#include <base/containers/unique_ptr.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>

#include "render/render_node_scene_util.h"
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
 * @tc.name: GetInterfaceTest
 * @tc.desc: Tests for Get Interface Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeSceneUtil, GetInterfaceTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    EXPECT_TRUE(testContext);

    // RenderNodeSceneUtilImpl doesn't implement Ref/Unref so just a local variable.
    RenderNodeSceneUtilImpl sceneUtilInstance;
    EXPECT_TRUE(sceneUtilInstance.GetInterface(IRenderNodeSceneUtil::UID));
    EXPECT_TRUE(sceneUtilInstance.GetInterface(IInterface::UID));
    EXPECT_FALSE(sceneUtilInstance.GetInterface(IClassFactory::UID));

    const IRenderNodeSceneUtil& sceneUtilConst = sceneUtilInstance;
    EXPECT_TRUE(sceneUtilConst.GetInterface(IRenderNodeSceneUtil::UID));
    EXPECT_TRUE(sceneUtilConst.GetInterface(IInterface::UID));
    EXPECT_FALSE(sceneUtilConst.GetInterface(IClassFactory::UID));
}

/**
 * @tc.name: CustomCameraTargetsAttachmentIndexClamped
 * @tc.desc: A malformed render node graph subpass can carry depthAttachmentIndex/colorAttachmentIndices[0] beyond the
 *           fixed MAX_RENDER_PASS_ATTACHMENT_COUNT (8) attachmentHandles array. UpdateCustomCameraTargets must not
 *           write past that array.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeSceneUtil, CustomCameraTargetsAttachmentIndexClamped, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    ASSERT_TRUE(testContext);
    ASSERT_TRUE(testContext->renderContext);
    auto& gpuResourceMgr = testContext->renderContext->GetDevice().GetGpuResourceManager();

    // A valid (truthy) target so UpdateCustomCameraTargets attempts the attachment write.
    GpuImageDesc imageDesc;
    imageDesc.width = 4u;
    imageDesc.height = 4u;
    imageDesc.depth = 1u;
    imageDesc.format = BASE_FORMAT_R8G8B8A8_UNORM;
    imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
    imageDesc.imageType = CORE_IMAGE_TYPE_2D;
    imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
    imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    imageDesc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    RenderHandleReference target = gpuResourceMgr.Create("RNSU_OOB_target", imageDesc);
    ASSERT_TRUE(target);

    RenderNodeSceneUtilImpl util;
    RenderCamera camera;
    camera.renderResolution = {64u, 64u};
    camera.flags = RenderCamera::CameraFlagBits::CAMERA_FLAG_CUSTOM_TARGETS_BIT;
    camera.depthTarget = target;
    camera.colorTargets[0u] = target;

    // Heap-allocate so an unclamped write lands in an ASan heap redzone rather than adjacent stack slots.
    auto renderPass = BASE_NS::make_unique<RenderPass>();
    // Out-of-range indices, as a malformed subpass could supply (parser clamps counts, not values).
    renderPass->subpassDesc.depthAttachmentCount = 1u;
    renderPass->subpassDesc.depthAttachmentIndex = 100u;
    renderPass->subpassDesc.colorAttachmentCount = 1u;
    renderPass->subpassDesc.colorAttachmentIndices[0u] = 100u;

    // Without the clamp these write past the attachmentHandles array.
    util.UpdateRenderPassFromCamera(camera, *renderPass);
}
