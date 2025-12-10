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

#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>

#include "ecs/components/initial_transform_component.h"
#include "ecs/components/previous_joint_matrices_component.h"
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
template<typename ComponentType, typename ManagerType>
void BaseManagerCreateTest(BASE_NS::string componentName)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto baseManager = GetManager<ManagerType>(*ecs);
    auto componentManager = static_cast<IComponentManager*>(baseManager);
    EXPECT_EQ(componentName, componentManager->GetName());

    Entity entity = ecs->GetEntityManager().Create();
    uint32_t generation;
    {
        baseManager->Create(entity);
        ASSERT_TRUE(baseManager->HasComponent(entity));
        auto id = baseManager->GetComponentId(entity);
        ASSERT_NE(IComponentManager::INVALID_COMPONENT_ID, id);
        EXPECT_EQ(IComponentManager::INVALID_COMPONENT_ID, baseManager->GetComponentId(Entity {}));
        ASSERT_EQ(entity, baseManager->GetEntity(id));
        generation = baseManager->GetComponentGeneration(id);
        EXPECT_LE(generation, baseManager->GetGenerationCounter());
        EXPECT_EQ(0u, baseManager->GetComponentGeneration(IComponentManager::INVALID_COMPONENT_ID));
        EXPECT_TRUE(baseManager->GetModifiedFlags() & CORE_NS::CORE_COMPONENT_MANAGER_COMPONENT_ADDED_BIT);
    }
    // Recreate entity
    {
        baseManager->Create(entity);
        ASSERT_TRUE(baseManager->HasComponent(entity));
        auto id = baseManager->GetComponentId(entity);
        ASSERT_NE(IComponentManager::INVALID_COMPONENT_ID, id);
        ASSERT_EQ(entity, baseManager->GetEntity(id));
        ASSERT_NE(generation, baseManager->GetComponentGeneration(id));
    }
}

template<typename ComponentType, typename ManagerType>
void BaseManagerIPropertyApiTest(BASE_NS::string componentName)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto baseManager = GetManager<ManagerType>(*ecs);
    auto& propertyApi = baseManager->GetPropertyApi();
    const uint64_t typeHash = BASE_NS::FNV1aHash(componentName.data(), componentName.size());
    EXPECT_EQ(typeHash, propertyApi.Type());

    {
        auto propertyHandle = propertyApi.Create();
        ASSERT_NE(nullptr, propertyHandle);
        ASSERT_EQ(sizeof(ComponentType), propertyHandle->Size());
        {
            auto propertyApi2 = propertyHandle->Owner();
            ASSERT_EQ(propertyApi.Type(), propertyApi2->Type());
            ASSERT_EQ(propertyApi.PropertyCount(), propertyApi2->PropertyCount());
            for (uint32_t i = 0; i < propertyApi2->PropertyCount(); ++i) {
                ASSERT_EQ(propertyApi.MetaData(i), propertyApi2->MetaData(i));
            }
            ASSERT_EQ(nullptr, propertyApi2->MetaData(propertyApi2->PropertyCount()));
            auto propertyHandle2 = propertyApi2->Create();
            auto clone = propertyApi2->Clone(propertyHandle2);
            EXPECT_NE(nullptr, clone);
            propertyApi2->Release(propertyHandle2);
            propertyApi2->Release(clone);
        }
        auto clone = propertyApi.Clone(propertyHandle);
        EXPECT_NE(nullptr, clone);
        propertyApi.Release(propertyHandle);
        propertyApi.Release(clone);
    }

    {
        size_t propertyCount = propertyApi.PropertyCount();
        auto metadata = propertyApi.MetaData();
        ASSERT_EQ(propertyCount, metadata.size());
        for (size_t i = 0; i < propertyCount; ++i) {
            ASSERT_EQ(&metadata[i], propertyApi.MetaData(i));
        }
        ASSERT_EQ(nullptr, propertyApi.MetaData(propertyCount));
    }
}
} // namespace

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsInitialTransformComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<InitialTransformComponent, IInitialTransformComponentManager>("InitialTransformComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsInitialTransformComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<InitialTransformComponent, IInitialTransformComponentManager>(
        "InitialTransformComponent");
}

/**
 * @tc.name: CreateTest
 * @tc.desc: Tests for Create Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsPreviousJointMatricesComponent, CreateTest, testing::ext::TestSize.Level1)
{
    BaseManagerCreateTest<PreviousJointMatricesComponent, IPreviousJointMatricesComponentManager>(
        "PreviousJointMatricesComponent");
}
/**
 * @tc.name: IPropertyApiTest
 * @tc.desc: Tests for Iproperty Api Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_EcsPreviousJointMatricesComponent, IPropertyApiTest, testing::ext::TestSize.Level1)
{
    BaseManagerIPropertyApiTest<PreviousJointMatricesComponent, IPreviousJointMatricesComponentManager>(
        "PreviousJointMatricesComponent");
}
