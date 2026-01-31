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

#include "scene_adapter/scene_bridge.h"
#include "scene_adapter/intf_scene_adapter.h"
#include "3d_widget_adapter_log.h"

// Mock SceneJS class for testing
class MockSceneJS {
public:
    std::shared_ptr<OHOS::Render3D::ISceneAdapter> scene_;
};

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {

class SceneBridgeUT : public ::testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: UnwrapSceneFromJs001
 * @tc.desc: test UnwrapSceneFromJs with null env
 * @tc.type: FUNC
 */
HWTEST_F(SceneBridgeUT, UnwrapSceneFromJs001, TestSize.Level1)
{
    WIDGET_LOGD("UnwrapSceneFromJs001");
    napi_env env = nullptr;
    napi_value value = nullptr;

    auto result = SceneBridge::UnwrapSceneFromJs(env, value);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: UnwrapSceneFromJs002
 * @tc.desc: test UnwrapSceneFromJs with null env and non-null value
 * @tc.type: FUNC
 */
HWTEST_F(SceneBridgeUT, UnwrapSceneFromJs002, TestSize.Level1)
{
    WIDGET_LOGD("UnwrapSceneFromJs002");
    napi_env env = nullptr;
    napi_value value = reinterpret_cast<napi_value>(0x1);

    // Should return nullptr when env is null
    auto result = SceneBridge::UnwrapSceneFromJs(env, value);
    EXPECT_EQ(result, nullptr);
}

} // namespace OHOS::Render3D
