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

#ifndef TEST_ENVIRONMENT
#define TEST_ENVIRONMENT

#include <jpg/implementation_uids.h>
#include <png/implementation_uids.h>

#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/util/color.h>
#include <core/engine_info.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_filesystem_api.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/os/intf_platform.h>
#include <core/os/platform_create_info.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>

#include "test_framework.h"

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
#include <core/plugin/intf_plugin_register.h>

#include "load_lib.h"
#endif

#if (RENDER_HAS_VULKAN_BACKEND)
#include <vulkan/vulkan.h>

#include <render/vulkan/intf_device_vk.h>
#endif
#if (RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND)
#include <render/gles/intf_device_gles.h>
#endif

#include <native_image.h> //native window

namespace Test {
// Scoped log. Previous log level is restored when the LogLevelScope object goes out of scope.
class LogLevelScope {
public:
    LogLevelScope(CORE_NS::ILogger* logger, CORE_NS::ILogger::LogLevel logLevel)
    {
        if (logger_) {
            // Save the current log level and set a new one.
            oldLevel_ = logger_->GetLogLevel();
            logger_->SetLogLevel(logLevel);
        }
    }
    ~LogLevelScope()
    {
        if (logger_) {
            // Restore previous log level.
            logger_->SetLogLevel(oldLevel_);
        }
    }

private:
    CORE_NS::ILogger* logger_;
    CORE_NS::ILogger::LogLevel oldLevel_;
};

class OHOSApp {
public:
    OHOSApp()
    {
        Init();
    }
    ~OHOSApp()
    {
        Terminate();
    }

    void CreateNativeWindow();
    void Init()
    {
        CreateNativeWindow();
    }
    void Run() {}
    void Terminate();
    OHNativeWindow* GetWindowHandle() const
    {
        return m_nativeWindow;
    }

private:
    // Window
    OHNativeWindow* m_nativeWindow;
    OH_NativeImage* m_nativeImage;
};

// Native window
inline OHOSApp* g_ohosApp;

} // namespace Test

RENDER_BEGIN_NAMESPACE()

#if defined(RENDER_STATIC_LIB)
CORE_NS::PluginToken RegisterInterfaces(CORE_NS::IPluginRegister&);
#endif

namespace UTest {

constexpr const char* TEST_NATIVE_WINDOW_NAME = "TestWindow";

struct EngineResources {
    CORE_NS::IEngine::Ptr engine;
    IRenderContext::Ptr context;
    IDevice* device;
#if RENDER_HAS_VULKAN_BACKEND
    DeviceBackendType backend { DeviceBackendType::VULKAN };
#elif RENDER_HAS_GLES_BACKEND
    DeviceBackendType backend { DeviceBackendType::OPENGLES };
#else // RENDER_HAS_GL_BACKEND
    DeviceBackendType backend { DeviceBackendType::OPENGL };
#endif
    bool enableMultiQueue { false };
    bool createWindow { false };
    int width { 128 };
    int height { 128 };
};

struct TestEnvironment {
    struct CORE_NS::PlatformCreateInfo platformCreateInfo;
    BASE_NS::string appAssetPath;
    BASE_NS::string cpuVendor;
    EngineResources er;
};

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
DeviceBackendType GetOpenGLBackend();
#endif
TestEnvironment* GetTestEnv();
void RegisterPaths(CORE_NS::IEngine& engine);
void RegisterTestFileProtocol(CORE_NS::IFileManager& fileManager,
    const RENDER_NS::DeviceBackendType& backend = RENDER_NS::DeviceBackendType::VULKAN);
uint64_t CreateSurface(const EngineResources& er, const BASE_NS::string& name = TEST_NATIVE_WINDOW_NAME);
void DestroySurface(const EngineResources& er, uint64_t surfaceHandle);
void CreateEngineSetup(EngineResources& er);
void DestroyEngine(EngineResources& er);
void CreateNativeWindow(int width, int height, const BASE_NS::string& name);
void DestroyNativeWindow(const BASE_NS::string& name);
void WritePng(
    const BASE_NS::string& fileName, uint32_t x, uint32_t y, uint32_t comp, const void* data, uint32_t strideBytes);
void SaveImage(const BASE_NS::string& fileName, uint32_t x, uint32_t y, const float* floatData);
float FromHDR(float color);
void SaveHdrImage(const BASE_NS::string& fileName, uint32_t x, uint32_t y, const float* floatData);
void PrintDeviceInfo(EngineResources& er);

namespace {

CORE_NS::IFileManager::Ptr CreateFileManager()
{
    auto factory = CORE_NS::GetInstance<CORE_NS::IFileSystemApi>(CORE_NS::UID_FILESYSTEM_API_FACTORY);
    auto fileManager = factory->CreateFilemanager();
    fileManager->RegisterFilesystem("file", factory->CreateStdFileSystem());
    fileManager->RegisterFilesystem("memory", factory->CreateMemFileSystem());
    return fileManager;
}

std::unique_ptr<TestEnvironment> g_testEnv {};

class TestRunnerEnv : public ::testing::Environment {
public:
    void SetUp() override
    {
        // Create native window/surface
        ::Test::g_ohosApp = new ::Test::OHOSApp();
        auto windowHandle = ::Test::g_ohosApp->GetWindowHandle();
        ASSERT_TRUE(windowHandle);

        // OHOS specific paths
        CORE_NS::PlatformCreateInfo info {};
        info.coreRootPath = OHOS_PLATFORM_CORE_PATH;
        info.appPluginPath = OHOS_PLATFORM_PLUGINS_PATH;

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
        // Load engine lib
        m_engineLib.Load("/system/lib64/libAGPDLL.z.so");
        CORE_ASSERT(m_engineLib.IsLoaded());

        // Load functions
        m_engineLib.LoadFunction<decltype(CORE_NS::CreatePluginRegistry)>(
            CORE_NS::CreatePluginRegistry, "_ZN4Core20CreatePluginRegistryERKNS_18PlatformCreateInfoE");
        m_engineLib.LoadFunction<decltype(CORE_NS::GetPluginRegister)>(
            CORE_NS::GetPluginRegister, "_ZN4Core17GetPluginRegisterEv");
#endif

        CORE_NS::CreatePluginRegistry(info);

#if defined(RENDER_STATIC_LIB)
        // Register LumeRender static lib
        RENDER_NS::RegisterInterfaces(CORE_NS::GetPluginRegister());
        // Load shared libs
        constexpr BASE_NS::Uid uids[] = { JPGPlugin::UID_JPG_PLUGIN, PNGPlugin::UID_PNG_PLUGIN };
#else
        // Load shared libs
        constexpr BASE_NS::Uid uids[] = { RENDER_NS::UID_RENDER_PLUGIN, JPGPlugin::UID_JPG_PLUGIN,
            PNGPlugin::UID_PNG_PLUGIN };
#endif

        ASSERT_TRUE(CORE_NS::GetPluginRegister().LoadPlugins(uids));

        // Create a engine file manager that can be used when running tests to load test data.
        g_testEnv = std::make_unique<TestEnvironment>();
        g_testEnv->platformCreateInfo = info;
        g_testEnv->appAssetPath = BASE_NS::string(OHOS_PLATFORM_ASSETS_PATH);
        g_testEnv->cpuVendor = "";
        g_testEnv->er.createWindow = false;
        CreateEngineSetup(g_testEnv->er);
        PrintDeviceInfo(g_testEnv->er);
    }

    void TearDown() override
    {
        DestroyEngine(g_testEnv->er);
        CORE_NS::GetPluginRegister().UnloadPlugins({});
        g_testEnv.reset();
    }

private:
#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
    Test::DynamicLibrary m_engineLib;
#endif
};
} // namespace

} // namespace UTest
RENDER_END_NAMESPACE()

#endif // TEST_ENVIRONMENT
