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

#include <map>

#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/property/intf_property_handle.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;

namespace {
class EcsListener : public IEcs::EntityListener, public IEcs::ComponentListener {
public:
    void OnEntityEvent(EntityListener::EventType type, const array_view<const Entity> entities) override
    {
        if (type == EntityListener::EventType::CREATED) {
            OnEntitiesAdded(entities);
        } else if (type == EntityListener::EventType::DESTROYED) {
            OnEntitiesRemoved(entities);
        }
    }

    void OnComponentEvent(ComponentListener::EventType type, const IComponentManager& componentManager,
        const array_view<const Entity> entities) override
    {
        if (type == ComponentListener::EventType::CREATED) {
            OnComponentsAdded(componentManager, entities);
        } else if (type == ComponentListener::EventType::MODIFIED) {
            OnComponentsUpdated(componentManager, entities);
        } else if (type == ComponentListener::EventType::DESTROYED) {
            OnComponentsRemoved(componentManager, entities);
        }
    }

    void OnEntitiesAdded(const array_view<const Entity> entities)
    {
        mAddedEntities += entities.size();
    }

    void OnEntitiesRemoved(const array_view<const Entity> entities)
    {
        mRemovedEntities += entities.size();
    }

    void OnComponentsAdded(const IComponentManager& componentManager, const array_view<const Entity> entities)
    {
        size_t amount = 0;

        auto it = mAddedComponents.find(&componentManager);
        if (it != mAddedComponents.end()) {
            amount = it->second;
        }

        mAddedComponents[&componentManager] = amount + entities.size();
    }

    void OnComponentsRemoved(const IComponentManager& componentManager, const array_view<const Entity> entities)
    {
        size_t amount = 0;

        auto it = mRemovedComponents.find(&componentManager);
        if (it != mRemovedComponents.end()) {
            amount = it->second;
        }

        mRemovedComponents[&componentManager] = amount + entities.size();
    }

    void OnComponentsUpdated(const IComponentManager& componentManager, const array_view<const Entity> entities)
    {
        size_t amount = 0;

        auto it = mUpdatedComponents.find(&componentManager);
        if (it != mUpdatedComponents.end()) {
            amount = it->second;
        }

        mUpdatedComponents[&componentManager] = amount + entities.size();
    }

    void Clear()
    {
        mAddedEntities = 0;
        mRemovedEntities = 0;
        mAddedComponents.clear();
        mRemovedComponents.clear();
        mUpdatedComponents.clear();
    }

    size_t GetAddedEntities() const
    {
        return mAddedEntities;
    }

    size_t GetRemovedEntities() const
    {
        return mRemovedEntities;
    }

    size_t GetAddedComponents(const IComponentManager& componentManager) const
    {
        auto it = mAddedComponents.find(&componentManager);
        if (it != mAddedComponents.end()) {
            return it->second;
        }

        return 0;
    }

    size_t GetRemovedComponents(const IComponentManager& componentManager) const
    {
        auto it = mRemovedComponents.find(&componentManager);
        if (it != mRemovedComponents.end()) {
            return it->second;
        }

        return 0;
    }

    size_t GetUpdatedComponents(const IComponentManager& componentManager) const
    {
        auto it = mUpdatedComponents.find(&componentManager);
        if (it != mUpdatedComponents.end()) {
            return it->second;
        }

        return 0;
    }

private:
    size_t mAddedEntities { 0 };
    size_t mRemovedEntities { 0 };
    std::map<const IComponentManager*, size_t> mAddedComponents;
    std::map<const IComponentManager*, size_t> mRemovedComponents;
    std::map<const IComponentManager*, size_t> mUpdatedComponents;
}; // namespace

/**
 * @tc.name: listenerTest
 * @tc.desc: Tests for Listener Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GpuTest_EcsTest, listenerTest, testing::ext::TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& nodeSystem = *(GetSystem<INodeSystem>(*ecs));
    auto& nodeComponentManager = *(GetManager<INodeComponentManager>(*ecs));
    auto& nameComponentManager = *(GetManager<INameComponentManager>(*ecs));
    auto& transformComponentManager = *(GetManager<ITransformComponentManager>(*ecs));

    EcsListener listener;
    ecs->AddListener(static_cast<IEcs::EntityListener&>(listener));
    ecs->AddListener(static_cast<IEcs::ComponentListener&>(listener));

    std::vector<ISceneNode*> nodes;

    const size_t nodeCount = 5;

    ecs->ProcessEvents();
    const size_t initialEntities = listener.GetAddedEntities();
    const size_t initialNames = listener.GetAddedComponents(nameComponentManager);

    // Create nodes.
    for (size_t i = 0; i < nodeCount; ++i) {
        nodes.push_back(nodeSystem.CreateNode());
    }

    ecs->ProcessEvents();

    // See that listeners got fired.
    ASSERT_EQ(listener.GetAddedEntities(), initialEntities + nodeCount);
    ASSERT_EQ(listener.GetAddedComponents(nodeComponentManager), nodeCount);
    ASSERT_EQ(listener.GetAddedComponents(nameComponentManager), initialNames + nodeCount);
    ASSERT_EQ(listener.GetAddedComponents(transformComponentManager), nodeCount);
    ASSERT_EQ(listener.GetUpdatedComponents(nodeComponentManager), 0);
    ASSERT_EQ(listener.GetUpdatedComponents(nameComponentManager), 0);
    ASSERT_EQ(listener.GetUpdatedComponents(transformComponentManager), 0);
    listener.Clear();

    // Modify nodes.
    nodes.front()->SetName("test");
    nodes.back()->SetPosition(Math::Vec3(1.f, 2.f, 3.f));

    ecs->ProcessEvents();

    ASSERT_EQ(listener.GetUpdatedComponents(nodeComponentManager), 0);
    ASSERT_EQ(listener.GetUpdatedComponents(nameComponentManager), 1);
    ASSERT_EQ(listener.GetUpdatedComponents(transformComponentManager), 1);
    listener.Clear();

    auto handle = nodeComponentManager.GetData(nodes.front()->GetEntity());
    handle->WLock();
    handle->WUnlock();

    ecs->ProcessEvents();

    ASSERT_EQ(listener.GetUpdatedComponents(nodeComponentManager), 1);
    ASSERT_EQ(listener.GetUpdatedComponents(nameComponentManager), 0);
    ASSERT_EQ(listener.GetUpdatedComponents(transformComponentManager), 0);
    listener.Clear();

    // Remove all node components.
    for (auto& node : nodes) {
        nodeComponentManager.Destroy(node->GetEntity());
    }

    ecs->ProcessEvents();

    ASSERT_EQ(listener.GetRemovedEntities(), 0);
    ASSERT_EQ(listener.GetRemovedComponents(nodeComponentManager), nodeCount);
    ASSERT_EQ(listener.GetRemovedComponents(nameComponentManager), 0);
    ASSERT_EQ(listener.GetRemovedComponents(transformComponentManager), 0);
    listener.Clear();

    IEntityManager& entityManager = ecs->GetEntityManager();
    entityManager.DestroyAllEntities();

    ecs->ProcessEvents();

    ASSERT_EQ(listener.GetRemovedEntities(), initialEntities + nodeCount);
    ASSERT_EQ(listener.GetRemovedComponents(nodeComponentManager), 0);
    ASSERT_EQ(listener.GetRemovedComponents(nameComponentManager), initialNames + nodeCount);
    ASSERT_EQ(listener.GetRemovedComponents(transformComponentManager), nodeCount);
    listener.Clear();

    ecs->RemoveListener(static_cast<IEcs::EntityListener&>(listener));
    ecs->RemoveListener(static_cast<IEcs::ComponentListener&>(listener));
}
} // namespace
