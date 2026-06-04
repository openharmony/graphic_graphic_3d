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

#include <algorithm>
#include <dynamic_uids.h>
#include <dynamic_with_dependency3_uids.h>
#include <dynamic_with_dependency_uids.h>
#include <static_uids.h>

#include <base/containers/string_view.h>
#include <base/util/uid_util.h>
#include <core/log.h>
#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>

#include "test_framework.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
#include "plugin_registry.h"

namespace UTest {
constexpr BASE_NS::string_view STATIC_PLUGIN_NAME = "Static Test Plugin";
constexpr BASE_NS::string_view SHARED_PLUGIN_NAME = "Shared Test Plugin";

constexpr BASE_NS::Uid UNKNOWN_UID{"c0ffee00-600d-1234-1234-deadbeef0bad"};

class TypeInfoListener final : public CORE_NS::IPluginRegister::ITypeInfoListener {
public:
    TypeInfoListener(CORE_NS::IPluginRegister& pluginRegister) : pluginRegister_(pluginRegister)
    {
        pluginRegister.AddListener(*this);
    }
    ~TypeInfoListener() override
    {
        pluginRegister_.RemoveListener(*this);
    }

    void OnTypeInfoEvent(EventType type, BASE_NS::array_view<const CORE_NS::ITypeInfo* const> typeInfos) override
    {
        if (type_ != type) {
            typeInfos_.clear();
        }
        type_ = type;
        std::transform(typeInfos.begin(),
            typeInfos.end(),
            std::back_inserter(typeInfos_),
            [](const CORE_NS::ITypeInfo* const typeInfo) {
                CORE_LOG_D("%s", BASE_NS::to_string(typeInfo->typeUid).data());
                return typeInfo->typeUid;
            });
    }
    CORE_NS::IPluginRegister& pluginRegister_;
    EventType type_;
    BASE_NS::vector<BASE_NS::Uid> typeInfos_;
};
}  // namespace UTest

extern "C" const uint64_t SIZEOFDATAFORTEST;
extern "C" const void* const BINARYDATAFORTEST[];

namespace {
constexpr BASE_NS::string_view INVALID_PLUGIN_PROTOCOL = "invalidrofs";

#if defined(UNIT_TESTS_USE_HCPPTEST)
BASE_NS::string GetInvalidPluginRofsSourceDir(CORE_NS::FileManager& fileManager)
{
    if (!fileManager.GetFilesystem(INVALID_PLUGIN_PROTOCOL)) {
        auto rofs = fileManager.CreateROFilesystem(BINARYDATAFORTEST, static_cast<size_t>(SIZEOFDATAFORTEST));
        if (!rofs || !fileManager.RegisterFilesystem(INVALID_PLUGIN_PROTOCOL, BASE_NS::move(rofs))) {
            return {};
        }
    }
    return BASE_NS::string(INVALID_PLUGIN_PROTOCOL) + ":///invalid_plugins/";
}
#endif

BASE_NS::string GetInvalidPluginSourceDir(CORE_NS::FileManager& fileManager)
{
#if defined(UNIT_TESTS_USE_HCPPTEST)
    return GetInvalidPluginRofsSourceDir(fileManager);
#elif defined(__ANDROID__)
#if defined(__aarch64__)
    return "test://invalid_plugins/arm64-v8a/";
#elif defined(__arm__)
    return "test://invalid_plugins/armeabi-v7a/";
#elif defined(__i386__)
    return "test://invalid_plugins/x86/";
#else
    return "test://invalid_plugins/x86_64/";
#endif
#else
    return "test://invalid_plugins/";
#endif
}

bool CopyPluginFile(CORE_NS::IFileManager& fileManager, BASE_NS::string_view sourceUri, BASE_NS::string_view targetUri)
{
    auto sourceFile = fileManager.OpenFile(sourceUri);
    auto targetFile = fileManager.CreateFile(targetUri);
    if (!sourceFile || !targetFile) {
        return false;
    }

    const auto length = sourceFile->GetLength();
    BASE_NS::vector<uint8_t> buffer(static_cast<size_t>(length));
    return sourceFile->Read(buffer.data(), length) == length && targetFile->Write(buffer.data(), length) == length;
}

void ClearDirectory(CORE_NS::IFileManager& fileManager, BASE_NS::string_view directory)
{
    if (auto entries = fileManager.OpenDirectory(directory)) {
        for (const auto& entry : entries->GetEntries()) {
            if (entry.type == CORE_NS::IDirectory::Entry::FILE) {
                fileManager.DeleteFile(BASE_NS::string(directory) + entry.name);
            }
        }
    }
    fileManager.DeleteDirectory(directory);
}

BASE_NS::string StageInvalidPlugins(CORE_NS::FileManager& fileManager)
{
    constexpr BASE_NS::string_view targetDir = "cache://invalid_plugins/";
    const BASE_NS::string sourceDir = GetInvalidPluginSourceDir(fileManager);
    auto sourceFiles = fileManager.OpenDirectory(sourceDir);
    if (!sourceFiles) {
        return {};
    }
    ClearDirectory(fileManager, targetDir);
    if (!fileManager.DirectoryExists(targetDir) && !fileManager.CreateDirectory(targetDir)) {
        return {};
    }

    const auto targetPaths = fileManager.GetAbsolutePaths(targetDir);
    if (targetPaths.empty()) {
        return {};
    }

    const BASE_NS::string targetUri = BASE_NS::string("file://") + targetPaths.front();
    size_t sourceCount = 0;
    size_t copied = 0;
    for (const auto& entry : sourceFiles->GetEntries()) {
        if (entry.type == CORE_NS::IDirectory::Entry::FILE) {
            ++sourceCount;
            copied += CopyPluginFile(fileManager, sourceDir + entry.name, targetUri + entry.name) ? 1U : 0U;
        }
    }
    return (sourceCount != 0 && sourceCount == copied) ? targetUri : BASE_NS::string{};
}
}  // namespace

/**
 * @tc.name: GetPlugins
 * @tc.desc: Tests for Get Plugins. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_PluginTest, GetPlugins, testing::ext::TestSize.Level1)
{
    CORE_NS::IPluginRegister& pluginRegister = CORE_NS::GetPluginRegister();
    pluginRegister.LoadPlugins({});

    const auto& pluginRegConP = &pluginRegister;
    EXPECT_TRUE(pluginRegConP != nullptr);
    const auto& fmPointer = &(pluginRegConP->GetFileManager());
    EXPECT_TRUE(fmPointer != nullptr);
    CORE_NS::IPluginRegister* regPointer = &pluginRegister;
    CORE_NS::PluginRegistry* pRegP = static_cast<CORE_NS::PluginRegistry*>(regPointer);
    pRegP->Ref();
    pRegP->Unref();
    const auto& regCons = *pRegP;
    EXPECT_TRUE(regCons.GetInterface(CORE_NS::IInterface::UID) != nullptr);
    EXPECT_FALSE(regCons.GetInterface(UTest::UNKNOWN_UID) != nullptr);

    const BASE_NS::array_view<const CORE_NS::IPlugin* const> plugins = pluginRegister.GetPlugins();
    EXPECT_GE(plugins.size(), 2);
    bool foundStaticTestPlugin = false;
    bool foundSharedTestPlugin = false;
    for (auto&& plugin : plugins) {
        EXPECT_TRUE(plugin);
        if (!plugin) {
            continue;
        }
        if (plugin->name == UTest::STATIC_PLUGIN_NAME) {
            foundStaticTestPlugin = true;
        } else if (plugin->name == UTest::SHARED_PLUGIN_NAME) {
            foundSharedTestPlugin = true;
        }
    }
    EXPECT_TRUE(foundStaticTestPlugin);
    EXPECT_TRUE(foundSharedTestPlugin);

    pluginRegister.UnloadPlugins({});
}

/**
 * @tc.name: GetTypeInfos
 * @tc.desc: Tests for Get Type Infos. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_PluginTest, GetTypeInfos, testing::ext::TestSize.Level1)
{
    CORE_NS::IPluginRegister& pluginRegister = CORE_NS::GetPluginRegister();
    pluginRegister.LoadPlugins({});

    BASE_NS::array_view<const CORE_NS::ITypeInfo* const> typeInfos =
        pluginRegister.GetTypeInfos(CORE_NS::IEnginePlugin::UID);
    EXPECT_GE(typeInfos.size(), 2);
    for (auto&& typeInfo : typeInfos) {
        EXPECT_TRUE(typeInfo);
        EXPECT_TRUE(typeInfo->typeUid == CORE_NS::IEnginePlugin::UID);
        auto enginePlugin = static_cast<const CORE_NS::IEnginePlugin* const>(typeInfo);
    }

    pluginRegister.UnloadPlugins({});
}

/**
 * @tc.name: LoadPlugins
 * @tc.desc: Tests for Load Plugins. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_PluginTest, LoadPlugins, testing::ext::TestSize.Level1)
{
    CORE_NS::IPluginRegister& pluginRegister = CORE_NS::GetPluginRegister();

    {
        // Load one plugin
        constexpr BASE_NS::Uid uids[] = {UTest::UID_STATIC_PLUGIN};
        {
            const bool loaded = pluginRegister.LoadPlugins(uids);
            EXPECT_TRUE(loaded);
            const BASE_NS::array_view<const CORE_NS::IPlugin* const> plugins = pluginRegister.GetPlugins();
            EXPECT_EQ(plugins.size(), 1);
        }

        // Load a plugin again (should be OK)
        {
            const bool loadedAgain = pluginRegister.LoadPlugins(uids);
            EXPECT_TRUE(loadedAgain);
            const BASE_NS::array_view<const CORE_NS::IPlugin* const> plugins = pluginRegister.GetPlugins();
            EXPECT_EQ(plugins.size(), 1);
        }

        // Load one more plugin
        constexpr BASE_NS::Uid uids2[] = {UTest::UID_SHARED_PLUGIN};
        {
            const bool loaded = pluginRegister.LoadPlugins(uids2);
            EXPECT_TRUE(loaded);
            const BASE_NS::array_view<const CORE_NS::IPlugin* const> plugins = pluginRegister.GetPlugins();
            EXPECT_EQ(plugins.size(), 2);
        }
        pluginRegister.UnloadPlugins(uids2);
        pluginRegister.UnloadPlugins(uids);
        pluginRegister.UnloadPlugins(uids);
    }

    // Load multiple plugins in one go
    {
        constexpr BASE_NS::Uid uids[] = {UTest::UID_STATIC_PLUGIN, UTest::UID_SHARED_PLUGIN};
        const bool loaded = pluginRegister.LoadPlugins(uids);
        EXPECT_TRUE(loaded);

        const BASE_NS::array_view<const CORE_NS::IPlugin* const> plugins = pluginRegister.GetPlugins();
        EXPECT_EQ(plugins.size(), 2);

        pluginRegister.UnloadPlugins(uids);
    }
    // Try loading plugin with unknown UID (should fail)
    {
        constexpr BASE_NS::Uid uids[] = {UTest::UNKNOWN_UID};
        const bool loaded = pluginRegister.LoadPlugins(uids);
        EXPECT_FALSE(loaded);

        const BASE_NS::array_view<const CORE_NS::IPlugin* const> plugins = pluginRegister.GetPlugins();
        EXPECT_EQ(plugins.size(), 0);

        pluginRegister.UnloadPlugins({});
    }
    // Try loading multiple plugins with one unknown UID (should fail)
    {
        constexpr BASE_NS::Uid uids[] = {UTest::UID_SHARED_PLUGIN, UTest::UNKNOWN_UID, UTest::UID_STATIC_PLUGIN};
        const bool loaded = pluginRegister.LoadPlugins(uids);
        EXPECT_FALSE(loaded);

        const BASE_NS::array_view<const CORE_NS::IPlugin* const> plugins = pluginRegister.GetPlugins();
        EXPECT_EQ(plugins.size(), 0);

        pluginRegister.UnloadPlugins({});
    }
}

/**
 * @tc.name: LoadPluginsWithExplicitDependency
 * @tc.desc: Tests loading a plugin and its hard dependency together.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_PluginTest, LoadPluginsWithExplicitDependency, testing::ext::TestSize.Level1)
{
    CORE_NS::IPluginRegister& pluginRegister = CORE_NS::GetPluginRegister();
    constexpr BASE_NS::Uid uids[] = {UTest3::UID_SHARED_PLUGIN, UTest2::UID_SHARED_PLUGIN};

    const bool loaded = pluginRegister.LoadPlugins(uids);
    EXPECT_TRUE(loaded);
    EXPECT_EQ(pluginRegister.GetPlugins().size(), 3);

    pluginRegister.UnloadPlugins(uids);
    EXPECT_EQ(pluginRegister.GetPlugins().size(), 0);
}

#ifndef __OHOS__
/**
 * @tc.name: LoadPluginsRejectCircularDependency
 * @tc.desc: Tests that circular hard plugin dependencies are rejected.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_PluginTest, LoadPluginsRejectCircularDependency, testing::ext::TestSize.Level1)
{
    constexpr BASE_NS::Uid circularPluginUids[] = {
        BASE_NS::Uid{"12345678-1234-1234-1234-deadbeef0050"}, BASE_NS::Uid{"12345678-1234-1234-1234-deadbeef0051"}};

    auto& fileManager = static_cast<CORE_NS::FileManager&>(CORE_NS::UTest::GetTestEnv()->engine->GetFileManager());
    const BASE_NS::string pluginPath = StageInvalidPlugins(fileManager);
    ASSERT_FALSE(pluginPath.empty());

    CORE_NS::PluginRegistry registry;
    registry.RegisterPluginPath(pluginPath);

    EXPECT_FALSE(registry.LoadPlugins({&circularPluginUids[0], 1}));
    EXPECT_EQ(registry.GetPlugins().size(), 0);
    EXPECT_FALSE(registry.LoadPlugins({&circularPluginUids[1], 1}));
    EXPECT_EQ(registry.GetPlugins().size(), 0);

    ClearDirectory(fileManager, "cache://invalid_plugins/");
}

#endif

/**
 * @tc.name: globalInterfaces
 * @tc.desc: Tests for Global Interfaces. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_ClassRegister, globalInterfaces, testing::ext::TestSize.Level1)
{
    CORE_NS::IPluginRegister& pluginRegister = CORE_NS::GetPluginRegister();
    CORE_NS::IClassRegister& globalClassRegister = pluginRegister.GetClassRegister();
    const auto interfaceCount = globalClassRegister.GetInterfaceMetadata().size();
    {
        EXPECT_NE(globalClassRegister.GetInterfaceMetadata(UTest::UID_STATIC_GLOBAL_TEST_IMPL).uid,
            UTest::UID_STATIC_GLOBAL_TEST_IMPL);

        const bool loaded = pluginRegister.LoadPlugins({&UTest::UID_STATIC_PLUGIN, 1});
        EXPECT_TRUE(loaded);
        // Should have added one interface to the global
        EXPECT_EQ(globalClassRegister.GetInterfaceMetadata().size(), interfaceCount + 1);

        const CORE_NS::InterfaceTypeInfo& info =
            globalClassRegister.GetInterfaceMetadata(UTest::UID_STATIC_GLOBAL_TEST_IMPL);
        EXPECT_EQ(info.uid, UTest::UID_STATIC_GLOBAL_TEST_IMPL);
        // Interface registered during IEnginePlugin::createPlugin should not be found from global registry.
        EXPECT_NE(globalClassRegister.GetInterfaceMetadata(UTest::UID_STATIC_ENGINE_TEST_IMPL).uid,
            UTest::UID_STATIC_ENGINE_TEST_IMPL);

        EXPECT_TRUE(info.createInterface);
        UTest::ITest::Ptr staticGlobal = CORE_NS::CreateInstance<UTest::ITest>(UTest::UID_STATIC_GLOBAL_TEST_IMPL);
        EXPECT_TRUE(staticGlobal);
        EXPECT_EQ(staticGlobal->GetType(), UTest::StaticGlobalTest().GetType());

        EXPECT_TRUE(info.getInterface);
        UTest::ITest* staticGlobal2 = CORE_NS::GetInstance<UTest::ITest>(UTest::UID_STATIC_GLOBAL_TEST_IMPL);
        EXPECT_TRUE(staticGlobal2);
        EXPECT_EQ(staticGlobal2->GetType(), UTest::StaticGlobalTest().GetType());
        EXPECT_NE(staticGlobal.get(), staticGlobal2);

        const bool loaded2 = pluginRegister.LoadPlugins({&UTest::UID_SHARED_PLUGIN, 1});
        EXPECT_TRUE(loaded2);
        // Should have added one interface to the global
        EXPECT_EQ(globalClassRegister.GetInterfaceMetadata().size(), interfaceCount + 2);

        const CORE_NS::InterfaceTypeInfo& info2 =
            globalClassRegister.GetInterfaceMetadata(UTest::UID_SHARED_GLOBAL_TEST_IMPL);
        EXPECT_EQ(info2.uid, UTest::UID_SHARED_GLOBAL_TEST_IMPL);

        EXPECT_FALSE(info2.createInterface);
        EXPECT_FALSE(CORE_NS::CreateInstance<UTest::ITest>(UTest::UID_SHARED_GLOBAL_TEST_IMPL));

        EXPECT_TRUE(info2.getInterface);
        UTest::ITest* sharedGlobal = CORE_NS::GetInstance<UTest::ITest>(UTest::UID_SHARED_GLOBAL_TEST_IMPL);
        EXPECT_TRUE(sharedGlobal);
        if (sharedGlobal) {
            EXPECT_EQ(sharedGlobal->GetType(), UTest::SharedGlobalTest().GetType());
            EXPECT_NE(staticGlobal2, sharedGlobal);
        }
        pluginRegister.UnloadPlugins({});
    }
    EXPECT_EQ(globalClassRegister.GetInterfaceMetadata().size(), interfaceCount);
}

/**
 * @tc.name: engineInterfaces
 * @tc.desc: Tests for Engine Interfaces. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_ClassRegister, engineInterfaces, testing::ext::TestSize.Level1)
{
    CORE_NS::IPluginRegister& pluginRegister = CORE_NS::GetPluginRegister();

    UTest::TypeInfoListener listener(pluginRegister);

    CORE_NS::IClassRegister& globalClassRegister = pluginRegister.GetClassRegister();
    const auto globalInterfaceCount = globalClassRegister.GetInterfaceMetadata().size();

    CORE_NS::IEngine::Ptr engine = CORE_NS::UTest::CreateEngine();
    ASSERT_TRUE(engine);
    CORE_NS::IClassRegister* engineRegister = engine->GetInterface<CORE_NS::IClassRegister>();
    ASSERT_TRUE(engineRegister);
    const auto engineInterfaceCount = engineRegister->GetInterfaceMetadata().size();

    // Interfaces shouldn't be available before loading plugins.
    EXPECT_NE(engineRegister->GetInterfaceMetadata(UTest::UID_SHARED_ENGINE_TEST_IMPL).uid,
        UTest::UID_SHARED_ENGINE_TEST_IMPL);
    EXPECT_NE(engineRegister->GetInterfaceMetadata(UTest::UID_STATIC_ENGINE_TEST_IMPL).uid,
        UTest::UID_STATIC_ENGINE_TEST_IMPL);

    constexpr BASE_NS::Uid uids[] = {UTest::UID_SHARED_PLUGIN, UTest::UID_STATIC_PLUGIN};
    const bool loaded = pluginRegister.LoadPlugins(uids);
    EXPECT_TRUE(loaded);
    EXPECT_EQ(listener.type_, CORE_NS::IPluginRegister::ITypeInfoListener::EventType::ADDED);
    EXPECT_EQ(listener.typeInfos_.size(), 2);
    listener.typeInfos_.clear();

    EXPECT_EQ(engineRegister->GetInterfaceMetadata().size(), engineInterfaceCount + 2);

    // Interface registered during IPlugin::registerInterfaces shouldn't be found from engine.
    EXPECT_NE(engineRegister->GetInterfaceMetadata(UTest::UID_SHARED_GLOBAL_TEST_IMPL).uid,
        UTest::UID_SHARED_GLOBAL_TEST_IMPL);
    // Interface registered during IEnginePlugin::createPlugin should be found from engine.
    EXPECT_EQ(engineRegister->GetInterfaceMetadata(UTest::UID_SHARED_ENGINE_TEST_IMPL).uid,
        UTest::UID_SHARED_ENGINE_TEST_IMPL);
    EXPECT_EQ(engineRegister->GetInterfaceMetadata(UTest::UID_STATIC_ENGINE_TEST_IMPL).uid,
        UTest::UID_STATIC_ENGINE_TEST_IMPL);
    CORE_NS::IClassFactory* engineFactory = engine->GetInterface<CORE_NS::IClassFactory>();
    EXPECT_TRUE(engineFactory);
    if (engineFactory) {
        EXPECT_TRUE(CORE_NS::CreateInstance<UTest::ITest>(*engineFactory, UTest::UID_SHARED_ENGINE_TEST_IMPL));
        EXPECT_TRUE(CORE_NS::CreateInstance<UTest::ITest>(*engineFactory, UTest::UID_STATIC_ENGINE_TEST_IMPL));
    }
    CORE_NS::IClassRegister* engReg = engine->GetInterface<CORE_NS::IClassRegister>();
    EXPECT_TRUE(engReg);
    if (engReg) {
        EXPECT_FALSE(CORE_NS::GetInstance<UTest::ITest>(engReg, UTest::UID_SHARED_ENGINE_TEST_IMPL));
    }
    EXPECT_FALSE(CORE_NS::GetInstance<UTest::ITest>(nullptr, UTest::UID_SHARED_ENGINE_TEST_IMPL));

    pluginRegister.UnloadPlugins(uids);
    EXPECT_EQ(listener.type_, CORE_NS::IPluginRegister::ITypeInfoListener::EventType::REMOVED);
    EXPECT_EQ(listener.typeInfos_.size(), 2);
    EXPECT_EQ(engineRegister->GetInterfaceMetadata().size(), engineInterfaceCount);
    EXPECT_EQ(globalClassRegister.GetInterfaceMetadata().size(), globalInterfaceCount);
}
