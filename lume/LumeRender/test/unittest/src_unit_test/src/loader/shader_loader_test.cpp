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

#include <device/shader_pipeline_binder.h>
#include <loader/shader_loader.h>

#include <core/io/intf_file_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>

#include "loader/json_util.h"
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
void TestShaderLoader(const UTest::EngineResources& engine)
{
    IFileManager& fileMgr = engine.engine->GetFileManager();
    ShaderManager& shaderMgr = static_cast<ShaderManager&>(engine.device->GetShaderManager());
    ShaderLoader shaderLoader { fileMgr, shaderMgr, engine.backend };
    {
        IShaderManager::ShaderFilePathDesc desc;
        desc.pipelineLayoutPath = "nonExistingPath";
        desc.shaderPath = "nonExistingPath";
        desc.shaderStatePath = "nonExistingPath";
        desc.vertexInputDeclarationPath = "nonExistingPath";
        shaderLoader.Load(desc);
    }
}
} // namespace

/**
 * @tc.name: ShaderLoaderTest
 * @tc.desc: Tests for ShaderLoader by using non-existent paths.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_ShaderLoader, ShaderLoaderTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);
    TestShaderLoader(engine);
    UTest::DestroyEngine(engine);
}

/**
 * @tc.name: ParseHex
 * @tc.desc: Tests for parsing hexadecimal strings.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_Loader, ParseHex, testing::ext::TestSize.Level1)
{
    uint32_t val;
    EXPECT_TRUE(RENDER_NS::ParseHex("1", val));
    EXPECT_EQ(val, 1U);
    EXPECT_TRUE(RENDER_NS::ParseHex("10", val));
    EXPECT_EQ(val, 16U);
    EXPECT_TRUE(RENDER_NS::ParseHex("ffFFffFF", val));
    EXPECT_EQ(val, UINT32_MAX);

    // empty string
    EXPECT_FALSE(RENDER_NS::ParseHex("", val));
    EXPECT_EQ(val, 0U);
    EXPECT_FALSE(RENDER_NS::ParseHex("  ", val));
    EXPECT_EQ(val, 0U);
    EXPECT_FALSE(RENDER_NS::ParseHex("\t", val));
    EXPECT_EQ(val, 0U);

    // something else after number
    EXPECT_FALSE(RENDER_NS::ParseHex("1 1", val));
    EXPECT_EQ(val, 0U);
    // doesn't fix in 32 bits
    EXPECT_FALSE(RENDER_NS::ParseHex("100000000", val));
    EXPECT_EQ(val, 0U);
    // non-hex character
    EXPECT_FALSE(RENDER_NS::ParseHex("G", val));
    EXPECT_EQ(val, 0U);
    // Empty String
    EXPECT_FALSE(RENDER_NS::ParseHex("", val));
    EXPECT_EQ(val, 0U);
}
