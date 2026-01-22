/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <native_buffer/native_buffer.h>

#include <base/math/matrix.h>
#include <base/math/matrix_util.h>
#include <base/math/vector.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
#include <render/resource_handle.h>

#include "log_util.h"
#if RENDER_HAS_GLES_BACKEND || RENDER_HAS_GL_BACKEND
#include <render/gles/intf_device_gles.h>
#endif
#if RENDER_HAS_VULKAN_BACKEND
#include <render/vulkan/intf_device_vk.h>
#endif
#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
namespace {

constexpr BASE_NS::Math::Mat3X3 bt709(
    { 0.2126f, -0.1146f, 0.5f }, { 0.7152f, -0.3854f, -0.4542f }, { 0.0722f, 0.5f, -0.0458f });
constexpr BASE_NS::Math::Mat3X3 bt601(
    { 65.481f, -37.797f, 112.f }, { 128.553f, -74.203f, -93.786f }, { 24.966, 112.f, -18.214f });

constexpr BASE_NS::Math::Vec3 red(1.f, 0.f, 0.f);
constexpr BASE_NS::Math::Vec3 green(0.f, 1.f, 0.f);
constexpr BASE_NS::Math::Vec3 blue(0.f, 0.f, 1.f);
constexpr BASE_NS::Math::Vec3 white(1.f, 1.f, 1.f);
constexpr BASE_NS::Math::Vec3 offset(16.f, 128.f, 128.f);
constexpr BASE_NS::Math::Vec3 range(219.f, 224.f, 224.f);
struct TestResources {
    RENDER_NS::RenderHandleReference cpuCopyBufferHandle;
    RENDER_NS::RenderHandleReference outputImageHandle;
    // TestImage images[TEST_IMAGE_COUNT];

    // BASE_NS::unique_ptr<BASE_NS::ByteArray> byteArray;
    RENDER_NS::GpuImageDesc imageDesc;
    RENDER_NS::GpuBufferDesc bufferDesc;
};
struct TestData {
    RENDER_NS::UTest::EngineResources engine;
    RENDER_NS::RenderHandleReference renderNodeGraph;
    TestResources resources;

    uint32_t windowWidth { 0u };
    uint32_t windowHeight { 0u };
    uint32_t elementByteSize { 4u };
};
RENDER_NS::RenderHandleReference CreateColorImage(RENDER_NS::IRenderContext& renderContext)
{
    /* how native_buffer.h formats map to BASE_NS::Format:
     NATIVEBUFFER_PIXEL_FMT_YUV_422_I    BASE_FORMAT_UNDEFINED                 YUV422 interleaved
     NATIVEBUFFER_PIXEL_FMT_YCBCR_422_SP BASE_FORMAT_G8_B8R8_2PLANE_422_UNORM  YCBCR422 semi-plannar
     NATIVEBUFFER_PIXEL_FMT_YCRCB_422_SP BASE_FORMAT_UNDEFINED                 YCRCB422 semi-plannar
     NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP BASE_FORMAT_G8_B8R8_2PLANE_420_UNORM  YCBCR420 semi-plannar
     NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP BASE_FORMAT_UNDEFINED                 YCRCB420 semi-plannar
     NATIVEBUFFER_PIXEL_FMT_YCBCR_422_P  BASE_FORMAT_G8_B8_R8_3PLANE_422_UNORM YCBCR422 plannar
     NATIVEBUFFER_PIXEL_FMT_YCRCB_422_P  BASE_FORMAT_UNDEFINED                 YCRCB422 plannar
     NATIVEBUFFER_PIXEL_FMT_YCBCR_420_P  BASE_FORMAT_G8_B8_R8_3PLANE_420_UNORM YCBCR420 plannar NV12
     NATIVEBUFFER_PIXEL_FMT_YCRCB_420_P  BASE_FORMAT_UNDEFINED                 YCRCB420 plannar NV21
     NATIVEBUFFER_PIXEL_FMT_YUYV_422_PKG BASE_FORMAT_UNDEFINED                 YUYV422 packed
     NATIVEBUFFER_PIXEL_FMT_UYVY_422_PKG BASE_FORMAT_UNDEFINED                 UYVY422 packed
     NATIVEBUFFER_PIXEL_FMT_YVYU_422_PKG BASE_FORMAT_UNDEFINED                 YVYU422 packed
     NATIVEBUFFER_PIXEL_FMT_VYUY_422_PKG BASE_FORMAT_UNDEFINED                 VYUY422 packed
     NATIVEBUFFER_PIXEL_FMT_RGBA_1010102 BASE_FORMAT_A2R10G10B10_UNORM_PACK32  RGBA_1010102 packed
     NATIVEBUFFER_PIXEL_FMT_YCBCR_P010   BASE_FORMAT_UNDEFINED                 YCBCR420 semi-planar 10bit packed
     NATIVEBUFFER_PIXEL_FMT_YCRCB_P010   BASE_FORMAT_UNDEFINED                 YCRCB420 semi-planar 10bit packed
     NATIVEBUFFER_PIXEL_FMT_RAW10        BASE_FORMAT_UNDEFINED                 Raw 10bit packed
     */
    // Mate60 Pro 709, ITU_NARROW, MIDPOINT
    constexpr auto redYCrCb = (bt709 * red) * range + offset;
    constexpr auto greenYCrCb = (bt709 * green) * range + offset;
    constexpr auto blueYCrCb = (bt709 * blue) * range + offset;
    constexpr auto whiteYCrCb = (bt709 * white) * range + offset;

    constexpr const uint8_t dataY[] = { uint8_t(redYCrCb.x), uint8_t(redYCrCb.x), uint8_t(greenYCrCb.x),
        uint8_t(greenYCrCb.x), uint8_t(redYCrCb.x), uint8_t(redYCrCb.x), uint8_t(greenYCrCb.x), uint8_t(greenYCrCb.x),
        uint8_t(blueYCrCb.x), uint8_t(blueYCrCb.x), uint8_t(whiteYCrCb.x), uint8_t(whiteYCrCb.x), uint8_t(blueYCrCb.x),
        uint8_t(blueYCrCb.x), uint8_t(whiteYCrCb.x), uint8_t(whiteYCrCb.x) };
    constexpr const uint8_t dataCb[] = { uint8_t(redYCrCb.y), uint8_t(greenYCrCb.y), uint8_t(blueYCrCb.y),
        uint8_t(whiteYCrCb.y) };
    constexpr const uint8_t dataCr[] = { uint8_t(redYCrCb.z), uint8_t(greenYCrCb.z), uint8_t(blueYCrCb.z),
        uint8_t(whiteYCrCb.z) };

    OH_NativeBuffer_Config config { 4, 4, OH_NativeBuffer_Format::NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP,
        OH_NativeBuffer_Usage::NATIVEBUFFER_USAGE_HW_TEXTURE, 8 };

    OH_NativeBuffer* buffer = OH_NativeBuffer_Alloc(&config);
    if (!buffer) {
        CORE_LOG_E("OH_NativeBuffer_Alloc failed");
        return {};
    }
    OH_NativeBuffer_GetConfig(buffer, &config);
    void* ptr = nullptr;
    OH_NativeBuffer_Planes planes {};
    if (auto err = OH_NativeBuffer_MapPlanes(buffer, &ptr, &planes); err || !ptr) {
        CORE_LOG_E("OH_NativeBuffer_Map failed %d", err);
        return {};
    }
    auto* src = dataY;
    for (int row = 0; row < config.height; ++row) {
        auto dst = reinterpret_cast<uint8_t*>(ptr) + planes.planes[0].offset + row * planes.planes[0].columnStride;
        for (int col = 0; col < config.width; ++col) {
            *dst = *src;
            ++src;
            dst += planes.planes[0].rowStride;
        }
    }
    src = dataCb;
    for (int row = 0; row < config.height / 2; ++row) {
        auto dst = reinterpret_cast<uint8_t*>(ptr) + planes.planes[1].offset + row * planes.planes[1].columnStride;
        for (int col = 0; col < config.width / 2; ++col) {
            *dst = *src;
            ++src;
            dst += planes.planes[1].rowStride;
        }
    }

    src = dataCr;
    for (int row = 0; row < config.height / 2; ++row) {
        auto dst = reinterpret_cast<uint8_t*>(ptr) + planes.planes[2].offset + row * planes.planes[2].columnStride;
        for (int col = 0; col < config.width / 2; ++col) {
            *dst = *src;
            ++src;
            dst += planes.planes[2].rowStride;
        }
    }

    IGpuResourceManager& gpuResourceMgr = renderContext.GetDevice().GetGpuResourceManager();
    constexpr GpuImageDesc desc { ImageType::CORE_IMAGE_TYPE_2D, ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
        Format::BASE_FORMAT_UNDEFINED, ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
        ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT, MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        0, // ImageCreateFlags
        0, // EngineImageCreationFlags
        4, 4, 1, 1, 1, SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT, {} };

    if (renderContext.GetDevice().GetBackendType() == RENDER_NS::DeviceBackendType::VULKAN) {
        const RENDER_NS::ImageDescVk plat { {}, VK_NULL_HANDLE, VK_NULL_HANDLE, reinterpret_cast<uintptr_t>(buffer) };
        return gpuResourceMgr.CreateView("", gpuImageDesc, plat);
    } else {
        const RENDER_NS::ImageDescGLES plat { {}, {}, {}, {}, {}, {}, {}, {}, reinterpret_cast<uintptr_t>(buffer) };
        return gpuResourceMgr.CreateView("", gpuImageDesc, plat);
    }
}

RENDER_NS::RenderHandleReference CreateBuffer(RENDER_NS::IRenderContext& renderContext)
{
    OH_NativeBuffer_Config config { 256, 1, OH_NativeBuffer_Format::NATIVEBUFFER_PIXEL_FMT_BLOB,
        OH_NativeBuffer_Usage::NATIVEBUFFER_USAGE_CPU_WRITE | OH_NativeBuffer_Usage::NATIVEBUFFER_USAGE_HW_TEXTURE,
        256 };

    OH_NativeBuffer* buffer = OH_NativeBuffer_Alloc(&config);
    if (!buffer) {
        CORE_LOG_E("OH_NativeBuffer_Alloc failed");
        return {};
    }
    OH_NativeBuffer_GetConfig(buffer, &config);

    void* ptr;
    if (OH_NativeBuffer_Map(buffer, &ptr) != 0) {
        CORE_LOG_E("OH_NativeBuffer_Map failed");
        return {};
    }
    *static_cast<BASE_NS::Math::Vec4*>(ptr) = BASE_NS::Math::Vec4(0.1f, 0.2f, 0.5f, 1.f);
    OH_NativeBuffer_Unmap(buffer);

    IGpuResourceManager& gpuResourceMgr = renderContext.GetDevice().GetGpuResourceManager();
    const GpuBufferDesc gpuBufferDesc { BufferUsageFlagBits::CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                            BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, static_cast<uint32_t>(config.width),
        Format::BASE_FORMAT_UNDEFINED };
    const BufferDescVk plat { {}, VK_NULL_HANDLE, reinterpret_cast<uintptr_t>(buffer) };
    return gpuResourceMgr.Create("uNativeBuffer", plat);
}

void TestNativeBuffer(RENDER_NS::DeviceBackendType backend)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == RENDER_NS::DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }

    {
        CreateEngineSetup(testData.engine);
    }
    auto res = CreateColorImage(*testData.engine.context);
    testData.engine.context->GetRenderer().RenderFrame({});
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: NativeBufferTestVulkan
 * @tc.desc: Tests ImageView creation from OH_NativeBuffer.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceManagerTest, NativeBufferTestVulkan, testing::ext::TestSize.Level1)
{
    TestNativeBuffer(RENDER_NS::DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: NativeBufferTestOpenGL
 * @tc.desc: Tests ImageView creation from OH_NativeBuffer.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceManagerTest, NativeBufferTestOpenGL, testing::ext::TestSize.Level1)
{
    TestNativeBuffer(RENDER_NS::UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
