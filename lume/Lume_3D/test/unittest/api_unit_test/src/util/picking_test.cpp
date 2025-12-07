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

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/joint_matrices_component.h>
#include <3d/ecs/components/layer_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/implementation_uids.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_picking.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_class_register.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/implementation_uids.h>

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
void SetAabb(Entity entity, IEcs& ecs, Math::Vec3 aabbMin, Math::Vec3 aabbMax)
{
    auto renderMeshManager = GetManager<IRenderMeshComponentManager>(ecs);
    auto meshManager = GetManager<IMeshComponentManager>(ecs);
    renderMeshManager->Create(entity);
    Entity mesh = ecs.GetEntityManager().Create();
    meshManager->Create(mesh);
    if (auto scopedHandle = meshManager->Write(mesh); scopedHandle) {
        scopedHandle->aabbMin = aabbMin;
        scopedHandle->aabbMax = aabbMax;
    }
    if (auto scopedHandle = renderMeshManager->Write(entity); scopedHandle) {
        scopedHandle->mesh = mesh;
    }
}

void SetWorldMatrixFromPosition(Entity entity, IEcs& ecs, Math::Vec3 position)
{
    auto worldMatrixManager = GetManager<IWorldMatrixComponentManager>(ecs);
    worldMatrixManager->Create(entity);
    if (auto scopedHandle = worldMatrixManager->Write(entity); scopedHandle) {
        scopedHandle->matrix =
            Math::Trs(position, Math::FromEulerRad(Math::Vec3 { 0.0f, 0.0f, 0.0f }), Math::Vec3 { 1.0f, 1.0f, 1.0f });
    }
}
} // namespace

/**
 * @tc.name: IInterfaceTest
 * @tc.desc: Tests for Iinterface Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilPicking, IInterfaceTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto picking = GetInstance<IPicking>(*renderContext->GetInterface<IClassRegister>(), UID_PICKING);
    ASSERT_NE(nullptr, picking);
    picking->Ref();
    picking->Unref();
    EXPECT_EQ(picking, picking->GetInterface(IPicking::UID));
    EXPECT_EQ(picking, picking->GetInterface(IInterface::UID));
    EXPECT_EQ(nullptr, picking->GetInterface(INodeSystem::UID));
    EXPECT_EQ(picking, ((const IPicking*)picking)->GetInterface(IPicking::UID));
    EXPECT_EQ(picking, ((const IPicking*)picking)->GetInterface(IInterface::UID));
    EXPECT_EQ(nullptr, ((const IPicking*)picking)->GetInterface(INodeSystem::UID));
}

/**
 * @tc.name: GetTransformComponentAABBTest
 * @tc.desc: Tests for Get Transform Component Aabbtest. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilPicking, GetTransformComponentAABBTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto picking = GetInstance<IPicking>(*renderContext->GetInterface<IClassRegister>(), UID_PICKING);
    ASSERT_NE(nullptr, picking);
    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);

    ISceneNode* root = nodeSystem->CreateNode();
    root->SetPosition(Math::Vec3 { -5.0f, 0.0f, 5.0f });
    SetAabb(root->GetEntity(), *ecs, Math::Vec3 { -1.0f, -1.0f, -1.0f }, Math::Vec3 { 2.0f, 2.0f, 2.0f });

    ISceneNode* child1 = nodeSystem->CreateNode();
    child1->SetPosition(Math::Vec3 { 0.0f, 5.0f, -5.0f });
    child1->SetParent(*root);
    SetAabb(child1->GetEntity(), *ecs, Math::Vec3 { -2.0f, -2.0f, -2.0f }, Math::Vec3 { 0.0f, 0.0f, 0.0f });

    ISceneNode* child2 = nodeSystem->CreateNode();
    child2->SetPosition(Math::Vec3 { 5.0f, -5.0f, 0.0f });
    child2->SetParent(*root);
    SetAabb(child2->GetEntity(), *ecs, Math::Vec3 { 0.0f, 0.0f, 0.0f }, Math::Vec3 { 3.0f, 3.0f, 3.0f });
    {
        MinAndMax mam = picking->GetTransformComponentAABB(root->GetEntity(), true, *ecs);
        EXPECT_EQ(-7.0, mam.minAABB.x);
        EXPECT_EQ(-5.0, mam.minAABB.y);
        EXPECT_EQ(-2.0, mam.minAABB.z);
        EXPECT_EQ(3.0, mam.maxAABB.x);
        EXPECT_EQ(5.0, mam.maxAABB.y);
        EXPECT_EQ(8.0, mam.maxAABB.z);
    }
    // tick one so that LocalMatrixComponents are up-to-date.
    IEcs* ecsArr[] = { &*ecs };
    engine->TickFrame(ecsArr);

    // remove TransfromComponents and check that GetTransformComponentAABB works with local matrices.
    auto* transformManager = GetManager<ITransformComponentManager>(*ecs);
    transformManager->Destroy(root->GetEntity());
    transformManager->Destroy(child1->GetEntity());
    transformManager->Destroy(child2->GetEntity());
    {
        MinAndMax mam = picking->GetTransformComponentAABB(root->GetEntity(), true, *ecs);
        EXPECT_EQ(-7.0, mam.minAABB.x);
        EXPECT_EQ(-5.0, mam.minAABB.y);
        EXPECT_EQ(-2.0, mam.minAABB.z);
        EXPECT_EQ(3.0, mam.maxAABB.x);
        EXPECT_EQ(5.0, mam.maxAABB.y);
        EXPECT_EQ(8.0, mam.maxAABB.z);
    }
}

/**
 * @tc.name: GetWorldMatrixComponentAABBTest
 * @tc.desc: Tests for Get World Matrix Component Aabbtest. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilPicking, GetWorldMatrixComponentAABBTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto picking = GetInstance<IPicking>(*renderContext->GetInterface<IClassRegister>(), UID_PICKING);
    ASSERT_NE(nullptr, picking);
    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);
    auto jointMatricesManager = GetManager<IJointMatricesComponentManager>(*ecs);
    ASSERT_NE(nullptr, jointMatricesManager);

    ISceneNode* root = nodeSystem->CreateNode();
    SetWorldMatrixFromPosition(root->GetEntity(), *ecs, Math::Vec3 { -5.0f, 0.0f, 5.0f });
    jointMatricesManager->Create(root->GetEntity());
    if (auto scopedHandle = jointMatricesManager->Write(root->GetEntity()); scopedHandle) {
        scopedHandle->jointsAabbMin = Math::Vec3 { -6.0f, -1.0f, 4.0f };
        scopedHandle->jointsAabbMax = Math::Vec3 { -3.0f, 2.0f, 7.0f };
    }

    ISceneNode* child1 = nodeSystem->CreateNode();
    SetWorldMatrixFromPosition(child1->GetEntity(), *ecs, Math::Vec3 { -5.0f, 5.0f, 0.0f });
    child1->SetParent(*root);
    SetAabb(child1->GetEntity(), *ecs, Math::Vec3 { -2.0f, -2.0f, -2.0f }, Math::Vec3 { 0.0f, 0.0f, 0.0f });

    ISceneNode* child2 = nodeSystem->CreateNode();
    SetWorldMatrixFromPosition(child2->GetEntity(), *ecs, Math::Vec3 { 0.0f, -5.0f, 5.0f });
    child2->SetParent(*root);
    SetAabb(child2->GetEntity(), *ecs, Math::Vec3 { 0.0f, 0.0f, 0.0f }, Math::Vec3 { 3.0f, 3.0f, 3.0f });

    MinAndMax mam = picking->GetWorldMatrixComponentAABB(root->GetEntity(), true, *ecs);
    EXPECT_EQ(-7.0, mam.minAABB.x);
    EXPECT_EQ(-5.0, mam.minAABB.y);
    EXPECT_EQ(-2.0, mam.minAABB.z);
    EXPECT_EQ(3.0, mam.maxAABB.x);
    EXPECT_EQ(5.0, mam.maxAABB.y);
    EXPECT_EQ(8.0, mam.maxAABB.z);
}

/**
 * @tc.name: RayCastTest
 * @tc.desc: Tests for Ray Cast Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilPicking, RayCastTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto picking = GetInstance<IPicking>(*renderContext->GetInterface<IClassRegister>(), UID_PICKING);
    ASSERT_NE(nullptr, picking);
    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);
    auto renderMeshManager = GetManager<IRenderMeshComponentManager>(*ecs);
    ASSERT_NE(nullptr, renderMeshManager);
    auto meshManager = GetManager<IMeshComponentManager>(*ecs);
    ASSERT_NE(nullptr, meshManager);

    // Perspective projection
    Entity camera = graphicsContext->GetSceneUtil().CreateCamera(*ecs, Math::Vec3(0.0f, 0.5f, 4.0f),
        Math::FromEulerRad(Math::Vec3 { 0.03f, 0.05f, 0.07f }), 2.0f, 100.0f, 75.0f);
    auto cube =
        nodeSystem->GetNode(graphicsContext->GetMeshUtil().GenerateCube(*ecs, "cube0", Entity {}, 3.0f, 3.0f, 3.0f));
    cube->SetPosition({ -1.0f, -2.0f, -5.0f });
    cube->SetRotation(Math::FromEulerRad(Math::Vec3 { 1.4f, 5.2f, 0.14f }));
    auto dummyNode = nodeSystem->CreateNode();
    renderMeshManager->Create(dummyNode->GetEntity());

    {
        auto result = picking->RayCastFromCamera(*ecs, camera, Math::Vec2 { 0.4f, 0.63f });
        ASSERT_EQ(1, result.size());
        EXPECT_EQ(cube, result[0].node);
    }

    // Orthographic projection
    auto cameraManager = GetManager<ICameraComponentManager>(*ecs);
    if (auto scopedHandle = cameraManager->Write(camera); scopedHandle) {
        scopedHandle->projection = CameraComponent::Projection::ORTHOGRAPHIC;
    }
    graphicsContext->GetSceneUtil().UpdateCameraViewport(
        *ecs, camera, Math::UVec2 { 512u, 512u }, true, Math::DEG2RAD * 75.0f, 10.0f);
    cube->SetPosition(Math::Vec3(0.0f, 0.5f, -5.0f));

    ecs->ProcessEvents();
    ecs->Update(1u, 1u);
    ecs->ProcessEvents();

    {
        auto result = picking->RayCastFromCamera(*ecs, camera, Math::Vec2 { 0.4f, 0.63f });
        ASSERT_EQ(1, result.size());
        EXPECT_EQ(cube, result[0].node);
    }

    // Ray cast multiple submeshes
    if (auto scopedHandle = meshManager->Write(renderMeshManager->Read(cube->GetEntity())->mesh); scopedHandle) {
        ASSERT_EQ(1, scopedHandle->submeshes.size());
        scopedHandle->submeshes.push_back(scopedHandle->submeshes[0]);
        Math::Vec3 shift { 0.03f, 0.04f, 0.01f };
        scopedHandle->submeshes[1].aabbMin += shift;
        scopedHandle->submeshes[1].aabbMax += shift;
    }

    {
        auto result = picking->RayCastFromCamera(*ecs, camera, Math::Vec2 { 0.4f, 0.63f });
        ASSERT_EQ(1, result.size());
        EXPECT_EQ(cube, result[0].node);
    }

    // Ray cast world position
    {
        auto result = picking->RayCastFromCamera(*ecs, camera, Math::Vec2 { 0.4f, 0.63f });
        ASSERT_EQ(1, result.size());
        EXPECT_EQ(cube, result[0].node);
        EXPECT_NE(0.0f, result[0].worldPosition.x);
        EXPECT_NE(0.0f, result[0].worldPosition.y);
        EXPECT_NE(0.0f, result[0].worldPosition.z);

        // Ray cast hit position (world) projected back to screen should match ray origin (screen)
        const Math::Vec3 screenCoords = picking->WorldToScreen(*ecs, camera, result[0].worldPosition);
        EXPECT_FLOAT_EQ(screenCoords.x, 0.4f);
        EXPECT_FLOAT_EQ(screenCoords.y, 0.63f);
    }

    // Ray cast to joints
    Math::Vec3 aabbMin;
    Math::Vec3 aabbMax;
    if (auto scopedHandle = meshManager->Read(renderMeshManager->Read(cube->GetEntity())->mesh); scopedHandle) {
        ASSERT_EQ(2, scopedHandle->submeshes.size());
        aabbMin = scopedHandle->aabbMin;
        aabbMax = scopedHandle->aabbMax;
    }

    auto jointMatricesManager = GetManager<IJointMatricesComponentManager>(*ecs);
    jointMatricesManager->Create(cube->GetEntity());
    if (auto scopedHandle = jointMatricesManager->Write(cube->GetEntity()); scopedHandle) {
        scopedHandle->jointsAabbMin = aabbMin;
        scopedHandle->jointsAabbMax = aabbMax;
    }

    {
        auto result = picking->RayCastFromCamera(*ecs, camera, Math::Vec2 { 0.4f, 0.63f });
        ASSERT_EQ(1, result.size());
        EXPECT_EQ(cube, result[0].node);
        EXPECT_EQ(0, picking->RayCastFromCamera(*ecs, Entity {}, Math::Vec2 { 0.0f, 0.0f }).size());
    }

    // Layer mask
    {
        // Should find cube with default mask although it doesn't have a LayerComponent
        auto result =
            picking->RayCastFromCamera(*ecs, camera, Math::Vec2 { 0.4f, 0.63f }, LayerConstants::DEFAULT_LAYER_MASK);
        ASSERT_EQ(1, result.size());
        EXPECT_EQ(cube, result[0].node);
    }
    {
        // Shouldn't find cube with non-default mask as it doesn't have a LayerComponent
        auto result =
            picking->RayCastFromCamera(*ecs, camera, Math::Vec2 { 0.4f, 0.63f }, ~LayerConstants::DEFAULT_LAYER_MASK);
        EXPECT_EQ(0, result.size());
    }

    auto layerManager = GetManager<ILayerComponentManager>(*ecs);
    ASSERT_NE(nullptr, layerManager);
    layerManager->Create(cube->GetEntity());
    layerManager->Write(cube->GetEntity())->layerMask =
        LayerFlagBits::CORE_LAYER_FLAG_BIT_04 | LayerFlagBits::CORE_LAYER_FLAG_BIT_02;

    {
        // Should find cube when mask includes something the cube has
        auto result =
            picking->RayCastFromCamera(*ecs, camera, Math::Vec2 { 0.4f, 0.63f }, LayerFlagBits::CORE_LAYER_FLAG_BIT_04);
        ASSERT_EQ(1, result.size());
        EXPECT_EQ(cube, result[0].node);
    }
    {
        // Shouldn't find cube with mask which doesn't overlap cube's layer mask
        auto result =
            picking->RayCastFromCamera(*ecs, camera, Math::Vec2 { 0.4f, 0.63f }, LayerFlagBits::CORE_LAYER_FLAG_BIT_01);
        EXPECT_EQ(0, result.size());
    }
}

/**
 * @tc.name: RayTriangleCastTest
 * @tc.desc: Tests for Ray Triangle Cast Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilPicking, RayTriangleCastTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto picking = GetInstance<IPicking>(*renderContext->GetInterface<IClassRegister>(), UID_PICKING);
    ASSERT_NE(nullptr, picking);
    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);
    auto renderMeshManager = GetManager<IRenderMeshComponentManager>(*ecs);
    ASSERT_NE(nullptr, renderMeshManager);
    auto meshManager = GetManager<IMeshComponentManager>(*ecs);
    ASSERT_NE(nullptr, meshManager);

    // Perspective projection
    Entity camera = graphicsContext->GetSceneUtil().CreateCamera(*ecs, Math::Vec3(0.0f, 0.5f, 4.0f),
        Math::FromEulerRad(Math::Vec3 { 0.03f, 0.05f, 0.07f }), 2.0f, 100.0f, 75.0f);

    // Orthographic projection
    auto cameraManager = GetManager<ICameraComponentManager>(*ecs);
    if (auto scopedHandle = cameraManager->Write(camera); scopedHandle) {
        scopedHandle->projection = CameraComponent::Projection::ORTHOGRAPHIC;
    }
    graphicsContext->GetSceneUtil().UpdateCameraViewport(
        *ecs, camera, Math::UVec2 { 512u, 512u }, true, Math::DEG2RAD * 75.0f, 10.0f);

    ecs->ProcessEvents();
    ecs->Update(1u, 1u);
    ecs->ProcessEvents();

    // Ray cast against a CCw and a CC triangle, expect to not hit the CC triangle.
    {
        // the 1st triangle has CCw winding, the 2nd one CC, only the 1st triangle should be hit.
        const BASE_NS::vector<Math::Vec3> triangles = { Math::Vec3(2, 2, 0), Math::Vec3(-2, 2, 0),
            Math::Vec3(-2, -2, 0), Math::Vec3(-2, -2, 0), Math::Vec3(-2, 2, 0), Math::Vec3(2, 2, 0) };

        auto result = picking->RayCastFromCamera(*ecs, camera, Math::Vec2 { 0.6f, 0.4f }, triangles);
        ASSERT_EQ(1, result.size());
        EXPECT_NE(0, result[0].distance);

        EXPECT_EQ(0, result[0].triangleIndex);

        const Math::Vec3 screenCoords = picking->WorldToScreen(*ecs, camera, result[0].worldPosition);
        EXPECT_FLOAT_EQ(screenCoords.x, 0.6f);
        EXPECT_FLOAT_EQ(screenCoords.y, 0.4f);

        // expect the worldPosition coords to be within the triangle min-maxs
        const auto pos = result[0].worldPosition;
        EXPECT_TRUE(pos.x < 2 && pos.x > -2);
        EXPECT_TRUE(pos.y < 2 && pos.y > -2);
        EXPECT_NEAR(0, pos.z, 0.0001f);
    }

    // Ray cast against multiple triangles, expect to hit the first 2 triangles.
    {
        const BASE_NS::vector<Math::Vec3> triangles = { Math::Vec3(2, 2, 0), Math::Vec3(-2, 2, 0),
            Math::Vec3(-2, -2, 0), Math::Vec3(2, 2, -1), Math::Vec3(-2, 2, -1), Math::Vec3(-2, -2, -1),
            Math::Vec3(2, 4, -1), Math::Vec3(-2, 4, -1), Math::Vec3(-2, 0, -1) };

        auto result = picking->RayCastFromCamera(*ecs, camera, Math::Vec2 { 0.6f, 0.4f }, triangles);
        ASSERT_EQ(2, result.size());
        EXPECT_NE(0, result[0].distance);

        EXPECT_EQ(0, result[0].triangleIndex);
        EXPECT_EQ(1, result[1].triangleIndex);

        const Math::Vec3 screenCoords = picking->WorldToScreen(*ecs, camera, result[0].worldPosition);
        EXPECT_FLOAT_EQ(screenCoords.x, 0.6f);
        EXPECT_FLOAT_EQ(screenCoords.y, 0.4f);

        // expect the worldPosition coords to be within the triangle min-maxs
        const auto pos = result[0].worldPosition;
        EXPECT_TRUE(pos.x < 2 && pos.x > -2);
        EXPECT_TRUE(pos.y < 2 && pos.y > -2);
        EXPECT_NEAR(0, pos.z, 0.0001f);
    }
}

/**
 * @tc.name: WorldAndScreenCoordinatesTest
 * @tc.desc: Tests for World And Screen Coordinates Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilPicking, WorldAndScreenCoordinatesTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto picking = GetInstance<IPicking>(*renderContext->GetInterface<IClassRegister>(), UID_PICKING);
    ASSERT_NE(nullptr, picking);

    Entity camera = graphicsContext->GetSceneUtil().CreateCamera(*ecs, Math::Vec3(0.0f, 0.5f, 4.0f),
        Math::FromEulerRad(Math::Vec3 { 0.03f, 0.05f, 0.07f }), 2.0f, 100.0f, 75.0f);

    constexpr const float eps = 1e-6f;
    {
        Math::Vec3 worldCoords { -0.1f, 0.6f, -4.0f };
        Math::Vec3 screenCoords = picking->WorldToScreen(*ecs, camera, worldCoords);
        Math::Vec3 result = picking->ScreenToWorld(*ecs, camera, screenCoords);
        for (uint32_t i = 0; i < 3; ++i) {
            EXPECT_LE(worldCoords[i] - eps, result[i]);
            EXPECT_GE(worldCoords[i] + eps, result[i]);
        }
    }
    {
        Math::Vec3 screenCoords { -0.1f, 0.3f, -0.25f };
        Math::Vec3 worldCoords = picking->ScreenToWorld(*ecs, camera, screenCoords);
        Math::Vec3 result = picking->WorldToScreen(*ecs, camera, worldCoords);
        for (uint32_t i = 0; i < 3; ++i) {
            EXPECT_LE(screenCoords[i] - eps, result[i]);
            EXPECT_GE(screenCoords[i] + eps, result[i]);
        }
    }
    {
        EXPECT_EQ(Math::Vec3 {}, picking->WorldToScreen(*ecs, Entity {}, Math::Vec3 {}));
        EXPECT_EQ(Math::Vec3 {}, picking->ScreenToWorld(*ecs, Entity {}, Math::Vec3 {}));
    }
}
