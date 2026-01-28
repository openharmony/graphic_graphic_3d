/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <memory>

#include "common/ets_test.h"
#include "SceneETS.h"

using namespace testing;
using namespace testing::ext;
namespace OHOS::Render3D {
class SceneETSUnitTest : public EtsTest {
};

/**
 * @tc.name: SceneETS_Load_001
 * @tc.desc: Test SceneETS::Load with empty path
 * @tc.type: FUNC
 */
HWTEST_F(SceneETSUnitTest, SceneETS_Load_001, TestSize.Level1)
{
    auto sceneETS = std::make_shared<SceneETS>();
    bool result = sceneETS->Load("");
    EXPECT_TRUE(result);
    sceneETS->Destroy();
}
} // namespace OHOS::Render3D
