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

#include <3d/ecs/components/joint_matrices_component.h>
#include <3d/ecs/components/skin_component.h>
#include <3d/ecs/components/skin_ibm_component.h>
#include <3d/ecs/components/skin_joints_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_skinning_system.h>
#include <3d/util/intf_mesh_util.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/datastore/intf_render_data_store_manager.h>

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
 * @tc.name: CreateDestroyInstanceTest
 * @tc.desc: Tests for Create Destroy Instance Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsSkinningSystem, CreateDestroyInstanceTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto skinningSystem = GetSystem<ISkinningSystem>(*ecs);
    ASSERT_NE(nullptr, skinningSystem);
    skinningSystem->SetActive(true);
    EXPECT_TRUE(skinningSystem->IsActive());
    EXPECT_EQ("SkinningSystem", skinningSystem->GetName());
    EXPECT_EQ(ISkinningSystem::UID, ((ISystem*)skinningSystem)->GetUid());
    EXPECT_NE(nullptr, skinningSystem->GetProperties());
    EXPECT_NE(nullptr, ((const ISkinningSystem*)skinningSystem)->GetProperties());
    EXPECT_EQ(ecs.get(), &skinningSystem->GetECS());

    auto skinManager = GetManager<ISkinComponentManager>(*ecs);
    ASSERT_NE(nullptr, skinManager);
    auto skinJointsManager = GetManager<ISkinJointsComponentManager>(*ecs);
    ASSERT_NE(nullptr, skinJointsManager);
    auto skinIbmManager = GetManager<ISkinIbmComponentManager>(*ecs);
    ASSERT_NE(nullptr, skinIbmManager);
    auto jointMatricesManager = GetManager<IJointMatricesComponentManager>(*ecs);
    ASSERT_NE(nullptr, jointMatricesManager);

    constexpr const uint32_t numJoints = 16u;

    Entity entity = ecs->GetEntityManager().Create();
    Entity skeleton = ecs->GetEntityManager().Create();
    Entity skinIbm = ecs->GetEntityManager().Create();
    Entity joint = ecs->GetEntityManager().Create();
    skinIbmManager->Create(skinIbm);
    if (auto scopedHandle = skinIbmManager->Write(skinIbm); scopedHandle) {
        scopedHandle->matrices.resize(numJoints);
    }
    skinJointsManager->Create(skinIbm);
    if (auto scopedHandle = skinJointsManager->Write(skinIbm); scopedHandle) {
        scopedHandle->count = numJoints;
        for (uint32_t i = 0; i < numJoints; ++i) {
            scopedHandle->jointEntities[i] = joint;
        }
    }

    skinningSystem->CreateInstance(skinIbm, entity, skeleton);
    EXPECT_TRUE(skinManager->HasComponent(entity));
    EXPECT_TRUE(jointMatricesManager->HasComponent(entity));
    if (auto scopedHandle = skinJointsManager->Read(entity); scopedHandle) {
        EXPECT_EQ(numJoints, scopedHandle->count);
    }
    skinningSystem->DestroyInstance(entity);
    EXPECT_FALSE(skinManager->HasComponent(entity));
    EXPECT_FALSE(jointMatricesManager->HasComponent(entity));
    EXPECT_FALSE(skinJointsManager->HasComponent(entity));
}

/**
 * @tc.name: CreateInvalidInstanceTest
 * @tc.desc: Tests for Create Invalid Instance Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsSkinningSystem, CreateInvalidInstanceTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto skinningSystem = GetSystem<ISkinningSystem>(*ecs);
    ASSERT_NE(nullptr, skinningSystem);
    skinningSystem->SetActive(true);
    EXPECT_TRUE(skinningSystem->IsActive());
    EXPECT_EQ("SkinningSystem", skinningSystem->GetName());
    EXPECT_EQ(ISkinningSystem::UID, ((ISystem*)skinningSystem)->GetUid());
    EXPECT_NE(nullptr, skinningSystem->GetProperties());
    EXPECT_NE(nullptr, ((const ISkinningSystem*)skinningSystem)->GetProperties());
    EXPECT_EQ(ecs.get(), &skinningSystem->GetECS());

    auto skinManager = GetManager<ISkinComponentManager>(*ecs);
    ASSERT_NE(nullptr, skinManager);
    auto skinJointsManager = GetManager<ISkinJointsComponentManager>(*ecs);
    ASSERT_NE(nullptr, skinJointsManager);
    auto skinIbmManager = GetManager<ISkinIbmComponentManager>(*ecs);
    ASSERT_NE(nullptr, skinIbmManager);
    auto jointMatricesManager = GetManager<IJointMatricesComponentManager>(*ecs);
    ASSERT_NE(nullptr, jointMatricesManager);

    constexpr const uint32_t numJoints = 16u;

    Entity entity = ecs->GetEntityManager().Create();
    Entity skeleton = ecs->GetEntityManager().Create();
    Entity skinIbm = ecs->GetEntityManager().Create();
    Entity joint = ecs->GetEntityManager().Create();
    skinIbmManager->Create(skinIbm);
    if (auto scopedHandle = skinIbmManager->Write(skinIbm); scopedHandle) {
        scopedHandle->matrices.resize(numJoints + 1);
    }
    skinJointsManager->Create(skinIbm);
    if (auto scopedHandle = skinJointsManager->Write(skinIbm); scopedHandle) {
        scopedHandle->count = numJoints;
        for (uint32_t i = 0; i < numJoints; ++i) {
            scopedHandle->jointEntities[i] = joint;
        }
    }

    skinningSystem->CreateInstance(skinIbm, entity, skeleton);
    EXPECT_FALSE(skinManager->HasComponent(entity));
    EXPECT_FALSE(jointMatricesManager->HasComponent(entity));
    EXPECT_FALSE(skinJointsManager->HasComponent(entity));

    Entity joints[numJoints];
    for (uint32_t i = 0; i < numJoints; ++i) {
        joints[i] = joint;
    }
    skinningSystem->CreateInstance(skinIbm, { joints, countof(joints) }, entity, skeleton);
    EXPECT_FALSE(skinManager->HasComponent(entity));
    EXPECT_FALSE(jointMatricesManager->HasComponent(entity));
    EXPECT_FALSE(skinJointsManager->HasComponent(entity));
}

/**
 * @tc.name: SkinTasksTest
 * @tc.desc: Tests for Skin Tasks Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsSkinningSystem, SkinTasksTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto skinningSystem = GetSystem<ISkinningSystem>(*ecs);
    ASSERT_NE(nullptr, skinningSystem);
    skinningSystem->SetActive(true);
    EXPECT_TRUE(skinningSystem->IsActive());
    EXPECT_EQ("SkinningSystem", skinningSystem->GetName());
    EXPECT_EQ(ISkinningSystem::UID, ((ISystem*)skinningSystem)->GetUid());
    EXPECT_NE(nullptr, skinningSystem->GetProperties());
    EXPECT_NE(nullptr, ((const ISkinningSystem*)skinningSystem)->GetProperties());
    EXPECT_EQ(ecs.get(), &skinningSystem->GetECS());

    auto skinManager = GetManager<ISkinComponentManager>(*ecs);
    ASSERT_NE(nullptr, skinManager);
    auto skinJointsManager = GetManager<ISkinJointsComponentManager>(*ecs);
    ASSERT_NE(nullptr, skinJointsManager);
    auto skinIbmManager = GetManager<ISkinIbmComponentManager>(*ecs);
    ASSERT_NE(nullptr, skinIbmManager);
    auto jointMatricesManager = GetManager<IJointMatricesComponentManager>(*ecs);
    ASSERT_NE(nullptr, jointMatricesManager);
    auto worldMatrixManager = GetManager<IWorldMatrixComponentManager>(*ecs);
    ASSERT_NE(nullptr, worldMatrixManager);

    constexpr const uint32_t numSkins = 17u;

    vector<Entity> entities;
    const Math::Mat4X4 matrix = Math::Trs(Math::Vec3 { 1.0f, 2.0f, 1.0f },
        Math::FromEulerRad(Math::Vec3(0.4f, 0.1f, 0.5f)), Math::Vec3 { 1.0f, 1.0f, 1.0f });
    for (uint32_t i = 0; i < numSkins; ++i) {
        Entity entity = ecs->GetEntityManager().Create();
        Entity skeleton = ecs->GetEntityManager().Create();

        Entity joint = ecs->GetEntityManager().Create();
        worldMatrixManager->Create(joint);
        if (auto scopedHandle = worldMatrixManager->Write(joint); scopedHandle) {
            scopedHandle->matrix = matrix;
        }

        Entity skinIbm = ecs->GetEntityManager().Create();
        skinIbmManager->Create(skinIbm);
        if (auto scopedHandle = skinIbmManager->Write(skinIbm); scopedHandle) {
            scopedHandle->matrices.push_back(Math::Mat4X4 { 1.0f });
        }
        skinJointsManager->Create(skinIbm);
        if (auto scopedHandle = skinJointsManager->Write(skinIbm); scopedHandle) {
            scopedHandle->count = 1;
            scopedHandle->jointEntities[0] = joint;
        }

        skinningSystem->CreateInstance(skinIbm, entity, skeleton);
        EXPECT_TRUE(skinManager->HasComponent(entity));
        EXPECT_TRUE(jointMatricesManager->HasComponent(entity));
        EXPECT_TRUE(skinJointsManager->HasComponent(entity));
        entities.push_back(entity);
    }

    skinningSystem->Update(true, 1u, 1u);

    for (Entity entity : entities) {
        if (auto scopedHandle = jointMatricesManager->Read(entity); scopedHandle) {
            ASSERT_EQ(1u, scopedHandle->count);
            for (uint32_t i = 0; i < 16u; ++i) {
                EXPECT_EQ(matrix.data[i], scopedHandle->jointMatrices[0].data[i]);
            }
        }
    }
}
