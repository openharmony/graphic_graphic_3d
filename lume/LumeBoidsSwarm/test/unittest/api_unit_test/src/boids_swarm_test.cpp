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

#include <boids_swarm/ecs/components/boids_swarm_component.h>
#include <boids_swarm/ecs/components/boids_swarm_state_component.h>
#include <boids_swarm/ecs/systems/intf_boids_swarm_system.h>
#include <boids_swarm/implementation_uids.h>

#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/intf_graphics_context.h>
#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/math/vector_util.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/plugin/intf_class_register.h>
#include <core/property/intf_property_api.h>
#include <render/intf_render_context.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

namespace BoidSwarm {
namespace UTest {

class API_BoidsSwarmSystemTest : public ::testing::Test {
    void SetUp() override
    {}

    void TearDown() override
    {}

protected:
    BOIDSSWARM_NS::IBoidsSwarmSystem* GetBoidsSwarmSystem(const CORE_NS::IEcs& ecs)
    {
        return CORE_NS::GetSystem<BOIDSSWARM_NS::IBoidsSwarmSystem>(ecs);
    }

    CORE_NS::Entity CreateSwarmEntity(
        CORE_NS::IEcs& ecs, CORE3D_NS::INodeSystem*& outNodeSystem, CORE_NS::IComponentManager*& outCm)
    {
        outNodeSystem = CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(ecs);
        CORE_NS::Entity entity = outNodeSystem->CreateNode()->GetEntity();
        outCm = ecs.GetComponentManager(BOIDSSWARM_NS::IBoidsSwarmComponentManager::UID);
        outCm->Create(entity);
        return entity;
    }

    void SetupSwarmForTick(CORE_NS::IEcs& ecs, CORE_NS::Entity entity, CORE_NS::IComponentManager* cm)
    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        data->initialVelocity = {0.01f, 0.0f, 0.01f};
        data->boundaryMinPos = {-10.0f, 0.0f, -10.0f};
        data->boundaryMaxPos = {10.0f, 10.0f, 10.0f};
        data->separationWeight = 1.5f;
        data->separationDistance = 1.0f;
        data->alignmentWeight = 1.0f;
        data->alignmentDistance = 3.0f;
        data->cohesionWeight = 0.8f;
        data->cohesionDistance = 4.0f;
    }

    void TickOnce(CORE_NS::IEngine* engine, CORE_NS::IEcs* ecs)
    {
        auto* ecsRawPtr = ecs;
        engine->TickFrame(BASE_NS::array_view(&ecsRawPtr, 1));
    }
};

// ============================================================================
// IBoidsSwarmSystem interface tests
// ============================================================================

/**
 * @tc.name: IBoidsSwarmSystem1
 * @tc.desc: Verify GetName returns the correct system name.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem1, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    EXPECT_EQ(system->GetName(), "BoidsSwarmSystem");
}

/**
 * @tc.name: IBoidsSwarmSystem2
 * @tc.desc: Verify GetUid returns the correct system UID.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem2, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    EXPECT_EQ(system->GetUid(), BOIDSSWARM_NS::IBoidsSwarmSystem::UID);
}

/**
 * @tc.name: IBoidsSwarmSystem3
 * @tc.desc: Verify GetProperties returns nullptr.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem3, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    CORE_NS::IPropertyHandle* properties = system->GetProperties();
    EXPECT_FALSE(properties);
}

/**
 * @tc.name: IBoidsSwarmSystem4
 * @tc.desc: Verify default state is Playing (active).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem4, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    EXPECT_TRUE(system->IsPlaying());
}

/**
 * @tc.name: IBoidsSwarmSystem5
 * @tc.desc: Verify Pause changes state to not playing.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem5, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    system->Pause();
    EXPECT_FALSE(system->IsPlaying());
}

/**
 * @tc.name: IBoidsSwarmSystem6
 * @tc.desc: Verify Play changes state to playing.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem6, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    system->Pause();
    system->Play();
    EXPECT_TRUE(system->IsPlaying());
}

/**
 * @tc.name: IBoidsSwarmSystem7
 * @tc.desc: Verify Stop changes state to not playing.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem7, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    system->Stop();
    EXPECT_FALSE(system->IsPlaying());
}

/**
 * @tc.name: IBoidsSwarmSystem8
 * @tc.desc: Verify GetTimeStepSec returns default value.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem8, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    EXPECT_FLOAT_EQ(system->GetTimeStepSec(), BOIDSSWARM_NS::IBoidsSwarmSystem::DEFAULT_TIME_STEP_SEC);
}

/**
 * @tc.name: IBoidsSwarmSystem9
 * @tc.desc: Verify SetTimeStepSec persists the new value.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem9, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    constexpr float newTimeStep = 0.032f;
    system->SetTimeStepSec(newTimeStep);
    EXPECT_FLOAT_EQ(system->GetTimeStepSec(), newTimeStep);
}

/**
 * @tc.name: IBoidsSwarmSystem10
 * @tc.desc: Verify GetPlaySpeed returns default value.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem10, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    EXPECT_FLOAT_EQ(system->GetPlaySpeed(), BOIDSSWARM_NS::IBoidsSwarmSystem::DEFAULT_PLAY_SPEED);
}

/**
 * @tc.name: IBoidsSwarmSystem11
 * @tc.desc: Verify SetPlaySpeed persists a custom value.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem11, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    constexpr float newSpeed = 2.0f;
    system->SetPlaySpeed(newSpeed);
    EXPECT_FLOAT_EQ(system->GetPlaySpeed(), newSpeed);
}

/**
 * @tc.name: IBoidsSwarmSystem12
 * @tc.desc: Verify SetPlaySpeed accepts minimum boundary value.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem12, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    system->SetPlaySpeed(BOIDSSWARM_NS::IBoidsSwarmSystem::MIN_PLAY_SPEED);
    EXPECT_FLOAT_EQ(system->GetPlaySpeed(), BOIDSSWARM_NS::IBoidsSwarmSystem::MIN_PLAY_SPEED);
}

/**
 * @tc.name: IBoidsSwarmSystem13
 * @tc.desc: Verify SetPlaySpeed accepts maximum boundary value.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem13, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    system->SetPlaySpeed(BOIDSSWARM_NS::IBoidsSwarmSystem::MAX_PLAY_SPEED);
    EXPECT_FLOAT_EQ(system->GetPlaySpeed(), BOIDSSWARM_NS::IBoidsSwarmSystem::MAX_PLAY_SPEED);
}

/**
 * @tc.name: IBoidsSwarmSystem14
 * @tc.desc: Verify GetAxisMask returns default value (XZ plane).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem14, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    BASE_NS::Math::IVec3 defaultMask = BOIDSSWARM_NS::IBoidsSwarmSystem::DEFAULT_AXIS_MASK;
    BASE_NS::Math::IVec3 currentMask = system->GetAxisMask();
    EXPECT_EQ(currentMask.x, defaultMask.x);
}

/**
 * @tc.name: IBoidsSwarmSystem15
 * @tc.desc: Verify SetAxisMask persists all-axes value.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem15, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    constexpr BASE_NS::Math::IVec3 allAxes{1, 1, 1};
    system->SetAxisMask(allAxes);
    BASE_NS::Math::IVec3 currentMask = system->GetAxisMask();
    EXPECT_EQ(currentMask.x, 1);
    EXPECT_EQ(currentMask.y, 1);
    EXPECT_EQ(currentMask.z, 1);
}

/**
 * @tc.name: IBoidsSwarmSystem16
 * @tc.desc: Verify SetAxisMask persists single-axis value.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem16, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    constexpr BASE_NS::Math::IVec3 xAxisOnly{1, 0, 0};
    system->SetAxisMask(xAxisOnly);
    BASE_NS::Math::IVec3 currentMask = system->GetAxisMask();
    EXPECT_EQ(currentMask.x, 1);
    EXPECT_EQ(currentMask.y, 0);
    EXPECT_EQ(currentMask.z, 0);
}

/**
 * @tc.name: IBoidsSwarmSystem19
 * @tc.desc: Verify GetVelocitySmoothingFactor returns default value.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem19, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    EXPECT_FLOAT_EQ(
        system->GetVelocitySmoothingFactor(), BOIDSSWARM_NS::IBoidsSwarmSystem::DEFAULT_VELOCITY_SMOOTHING_FACTOR);
}

/**
 * @tc.name: IBoidsSwarmSystem20
 * @tc.desc: Verify SetVelocitySmoothingFactor persists the new value.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem20, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;
    const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

    BOIDSSWARM_NS::IBoidsSwarmSystem* system = GetBoidsSwarmSystem(ecsRef);
    ASSERT_TRUE(system);

    constexpr float newFactor = 0.8f;
    system->SetVelocitySmoothingFactor(newFactor);
    EXPECT_FLOAT_EQ(system->GetVelocitySmoothingFactor(), newFactor);
}

/**
 * @tc.name: IBoidsSwarmSystem21
 * @tc.desc: Verify first TickFrame triggers Reset and returns true (need render).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem21, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto engine = testContext->engine;
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    SetupSwarmForTick(*ecsContext, entity, cm);

    auto* ecsRawPtr = ecsContext.get();
    const bool needRender = engine->TickFrame(BASE_NS::array_view(&ecsRawPtr, 1));
    EXPECT_TRUE(needRender);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: IBoidsSwarmSystem22
 * @tc.desc: Verify first TickFrame resets state velocity to initialVelocity magnitude.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem22, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto engine = testContext->engine;
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    SetupSwarmForTick(*ecsContext, entity, cm);

    constexpr BASE_NS::Math::Vec3 initVel{0.01f, 0.0f, 0.01f};
    const float expectedMag = BASE_NS::Math::Magnitude(initVel);

    TickOnce(engine.get(), ecsContext.get());

    CORE_NS::IComponentManager* stateManager =
        ecsContext->GetComponentManager(BOIDSSWARM_NS::IBoidsSwarmStateComponentManager::UID);
    ASSERT_TRUE(stateManager);

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmStateComponent> stateData(stateManager->GetData(entity));
    ASSERT_TRUE(stateData);
    EXPECT_FLOAT_EQ(stateData->velocityMag, expectedMag);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: IBoidsSwarmSystem23
 * @tc.desc: Verify second TickFrame causes simulation forces to modify velocity.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, IBoidsSwarmSystem23, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto engine = testContext->engine;
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    SetupSwarmForTick(*ecsContext, entity, cm);

    constexpr BASE_NS::Math::Vec3 initVel{0.1f, 0.0f, 0.1f};
    const float initMag = BASE_NS::Math::Magnitude(initVel);

    TickOnce(engine.get(), ecsContext.get());
    TickOnce(engine.get(), ecsContext.get());

    CORE_NS::IComponentManager* stateManager =
        ecsContext->GetComponentManager(BOIDSSWARM_NS::IBoidsSwarmStateComponentManager::UID);
    ASSERT_TRUE(stateManager);

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmStateComponent> stateData(stateManager->GetData(entity));
    ASSERT_TRUE(stateData);
    EXPECT_NE(stateData->velocityMag, initMag);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

// ============================================================================
// BoidsSwarmComponent property read/write tests
// ============================================================================

/**
 * @tc.name: BoidsSwarmComponent1
 * @tc.desc: Verify ComponentManager can be obtained.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent1, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE_NS::IComponentManager* cm = ecsContext->GetComponentManager(BOIDSSWARM_NS::IBoidsSwarmComponentManager::UID);
    EXPECT_TRUE(cm);
}

/**
 * @tc.name: BoidsSwarmComponent2
 * @tc.desc: Verify component data is accessible after creation.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent2, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
    EXPECT_TRUE(data);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent3
 * @tc.desc: Verify initialVelocity round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent3, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->initialVelocity = {0.01f, 0.0f, 0.01f};
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->initialVelocity.x, 0.01f);
    EXPECT_FLOAT_EQ(read->initialVelocity.y, 0.0f);
    EXPECT_FLOAT_EQ(read->initialVelocity.z, 0.01f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent4
 * @tc.desc: Verify initialPosition round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent4, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->initialPosition = {1.0f, 2.0f, 3.0f};
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->initialPosition.x, 1.0f);
    EXPECT_FLOAT_EQ(read->initialPosition.y, 2.0f);
    EXPECT_FLOAT_EQ(read->initialPosition.z, 3.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent5
 * @tc.desc: Verify initialRotation round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent5, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->initialRotation = {0.0f, 0.7f, 0.0f, 0.7f};
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->initialRotation.x, 0.0f);
    EXPECT_FLOAT_EQ(read->initialRotation.y, 0.7f);
    EXPECT_FLOAT_EQ(read->initialRotation.z, 0.0f);
    EXPECT_FLOAT_EQ(read->initialRotation.w, 0.7f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent6
 * @tc.desc: Verify boundaryMinPos round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent6, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->boundaryMinPos = {-5.0f, 0.0f, -5.0f};
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->boundaryMinPos.x, -5.0f);
    EXPECT_FLOAT_EQ(read->boundaryMinPos.y, 0.0f);
    EXPECT_FLOAT_EQ(read->boundaryMinPos.z, -5.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent7
 * @tc.desc: Verify boundaryMaxPos round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent7, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->boundaryMaxPos = {5.0f, 10.0f, 5.0f};
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->boundaryMaxPos.x, 5.0f);
    EXPECT_FLOAT_EQ(read->boundaryMaxPos.y, 10.0f);
    EXPECT_FLOAT_EQ(read->boundaryMaxPos.z, 5.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent8
 * @tc.desc: Verify maxVelocityMag round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent8, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->maxVelocityMag = 1.0f;
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->maxVelocityMag, 1.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent9
 * @tc.desc: Verify maxAccelerationMag round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent9, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->maxAccelerationMag = 50.0f;
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->maxAccelerationMag, 50.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent10
 * @tc.desc: Verify maxTurnRate round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent10, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->maxTurnRate = {0.1f, 0.2f, 0.3f};
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->maxTurnRate.x, 0.1f);
    EXPECT_FLOAT_EQ(read->maxTurnRate.y, 0.2f);
    EXPECT_FLOAT_EQ(read->maxTurnRate.z, 0.3f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent11
 * @tc.desc: Verify separationWeight positive round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent11, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->separationWeight = 1.5f;
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->separationWeight, 1.5f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent12
 * @tc.desc: Verify separationDistance positive round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent12, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->separationDistance = 0.5f;
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->separationDistance, 0.5f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent13
 * @tc.desc: Verify alignmentWeight positive round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent13, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->alignmentWeight = 1.0f;
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->alignmentWeight, 1.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent14
 * @tc.desc: Verify alignmentDistance positive round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent14, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->alignmentDistance = 2.0f;
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->alignmentDistance, 2.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent15
 * @tc.desc: Verify cohesionWeight positive round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent15, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->cohesionWeight = 0.8f;
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->cohesionWeight, 0.8f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent16
 * @tc.desc: Verify cohesionDistance positive round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent16, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->cohesionDistance = 3.0f;
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->cohesionDistance, 3.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent17
 * @tc.desc: Verify boundaryWeight positive round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent17, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->boundaryWeight = 2.0f;
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->boundaryWeight, 2.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent18
 * @tc.desc: Verify boundaryDistance positive round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent18, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->boundaryDistance = 1.0f;
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->boundaryDistance, 1.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent19
 * @tc.desc: Verify gravityWeight positive round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent19, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->gravityWeight = 0.5f;
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->gravityWeight, 0.5f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent20
 * @tc.desc: Verify repulsionWeight positive round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent20, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->repulsionWeight = 0.3f;
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->repulsionWeight, 0.3f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent21
 * @tc.desc: Verify drivenForce round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent21, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->drivenForce = {0.0f, -0.001f, 0.0f};
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->drivenForce.x, 0.0f);
    EXPECT_FLOAT_EQ(read->drivenForce.y, -0.001f);
    EXPECT_FLOAT_EQ(read->drivenForce.z, 0.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent22
 * @tc.desc: Verify drivenForceWeight positive round-trip.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent22, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->drivenForceWeight = 1.0f;
    }

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->drivenForceWeight, 1.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

// ============================================================================
// BoidsSwarmComponent HAS_MIN boundary validation tests
// ============================================================================

/**
 * @tc.name: BoidsSwarmComponent24
 * @tc.desc: Verify negative separationWeight is clamped to 0.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent24, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->separationWeight = -1.0f;
    }
    ecsContext->ProcessEvents();

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->separationWeight, 0.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent25
 * @tc.desc: Verify negative separationDistance is clamped to 0.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent25, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->separationDistance = -2.0f;
    }
    ecsContext->ProcessEvents();

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->separationDistance, 0.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent26
 * @tc.desc: Verify negative alignmentWeight is clamped to 0.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent26, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->alignmentWeight = -1.0f;
    }
    ecsContext->ProcessEvents();

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->alignmentWeight, 0.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent27
 * @tc.desc: Verify negative alignmentDistance is clamped to 0.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent27, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->alignmentDistance = -2.0f;
    }
    ecsContext->ProcessEvents();

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->alignmentDistance, 0.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent28
 * @tc.desc: Verify negative cohesionWeight is clamped to 0.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent28, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->cohesionWeight = -1.0f;
    }
    ecsContext->ProcessEvents();

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->cohesionWeight, 0.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent29
 * @tc.desc: Verify negative cohesionDistance is clamped to 0.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent29, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->cohesionDistance = -2.0f;
    }
    ecsContext->ProcessEvents();

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->cohesionDistance, 0.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent30
 * @tc.desc: Verify negative boundaryWeight is clamped to 0.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent30, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->boundaryWeight = -1.0f;
    }
    ecsContext->ProcessEvents();

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->boundaryWeight, 0.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent31
 * @tc.desc: Verify negative boundaryDistance is clamped to 0.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent31, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->boundaryDistance = -2.0f;
    }
    ecsContext->ProcessEvents();

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->boundaryDistance, 0.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent32
 * @tc.desc: Verify negative gravityWeight is clamped to 0.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent32, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->gravityWeight = -1.0f;
    }
    ecsContext->ProcessEvents();

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->gravityWeight, 0.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent33
 * @tc.desc: Verify negative repulsionWeight is clamped to 0.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent33, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->repulsionWeight = -1.0f;
    }
    ecsContext->ProcessEvents();

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->repulsionWeight, 0.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent34
 * @tc.desc: Verify negative drivenForceWeight is clamped to 0.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent34, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->drivenForceWeight = -1.0f;
    }
    ecsContext->ProcessEvents();

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->drivenForceWeight, 0.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

/**
 * @tc.name: BoidsSwarmComponent35
 * @tc.desc: Verify drivenForce accepts negative components (external force vector).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_BoidsSwarmSystemTest, BoidsSwarmComponent35, testing::ext::TestSize.Level1)
{
    TestContext* testContext = GetTestContext();
    auto ecsContext = testContext->ecs;

    CORE3D_NS::INodeSystem* nodeSystem = nullptr;
    CORE_NS::IComponentManager* cm = nullptr;
    CORE_NS::Entity entity = CreateSwarmEntity(*ecsContext, nodeSystem, cm);
    ASSERT_TRUE(nodeSystem);
    ASSERT_TRUE(cm);

    {
        CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> data(cm->GetData(entity));
        ASSERT_TRUE(data);
        data->drivenForce = {+1.0f, -2.0f, -3.0f};
    }
    ecsContext->ProcessEvents();

    CORE_NS::ScopedHandle<BOIDSSWARM_NS::BoidsSwarmComponent> read(cm->GetData(entity));
    ASSERT_TRUE(read);
    EXPECT_FLOAT_EQ(read->drivenForce.x, +1.0f);
    EXPECT_FLOAT_EQ(read->drivenForce.y, -2.0f);
    EXPECT_FLOAT_EQ(read->drivenForce.z, -3.0f);

    nodeSystem->DestroyNode(*nodeSystem->GetNode(entity));
}

}  // namespace UTest
}  // namespace BoidSwarm
