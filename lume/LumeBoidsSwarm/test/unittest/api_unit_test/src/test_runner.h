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
#ifndef BOIDS_SWARM_TEST_RUNNER
#define BOIDS_SWARM_TEST_RUNNER

#include <boids_swarm/implementation_uids.h>

#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_filesystem_api.h>
#include <core/os/intf_platform.h>
#include <core/os/platform_create_info.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>

#include "test_framework.h"

#if defined(__ANDROID__)
#include <test_runner_android.h>
#elif defined(__OHOS__)
#include <test_runner_ohos.h>
#elif defined(_WIN32)
#include <test_runner_windows.h>
#undef CreateDirectory
#elif defined(__linux__)
#include <test_runner_linux.h>
#endif

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
#include <core/plugin/intf_plugin_register.h>

#include "load_lib.h"
#endif

// Just making sure that RTTI is disabled. Remove this if we decide to enable RTTI at some point.
#if defined(_CPPRTTI) || defined(__GXX_RTTI)
// #error RTTI not disabled
#endif

namespace BoidSwarm {
namespace UTest {

struct TestEnvironment {
    struct CORE_NS::PlatformCreateInfo platformCreateInfo;
    BASE_NS::string appAssetPath;
    RENDER_NS::IDevice* device;
};

struct TestContext {
    CORE_NS::IEcs::Ptr ecs;
    CORE_NS::IEngine::Ptr engine;
    RENDER_NS::IRenderContext::Ptr renderContext;
    CORE3D_NS::IGraphicsContext::Ptr graphicsContext;
    ~TestContext();
};

TestEnvironment* GetTestEnv();
void RegisterPaths(CORE_NS::IEngine& engine);
void RegisterTestFileProtocol(CORE_NS::IFileManager& fileManager);
CORE_NS::IEngine::Ptr CreateEngine();
RENDER_NS::IRenderContext::Ptr CreateContext(CORE_NS::IEngine& engine, RENDER_NS::DeviceBackendType backend);
CORE3D_NS::IGraphicsContext::Ptr CreateContext(RENDER_NS::IRenderContext& renderContext);
CORE_NS::IEcs::Ptr CreateAndInitializeDefaultEcs(CORE_NS::IEngine& engine);
RENDER_NS::IDevice* GetDevice();
TestContext* GetTestContext();
TestContext* RecreateTestContext(RENDER_NS::DeviceBackendType backend);

namespace {
CORE_NS::IFileManager::Ptr CreateFileManager()
{
    auto factory = CORE_NS::GetInstance<CORE_NS::IFileSystemApi>(CORE_NS::UID_FILESYSTEM_API_FACTORY);
    auto fileManager = factory->CreateFilemanager();
    fileManager->RegisterFilesystem("file", factory->CreateStdFileSystem());
    fileManager->RegisterFilesystem("memory", factory->CreateMemFileSystem());
    return fileManager;
}

std::unique_ptr<TestEnvironment> g_testEnv{};
std::unique_ptr<TestContext> g_testContext{};

class TestRunnerEnv : public ::testing::Environment {
public:
    TestRunnerEnv(int argc, char** argv)
    {}

    void SetUp() override
    {
#if defined(__ANDROID__)
        const CORE_NS::PlatformCreateInfo info{::Test::g_androidApp->GetEnv(), *(::Test::g_androidApp->GetContext())};
        const BASE_NS::string appAssetPath = "";
        const BASE_NS::string cpuVendor = "";
#elif defined(__OHOS__)
        const CORE_NS::PlatformCreateInfo info{::Test::g_ohosApp->GetEnv(),
            ::Test::g_ohosApp->GetContext(),
            nullptr,
            ::Test::g_ohosApp->GetRsManager(),
            ::Test::g_ohosApp->GetNativeLibDir()};
        const BASE_NS::string appAssetPath = "";
        const BASE_NS::string cpuVendor = "";
#elif defined(_WIN32)
        const CORE_NS::PlatformCreateInfo info{::Test::g_windowsApp->GetEngineDir(),
            ::Test::g_windowsApp->GetAppDir(),
            ::Test::g_windowsApp->GetPluginsDir()};

        const BASE_NS::string appAssetPath = ::Test::g_windowsApp->GetAssetDir();
        const BASE_NS::string cpuVendor = ::Test::g_windowsApp->GetAssetDir();
#elif defined(__linux__)
        const CORE_NS::PlatformCreateInfo info{
            ::Test::g_linuxApp->GetEngineDir(), ::Test::g_linuxApp->GetAppDir(), ::Test::g_linuxApp->GetPluginsDir()};

        const BASE_NS::string appAssetPath = ::Test::g_linuxApp->GetAssetDir();
        const BASE_NS::string cpuVendor = ::Test::g_linuxApp->GetAssetDir();
#endif

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
        // Load engine lib
#if defined(_WIN32)
        m_engineLib.Load("AGPEngineDLL");
#elif defined(__ANDROID__) || defined(__OHOS__)
        m_engineLib.Load("libAGPEngineDLL");
#else  // Linux
        m_engineLib.Load("./libAGPEngineDLL");
#endif

        ASSERT_TRUE(m_engineLib.IsLoaded());
        // Load functions
#if defined(_WIN32)
        ASSERT_TRUE(m_engineLib.LoadFunction<decltype(CORE_NS::CreatePluginRegistry)>(
            CORE_NS::CreatePluginRegistry, "?CreatePluginRegistry@Core@@YAXAEBUPlatformCreateInfo@1@@Z"));
        ASSERT_TRUE(m_engineLib.LoadFunction<decltype(CORE_NS::GetPluginRegister)>(
            CORE_NS::GetPluginRegister, "?GetPluginRegister@Core@@YAAEAVIPluginRegister@1@XZ"));
#else
        m_engineLib.LoadFunction<decltype(CORE_NS::CreatePluginRegistry)>(
            CORE_NS::CreatePluginRegistry, "_ZN4Core20CreatePluginRegistryERKNS_18PlatformCreateInfoE");
        m_engineLib.LoadFunction<decltype(CORE_NS::GetPluginRegister)>(
            CORE_NS::GetPluginRegister, "_ZN4Core17GetPluginRegisterEv");
#endif
#endif  // defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)

        CORE_NS::CreatePluginRegistry(info);

        constexpr BASE_NS::Uid uids[]{
            RENDER_NS::UID_RENDER_PLUGIN, CORE3D_NS::UID_3D_PLUGIN, BOIDSSWARM_NS::UID_BOIDS_SWARM_PLUGIN};
        ASSERT_TRUE(CORE_NS::GetPluginRegister().LoadPlugins(uids));

        g_testEnv = std::make_unique<TestEnvironment>();
        g_testEnv->platformCreateInfo = info;
        g_testEnv->appAssetPath = appAssetPath;

        g_testContext = std::make_unique<TestContext>();
        g_testContext->engine = CreateEngine();
        g_testContext->renderContext = CreateContext(*g_testContext->engine, RENDER_NS::DeviceBackendType::VULKAN);
        g_testContext->graphicsContext = CreateContext(*g_testContext->renderContext);
        g_testContext->ecs = CreateAndInitializeDefaultEcs(*g_testContext->engine);
    }

    void TearDown() override
    {
        g_testContext.reset();
        g_testEnv.reset();
    }

private:
#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
    Test::DynamicLibrary m_engineLib;
#endif
};
}  // namespace

}  // namespace UTest
}  // namespace BoidSwarm

#endif  // BOIDS_SWARM_TEST_RUNNER
