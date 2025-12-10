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

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

#if RENDER_HAS_VULKAN_BACKEND
#include <vulkan/node_context_descriptor_set_manager_vk.h>
#endif // RENDER_HAS_VULKAN_BACKEND

using namespace BASE_NS;
using CORE_NS::IEngine;
using namespace RENDER_NS;

namespace {
void TestNodeContextDescriptorSetManager(const UTest::EngineResources& engine)
{
    auto nodeContextDescriptorSetMgr = ((Device*)engine.device)->CreateNodeContextDescriptorSetManager();
    {
        DescriptorCounts descCounts;
        descCounts.counts.emplace_back(
            DescriptorCounts::TypedCount { DescriptorType::CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2 });
        descCounts.counts.emplace_back(
            DescriptorCounts::TypedCount { DescriptorType::CORE_DESCRIPTOR_TYPE_SAMPLER, 1 });
        descCounts.counts.emplace_back(
            DescriptorCounts::TypedCount { DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 });
        nodeContextDescriptorSetMgr->ResetAndReserve(descCounts);
        nodeContextDescriptorSetMgr->BeginFrame();
    }
    vector<RenderHandle> allHandles;
    {
        DescriptorSetLayoutBindings bindings[2];
        for (uint32_t i = 0; i < countof(bindings); ++i) {
            bindings[i] = DescriptorSetLayoutBindings {};
            {
                DescriptorSetLayoutBinding binding;
                binding.binding = 0;
                binding.descriptorCount = 1;
                if (i == 0) {
                    binding.descriptorType = DescriptorType::CORE_DESCRIPTOR_TYPE_SAMPLER;
                } else {
                    binding.descriptorType = DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                }
                binding.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_ALL_GRAPHICS;
                bindings[i].binding.emplace_back(move(binding));
            }
            {
                DescriptorSetLayoutBinding binding;
                binding.binding = 1;
                binding.descriptorCount = 1;
                binding.descriptorType = DescriptorType::CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                binding.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT;
                bindings[i].binding.emplace_back(move(binding));
            }
        }
        {
            auto handles = nodeContextDescriptorSetMgr->CreateDescriptorSets({ bindings, countof(bindings) });
            ASSERT_EQ(countof(bindings), handles.size());
            for (uint32_t i = 0; i < countof(bindings); ++i) {
                ASSERT_NE(RenderHandle {}, handles[i]);
                allHandles.emplace_back(handles[i]);
            }
        }
        {
            auto handles = nodeContextDescriptorSetMgr->CreateOneFrameDescriptorSets({ bindings, countof(bindings) });
            ASSERT_EQ(countof(bindings), handles.size());
            for (uint32_t i = 0; i < countof(bindings); ++i) {
                ASSERT_NE(RenderHandle {}, handles[i]);
                allHandles.emplace_back(handles[i]);
            }
            PipelineLayout pipelineLayout;
            pipelineLayout.pushConstant.byteSize = 16;
            pipelineLayout.pushConstant.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT;
            pipelineLayout.descriptorSetLayouts[0].set = 0;
            pipelineLayout.descriptorSetLayouts[0].bindings = bindings[0].binding;
            auto handle = nodeContextDescriptorSetMgr->CreateOneFrameDescriptorSet(0u, pipelineLayout);
            ASSERT_NE(RenderHandle {}, handle);
            allHandles.emplace_back(handle);
        }
        {
            auto result = nodeContextDescriptorSetMgr->GetDynamicOffsetDescriptors(allHandles[0]);
            ASSERT_EQ(1, result.resources.size());
        }
        {
            for (uint32_t i = 0; i < allHandles.size(); ++i) {
                ASSERT_FALSE(nodeContextDescriptorSetMgr->HasDynamicBarrierResources(allHandles[i]));
            }
        }
        if (engine.backend == DeviceBackendType::VULKAN) {
#if RENDER_HAS_VULKAN_BACKEND
            ((NodeContextDescriptorSetManagerVk&)(*nodeContextDescriptorSetMgr)).BeginBackendFrame();
#endif // RENDER_HAS_VULKAN_BACKEND
        }
    }
    {
        DescriptorCounts descCounts;
        descCounts.counts.emplace_back(
            DescriptorCounts::TypedCount { DescriptorType::CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 });
        descCounts.counts.emplace_back(
            DescriptorCounts::TypedCount { DescriptorType::CORE_DESCRIPTOR_TYPE_SAMPLER, 1 });
        nodeContextDescriptorSetMgr->ResetAndReserve(descCounts);
        nodeContextDescriptorSetMgr->BeginFrame();
        auto conversionHandle = RenderHandleUtil::CreateHandle(
            RenderHandleType::GPU_IMAGE, 0, 0, RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_PLATFORM_CONVERSION);
        DescriptorSetLayoutBindings bindings;
        DescriptorSetLayoutBinding binding;
        binding.binding = 0;
        binding.descriptorCount = 1;
        binding.descriptorType = CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_ALL_GRAPHICS;
        bindings.binding.emplace_back(binding);

        auto handles = nodeContextDescriptorSetMgr->CreateDescriptorSets({ &bindings, 1 });
        ASSERT_EQ(1, handles.size());
        auto setHandle = handles[0];
        ASSERT_NE(RenderHandle {}, setHandle);
        auto binder = nodeContextDescriptorSetMgr->CreateDescriptorSetBinder(setHandle, bindings.binding);
        BindableImage bindableImage;
        bindableImage.handle = conversionHandle;
        bindableImage.samplerHandle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_SAMPLER, 0);
        binder->BindImage(
            0, bindableImage, AdditionalDescriptorFlagBits::CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT);
        GpuQueue gpuQueue { GpuQueue::QueueType::GRAPHICS, 0 };
        nodeContextDescriptorSetMgr->UpdateCpuDescriptorSet(
            setHandle, binder->GetDescriptorSetLayoutBindingResources(), gpuQueue);
        if (engine.backend == DeviceBackendType::VULKAN) {
#if RENDER_HAS_VULKAN_BACKEND
            ((NodeContextDescriptorSetManagerVk&)(*nodeContextDescriptorSetMgr)).BeginBackendFrame();
#endif // RENDER_HAS_VULKAN_BACKEND
        }
    }
    {
        DescriptorCounts descCounts;
        descCounts.counts.emplace_back(
            DescriptorCounts::TypedCount { DescriptorType::CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 });
        descCounts.counts.emplace_back(
            DescriptorCounts::TypedCount { DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 });
        descCounts.counts.emplace_back(
            DescriptorCounts::TypedCount { DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 3 });
        descCounts.counts.emplace_back(
            DescriptorCounts::TypedCount { DescriptorType::CORE_DESCRIPTOR_TYPE_SAMPLER, 3 });
        nodeContextDescriptorSetMgr->ResetAndReserve(descCounts);
        nodeContextDescriptorSetMgr->BeginFrame();
        auto conversionHandle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_IMAGE, 0, 0);
        uint32_t bindIdx = 0;
        DescriptorSetLayoutBindings bindings;
        {
            DescriptorSetLayoutBinding binding;
            binding.binding = bindIdx++;
            binding.descriptorCount = 1;
            binding.descriptorType = CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_ALL_GRAPHICS;
            bindings.binding.emplace_back(binding);
            binding.binding = bindIdx++;
            binding.descriptorCount = 2;
            bindings.binding.emplace_back(binding);
        }
        {
            DescriptorSetLayoutBinding binding;
            binding.binding = bindIdx++;
            binding.descriptorCount = 1;
            binding.descriptorType = CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            binding.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_ALL_GRAPHICS;
            bindings.binding.emplace_back(binding);
            binding.binding = bindIdx++;
            binding.descriptorCount = 2;
            bindings.binding.emplace_back(binding);
        }
        {
            DescriptorSetLayoutBinding binding;
            binding.binding = bindIdx++;
            binding.descriptorCount = 1;
            binding.descriptorType = CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            binding.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_ALL_GRAPHICS;
            bindings.binding.emplace_back(binding);
        }
        {
            DescriptorSetLayoutBinding binding;
            binding.binding = bindIdx++;
            binding.descriptorCount = 1;
            binding.descriptorType = CORE_DESCRIPTOR_TYPE_SAMPLER;
            binding.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_ALL_GRAPHICS;
            bindings.binding.emplace_back(binding);
            binding.binding = bindIdx++;
            binding.descriptorCount = 2;
            bindings.binding.emplace_back(binding);
        }

        auto handles = nodeContextDescriptorSetMgr->CreateDescriptorSets({ &bindings, 1 });
        ASSERT_EQ(1, handles.size());
        auto setHandle = handles[0];
        ASSERT_NE(RenderHandle {}, setHandle);
        auto binder = nodeContextDescriptorSetMgr->CreateDescriptorSetBinder(setHandle, bindings.binding);
        BindableImage bindableImage;
        bindableImage.handle = conversionHandle;
        bindableImage.samplerHandle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_SAMPLER, 0, 1);
        binder->BindImage(
            0, bindableImage, AdditionalDescriptorFlagBits::CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT);
        GpuQueue gpuQueue { GpuQueue::QueueType::GRAPHICS, 0 };
        nodeContextDescriptorSetMgr->UpdateCpuDescriptorSet(
            setHandle, binder->GetDescriptorSetLayoutBindingResources(), gpuQueue);
        nodeContextDescriptorSetMgr->UpdateDescriptorSetGpuHandle(setHandle);
        if (engine.backend == DeviceBackendType::VULKAN) {
#if RENDER_HAS_VULKAN_BACKEND
            ((NodeContextDescriptorSetManagerVk&)(*nodeContextDescriptorSetMgr)).BeginBackendFrame();
#endif // RENDER_HAS_VULKAN_BACKEND
        }
    }
#if NDEBUG
    {
        DescriptorCounts descCounts;
        descCounts.counts.emplace_back(
            DescriptorCounts::TypedCount { DescriptorType::CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2 });
        descCounts.counts.emplace_back(
            DescriptorCounts::TypedCount { DescriptorType::CORE_DESCRIPTOR_TYPE_SAMPLER, 1 });
        descCounts.counts.emplace_back(
            DescriptorCounts::TypedCount { DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 });
        nodeContextDescriptorSetMgr->ResetAndReserve(descCounts);
        nodeContextDescriptorSetMgr->BeginFrame();
        for (auto type : { CORE_DESCRIPTOR_TYPE_SAMPLER, CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                 CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE, CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                 CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, CORE_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                 CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
                 CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE }) {
            DescriptorSetLayoutBindings bindings;
            DescriptorSetLayoutBinding binding;
            binding.binding = 0;
            binding.descriptorCount = 3;
            binding.descriptorType = type;
            binding.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_ALL_GRAPHICS;
            bindings.binding.emplace_back(binding);
            auto handles = nodeContextDescriptorSetMgr->CreateDescriptorSets({ &bindings, 1 });
            ASSERT_EQ(1, handles.size());
        }
    }
#endif // NDEBUG
    {
        DescriptorCounts descCounts;
        descCounts.counts.emplace_back(
            DescriptorCounts::TypedCount { DescriptorType::CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2 });
        descCounts.counts.emplace_back(
            DescriptorCounts::TypedCount { DescriptorType::CORE_DESCRIPTOR_TYPE_SAMPLER, 1 });
        descCounts.counts.emplace_back(
            DescriptorCounts::TypedCount { DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 });
        nodeContextDescriptorSetMgr->ResetAndReserve(descCounts);
        nodeContextDescriptorSetMgr->BeginFrame();
        {
            auto handle = nodeContextDescriptorSetMgr->CreateDescriptorSet(16u, {});
            ASSERT_EQ(RenderHandle {}, handle);
        }
        {
            PipelineLayout pl;
            pl.descriptorSetLayouts[0].set = 1u;
            auto handle = nodeContextDescriptorSetMgr->CreateDescriptorSet(0u, pl);
            ASSERT_EQ(RenderHandle {}, handle);
        }
        {
            PipelineLayout pl;
            pl.descriptorSetLayouts[0].set = 1u;
            auto handle = nodeContextDescriptorSetMgr->CreateOneFrameDescriptorSet(0u, pl);
            ASSERT_EQ(RenderHandle {}, handle);
        }
        {
            auto binder = nodeContextDescriptorSetMgr->CreatePipelineDescriptorSetBinder({}, {}, {});
            ASSERT_TRUE(binder);
        }
        {
            auto handle = RenderHandleUtil::CreateHandle(RenderHandleType::DESCRIPTOR_SET, 6);
            nodeContextDescriptorSetMgr->UpdateCpuDescriptorSet(handle, {}, {});
            auto data = nodeContextDescriptorSetMgr->GetCpuDescriptorSetData(handle);
            ASSERT_EQ(0, data.bindings.size());
            auto offset = nodeContextDescriptorSetMgr->GetDynamicOffsetDescriptors(handle);
            ASSERT_EQ(0, offset.resources.size());
            ASSERT_FALSE(nodeContextDescriptorSetMgr->HasDynamicBarrierResources(handle));
            ASSERT_EQ(0, nodeContextDescriptorSetMgr->GetDynamicOffsetDescriptorCount(handle));
            DescriptorSetLayoutBindings bindings;
            DescriptorSetLayoutBinding binding;
            binding.binding = 0;
            binding.descriptorCount = 3;
            binding.descriptorType = CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            binding.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_ALL_GRAPHICS;
            bindings.binding.emplace_back(binding);
            auto handles = nodeContextDescriptorSetMgr->CreateDescriptorSets({ &bindings, 1 });
            ASSERT_EQ(1, handles.size());
#if NDEBUG
            nodeContextDescriptorSetMgr->UpdateCpuDescriptorSet(handles[0], {}, {});
#endif // NDEBUG
        }
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: NodeContextDescriptorSetManagerTestVulkan
 * @tc.desc: Tests NodeContextDescriptorSetManager by creating, updating descriptors sets and verifying the behaviour is
 * correct for Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_NodeContextDescriptorSetManager, NodeContextDescriptorSetManagerTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(engine);
    TestNodeContextDescriptorSetManager(engine);
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: NodeContextDescriptorSetManagerTestOpenGL
 * @tc.desc: Tests NodeContextDescriptorSetManager by creating, updating descriptors sets and verifying the behaviour is
 * correct for OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_NodeContextDescriptorSetManager, NodeContextDescriptorSetManagerTestOpenGL, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = UTest::GetOpenGLBackend();
    engine.createWindow = true;
    UTest::CreateEngineSetup(engine);
    TestNodeContextDescriptorSetManager(engine);
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
