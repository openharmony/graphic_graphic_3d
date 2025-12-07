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

#include <3d/ecs/components/physical_camera_component.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/engine_info.h>
#include <core/plugin/intf_plugin_register.h>

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

CORE3D_BEGIN_NAMESPACE()
namespace UTest {

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
RENDER_NS::DeviceBackendType GetOpenGLBackend()
{
#if RENDER_HAS_GL_BACKEND
    return RENDER_NS::DeviceBackendType::OPENGL;
#elif RENDER_HAS_GLES_BACKEND
    return RENDER_NS::DeviceBackendType::OPENGLES;
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

void RegisterTestFileProtocol(CORE_NS::IFileManager& fileManager)
{
    // OHOS specific paths
    const BASE_NS::string applicationTestAssetsDirectory = "file://" + GetTestEnv()->appAssetPath;

    // Create test:// protocol that points to application test data directory.
    fileManager.RegisterPath("test", applicationTestAssetsDirectory, true);
}

CORE_NS::IEngine::Ptr CreateEngine()
{
    const ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_ERROR);

    const CORE_NS::EngineCreateInfo engineCreateInfo { GetTestEnv()->platformCreateInfo,
        // applicationVersion
        {
            "test", // name
            0,      // versionMajor
            1,      // versionMinor
            0,      // versionPatch
        },
        // applicationContext
        {} };

    auto factory = CORE_NS::GetInstance<CORE_NS::IEngineFactory>(CORE_NS::UID_ENGINE_FACTORY);
    auto engine = factory->Create(engineCreateInfo);

    RegisterPaths(*engine);
    RegisterTestFileProtocol(engine->GetFileManager());

    engine->Init();

    return engine;
}

RENDER_NS::IRenderContext::Ptr CreateContext(CORE_NS::IEngine& engine, const RENDER_NS::DeviceBackendType backend)
{
    auto renderContext = static_cast<RENDER_NS::IRenderContext::Ptr>(
        engine.GetInterface<CORE_NS::IClassFactory>()->CreateInstance(RENDER_NS::UID_RENDER_CONTEXT));

    RENDER_NS::DeviceCreateInfo deviceCreateInfo {
        backend, // backendType
    };

    const RENDER_NS::RenderCreateInfo info {
        {
            "3d_test", // name
            1,         // versionMajor
            0,         // versionMinor
            0,         // versionPatch
        },
        deviceCreateInfo,
    };
    renderContext->Init(info);
    return renderContext;
}

IGraphicsContext::Ptr CreateContext(RENDER_NS::IRenderContext& renderContext)
{
    IGraphicsContext::CreateInfo gfxInf;
    gfxInf.colorSpaceFlags = 0;

    if (!g_testConfig.expired()) {
        auto config = g_testConfig.lock();
        // No support for GraphicsContext::CreateInfo::CreateInfoFlagBits::ENABLE_BINDLESS_PIPELINES_BIT
        gfxInf.createFlags = 0;
    }

    auto context = CORE_NS::CreateInstance<IGraphicsContext>(
        *renderContext.GetInterface<CORE_NS::IClassFactory>(), UID_GRAPHICS_CONTEXT);
    context->Init(gfxInf);

    return context;
}

CORE_NS::IEcs::Ptr CreateAndInitializeDefaultEcs(CORE_NS::IEngine& engine)
{
    CORE_NS::IEcs::Ptr ecs = engine.CreateEcs();
    auto factory = CORE_NS::GetInstance<CORE_NS::ISystemGraphLoaderFactory>(CORE_NS::UID_SYSTEM_GRAPH_LOADER);
    auto systemGraphLoader = factory->Create(engine.GetFileManager());
    const auto result = systemGraphLoader->Load("test://system_graph.json", *ecs);
    EXPECT_TRUE(result.success);
    for (const auto* info : CORE_NS::GetPluginRegister().GetTypeInfos(CORE_NS::ComponentManagerTypeInfo::UID)) {
        auto componentInfo = static_cast<const CORE_NS::ComponentManagerTypeInfo*>(info);
        if (componentInfo->uid == IPhysicalCameraComponentManager::UID) {
            EXPECT_TRUE(ecs->CreateComponentManager(*componentInfo));
            break;
        }
    }
    ecs->Initialize();
    return ecs;
}

RENDER_NS::IDevice* GetDevice()
{
    if (!GetTestEnv()->device) {
        const ::Test::LogLevelScope logLevel =
            ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_ERROR);

        // const DeviceCreateInfo deviceCreateInfo{ DeviceBackendType::VULKAN };

        // getTestEnv()->device = CreateDevice(deviceCreateInfo);
        CORE_ASSERT(GetTestEnv()->device != nullptr);
    }

    return GetTestEnv()->device;
}

TestContext* GetTestContext()
{
    g_testContext->ecs->ProcessEvents();
    g_testContext->ecs->Uninitialize();

    g_testContext->ecs->ProcessEvents();
    g_testContext->ecs->Initialize();
    return g_testContext.get();
}

std::shared_ptr<TestConfiguration> CreateTestConfig()
{
    auto config = std::make_shared<TestConfiguration>();
    g_testConfig = config;
    return config;
}

TestContext* RecreateTestContext(const RENDER_NS::DeviceBackendType backend)
{
    g_testContext.reset();
    g_testContext = std::make_unique<TestContext>();
    g_testContext->engine = CreateEngine();
    g_testContext->renderContext = CreateContext(*g_testContext->engine, backend);
    g_testContext->graphicsContext = CreateContext(*g_testContext->renderContext);
    g_testContext->ecs = CreateAndInitializeDefaultEcs(*g_testContext->engine);
    return g_testContext.get();
}

TestContext::~TestContext()
{
    ecs.reset();
    graphicsContext.reset();
    renderContext.reset();
    engine.reset();
}

void WritePng(
    const BASE_NS::string& fileName, uint32_t x, uint32_t y, uint32_t comp, const void* data, uint32_t strideBytes)
{
#if defined(USE_STB_IMAGE) && (USE_STB_IMAGE == 1)
    stbi_write_png(fileName.c_str(), static_cast<int32_t>(x), static_cast<int32_t>(y), static_cast<int32_t>(comp), data,
        static_cast<int32_t>(strideBytes));
#endif
}

} // namespace UTest
CORE3D_END_NAMESPACE()

testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new CORE3D_NS::UTest::TestRunnerEnv);
