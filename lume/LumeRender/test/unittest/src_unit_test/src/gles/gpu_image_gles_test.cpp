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
#include <gles/gpu_image_gles.h>
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
 * @tc.name: GpuImageTestOpenGL
 * @tc.desc: Tests GLES image creation with differen usage flags and swizzling.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuImage, GpuImageTestOpenGL, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = UTest::GetOpenGLBackend();
    engine.createWindow = true;
    UTest::CreateEngineSetup(engine);
    Device& device = *static_cast<Device*>(engine.device);
    device.Activate();
    {
        GpuImageDesc desc;
        desc.width = 2u;
        desc.height = 2u;
        desc.depth = 1u;
        desc.format = BASE_FORMAT_B10G11R11_UFLOAT_PACK32;
        desc.componentMapping.r = CORE_COMPONENT_SWIZZLE_ZERO;
        desc.componentMapping.g = CORE_COMPONENT_SWIZZLE_ONE;
        desc.componentMapping.b = CORE_COMPONENT_SWIZZLE_R;
        desc.componentMapping.a = CORE_COMPONENT_SWIZZLE_G;
        desc.sampleCountFlags = CORE_SAMPLE_COUNT_4_BIT;
        desc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        desc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        desc.imageType = CORE_IMAGE_TYPE_2D;
        desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        desc.layerCount = 1u;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.mipCount = 1u;
        desc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        GpuImageGLES image { device, desc };
        auto realDesc = image.GetDesc();
        ASSERT_EQ(desc.width, realDesc.width);
        ASSERT_EQ(desc.height, realDesc.height);
        ASSERT_EQ(0u, image.GetAdditionalFlags());
    }
    {
        GpuImageDesc desc;
        desc.width = 2u;
        desc.height = 2u;
        desc.depth = 1u;
        desc.format = BASE_FORMAT_B10G11R11_UFLOAT_PACK32;
        desc.componentMapping.r = CORE_COMPONENT_SWIZZLE_ZERO;
        desc.componentMapping.g = CORE_COMPONENT_SWIZZLE_ONE;
        desc.componentMapping.b = CORE_COMPONENT_SWIZZLE_R;
        desc.componentMapping.a = CORE_COMPONENT_SWIZZLE_G;
        desc.sampleCountFlags = CORE_SAMPLE_COUNT_4_BIT;
        desc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        desc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        desc.imageType = CORE_IMAGE_TYPE_2D;
        desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        desc.layerCount = 1u;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.mipCount = 1u;
        desc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
        GpuImageGLES image { device, desc };
        auto realDesc = image.GetDesc();
        ASSERT_EQ(desc.width, realDesc.width);
        ASSERT_EQ(desc.height, realDesc.height);
        ASSERT_EQ(0u, image.GetAdditionalFlags());
    }
    {
        GpuImageDesc desc;
        desc.width = 2u;
        desc.height = 2u;
        desc.depth = 1u;
        desc.format = BASE_FORMAT_B10G11R11_UFLOAT_PACK32;
        desc.componentMapping.r = CORE_COMPONENT_SWIZZLE_B;
        desc.componentMapping.g = CORE_COMPONENT_SWIZZLE_A;
        desc.componentMapping.b = CORE_COMPONENT_SWIZZLE_R;
        desc.componentMapping.a = CORE_COMPONENT_SWIZZLE_G;
        desc.sampleCountFlags = CORE_SAMPLE_COUNT_4_BIT;
        desc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        desc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        desc.imageType = CORE_IMAGE_TYPE_2D;
        desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        desc.layerCount = 1u;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.mipCount = 1u;
        desc.usageFlags = CORE_IMAGE_USAGE_STORAGE_BIT;
        GpuImageGLES image { device, desc };
        auto realDesc = image.GetDesc();
        ASSERT_EQ(desc.width, realDesc.width);
        ASSERT_EQ(desc.height, realDesc.height);
        ASSERT_EQ(0u, image.GetAdditionalFlags());
    }
    device.Deactivate();
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
