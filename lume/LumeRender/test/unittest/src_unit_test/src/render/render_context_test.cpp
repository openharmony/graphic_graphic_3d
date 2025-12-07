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

#include <render_context.h>

#include <render/datastore/intf_render_data_store.h>
#include <render/intf_plugin.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace {
void TestRenderContext(const UTest::EngineResources& er)
{
    IRenderContext& context = *er.context.get();
    ASSERT_EQ(er.engine.get(), &context.GetEngine());
    ASSERT_EQ(er.device, &context.GetDevice());
    ASSERT_EQ(0, context.GetVersion().size());
    ASSERT_EQ("lume_test", ((RenderContext&)context).GetCreateInfo().applicationInfo.name);

    ASSERT_TRUE(context.GetInterface(CORE_NS::IInterface::UID));
    ASSERT_TRUE(context.GetInterface(CORE_NS::IInterface::UID)->GetInterface<IRenderContext>());
    ASSERT_TRUE(context.GetInterface(IRenderContext::UID));
    ASSERT_TRUE(context.GetInterface(IClassRegister::UID));
    ASSERT_TRUE(context.GetInterface(IClassFactory::UID));
    ASSERT_FALSE(context.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));
    const auto& constContext = context;
    ASSERT_TRUE(constContext.GetInterface(CORE_NS::IInterface::UID));
    ASSERT_TRUE(constContext.GetInterface(CORE_NS::IInterface::UID)->GetInterface<IRenderContext>());
    ASSERT_TRUE(constContext.GetInterface(IRenderContext::UID));
    ASSERT_TRUE(constContext.GetInterface(IClassRegister::UID));
    ASSERT_TRUE(constContext.GetInterface(IClassFactory::UID));
    ASSERT_FALSE(constContext.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));

    IRenderDataConfigurationLoader* loader = static_cast<IRenderDataConfigurationLoader*>(
        ((RenderContext&)context).GetInstance(UID_RENDER_DATA_CONFIGURATION_LOADER));
    ASSERT_NE(nullptr, loader);
    ASSERT_EQ(nullptr, context.CreateInstance({}).get());
    ASSERT_EQ(nullptr, context.CreateInstance(IClassFactory::UID).get());
    ASSERT_EQ(nullptr, ((RenderContext&)context).GetInstance({}));
    ASSERT_EQ(nullptr, ((RenderContext&)context).GetInstance(IClassFactory::UID));

    auto metadata = ((RenderContext&)context).GetInterfaceMetadata();
    uint32_t initialSize = metadata.size();
    {
        InterfaceTypeInfo interfaceInfo { nullptr, UID_RENDER_DATA_CONFIGURATION_LOADER };
        ((RenderContext&)context).UnregisterInterfaceType(interfaceInfo);
    }
    {
        auto metadata = ((RenderContext&)context).GetInterfaceMetadata();
        ASSERT_EQ(initialSize - 1, metadata.size());
    }

    RenderContext* renderContext = static_cast<RenderContext*>(&context);

    IPluginRegister::ITypeInfoListener* typeInfoListener = (IPluginRegister::ITypeInfoListener*)renderContext;
    ASSERT_NE(nullptr, typeInfoListener);
    {
        IRenderPlugin plugin0 { [](IRenderContext&) -> PluginToken { return PluginToken(); }, [](PluginToken) {} };
        IRenderPlugin plugin1 { nullptr, nullptr };
        RenderDataStoreTypeInfo plugin2 { { RenderDataStoreTypeInfo::UID }, RenderDataStoreTypeInfo::UID, "plugin2",
#ifdef NDEBUG
            nullptr
#else
            [](IRenderContext&, char const*) -> BASE_NS::refcnt_ptr<IRenderDataStore> { return nullptr; }
#endif
        };
        RenderNodeTypeInfo plugin3 { { RenderNodeTypeInfo::UID }, RenderNodeTypeInfo::UID, "plugin3",
#ifdef NDEBUG
            nullptr, nullptr
#else
            []() -> IRenderNode* { return nullptr; }, [](IRenderNode*) -> void {}
#endif
        };
        ITypeInfo* typeInfos[] = { &plugin0, &plugin1, &plugin2, &plugin3 };
        typeInfoListener->OnTypeInfoEvent(
            IPluginRegister::ITypeInfoListener::EventType::ADDED, { typeInfos, countof(typeInfos) });
        typeInfoListener->OnTypeInfoEvent(
            IPluginRegister::ITypeInfoListener::EventType::REMOVED, { typeInfos, countof(typeInfos) });
    }
}
} // namespace

/**
 * @tc.name: RenderContextTest
 * @tc.desc: Tests for RenderContext by querying interfaces and testing the RenderContext class methods.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderContext, RenderContextTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);
    TestRenderContext(engine);
    UTest::DestroyEngine(engine);
}
