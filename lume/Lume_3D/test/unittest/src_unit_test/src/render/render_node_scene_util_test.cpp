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

#include <algorithm>

#include <3d/implementation_uids.h>
#include <3d/render/intf_render_data_store_morph.h>
#include <3d/render/intf_render_node_scene_util.h>
#include <3d/util/intf_mesh_builder.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/intf_renderer.h>

#include "render/render_node_scene_util.h"
#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

/**
 * @tc.name: GetInterfaceTest
 * @tc.desc: Tests for Get Interface Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeSceneUtil, GetInterfaceTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    EXPECT_TRUE(testContext);

    // RenderNodeSceneUtilImpl doesn't implement Ref/Unref so just a local variable.
    RenderNodeSceneUtilImpl sceneUtilInstance;
    EXPECT_TRUE(sceneUtilInstance.GetInterface(IRenderNodeSceneUtil::UID));
    EXPECT_TRUE(sceneUtilInstance.GetInterface(IInterface::UID));
    EXPECT_FALSE(sceneUtilInstance.GetInterface(IClassFactory::UID));

    const IRenderNodeSceneUtil& sceneUtilConst = sceneUtilInstance;
    EXPECT_TRUE(sceneUtilConst.GetInterface(IRenderNodeSceneUtil::UID));
    EXPECT_TRUE(sceneUtilConst.GetInterface(IInterface::UID));
    EXPECT_FALSE(sceneUtilConst.GetInterface(IClassFactory::UID));
}
