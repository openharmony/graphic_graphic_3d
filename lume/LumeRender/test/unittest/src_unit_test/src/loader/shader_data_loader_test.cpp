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

#include <loader/shader_data_loader.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace RENDER_NS;

/**
 * @tc.name: LoadShaderDataTest
 * @tc.desc: Tests for loading ShaderData from a json file.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_ShaderDataLoader, LoadShaderDataTest, testing::ext::TestSize.Level1)
{
    // Get engine file manager
    CORE_NS::IFileManager& fileMng = UTest::GetTestEnv()->er.engine->GetFileManager();
    ShaderDataLoader loader;

    {
        auto result = loader.Load(fileMng, "test://nonExistingFile.json");
        ASSERT_FALSE(result.success);
    }
    {
        auto result = loader.Load(fileMng, "test://ShaderDataLoaderTest.json");
        ASSERT_TRUE(result.success);
    }
    {
        auto result = loader.Load(fileMng, "test://ShaderDataLoaderTest2.json");
        ASSERT_FALSE(result.success);
    }
    {
        auto result = loader.Load(fileMng, "test://ShaderDataLoaderTest3.json");
        ASSERT_FALSE(result.success);
    }
    {
        auto result = loader.Load(fileMng, "test://ShaderDataLoaderTest4.json");
        ASSERT_FALSE(result.success);
    }
}
