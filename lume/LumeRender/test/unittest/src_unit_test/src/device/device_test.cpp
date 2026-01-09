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
#include <device/gpu_resource_manager.h>
#include <nodecontext/node_context_descriptor_set_manager.h>
#include <render_context.h>

#include <render/device/intf_shader_manager.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

#if RENDER_HAS_VULKAN_BACKEND
#include <vulkan/device_vk.h>
#endif // RENDER_HAS_VULKAN_BACKEND
#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
#include <gles/device_gles.h>
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND

using namespace BASE_NS;
using CORE_NS::IEngine;
using namespace RENDER_NS;

namespace {
static constexpr uint8_t pipelineCache[50] = { 0u };

void TestConfig(UTest::EngineResources& er)
{
    DeviceCreateInfo dci;
    dci.deviceConfiguration.bufferingCount = 4U;
    dci.deviceConfiguration.swapchainImageCount = 2U;
    dci.deviceConfiguration.configurationFlags = 0U;
    dci.deviceConfiguration.requiredIntegratedMemoryFlags = 0U;
    dci.backendType = er.backend;

    // create render context which gives the device create info to devices
    er.context.reset();
    er.context.reset(static_cast<IRenderContext*>(
        er.engine->GetInterface<CORE_NS::IClassFactory>()->CreateInstance(UID_RENDER_CONTEXT).get()));
    ASSERT_TRUE(er.context);

    const RenderCreateInfo info {
        {
            "", // name
            1,  // versionMajor
            0,  // versionMinor
            0,  // versionPatch
        },
        dci,
    };
    const RenderResultCode rrc = er.context->Init(info);
    if (rrc == RenderResultCode::RENDER_SUCCESS) {
        er.device = &er.context->GetDevice();
        er.context->GetRenderer().RenderFrame({});
    } else {
        CORE_LOG_E("Render context init failed");
        CORE_ASSERT(false);
    }
    unique_ptr<Device> tmpDevice = nullptr;
#if RENDER_HAS_VULKAN_BACKEND
    if (er.backend == DeviceBackendType::VULKAN) {
        tmpDevice = CreateDeviceVk((RenderContext&)(*er.context.get()));
    }
#endif // RENDER_HAS_VULKAN_BACKEND
#if RENDER_HAS_GL_BACKEND
    if (er.backend == DeviceBackendType::OPENGL) {
        tmpDevice = CreateDeviceGL((RenderContext&)(*er.context.get()));
    }
#endif // RENDER_HAS_GL_BACKEND
#if RENDER_HAS_GLES_BACKEND
    if (er.backend == DeviceBackendType::OPENGLES) {
        tmpDevice = CreateDeviceGLES((RenderContext&)(*er.context.get()));
    }
#endif // RENDER_HAS_GLES_BACKEND

    const auto devCon = tmpDevice->GetDeviceConfiguration();
    ASSERT_EQ(dci.deviceConfiguration.bufferingCount, devCon.bufferingCount);
    ASSERT_EQ(dci.deviceConfiguration.swapchainImageCount, devCon.swapchainImageCount);
    ASSERT_EQ(dci.deviceConfiguration.configurationFlags, devCon.configurationFlags);
    ASSERT_EQ(dci.deviceConfiguration.requiredIntegratedMemoryFlags, devCon.requiredIntegratedMemoryFlags);
}
void TestPipelineCache(const UTest::EngineResources& er)
{
    Device& device = *static_cast<Device*>(er.device);
    {
        device.InitializePipelineCache({ pipelineCache, countof(pipelineCache) });
        auto data = device.GetPipelineCache();
        ASSERT_NE(0, data.size());
        device.InitializePipelineCache(data);
        data = device.GetPipelineCache();
        ASSERT_NE(0, data.size());
    }
}
void TestLowLevelDevice(const UTest::EngineResources& er)
{
    Device& device = *static_cast<Device*>(er.device);
    GpuResourceManager& gpuResourceMgr = static_cast<GpuResourceManager&>(device.GetGpuResourceManager());
    {
        device.SetLockResourceBackendAccess(true);
        ASSERT_TRUE(device.GetLockResourceBackendAccess());
        ILowLevelDevice& lowLevelDevice = device.GetLowLevelDevice();
        ASSERT_EQ(er.backend, lowLevelDevice.GetBackendType());
#if RENDER_HAS_VULKAN_BACKEND
        if (er.backend == DeviceBackendType::VULKAN) {
            LowLevelDeviceVk& deviceVk = static_cast<LowLevelDeviceVk&>(lowLevelDevice);
            ASSERT_EQ(er.backend, deviceVk.GetBackendType());
            auto platData = deviceVk.GetPlatformDataVk();
            ASSERT_EQ(((DeviceVk&)device).GetPlatformDataVk().device, platData.device);
            {
                GpuBufferDesc desc {};
                desc.byteSize = 16;
                desc.engineCreationFlags = EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE |
                                           EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
                desc.format = BASE_FORMAT_R32G32B32A32_SFLOAT;
                desc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                desc.usageFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                auto handle = gpuResourceMgr.Create(desc);
                device.Activate();
                gpuResourceMgr.HandlePendingAllocations(true);
                device.Deactivate();
                auto buffer = deviceVk.GetBuffer(handle.GetHandle());
                ASSERT_TRUE(buffer.buffer);
            }
            {
                GpuImageDesc desc {};
                desc.engineCreationFlags = EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
                desc.format = BASE_FORMAT_R32G32B32A32_SFLOAT;
                desc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                desc.usageFlags = ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                                  ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
                desc.width = 4;
                desc.height = 4;
                desc.depth = 1;
                desc.mipCount = 1;
                desc.imageTiling = ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
                desc.imageType = ImageType::CORE_IMAGE_TYPE_2D;
                desc.imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
                desc.layerCount = 1;
                auto handle = gpuResourceMgr.Create(desc);
                device.Activate();
                gpuResourceMgr.HandlePendingAllocations(true);
                device.Deactivate();
                auto image = deviceVk.GetImage(handle.GetHandle());
                ASSERT_TRUE(image.image);
            }
            {
                GpuSamplerDesc desc {};
                desc.engineCreationFlags = 0;
                desc.addressModeU = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT;
                desc.addressModeV = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT;
                desc.addressModeW = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT;
                desc.borderColor = BorderColor::CORE_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
                auto handle = gpuResourceMgr.Create(desc);
                device.Activate();
                gpuResourceMgr.HandlePendingAllocations(true);
                device.Deactivate();
                auto sampler = deviceVk.GetSampler(handle.GetHandle());
                ASSERT_TRUE(sampler.sampler);
            }
            {
                auto buffer = deviceVk.GetBuffer({});
                ASSERT_FALSE(buffer.buffer);
                auto image = deviceVk.GetImage({});
                ASSERT_FALSE(image.image);
                auto sampler = deviceVk.GetSampler({});
                ASSERT_FALSE(sampler.sampler);
            }
            device.SetLockResourceBackendAccess(false);
            {
                auto buffer = deviceVk.GetBuffer({});
                ASSERT_FALSE(buffer.buffer);
                auto image = deviceVk.GetImage({});
                ASSERT_FALSE(image.image);
                auto sampler = deviceVk.GetSampler({});
                ASSERT_FALSE(sampler.sampler);
            }
        }
#endif // RENDER_HAS_VULKAN_BACKEND
#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
        if (er.backend == DeviceBackendType::OPENGL || er.backend == DeviceBackendType::OPENGLES) {
            LowLevelDeviceGLES& deviceGL = static_cast<LowLevelDeviceGLES&>(lowLevelDevice);
            ASSERT_EQ(er.backend, deviceGL.GetBackendType());
        }
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
    }
}
void TestDevice(const UTest::EngineResources& er)
{
    Device& device = *static_cast<Device*>(er.device);
    {
        device.WaitForIdle();
        device.SetRenderBackendRunning(true);
        device.SetRenderBackendRunning(false);
        device.SetLockResourceBackendAccess(true);
        ASSERT_TRUE(device.GetLockResourceBackendAccess());
        device.SetBackendConfig({});
#if RENDER_HAS_VULKAN_BACKEND
        if (er.backend == DeviceBackendType::VULKAN) {
            DeviceVk& deviceVk = static_cast<DeviceVk&>(device);
            ASSERT_EQ(er.backend, deviceVk.GetBackendType());
            auto queues = deviceVk.GetLowLevelGpuQueues();
            ASSERT_LT(0, queues.size());
            auto featConfigs = deviceVk.GetFeatureConfigurations();
            ASSERT_NE(0.f, featConfigs.minSampleShading);
            auto platExts = deviceVk.GetPlatformDeviceExtensions();
            auto platExtFns = deviceVk.GetPlatformExtFunctions();
            ASSERT_FALSE(deviceVk.HasDeviceExtension("Non_existing_extension"));
            {
                // NOTE: this is a low-level test which user does not have access to
                // one could create proper test to the previous IGpuResourceManager::Create()
                GpuAccelerationStructureDesc accStructDesc {};
                accStructDesc.accelerationStructureType =
                    AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
                accStructDesc.bufferDesc.usageFlags = CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT;
                accStructDesc.bufferDesc.byteSize = 8;
                accStructDesc.bufferDesc.engineCreationFlags =
                    EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
                auto accStruct = deviceVk.CreateGpuBuffer(accStructDesc);
                ASSERT_TRUE(accStruct);
            }
            {
                auto accBuildSizes = deviceVk.GetAccelerationStructureBuildSizes({}, {}, {}, {});
#if !(RENDER_VULKAN_RT_ENABLED)
                ASSERT_EQ(0u, accBuildSizes.accelerationStructureSize);
                ASSERT_EQ(0u, accBuildSizes.buildScratchSize);
                ASSERT_EQ(0u, accBuildSizes.updateScratchSize);
#endif // !(RENDER_VULKAN_RT_ENABLED)
            }
            {
                uint32_t formatIdx = static_cast<uint32_t>(BASE_FORMAT_G8_B8_R8_3PLANE_444_UNORM);
                auto properties0 = deviceVk.GetFormatProperties(
                    static_cast<Format>(formatIdx - DeviceFormatSupportConstants::ADDITIONAL_FORMAT_START_NUMBER));
                auto properties1 = deviceVk.GetFormatProperties(BASE_FORMAT_G8_B8_R8_3PLANE_444_UNORM);
                ASSERT_EQ(properties0.bufferFeatures, properties1.bufferFeatures);
                ASSERT_EQ(properties0.bytesPerPixel, properties1.bytesPerPixel);
                ASSERT_EQ(properties0.linearTilingFeatures, properties1.linearTilingFeatures);
                ASSERT_EQ(properties0.optimalTilingFeatures, properties1.optimalTilingFeatures);
            }
            {
                auto properties = deviceVk.GetFormatProperties(Format::BASE_FORMAT_UNDEFINED);
                ASSERT_EQ(FormatProperties {}.bufferFeatures, properties.bufferFeatures);
                ASSERT_EQ(FormatProperties {}.bytesPerPixel, properties.bytesPerPixel);
                ASSERT_EQ(FormatProperties {}.linearTilingFeatures, properties.linearTilingFeatures);
                ASSERT_EQ(FormatProperties {}.optimalTilingFeatures, properties.optimalTilingFeatures);
            }
        }
#endif // RENDER_HAS_VULKAN_BACKEND
#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
        if (er.backend == DeviceBackendType::OPENGL || er.backend == DeviceBackendType::OPENGLES) {
            DeviceGLES& deviceGL = static_cast<DeviceGLES&>(device);
            ASSERT_EQ(er.backend, deviceGL.GetBackendType());

            ASSERT_EQ(0u, deviceGL.BoundWriteFrameBuffer());
            ASSERT_EQ(0u, deviceGL.BoundReadFrameBuffer());
            ASSERT_EQ(0u, deviceGL.BoundProgram());
#ifdef NDEBUG
            ASSERT_EQ(0u, deviceGL.BoundBuffer(0u, 0u));
            ASSERT_EQ(0u, deviceGL.BoundTexture(0u, 0u));
            ASSERT_EQ(0u, deviceGL.BoundTexture(0u, 0u));
#endif // NDEBUG

            ASSERT_EQ(0u, deviceGL.BoundSampler(0u));

            {
                // NOTE: this is a low-level test which user does not have access to
                // one could create proper test to the previous IGpuResourceManager::Create()

                deviceGL.Activate();

                GpuAccelerationStructureDesc accStructDesc {};
                accStructDesc.accelerationStructureType =
                    AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
                accStructDesc.bufferDesc.usageFlags = CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT;
                accStructDesc.bufferDesc.byteSize = 8;
                accStructDesc.bufferDesc.engineCreationFlags =
                    EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
                auto gpuBuffer = deviceGL.CreateGpuBuffer(accStructDesc);
                ASSERT_TRUE(gpuBuffer);
                auto buildSizes = deviceGL.GetAccelerationStructureBuildSizes({}, {}, {}, {});
                ASSERT_EQ(AsBuildSizes {}.accelerationStructureSize, buildSizes.accelerationStructureSize);
                ASSERT_EQ(AsBuildSizes {}.buildScratchSize, buildSizes.buildScratchSize);
                ASSERT_EQ(AsBuildSizes {}.updateScratchSize, buildSizes.updateScratchSize);

                gpuBuffer.reset();

                deviceGL.Deactivate();
            }
        }
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
    }
}
void TestBackendExtra(const UTest::EngineResources& er)
{
#if RENDER_HAS_VULKAN_BACKEND
    // NOTE: this test cannot run
    // it should be done from the API level with different vulkan instances
#endif // RENDER_HAS_VULKAN_BACKEND
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: DeviceConfigurationTestVulkan
 * @tc.desc: Tests device configuration for Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_Device, DeviceConfigurationTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(engine);
    TestConfig(engine);
    UTest::DestroyEngine(engine);
}
/**
 * @tc.name: DevicePipelineCacheTestVulkan
 * @tc.desc: Tests device pipeline cache functionality in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_Device, DevicePipelineCacheTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(engine);
    TestPipelineCache(engine);
    UTest::DestroyEngine(engine);
}
/**
 * @tc.name: LowLevelDeviceTestVulkan
 * @tc.desc: Tests ILowLevelDevice in Vulkan by trying to create valid and invalid buffer, image and sampler for
 * Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_Device, LowLevelDeviceTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(engine);
    TestLowLevelDevice(engine);
    UTest::DestroyEngine(engine);
}
/**
 * @tc.name: DeviceTestVulkan
 * @tc.desc: Tests many IDevice methods. Queries for device information (extensions, features, queues, etc.) and also
 * tests acceleration structure related methods for Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_Device, DeviceTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(engine);
    TestDevice(engine);
    UTest::DestroyEngine(engine);
}
/**
 * @tc.name: DeviceBackendExtraTest2Vulkan
 * @tc.desc: DOES NOTHING RIGH NOW
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_Device, DISABLED_DeviceBackendExtraTest2Vulkan, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(engine);
    TestBackendExtra(engine);
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: LowLevelDeviceTestOpenGL
 * @tc.desc: Only tests for querying ILowLevelDevice and checks the backend type.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_Device, LowLevelDeviceTestOpenGL, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = UTest::GetOpenGLBackend();
    engine.createWindow = true;
    UTest::CreateEngineSetup(engine);
    TestLowLevelDevice(engine);
    UTest::DestroyEngine(engine);
}

/**
 * @tc.name: DeviceConfigurationTestOpenGL
 * @tc.desc: Tests device configuration for OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_Device, DISABLED_DeviceConfigurationTestOpenGL, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = UTest::GetOpenGLBackend();
    engine.createWindow = true;
    UTest::CreateEngineSetup(engine);
    TestConfig(engine);
    UTest::DestroyEngine(engine);
}
/**
 * @tc.name: DeviceTestOpenGL
 * @tc.desc: Tests for OpenGL device properties.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_Device, DeviceTestOpenGL, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = UTest::GetOpenGLBackend();
    engine.createWindow = true;
    UTest::CreateEngineSetup(engine);
    TestDevice(engine);
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
