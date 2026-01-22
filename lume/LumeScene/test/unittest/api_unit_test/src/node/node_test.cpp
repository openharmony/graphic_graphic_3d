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

#include <scene/api/node.h>
#include <scene/api/scene.h>
#include <scene/ext/scene_utils.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_render_configuration.h>

#include <meta/api/make_callback.h>
#include <meta/interface/intf_class_registry.h>
#include <meta/interface/intf_containable.h>

#include "../compare_util.h"
#include "node/util.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginNodeTest : public ScenePluginTest {
public:
    void CloneNodeTest(
        IScene::Ptr scene, BASE_NS::string_view name, INode::Ptr parent, BASE_NS::string_view expectedName);
};

/**
 * @tc.name: NodeMembers
 * @tc.desc: Tests for Node Members. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, NodeMembers, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(node);

    EXPECT_TRUE(node->Enabled()->GetValue());
    EXPECT_EQ(node->GetScene(), scene);
    EXPECT_EQ(node->GetPath().GetResult(), "//test");
    EXPECT_EQ(node->GetParent().GetResult(), scene->GetRootNode().GetResult());
    EXPECT_TRUE(node->GetChildren().GetResult().empty());

    CheckNativeEcsPath(node, "//test");

    EXPECT_TRUE(META_NS::SetName(node, "name"));
    auto object = interface_pointer_cast<META_NS::IObject>(node);
    ASSERT_TRUE(object);
    EXPECT_EQ(object->GetName(), "name");

    EXPECT_TRUE(node->Enabled()->IsDefaultValue());
    node->Enabled()->SetValue(false);
    EXPECT_FALSE(node->Enabled()->IsDefaultValue());
    node->Enabled()->ResetValue();
    EXPECT_TRUE(node->Enabled()->IsDefaultValue());
}

/**
 * @tc.name: Enabled
 * @tc.desc: Tests for Enabled. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, Enabled, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto root = scene->GetRootNode().GetResult();
    auto node = scene->CreateNode("//test").GetResult();
    auto c1 = scene->CreateNode("//other").GetResult();
    auto c2 = scene->CreateNode("//other/child").GetResult();
    auto c3 = scene->CreateNode("//other/child/child2").GetResult();
    ASSERT_TRUE(root);
    ASSERT_TRUE(node);
    ASSERT_TRUE(c1);
    ASSERT_TRUE(c2);
    ASSERT_TRUE(c3);

    c1->Enabled()->SetValue(false);
    UpdateScene();
    WaitForUserQueue();
    EXPECT_TRUE(node->IsEnabledInHierarchy().GetResult());
    EXPECT_FALSE(c1->IsEnabledInHierarchy().GetResult());
    EXPECT_FALSE(c2->IsEnabledInHierarchy().GetResult());
    EXPECT_FALSE(c3->IsEnabledInHierarchy().GetResult());

    c3->Enabled()->SetValue(false);
    c1->Enabled()->SetValue(true);
    UpdateScene();
    WaitForUserQueue();
    auto i = scene->GetInternalScene();
    EXPECT_TRUE(node->IsEnabledInHierarchy().GetResult());
    EXPECT_TRUE(c1->IsEnabledInHierarchy().GetResult());
    EXPECT_TRUE(c2->IsEnabledInHierarchy().GetResult());
    EXPECT_FALSE(c3->IsEnabledInHierarchy().GetResult());

    root->Enabled()->SetValue(false);
    UpdateScene();
    WaitForUserQueue();
    EXPECT_FALSE(node->IsEnabledInHierarchy().GetResult());
    EXPECT_FALSE(c1->IsEnabledInHierarchy().GetResult());
    EXPECT_FALSE(c2->IsEnabledInHierarchy().GetResult());
    EXPECT_FALSE(c3->IsEnabledInHierarchy().GetResult());
}

/**
 * @tc.name: Active
 * @tc.desc: Tests for Active. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, Active, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto c1 = scene->CreateNode("//other").GetResult();
    auto c2 = scene->CreateNode("//other/child").GetResult();
    ASSERT_TRUE(c1);
    ASSERT_TRUE(c2);

    auto c1acc = interface_cast<IEcsObjectAccess>(c1);
    c1acc->GetEcsObject()->SetActive(false).Wait();
    UpdateScene();

    EXPECT_FALSE(scene->FindNode("//other").GetResult());
    EXPECT_FALSE(scene->FindNode("//other/child").GetResult());

    c1acc->GetEcsObject()->SetActive(true).Wait();
    UpdateScene();

    EXPECT_TRUE(scene->FindNode("//other").GetResult());
    EXPECT_TRUE(scene->FindNode("//other/child").GetResult());
}

/**
 * @tc.name: NodeChild
 * @tc.desc: Tests for Node Child. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, NodeChild, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    auto child = scene->CreateNode("//child").GetResult();
    ASSERT_TRUE(node);
    ASSERT_TRUE(child);

    auto root = scene->GetRootNode().GetResult();
    ASSERT_TRUE(root);

    EXPECT_EQ(root->GetChildren().GetResult().size(), 2);
    EXPECT_TRUE(node->AddChild(child).GetResult());
    EXPECT_EQ(root->GetChildren().GetResult().size(), 1);

    EXPECT_EQ(child->GetPath().GetResult(), "//test/child");
    EXPECT_EQ(child->GetParent().GetResult(), node);
    auto cs = node->GetChildren().GetResult();
    ASSERT_EQ(cs.size(), 1);
    EXPECT_EQ(cs[0], child);

    CheckNativeEcsPath(child, "//test/child");

    EXPECT_TRUE(node->RemoveChild(child).GetResult());
    EXPECT_TRUE(node->GetChildren().GetResult().empty());
}

/**
 * @tc.name: EcsConnection
 * @tc.desc: Tests for Ecs Connection. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, EcsConnection, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(node);

    EXPECT_TRUE(node->Enabled()->GetValue());
    node->Enabled()->SetValue(false);
    EXPECT_FALSE(node->Enabled()->GetValue());

    UpdateScene();

    EXPECT_FALSE(node->Enabled()->GetValue());
    CheckNativePropertyValue<bool>(node->Enabled(), false);

    node->Enabled()->SetValue(true);

    UpdateScene();

    EXPECT_TRUE(node->Enabled()->GetValue());
    CheckNativePropertyValue<bool>(node->Enabled(), true);
}

/**
 * @tc.name: RootNodeRename
 * @tc.desc: Tests for Root Node Rename. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, RootNodeRename, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    auto child = scene->CreateNode("//child").GetResult();
    ASSERT_TRUE(node);
    ASSERT_TRUE(child);

    auto root = scene->GetRootNode().GetResult();
    EXPECT_EQ(node->GetParent().GetResult(), root);
    EXPECT_EQ(child->GetParent().GetResult(), root);

    EXPECT_FALSE(root->GetParent().GetResult());
    EXPECT_TRUE(META_NS::SetName(root, "hipshops"));
    UpdateScene();
    EXPECT_FALSE(root->GetParent().GetResult());
    EXPECT_TRUE(scene->FindNode("/hipshops").GetResult());

    EXPECT_EQ(root->GetChildren().GetResult().size(), 2);
    EXPECT_TRUE(node->AddChild(child).GetResult());
    EXPECT_EQ(root->GetChildren().GetResult().size(), 1);

    EXPECT_EQ(child->GetPath().GetResult(), "/hipshops/test/child");

    EXPECT_EQ(scene->GetRootNode().GetResult(), root);
    EXPECT_EQ(node->GetParent().GetResult(), root);
    EXPECT_EQ(child->GetParent().GetResult(), node);
}

/**
 * @tc.name: NodeIteration
 * @tc.desc: Tests for Node Iteration. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, NodeIteration, testing::ext::TestSize.Level1)
{
    using namespace testing;

    auto scene = CreateEmptyScene();
    auto root = scene->GetRootNode().GetResult();
    auto node = scene->CreateNode("//test").GetResult();
    auto child1 = scene->CreateNode("//child").GetResult();
    auto child2 = scene->CreateNode("//test/child").GetResult();
    auto child3 = scene->CreateNode("//test/child2").GetResult();
    ASSERT_TRUE(root);
    ASSERT_TRUE(node);
    ASSERT_TRUE(child1);
    ASSERT_TRUE(child2);
    ASSERT_TRUE(child3);

    {
        BASE_NS::vector<META_NS::IObject::Ptr> res;
        ConstIterate(
            interface_pointer_cast<META_NS::IIterable>(root),
            [&](META_NS::IObject::Ptr obj) {
                res.push_back(obj);
                return true;
            },
            META_NS::IterateStrategy { META_NS::TraversalType::NO_HIERARCHY });

        ASSERT_EQ(res.size(), 2);
        EXPECT_EQ(res[0], interface_pointer_cast<META_NS::IObject>(node));
        EXPECT_EQ(res[1], interface_pointer_cast<META_NS::IObject>(child1));
    }
    {
        BASE_NS::vector<META_NS::IObject::Ptr> res;
        ConstIterate(
            interface_pointer_cast<META_NS::IIterable>(root),
            [&](META_NS::IObject::Ptr obj) {
                res.push_back(obj);
                return true;
            },
            META_NS::IterateStrategy { META_NS::TraversalType::FULL_HIERARCHY });

        ASSERT_EQ(res.size(), 4);
        EXPECT_THAT(res, Contains(interface_pointer_cast<META_NS::IObject>(node)));
        EXPECT_THAT(res, Contains(interface_pointer_cast<META_NS::IObject>(child1)));
        EXPECT_THAT(res, Contains(interface_pointer_cast<META_NS::IObject>(child2)));
        EXPECT_THAT(res, Contains(interface_pointer_cast<META_NS::IObject>(child3)));
    }
    {
        // free cached nodes
        node.reset();
        child1.reset();
        child2.reset();
        child3.reset();
        scene->ReleaseNode(BASE_NS::move(root), true).Wait();

        root = scene->GetRootNode().GetResult();

        BASE_NS::vector<META_NS::IObject::Ptr> res;
        ConstIterate(
            interface_pointer_cast<META_NS::IIterable>(root),
            [&](META_NS::IObject::Ptr obj) {
                res.push_back(obj);
                return true;
            },
            META_NS::IterateStrategy { META_NS::TraversalType::FULL_HIERARCHY });

        node = scene->FindNode("//test").GetResult();
        child1 = scene->FindNode("//child").GetResult();
        child2 = scene->FindNode("//test/child").GetResult();
        child3 = scene->FindNode("//test/child2").GetResult();
        ASSERT_TRUE(node);
        ASSERT_TRUE(child1);
        ASSERT_TRUE(child2);
        ASSERT_TRUE(child3);

        ASSERT_EQ(res.size(), 4);

        EXPECT_THAT(res, Contains(interface_pointer_cast<META_NS::IObject>(node)));
        EXPECT_THAT(res, Contains(interface_pointer_cast<META_NS::IObject>(child1)));
        EXPECT_THAT(res, Contains(interface_pointer_cast<META_NS::IObject>(child2)));
        EXPECT_THAT(res, Contains(interface_pointer_cast<META_NS::IObject>(child3)));
    }
}

static bool operator==(const META_NS::ChildChangedInfo& lhs, const META_NS::ChildChangedInfo& rhs)
{
    return lhs.object == rhs.object && lhs.to == rhs.to && lhs.from == rhs.from &&
           lhs.parent.lock() == rhs.parent.lock();
}

/**
 * @tc.name: NodeContrainerEvents
 * @tc.desc: Tests for Node Contrainer Events. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, NodeContrainerEvents, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    auto child1 = scene->CreateNode("//test/child1").GetResult();
    auto child2 = scene->CreateNode("//test/child2").GetResult();

    auto nodeCont = interface_pointer_cast<META_NS::IContainer>(node);
    ASSERT_TRUE(nodeCont);

    BASE_NS::vector<META_NS::ChildChangedInfo> changes;
    nodeCont->OnContainerChanged()->AddHandler(
        META_NS::MakeCallback<META_NS::IOnChildChanged>([&](auto const& i) { changes.push_back(i); }));

    {
        node->RemoveChild(child1).Wait();
        WaitForUserQueue();
        ASSERT_EQ(changes.size(), 1);
        auto expected = META_NS::ChildChangedInfo { META_NS::ContainerChangeType::REMOVED,
            interface_pointer_cast<META_NS::IObject>(child1), nodeCont, 0, size_t(-1) };
        EXPECT_TRUE(changes[0] == expected);
        changes.clear();
    }
    {
        auto child3 = scene->CreateNode("//test/child3").GetResult();
        WaitForUserQueue();
        ASSERT_EQ(changes.size(), 1);
        auto expected = META_NS::ChildChangedInfo { META_NS::ContainerChangeType::ADDED,
            interface_pointer_cast<META_NS::IObject>(child3), nodeCont, size_t(-1), 1 };
        EXPECT_TRUE(changes[0] == expected);
        changes.clear();
    }
    {
        node->AddChild(child1, 1).Wait();
        WaitForUserQueue();
        ASSERT_EQ(changes.size(), 1);
        auto expected = META_NS::ChildChangedInfo { META_NS::ContainerChangeType::ADDED,
            interface_pointer_cast<META_NS::IObject>(child1), nodeCont, size_t(-1), 1 };
        EXPECT_TRUE(changes[0] == expected);
        changes.clear();
    }

    child1.reset();
    child2.reset();
    nodeCont.reset();

    scene->ReleaseNode(BASE_NS::move(node), true).Wait();
}

/**
 * @tc.name: NodeContrainerEventsImportScene
 * @tc.desc: Tests for Node Contrainer Events Import Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, NodeContrainerEventsImportScene, testing::ext::TestSize.Level1)
{
    auto celia = LoadScene("test://celia/Celia.gltf");
    ASSERT_TRUE(celia);

    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();

    auto nodeCont = interface_pointer_cast<META_NS::IContainer>(node);
    ASSERT_TRUE(nodeCont);

    BASE_NS::vector<META_NS::ChildChangedInfo> changes;
    nodeCont->OnContainerChanged()->AddHandler(
        META_NS::MakeCallback<META_NS::IOnChildChanged>([&](auto const& i) { changes.push_back(i); }));

    auto ext = interface_cast<INodeImport>(node);
    ASSERT_TRUE(ext);
    auto newNode = ext->ImportChildScene(celia, "some").GetResult();
    ASSERT_TRUE(newNode);

    WaitForUserQueue();
    ASSERT_EQ(changes.size(), 1);
    auto expected = META_NS::ChildChangedInfo { META_NS::ContainerChangeType::ADDED,
        interface_pointer_cast<META_NS::IObject>(newNode), nodeCont, size_t(-1), 0 };
    EXPECT_TRUE(changes[0] == expected);
}

/**
 * @tc.name: NodeContrainerEventsImportNode
 * @tc.desc: Tests for Node Contrainer Events Import Node. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, NodeContrainerEventsImportNode, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    auto otherScene = CreateEmptyScene();
    auto child = otherScene->CreateNode("//child").GetResult();

    auto nodeCont = interface_pointer_cast<META_NS::IContainer>(node);
    ASSERT_TRUE(nodeCont);

    BASE_NS::vector<META_NS::ChildChangedInfo> changes;
    nodeCont->OnContainerChanged()->AddHandler(
        META_NS::MakeCallback<META_NS::IOnChildChanged>([&](auto const& i) { changes.push_back(i); }));

    auto ext = interface_cast<INodeImport>(node);
    ASSERT_TRUE(ext);
    auto newNode = ext->ImportChild(child).GetResult();
    ASSERT_TRUE(newNode);

    WaitForUserQueue();
    ASSERT_EQ(changes.size(), 1);
    auto expected = META_NS::ChildChangedInfo { META_NS::ContainerChangeType::ADDED,
        interface_pointer_cast<META_NS::IObject>(newNode), nodeCont, size_t(-1), 0 };
    EXPECT_TRUE(changes[0] == expected);
}

/**
 * @tc.name: NodeContrainerEventsReparent
 * @tc.desc: Tests for Node Contrainer Events Reparent. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, NodeContrainerEventsReparent, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto root = scene->GetRootNode().GetResult();
    auto node = scene->CreateNode("//test").GetResult();
    auto child = scene->CreateNode("//child").GetResult();

    auto rootCont = interface_pointer_cast<META_NS::IContainer>(root);
    ASSERT_TRUE(rootCont);

    auto nodeCont = interface_pointer_cast<META_NS::IContainer>(node);
    ASSERT_TRUE(nodeCont);

    BASE_NS::vector<META_NS::ChildChangedInfo> rootChanges;
    rootCont->OnContainerChanged()->AddHandler(
        META_NS::MakeCallback<META_NS::IOnChildChanged>([&](auto const& i) { rootChanges.push_back(i); }));

    BASE_NS::vector<META_NS::ChildChangedInfo> nodeChanges;
    nodeCont->OnContainerChanged()->AddHandler(
        META_NS::MakeCallback<META_NS::IOnChildChanged>([&](auto const& i) { nodeChanges.push_back(i); }));

    {
        node->AddChild(child).Wait();
        WaitForUserQueue();
        ASSERT_EQ(nodeChanges.size(), 1);
        auto expected1 = META_NS::ChildChangedInfo { META_NS::ContainerChangeType::ADDED,
            interface_pointer_cast<META_NS::IObject>(child), nodeCont, size_t(-1), 0 };
        EXPECT_TRUE(nodeChanges[0] == expected1);
        nodeChanges.clear();

        ASSERT_EQ(rootChanges.size(), 1);
        auto expected2 = META_NS::ChildChangedInfo { META_NS::ContainerChangeType::REMOVED,
            interface_pointer_cast<META_NS::IObject>(child), rootCont, 1, size_t(-1) };
        EXPECT_TRUE(rootChanges[0] == expected2);
        rootChanges.clear();
    }
}

/**
 * @tc.name: NodeContrainerBasic
 * @tc.desc: Tests for Node Contrainer Basic. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, NodeContrainerBasic, testing::ext::TestSize.Level1)
{
    using namespace testing;

    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    auto child1 = scene->CreateNode("//test/child1").GetResult();
    auto child2 = scene->CreateNode("//test/child2").GetResult();
    auto child2Child = scene->CreateNode("//test/child2/child").GetResult();
    auto obj1 = interface_pointer_cast<META_NS::IObject>(child1);
    auto obj2 = interface_pointer_cast<META_NS::IObject>(child2);
    auto obj2Obj = interface_pointer_cast<META_NS::IObject>(child2Child);

    auto nodeCont = interface_pointer_cast<META_NS::IContainer>(node);
    ASSERT_TRUE(nodeCont);
    auto child1Cont = interface_pointer_cast<META_NS::IContainer>(child1);
    auto child1Containable = interface_pointer_cast<META_NS::IContainable>(child1);
    ASSERT_TRUE(child1Containable);

    EXPECT_THAT(nodeCont->GetAll(), ElementsAre(obj1, obj2));
    EXPECT_EQ(nodeCont->GetAt(0), obj1);
    EXPECT_EQ(nodeCont->GetAt(1), obj2);
    EXPECT_EQ(nodeCont->GetSize(), 2);
    EXPECT_EQ(nodeCont->FindByName("child1"), obj1);
    EXPECT_EQ(nodeCont->FindByName("child2"), obj2);
    EXPECT_TRUE(nodeCont->IsAncestorOf(obj2Obj));
    EXPECT_TRUE(nodeCont->IsAncestorOf(obj1));
    EXPECT_TRUE(child1Cont->IsAncestorOf(obj1));
    EXPECT_FALSE(child1Cont->IsAncestorOf(obj2Obj));
    EXPECT_EQ(child1Containable->GetParent(), interface_pointer_cast<META_NS::IObject>(node));

    EXPECT_THAT(nodeCont->FindAll({}), ElementsAre(obj1, obj2, obj2Obj));
    EXPECT_THAT(nodeCont->FindAll({ "child" }), ElementsAre(obj2Obj));
    EXPECT_THAT(nodeCont->FindAll({ "", META_NS::TraversalType::NO_HIERARCHY }), ElementsAre(obj1, obj2));

    EXPECT_EQ(nodeCont->FindAny({}), obj1);
    EXPECT_EQ(nodeCont->FindAny({ "child" }), obj2Obj);
    EXPECT_EQ(nodeCont->FindAny({ "child", META_NS::TraversalType::NO_HIERARCHY }), nullptr);

    EXPECT_TRUE(nodeCont->Remove(obj1));
    EXPECT_THAT(nodeCont->GetAll(), ElementsAre(obj2));
    EXPECT_TRUE(nodeCont->Add(obj1));
    EXPECT_THAT(nodeCont->GetAll(), ElementsAre(obj2, obj1));
    EXPECT_TRUE(nodeCont->Remove(0));
    EXPECT_THAT(nodeCont->GetAll(), ElementsAre(obj1));
    EXPECT_TRUE(nodeCont->Insert(0, obj2));
    EXPECT_THAT(nodeCont->GetAll(), ElementsAre(obj2, obj1));
    EXPECT_TRUE(nodeCont->Move(1, 0));
    EXPECT_THAT(nodeCont->GetAll(), ElementsAre(obj1, obj2));
    EXPECT_TRUE(nodeCont->Move(obj2, 0));
    EXPECT_THAT(nodeCont->GetAll(), ElementsAre(obj2, obj1));

    EXPECT_TRUE(nodeCont->Replace(obj2, obj2Obj, false));
    EXPECT_THAT(nodeCont->GetAll(), ElementsAre(obj2Obj, obj1));
    EXPECT_TRUE(nodeCont->Replace(obj2, obj2, true));
    EXPECT_THAT(nodeCont->GetAll(), ElementsAre(obj2Obj, obj1, obj2));

    nodeCont->RemoveAll();
    EXPECT_THAT(nodeCont->GetAll(), ElementsAre());

    obj1.reset();
    obj2.reset();
    obj2Obj.reset();
    child1.reset();
    child2.reset();
    child2Child.reset();
    child1Cont.reset();
    nodeCont.reset();

    scene->ReleaseNode(BASE_NS::move(node), true).Wait();
}

/**
 * @tc.name: NodeMetadata
 * @tc.desc: Tests for Node Metadata. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, NodeMetadata, testing::ext::TestSize.Level1)
{
    TestNodeMetadata(GetSceneManager(), ClassId::Node);
}
/**
 * @tc.name: MeshNodeMetadata
 * @tc.desc: Tests for Mesh Node Metadata. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, MeshNodeMetadata, testing::ext::TestSize.Level1)
{
    TestNodeMetadata(GetSceneManager(), ClassId::MeshNode);
}
/**
 * @tc.name: CameraNodeMetadata
 * @tc.desc: Tests for Camera Node Metadata. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, CameraNodeMetadata, testing::ext::TestSize.Level1)
{
    TestNodeMetadata(GetSceneManager(), ClassId::CameraNode);
}
/**
 * @tc.name: LightNodeMetadata
 * @tc.desc: Tests for Light Node Metadata. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, LightNodeMetadata, testing::ext::TestSize.Level1)
{
    TestNodeMetadata(GetSceneManager(), ClassId::LightNode);
}
/**
 * @tc.name: TextNodeMetadata
 * @tc.desc: Tests for Text Node Metadata. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, TextNodeMetadata, testing::ext::TestSize.Level1)
{
    TestNodeMetadata(GetSceneManager(), ClassId::TextNode);
}

/**
 * @tc.name: NodeContrainerRemoveImportScene
 * @tc.desc: Tests for Node Contrainer Remove Import Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, NodeContrainerRemoveImportScene, testing::ext::TestSize.Level1)
{
    auto cube = LoadScene("test_assets://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(cube);

    auto scene = CreateEmptyScene();
    auto iScene = scene->GetInternalScene();
    auto node = scene->CreateNode("//test").GetResult();

    auto nodeCont = interface_pointer_cast<META_NS::IContainer>(node);
    ASSERT_TRUE(nodeCont);

    {
        auto groups = iScene->GetResourceGroups();
        ASSERT_EQ(groups.GetAllHandles().size(), 1);
        EXPECT_TRUE(groups.GetHandle("Scene"));
    }

    auto ext = interface_cast<INodeImport>(node);
    ASSERT_TRUE(ext);
    auto newNode = ext->ImportChildScene(cube, "some").GetResult();
    ASSERT_TRUE(newNode);

    {
        auto groups = iScene->GetResourceGroups();
        ASSERT_EQ(groups.GetAllHandles().size(), 2);
        EXPECT_TRUE(groups.GetHandle("Scene"));
        EXPECT_TRUE(groups.GetHandle("Scene - test_assets://AnimatedCube/AnimatedCube.gltf"));
    }

    EXPECT_TRUE(scene->FindNode("//test/some").GetResult());
    EXPECT_TRUE(
        resources
            ->GetResourceInfo({ "AnimatedCube.gltf/images/0", "Scene - test_assets://AnimatedCube/AnimatedCube.gltf" })
            .id.IsValid());

    nodeCont->Remove(interface_pointer_cast<META_NS::IObject>(newNode));
    EXPECT_FALSE(scene->FindNode("//test/some").GetResult());
    EXPECT_FALSE(
        resources
            ->GetResourceInfo({ "AnimatedCube.gltf/images/0", "Scene - test_assets://AnimatedCube/AnimatedCube.gltf" })
            .id.IsValid());

    {
        auto groups = iScene->GetResourceGroups();
        ASSERT_EQ(groups.GetAllHandles().size(), 1);
        EXPECT_TRUE(groups.GetHandle("Scene"));
    }

    nodeCont->Add(interface_pointer_cast<META_NS::IObject>(newNode));

    {
        auto groups = iScene->GetResourceGroups();
        ASSERT_EQ(groups.GetAllHandles().size(), 2);
        EXPECT_TRUE(groups.GetHandle("Scene"));
        EXPECT_TRUE(groups.GetHandle("Scene - test_assets://AnimatedCube/AnimatedCube.gltf"));
    }

    EXPECT_TRUE(scene->FindNode("//test/some").GetResult());
    EXPECT_TRUE(
        resources
            ->GetResourceInfo({ "AnimatedCube.gltf/images/0", "Scene - test_assets://AnimatedCube/AnimatedCube.gltf" })
            .id.IsValid());
}

/**
 * @tc.name: LayerMaskOfGltfScene
 * @tc.desc: Tests for Layer Mask Of Gltf Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, LayerMaskOfGltfScene, testing::ext::TestSize.Level1)
{
    auto scene = Scene(LoadScene("test://AnimatedCube/AnimatedCube.gltf"));
    auto node = scene.FindNode("AnimatedCube");
    EXPECT_TRUE(node);
    auto i = interface_pointer_cast<ILayer>(node);
    ASSERT_TRUE(i);
    auto p = i->LayerMask();
    ASSERT_TRUE(p);
}

/**
 * @tc.name: InvalidNode
 * @tc.desc: Tests for Invalid Node. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, InvalidNode, testing::ext::TestSize.Level1)
{
    auto& reg = META_NS::GetObjectRegistry();
    const auto allNodes = reg.GetClassRegistry().GetAllTypes({ INode::UID });
    ASSERT_FALSE(allNodes.empty());

    for (const auto& t : allNodes) {
        auto clsi = t->GetClassInfo();
        auto node = reg.Create<INode>(clsi);
        ASSERT_TRUE(node);
        EXPECT_FALSE(node->GetScene());
        EXPECT_FALSE(node->GetParent().GetResult());
        EXPECT_TRUE(node->GetChildren().GetResult().empty());
        EXPECT_TRUE(node->GetPath().GetResult().empty());
        EXPECT_FALSE(node->AddChild({}).GetResult());
        EXPECT_FALSE(node->RemoveChild({}).GetResult());
        auto import = interface_cast<INodeImport>(node);
        ASSERT_TRUE(import);
        EXPECT_FALSE(import->ImportChild({}).GetResult());
        EXPECT_FALSE(import->ImportChildScene({}, "", "").GetResult());
        EXPECT_FALSE(import->ImportChildScene("", "").GetResult());
        auto container = interface_cast<META_NS::IContainer>(node);
        ASSERT_TRUE(container);
        EXPECT_FALSE(container->Add({}));
        EXPECT_FALSE(container->Insert(0, {}));
        EXPECT_FALSE(container->Remove(0));
        EXPECT_FALSE(container->Remove(META_NS::IObject::Ptr {}));
        EXPECT_FALSE(container->Move(0, 1));
        EXPECT_FALSE(container->Move(META_NS::IObject::Ptr {}, 1));
        EXPECT_FALSE(container->Replace(META_NS::IObject::Ptr {}, META_NS::IObject::Ptr {}, true));
        container->RemoveAll();
        EXPECT_FALSE(container->IsAncestorOf({}));
    }
}

/**
 * @tc.name: UniqueName
 * @tc.desc: Tests INode::GetUniqueChildName
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, UniqueName, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    scene->CreateNode("//test/child").GetResult();
    scene->CreateNode("//test/child(1)").GetResult();
    scene->CreateNode("//test/child 1)").GetResult();
    scene->CreateNode("//test/child (2)").GetResult();
    scene->CreateNode("//test/child (3)").GetResult();
    scene->CreateNode("//test/child (4)").GetResult();
    scene->CreateNode("//test/child (100)").GetResult();
    scene->CreateNode("//test/(2)").GetResult();
    scene->CreateNode("//test/child2/child").GetResult();

    auto getUniqueName = [](const INode::Ptr& node, BASE_NS::string_view name) -> BASE_NS::string {
        return node->GetUniqueChildName(name).GetResult();
    };

    EXPECT_EQ(getUniqueName(node, ""), "");                     // Already unique
    EXPECT_EQ(getUniqueName(node, "child1"), "child1");         // Already unique
    EXPECT_EQ(getUniqueName(node, "child"), "child (1)");       // 'child' used, expect 'child (1)'
    EXPECT_EQ(getUniqueName(node, "child (2)"), "child (1)");   // 'child (2)' taken, but child (1) is not
    EXPECT_EQ(getUniqueName(node, "child (100)"), "child (1)"); // 'child (2)' taken, but child (1) is not
    EXPECT_EQ(getUniqueName(node, "(2)"), "(1)");               // '(2)' used, expect '(1)'
    EXPECT_EQ(getUniqueName(node, "(1)"), "(1)");               // '(1)' is already unique
    EXPECT_EQ(getUniqueName(node, "child 1)"), "child 1) (1)"); // 'child 1)' is already unique
    EXPECT_EQ(getUniqueName(node, "child 2)"), "child 2)");     // 'child 2)' is already unique
    // 'child(1)' is in use but no ' ' between name and postfix so (1) should be added
    EXPECT_EQ(getUniqueName(node, "child(1)"), "child(1) (1)");

    auto child1 =
        scene->CreateNode("//test/child (1)").GetResult(); // Take 'child (1)' into use, slot 5 is the first free
    ASSERT_TRUE(child1);
    EXPECT_EQ(getUniqueName(node, "child"), "child (5)");       // 'child' used, expect 'child (5)'
    EXPECT_EQ(getUniqueName(node, "child (2)"), "child (5)");   // 'child (2)' taken, but child (5) is not
    EXPECT_EQ(getUniqueName(node, "child (100)"), "child (5)"); // 'child (2)' taken, but child (5) is not

    META_NS::SetValue(child1->Enabled(), false);
    UpdateScene();
    EXPECT_EQ(getUniqueName(node, "child"), "child (5)");       // 'child' used, expect 'child (5)'
    EXPECT_EQ(getUniqueName(node, "child (2)"), "child (5)");   // 'child (2)' taken, but child (5) is not
    EXPECT_EQ(getUniqueName(node, "child (100)"), "child (5)"); // 'child (2)' taken, but child (5) is not

    EXPECT_TRUE(node->RemoveChild(child1).GetResult()); // Free Child (1) slot
    UpdateScene();
    EXPECT_EQ(getUniqueName(node, "child"), "child (1)");       // 'child' used, expect 'child (1)'
    EXPECT_EQ(getUniqueName(node, "child (2)"), "child (1)");   // 'child (2)' taken, but child (1) is not
    EXPECT_EQ(getUniqueName(node, "child (100)"), "child (1)"); // 'child (2)' taken, but child (1) is not
}

/**
 * @tc.name: UniqueNameNoScene
 * @tc.desc: Tests INode::GetUniqueChildName with no attached scene.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, UniqueNameNoScene, testing::ext::TestSize.Level1)
{
    Node node(META_NS::GetObjectRegistry().Create<INode>(ClassId::Node));
    ASSERT_TRUE(node);
    EXPECT_EQ(node.GetUniqueChildName("child"), "child");
}

/**
 * @tc.name: UniqueNameInvalidEntity
 * @tc.desc: Tests IInternalScene::GetUniqueName with an invalid entity
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, UniqueNameInvalidEntity, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test").GetResult();
    scene->CreateNode("//test/child").GetResult();

    auto internal = scene->GetInternalScene();
    ASSERT_TRUE(internal);
    EXPECT_EQ(internal->GetUniqueName("child", CORE_NS::Entity {}), "child");
}

void ExpectResource(CORE_NS::IResourceManager::Ptr resources, CORE_NS::ResourceId id)
{
    EXPECT_TRUE(resources->GetResourceInfo(id).id.IsValid());
}

void API_ScenePluginNodeTest::CloneNodeTest(
    IScene::Ptr scene, BASE_NS::string_view name, INode::Ptr parent, BASE_NS::string_view expectedName)
{
    auto cube = LoadScene("test_assets://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(cube);

    auto iScene = scene->GetInternalScene();
    auto node = scene->CreateNode("//test").GetResult();

    auto ext = interface_cast<INodeImport>(node);
    ASSERT_TRUE(ext);
    auto newNode = ext->ImportChildScene(cube, "some").GetResult();
    ASSERT_TRUE(newNode);

    auto otherNode = newNode->Clone(name, parent).GetResult();
    ASSERT_TRUE(otherNode);

    BASE_NS::string expPath = parent ? parent->GetPath().GetResult() + "/" + expectedName : "//test/" + expectedName;

    auto c = scene->FindNode(expPath).GetResult();
    EXPECT_EQ(otherNode, c);

    EXPECT_EQ(parent ? parent : node, otherNode->GetParent().GetResult());

    auto n = scene->FindNode<IMesh>(expPath + "//AnimatedCube").GetResult();
    ASSERT_TRUE(n);

    auto subs = n->SubMeshes()->GetValue();
    ASSERT_TRUE(!subs.empty());

    CompareTrees(otherNode, cube->GetRootNode().GetResult(), true);

    BASE_NS::vector<CORE_NS::MatchingResourceId> res {
        { CORE_NS::ResourceId("AnimatedCube.gltf/images/0", "test_assets://AnimatedCube/AnimatedCube.gltf") },
        { CORE_NS::ResourceId("AnimatedCube.gltf/images/1", "test_assets://AnimatedCube/AnimatedCube.gltf") },
        { CORE_NS::ResourceId("AnimatedCube", "test_assets://AnimatedCube/AnimatedCube.gltf") } };

    auto infos = GetResourceManager(cube)->GetResourceInfos(res);
    ASSERT_TRUE(!infos.empty());

    ExpectAtleastResources(GetResourceManager(scene),
        ChangeGroup(AddToResourceName(infos, " (1)"), "Scene - test_assets://AnimatedCube/AnimatedCube.gltf"));
}

/**
 * @tc.name: CloneNode
 * @tc.desc: Tests for Clone Node. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, CloneNode, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    CloneNodeTest(scene, "other", nullptr, "other");
}

/**
 * @tc.name: CloneNodeDifferentScene
 * @tc.desc: Tests for Clone Node from different scene.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, CloneNodeDifferentScene, testing::ext::TestSize.Level1)
{
    auto cube = Scene(LoadScene("test://AnimatedCube/AnimatedCube.gltf"));
    auto cube2 = Scene(LoadScene("test://AnimatedCube/AnimatedCube.gltf"));
    auto node = cube.FindNode("AnimatedCube");
    ASSERT_TRUE(node);
    auto differentSceneRoot = cube2.GetRootNode();
    ASSERT_TRUE(differentSceneRoot);
    // Should fail from different scene
    EXPECT_FALSE(node.Clone("invalid", differentSceneRoot));
}

/**
 * @tc.name: CloneNodeNoName
 * @tc.desc: Tests for Clone Node No Name. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, CloneNodeNoName, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    CloneNodeTest(scene, "", nullptr, "some (1)");
}

/**
 * @tc.name: CloneNodeParent
 * @tc.desc: Tests for Clone Node Parent. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, CloneNodeParent, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//aaa").GetResult();
    CloneNodeTest(scene, "other", node, "other");
}

/**
 * @tc.name: CloneNodeParentNoName
 * @tc.desc: Tests for Clone Node Parent NoName. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, CloneNodeParentNoName, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//aaa").GetResult();
    CloneNodeTest(scene, "", node, "some");
}

/**
 * @tc.name: CloneNodeParentNoNameConflict
 * @tc.desc: Tests for Clone Node Parent NoName Conflict. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, CloneNodeParentNoNameConflict, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//aaa").GetResult();
    auto dummy = scene->CreateNode("//aaa/some").GetResult();
    CloneNodeTest(scene, "", node, "some (1)");
}

/**
 * @tc.name: CloneNodeTwice
 * @tc.desc: Tests for Clone Node Twice. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, CloneNodeTwice, testing::ext::TestSize.Level1)
{
    auto cube = LoadScene("test_assets://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(cube);

    auto scene = CreateEmptyScene();
    auto iScene = scene->GetInternalScene();
    auto resources = GetResourceManager(iScene);
    auto node = scene->CreateNode("//test").GetResult();

    auto ext = interface_cast<INodeImport>(node);
    ASSERT_TRUE(ext);
    auto newNode = ext->ImportChildScene(cube, "some").GetResult();
    ASSERT_TRUE(newNode);

    auto otherNode = newNode->Clone("other").GetResult();
    ASSERT_TRUE(otherNode);
    auto otherNode2 = otherNode->Clone("hihaa").GetResult();
    ASSERT_TRUE(otherNode2);

    auto n = scene->FindNode<IMesh>("//test/hihaa//AnimatedCube").GetResult();
    ASSERT_TRUE(n);

    auto subs = n->SubMeshes()->GetValue();
    ASSERT_TRUE(!subs.empty());

    CompareTrees(scene->FindNode("//test/other").GetResult(), cube->GetRootNode().GetResult(), true);
    CompareTrees(scene->FindNode("//test/hihaa").GetResult(), cube->GetRootNode().GetResult(), true);

    BASE_NS::vector<CORE_NS::MatchingResourceId> res {
        { CORE_NS::ResourceId("AnimatedCube.gltf/images/0", "test_assets://AnimatedCube/AnimatedCube.gltf") },
        { CORE_NS::ResourceId("AnimatedCube.gltf/images/1", "test_assets://AnimatedCube/AnimatedCube.gltf") },
        { CORE_NS::ResourceId("AnimatedCube", "test_assets://AnimatedCube/AnimatedCube.gltf") } };

    auto infos = GetResourceManager(cube)->GetResourceInfos(res);
    ASSERT_TRUE(!infos.empty());

    auto res1 = ChangeGroup(AddToResourceName(infos, " (1)"), "Scene - test_assets://AnimatedCube/AnimatedCube.gltf");
    auto res2 = ChangeGroup(AddToResourceName(infos, " (1) (1)"), "Scene - test_assets://AnimatedCube/AnimatedCube.gltf");
    auto allRes = res1;
    allRes.insert(allRes.end(), res2.begin(), res2.end());

    ExpectAtleastResources(GetResourceManager(scene), allRes);
}

/**
 * @tc.name: CloneRoot
 * @tc.desc: Tests for Clone Root. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, CloneRoot, testing::ext::TestSize.Level1)
{
    auto cube = LoadScene("test_assets://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(cube);

    auto root = cube->GetRootNode().GetResult();

    auto otherNode = root->Clone("other").GetResult();
    ASSERT_TRUE(otherNode);

    // also the parent ref is cloned, so this one does not have parent and is not part of the scene hierarchy
    EXPECT_TRUE(!otherNode->GetParent().GetResult());

    CompareTrees(otherNode, root, true);

    BASE_NS::vector<CORE_NS::MatchingResourceId> res {
        { CORE_NS::ResourceId("AnimatedCube.gltf/images/0", "test_assets://AnimatedCube/AnimatedCube.gltf") },
        { CORE_NS::ResourceId("AnimatedCube.gltf/images/1", "test_assets://AnimatedCube/AnimatedCube.gltf") },
        { CORE_NS::ResourceId("AnimatedCube", "test_assets://AnimatedCube/AnimatedCube.gltf") }
    };

    auto infos = GetResourceManager(cube)->GetResourceInfos(res);
    ASSERT_TRUE(!infos.empty());

    ExpectAtleastResources(GetResourceManager(cube), AddToResourceName(infos, " (1)"));

    EXPECT_TRUE(root->AddChild(otherNode).GetResult());
    EXPECT_EQ(root, otherNode->GetParent().GetResult());
}

/**
 * @tc.name: CloneConflictingName
 * @tc.desc: Tests for Clone Conflicting Name. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginNodeTest, CloneConflictingName, testing::ext::TestSize.Level1)
{
    auto cube = LoadScene("test_assets://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(cube);

    auto scene = CreateEmptyScene();
    auto iScene = scene->GetInternalScene();
    auto resources = GetResourceManager(iScene);
    auto node = scene->CreateNode("//test").GetResult();

    auto ext = interface_cast<INodeImport>(node);
    ASSERT_TRUE(ext);
    auto newNode = ext->ImportChildScene(cube, "some").GetResult();
    ASSERT_TRUE(newNode);

    auto otherNode = newNode->Clone("test").GetResult();
    ASSERT_TRUE(otherNode);

    EXPECT_EQ(node, otherNode->GetParent().GetResult());

    CompareTrees(otherNode, cube->GetRootNode().GetResult(), true);

    BASE_NS::vector<CORE_NS::MatchingResourceId> res {
        { CORE_NS::ResourceId("AnimatedCube.gltf/images/0", "test_assets://AnimatedCube/AnimatedCube.gltf") },
        { CORE_NS::ResourceId("AnimatedCube.gltf/images/1", "test_assets://AnimatedCube/AnimatedCube.gltf") },
        { CORE_NS::ResourceId("AnimatedCube", "test_assets://AnimatedCube/AnimatedCube.gltf") } };

    auto infos = GetResourceManager(cube)->GetResourceInfos(res);
    ASSERT_TRUE(!infos.empty());

    ExpectAtleastResources(GetResourceManager(scene),
        ChangeGroup(AddToResourceName(infos, " (1)"), "Scene - test_assets://AnimatedCube/AnimatedCube.gltf"));
}

} // namespace UTest

SCENE_END_NAMESPACE()
