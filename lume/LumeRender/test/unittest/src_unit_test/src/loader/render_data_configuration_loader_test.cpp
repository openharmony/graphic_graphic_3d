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

#include <loader/render_data_configuration_loader.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace RENDER_NS;

/**
 * @tc.name: LoadPostProcessTest
 * @tc.desc: Tests for loading render data configuration files.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderDataConfigurationLoader, LoadPostProcessTest, testing::ext::TestSize.Level1)
{
    // Get engine file manager
    CORE_NS::IFileManager& fileMng = UTest::GetTestEnv()->er.engine->GetFileManager();
    RenderDataConfigurationLoaderImpl loader;
    {
        BASE_NS::string postProcessString = "in{valid:\"]js}on";
        auto result = loader.LoadPostProcess(postProcessString);
        ASSERT_FALSE(result.loadResult.success);
    }
    {
        BASE_NS::string postProcessString = "{\"name\": \"config\", \"version\": 0.5}";
        auto result = loader.LoadPostProcess(postProcessString);
        ASSERT_FALSE(result.loadResult.success);
    }
    {
        auto result = loader.LoadPostProcess(fileMng, "test://RenderDataConfigurationLoaderTest.json");
        ASSERT_TRUE(result.loadResult.success);
    }
    {
        auto result = loader.LoadPostProcess(fileMng, "test://NonExistingFile.json");
        ASSERT_FALSE(result.loadResult.success);
    }
}

/**
 * @tc.name: GetInterfaceTest
 * @tc.desc: Tests for getting IRenderDataConfigurationLoader interface.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderDataConfigurationLoader, GetInterfaceTest, testing::ext::TestSize.Level1)
{
    {
        RenderDataConfigurationLoaderImpl loader;
        loader.Ref();
        loader.Unref();
        ASSERT_TRUE(loader.GetInterface(CORE_NS::IInterface::UID));
        ASSERT_TRUE(loader.GetInterface(CORE_NS::IInterface::UID)->GetInterface<IRenderDataConfigurationLoader>());
        ASSERT_TRUE(loader.GetInterface(IRenderDataConfigurationLoader::UID));
        ASSERT_FALSE(loader.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));
    }
    {
        const RenderDataConfigurationLoaderImpl loader;
        ASSERT_TRUE(loader.GetInterface(CORE_NS::IInterface::UID));
        ASSERT_TRUE(loader.GetInterface(CORE_NS::IInterface::UID)->GetInterface<IRenderDataConfigurationLoader>());
        ASSERT_TRUE(loader.GetInterface(IRenderDataConfigurationLoader::UID));
        ASSERT_FALSE(loader.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));
    }
}
