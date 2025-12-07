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
#include <device/shader_pipeline_binder.h>
#include <nodecontext/node_context_descriptor_set_manager.h>

#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
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

template<typename T, uint32_t count>
vector<RenderHandleReference> CreateResource(IGpuResourceManager& gpuResourceMgr, const T& desc)
{
    vector<RenderHandleReference> outRes(count);
    for (uint32_t i = 0; i < count; i++) {
        outRes[i] = gpuResourceMgr.Create(desc);
    }
    return move(outRes);
}

void TestShaderPipelineBinder(const UTest::EngineResources& engine)
{
    auto nodeContextDescriptorSetMgr = ((Device*)engine.device)->CreateNodeContextDescriptorSetManager();
    auto shaderHandle =
        engine.device->GetShaderManager().GetShaderHandle("rendershaders://shader/ShaderPipelineBinderTest.shader");
    ASSERT_NE(RenderHandle {}, shaderHandle.GetHandle());
    auto plHandle = engine.device->GetShaderManager().GetReflectionPipelineLayoutHandle(shaderHandle);
    auto pipelineLayout = engine.device->GetShaderManager().GetReflectionPipelineLayout(shaderHandle);
    ShaderPipelineBinder binder { engine.device->GetShaderManager(), shaderHandle, pipelineLayout };
    ASSERT_EQ(shaderHandle.GetHandle(), binder.GetShaderHandle().GetHandle());

    GpuBufferDesc bufDesc;
    bufDesc.byteSize = 28;
    bufDesc.usageFlags = CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE;
    bufDesc.format = BASE_FORMAT_R32_SFLOAT;
    bufDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    GpuImageDesc imgDesc;
    imgDesc.format = BASE_FORMAT_R8G8B8_UNORM;
    imgDesc.usageFlags = ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
    imgDesc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    auto& gpuResourceMgr = engine.device->GetGpuResourceManager();
    auto samplerHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    auto imageHandle = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");
    auto bufferHandle = CreateResource<GpuBufferDesc, 1>(gpuResourceMgr, bufDesc).at(0);

    auto imageHandles = CreateResource<GpuImageDesc, 2>(gpuResourceMgr, imgDesc);
    auto bufferHandles = CreateResource<GpuBufferDesc, 2>(gpuResourceMgr, bufDesc);

    vector<BindableImageWithHandleReference> bindableImgs;
    for (auto& buf : imageHandles) {
        BindableImageWithHandleReference bindableImg = {};
        bindableImg.handle = buf;
        bindableImgs.push_back(bindableImg);
    }
    vector<BindableBufferWithHandleReference> bindableBufs;
    for (auto& buf : bufferHandles) {
        BindableBufferWithHandleReference bindableBuf = {};
        bindableBuf.handle = buf;
        bindableBufs.push_back(bindableBuf);
    }

    binder.Bind(0, 0, samplerHandle);
    binder.Bind(0, 1, imageHandle);
    binder.Bind(0, 2, bufferHandle);
    binder.BindImages(0, 3, { bindableImgs.data(), bindableImgs.size() });
    binder.BindBuffers(0, 4, { bindableBufs.data(), bindableBufs.size() });

    ASSERT_TRUE(binder.GetBindingValidity());

    binder.ClearBindings();
    ASSERT_FALSE(binder.GetBindingValidity());
    {
        if (auto* bindingProperties = binder.GetBindingProperties(); bindingProperties) {
            {
                BindableImageWithHandleReference bindable;
                bindable.handle = imageHandle;
                CORE_NS::SetPropertyValue(*bindingProperties, "uTex",
                    CORE_NS::PropertyType::BINDABLE_IMAGE_WITH_HANDLE_REFERENCE_T, bindable);
            }
            {
                BindableSamplerWithHandleReference bindable;
                bindable.handle = samplerHandle;
                CORE_NS::SetPropertyValue(*bindingProperties, "uSampler",
                    CORE_NS::PropertyType::BINDABLE_SAMPLER_WITH_HANDLE_REFERENCE_T, bindable);
            }
        }
        binder.Bind(0, 2, bufferHandle);
        binder.BindImages(0, 3, { bindableImgs.data(), bindableImgs.size() });
        binder.BindBuffers(0, 4, { bindableBufs.data(), bindableBufs.size() });

        ASSERT_TRUE(binder.GetBindingValidity());
        const auto resRef = binder.GetDescriptorSetLayoutBindingResources(0U);
        ASSERT_TRUE(resRef.buffers.size() == 3U);
        ASSERT_TRUE(resRef.images.size() == 3U);
        ASSERT_TRUE(resRef.samplers.size() == 1U);

        {
            const auto buf = binder.GetResourceBinding(0, 2);
            ASSERT_TRUE(buf.handle.GetHandleType() == RenderHandleType::GPU_BUFFER);
            const auto img = binder.GetResourceBinding(0, 1);
            ASSERT_TRUE(img.handle.GetHandleType() == RenderHandleType::GPU_IMAGE);
            const auto sam = binder.GetResourceBinding(0, 0);
            ASSERT_TRUE(sam.handle.GetHandleType() == RenderHandleType::GPU_SAMPLER);
            const auto img2 = binder.GetResourceBinding(0, 3);
            ASSERT_TRUE(img2.handle.GetHandleType() == RenderHandleType::GPU_IMAGE);
            const auto buf2 = binder.GetResourceBinding(0, 4);
            ASSERT_TRUE(buf2.handle.GetHandleType() == RenderHandleType::GPU_BUFFER);
        }

        {
            GpuBufferDesc desc;
            desc.byteSize = 28 * 4;
            desc.usageFlags = CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            desc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE;
            desc.format = BASE_FORMAT_R32_SFLOAT;
            desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }
    }

    float floatData[] = { 1.f, 1.f, 1.f, 1.f, 2.f, 2.f, 2.f, 2.f };
    const uint8_t* byteData = reinterpret_cast<const uint8_t*>(floatData);
    binder.SetPushConstantData({ byteData, sizeof(floatData) });
    auto pushData = binder.GetPushConstantData();
    for (uint32_t i = 0; i < pushData.size(); ++i) {
        ASSERT_EQ(byteData[i], pushData[i]);
    }
}
} // namespace

/**
 * @tc.name: ShaderPipelineBinderTest
 * @tc.desc: Tests the ShaderPipelineBinder to bind resources to the correct bind points and send push constant data.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_ShaderPipelineBinder, ShaderPipelineBinderTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    if (engine.backend == DeviceBackendType::OPENGL) {
        engine.createWindow = true;
    }
    UTest::CreateEngineSetup(engine);
    TestShaderPipelineBinder(engine);
    UTest::DestroyEngine(engine);
}
