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

#include <chrono>
#include <thread>

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

/**
 * @tc.name: nodeLookup
 * @tc.desc: Tests for Node Lookup. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GpuTest_NodeSystemTest, nodeLookup, testing::ext::TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    IEntityManager& entityManager = ecs->GetEntityManager();

    auto& nodeSystem = *(GetSystem<INodeSystem>(*ecs));
    auto& nodeComponentManager = *(GetManager<INodeComponentManager>(*ecs));
    auto& nameComponentManager = *(GetManager<INameComponentManager>(*ecs));

    ISceneNode& rootNode = nodeSystem.GetRootNode();

    size_t generatedChildNodeCount = 5;
    size_t recursionCount = 2;

    // Add nodes through ecs interface.
    recursivelyGenerateChildNodes(rootNode.GetEntity(), entityManager, nodeComponentManager, nameComponentManager,
        generatedChildNodeCount, recursionCount);

    // Ensure that it is possible to access nodes through nodesystem API.
    recursivelyValidateHierarchy(rootNode, nodeSystem, generatedChildNodeCount, recursionCount);
    recursivelyValidateHierarchyFromRoot(rootNode, "", nodeSystem, generatedChildNodeCount, recursionCount);

    nodeSystem.DestroyNode(rootNode);
}

/**
 * @tc.name: nodeReparenting
 * @tc.desc: Tests for Node Reparenting. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GpuTest_NodeSystemTest, nodeReparenting, testing::ext::TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

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
 * @tc.name: nodeRemove
 * @tc.desc: Tests for Node Remove. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GpuTest_NodeSystemTest, nodeRemove, testing::ext::TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    IEntityManager& entityManager = ecs->GetEntityManager();

    auto& nodeSystem = *(GetSystem<INodeSystem>(*ecs));
    auto& nodeComponentManager = *(GetManager<INodeComponentManager>(*ecs));
    auto& nameComponentManager = *(GetManager<INameComponentManager>(*ecs));

    size_t generatedChildNodeCount = 1;
    size_t recursionCount = 0;

    ISceneNode& rootNode = nodeSystem.GetRootNode();

    // Add nodes through ecs interface.
    recursivelyGenerateChildNodes(rootNode.GetEntity(), entityManager, nodeComponentManager, nameComponentManager,
        generatedChildNodeCount, recursionCount);

    // Grab node, ensure it is there.
    ISceneNode* node = rootNode.LookupNodeByPath("node 0");
    ASSERT_TRUE(node != nullptr);

    // Remove entity.
    entityManager.Destroy(node->GetEntity());

    // Node should no longer exist.
    node = rootNode.LookupNodeByPath("node 0");
    ASSERT_TRUE(node == nullptr);
}

/**
 * @tc.name: nodeLookupByNameAndComponent
 * @tc.desc: Tests for Node Lookup By Name And Component. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_GpuTest_NodeSystemTest, nodeLookupByNameAndComponent, testing::ext::TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

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
    vector<const ISceneNode*> transformableNodes = constRootNode.LookupNodesByComponent(transformComponentManager);
    ASSERT_TRUE(transformableNodes.size() == 2);
}
