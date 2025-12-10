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

#include <core/engine_info.h>
#include <core/log.h>

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
CORE_NS::IPluginRegister& (*CORE_NS::GetPluginRegister)() { nullptr };
void (*CORE_NS::CreatePluginRegistry)(const struct CORE_NS::PlatformCreateInfo& platformCreateInfo) { nullptr };
#endif

CORE_BEGIN_NAMESPACE()
namespace UTest {

TestEnvironment* GetTestEnv()
{
    CORE_ASSERT(g_testEnv.get());
    return g_testEnv.get();
}

void RegisterPaths(IEngine& engine)
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

void RegisterTestFileProtocol(IFileManager& fileManager)
{
    // OHOS specific paths
    const BASE_NS::string applicationTestAssetsDirectory = "file://" + GetTestEnv()->appAssetPath;

    // Create test:// protocol that points to application test data directory.
    fileManager.RegisterPath("test", applicationTestAssetsDirectory, true);
}

IEngine::Ptr CreateEngine()
{
    // Show only errors because we don't want to clutter the test output.
    const ::Test::LogLevelScope logLevel = ::Test::LogLevelScope(GetLogger(), ILogger::LogLevel::LOG_ERROR);

    const EngineCreateInfo engineCreateInfo {
        GetTestEnv()->platformCreateInfo,
        {
            "test", // name
            0,      // versionMajor
            1,      // versionMinor
            0,      // versionPatch
        },          // applicationVersion
        {}          // applicationContext
    };
    auto factory = IEngineFactory::Ptr { GetInstance<IEngineFactory>(UID_ENGINE_FACTORY) };
    auto engine = factory->Create(engineCreateInfo);

    RegisterPaths(*engine);
    engine->Init();
    RegisterTestFileProtocol(engine->GetFileManager());

    return engine;
}

} // namespace UTest
CORE_END_NAMESPACE()

testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new CORE_NS::UTest::TestRunnerEnv);
