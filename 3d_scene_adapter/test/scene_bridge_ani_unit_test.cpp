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

#include <ani.h>
#include "scene_adapter/scene_bridge_ani.h"
#include "3d_widget_adapter_log.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {

class SceneBridgeAniUT : public ::testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: UnwrapSceneFromAni001
 * @tc.desc: test UnwrapSceneFromAni with null env
 * @tc.type: FUNC
 */
HWTEST_F(SceneBridgeAniUT, UnwrapSceneFromAni001, TestSize.Level1)
{
    WIDGET_LOGD("UnwrapSceneFromAni001");
    ani_env* env = nullptr;
    ani_object value = nullptr;

    auto result = SceneBridgeAni::UnwrapSceneFromAni(env, value);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: UnwrapSceneFromAni002
 * @tc.desc: test UnwrapSceneFromAni with null env and non-null value
 * @tc.type: FUNC
 */
HWTEST_F(SceneBridgeAniUT, UnwrapSceneFromAni002, TestSize.Level1)
{
    WIDGET_LOGD("UnwrapSceneFromAni002");
    ani_env* env = nullptr;
    ani_object value = reinterpret_cast<ani_object>(0x1);

    // Should return nullptr when env is null
    auto result = SceneBridgeAni::UnwrapSceneFromAni(env, value);
    EXPECT_EQ(result, nullptr);
}

} // namespace OHOS::Render3D
