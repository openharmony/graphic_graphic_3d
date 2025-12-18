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

#include <3d_test_plugin.h>
#include <algorithm>

#include <3d/intf_plugin.h>
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

namespace {
constexpr BASE_NS::string_view TEST_PLUGIN_NAME = "3D Test Plugin";

constexpr BASE_NS::Uid UNKNOWN_UID { "c0ffee00-600d-1234-1234-deadbeef0bad" };

class TypeInfoListener final : public CORE_NS::IPluginRegister::ITypeInfoListener {
public:
    explicit TypeInfoListener(CORE_NS::IPluginRegister& pluginRegister) : pluginRegister_(pluginRegister)
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
        std::transform(typeInfos.begin(), typeInfos.end(), std::back_inserter(typeInfos_),
            [](const CORE_NS::ITypeInfo* const typeInfo) {
                CORE_LOG_D("%s", BASE_NS::to_string(typeInfo->typeUid).data());
                return typeInfo->typeUid;
            });
    }
    CORE_NS::IPluginRegister& pluginRegister_;
    EventType type_;
    BASE_NS::vector<BASE_NS::Uid> typeInfos_;
};
} // namespace

/**
 * @tc.name: GetPlugins
 * @tc.desc: Tests for Get Plugins. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_PluginTest, GetPlugins, testing::ext::TestSize.Level1)
{
    CORE_NS::IPluginRegister& pluginRegister = CORE_NS::GetPluginRegister();

    constexpr BASE_NS::Uid uids[] = { TestPlugin3D::UID_3D_TEST_PLUGIN };
    ASSERT_TRUE(pluginRegister.LoadPlugins(uids));

    const BASE_NS::array_view<const CORE_NS::IPlugin* const> plugins = pluginRegister.GetPlugins();
    EXPECT_GE(plugins.size(), 3U); // expect to find test plugin and it's dependencies 3d and render
    bool foundTestPlugin = false;
    for (auto&& plugin : plugins) {
        ASSERT_TRUE(plugin);
        if (plugin->name == TEST_PLUGIN_NAME) {
            foundTestPlugin = true;
        }
    }
    ASSERT_TRUE(foundTestPlugin);

    pluginRegister.UnloadPlugins(uids);
}

/**
 * @tc.name: GetTypeInfos
 * @tc.desc: Tests for Get Type Infos. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_PluginTest, GetTypeInfos, testing::ext::TestSize.Level1)
{
    CORE_NS::IPluginRegister& pluginRegister = CORE_NS::GetPluginRegister();

    constexpr BASE_NS::Uid uids[] = { TestPlugin3D::UID_3D_TEST_PLUGIN };
    pluginRegister.LoadPlugins(uids);

    BASE_NS::array_view<const CORE_NS::ITypeInfo* const> typeInfos =
        pluginRegister.GetTypeInfos(CORE3D_NS::I3DPlugin::UID);
    EXPECT_GE(typeInfos.size(), 1);
    for (auto&& typeInfo : typeInfos) {
        ASSERT_TRUE(typeInfo);
        ASSERT_TRUE(typeInfo->typeUid == CORE3D_NS::I3DPlugin::UID);
        auto plugin3d = static_cast<const CORE3D_NS::I3DPlugin* const>(typeInfo);
        EXPECT_TRUE(plugin3d->createPlugin);
        EXPECT_TRUE(plugin3d->destroyPlugin);
    }

    pluginRegister.UnloadPlugins({ uids });
}

/**
 * @tc.name: globalInterfaces
 * @tc.desc: Tests for Global Interfaces. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_PluginTest, globalInterfaces, testing::ext::TestSize.Level1)
{
    CORE_NS::IPluginRegister& pluginRegister = CORE_NS::GetPluginRegister();
    CORE_NS::IClassRegister& globalClassRegister = pluginRegister.GetClassRegister();

    const auto interfaceCount = globalClassRegister.GetInterfaceMetadata().size();
    {
        constexpr BASE_NS::Uid uids[] = { TestPlugin3D::UID_3D_TEST_PLUGIN };
        ASSERT_TRUE(pluginRegister.LoadPlugins(uids));

        const CORE_NS::InterfaceTypeInfo& info =
            globalClassRegister.GetInterfaceMetadata(TestPlugin3D::UID_3D_GLOBAL_TEST_IMPL);

        EXPECT_EQ(info.uid, TestPlugin3D::UID_3D_GLOBAL_TEST_IMPL);
        // Interface registered during I3DPlugin::createPlugin should not be found from global registry.
        EXPECT_NE(globalClassRegister.GetInterfaceMetadata(TestPlugin3D::UID_3D_CONTEXT_TEST_IMPL).uid,
            TestPlugin3D::UID_3D_CONTEXT_TEST_IMPL);

        EXPECT_FALSE(info.createInterface);
        EXPECT_FALSE(CORE_NS::CreateInstance<TestPlugin3D::ITest>(TestPlugin3D::UID_3D_GLOBAL_TEST_IMPL));

        EXPECT_TRUE(info.getInterface);
        TestPlugin3D::ITest* sharedGlobal =
            CORE_NS::GetInstance<TestPlugin3D::ITest>(TestPlugin3D::UID_3D_GLOBAL_TEST_IMPL);
        EXPECT_TRUE(sharedGlobal);
        EXPECT_EQ(sharedGlobal->GetType(), TestPlugin3D::GlobalImpl().GetType());
        pluginRegister.UnloadPlugins({ uids });
    }
    EXPECT_EQ(globalClassRegister.GetInterfaceMetadata().size(), interfaceCount);
}

/**
 * @tc.name: pluginInterfaces
 * @tc.desc: Tests for Plugin Interfaces. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_PluginTest, pluginInterfaces, testing::ext::TestSize.Level1)
{
    CORE_NS::IPluginRegister& pluginRegister = CORE_NS::GetPluginRegister();

    CORE_NS::IClassRegister& globalClassRegister = pluginRegister.GetClassRegister();
    const auto globalInterfaceCount = globalClassRegister.GetInterfaceMetadata().size();

    CORE3D_NS::UTest::TestContext* testContext = CORE3D_NS::UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderCtx = testContext->renderContext;
    auto graphicsCtx = testContext->graphicsContext;

    CORE_NS::IClassRegister* contextRegister = graphicsCtx->GetInterface<CORE_NS::IClassRegister>();
    ASSERT_TRUE(contextRegister);
    const auto contextInterfaceCount = contextRegister->GetInterfaceMetadata().size();

    // Interfaces shouldn't be available before loading plugins.
    EXPECT_NE(contextRegister->GetInterfaceMetadata(TestPlugin3D::UID_3D_CONTEXT_TEST_IMPL).uid,
        TestPlugin3D::UID_3D_CONTEXT_TEST_IMPL);

    TypeInfoListener listener(pluginRegister);

    constexpr BASE_NS::Uid uids[] = { TestPlugin3D::UID_3D_TEST_PLUGIN };
    ASSERT_TRUE(pluginRegister.LoadPlugins(uids));
    EXPECT_EQ(listener.type_, CORE_NS::IPluginRegister::ITypeInfoListener::EventType::ADDED);
    EXPECT_EQ(listener.typeInfos_.size(), 1U);
    listener.typeInfos_.clear();

    EXPECT_EQ(contextRegister->GetInterfaceMetadata().size(), contextInterfaceCount + 1U);

    // Interface registered during IPlugin::registerInterfaces shouldn't be found from context.
    EXPECT_NE(contextRegister->GetInterfaceMetadata(TestPlugin3D::UID_3D_GLOBAL_TEST_IMPL).uid,
        TestPlugin3D::UID_3D_GLOBAL_TEST_IMPL);

    // Interface registered during I3DPlugin::createPlugin should be found from context.
    EXPECT_EQ(contextRegister->GetInterfaceMetadata(TestPlugin3D::UID_3D_CONTEXT_TEST_IMPL).uid,
        TestPlugin3D::UID_3D_CONTEXT_TEST_IMPL);

    CORE_NS::IClassFactory* contextFactory = graphicsCtx->GetInterface<CORE_NS::IClassFactory>();
    ASSERT_TRUE(contextFactory);
    ASSERT_TRUE(CORE_NS::CreateInstance<TestPlugin3D::ITest>(*contextFactory, TestPlugin3D::UID_3D_CONTEXT_TEST_IMPL));

    ASSERT_FALSE(CORE_NS::GetInstance<TestPlugin3D::ITest>(*contextRegister, TestPlugin3D::UID_3D_CONTEXT_TEST_IMPL));

    pluginRegister.UnloadPlugins(uids);

    EXPECT_EQ(listener.type_, CORE_NS::IPluginRegister::ITypeInfoListener::EventType::REMOVED);
    EXPECT_EQ(listener.typeInfos_.size(), 1U);
    EXPECT_EQ(contextRegister->GetInterfaceMetadata().size(), contextInterfaceCount);
    EXPECT_EQ(globalClassRegister.GetInterfaceMetadata().size(), globalInterfaceCount);
}
