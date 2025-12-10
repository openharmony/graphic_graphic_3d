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

#include <render/device/intf_gpu_resource_manager.h>
#include <render/util/intf_render_frame_util.h>
#include <render/util/intf_render_util.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace {
void TestRenderFrameUtil(const UTest::EngineResources& er)
{
    IRenderContext& context = *er.context.get();
    IRenderUtil& renderUtil = context.GetRenderUtil();
    IRenderFrameUtil& rfUtil = renderUtil.GetRenderFrameUtil();
    IGpuResourceManager& gpuResourceMgr = er.device->GetGpuResourceManager();

    {
        rfUtil.CopyToCpu({}, IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE);
        const auto& copyData = rfUtil.GetFrameCopyData({});
        ASSERT_EQ(RenderHandle {}, copyData.handle.GetHandle());
        ASSERT_EQ(0u, copyData.copyFlags);
    }
    {
        GpuBufferDesc desc;
        desc.byteSize = 16;
        desc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE;
        desc.format = BASE_FORMAT_R16G16B16A16_SFLOAT;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.usageFlags = CORE_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        auto bufferHandle = gpuResourceMgr.Create(desc);
        // Buffer doesn't have CORE_BUFFER_USAGE_TRANSFER_SRC_BIT so copy is invalid
        rfUtil.CopyToCpu(bufferHandle, IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE);
        const auto& copyData = rfUtil.GetFrameCopyData(bufferHandle);
        ASSERT_EQ(RenderHandle {}, copyData.handle.GetHandle());
        ASSERT_EQ(0u, copyData.copyFlags);
    }
    {
        GpuImageDesc imageDesc;
        imageDesc.width = 16;
        imageDesc.height = 16;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R16G16B16A16_SFLOAT;
        imageDesc.imageTiling = ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = ImageType::CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        auto imageHandle = gpuResourceMgr.Create(imageDesc);
        // Image doesn't have CORE_IMAGE_USAGE_TRANSFER_SRC_BIT so copy is invalid
        rfUtil.CopyToCpu(imageHandle, IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE);
        const auto& copyData = rfUtil.GetFrameCopyData(imageHandle);
        ASSERT_EQ(RenderHandle {}, copyData.handle.GetHandle());
        ASSERT_EQ(0u, copyData.copyFlags);
    }
    {
        auto samplerHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
        rfUtil.CopyToCpu(samplerHandle, IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE);
        auto& copyData = rfUtil.GetFrameCopyData(samplerHandle);
        ASSERT_EQ(RenderHandle {}, copyData.handle.GetHandle());
    }
    {
        auto copyData = rfUtil.GetFrameCopyData();
        ASSERT_EQ(0, copyData.size());
    }
}
} // namespace

/**
 * @tc.name: RenderFrameUtilTest
 * @tc.desc: Tests for IRenderFrameUtil's data copy functions and their validation by issuing invalid copies.
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderFrameUtil, RenderFrameUtilTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    CreateEngineSetup(engine);
    TestRenderFrameUtil(engine);
    DestroyEngine(engine);
}
