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

#include "gfx/gfx_common.h"

#include <array>

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/post_process_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_morphing_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/ecs/systems/intf_render_system.h>
#include <3d/gltf/gltf.h>
#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/unique_ptr.h>
#include <base/math/quaternion_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/engine_info.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/plugin/intf_plugin_register.h>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/gles/intf_device_gles.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/util/intf_render_frame_util.h>
#include <render/util/intf_render_util.h>
#include <render/util/performance_data_structures.h>
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
using namespace CORE3D_NS;

// Vulkan (0), OpenGL (1)
#define OGL 0

CORE3D_BEGIN_NAMESPACE();
namespace UTest {

void TestResources::CreateTestWindow(int w, int h)
{
#if defined(__ANDROID__)
// skip
#elif defined(__OHOS__)
// skip
#elif defined(_WIN32)
    ::Test::g_windowsApp->CreateNativeWindow(w, h);
    ShowWindow(::Test::g_windowsApp->GetWindowHandle(), true);
    UpdateWindow(::Test::g_windowsApp->GetWindowHandle());
#elif defined(__linux__)
    ::Test::g_linuxApp->CreateNativeWindow(w, h);
#endif
}

void TestResources::DestroyTestWindow()
{
#if defined(__ANDROID__)
    // skip
#elif defined(__OHOS__)
    // skip
#elif defined(_WIN32)
    ::Test::g_windowsApp->DestroyNativeWindow();
#elif defined(__linux__)
    ::Test::g_linuxApp->DestroyNativeWindow();
#endif
}

void TestResources::TakeImageData()
{
#if 0
    engine_->CreateSwapchain(aSwapchainCreateInfo);
#else
    GpuImageDesc imageDesc;
    imageDesc.width = windowWidth_;
    imageDesc.height = windowHeight_;
    imageDesc.depth = 1;
    imageDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
    imageDesc.format = BASE_FORMAT_R8G8B8A8_SRGB;
    imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
    imageDesc.imageType = CORE_IMAGE_TYPE_2D;
    imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
    imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    imageDesc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageHandle_ = renderContext_->GetDevice().GetGpuResourceManager().Create("TestOutputImage", imageDesc);

    GpuBufferDesc bufferDesc;
    bufferDesc.byteSize = windowWidth_ * windowHeight_ * 4;
    bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
    bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    bufferHandle_ = renderContext_->GetDevice().GetGpuResourceManager().Create("TestOutputBuffer", bufferDesc);

    auto& rdsMgr = renderContext_->GetRenderDataStoreManager();

    dataStoreDataCopy_ = refcnt_ptr<IRenderDataStoreDefaultGpuResourceDataCopy>(
        rdsMgr.GetRenderDataStore("RenderDataStoreDefaultGpuResourceDataCopy"));
    if (dataStoreDataCopy_) {
        {
            IRenderFrameUtil::BackBufferConfiguration bbc;
            bbc.backBufferType = IRenderFrameUtil::BackBufferConfiguration::BackBufferType::GPU_IMAGE_BUFFER_COPY;
            bbc.present = false;
            bbc.backBufferHandle = imageHandle_;
            bbc.gpuBufferHandle = bufferHandle_;
            renderContext_->GetRenderUtil().GetRenderFrameUtil().SetBackBufferConfiguration(bbc);
        }
        byteArray_ = make_unique<ByteArray>(bufferDesc.byteSize);
    }
#endif
}

void TestResources::SetBackendExtra(DeviceCreateInfo& deviceCreateInfo)
{
#if defined(RENDER_HAS_GL_BACKEND) && RENDER_HAS_GL_BACKEND
    if (testBackend_ == DeviceBackendType::OPENGL) {
        deviceCreateInfo.backendType = DeviceBackendType::OPENGL;
        deviceCreateInfo.backendConfiguration = &glExtra_;
    }
#endif
    if (testBackend_ == DeviceBackendType::VULKAN) {
        deviceCreateInfo.backendType = DeviceBackendType::VULKAN;
        deviceCreateInfo.backendConfiguration = &vkExtra_;
    }
}

void TestResources::SetBackendSurface(SwapchainCreateInfo& swapchainCreateInfo, IDevice& device)
{
#if RENDER_HAS_VULKAN_BACKEND && defined(VK_USE_PLATFORM_WIN32_KHR)
    if (testBackend_ == DeviceBackendType::VULKAN) {
        VkSurfaceKHR surface { VK_NULL_HANDLE };
        const DevicePlatformDataVk& plat = static_cast<const DevicePlatformDataVk&>(device.GetPlatformData());
        VkWin32SurfaceCreateInfoKHR sci;
        sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        sci.hinstance = GetModuleHandle(NULL);
        sci.hwnd = ::Test::g_windowsApp->GetWindowHandle();
        sci.flags = 0;
        sci.pNext = nullptr;
        VkResult err = vkCreateWin32SurfaceKHR(plat.instance, &sci, nullptr, &surface);
        if (err) {
            CORE_LOG_F("Window surface creation failed.");
            return;
        }
        surface_ = (uint64_t)surface;

    } else
#elif RENDER_HAS_VULKAN_BACKEND && defined(VK_USE_PLATFORM_XCB_KHR)
    if (testBackend_ == DeviceBackendType::VULKAN) {
        VkSurfaceKHR surface = {};
        auto& plat = static_cast<const DevicePlatformDataVk&>(device.GetPlatformData());
        VkXcbSurfaceCreateInfoKHR sci = {};
        sci.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        sci.connection = ::Test::g_linuxApp->GetWindowConnection();
        sci.window = ::Test::g_linuxApp->GetWindowID();

        VkResult err = vkCreateXcbSurfaceKHR(plat.instance, &sci, nullptr, &surface);
        if (err) {
            CORE_LOG_F("Window surface creation failed.");
            return;
        }
        surface_ = (uint64_t)surface;
    } else
#endif
#if RENDER_HAS_GL_BACKEND && _WIN32
        if (testBackend_ == DeviceBackendType::OPENGL) {
        const DevicePlatformDataGL& data = ((const DevicePlatformDataGL&)device.GetPlatformData());
        surface_ = (uint64_t)GetDC(data.mhWnd);
    } else
#endif
#if defined(__ANDROID__)
        if (testBackend_ == DeviceBackendType::VULKAN) {
        VkResult result;
        VkSurfaceKHR surface { VK_NULL_HANDLE };
        const RENDER_NS::DevicePlatformDataVk& plat =
            static_cast<const RENDER_NS::DevicePlatformDataVk&>(device.GetPlatformData());

        auto vkCreateAndroidSurfaceKHR =
            (PFN_vkCreateAndroidSurfaceKHR)vkGetInstanceProcAddr(plat.instance, "vkCreateAndroidSurfaceKHR");
        if (!vkCreateAndroidSurfaceKHR) {
            CORE_LOG_E("Missing VK_KHR_android_surface extension!");
            return;
        }

        auto const surfaceCreateInfo =
            VkAndroidSurfaceCreateInfoKHR { VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, nullptr, 0,
                ::Test::g_androidApp->GetWindowHandle() };

        result = vkCreateAndroidSurfaceKHR(plat.instance, &surfaceCreateInfo, nullptr, &surface);
        if (result != VK_SUCCESS) {
            CORE_LOG_F("Window surface creation failed.");
            return;
        }
        surface_ = (uint64_t)surface;

    } else if (device.GetBackendType() == RENDER_NS::DeviceBackendType::OPENGL ||
               device.GetBackendType() == RENDER_NS::DeviceBackendType::OPENGLES) {
        EGLNativeWindowType window = ::Test::g_androidApp->GetWindowHandle();
        EGLint COLOR_SPACE = 0, COLOR_SPACE_SRGB = 0, COLOR_SPACE_LINEAR = 0;
#if !defined(EGL_VERSION_1_5)
        // If EGL_KHR_create_context extension not defined in headers, so just define the values here.
        // (copied from khronos specifications)
#define EGL_GL_COLORSPACE 0x309D
#define EGL_GL_COLORSPACE_SRGB 0x3089
#define EGL_GL_COLORSPACE_LINEAR 0x308A
#endif
#if !defined(EGL_KHR_gl_colorspace)
#define EGL_GL_COLORSPACE_KHR 0x309D
#define EGL_GL_COLORSPACE_SRGB_KHR 0x3089
#define EGL_GL_COLORSPACE_LINEAR_KHR 0x308A
#endif
        EGLSurface eglSurface = EGL_NO_SURFACE;
        const auto& data = ((const RENDER_NS::DevicePlatformDataGLES&)device.GetPlatformData());
        EGLConfig config = data.config;

        bool hasSRGB = true;
        if ((data.majorVersion > 1) || ((data.majorVersion == 1) && (data.minorVersion > 4))) {
            COLOR_SPACE = EGL_GL_COLORSPACE;
            COLOR_SPACE_SRGB = EGL_GL_COLORSPACE_SRGB;
            COLOR_SPACE_LINEAR = EGL_GL_COLORSPACE_LINEAR;
        } else if (data.hasColorSpaceExt) {
            COLOR_SPACE = EGL_GL_COLORSPACE_KHR;
            COLOR_SPACE_SRGB = EGL_GL_COLORSPACE_SRGB_KHR;
            COLOR_SPACE_LINEAR = EGL_GL_COLORSPACE_LINEAR_KHR;
        } else {
            hasSRGB = false;
        }

        if (hasSRGB) {
            const EGLint attribs[] = { COLOR_SPACE, COLOR_SPACE_SRGB, EGL_NONE };
            eglSurface = eglCreateWindowSurface(data.display, config, window, attribs);
            if (eglSurface == EGL_NO_SURFACE) {
                // okay fallback to whatever colorformat
                EGLint error = eglGetError();
                CORE_LOG_E("eglCreateWindowSurface failed to create SRGB surface: %d", error);
                hasSRGB = false;
            }
        }
        if (!hasSRGB) {
            eglSurface = eglCreateWindowSurface(data.display, config, window, nullptr);
            if (eglSurface == EGL_NO_SURFACE) {
                EGLint error = eglGetError();
                CORE_LOG_E("eglCreateWindowSurface failed (with null attributes): %d", error);
                CORE_ASSERT_MSG(false, "eglCreateWindowSurface failed (with null attributes): %d", error);
            }
        }
        surface_ = (uint64_t)eglSurface;
    } else
#endif
    {
        CORE_LOG_F("Unknown device backend.");
        return;
    }

    swapchainCreateInfo.surfaceHandle = (uint64_t)surface_;
    ASSERT_NE(swapchainCreateInfo.surfaceHandle, 0);
}

void TestResources::LiftTestUp(int viewportSizeX, int viewportSizeY)
{
    CreateTestWindow(viewportSizeX, viewportSizeY);

    IDevice* device = nullptr;

    UTest::TestContext* testContext = UTest::RecreateTestContext(testBackend_);
    engine_ = testContext->engine;
    renderContext_ = testContext->renderContext;
    graphicsContext_ = testContext->graphicsContext;

    device = &renderContext_->GetDevice();
    if constexpr (WITH_SWAPCHAIN) {
        SwapchainCreateInfo swapchainCreateInfo {};
#if defined(__OHOS__)
        swapchainCreateInfo.window.window = reinterpret_cast<uintptr_t>(::Test::g_ohosApp->GetWindowHandle());
        ASSERT_NE(::Test::g_ohosApp->GetWindowHandle(), nullptr);
#else
        SetBackendSurface(swapchainCreateInfo, *device);
#endif
        swapchainCreateInfo.swapchainFlags =
            SwapchainFlagBits::CORE_SWAPCHAIN_COLOR_BUFFER_BIT | SwapchainFlagBits::CORE_SWAPCHAIN_DEPTH_BUFFER_BIT |
            SwapchainFlagBits::CORE_SWAPCHAIN_VSYNC_BIT | SwapchainFlagBits::CORE_SWAPCHAIN_SRGB_BIT;

        renderContext_->GetDevice().CreateSwapchain(swapchainCreateInfo);
    } else {
        TakeImageData();
    }

    if (IPerformanceDataManagerFactory* globalPerfData =
            GetInstance<IPerformanceDataManagerFactory>(CORE_NS::UID_PERFORMANCE_FACTORY);
        globalPerfData) {
        if (IPerformanceDataManager* perfData =
                globalPerfData->Get(RenderPerformanceDataConstants::RENDER_NODE_COUNTERS_NAME);
            perfData) {
            perfData->ResetData();
        }
    }

    ecs_ = testContext->ecs;

    IRenderNodeGraphManager& graphManager = renderContext_->GetRenderNodeGraphManager();
    auto loader = &graphManager.GetRenderNodeGraphLoader();
    if (auto const result = loader->Load("test://test_core3d_rng_hdrp.rng"); !result.error.empty()) {
        CORE_LOG_E("Loading render node graph failed: %s", result.error.c_str());
    } else {
        constexpr string_view dataStoreName = "RenderDataStorePod";
        constexpr string_view postProcName = "PostProcessCore";
        IRenderDataStoreManager& renderDataStoreMgr = renderContext_->GetRenderDataStoreManager();
        auto dataStorePodPost = refcnt_ptr<IRenderDataStorePod>(renderDataStoreMgr.GetRenderDataStore(dataStoreName));
        if (dataStorePodPost) {
            const auto dataView = dataStorePodPost->Get(postProcName);
            if (!dataView.empty()) {
                CORE_ASSERT(dataView.size_bytes() == sizeof(PostProcessConfiguration));
                PostProcessConfiguration ppConfig;
                ppConfig.enableFlags = 0;
                ppConfig.enableFlags |= PostProcessConfiguration::ENABLE_TONEMAP_BIT;
                dataStorePodPost->Set(postProcName, arrayviewU8(ppConfig));
            }
        }

        // RendererDataHandle renderNodeGraph
        renderNodeGraph_ = graphManager.Create(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, result.desc);
    }
}

void TestResources::TickTest(int frameCountToTick)
{
    // Tick for 4 frames and collect imaging for comparison
    for (int i = 0; i < frameCountToTick; i++) {
        // stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);
        // We need modified or new rendernodegraph and possibly a buffer where to get the image write data.
        // stbi_write_png("./test.png", 200, 200, 0, &data, 100);

        auto* ecs = ecs_.get();
        engine_->TickFrame(array_view(&ecs, 1));

        if (i == frameCountToTick - 1) {
            if (dataStoreDataCopy_) {
                IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                dataCopy.gpuHandle = bufferHandle_;
                dataCopy.byteArray = byteArray_.get();
                dataStoreDataCopy_->AddCopyOperation(dataCopy);
            }
        }

        const RenderHandleReference rngs[] = { renderNodeGraph_ };
        renderContext_->GetRenderer().RenderFrame(array_view(rngs, 1));
    }
}

void TestResources::TickTestAutoRng(int frameCountToTick)
{
    // Tick for frameCountToTick frames and collect imaging for comparison
    for (int i = 0; i < frameCountToTick; i++) {
        // stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);
        // We need modified or new rendernodegraph and possibly a buffer where to get the image write data.
        // stbi_write_png("./test.png", 200, 200, 0, &data, 100);
        auto* ecs = ecs_.get();
        engine_->TickFrame(array_view(&ecs, 1));

        if (i == frameCountToTick - 1) {
            if (dataStoreDataCopy_) {
                IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                dataCopy.gpuHandle = bufferHandle_;
                dataCopy.byteArray = byteArray_.get();
                dataStoreDataCopy_->AddCopyOperation(dataCopy);
            }
        }

        const auto renderNodeGraphs = graphicsContext_->GetRenderNodeGraphs(*ecs);
        renderContext_->GetRenderer().RenderFrame(renderNodeGraphs);
    }
}

void TestResources::ShutdownTest()
{
    byteArray_.reset();
    if constexpr (WITH_SWAPCHAIN) {
        renderContext_->GetDevice().DestroySwapchain();

        // Destroy surface
        IDevice& device = renderContext_->GetDevice();
#if RENDER_HAS_VULKAN_BACKEND
        if (device.GetBackendType() == DeviceBackendType::VULKAN) {
            const RENDER_NS::DevicePlatformDataVk& plat =
                static_cast<const RENDER_NS::DevicePlatformDataVk&>(device.GetPlatformData());
            vkDestroySurfaceKHR(plat.instance, (VkSurfaceKHR)surface_, nullptr);
        } else
#endif
#if RENDER_HAS_GLES_BACKEND
            if (device.GetBackendType() == DeviceBackendType::OPENGLES) {
            const DevicePlatformDataGLES& data = ((const DevicePlatformDataGLES&)device.GetPlatformData());
            eglDestroySurface(data.display, (EGLSurface)surface_);
        } else
#endif
#if RENDER_HAS_GL_BACKEND && _WIN32
            if (device.GetBackendType() == DeviceBackendType::OPENGL) {
            const RENDER_NS::DevicePlatformDataGL& data =
                ((const RENDER_NS::DevicePlatformDataGL&)device.GetPlatformData());
            ReleaseDC(data.mhWnd, (HDC)surface_);
        } else
#endif
        {
            CORE_LOG_F("Destroy surface - unknown device backend.");
        }
    }

    imageHandle_ = {};
    bufferHandle_ = {};
    renderNodeGraph_ = {};
    importedResources_.clear();
    DestroyTestWindow();
}

IEcs& TestResources::GetEcs()
{
    return *ecs_;
}

IGraphicsContext& TestResources::GetGraphicsContext()
{
    return *graphicsContext_;
}

IEngine& TestResources::GetEngine()
{
    return *engine_;
}

IRenderContext& TestResources::GetRenderContext()
{
    return *renderContext_;
}

uint32_t TestResources::GetWindowWidth()
{
    return windowWidth_;
}

uint32_t TestResources::GetWindowHeight()
{
    return windowHeight_;
}

RenderHandleReference TestResources::GetImage()
{
    return imageHandle_;
}

ByteArray* TestResources::GetByteArray()
{
    return byteArray_.get();
}

vector<EntityReference>& TestResources::GetImportedResources()
{
    return importedResources_;
}

DeviceBackendType TestResources::GetDeviceBackendType()
{
    return testBackend_;
}

void TestResources::AppendResources(const GLTFResourceData& resources)
{
    importedResources_.reserve(importedResources_.size() + resources.samplers.size() + resources.images.size() +
                               resources.textures.size() + resources.materials.size() + resources.meshes.size() +
                               resources.skins.size() + resources.animations.size() +
                               resources.specularRadianceCubemaps.size());

    importedResources_.append(resources.samplers.begin(), resources.samplers.end());
    importedResources_.append(resources.images.begin(), resources.images.end());
    importedResources_.append(resources.textures.begin(), resources.textures.end());

    importedResources_.append(resources.materials.begin(), resources.materials.end());
    importedResources_.append(resources.meshes.begin(), resources.meshes.end());
    importedResources_.append(resources.skins.begin(), resources.skins.end());
    importedResources_.append(resources.animations.begin(), resources.animations.end());
    importedResources_.insert(
        importedResources_.end(), resources.specularRadianceCubemaps.begin(), resources.specularRadianceCubemaps.end());
    importedResources_.append(resources.samplers.begin(), resources.samplers.end());
}

Entity CreateSolidColorMaterial(IEcs& ecs, Math::Vec4 color)
{
    IMaterialComponentManager* materialManager = GetManager<IMaterialComponentManager>(ecs);
    Entity entity = ecs.GetEntityManager().Create();
    materialManager->Create(entity);
    if (auto materialHandle = materialManager->Write(entity); materialHandle) {
        materialHandle->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor = color;
    }
    return entity;
}

} // namespace UTest
CORE3D_END_NAMESPACE()
