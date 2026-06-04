/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <3d/ecs/components/light_probe_group_component.h>
#include <3d/ecs/systems/intf_light_probe_baker_system.h>
#include <3d/implementation_uids.h>
#include <3d/render/intf_render_data_store_morph.h>
#include <3d/render/intf_render_node_scene_util.h>
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

UNIT_TEST(API_LightProbeBaker, BakeTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto graphicsContext = testContext->graphicsContext;
    auto lightProbeBaker = CreateInstance<ILightProbeBaker>(
        *testContext->renderContext->GetInterface<CORE_NS::IClassFactory>(), UID_LIGHT_PROBE_BAKER);

    lightProbeBaker->Initialize(*testContext->ecs);

    lightProbeBaker->StartBake({});

    auto entity = testContext->ecs->GetEntityManager().Create();
    lightProbeBaker->StartBake({&entity, 1});

    auto lightProbeManager = GetManager<ILightProbeGroupComponentManager>(*testContext->ecs);

    lightProbeManager->Create(entity);
    if (auto lightProbeGroupComponent = lightProbeManager->Write(entity)) {
        using LightProbe = CORE3D_NS::LightProbeGroupComponent::LightProbe;
        LightProbe lp{};
        lp.position = {1, 0, 0};
        lp.bentNormal = {0, 1, 0};
        lp.ao = 1.0f;
        lightProbeGroupComponent->lightProbes.push_back(lp);
        lp.position = {1, 1, 0};
        lightProbeGroupComponent->lightProbes.push_back(lp);
        lp.position = {0, 1, 0};
        lightProbeGroupComponent->lightProbes.push_back(lp);
        lp.position = {0, 1, 1};
        lightProbeGroupComponent->lightProbes.push_back(lp);
    }

    auto p = testContext->ecs.get();
    auto a = array_view<CORE_NS::IEcs*>(&p, 1);
    testContext->engine->TickFrame(a);

    lightProbeBaker->StartBake({&entity, 1});
    testContext->engine->TickFrame(a);
    testContext->engine->TickFrame(a);
    testContext->engine->TickFrame(a);
    testContext->engine->TickFrame(a);
    lightProbeBaker->StartBake({&entity, 1});
    testContext->engine->TickFrame(a);
}
