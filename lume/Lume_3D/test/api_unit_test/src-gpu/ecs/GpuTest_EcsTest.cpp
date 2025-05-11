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

#include <map>

#include <gtest/gtest.h>

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

#include "TestRunner.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;
using namespace testing::ext;

namespace {
struct TestContext {
    std::shared_ptr<ISceneInit> sceneInit_ = nullptr;
    CORE_NS::IEcs::Ptr ecs_;
};
static TestContext g_context;

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
static constexpr BASE_NS::Uid ENGINE_THREAD{"2070e705-d061-40e4-bfb7-90fad2c280af"};
static constexpr BASE_NS::Uid APP_THREAD{"b2e8cef3-453a-4651-b564-5190f8b5190d"};
static constexpr BASE_NS::Uid IO_QUEUE{"be88e9a0-9cd8-45ab-be48-937953dc258f"};
static constexpr BASE_NS::Uid JS_RELEASE_THREAD{"3784fa96-b25b-4e9c-bbf1-e897d36f73af"};

bool SceneDispose(TestContext &context)
{
    context.ecs_ = nullptr;
    context.sceneInit_ = nullptr;
    return true;
}

bool SceneCreate(TestContext &context)
{
    context.sceneInit_ = CreateTestScene();
    context.sceneInit_->LoadPluginsAndInit();
    if (!context.sceneInit_->GetEngineInstance().engine_) {
        WIDGET_LOGE("fail to get engine");
        return false;
    }
    context.ecs_ = context.sceneInit_->GetEngineInstance().engine_->CreateEcs();
    if (!context.ecs_) {
        WIDGET_LOGE("fail to get ecs");
        return false;
    }
    auto factory = GetInstance<Core::ISystemGraphLoaderFactory>(UID_SYSTEM_GRAPH_LOADER);
    auto systemGraphLoader = factory->Create(context.sceneInit_->GetEngineInstance().engine_->GetFileManager());
    systemGraphLoader->Load("rofs3D://systemGraph.json", *(context.ecs_));
    auto& ecs = *(context.ecs_);
    ecs.Initialize();

    using namespace SCENE_NS;
#if SCENE_META_TEST
    auto fun = [&context]() {
        auto &obr = META_NS::GetObjectRegistry();

        context.params_ = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        if (!context.params_) {
            CORE_LOG_E("default obj null");
        }
        context.scene_ =
            interface_pointer_cast<SCENE_NS::IScene>(obr.Create(SCENE_NS::ClassId::Scene, context.params_));

        auto onLoaded = META_NS::MakeCallback<META_NS::IOnChanged>([&context]() {
            bool complete = false;
            auto status = context.scene_->Status()->GetValue();
            if (status == SCENE_NS::IScene::SCENE_STATUS_READY) {
                // still in engine thread
                complete = true;
            } else if (status == SCENE_NS::IScene::SCENE_STATUS_LOADING_FAILED) {
                // make sure we don't have anything in result if error
                complete = true;
            }

            if (complete) {
                if (context.scene_) {
                    auto &obr = META_NS::GetObjectRegistry();
                    // make sure we have renderconfig
                    auto rc = context.scene_->RenderConfiguration()->GetValue();
                    if (!rc) {
                        // Create renderconfig
                        rc = obr.Create<SCENE_NS::IRenderConfiguration>(SCENE_NS::ClassId::RenderConfiguration);
                        context.scene_->RenderConfiguration()->SetValue(rc);
                    }

                    interface_cast<IEcsScene>(context.scene_)
                        ->RenderMode()
                        ->SetValue(IEcsScene::RenderMode::RENDER_ALWAYS);
                    auto duh = context.params_->GetArrayPropertyByName<IntfWeakPtr>("Scenes");
                    if (!duh) {
                        return ;
                    }
                    duh->AddValue(interface_pointer_cast<CORE_NS::IInterface>(context.scene_));
                }
            }
        });
        context.scene_->Asynchronous()->SetValue(false);
        context.scene_->Uri()->SetValue("scene://empty");
        return META_NS::IAny::Ptr{};
    };
    // Should it be possible to cancel? (ie. do we need to store the token for something ..)
    META_NS::GetTaskQueueRegistry()
        .GetTaskQueue(ENGINE_THREAD)
        ->AddWaitableTask(META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>(BASE_NS::move(fun)))
        ->Wait();
#endif
    return true;
}
} // namespace

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

    size_t getAddedComponents(const IComponentManager& componentManager) const
    {
        auto it = mAddedComponents.find(&componentManager);
        if (it != mAddedComponents.end()) {
            return it->second;
        }

        return 0;
    }

    size_t getRemovedComponents(const IComponentManager& componentManager) const
    {
        auto it = mRemovedComponents.find(&componentManager);
        if (it != mRemovedComponents.end()) {
            return it->second;
        }

        return 0;
    }

    size_t getUpdatedComponents(const IComponentManager& componentManager) const
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

class GpuTestEcsTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SceneCreate(g_context);
    }
    static void TearDownTestSuite()
    {
        SceneDispose(g_context);
    }
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: ListenerTest
 * @tc.desc: test Listener
 * @tc.type: FUNC
 */
HWTEST_F(GpuTestEcsTest, ListenerTest, TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.

    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

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
    const size_t initialNames = listener.getAddedComponents(nameComponentManager);

    // Create nodes.
    for (size_t i = 0; i < nodeCount; ++i) {
        nodes.push_back(nodeSystem.CreateNode());
    }

    ecs->ProcessEvents();

    // See that listeners got fired.
    ASSERT_EQ(listener.GetAddedEntities(), initialEntities + nodeCount);
    ASSERT_EQ(listener.getAddedComponents(nodeComponentManager), nodeCount);
    ASSERT_EQ(listener.getAddedComponents(nameComponentManager), initialNames + nodeCount);
    ASSERT_EQ(listener.getAddedComponents(transformComponentManager), nodeCount);
    ASSERT_EQ(listener.getUpdatedComponents(nodeComponentManager), 0);
    ASSERT_EQ(listener.getUpdatedComponents(nameComponentManager), 0);
    ASSERT_EQ(listener.getUpdatedComponents(transformComponentManager), 0);
    listener.Clear();

    // Modify nodes.
    nodes.front()->SetName("test");
    nodes.back()->SetPosition(Math::Vec3(1.f, 2.f, 3.f));

    ecs->ProcessEvents();

    ASSERT_EQ(listener.getUpdatedComponents(nodeComponentManager), 0);
    ASSERT_EQ(listener.getUpdatedComponents(nameComponentManager), 1);
    ASSERT_EQ(listener.getUpdatedComponents(transformComponentManager), 1);
    listener.Clear();

    auto handle = nodeComponentManager.GetData(nodes.front()->GetEntity());
    handle->WLock();
    handle->WUnlock();

    ecs->ProcessEvents();

    ASSERT_EQ(listener.getUpdatedComponents(nodeComponentManager), 1);
    ASSERT_EQ(listener.getUpdatedComponents(nameComponentManager), 0);
    ASSERT_EQ(listener.getUpdatedComponents(transformComponentManager), 0);
    listener.Clear();

    // Remove all node components.
    for (auto& node : nodes) {
        nodeComponentManager.Destroy(node->GetEntity());
    }

    ecs->ProcessEvents();

    ASSERT_EQ(listener.GetRemovedEntities(), 0);
    ASSERT_EQ(listener.getRemovedComponents(nodeComponentManager), nodeCount);
    ASSERT_EQ(listener.getRemovedComponents(nameComponentManager), 0);
    ASSERT_EQ(listener.getRemovedComponents(transformComponentManager), 0);
    listener.Clear();

    IEntityManager& entityManager = ecs->GetEntityManager();
    entityManager.DestroyAllEntities();

    ecs->ProcessEvents();

    ASSERT_EQ(listener.GetRemovedEntities(), initialEntities + nodeCount);
    ASSERT_EQ(listener.getRemovedComponents(nodeComponentManager), 0);
    ASSERT_EQ(listener.getRemovedComponents(nameComponentManager), initialNames + nodeCount);
    ASSERT_EQ(listener.getRemovedComponents(transformComponentManager), nodeCount);
    listener.Clear();

    ecs->RemoveListener(static_cast<IEcs::EntityListener&>(listener));
    ecs->RemoveListener(static_cast<IEcs::ComponentListener&>(listener));
}
} // namespace
