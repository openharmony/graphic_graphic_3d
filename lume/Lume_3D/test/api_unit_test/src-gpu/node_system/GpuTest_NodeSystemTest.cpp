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

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/intf_graphics_context.h>
#include <base/containers/fixed_string.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/os/intf_platform.h>

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
auto generateNameFromIndex(size_t aIndex)
{
    return "node " + to_string(aIndex);
}

void recursivelyGenerateChildNodes(Entity aParentEntity, IEntityManager& aEntityManager,
    INodeComponentManager& aNodeComponentManager, INameComponentManager& aNameComponentManager,
    size_t aGeneratedChildNodeCount, size_t aRecursionCount)
{
    for (size_t i = 0; i < aGeneratedChildNodeCount; ++i) {
        const Entity entity = aEntityManager.Create();

        NodeComponent nodeData;
        nodeData.parent = aParentEntity;
        aNodeComponentManager.Set(entity, nodeData);

        NameComponent nameData;
        nameData.name = generateNameFromIndex(i);
        aNameComponentManager.Set(entity, nameData);

        if (aRecursionCount > 0) {
            recursivelyGenerateChildNodes(entity, aEntityManager, aNodeComponentManager, aNameComponentManager,
                aGeneratedChildNodeCount, aRecursionCount - 1);
        }
    }
}

void recursivelyValidateHierarchy(
    ISceneNode const& aParentNode, INodeSystem& aNodeSystem, size_t aGeneratedChildNodeCount, size_t aRecursionCount)
{
    for (size_t i = 0; i < aGeneratedChildNodeCount; ++i) {
        const auto nodeName = generateNameFromIndex(i);

        const ISceneNode* node = aParentNode.GetChild(nodeName);

        ASSERT_TRUE(node != nullptr);
        ASSERT_TRUE(node->GetName() == nodeName);

        if (aRecursionCount > 0) {
            recursivelyValidateHierarchy(aParentNode, aNodeSystem, aGeneratedChildNodeCount, aRecursionCount - 1);
        }
    }
}

void recursivelyValidateHierarchyFromRoot(ISceneNode& aRootNode, string_view const& aPath, INodeSystem& aNodeSystem,
    size_t aGeneratedChildNodeCount, size_t aRecursionCount)
{
    for (size_t i = 0; i < aGeneratedChildNodeCount; ++i) {
        const auto nodeName = generateNameFromIndex(i);

        ISceneNode* node = aRootNode.LookupNodeByPath(aPath + nodeName);

        ASSERT_TRUE(node != nullptr);
        ASSERT_TRUE(node->GetName() == nodeName);

        if (aRecursionCount > 0) {
            recursivelyValidateHierarchyFromRoot(
                aRootNode, aPath + nodeName + "/", aNodeSystem, aGeneratedChildNodeCount, aRecursionCount - 1);
        }
    }
}
} // namespace

class GpuTestNodeSystemTest : public testing::Test {
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
 * @tc.name: NodeReparenting
 * @tc.desc: test NodeReparenting
 * @tc.type: FUNC
 */
HWTEST_F(GpuTestNodeSystemTest, NodeReparenting, TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.

    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

    IEntityManager& entityManager = ecs->GetEntityManager();

    auto& nodeSystem = *(GetSystem<INodeSystem>(*ecs));
    auto& nodeComponentManager = *(GetManager<INodeComponentManager>(*ecs));
    auto& nameComponentManager = *(GetManager<INameComponentManager>(*ecs));

    ISceneNode& rootNode = nodeSystem.GetRootNode();

    size_t generatedChildNodeCount = 5;
    size_t recursionCount = 0;

    // Add nodes through ecs interface.
    recursivelyGenerateChildNodes(rootNode.GetEntity(), entityManager, nodeComponentManager, nameComponentManager,
        generatedChildNodeCount, recursionCount);

    // Grab node0 and node1
    ISceneNode* node0 = rootNode.LookupNodeByPath("node 0");
    ISceneNode* node1 = rootNode.LookupNodeByPath("node 1");
    ASSERT_TRUE(node0 != nullptr);
    ASSERT_TRUE(node1 != nullptr);

    // Ensure node0 is not parent of node1
    ASSERT_TRUE(node1->GetParent() == &rootNode);
    ASSERT_TRUE(rootNode.LookupNodeByPath("node 0/node 1") == nullptr);

    // Make node0 parent of node1 through ecs api.
    NodeComponent n1 = nodeComponentManager.Get(node1->GetEntity());
    n1.parent = node0->GetEntity();
    nodeComponentManager.Set(node1->GetEntity(), n1);

    // Re-grab node1.
    ISceneNode* node = rootNode.LookupNodeByPath("node 0/node 1");

    // Ensure it was node1 that was re-parented.
    ASSERT_TRUE(node == node1);
    ASSERT_TRUE(node->GetParent() == node0);
    ASSERT_TRUE(node0->GetChild("node 1") == node);

    // Re-parent back using the node api.
    node1->SetParent(rootNode);

    // Ensure the operation was successful and look-up still works.
    ISceneNode* reparentedNode = rootNode.LookupNodeByPath("node 1");

    ASSERT_TRUE(reparentedNode != nullptr);
    ASSERT_TRUE(node == node1 && node == reparentedNode);
    ASSERT_TRUE(reparentedNode->GetParent() == &rootNode);
    ASSERT_TRUE(node0->GetChild("node 1") == nullptr);

    nodeSystem.DestroyNode(rootNode);
}

/**
 * @tc.name: NodeLookupByNameAndComponent
 * @tc.desc: test NodeLookupByNameAndComponent
 * @tc.type: FUNC
 */
HWTEST_F(GpuTestNodeSystemTest, NodeLookupByNameAndComponent, TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.

    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

    IEntityManager& entityManager = ecs->GetEntityManager();

    auto& nodeSystem = *(GetSystem<INodeSystem>(*ecs));
    auto& nodeComponentManager = *(GetManager<INodeComponentManager>(*ecs));
    auto& nameComponentManager = *(GetManager<INameComponentManager>(*ecs));

    ISceneNode& rootNode = nodeSystem.GetRootNode();
    const ISceneNode& constRootNode = nodeSystem.GetRootNode();

    size_t generatedChildNodeCount = 5;
    size_t recursionCount = 2;

    // Add nodes through ecs interface.
    recursivelyGenerateChildNodes(rootNode.GetEntity(), entityManager, nodeComponentManager, nameComponentManager,
        generatedChildNodeCount, recursionCount);

    // Check that we can find nodes by name from the hierarchy.
    ISceneNode* node1 = rootNode.LookupNodeByName("node 1");
    ASSERT_TRUE(node1 != nullptr);
    const ISceneNode* constNode2 = constRootNode.LookupNodeByName("node 2");
    ASSERT_TRUE(constNode2 != nullptr);

    // Create 2 nodes that have a transform component in them.
    ISceneNode* nodeWithTransform0 = nodeSystem.CreateNode();
    nodeWithTransform0->SetParent(*node1);
    ISceneNode* nodeWithTransform1 = nodeSystem.CreateNode();
    nodeWithTransform1->SetParent(*constNode2);

    // Check that we can find a node with transform component.
    auto& transformComponentManager = *(GetManager<ITransformComponentManager>(*ecs));
    ISceneNode* transformableNode = rootNode.LookupNodeByComponent(transformComponentManager);
    ASSERT_TRUE(transformableNode != nullptr);

    // Check that we can find all nodes with transform component.
    Base::vector<const ISceneNode*> transformableNodes =
        constRootNode.LookupNodesByComponent(transformComponentManager);
    ASSERT_TRUE(transformableNodes.size() == 2);
}
