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

#if RENDER_HAS_VULKAN_BACKEND
#include <vulkan/device_vk.h>
#include <vulkan/gpu_memory_allocator_vk.h>
#endif // RENDER_HAS_VULKAN_BACKEND

#include <device/device.h>

#include <render/vulkan/intf_device_vk.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: GpuMemoryAllocatorTestVulkan
 * @tc.desc: Tests Vulkan PlatformGpuMemoryAllocator by creating a custom pool and an allocator for images.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuMemoryAllocator, GpuMemoryAllocatorTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(engine);
    Device& device = *static_cast<Device*>(engine.device);
    DeviceVk& deviceVk = static_cast<DeviceVk&>(device);
    PlatformGpuMemoryAllocator::GpuMemoryAllocatorCreateInfo createInfo;
    GpuImageDesc imageDesc;
    imageDesc.imageType = CORE_IMAGE_TYPE_2D;
    imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
    imageDesc.format = BASE_FORMAT_R8G8B8A8_UINT;
    imageDesc.width = 16u;
    imageDesc.height = 10u;
    imageDesc.depth = 1u;
    imageDesc.mipCount = 1u;
    imageDesc.layerCount = 1u;
    imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
    imageDesc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    PlatformGpuMemoryAllocator::GpuMemoryAllocatorCustomPool::GpuResourceDesc desc { { 0 } };
    desc.image = imageDesc;
    PlatformGpuMemoryAllocator::GpuMemoryAllocatorCustomPool pool { "poolName",
        PlatformGpuMemoryAllocator::MemoryAllocatorResourceType::GPU_IMAGE, 0, false, desc };
    createInfo.customPools.push_back(pool);
    createInfo.preferredLargeHeapBlockSize = 0;
    PlatformGpuMemoryAllocator allocator { deviceVk.GetPlatformDataVk().instance,
        deviceVk.GetPlatformDataVk().physicalDevice, deviceVk.GetPlatformDataVk().device, createInfo };
    VmaPool imgPool = allocator.GetImagePool(imageDesc);
    ASSERT_NE(reinterpret_cast<VmaPool_T *>(VK_NULL_HANDLE), imgPool);
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_VULKAN_BACKEND
