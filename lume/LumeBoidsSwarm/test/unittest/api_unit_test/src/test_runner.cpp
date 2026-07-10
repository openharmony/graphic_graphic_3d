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

#include "test_runner.h"

#include <core/engine_info.h>

#if RENDER_HAS_GLES_BACKEND || RENDER_HAS_GL_BACKEND
#include <render/gles/intf_device_gles.h>
#endif
#if RENDER_HAS_VULKAN_BACKEND
#include <vulkan/vulkan.h>

#include <render/vulkan/intf_device_vk.h>
#endif

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
CORE_BEGIN_NAMESPACE()
IPluginRegister& (*GetPluginRegister)(){nullptr};
void (*CreatePluginRegistry)(const struct PlatformCreateInfo& platformCreateInfo){nullptr};
CORE_END_NAMESPACE()
#endif

namespace BoidSwarm {
namespace UTest {

TestEnvironment* GetTestEnv()
{
    CORE_ASSERT(g_testEnv.get());
    return g_testEnv.get();
}

void RegisterPaths(CORE_NS::IEngine& engine)
{
    // Register filesystems/paths used in the tests.
    CORE_NS::IFileManager& fileManager = engine.GetFileManager();
#if defined(__ANDROID__)
    // Android specific paths.
    auto& androidPlatform = static_cast<const CORE_NS::IPlatformAndroid&>(engine.GetPlatform());
    const BASE_NS::string applicationDirectory = androidPlatform.GetAndroidFilesDir() + '/';
    const BASE_NS::string cacheDirectory = androidPlatform.GetAndroidCacheDir() + '/';

    // register apk protocol (to access files inside apk)
    androidPlatform.RegisterApkFilesystem(fileManager, "apk");

#elif defined(__OHOS__)
    // OHOS specific paths.
    const BASE_NS::string applicationDirectory = "file://" + ::Test::g_ohosApp->GetFilesDir() + '/';
    const BASE_NS::string cacheDirectory = "file://" + ::Test::g_ohosApp->GetCacheDir() + '/';

    // register hap protocol (to access files inside hap)
    engine.GetPlatform().RegisterRawFilesystem(fileManager, "hap");

#else
    const BASE_NS::string applicationDirectory = "file://" + GetTestEnv()->platformCreateInfo.appRootPath;
    const BASE_NS::string cacheDirectory = applicationDirectory + "cache/";
#endif

    // Make sure that the cache folder exists.
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
#if defined(__ANDROID__)
    constexpr BASE_NS::string_view applicationTestAssetsDirectory = "apk://test_data/";

#elif defined(__OHOS__)
    constexpr BASE_NS::string_view applicationTestAssetsDirectory = "hap://test_data/";

#else
    const BASE_NS::string applicationTestAssetsDirectory = "file://" + GetTestEnv()->appAssetPath;
#endif

    // Create test:// protocol that points to application test data directory.
    fileManager.RegisterPath("test", applicationTestAssetsDirectory, true);
}

CORE_NS::IEngine::Ptr CreateEngine()
{
    const ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_ERROR);

    const CORE_NS::EngineCreateInfo engineCreateInfo{GetTestEnv()->platformCreateInfo,
        // applicationVersion
        {
            "boids_swarm_test",  // name
            0,                   // versionMajor
            1,                   // versionMinor
            0,                   // versionPatch
        },
        // applicationContext
        {}};

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

    RENDER_NS::DeviceCreateInfo deviceCreateInfo;

#if RENDER_HAS_VULKAN_BACKEND
    RENDER_NS::BackendExtraVk vkExtra;
    deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::VULKAN;
    deviceCreateInfo.backendConfiguration = &vkExtra;
#elif RENDER_HAS_GLES_BACKEND
    RENDER_NS::BackendExtraGLES glesExtra;
    glesExtra.applicationContext = EGL_NO_CONTEXT;
    glesExtra.sharedContext = EGL_NO_CONTEXT;
    glesExtra.MSAASamples = 0;
    glesExtra.depthBits = 24;  // 24 bits of depth buffer.
    deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::OPENGLES;
    deviceCreateInfo.backendConfiguration = &glesExtra;
#elif RENDER_HAS_GL_BACKEND
    RENDER_NS::BackendExtraGL glExtra;
    glExtra.MSAASamples = 0;
    glExtra.depthBits = 24;
    glExtra.alphaBits = 8;
    glExtra.stencilBits = 0;
    deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::OPENGL;
    deviceCreateInfo.backendConfiguration = &glExtra;
#endif

    const RENDER_NS::RenderCreateInfo info{
        {
            "boids_swarm_test",  // name
            1,                   // versionMajor
            0,                   // versionMinor
            0,                   // versionPatch
        },
        deviceCreateInfo,
    };
    renderContext->Init(info);
    return renderContext;
}

CORE3D_NS::IGraphicsContext::Ptr CreateContext(RENDER_NS::IRenderContext& renderContext)
{
    CORE3D_NS::IGraphicsContext::CreateInfo gfxInf;
    gfxInf.colorSpaceFlags = 0;

    auto context = CORE_NS::CreateInstance<CORE3D_NS::IGraphicsContext>(
        *renderContext.GetInterface<CORE_NS::IClassFactory>(), CORE3D_NS::UID_GRAPHICS_CONTEXT);
    context->Init(gfxInf);

    return context;
}

CORE_NS::IEcs::Ptr CreateAndInitializeDefaultEcs(CORE_NS::IEngine& engine)
{
    CORE_NS::IEcs::Ptr ecs = engine.CreateEcs();
    ecs->Initialize();
    return ecs;
}

RENDER_NS::IDevice* GetDevice()
{
    if (!GetTestEnv()->device) {
        const ::Test::LogLevelScope logLevel =
            ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_ERROR);
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

}  // namespace UTest
}  // namespace BoidSwarm

namespace Test {

// Runs googletest with given parameters.
// NOTE: that this can be only called once in a process as googletest cannot be initialized
// multiple times with different arguments.
//
// To write an xml/json output file use:
//  ::testing::GTEST_FLAG(output) = "xml:hello.xml";
int RunGoogleTest(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new ::BoidSwarm::UTest::TestRunnerEnv(argc, argv));
    int result = RUN_ALL_TESTS();
    return result;
}

}  // namespace Test
