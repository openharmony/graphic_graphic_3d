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

#include "test_runner_ohos_system.h"

#if defined(USE_STB_IMAGE) && (USE_STB_IMAGE == 1)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#endif

namespace Test {

void OHOSApp::CreateNativeWindow()
{
    // Using graphic_2d native image and getting NativeWindow from it
    m_nativeImage = OH_NativeImage_Create(0, 0);
    ASSERT_NE(m_nativeImage, nullptr);
    m_nativeWindow = OH_NativeImage_AcquireNativeWindow(m_nativeImage);
    ASSERT_NE(m_nativeWindow, nullptr);
}

void OHOSApp::Terminate()
{
    // Destroy native image and native window
    OH_NativeImage_Destroy(&m_nativeImage);
}

} // namespace Test

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
CORE_NS::IPluginRegister& (*CORE_NS::GetPluginRegister)() { nullptr };
void (*CORE_NS::CreatePluginRegistry)(const struct CORE_NS::PlatformCreateInfo& platformCreateInfo) { nullptr };
#endif

RENDER_BEGIN_NAMESPACE()
namespace UTest {

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
DeviceBackendType GetOpenGLBackend()
{
#if RENDER_HAS_GLES_BACKEND
    return DeviceBackendType::OPENGLES;
#else // RENDER_HAS_GL_BACKEND
    return DeviceBackendType::OPENGL;
#endif
}
#endif

TestEnvironment* GetTestEnv()
{
    CORE_ASSERT(g_testEnv.get());
    return g_testEnv.get();
}

void RegisterPaths(CORE_NS::IEngine& engine)
{
    // Register filesystems/paths used in the tests.
    CORE_NS::IFileManager& fileManager = engine.GetFileManager();

    // OHOS specific paths.
    const BASE_NS::string applicationDirectory = "file://" + GetTestEnv()->platformCreateInfo.appRootPath;
    const BASE_NS::string cacheDirectory = applicationDirectory + "cache/";

    if (fileManager.OpenDirectory(cacheDirectory) == nullptr) {
        const auto _ = fileManager.CreateDirectory(cacheDirectory);
    }
    CORE_ASSERT(fileManager.OpenDirectory(cacheDirectory) != nullptr);

    // Create app:// protocol that points to application root directory or application private data directory.
    fileManager.RegisterPath("app", applicationDirectory, true);
    // Create cache:// protocol that points to application cache directory.
    fileManager.RegisterPath("cache", cacheDirectory, true);
}

void RegisterTestFileProtocol(CORE_NS::IFileManager& fileManager, const DeviceBackendType& backend)
{
    // OHOS specific paths
    const BASE_NS::string applicationTestAssetsDirectory = "file://" + GetTestEnv()->appAssetPath;

    // Create test:// protocol that points to application test data directory.
    fileManager.RegisterPath("test", applicationTestAssetsDirectory, true);

    fileManager.RegisterPath("shaders", applicationTestAssetsDirectory + "shaders/", true);
    fileManager.RegisterPath("rendershaders", applicationTestAssetsDirectory + "shaders/", true);
    if (backend == DeviceBackendType::VULKAN) {
        fileManager.RegisterPath("shaders", applicationTestAssetsDirectory + "vulkanspecific/shaders/", true);
        fileManager.RegisterPath("rendershaders", applicationTestAssetsDirectory + "vulkanspecific/shaders/", true);
    }
}

enum class DesiredOutput { SDR_LINEAR, SDR_SRGB, HDR_1010102, HDR_SCRGB_F16 };

struct BackbufferReq {
    bool needDefaultDepthStencil = false;
    int minDepthBits = 0;
    int minStencilBits = 0;
    int msaaSamples = 0;
};
#if RENDER_HAS_GLES_BACKEND
#ifndef EGL_KHR_no_config_context
// If #ifndef EGL_KHR_no_config_context extension not defined in headers, so just define the values here.
// (copied from khronos specifications)
#define EGL_NO_CONFIG_KHR ((EGLConfig)0)
#endif

static EGLConfig TryChoose(EGLDisplay dpy, const EGLint* attrs)
{
    EGLConfig cfg = EGL_NO_CONFIG_KHR;
    EGLint n = 0;
    if (eglChooseConfig(dpy, attrs, &cfg, 1, &n) && n > 0)
        return cfg;
    return EGL_NO_CONFIG_KHR;
}

static void Add(BASE_NS::vector<EGLint>& a, EGLint k, EGLint v)
{
    a.push_back(k);
    a.push_back(v);
}

static EGLConfig TryPattern(EGLDisplay dpy, const BackbufferReq& req, int r, int g, int b, int aBits, int depthBits,
    int stencilBits, bool wantFloatComponents, int msaaSamples)
{
    BASE_NS::vector<EGLint> at;
    Add(at, EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT);
    Add(at, EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER);
    Add(at, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT); // or ES2 fallback if needed

    Add(at, EGL_RED_SIZE, r);
    Add(at, EGL_GREEN_SIZE, g);
    Add(at, EGL_BLUE_SIZE, b);
    Add(at, EGL_ALPHA_SIZE, aBits);

    if (req.needDefaultDepthStencil) {
        Add(at, EGL_DEPTH_SIZE, depthBits);
        if (stencilBits > 0)
            Add(at, EGL_STENCIL_SIZE, stencilBits);
    }

#ifdef EGL_COLOR_COMPONENT_TYPE_EXT
    if (wantFloatComponents) {
        // Requires EGL_EXT_pixel_format_float
        Add(at, EGL_COLOR_COMPONENT_TYPE_EXT, EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT);
    }
#endif

    Add(at, EGL_CONFIG_CAVEAT, EGL_NONE);
    if (msaaSamples > 1) {
        Add(at, EGL_SAMPLES, (EGLint)msaaSamples);
        Add(at, EGL_SAMPLE_BUFFERS, 1);
    }
    at.push_back(EGL_NONE);
    at.push_back(EGL_NONE);

    return TryChoose(dpy, at.data());
}

EGLConfig PickFirstWindowConfig(EGLDisplay dpy, const BackbufferReq& req, DesiredOutput desired,
    bool havePixelFloatExt /* EGL_EXT_pixel_format_float */)
{
    switch (desired) {
        case DesiredOutput::HDR_SCRGB_F16:
            if (havePixelFloatExt) {
                if (auto c = TryPattern(
                        dpy, req, 16, 16, 16, 16, req.minDepthBits, req.minStencilBits, true, req.msaaSamples))
                    return c;
            }
        case DesiredOutput::HDR_1010102: {
            // 10:10:10:2
            if (auto c =
                    TryPattern(dpy, req, 10, 10, 10, 2, req.minDepthBits, req.minStencilBits, false, req.msaaSamples))
                return c;
        }
        case DesiredOutput::SDR_SRGB:
        case DesiredOutput::SDR_LINEAR: {
            // Prefer 8:8:8:8
            if (auto c =
                    TryPattern(dpy, req, 8, 8, 8, 8, req.minDepthBits, req.minStencilBits, false, req.msaaSamples)) {
                return c;
            }
            break;
        }
    }
    return EGL_NO_CONFIG_KHR;
}
#endif

uint64_t CreateSurface(const EngineResources& er, const BASE_NS::string& name)
{
    // skip
    return 0;
}

void DestroySurface(const EngineResources& er, uint64_t surfaceHandle)
{
    // Skip
    return;
}

void CreateEngineSetup(EngineResources& er)
{
    const ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    if (er.createWindow) {
        // skip
    }

    const CORE_NS::EngineCreateInfo engineCreateInfo { GetTestEnv()->platformCreateInfo,
        {
            "lume_test", // name
            1,           // versionMajor
            0,           // versionMinor
            0,           // versionPatch
        },
        // applicationContext
        {} };

    auto factory = CORE_NS::GetInstance<CORE_NS::IEngineFactory>(CORE_NS::UID_ENGINE_FACTORY);
    er.engine = factory->Create(engineCreateInfo);

    RegisterPaths(*er.engine);
    RegisterTestFileProtocol(er.engine->GetFileManager(), er.backend);
    er.engine->Init();

    {
        er.context.reset(static_cast<IRenderContext*>(
            er.engine->GetInterface<CORE_NS::IClassFactory>()->CreateInstance(UID_RENDER_CONTEXT).get()));
        ASSERT_TRUE(er.context);

        DeviceCreateInfo deviceCreateInfo;
#if RENDER_HAS_VULKAN_BACKEND
        BackendExtraVk vkExtra;
        if (er.backend == DeviceBackendType::VULKAN) {
            vkExtra.enableMultiQueue = er.enableMultiQueue;
            deviceCreateInfo.backendType = er.backend;
            deviceCreateInfo.backendConfiguration = &vkExtra;
        }

#endif

#if RENDER_HAS_GL_BACKEND
        BackendExtraGL glExtra;
        if (er.backend == DeviceBackendType::OPENGL) {
            glExtra.MSAASamples = 0;
            glExtra.depthBits = 24;
            glExtra.alphaBits = 8;
            glExtra.stencilBits = 0;
            deviceCreateInfo.backendType = DeviceBackendType::OPENGL;
            deviceCreateInfo.backendConfiguration = &glExtra;
        }
#endif

#if RENDER_HAS_GLES_BACKEND
        BackendExtraGLES glesExtra;
        if (er.backend == DeviceBackendType::OPENGLES) {
            glesExtra.MSAASamples = 0;
            glesExtra.depthBits = 24;
            glesExtra.alphaBits = 8;
            glesExtra.stencilBits = 0;
            deviceCreateInfo.backendType = DeviceBackendType::OPENGLES;
            deviceCreateInfo.backendConfiguration = &glesExtra;
        }
#endif

        const RenderCreateInfo info {
            {
                "lume_test", // name
                1,           // versionMajor
                0,           // versionMinor
                0,           // versionPatch
            },
            deviceCreateInfo,
        };
        const RenderResultCode rrc = er.context->Init(info);
        if (rrc == RenderResultCode::RENDER_SUCCESS) {
            er.device = &er.context->GetDevice();
            er.context->GetRenderer().RenderFrame({});
        } else {
            CORE_LOG_E("Render context init failed");
            ASSERT_TRUE(rrc == RenderResultCode::RENDER_SUCCESS);
        }
    }
}

void DestroyEngine(EngineResources& er)
{
    er.context.reset();
    er.engine.reset();
    if (er.createWindow) {
        // skip
    }
}

void CreateNativeWindow(int width, int height, const BASE_NS::string& name)
{
    // skip
}

void DestroyNativeWindow(const BASE_NS::string& name)
{
    // skip
}

void WritePng(
    const BASE_NS::string& fileName, uint32_t x, uint32_t y, uint32_t comp, const void* data, uint32_t strideBytes)
{
#if defined(USE_STB_IMAGE) && (USE_STB_IMAGE == 1)
    stbi_write_png(fileName.c_str(), static_cast<int32_t>(x), static_cast<int32_t>(y), static_cast<int32_t>(comp), data,
        static_cast<int32_t>(strideBytes));
#endif
}

constexpr uint32_t ToLinear(BASE_NS::Color color)
{
    color *= 255.f;
    uint32_t r = ((uint32_t)color.x & UINT8_MAX) << 0u;
    uint32_t g = ((uint32_t)color.y & UINT8_MAX) << 8u;
    uint32_t b = ((uint32_t)color.z & UINT8_MAX) << 16u;
    uint32_t a = ((uint32_t)color.w & UINT8_MAX) << 24u;

    return r | g | b | a;
}

void SaveImage(const BASE_NS::string& fileName, uint32_t x, uint32_t y, const float* floatData)
{
    uint32_t* data = new uint32_t[x * y];
    for (size_t i = 0; i < x; ++i) {
        for (size_t j = 0; j < y; ++j) {
            float R = floatData[i * y * 4u + j * 4u + 0u];
            float G = floatData[i * y * 4u + j * 4u + 1u];
            float B = floatData[i * y * 4u + j * 4u + 2u];
            float A = floatData[i * y * 4u + j * 4u + 3u];
            data[i * y + j] = ToLinear(BASE_NS::Color(R, G, B, A));
        }
    }
    WritePng(fileName, x, y, 4u, data, y * 4u);
    delete[] data;
}

float FromHDR(float color)
{
    return color / (color + 1.f);
}

void SaveHdrImage(const BASE_NS::string& fileName, uint32_t x, uint32_t y, const float* floatData)
{
    uint32_t* data = new uint32_t[x * y];
    for (size_t i = 0; i < x; ++i) {
        for (size_t j = 0; j < y; ++j) {
            float R = FromHDR(floatData[i * y * 4u + j * 4u + 0u]);
            float G = FromHDR(floatData[i * y * 4u + j * 4u + 1u]);
            float B = FromHDR(floatData[i * y * 4u + j * 4u + 2u]);
            float A = floatData[i * y * 4u + j * 4u + 3u];
            data[i * y + j] = ToLinear(BASE_NS::Color(R, G, B, A));
        }
    }
    WritePng(fileName, x, y, 4u, data, y * 4u);
    delete[] data;
}

void PrintDeviceInfo(EngineResources& er)
{
    const ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_INFO);
#if RENDER_HAS_VULKAN_BACKEND
    if (er.backend == DeviceBackendType::VULKAN) {
        auto& dataPlat = static_cast<const RENDER_NS::DevicePlatformDataVk&>(er.device->GetPlatformData());
        auto deviceName = BASE_NS::string(dataPlat.physicalDeviceProperties.physicalDeviceProperties.deviceName);
        CORE_LOG_I("CPU vendor: %s", GetTestEnv()->cpuVendor.c_str());
        CORE_LOG_I("GPU vendor: %s", deviceName.c_str());
    }
#endif
#if RENDER_HAS_GL_BACKEND
    if (er.backend == DeviceBackendType::OPENGL) {
        auto& dataPlatGL = static_cast<const RENDER_NS::DevicePlatformDataGL&>(er.device->GetPlatformData());
        auto deviceNameGL = BASE_NS::string(dataPlatGL.deviceName);
        CORE_LOG_I("CPU vendor: %s", GetTestEnv()->cpuVendor.c_str());
        CORE_LOG_I("GPU vendor: %s", deviceNameGL.c_str());
    }
#endif
}

} // namespace UTest
RENDER_END_NAMESPACE()

testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new RENDER_NS::UTest::TestRunnerEnv);
