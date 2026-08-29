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

#include <loader/pipeline_layout_loader.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace RENDER_NS;

/**
 * @tc.name: LoadPipelineLayoutTest
 * @tc.desc: Tests for Loading shader pipeline layouts.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_PipelineLayoutLoader, LoadPipelineLayoutTest, testing::ext::TestSize.Level1)
{
    // Get engine file manager
    CORE_NS::IFileManager& fileMng = UTest::GetTestEnv()->er.engine->GetFileManager();
    PipelineLayoutLoader loader;

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
        auto result = loader.Load(fileMng, "test://shaders/pipelinelayouts/PipelineLayoutLoaderTest.shaderpl");
        ASSERT_TRUE(result.success);
    }
    {
        auto result = loader.Load(fileMng, "test://shaders/pipelinelayouts/PipelineLayoutLoaderTest2.shaderpl");
        ASSERT_FALSE(result.success);
    }
    {
        auto result = loader.Load(fileMng, "test://shaders/pipelinelayouts/PipelineLayoutLoaderTest3.shaderpl");
        ASSERT_FALSE(result.success);
    }
    {
        auto result = loader.Load(fileMng, "test://shaders/pipelinelayouts/PipelineLayoutLoaderTest4.shaderpl");
        ASSERT_TRUE(result.success);
    }
}

/**
 * @tc.name: LoadPipelineLayoutOutOfRangeBindingTest
 * @tc.desc: Out of range binding slots must be dropped, backends index flat arrays with them.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_PipelineLayoutLoader, LoadPipelineLayoutOutOfRangeBindingTest, testing::ext::TestSize.Level1)
{
    PipelineLayoutLoader loader;
    {
        // binding == UINT32_MAX would overflow binding + 1U in the GLES backend and size the array to zero
        BASE_NS::string jsonStr = "{\"descriptorSetLayouts\":[{\"set\":0,\"bindings\":["
                                  "{\"binding\":4294967295,\"descriptorType\":\"uniform_buffer\"},"
                                  "{\"binding\":1,\"descriptorType\":\"uniform_buffer\"}]}]}";
        auto result = loader.Load(jsonStr);
        ASSERT_TRUE(result.success);
        const auto& bindings = loader.GetPipelineLayout().descriptorSetLayouts[0].bindings;
        ASSERT_EQ(bindings.size(), 1U);
        ASSERT_EQ(bindings[0].binding, 1U);
    }
    {
        // every binding at or above the maximum is dropped
        BASE_NS::string jsonStr = "{\"descriptorSetLayouts\":[{\"set\":0,\"bindings\":["
                                  "{\"binding\":16,\"descriptorType\":\"uniform_buffer\"}]}]}";
        auto result = loader.Load(jsonStr);
        ASSERT_TRUE(result.success);
        ASSERT_TRUE(loader.GetPipelineLayout().descriptorSetLayouts[0].bindings.empty());
    }
}
