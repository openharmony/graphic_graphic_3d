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

#include "graphics_manager.h"
#include "3d_widget_adapter_log.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {

class GraphicsManagerUT : public ::testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: GetInstance001
 * @tc.desc: test GetInstance returns singleton
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerUT, GetInstance001, TestSize.Level1)
{
    WIDGET_LOGD("GetInstance001");
    auto& instance1 = GraphicsManager::GetInstance();
    auto& instance2 = GraphicsManager::GetInstance();

    // Verify singleton pattern - same instance returned
    EXPECT_EQ(&instance1, &instance2);
}

/**
 * @tc.name: GetUseBasisEngine001
 * @tc.desc: test GetUseBasisEngine default value
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerUT, GetUseBasisEngine001, TestSize.Level1)
{
    WIDGET_LOGD("GetUseBasisEngine001");
    auto& manager = GraphicsManager::GetInstance();

    // Default value should be false
    bool useBasisEngine = manager.GetUseBasisEngine();
    EXPECT_EQ(useBasisEngine, false);
}

/**
 * @tc.name: SetUseBasisEngine001
 * @tc.desc: test SetUseBasisEngine with true
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerUT, SetUseBasisEngine001, TestSize.Level1)
{
    WIDGET_LOGD("SetUseBasisEngine001");
    auto& manager = GraphicsManager::GetInstance();

    manager.SetUseBasisEngine(true);
    bool useBasisEngine = manager.GetUseBasisEngine();
    EXPECT_EQ(useBasisEngine, true);

    // Reset to default
    manager.SetUseBasisEngine(false);
}

/**
 * @tc.name: SetUseBasisEngine002
 * @tc.desc: test SetUseBasisEngine with false
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerUT, SetUseBasisEngine002, TestSize.Level1)
{
    WIDGET_LOGD("SetUseBasisEngine002");
    auto& manager = GraphicsManager::GetInstance();

    manager.SetUseBasisEngine(true);
    manager.SetUseBasisEngine(false);
    bool useBasisEngine = manager.GetUseBasisEngine();
    EXPECT_EQ(useBasisEngine, false);
}

/**
 * @tc.name: SetUseBasisEngine003
 * @tc.desc: test SetUseBasisEngine multiple times
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerUT, SetUseBasisEngine003, TestSize.Level1)
{
    WIDGET_LOGD("SetUseBasisEngine003");
    auto& manager = GraphicsManager::GetInstance();

    manager.SetUseBasisEngine(true);
    EXPECT_EQ(manager.GetUseBasisEngine(), true);

    manager.SetUseBasisEngine(false);
    EXPECT_EQ(manager.GetUseBasisEngine(), false);

    manager.SetUseBasisEngine(true);
    EXPECT_EQ(manager.GetUseBasisEngine(), true);

    manager.SetUseBasisEngine(false);
    EXPECT_EQ(manager.GetUseBasisEngine(), false);
}

/**
 * @tc.name: GetPlatformData001
 * @tc.desc: test GetPlatformData without HapInfo
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerUT, GetPlatformData001, TestSize.Level1)
{
    WIDGET_LOGD("GetPlatformData001");
    auto& manager = GraphicsManager::GetInstance();

    PlatformData data = manager.GetPlatformData();

    // Verify platform data is returned with non-empty paths
    EXPECT_FALSE(data.coreRootPath_.empty());
    EXPECT_FALSE(data.corePluginPath_.empty());
    EXPECT_FALSE(data.appRootPath_.empty());
    EXPECT_FALSE(data.appPluginPath_.empty());
}

/**
 * @tc.name: GetPlatformData002
 * @tc.desc: test GetPlatformData with HapInfo
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerUT, GetPlatformData002, TestSize.Level1)
{
    WIDGET_LOGD("GetPlatformData002");
    auto& manager = GraphicsManager::GetInstance();

    HapInfo hapInfo;
    hapInfo.hapPath_ = "/data/test/test.hap";
    hapInfo.bundleName_ = "com.test.bundle";
    hapInfo.moduleName_ = "test_module";

    PlatformData data = manager.GetPlatformData(hapInfo);

    // Verify platform data is returned with HapInfo
    EXPECT_FALSE(data.coreRootPath_.empty());
    EXPECT_FALSE(data.corePluginPath_.empty());
    EXPECT_FALSE(data.appRootPath_.empty());
    EXPECT_FALSE(data.appPluginPath_.empty());
    EXPECT_EQ(data.hapInfo_.hapPath_, hapInfo.hapPath_);
    EXPECT_EQ(data.hapInfo_.bundleName_, hapInfo.bundleName_);
    EXPECT_EQ(data.hapInfo_.moduleName_, hapInfo.moduleName_);
}

} // namespace OHOS::Render3D
