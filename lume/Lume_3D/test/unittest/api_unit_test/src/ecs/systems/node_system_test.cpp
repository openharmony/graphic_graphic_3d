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
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <base/containers/array_view.h>
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
 * @tc.name: CreateDestroyNodeTest
 * @tc.desc: Tests for Create Destroy Node Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsNodeSystem, CreateDestroyNodeTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto nodeManager = GetManager<INodeComponentManager>(*ecs);
    ASSERT_NE(nullptr, nodeManager);
    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);
    nodeSystem->SetActive(true);
    EXPECT_TRUE(nodeSystem->IsActive());
    EXPECT_EQ("NodeSystem", nodeSystem->GetName());
    EXPECT_EQ(INodeSystem::UID, ((ISystem*)nodeSystem)->GetUid());
    EXPECT_EQ(nullptr, nodeSystem->GetProperties());
    EXPECT_EQ(nullptr, ((const INodeSystem*)nodeSystem)->GetProperties());
    EXPECT_EQ(ecs.get(), &nodeSystem->GetECS());

    auto node = nodeSystem->CreateNode();
    ASSERT_NE(nullptr, node);
    auto entity = node->GetEntity();
    EXPECT_TRUE(nodeManager->HasComponent(entity));
    EXPECT_TRUE(node->GetEffectivelyEnabled());
    nodeSystem->DestroyNode(*node);
}

/**
 * @tc.name: CreateDestroyNodeTreeTest
 * @tc.desc: Tests for Create Destroy Node Tree Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsNodeSystem, CreateDestroyNodeTreeTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto nodeManager = GetManager<INodeComponentManager>(*ecs);
    ASSERT_NE(nullptr, nodeManager);
    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);

    // Create tree
    constexpr uint32_t depth = 3u;
    constexpr uint32_t numNodes = (1u << depth) - 1;
    BASE_NS::vector<ISceneNode*> nodes(numNodes);
    for (uint32_t i = 0; i < numNodes; ++i) {
        nodes[i] = nodeSystem->CreateNode();
        nodes[i]->SetName(to_string(i));
        if (i > 0) {
            const uint32_t parentIdx = (i - 1) >> 1;
            nodes[i]->SetParent(*nodes[parentIdx]);
        }
    }
    // Destroy the second node (first child)
    nodeSystem->DestroyNode(*nodes[1]);
    // When node at index 1 was removed it's childred at indices 3 and 4 should also get destroyed. Cleanup our list.
    nodes.erase(nodes.cbegin() + 3U, nodes.cbegin() + 5U);
    nodes.erase(nodes.cbegin() + 1U);
    auto children = nodeSystem->GetRootNode().GetChildren();
    ASSERT_EQ(children.size(), 1U);
    EXPECT_EQ(children[0], nodes[0U]);
    auto grandChildren = children[0]->GetChildren();
    ASSERT_EQ(grandChildren.size(), 1U);
    EXPECT_EQ(grandChildren[0], nodes[1U]);
    auto greatGrandChildren = grandChildren[0]->GetChildren();
    ASSERT_EQ(greatGrandChildren.size(), 2U);
    EXPECT_EQ(greatGrandChildren[0], nodes[2U]);
    EXPECT_EQ(greatGrandChildren[1], nodes[3U]);
}

/**
 * @tc.name: CloneTest
 * @tc.desc: Tests for Clone Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsNodeSystem, CloneTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto nodeManager = GetManager<INodeComponentManager>(*ecs);
    ASSERT_NE(nullptr, nodeManager);
    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);

    // Non recursive clone
    {
        auto parentNode = nodeSystem->CreateNode();
        parentNode->SetName("Node0");
        parentNode->SetPosition(Math::Vec3 { 1.0f, 2.0f, 3.0f });
        auto quat = Math::FromEulerRad(Math::Vec3 { 0.5f, 0.2f, 0.1f });
        parentNode->SetRotation(quat);
        parentNode->SetScale(Math::Vec3 { 1.0f, 2.0f, 1.0f });
        parentNode->SetEnabled(true);
        auto childNode = nodeSystem->CreateNode();
        childNode->SetParent(*parentNode);
        auto clonedNode = nodeSystem->CloneNode(*parentNode, false);
        EXPECT_TRUE(clonedNode->GetChildren().empty());
        EXPECT_EQ("Node0", clonedNode->GetName());
        EXPECT_EQ(1.0f, clonedNode->GetPosition().x);
        EXPECT_EQ(2.0f, clonedNode->GetPosition().y);
        EXPECT_EQ(3.0f, clonedNode->GetPosition().z);
        EXPECT_EQ(quat.x, clonedNode->GetRotation().x);
        EXPECT_EQ(quat.y, clonedNode->GetRotation().y);
        EXPECT_EQ(quat.z, clonedNode->GetRotation().z);
        EXPECT_EQ(quat.w, clonedNode->GetRotation().w);
        EXPECT_EQ(1.0f, clonedNode->GetScale().x);
        EXPECT_EQ(2.0f, clonedNode->GetScale().y);
        EXPECT_EQ(1.0f, clonedNode->GetScale().z);
    }
    // Recursive clone
    {
        auto parentNode = nodeSystem->CreateNode();
        parentNode->SetName("Node0");
        parentNode->SetPosition(Math::Vec3 { 1.0f, 2.0f, 3.0f });
        auto quat = Math::FromEulerRad(Math::Vec3 { 0.5f, 0.2f, 0.1f });
        parentNode->SetRotation(quat);
        parentNode->SetScale(Math::Vec3 { 1.0f, 2.0f, 1.0f });
        parentNode->SetEnabled(true);
        auto childNode = nodeSystem->CreateNode();
        childNode->SetName("Node1");
        childNode->SetPosition(Math::Vec3 { 5.0f, 4.0f, 3.0f });
        childNode->SetEnabled(false);
        childNode->SetParent(*parentNode);
        auto clonedNode = nodeSystem->CloneNode(*parentNode, true);
        EXPECT_EQ(1u, clonedNode->GetChildren().size());
        EXPECT_EQ("Node0", clonedNode->GetName());
        EXPECT_EQ(1.0f, clonedNode->GetPosition().x);
        EXPECT_EQ(2.0f, clonedNode->GetPosition().y);
        EXPECT_EQ(3.0f, clonedNode->GetPosition().z);
        EXPECT_EQ(quat.x, clonedNode->GetRotation().x);
        EXPECT_EQ(quat.y, clonedNode->GetRotation().y);
        EXPECT_EQ(quat.z, clonedNode->GetRotation().z);
        EXPECT_EQ(quat.w, clonedNode->GetRotation().w);
        EXPECT_EQ(1.0f, clonedNode->GetScale().x);
        EXPECT_EQ(2.0f, clonedNode->GetScale().y);
        EXPECT_EQ(1.0f, clonedNode->GetScale().z);

        auto clonedChild = clonedNode->GetChild("Node1");
        ASSERT_NE(nullptr, clonedChild);
        EXPECT_EQ(nullptr, ((const ISceneNode*)clonedNode)->GetChild("NonExistingChild"));
        EXPECT_FALSE(clonedChild->GetEnabled());
        EXPECT_TRUE(clonedChild->GetEffectivelyEnabled());
        EXPECT_EQ(5.0f, clonedChild->GetPosition().x);
        EXPECT_EQ(4.0f, clonedChild->GetPosition().y);
        EXPECT_EQ(3.0f, clonedChild->GetPosition().z);
    }
}

/**
 * @tc.name: DeepRecursiveCloneTest
 * @tc.desc: Tests for Deep Recursive Clone Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsNodeSystem, DeepRecursiveCloneTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto nodeManager = GetManager<INodeComponentManager>(*ecs);
    ASSERT_NE(nullptr, nodeManager);
    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);

    // Full binary tree
    {
        // Create binary tree
        constexpr uint32_t depth = 10u;
        constexpr uint32_t numNodes = (1u << depth) - 1;
        BASE_NS::vector<ISceneNode*> nodes(numNodes);
        for (uint32_t i = 0; i < numNodes; ++i) {
            nodes[i] = nodeSystem->CreateNode();
            nodes[i]->SetName(to_string(i));
            if (i > 0) {
                const uint32_t parentIdx = (i - 1) >> 1;
                nodes[i]->SetParent(*nodes[parentIdx]);
            }
        }

        // Clone it
        const ISceneNode* rootNode = nodes[0];
        auto rootClone = nodeSystem->CloneNode(*rootNode, true);
        BASE_NS::vector<ISceneNode*> clones(numNodes);
        clones[0] = rootClone;

        // Check cloned tree structure
        constexpr uint32_t nonLeafNodes = numNodes >> 1;
        for (uint32_t i = 0; i < nonLeafNodes; ++i) {
            const uint32_t leftChildIdx = (i << 1) + 1;
            const uint32_t rightChildIdx = (i << 1) + 2;
            ASSERT_EQ(2u, clones[i]->GetChildren().size());
            clones[leftChildIdx] = clones[i]->GetChildren()[0];
            clones[rightChildIdx] = clones[i]->GetChildren()[1];
        }

        // Check that leaves have no children
        for (uint32_t i = nonLeafNodes; i < numNodes; ++i) {
            EXPECT_TRUE(clones[i]->GetChildren().empty());
        }

        // Check cloned components
        for (uint32_t i = 0; i < numNodes; ++i) {
            EXPECT_EQ(to_string(i), clones[i]->GetName());
        }
    }
    // Chain
    {
        // Create chain tree
        constexpr uint32_t depth = 1000u;
        BASE_NS::vector<ISceneNode*> nodes(depth);
        for (uint32_t i = 0; i < depth; ++i) {
            nodes[i] = nodeSystem->CreateNode();
            nodes[i]->SetName(to_string(i));
            if (i > 0) {
                const uint32_t parentIdx = i - 1;
                nodes[i]->SetParent(*nodes[parentIdx]);
            }
        }

        // Clone it
        const ISceneNode* rootNode = nodes[0];
        auto rootClone = nodeSystem->CloneNode(*rootNode, true);
        BASE_NS::vector<ISceneNode*> clones(depth);
        clones[0] = rootClone;

        // Check cloned tree structure
        constexpr uint32_t nonLeafNodes = depth - 1;
        for (uint32_t i = 0; i < nonLeafNodes; ++i) {
            const uint32_t childIdx = i + 1;
            ASSERT_EQ(1u, clones[i]->GetChildren().size());
            clones[childIdx] = clones[i]->GetChildren()[0];
        }

        // Check that leaf has no children
        EXPECT_TRUE(clones[depth - 1]->GetChildren().empty());

        // Check cloned components
        for (uint32_t i = 0; i < depth; ++i) {
            EXPECT_EQ(to_string(i), clones[i]->GetName());
        }
    }
}

/**
 * @tc.name: IsAncestorTest
 * @tc.desc: Tests for Is Ancestor Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsSceneNode, IsAncestorTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);

    auto node1 = nodeSystem->CreateNode();
    auto node2 = nodeSystem->CreateNode();
    auto node3 = nodeSystem->CreateNode();
    auto node4 = nodeSystem->CreateNode();

    node2->SetParent(*node1);
    node3->SetParent(*node2);
    node4->SetParent(*node1);

    EXPECT_TRUE(node1->IsAncestorOf(*node2));
    EXPECT_TRUE(node1->IsAncestorOf(*node3));
    EXPECT_TRUE(node1->IsAncestorOf(*node4));

    EXPECT_FALSE(node2->IsAncestorOf(*node1));
    EXPECT_TRUE(node2->IsAncestorOf(*node3));
    EXPECT_FALSE(node2->IsAncestorOf(*node4));

    EXPECT_FALSE(node3->IsAncestorOf(*node1));
    EXPECT_FALSE(node3->IsAncestorOf(*node2));
    EXPECT_FALSE(node3->IsAncestorOf(*node4));

    EXPECT_FALSE(node4->IsAncestorOf(*node1));
    EXPECT_FALSE(node4->IsAncestorOf(*node2));
    EXPECT_FALSE(node4->IsAncestorOf(*node3));
}

/**
 * @tc.name: GetNode
 * @tc.desc: Tests for Get Node. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsSceneNode, GetNode, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);

    // getting with a invalid/default entity should not find anything
    EXPECT_FALSE(nodeSystem->GetNode(Entity {}));

    // getting with an entity without NodeComponent should not find anything
    const auto entity = ecs->GetEntityManager().Create();
    EXPECT_FALSE(nodeSystem->GetNode(entity));

    // getting with the nodes entity should find the same node
    auto node1 = nodeSystem->CreateNode();
    ASSERT_NE(nullptr, node1);
    const auto node1Entity = node1->GetEntity();
    EXPECT_EQ(node1, nodeSystem->GetNode(node1Entity));

    // getting with the nodes entity should find the same node
    auto node2 = nodeSystem->CreateNode();
    ASSERT_NE(nullptr, node2);
    const auto node2Entity = node2->GetEntity();
    EXPECT_EQ(node2, nodeSystem->GetNode(node2Entity));

    node2->SetParent(*node1);

    // changing parent should not affect the result
    ASSERT_NE(nullptr, node1);
    EXPECT_EQ(node1, nodeSystem->GetNode(node1Entity));
    ASSERT_NE(nullptr, node2);
    EXPECT_EQ(node2, nodeSystem->GetNode(node2Entity));

    // after destroying should not find the nodes anymore
    nodeSystem->DestroyNode(*node1);
    EXPECT_EQ(nullptr, nodeSystem->GetNode(node1Entity));
    EXPECT_EQ(nullptr, nodeSystem->GetNode(node2Entity));
}

/**
 * @tc.name: LookupByComponentTest
 * @tc.desc: Tests for Lookup By Component Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsSceneNode, LookupByComponentTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);
    auto materialManager = GetManager<IMaterialComponentManager>(*ecs);

    auto node1 = nodeSystem->CreateNode();
    auto node2 = nodeSystem->CreateNode();
    auto node3 = nodeSystem->CreateNode();
    auto node4 = nodeSystem->CreateNode();
    auto node5 = nodeSystem->CreateNode();

    node2->SetParent(*node1);
    node3->SetParent(*node2);
    node5->SetParent(*node2);
    node4->SetParent(*node1);

    materialManager->Create(node3->GetEntity());
    materialManager->Create(node5->GetEntity());

    EXPECT_NE(nullptr, node2->LookupNodeByComponent(*materialManager));
    EXPECT_NE(nullptr, ((const ISceneNode*)node2)->LookupNodeByComponent(*materialManager));
    EXPECT_EQ(nullptr, node4->LookupNodeByComponent(*materialManager));
    EXPECT_EQ(nullptr, ((const ISceneNode*)node4)->LookupNodeByComponent(*materialManager));
    EXPECT_EQ(2u, node1->LookupNodesByComponent(*materialManager).size());
    EXPECT_EQ(2u, ((const ISceneNode*)node1)->LookupNodesByComponent(*materialManager).size());
    EXPECT_EQ(0u, node4->LookupNodesByComponent(*materialManager).size());
    EXPECT_EQ(0u, ((const ISceneNode*)node4)->LookupNodesByComponent(*materialManager).size());
}

/**
 * @tc.name: LookupByPathTest
 * @tc.desc: Tests for Lookup By Path Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsSceneNode, LookupByPathTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);

    auto node1 = nodeSystem->CreateNode();
    auto node2 = nodeSystem->CreateNode();
    auto node3 = nodeSystem->CreateNode();
    auto node4 = nodeSystem->CreateNode();
    auto node5 = nodeSystem->CreateNode();

    node2->SetParent(*node1);
    node3->SetParent(*node2);
    node5->SetParent(*node2);
    node4->SetParent(*node1);
    /*
       1
     2   4
    3 5
    */
    node1->SetName("node1");
    node2->SetName("node2");
    node3->SetName("node3");
    node4->SetName("node4");
    node5->SetName("node5");

    EXPECT_EQ(node2, node1->LookupNodeByPath("node2"));
    EXPECT_EQ(node2, ((const ISceneNode*)node1)->LookupNodeByPath("node2"));
    EXPECT_EQ(node3, node1->LookupNodeByPath("node2/node3"));
    EXPECT_EQ(node3, ((const ISceneNode*)node1)->LookupNodeByPath("node2/node3"));
    EXPECT_EQ(node4, node1->LookupNodeByPath("node4"));
    EXPECT_EQ(node4, ((const ISceneNode*)node1)->LookupNodeByPath("node4"));
    EXPECT_EQ(node5, node1->LookupNodeByPath("node2/node5"));
    EXPECT_EQ(node5, ((const ISceneNode*)node1)->LookupNodeByPath("node2/node5"));
    EXPECT_EQ(nullptr, node1->LookupNodeByPath("node5"));
    EXPECT_EQ(nullptr, ((const ISceneNode*)node1)->LookupNodeByPath("node5"));
    EXPECT_EQ(nullptr, node1->LookupNodeByPath(""));
    EXPECT_EQ(nullptr, ((const ISceneNode*)node1)->LookupNodeByPath(""));
}

/**
 * @tc.name: LookupByNameTest
 * @tc.desc: Tests for Lookup By Name Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsSceneNode, LookupByNameTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);

    auto node1 = nodeSystem->CreateNode();
    auto node2 = nodeSystem->CreateNode();
    auto node3 = nodeSystem->CreateNode();
    auto node4 = nodeSystem->CreateNode();
    auto node5 = nodeSystem->CreateNode();

    node2->SetParent(*node1);
    node3->SetParent(*node2);
    node5->SetParent(*node2);
    node4->SetParent(*node1);

    node1->SetName("node1");
    node2->SetName("node2");
    node3->SetName("node3");
    node4->SetName("node4");
    node5->SetName("node5");

    EXPECT_EQ(node3, node1->LookupNodeByName("node3"));
    EXPECT_EQ(node3, ((const ISceneNode*)node1)->LookupNodeByName("node3"));
    EXPECT_EQ(nullptr, node1->LookupNodeByName("node9"));
    EXPECT_EQ(nullptr, ((const ISceneNode*)node1)->LookupNodeByName("node9"));
    EXPECT_EQ(node5, node2->LookupNodeByName("node5"));
    EXPECT_EQ(node5, ((const ISceneNode*)node2)->LookupNodeByName("node5"));
}

/**
 * @tc.name: EffectivelyEnabledTest
 * @tc.desc: Tests for Effectively Enabled Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsNodeSystem, EffectivelyEnabledTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);
    {
        auto node1 = nodeSystem->CreateNode();
        auto node2 = nodeSystem->CreateNode();
        auto node3 = nodeSystem->CreateNode();
        auto node4 = nodeSystem->CreateNode();
        auto node5 = nodeSystem->CreateNode();

        node2->SetParent(*node1);
        node3->SetParent(*node2);
        node5->SetParent(*node2);
        node4->SetParent(*node1);

        ecs->Update(1u, 1u);
        node2->SetEnabled(false); // disables 2, and its children 3 and 5
        ecs->Update(2u, 1u);

        EXPECT_TRUE(node1->GetEffectivelyEnabled());
        EXPECT_FALSE(node2->GetEffectivelyEnabled());
        EXPECT_FALSE(node3->GetEffectivelyEnabled());
        EXPECT_TRUE(node4->GetEffectivelyEnabled());
        EXPECT_FALSE(node5->GetEffectivelyEnabled());

        node3->SetEnabled(false); // disables 3, but already disabled due to parent
        ecs->Update(3u, 1u);

        EXPECT_TRUE(node1->GetEffectivelyEnabled());
        EXPECT_FALSE(node2->GetEffectivelyEnabled());
        EXPECT_FALSE(node3->GetEffectivelyEnabled());
        EXPECT_TRUE(node4->GetEffectivelyEnabled());
        EXPECT_FALSE(node5->GetEffectivelyEnabled());

        node3->SetEnabled(true); // enables 3, but still disabled due to parent
        ecs->Update(4u, 1u);

        EXPECT_TRUE(node1->GetEffectivelyEnabled());
        EXPECT_FALSE(node2->GetEffectivelyEnabled());
        EXPECT_FALSE(node3->GetEffectivelyEnabled());
        EXPECT_TRUE(node4->GetEffectivelyEnabled());
        EXPECT_FALSE(node5->GetEffectivelyEnabled());

        node2->SetEnabled(true); // enables 2, and its children 3 and 5
        ecs->Update(5u, 1u);

        EXPECT_TRUE(node1->GetEffectivelyEnabled());
        EXPECT_TRUE(node2->GetEffectivelyEnabled());
        EXPECT_TRUE(node3->GetEffectivelyEnabled());
        EXPECT_TRUE(node4->GetEffectivelyEnabled());
        EXPECT_TRUE(node5->GetEffectivelyEnabled());
        nodeSystem->DestroyNode(*node1);
    }
    {
        auto node1 = nodeSystem->CreateNode();
        node1->SetEnabled(false);
        auto node2 = nodeSystem->CreateNode();
        node2->SetEnabled(false);
        node2->SetParent(*node1);

        ecs->ProcessEvents();
        ecs->Update(1u, 1u);

        // adding an enabled node in a disabled hiearchy should result in a effectively disable node.
        auto node3 = nodeSystem->CreateNode();
        node3->SetEnabled(true);
        node3->SetParent(*node2);

        ecs->ProcessEvents();
        ecs->Update(2u, 1u);

        EXPECT_FALSE(node1->GetEffectivelyEnabled());
        EXPECT_FALSE(node2->GetEffectivelyEnabled());
        EXPECT_FALSE(node3->GetEffectivelyEnabled());
        nodeSystem->DestroyNode(*node1);
    }
}

/**
 * @tc.name: AddChild
 * @tc.desc: Tests for Add Child. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsNodeSystem, AddChild, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);

    // Create tree
    constexpr uint32_t depth = 3u;
    constexpr uint32_t numNodes = (1u << depth) - 1;
    BASE_NS::vector<ISceneNode*> nodes(numNodes);
    for (uint32_t i = 0; i < numNodes; ++i) {
        nodes[i] = nodeSystem->CreateNode();
        nodes[i]->SetName(to_string(i));
        if (i > 0) {
            const uint32_t parentIdx = (i - 1) >> 1;
            nodes[parentIdx]->AddChild(*nodes[i]);
        }
    }
    auto children = nodeSystem->GetRootNode().GetChildren();
    ASSERT_EQ(children.size(), 1U);
    EXPECT_EQ(children[0], nodes[0U]);
    auto grandChildren = children[0]->GetChildren();
    ASSERT_EQ(grandChildren.size(), 2U);
    EXPECT_EQ(grandChildren[0], nodes[1U]);
    EXPECT_EQ(grandChildren[1], nodes[2U]);
    {
        auto greatGrandChildren = grandChildren[0]->GetChildren();
        ASSERT_EQ(greatGrandChildren.size(), 2U);
        EXPECT_EQ(greatGrandChildren[0], nodes[3U]);
        EXPECT_EQ(greatGrandChildren[1], nodes[4U]);
    }
    {
        auto greatGrandChildren = grandChildren[1]->GetChildren();
        ASSERT_EQ(greatGrandChildren.size(), 2U);
        EXPECT_EQ(greatGrandChildren[0], nodes[5U]);
        EXPECT_EQ(greatGrandChildren[1], nodes[6U]);
    }
    nodeSystem->DestroyNode(*nodes[0U]);
}

/**
 * @tc.name: InsertChild
 * @tc.desc: Tests for Insert Child. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsNodeSystem, InsertChild, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);

    auto node1 = nodeSystem->CreateNode();
    auto node2 = nodeSystem->CreateNode();
    auto node3 = nodeSystem->CreateNode();
    auto node4 = nodeSystem->CreateNode();
    auto node5 = nodeSystem->CreateNode();
    node1->InsertChild(0U, *node2); // 2
    node1->InsertChild(1U, *node3); // 2 3
    node1->InsertChild(1U, *node4); // 2 4 3
    node1->InsertChild(2U, *node5); // 2 4 5 3
    {
        auto children = node1->GetChildren();
        ASSERT_EQ(children.size(), 4U);
        EXPECT_EQ(children[0U], node2);
        EXPECT_EQ(children[1U], node4);
        EXPECT_EQ(children[2U], node5);
        EXPECT_EQ(children[3U], node3);
    }
    node1->RemoveChild(2U);         // 2 4 3
    node1->InsertChild(3U, *node5); // 2 4 3 5
    node1->RemoveChild(1U);         // 2 3 5
    node1->InsertChild(2U, *node4); // 2 3 4 5
    {
        auto children = node1->GetChildren();
        ASSERT_EQ(children.size(), 4U);
        EXPECT_EQ(children[0U], node2);
        EXPECT_EQ(children[1U], node3);
        EXPECT_EQ(children[2U], node4);
        EXPECT_EQ(children[3U], node5);
    }
    node1->RemoveChild(*node4);
    {
        auto children = node1->GetChildren();
        ASSERT_EQ(children.size(), 3U);
        EXPECT_EQ(children[0U], node2);
        EXPECT_EQ(children[1U], node3);
        EXPECT_EQ(children[2U], node5);
    }
    node1->RemoveChildren();
    {
        auto children = node1->GetChildren();
        ASSERT_EQ(children.size(), 0U);
    }
}

struct Listener : public CORE3D_NS::INodeSystem::SceneNodeListener {
    struct Event {
        const ISceneNode* parent;
        EventType type;
        const ISceneNode* child;
        size_t index;
    };
    BASE_NS::vector<Event> events;
    void OnChildChanged(const ISceneNode& parent, EventType type, const ISceneNode& child, size_t index) override
    {
        auto& event = events.emplace_back();
        event.parent = &parent;
        event.type = type;
        event.child = &child;
        event.index = index;
    }
};

/**
 * @tc.name: SceneNodeListener
 * @tc.desc: Tests for Scene Node Listener. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_EcsNodeSystem, SceneNodeListener, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto nodeSystem = GetSystem<INodeSystem>(*ecs);
    ASSERT_NE(nullptr, nodeSystem);

    auto listener = Listener {};
    nodeSystem->AddListener(listener);

    constexpr uint32_t numNodes = 5;
    BASE_NS::vector<ISceneNode*> nodes(numNodes);
    for (uint32_t i = 0; i < numNodes; ++i) {
        nodes[i] = nodeSystem->CreateNode();
        nodes[i]->SetName(to_string(i));
        if (i > 0) {
            const uint32_t parentIdx = (i - 1) >> 1;
            nodes[parentIdx]->AddChild(*nodes[i]);
        }
    }
    auto& root = nodeSystem->GetRootNode();
    ASSERT_EQ(listener.events.size(), 8U);

    EXPECT_EQ(listener.events[0].parent, &root);
    EXPECT_EQ(listener.events[0].type, INodeSystem::SceneNodeListener::EventType::REMOVED);
    EXPECT_EQ(listener.events[0].child, nodes[1]);
    EXPECT_EQ(listener.events[0].index, 1U);

    EXPECT_EQ(listener.events[1].parent, nodes[0]);
    EXPECT_EQ(listener.events[1].type, INodeSystem::SceneNodeListener::EventType::ADDED);
    EXPECT_EQ(listener.events[1].child, nodes[1]);
    EXPECT_EQ(listener.events[1].index, 0U);

    EXPECT_EQ(listener.events[2].parent, &root);
    EXPECT_EQ(listener.events[2].type, INodeSystem::SceneNodeListener::EventType::REMOVED);
    EXPECT_EQ(listener.events[2].child, nodes[2]);
    EXPECT_EQ(listener.events[2].index, 1U);

    EXPECT_EQ(listener.events[3].parent, nodes[0]);
    EXPECT_EQ(listener.events[3].type, INodeSystem::SceneNodeListener::EventType::ADDED);
    EXPECT_EQ(listener.events[3].child, nodes[2]);
    EXPECT_EQ(listener.events[3].index, 1U);

    EXPECT_EQ(listener.events[4].parent, &root);
    EXPECT_EQ(listener.events[4].type, INodeSystem::SceneNodeListener::EventType::REMOVED);
    EXPECT_EQ(listener.events[4].child, nodes[3]);
    EXPECT_EQ(listener.events[4].index, 1U);

    EXPECT_EQ(listener.events[5].parent, nodes[1]);
    EXPECT_EQ(listener.events[5].type, INodeSystem::SceneNodeListener::EventType::ADDED);
    EXPECT_EQ(listener.events[5].child, nodes[3]);
    EXPECT_EQ(listener.events[5].index, 0U);

    EXPECT_EQ(listener.events[6].parent, &root);
    EXPECT_EQ(listener.events[6].type, INodeSystem::SceneNodeListener::EventType::REMOVED);
    EXPECT_EQ(listener.events[6].child, nodes[4]);
    EXPECT_EQ(listener.events[6].index, 1U);

    EXPECT_EQ(listener.events[7].parent, nodes[1]);
    EXPECT_EQ(listener.events[7].type, INodeSystem::SceneNodeListener::EventType::ADDED);
    EXPECT_EQ(listener.events[7].child, nodes[4]);
    EXPECT_EQ(listener.events[7].index, 1U);

    listener.events.clear();

    nodes[0]->AddChild(*nodes[4]);

    ASSERT_EQ(listener.events.size(), 2U);

    EXPECT_EQ(listener.events[0].parent, nodes[1]);
    EXPECT_EQ(listener.events[0].type, INodeSystem::SceneNodeListener::EventType::REMOVED);
    EXPECT_EQ(listener.events[0].child, nodes[4]);
    EXPECT_EQ(listener.events[0].index, 1U);

    EXPECT_EQ(listener.events[1].parent, nodes[0]);
    EXPECT_EQ(listener.events[1].type, INodeSystem::SceneNodeListener::EventType::ADDED);
    EXPECT_EQ(listener.events[1].child, nodes[4]);
    EXPECT_EQ(listener.events[1].index, 2U);

    nodeSystem->RemoveListener(listener);
}

/**
 * @tc.name: VerifyThatNodesystemIsBroken
 * @tc.desc: Tests for Verify That Nodesystem Is Broken. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(ScenePlugin, VerifyThatNodesystemIsBroken, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& entityManager = ecs->GetEntityManager();
    auto nodeSystem = GetSystem<INodeSystem>(*ecs);

    auto root = nodeSystem->CreateNode();
    root->SetName("root");

    auto child1 = nodeSystem->CreateNode();
    child1->SetName("child1");
    root->AddChild(*child1);

    auto child2 = nodeSystem->CreateNode();
    child2->SetName("child2");
    child1->AddChild(*child2);

    auto entity = child1->GetEntity();
    ASSERT_TRUE(root->LookupNodeByPath("child1/child2"));
    entityManager.SetActive(entity, false);
    ASSERT_FALSE(root->LookupNodeByPath("child1/child2"));
    entityManager.SetActive(entity, true);
    ASSERT_TRUE(root->LookupNodeByPath("child1/child2"));
}