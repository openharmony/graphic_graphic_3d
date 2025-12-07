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
#include <vulkan/gpu_buffer_vk.h>
#endif // RENDER_HAS_VULKAN_BACKEND

#include <device/device.h>

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
 * @tc.name: GpuAccelerationStructureTestVulkan
 * @tc.desc: Tests Vulkan for creating a GpuBuffer for a GpuAccelerationStructureDesc.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuAccelerationStructure, GpuAccelerationStructureTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(engine);
    Device& device = *static_cast<Device*>(engine.device);
    device.Activate();
    {
        GpuAccelerationStructureDesc desc;
        desc.bufferDesc.byteSize = 16u;
        desc.bufferDesc.format = BASE_FORMAT_R8G8B8A8_UINT;
        desc.bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER;
        desc.bufferDesc.memoryPropertyFlags =
            CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        desc.bufferDesc.usageFlags = CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT;
        desc.accelerationStructureType = AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_GENERIC;
        GpuBufferVk accBuffer { device, desc };
        auto realDesc = accBuffer.GetDescAccelerationStructure();
        ASSERT_EQ(desc.bufferDesc.byteSize, realDesc.bufferDesc.byteSize);
        ASSERT_EQ(desc.bufferDesc.engineCreationFlags, realDesc.bufferDesc.engineCreationFlags);
        ASSERT_EQ(desc.bufferDesc.format, realDesc.bufferDesc.format);
        ASSERT_EQ(desc.bufferDesc.memoryPropertyFlags, realDesc.bufferDesc.memoryPropertyFlags);
        ASSERT_EQ(desc.bufferDesc.usageFlags, realDesc.bufferDesc.usageFlags);
        ASSERT_EQ(desc.accelerationStructureType, realDesc.accelerationStructureType);
        auto platDesc = accBuffer.GetPlatformData();
#if (RENDER_VULKAN_RT_ENABLED)
        // These are not the same thing, needs better documentation
        // ASSERT_EQ(desc.bufferDesc.byteSize, platDesc.byteSize);
#endif // RENDER_VULKAN_RT_ENABLED
    }
    device.Deactivate();
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_VULKAN_BACKEND
