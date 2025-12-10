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

#include <base/util/uid.h>
#include <core/intf_engine.h>
#include <core/io/intf_filesystem_api.h>
#include <core/log.h>
#include <core/os/intf_platform.h>
#include <core/os/platform_create_info.h>
#include <core/plugin/intf_plugin.h>

#include <meta/base/namespace.h>
#include <meta/base/plugin.h>

#include "helpers/testing_objects.h"
#include "test_framework.h"
#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
#include "load_lib.h"
#endif

// Just making sure that RTTI is disabled. Remove this if we decide to enable RTTI at some point.
#if defined(_CPPRTTI) || defined(__GXX_RTTI)
#error RTTI not disabled
#endif

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
} // namespace Test

META_BEGIN_NAMESPACE()
namespace UTest {

struct TestEnvironment {
    CORE_NS::PlatformCreateInfo platformCreateInfo;
    BASE_NS::string appAssetPath;
    CORE_NS::IEngine::Ptr engine;
};

TestEnvironment* GetTestEnv();
void RegisterPaths(CORE_NS::IFileManager& fileManager, CORE_NS::IEngine& engine);
void RegisterTestFileProtocol(CORE_NS::IFileManager& fileManager);
CORE_NS::IEngine::Ptr CreateEngine();

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
        CORE_NS::PlatformCreateInfo info {};
        info.coreRootPath = OHOS_PLATFORM_CORE_PATH;
        info.appPluginPath = OHOS_PLATFORM_PLUGINS_PATH;

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
        //  Load engine lib
        m_engineLib.Load("/system/lib64/libAGPDLL.z.so");
        CORE_ASSERT(m_engineLib.IsLoaded());

        // Load functions
        m_engineLib.LoadFunction<decltype(CORE_NS::CreatePluginRegistry)>(
            CORE_NS::CreatePluginRegistry, "_ZN4Core20CreatePluginRegistryERKNS_18PlatformCreateInfoE");
        m_engineLib.LoadFunction<decltype(CORE_NS::GetPluginRegister)>(
            CORE_NS::GetPluginRegister, "_ZN4Core17GetPluginRegisterEv");
#endif

        CORE_NS::CreatePluginRegistry(info);

        constexpr BASE_NS::Uid uids[] { META_NS::META_OBJECT_PLUGIN_UID };
        ASSERT_TRUE(CORE_NS::GetPluginRegister().LoadPlugins(uids));

        g_testEnv = std::make_unique<TestEnvironment>();
        g_testEnv->platformCreateInfo = info;
        g_testEnv->appAssetPath = BASE_NS::string(OHOS_PLATFORM_ASSETS_PATH);
        g_testEnv->engine = CreateEngine();

        RegisterTestTypes();
    }

    void TearDown() override
    {
        UnregisterTestTypes();
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
META_END_NAMESPACE()

#endif // META_TEST_RUNNER
