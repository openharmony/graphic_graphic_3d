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

#include <3d/ecs/components/render_handle_component.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/device/intf_gpu_resource_manager.h>

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
 * @tc.name: GetRenderHandleTest
 * @tc.desc: Tests for Get Render Handle Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsRenderHandleComponent, GetRenderHandleTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto renderHandleManager = GetManager<IRenderHandleComponentManager>(*ecs);
    auto componentManager = static_cast<IComponentManager*>(renderHandleManager);
    EXPECT_EQ("RenderHandleComponent", componentManager->GetName());

    EXPECT_EQ(RenderHandle {}, renderHandleManager->GetRenderHandle(Entity {}));
    EXPECT_EQ(RenderHandle {}, renderHandleManager->GetRenderHandle(~0u));
    EXPECT_EQ(RenderHandle {}, renderHandleManager->GetRenderHandleReference(Entity {}).GetHandle());
    EXPECT_EQ(RenderHandle {}, renderHandleManager->GetRenderHandleReference(~0u).GetHandle());

    GpuBufferDesc desc;
    desc.byteSize = 16u;
    desc.usageFlags = CORE_BUFFER_USAGE_INDEX_BUFFER_BIT;
    desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.format = BASE_FORMAT_R16G16B16A16_SFLOAT;
    desc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
    RenderHandleReference handle = renderContext->GetDevice().GetGpuResourceManager().Create(desc);

    Entity bufferEntity = ecs->GetEntityManager().Create();
    renderHandleManager->Create(bufferEntity);
    if (auto scopedHandle = renderHandleManager->Write(bufferEntity); scopedHandle) {
        scopedHandle->reference = handle;
    }

    auto componentId = renderHandleManager->GetComponentId(bufferEntity);
    EXPECT_EQ(handle.GetHandle(), renderHandleManager->GetRenderHandleReference(bufferEntity).GetHandle());
    EXPECT_EQ(handle.GetHandle(), renderHandleManager->GetRenderHandleReference(componentId).GetHandle());
    EXPECT_EQ(handle.GetHandle(), renderHandleManager->GetRenderHandle(bufferEntity));
    EXPECT_EQ(handle.GetHandle(), renderHandleManager->GetRenderHandle(componentId));

    handle = {};
}
