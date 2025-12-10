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

#include <loader/render_node_graph_loader.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

// Scope conflict with RenderNodeGraphLoader::LoadString and LoadString in libloaderapi.h
#undef LoadString

using namespace RENDER_NS;

/**
 * @tc.name: LoadRenderNodeGraphTest
 * @tc.desc: Tests for loading render node graphs.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeGraphLoader, LoadRenderNodeGraphTest, testing::ext::TestSize.Level1)
{
    // Get engine file manager
    CORE_NS::IFileManager& fileMng = UTest::GetTestEnv()->er.engine->GetFileManager();
    RenderNodeGraphLoader loader(fileMng);

    {
        auto result = loader.Load("test://renderNodeGraph.rng");
        if (!result.error.empty()) {
            CORE_LOG_E("RenderNodeGraphLoader error message: '%s'", result.error.c_str());
        }
        ASSERT_TRUE(result.success);
        ASSERT_TRUE(result.error.empty());

        auto const& nodeDesc = result.desc;
        ASSERT_EQ(9, nodeDesc.nodes.size());
    }
    {
        CORE_NS::IFile::Ptr file = fileMng.OpenFile("test://renderNodeGraph2.rng");
        ASSERT_TRUE(file);

        const uint64_t byteLength = file->GetLength();

        BASE_NS::string raw(static_cast<size_t>(byteLength), BASE_NS::string::value_type());
        ASSERT_EQ(byteLength, file->Read(raw.data(), byteLength));

        auto result = loader.LoadString(raw);

        if (!result.error.empty()) {
            CORE_LOG_E("RenderNodeGraphLoader error message: '%s'", result.error.c_str());
        }
        ASSERT_TRUE(result.success);
        ASSERT_TRUE(result.error.empty());

        auto const& nodeDesc = result.desc;
        ASSERT_EQ(2, nodeDesc.nodes.size());
    }
    {
        auto result = loader.Load("test://renderNodeGraph3.rng");
        if (!result.error.empty()) {
            CORE_LOG_E("RenderNodeGraphLoader error message: '%s'", result.error.c_str());
        }
        ASSERT_TRUE(result.success);
        ASSERT_TRUE(result.error.empty());

        auto const& nodeDesc = result.desc;
        ASSERT_EQ(1, nodeDesc.nodes.size());
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    {
        auto result = loader.Load("test://renderNodeGraphInvalid.rng");
        ASSERT_FALSE(result.success);
        ASSERT_FALSE(result.error.empty());
    }
    {
        auto result = loader.Load("test://renderNodeGraphInvalid2.rng");
        ASSERT_FALSE(result.success);
        ASSERT_FALSE(result.error.empty());
    }
    {
        auto result = loader.Load("test://renderNodeGraphInvalid3.rng");
        ASSERT_FALSE(result.success);
        ASSERT_FALSE(result.error.empty());
    }
    {
        auto result = loader.Load("test://renderNodeGraphInvalid4.rng");
        ASSERT_FALSE(result.success);
        ASSERT_FALSE(result.error.empty());
    }
    {
        auto result = loader.Load("test://renderNodeGraphInvalid5.rng");
        ASSERT_FALSE(result.success);
        ASSERT_FALSE(result.error.empty());
    }
    {
        auto result = loader.Load("test://nonExistingFile.rng");
        ASSERT_FALSE(result.success);
        ASSERT_FALSE(result.error.empty());
    }
    {
        // load large rng with too large node count, names, output resources etc.
        auto result = loader.Load("test://renderNodeGraphLarge.rng");
        ASSERT_TRUE(result.success);
        ASSERT_TRUE(result.error.empty());
    }
#endif
}
