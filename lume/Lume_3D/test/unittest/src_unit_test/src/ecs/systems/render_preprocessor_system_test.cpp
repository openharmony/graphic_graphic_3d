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

#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/render_mesh_batch_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/util/intf_mesh_util.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>

#include "ecs/systems/render_preprocessor_system.h"
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

namespace {
Entity CreateSolidColorMaterial(IEcs& ecs, Math::Vec4 color)
{
    IMaterialComponentManager* materialManager = GetManager<IMaterialComponentManager>(ecs);
    Entity entity = ecs.GetEntityManager().Create();
    materialManager->Create(entity);
    if (auto materialHandle = materialManager->Write(entity); materialHandle) {
        materialHandle->textures[MaterialComponent::TextureIndex::BASE_COLOR].factor = color;
    }
    return entity;
}
} // namespace

/**
 * @tc.name: RenderPreprocessorSystemTest
 * @tc.desc: Tests for Render Preprocessor System Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsRenderPreprocessorSystem, RenderPreprocessorSystemTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto irpSystem = GetSystem<IRenderPreprocessorSystem>(*ecs);
    ASSERT_NE(nullptr, irpSystem);
    irpSystem->SetActive(true);
    EXPECT_TRUE(irpSystem->IsActive());
    EXPECT_EQ("RenderPreprocessorSystem", irpSystem->GetName());
    EXPECT_EQ(IRenderPreprocessorSystem::UID, ((ISystem*)irpSystem)->GetUid());
    EXPECT_NE(nullptr, irpSystem->GetProperties());
    EXPECT_NE(nullptr, ((const IRenderPreprocessorSystem*)irpSystem)->GetProperties());
    EXPECT_EQ(ecs.get(), &irpSystem->GetECS());

    RenderPreprocessorSystem* rpSystem = static_cast<RenderPreprocessorSystem*>(irpSystem);

    Entity materialEntity = CreateSolidColorMaterial(*ecs, Math::Vec4 { 1.0f, 1.0f, 1.0f, 1.0f });
    Entity cube = graphicsContext->GetMeshUtil().GenerateCube(*ecs, "myCube", materialEntity, 1.0f, 1.0f, 1.0f);
    Entity additionalMaterial = CreateSolidColorMaterial(*ecs, Math::Vec4 { 1.0f, 0.0f, 0.0f, 1.0f });

    auto meshManager = GetManager<IMeshComponentManager>(*ecs);
    auto renderMeshManager = GetManager<IRenderMeshComponentManager>(*ecs);
    if (auto scopedHandle = meshManager->Write(renderMeshManager->Read(cube)->mesh); scopedHandle) {
        ASSERT_EQ(1, scopedHandle->submeshes.size());
        scopedHandle->submeshes.push_back(scopedHandle->submeshes[0]);
        scopedHandle->submeshes[0].additionalMaterials.push_back(additionalMaterial);
        scopedHandle->submeshes[0].additionalMaterials.push_back({});
    }

    ecs->ProcessEvents();
    rpSystem->Update(true, 1u, 1u);
}

/**
 * @tc.name: MeshBatch
 * @tc.desc: Tests for Mesh Batch. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsRenderPreprocessorSystem, MeshBatch, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto irpSystem = GetSystem<IRenderPreprocessorSystem>(*ecs);
    ASSERT_NE(nullptr, irpSystem);
    irpSystem->SetActive(true);
    RenderPreprocessorSystem* rpSystem = static_cast<RenderPreprocessorSystem*>(irpSystem);

    auto inodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, inodeSystem);
    inodeSystem->SetActive(true);

    Entity materialEntity = CreateSolidColorMaterial(*ecs, Math::Vec4 { 1.0f, 1.0f, 1.0f, 1.0f });
    Entity cube = graphicsContext->GetMeshUtil().GenerateCube(*ecs, "myCube", materialEntity, 1.0f, 1.0f, 1.0f);
    auto renderMeshBatchManager = GetManager<IRenderMeshBatchComponentManager>(*ecs);
    renderMeshBatchManager->Create(cube);
    auto renderMeshManager = GetManager<IRenderMeshComponentManager>(*ecs);
    const Entity cubeMesh = renderMeshManager->Read(cube)->mesh;
    inodeSystem->GetNode(cube)->SetPosition(Math::Vec3(0.5f, 0.0f, 0.0f));

    ISceneNode* node = nullptr;
    for (auto i = 0U; i < 3U; ++i) {
        node = inodeSystem->CreateNode();
        node->SetPosition({ static_cast<float>(i) + 1.5f, 0.0f, 0.0f });
        renderMeshManager->Create(node->GetEntity());
        if (auto handle = renderMeshManager->Write(node->GetEntity())) {
            handle->mesh = cubeMesh;
            handle->renderMeshBatch = cube;
        }
    }
    ecs->ProcessEvents();
    ecs->Update(1u, 1u);

    const Entity cloneMaterial = CreateSolidColorMaterial(*ecs, Math::Vec4 { 1.0f, 0.0f, 0.0f, 1.0f });
    IMaterialComponentManager* materialManager = GetManager<IMaterialComponentManager>(*ecs);
    if (auto materialHandle = materialManager->Write(cloneMaterial); materialHandle) {
        materialHandle->materialLightingFlags &= ~MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT;
    }

    const Entity cloneMesh = ecs->GetEntityManager().Create();
    IMeshComponentManager* meshManager = GetManager<IMeshComponentManager>(*ecs);
    meshManager->Create(cloneMesh);
    if (auto handle = meshManager->Write(cloneMesh)) {
        *handle = *meshManager->Read(cubeMesh);
        for (auto& submesh : handle->submeshes) {
            submesh.material = cloneMaterial;
        }
    }
    renderMeshManager->Write(node->GetEntity())->mesh = cloneMesh;

    ecs->ProcessEvents();
    ecs->Update(2u, 1u);
}