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

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
#include <gles/gpu_buffer_gles.h>
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND

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

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GpuBufferTestOpenGL
 * @tc.desc: Tests GLES buffer creation and mapping with different engne creation and memory property flags.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuBuffer, GpuBufferTestOpenGL, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = UTest::GetOpenGLBackend();
    engine.createWindow = true;
    UTest::CreateEngineSetup(engine);
    Device& device = *static_cast<Device*>(engine.device);
    device.Activate();
    {
        GpuBufferDesc desc;
        desc.byteSize = 16u;
        desc.format = BASE_FORMAT_R8G8B8A8_UINT;
        desc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_SRC_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        GpuBufferGLES buffer { device, desc };
        ASSERT_EQ(nullptr, buffer.Map());
        ASSERT_EQ(nullptr, buffer.MapMemory());
    }
    {
        GpuBufferDesc desc;
        desc.byteSize = 16u;
        desc.format = BASE_FORMAT_R8G8B8A8_UINT;
        desc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        desc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_SRC_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        GpuBufferGLES buffer { device, desc };
        ASSERT_NE(nullptr, buffer.Map());
        ASSERT_NE(nullptr, buffer.MapMemory());
        buffer.Unmap();
    }
    {
        GpuBufferDesc desc;
        desc.byteSize = 16u;
        desc.format = BASE_FORMAT_R8G8B8A8_UINT;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        desc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_SRC_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        GpuBufferGLES buffer { device, desc };
        ASSERT_NE(nullptr, buffer.MapMemory());
        ASSERT_NE(nullptr, buffer.Map());
        buffer.Unmap();
    }
    device.Deactivate();
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
