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

#include <core/intf_engine.h>
#include <core/property/property_handle_util.h>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/intf_render_data_store_shader_passes.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
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
static constexpr Math::UVec2 TEST_DATA_SIZE { 4u, 4u };
static constexpr size_t IMAGE_SIZE = TEST_DATA_SIZE.x * TEST_DATA_SIZE.y * 4u;
static constexpr size_t NUM_BYTES = IMAGE_SIZE * sizeof(float);
float imageData[IMAGE_SIZE];
static constexpr string_view INPUT_IMAGE_NAME_0 { "InputImage0" };
static constexpr string_view COPY_BUFFER_NAME_0 { "CopyBuffer0" };
static constexpr uint32_t PUSH_CONSTANT { 2u };
// NOTE: created in render node graph
static constexpr string_view OUTPUT_IMAGE_NAME_0 { "OutputImage0" };
static constexpr string_view SHADER_PASSES_TEST { "ShaderPassesTest" };

constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
constexpr const string_view RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY = "RenderDataStoreDefaultGpuResourceDataCopy";

struct TestResources {
    RenderHandleReference inputImageHandle0;
    RenderHandleReference copyBufferHandle0;

    unique_ptr<ByteArray> byteArray;

    RenderHandleReference renderNodeGraph;

    refcnt_ptr<IRenderDataStorePod> dataStorePod;

    refcnt_ptr<IRenderDataStoreShaderPasses> dataStoreShaderPasses;
    IRenderDataStoreShaderPasses::RenderPassData renderPassData;

    refcnt_ptr<IShaderPipelineBinder> shaderPipelineBinder;
    RenderHandleReference vertexBufferHandle;
    RenderHandleReference indexBufferHandle;
};

struct TestData {
    UTest::EngineResources engine;
    TestResources resources;
};

array_view<const uint8_t> CreateImageDataView()
{
    for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u] = 3.f; // R
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u] = 5.f; // G
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u] = 7.f; // B
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u] = 1.f; // A
        }
    }
    return array_view<const uint8_t> { reinterpret_cast<const uint8_t*>(imageData), sizeof(imageData) };
}

TestResources CreateTestResources(UTest::EngineResources& er, uint32_t constValue)
{
    TestResources res;
    // Input image
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R32G32B32A32_SFLOAT;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                               CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        const array_view<const uint8_t> dataView = CreateImageDataView();
        res.inputImageHandle0 = er.device->GetGpuResourceManager().Create(INPUT_IMAGE_NAME_0, imageDesc, dataView);
    }
    // Copy buffer
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = NUM_BYTES;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        res.copyBufferHandle0 = er.device->GetGpuResourceManager().Create(COPY_BUFFER_NAME_0, bufferDesc);
    }
    {
        res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://renderNodeGraphGfxFullscreenGenericRenderNodeTest.rng");
    }
    {
        res.dataStorePod = refcnt_ptr<IRenderDataStorePod>(
            er.context->GetRenderDataStoreManager().Create(IRenderDataStorePod::UID, "dataStore"));
        ShaderSpecializationRenderPod conf;
        conf.specializationConstantCount = 1;
        conf.specializationFlags[0].value = constValue;
        const array_view<const uint8_t> dataView = { reinterpret_cast<const uint8_t*>(&conf), sizeof(conf) };
        res.dataStorePod->CreatePod("ShaderSpecializationRenderPod", "fullscreenConfig", dataView);
        res.dataStorePod->CreatePod(
            "pushConstants", "pushConstants", { reinterpret_cast<const uint8_t*>(&PUSH_CONSTANT), sizeof(uint32_t) });
    }
    {
        auto& rdsMgr = er.context->GetRenderDataStoreManager();
        res.dataStoreShaderPasses = refcnt_ptr<IRenderDataStoreShaderPasses>(
            rdsMgr.Create(IRenderDataStoreShaderPasses::UID, "RenderDataStoreShaderPasses"));
    }
    {
        IShaderManager& shaderMgr = er.context->GetDevice().GetShaderManager();
        constexpr IShaderManager::ShaderFilePathDesc shaderPathDesc { "rendershaders://", "", "", "" };
        // shaderMgr.LoadShaderFiles(shaderPathDesc);

        RenderHandleReference shader =
            shaderMgr.GetShaderHandle("rendershaders://shader/GfxBackBufferRenderNodeTest.shader");

        res.shaderPipelineBinder = shaderMgr.CreateShaderPipelineBinder(shader);

        IShaderPipelineBinder::DrawCommand drawCmd {};
        drawCmd.vertexCount = 0u;
        drawCmd.indexCount = 3u;
        drawCmd.instanceCount = 1u;
        res.shaderPipelineBinder->SetDrawCommand(drawCmd);

        if (auto* bindingProperties = res.shaderPipelineBinder->GetBindingProperties(); bindingProperties) {
            {
                BindableImageWithHandleReference bindable;
                bindable.handle = res.inputImageHandle0;
                CORE_NS::SetPropertyValue(*bindingProperties, "uImage",
                    CORE_NS::PropertyType::BINDABLE_IMAGE_WITH_HANDLE_REFERENCE_T, bindable);
            }
            {
                IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();
                BindableSamplerWithHandleReference bindable;
                bindable.handle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
                CORE_NS::SetPropertyValue(*bindingProperties, "uSampler",
                    CORE_NS::PropertyType::BINDABLE_SAMPLER_WITH_HANDLE_REFERENCE_T, bindable);
            }
        }

        if (auto* customProperties = res.shaderPipelineBinder->GetProperties(); customProperties) {
            float param0 = 0.5f;
            Math::Vec2 param1 { 2.0f, 3.0f };

            CORE_NS::SetPropertyValue(*customProperties, "param_0", CORE_NS::PropertyType::FLOAT_T, param0);
            CORE_NS::SetPropertyValue(*customProperties, "param_1", CORE_NS::PropertyType::VEC2_T, param1);
        }
        {
            IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();

            struct Vertex {
                float px, py;
                float ux, uy;
            };
            const Vertex vertices[3] = {
                { -1.0f, -1.0f, 0.0f, 0.0f },
                { 3.0f, -1.0f, 2.0f, 0.0f },
                { -1.0f, 3.0f, 0.0f, 2.0f },
            };

            GpuBufferDesc vbDesc {};
            vbDesc.byteSize = sizeof(vertices);
            vbDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
            vbDesc.usageFlags = CORE_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            vbDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            const auto vbDataView = array_view(reinterpret_cast<const uint8_t*>(vertices), sizeof(vertices));

            res.vertexBufferHandle = gpuResourceMgr.Create("TestFullscreenVB", vbDesc, vbDataView);

            VertexBufferWithHandleReference vbRef {};
            vbRef.bufferHandle = res.vertexBufferHandle;
            vbRef.bufferOffset = 0;
            vbRef.byteSize = vbDesc.byteSize;

            res.shaderPipelineBinder->BindVertexBuffers({ &vbRef, 1u });
        }
        {
            IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();

            // 3 indices for the triangle
            const uint16_t indices[3] = { 0, 1, 2 };

            GpuBufferDesc ibDesc {};
            ibDesc.byteSize = sizeof(indices);
            ibDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
            ibDesc.usageFlags = CORE_BUFFER_USAGE_INDEX_BUFFER_BIT;
            ibDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            const auto ibDataView = array_view(reinterpret_cast<const uint8_t*>(indices), sizeof(indices));

            res.indexBufferHandle = gpuResourceMgr.Create("TestFullscreenIB", ibDesc, ibDataView);

            IndexBufferWithHandleReference ibRef {};
            ibRef.bufferHandle = res.indexBufferHandle;
            ibRef.bufferOffset = 0;
            ibRef.byteSize = ibDesc.byteSize;
            ibRef.indexType = CORE_INDEX_TYPE_UINT16;

            res.shaderPipelineBinder->BindIndexBuffer(ibRef);
        }
    }
    res.byteArray = make_unique<ByteArray>(NUM_BYTES);

    return res;
}

void DestroyTestResources(IDevice& device, TestResources& tr)
{
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    tr.inputImageHandle0 = {};
    tr.copyBufferHandle0 = {};
    tr.renderNodeGraph = {};
    tr.dataStorePod = {};
    tr.dataStoreShaderPasses = {};
    tr.shaderPipelineBinder = {};
    tr.vertexBufferHandle = {};
    tr.indexBufferHandle = {};
    tr.byteArray.reset();
}

void TickTest(TestData& td, int32_t frameCountToTick)
{
    UTest::EngineResources& er = td.engine;
    TestResources& tr = td.resources;

    for (int32_t idx = 0; idx < frameCountToTick; ++idx) {
        if (tr.dataStoreShaderPasses && tr.shaderPipelineBinder) {
            if (idx == 1) {
                IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();
                const RenderHandleReference outputImageHandle0 = gpuResourceMgr.GetImageHandle(OUTPUT_IMAGE_NAME_0);

                auto& rp = tr.renderPassData.renderPass;
                rp.subpassStartIndex = 0;

                rp.renderPassDesc.attachmentCount = 1;
                rp.renderPassDesc.subpassCount = 1;
                rp.renderPassDesc.renderArea = { 0, 0, TEST_DATA_SIZE.x, TEST_DATA_SIZE.y };

                rp.renderPassDesc.attachments[0].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
                rp.renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
                rp.renderPassDesc.attachmentHandles[0] = outputImageHandle0;

                rp.subpassDesc.colorAttachmentCount = 1;
                rp.subpassDesc.colorAttachmentIndices[0] = 0;
            }
            if (idx >= 1) {
                tr.renderPassData.shaderBinders.clear();
                tr.renderPassData.shaderBinders.push_back(tr.shaderPipelineBinder);
                tr.dataStoreShaderPasses->AddRenderData(SHADER_PASSES_TEST, { &tr.renderPassData, 1U });
            }
        }
        if (idx == 1) {
            auto& rdsMgr = er.context->GetRenderDataStoreManager();
            if (refcnt_ptr<IRenderDataStoreDefaultStaging> dsStaging =
                    rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING)) {
                BufferImageCopy bufferImageCopy;
                bufferImageCopy.bufferOffset = 0;
                bufferImageCopy.bufferRowLength = TEST_DATA_SIZE.x;
                bufferImageCopy.bufferImageHeight = TEST_DATA_SIZE.y;
                bufferImageCopy.imageSubresource =
                    ImageSubresourceLayers { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1u };
                bufferImageCopy.imageOffset = { 0, 0, 0 };
                bufferImageCopy.imageExtent = { TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, 1u };

                IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();
                const RenderHandleReference outputImageHandle0 = gpuResourceMgr.GetImageHandle(OUTPUT_IMAGE_NAME_0);

                dsStaging->CopyImageToBuffer(outputImageHandle0, tr.copyBufferHandle0, bufferImageCopy);
            }
        }

        if (idx == 2) {
            auto& rdsMgr = er.context->GetRenderDataStoreManager();
            if (refcnt_ptr<IRenderDataStoreDefaultGpuResourceDataCopy> dataStoreDataCopy =
                    rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY)) {
                IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                dataCopy.gpuHandle = tr.copyBufferHandle0;
                dataCopy.byteArray = tr.byteArray.get();
                dataStoreDataCopy->AddCopyOperation(dataCopy);
            }
        }

        er.engine->TickFrame();
        const RenderHandleReference inputs[] = { tr.inputImageHandle0 };
        er.context->GetRenderNodeGraphManager().SetRenderNodeGraphResources(
            tr.renderNodeGraph, { inputs, countof(inputs) }, {});

        if (idx == 0 || idx == 1) {
            er.context->GetRenderer().RenderFrame({ &tr.renderNodeGraph, 1u });
        } else {
            er.context->GetRenderer().RenderFrame({});
        }
    }
}

void Validate(const TestData& td, uint32_t constValue)
{
    ASSERT_EQ(NUM_BYTES, td.resources.byteArray->GetData().size());
    float* outputImageData = reinterpret_cast<float*>(td.resources.byteArray->GetData().data());
    for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
            float R = outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u];
            float G = outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u];
            float B = outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u];
            float A = outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u];

            if ((constValue + PUSH_CONSTANT) % 2 == 0) {
                ASSERT_EQ(5.f, R);
                ASSERT_EQ(7.f, G);
                ASSERT_EQ(3.f, B);
                ASSERT_EQ(1.f, A);
            } else {
                ASSERT_EQ(7.f, R);
                ASSERT_EQ(3.f, G);
                ASSERT_EQ(5.f, B);
                ASSERT_EQ(1.f, A);
            }
        }
    }
}

void TestFullscreenGenericRenderNode(DeviceBackendType backend, uint32_t constValue)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResources(testData.engine, constValue);
    }
    TickTest(testData, 3);
    Validate(testData, constValue);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: GfxFullscreenGenericRenderNodeTestVulkan
 * @tc.desc: Tests RenderNodeFullscreenGeneric by issuing a fullscreen pass with an input image and push constants. The
 * output is valitated in the end. Backend is Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxFullscreenGenericRenderNode, GfxFullscreenGenericRenderNodeTestVulkan, testing::ext::TestSize.Level1)
{
    TestFullscreenGenericRenderNode(DeviceBackendType::VULKAN, 0u);
    TestFullscreenGenericRenderNode(DeviceBackendType::VULKAN, 1u);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GfxFullscreenGenericRenderNodeTestOpenGL
 * @tc.desc: Tests RenderNodeFullscreenGeneric by issuing a fullscreen pass with an input image and push constants. The
 * output is valitated in the end. Backend is OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxFullscreenGenericRenderNode, GfxFullscreenGenericRenderNodeTestOpenGL, testing::ext::TestSize.Level1)
{
    TestFullscreenGenericRenderNode(UTest::GetOpenGLBackend(), 0u);
    TestFullscreenGenericRenderNode(UTest::GetOpenGLBackend(), 1u);
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
