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

#include <nodecontext/render_node_graph_manager.h>
#include <util/render_util.h>

#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_class_register.h>
#include <render/device/intf_gpu_resource_cache.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_plugin.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/resource_handle.h>
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
void TestRenderTimings(const UTest::EngineResources& er)
{
    IRenderContext& context = *er.context.get();
    IRenderUtil& renderUtil = context.GetRenderUtil();

    {
        RenderTimings::Times times;
        times.begin = 4;
        times.beginBackend = 5;
        times.beginBackendPresent = 6;
        times.endBackend = 7;
        times.end = 8;
        ((RenderUtil&)renderUtil).SetRenderTimings(times);
    }
    {
        RenderTimings::Times times;
        times.begin = 20;
        times.beginBackend = 22;
        times.beginBackendPresent = 24;
        times.endBackend = 26;
        times.end = 28;
        ((RenderUtil&)renderUtil).SetRenderTimings(times);
    }
    {
        RenderTimings times = renderUtil.GetRenderTimings();
        ASSERT_EQ(4, times.prevFrame.begin);
        ASSERT_EQ(5, times.prevFrame.beginBackend);
        ASSERT_EQ(6, times.prevFrame.beginBackendPresent);
        ASSERT_EQ(7, times.prevFrame.endBackend);
        ASSERT_EQ(8, times.prevFrame.end);

        ASSERT_EQ(20, times.frame.begin);
        ASSERT_EQ(22, times.frame.beginBackend);
        ASSERT_EQ(24, times.frame.beginBackendPresent);
        ASSERT_EQ(26, times.frame.endBackend);
        ASSERT_EQ(28, times.frame.end);
    }
}
void TestRenderHandle(const UTest::EngineResources& er)
{
    IRenderContext& context = *er.context.get();
    IRenderUtil& renderUtil = context.GetRenderUtil();

    {
        IGpuResourceManager& gpuResourceMgr = er.device->GetGpuResourceManager();
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
        auto imageHandle = gpuResourceMgr.Create("image0", imageDesc);
        auto desc = renderUtil.GetRenderHandleDesc(imageHandle);
        ASSERT_EQ("image0", desc.name);
        ASSERT_EQ(1, desc.refCount);
        ASSERT_EQ(RenderHandleType::GPU_IMAGE, desc.type);
        ASSERT_EQ("", desc.additionalName);

        ASSERT_EQ(imageHandle.GetHandle(), renderUtil.GetRenderHandle(desc).GetHandle());
    }
    {
        IGpuResourceManager& gpuResourceMgr = er.device->GetGpuResourceManager();
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = 16;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE;
        bufferDesc.format = BASE_FORMAT_R16G16B16A16_SFLOAT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_INDEX_BUFFER_BIT;
        auto bufferHandle = gpuResourceMgr.Create("buffer0", bufferDesc);
        auto desc = renderUtil.GetRenderHandleDesc(bufferHandle);
        ASSERT_EQ("buffer0", desc.name);
        ASSERT_EQ(1, desc.refCount);
        ASSERT_EQ(RenderHandleType::GPU_BUFFER, desc.type);
        ASSERT_EQ("", desc.additionalName);

        ASSERT_EQ(bufferHandle.GetHandle(), renderUtil.GetRenderHandle(desc).GetHandle());
    }
    {
        IShaderManager& shaderMgr = er.device->GetShaderManager();
        string shaderName = "rendershaders://shader/RenderUtilTest.shader";
        auto shaderHandle = shaderMgr.GetShaderHandle(shaderName);
        auto desc = renderUtil.GetRenderHandleDesc(shaderHandle);
        ASSERT_EQ(shaderName, desc.name);
        ASSERT_EQ(1, desc.refCount);
        ASSERT_EQ(RenderHandleType::SHADER_STATE_OBJECT, desc.type);
        ASSERT_EQ("", desc.additionalName);

        ASSERT_EQ(shaderHandle.GetHandle(), renderUtil.GetRenderHandle(desc).GetHandle());
    }
    {
        IRenderNodeGraphManager& renderNodeGraphManager = er.context->GetRenderNodeGraphManager();
        RenderNodeGraphDesc rngDesc;
        rngDesc.renderNodeGraphName = "rng0";
        rngDesc.renderNodeGraphUri = "test://renderNodeGraph.rng";
        auto rngHandle = renderNodeGraphManager.Create(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_DYNAMIC, rngDesc, "rng0");
        ((RenderNodeGraphManager&)renderNodeGraphManager).HandlePendingAllocations();
        auto desc = renderUtil.GetRenderHandleDesc(rngHandle);
        ASSERT_EQ("test://renderNodeGraph.rng", desc.name);
        ASSERT_EQ("rng0", desc.additionalName);
        ASSERT_EQ(1, desc.refCount);
        ASSERT_EQ(RenderHandleType::RENDER_NODE_GRAPH, desc.type);
    }
    {
        IGpuResourceManager& gpuResourceMgr = er.device->GetGpuResourceManager();
        string samplerName = "CORE_DEFAULT_SAMPLER_LINEAR_CLAMP";
        RenderHandleDesc desc;
        desc.type = RenderHandleType::GPU_SAMPLER;
        desc.name = samplerName;
        auto samplerHandle = renderUtil.GetRenderHandle(desc);
        ASSERT_EQ(gpuResourceMgr.GetSamplerHandle(samplerName).GetHandle(), samplerHandle.GetHandle());
    }
    {
        IShaderManager& shaderMgr = er.device->GetShaderManager();
        GraphicsState state;
        IShaderManager::GraphicsStateCreateInfo info { "state0", state };
        auto stateHandle = shaderMgr.CreateGraphicsState(info);
        RenderHandleDesc desc;
        desc.type = RenderHandleType::GRAPHICS_STATE;
        desc.name = "state0";
        ASSERT_EQ(stateHandle.GetHandle(), renderUtil.GetRenderHandle(desc).GetHandle());
    }
    {
        IShaderManager& shaderMgr = er.device->GetShaderManager();
        PipelineLayout pipelineLayout;
        IShaderManager::PipelineLayoutCreateInfo info { "pl0", pipelineLayout };
        auto plHandle = shaderMgr.CreatePipelineLayout(info);
        RenderHandleDesc desc;
        desc.type = RenderHandleType::PIPELINE_LAYOUT;
        desc.name = "pl0";
        ASSERT_EQ(plHandle.GetHandle(), renderUtil.GetRenderHandle(desc).GetHandle());
    }
    {
        IShaderManager& shaderMgr = er.device->GetShaderManager();
        VertexInputDeclarationView vid;
        IShaderManager::VertexInputDeclarationCreateInfo info { "vid0", vid };
        auto vidHandle = shaderMgr.CreateVertexInputDeclaration(info);
        RenderHandleDesc desc;
        desc.type = RenderHandleType::VERTEX_INPUT_DECLARATION;
        desc.name = "vid0";
        ASSERT_EQ(vidHandle.GetHandle(), renderUtil.GetRenderHandle(desc).GetHandle());
    }
}
} // namespace

/**
 * @tc.name: RenderTimingsTest
 * @tc.desc: Tests RenderUtil timing methods by setting render timings and veryfying that getter functions return the
 * original timings.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderUtil, RenderTimingsTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);
    TestRenderTimings(engine);
    UTest::DestroyEngine(engine);
}

/**
 * @tc.name: RenderHandleTest
 * @tc.desc: Tests RenderUtil by querying for render handles of created resources.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderUtil, RenderHandleTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);
    TestRenderHandle(engine);
    UTest::DestroyEngine(engine);
}
