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

#include <loader/shader_state_loader_util.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace RENDER_NS;

/**
 * @tc.name: ShaderStateUtilLoadTest
 * @tc.desc: Tests for ShaderStateLoaderUtil to parse shader states json data.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_ShaderStateLoaderUtil, ShaderStateUtilLoadTest, testing::ext::TestSize.Level1)
{
    // Get engine file manager
    CORE_NS::IFileManager& fileMng = UTest::GetTestEnv()->er.engine->GetFileManager();

    {
        BASE_NS::string shaderStateStr = "{\"shaderStates\": [ {\"variantName\": \"\"} ]}";
        auto jsonData = CORE_NS::json::parse(shaderStateStr.data());
        auto result = ShaderStateLoaderUtil::LoadStates(jsonData);
        ASSERT_FALSE(result.res.success);
    }
    {
        auto file = fileMng.OpenFile("test://ShaderStateLoaderUtilTest.json");
        ASSERT_TRUE(file != nullptr);
        const uint64_t byteLength = file->GetLength();
        BASE_NS::string shaderStateStr(static_cast<size_t>(byteLength), BASE_NS::string::value_type());
        ASSERT_EQ(byteLength, file->Read(shaderStateStr.data(), byteLength));
        auto jsonData = CORE_NS::json::parse(shaderStateStr.data());
        auto result = ShaderStateLoaderUtil::LoadStates(jsonData);
        ASSERT_TRUE(result.res.success);
    }
    {
        auto file = fileMng.OpenFile("test://ShaderStateLoaderUtilTest2.json");
        ASSERT_TRUE(file != nullptr);
        const uint64_t byteLength = file->GetLength();
        BASE_NS::string shaderStateStr(static_cast<size_t>(byteLength), BASE_NS::string::value_type());
        ASSERT_EQ(byteLength, file->Read(shaderStateStr.data(), byteLength));
        auto jsonData = CORE_NS::json::parse(shaderStateStr.data());
        auto result = ShaderStateLoaderUtil::LoadStates(jsonData);
        ASSERT_FALSE(result.res.success);
    }
}
