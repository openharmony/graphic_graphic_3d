/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
CORE_BEGIN_NAMESPACE()
IPluginRegister& (*GetPluginRegister)(){nullptr};
void (*CreatePluginRegistry)(const struct PlatformCreateInfo& platformCreateInfo){nullptr};
CORE_END_NAMESPACE()
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

void RegisterTestFileProtocol(IFileManager& fileManager)
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

IEngine::Ptr CreateEngine()
{
    // Show only errors because we don't want to clutter the test output.
    const ::Test::LogLevelScope logLevel = ::Test::LogLevelScope(GetLogger(), ILogger::LogLevel::LOG_ERROR);

    const EngineCreateInfo engineCreateInfo{
        GetTestEnv()->platformCreateInfo,
        {
            "test",  // name
            0,       // versionMajor
            1,       // versionMinor
            0,       // versionPatch
        },           // applicationVersion
        {}           // applicationContext
    };
    auto factory = IEngineFactory::Ptr{GetInstance<IEngineFactory>(UID_ENGINE_FACTORY)};
    auto engine = factory->Create(engineCreateInfo);

    RegisterPaths(*engine);
    engine->Init();
    RegisterTestFileProtocol(engine->GetFileManager());

    return engine;
}

}  // namespace UTest
CORE_END_NAMESPACE()

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
    ::testing::AddGlobalTestEnvironment(new ::CORE_NS::UTest::TestRunnerEnv(argc, argv));
    int result = RUN_ALL_TESTS();
    return result;
}

}  // namespace Test
