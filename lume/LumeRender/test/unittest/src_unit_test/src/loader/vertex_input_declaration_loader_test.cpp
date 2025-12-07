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

#include <loader/vertex_input_declaration_loader.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace RENDER_NS;

/**
 * @tc.name: LoadVertexInputDeclarationTest
 * @tc.desc: Tests for loading  vertex input declarations via json files.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_VertexInputDeclarationLoader, LoadVertexInputDeclarationTest, testing::ext::TestSize.Level1)
{
    // Get engine file manager
    CORE_NS::IFileManager& fileMng = UTest::GetTestEnv()->er.engine->GetFileManager();
    VertexInputDeclarationLoader loader;

    {
        BASE_NS::string vidJsonStr = "{\"inv\": alid\"\"j} son{]}";
        auto result = loader.Load(vidJsonStr);
        ASSERT_FALSE(result.success);
    }
    {
        auto result = loader.Load(fileMng, "test://nonExistingFile.json");
        ASSERT_FALSE(result.success);
    }
    {
        auto result =
            loader.Load(fileMng, "test://shaders/vertexinputdeclarations/VertexInputDeclarationLoaderTest.shadervid");
        ASSERT_TRUE(result.success);
    }
    {
        auto result =
            loader.Load(fileMng, "test://shaders/vertexinputdeclarations/VertexInputDeclarationLoaderTest2.shadervid");
        ASSERT_FALSE(result.success);
    }
    {
        auto result =
            loader.Load(fileMng, "test://shaders/vertexinputdeclarations/VertexInputDeclarationLoaderTest3.shadervid");
        ASSERT_FALSE(result.success);
    }
}
