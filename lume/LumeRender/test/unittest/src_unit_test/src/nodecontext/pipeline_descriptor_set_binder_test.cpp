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

#include <device/device.h>
#include <nodecontext/node_context_descriptor_set_manager.h>
#include <nodecontext/pipeline_descriptor_set_binder.h>

#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>

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
void TestPipelineDescriptorSetBinder(const UTest::EngineResources& engine)
{
    auto nodeContextDescriptorSetMgr = ((Device*)engine.device)->CreateNodeContextDescriptorSetManager();
    RenderHandleReference shaderHandle;
    if (engine.backend == DeviceBackendType::VULKAN) {
        shaderHandle = engine.device->GetShaderManager().GetShaderHandle(
            "rendershaders://shader/PipelineDescriptorSetBinderTest.shader");
    } else {
        shaderHandle = engine.device->GetShaderManager().GetShaderHandle(
            "rendershaders://shader/PipelineDescriptorSetBinderTest2.shader");
    }
    ASSERT_NE(RenderHandle {}, shaderHandle.GetHandle());
    auto pipelineLayout = engine.device->GetShaderManager().GetReflectionPipelineLayout(shaderHandle);
    DescriptorCounts dc;
    for (uint32_t setIdx = 0; setIdx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++setIdx) {
        const auto& setRef = pipelineLayout.descriptorSetLayouts[setIdx];
        if (setRef.set == PipelineLayoutConstants::INVALID_INDEX) {
            continue;
        }
        dc.counts.reserve(setRef.bindings.size());
        for (const auto& bindingRef : setRef.bindings) {
            dc.counts.emplace_back(
                DescriptorCounts::TypedCount { bindingRef.descriptorType, bindingRef.descriptorCount });
        }
    }
    nodeContextDescriptorSetMgr->ResetAndReserve(dc);
    auto binder = nodeContextDescriptorSetMgr->CreatePipelineDescriptorSetBinder(pipelineLayout);
    auto& gpuResourceMgr = engine.device->GetGpuResourceManager();
    auto samplerHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP").GetHandle();
    auto imageHandle = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE").GetHandle();
    GpuBufferDesc desc;
    desc.byteSize = 28;
    desc.usageFlags = CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    desc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE;
    desc.format = BASE_FORMAT_R32_SFLOAT;
    desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    auto bufferHandle = gpuResourceMgr.Create(desc).GetHandle();
    BindableSampler samplers[] = { { samplerHandle }, { samplerHandle } };
    BindableImage images[] = { { imageHandle }, { imageHandle } };
    BindableBuffer buffers[] = { { bufferHandle }, { bufferHandle } };
    binder->BindSamplers(0, 0, { samplers, countof(samplers) });
    binder->BindImages(0, 2, { images, countof(images) });
    binder->BindBuffers(0, 4, { buffers, countof(buffers) });
    ASSERT_TRUE(binder->GetPipelineDescriptorSetLayoutBindingValidity());
    ASSERT_EQ(1, binder->GetDescriptorSetCount());
    ASSERT_EQ(0, binder->GetFirstSet());
    auto descSetHandles = binder->GetDescriptorSetHandles(0u, 1u);
    ASSERT_EQ(1, descSetHandles.size());
    ASSERT_EQ(descSetHandles[0], binder->GetDescriptorSetHandle(0u));
    binder->PrintPipelineDescriptorSetLayoutBindingValidation();
    ASSERT_EQ(RenderHandle {}, binder->GetDescriptorSetHandle(100u));
    ASSERT_EQ(0, binder->GetDescriptorSetLayoutBindingResources(100u).bindings.size());
    {
        DescriptorSetBinder descSetBinder { {}, {} };
        BindableBuffer buffer;
        buffer.byteSize = 0;
        buffer.handle = {};
        descSetBinder.BindBuffer(0u, buffer, 0u);
        buffer.handle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_BUFFER, 0u);
        descSetBinder.BindBuffer(0u, buffer, 0u);
        descSetBinder.BindBuffer(0u, buffer.handle, buffer.byteOffset, buffer.byteSize);
        descSetBinder.BindBuffers(0u, { &buffer, 1 });
        descSetBinder.BindBuffers(0u, { &buffer.handle, 1 });
        BindableImage image;
        image.handle = {};
        descSetBinder.BindImage(0u, image, 0u);
        image.handle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_IMAGE, 0u);
        descSetBinder.BindImage(0u, image, 0u);
        descSetBinder.BindImages(0u, { &image, 1 });
        descSetBinder.BindImages(0u, { &image.handle, 1 });
        BindableSampler sampler;
        sampler.handle = {};
        descSetBinder.BindSampler(0u, sampler, 0u);
        image.handle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_IMAGE, 0u);
        descSetBinder.BindSampler(0u, sampler, 0u);
        descSetBinder.BindSamplers(0u, { &sampler, 1 });
        descSetBinder.BindSamplers(0u, { &sampler.handle, 1 });
    }
    {
        DescriptorSetLayoutBinding binding;
        binding.binding = 0u;
        binding.descriptorType = DescriptorType::CORE_DESCRIPTOR_TYPE_MAX_ENUM;
        DescriptorSetBinder descSetBinder { {}, { &binding, 1 } };
        BindableBuffer buffer;
        buffer.byteSize = 0;
        buffer.handle = {};
        descSetBinder.BindBuffer(0u, buffer, 0u);
        buffer.handle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_BUFFER, 0u);
        descSetBinder.BindBuffer(0u, buffer, 0u);
        descSetBinder.BindBuffer(0u, buffer.handle, buffer.byteOffset, buffer.byteSize);
        descSetBinder.BindBuffers(0u, { &buffer, 1 });
        descSetBinder.BindBuffers(0u, { &buffer.handle, 1 });
        BindableImage image;
        image.handle = {};
        descSetBinder.BindImage(0u, image, 0u);
        descSetBinder.BindImage(5u, image, 0u);
        image.handle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_IMAGE, 0u);
        descSetBinder.BindImage(0u, image, 0u);
        descSetBinder.BindImages(0u, { &image, 1 });
        descSetBinder.BindImages(0u, { &image.handle, 1 });
        BindableSampler sampler;
        sampler.handle = {};
        descSetBinder.BindSampler(0u, sampler, 0u);
        descSetBinder.BindSampler(5u, sampler, 0u);
        sampler.handle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_SAMPLER, 0u);
        descSetBinder.BindSampler(0u, sampler, 0u);
        descSetBinder.BindSamplers(0u, { &sampler, 1 });
        descSetBinder.BindSamplers(0u, { &sampler.handle, 1 });
        descSetBinder.BindSampler(0u, sampler, 0u);
    }
    {
        DescriptorSetLayoutBinding binding;
        binding.binding = 0u;
        binding.descriptorType = DescriptorType::CORE_DESCRIPTOR_TYPE_SAMPLER;
        binding.descriptorCount = 2;
        DescriptorSetBinder descSetBinder { {}, { &binding, 1 } };
        BindableSampler sampler;
        sampler.handle = {};
        descSetBinder.BindSamplers(0u, { &sampler, 1 });
        descSetBinder.BindSamplers(0u, { &sampler.handle, 1 });
    }
    {
        DescriptorSetLayoutBinding binding;
        binding.binding = 0u;
        binding.descriptorType = DescriptorType::CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        binding.descriptorCount = 2;
        DescriptorSetBinder descSetBinder { {}, { &binding, 1 } };
        BindableImage image;
        image.handle = {};
        descSetBinder.BindImages(0u, { &image, 1 });
        descSetBinder.BindImages(0u, { &image.handle, 1 });
    }
    {
        DescriptorSetLayoutBinding binding;
        binding.binding = 0u;
        binding.descriptorType = DescriptorType::CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 2;
        DescriptorSetBinder descSetBinder { {}, { &binding, 1 } };
        BindableBuffer buffer;
        buffer.handle = {};
        descSetBinder.BindBuffers(0u, { &buffer, 1 });
        descSetBinder.BindBuffers(0u, { &buffer.handle, 1 });
    }
    {
        PipelineLayout pl;
        PipelineDescriptorSetBinder pdsBinder { pl, {}, {} };
        {
            auto handles = pdsBinder.GetDescriptorSetHandles(8u, 3u);
            ASSERT_EQ(0u, handles.size());
        }
        {
            auto handles = pdsBinder.GetDescriptorSetHandles(0u, 3u);
            ASSERT_EQ(0u, handles.size());
        }
    }
    {
        PipelineLayout pl;
        pl.descriptorSetLayouts[0].set = 0u;
        DescriptorSetLayoutBinding binding;
        binding.binding = 0u;
        pl.descriptorSetLayouts[0].bindings.push_back(binding);
        DescriptorSetLayoutBindings bindings;
        binding.binding = 1u;
        bindings.binding.push_back(binding);
        RenderHandle setHandle;
        PipelineDescriptorSetBinder pdsBinder { pl, { &setHandle, 1 }, { &bindings, 1 } };
        {
            auto handles = pdsBinder.GetDescriptorSetHandles(8u, 3u);
            ASSERT_EQ(0u, handles.size());
        }
        {
            auto handles = pdsBinder.GetDescriptorSetHandles(0u, 3u);
            ASSERT_EQ(1u, handles.size());
        }
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: PipelineDescriptorSetBinderTestVulkan
 * @tc.desc: Tests DescriptorSetBinder by binding different resources to binding points via DescriptorSetBinder::Bind*()
 * functions. Also tests PipelineDescriptorSetBinder by binding many resources at once. Backend is Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_PipelineDescriptorSetBinder, PipelineDescriptorSetBinderTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(engine);
    TestPipelineDescriptorSetBinder(engine);
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: PipelineDescriptorSetBinderTestOpenGL
 * @tc.desc: Tests DescriptorSetBinder by binding different resources to binding points via DescriptorSetBinder::Bind*()
 * functions. Also tests PipelineDescriptorSetBinder by binding many resources at once. Backend is OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_PipelineDescriptorSetBinder, PipelineDescriptorSetBinderTestOpenGL, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.createWindow = true;
    engine.backend = UTest::GetOpenGLBackend();
    UTest::CreateEngineSetup(engine);
    TestPipelineDescriptorSetBinder(engine);
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
