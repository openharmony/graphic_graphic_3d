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

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/skin_ibm_component.h>
#include <3d/ecs/components/uri_component.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
#include "util/uri_lookup.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace {
Entity CreateUriEntity(IEcs& ecs, IUriComponentManager& uriManager, string_view uri)
{
    Entity entity = ecs.GetEntityManager().Create();
    uriManager.Create(entity);
    if (auto h = uriManager.Write(entity); h) {
        h->uri = uri;
    }
    return entity;
}
}  // namespace

/**
 * @tc.name: LookupResourceByUriAllInstantiations
 * @tc.desc: Drives every LookupResourceByUri template instantiation through the matched
 *           (found), unmatched-uri, and matched-without-component branches.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_UriLookup, LookupResourceByUriAllInstantiations, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto ecs = testContext->ecs;

    auto uriManager = GetManager<IUriComponentManager>(*ecs);
    auto animationManager = GetManager<IAnimationComponentManager>(*ecs);
    auto materialManager = GetManager<IMaterialComponentManager>(*ecs);
    auto meshManager = GetManager<IMeshComponentManager>(*ecs);
    auto skinIbmManager = GetManager<ISkinIbmComponentManager>(*ecs);
    auto renderHandleManager = GetManager<IRenderHandleComponentManager>(*ecs);
    ASSERT_NE(nullptr, uriManager);

    Entity animEntity = CreateUriEntity(*ecs, *uriManager, "test://anim");
    animationManager->Create(animEntity);

    Entity materialEntity = CreateUriEntity(*ecs, *uriManager, "test://material");
    materialManager->Create(materialEntity);

    Entity meshEntity = CreateUriEntity(*ecs, *uriManager, "test://mesh");
    meshManager->Create(meshEntity);

    Entity skinEntity = CreateUriEntity(*ecs, *uriManager, "test://skin");
    skinIbmManager->Create(skinEntity);

    Entity renderEntity = CreateUriEntity(*ecs, *uriManager, "test://render");
    renderHandleManager->Create(renderEntity);

    // URI-only entity (no matching component); exercises the found-but-no-component branch.
    Entity uriOnly = CreateUriEntity(*ecs, *uriManager, "test://anim");

    // Found path for every instantiation.
    EXPECT_EQ(animEntity, LookupResourceByUri("test://anim", *uriManager, *animationManager));
    EXPECT_EQ(materialEntity, LookupResourceByUri("test://material", *uriManager, *materialManager));
    EXPECT_EQ(meshEntity, LookupResourceByUri("test://mesh", *uriManager, *meshManager));
    EXPECT_EQ(skinEntity, LookupResourceByUri("test://skin", *uriManager, *skinIbmManager));
    EXPECT_EQ(renderEntity, LookupResourceByUri("test://render", *uriManager, *renderHandleManager));

    // No match — iterates through all URI entries without returning.
    EXPECT_EQ(Entity{}, LookupResourceByUri("test://missing", *uriManager, *animationManager));

    // URI exists twice but only one entity has the component — confirms the
    // HasComponent guard rejects the URI-only entity.
    EXPECT_EQ(animEntity, LookupResourceByUri("test://anim", *uriManager, *animationManager));

    // Destroyed entity exercises the IsAlive false arm.
    ecs->GetEntityManager().Destroy(meshEntity);
    EXPECT_EQ(Entity{}, LookupResourceByUri("test://mesh", *uriManager, *meshManager));

    (void)uriOnly;
}
