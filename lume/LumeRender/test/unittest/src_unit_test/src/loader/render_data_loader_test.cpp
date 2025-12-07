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

#include <datastore/render_data_store_pod.h>
#include <loader/render_data_loader.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace RENDER_NS;

/**
 * @tc.name: LoadTest
 * @tc.desc: Tests for loading renderdata via RenderDataLoader.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderDataLoader, LoadTest, testing::ext::TestSize.Level1)
{
    // Get engine file manager
    CORE_NS::IFileManager& fileMng = UTest::GetTestEnv()->er.engine->GetFileManager();

    RenderDataLoader loader { fileMng };
    IRenderDataStorePod* dataStore = new RenderDataStorePod { "POD" };
    loader.Load("NonExistingDirectory", *dataStore);
    ASSERT_TRUE(dataStore->Get("PostProcess").empty());
    delete (RenderDataStorePod*)dataStore;
}
