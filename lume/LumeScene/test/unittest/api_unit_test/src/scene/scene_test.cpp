
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

#include "scene/scene_test.h"

#include <scene/api/resource_utils.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/ext/scene_debug_info.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_raycast.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>

#include <3d/ecs/components/transform_component.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>

#include <meta/api/make_callback.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/construct_array_property.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/property/property_events.h>
#include <meta/interface/resource/intf_dynamic_resource.h>
#include <meta/interface/resource/intf_resource_query.h>

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePlugin : public ScenePluginTest {
public:
};

/**
 * @tc.name: LoadScene
 * @tc.desc: Tests for Load Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, LoadScene, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://celia/Celia.gltf");

    auto root = scene->GetRootNode().GetResult();
    ASSERT_TRUE(root != nullptr);

    auto plane = scene->FindNode("//Scene/Plane").GetResult();
    ASSERT_TRUE(interface_cast<IMesh>(plane));

    auto children = root->GetChildren().GetResult();
    ASSERT_EQ(children.size(), 1);

    auto scene_node = children.at(0);
    auto scene_node_children = scene_node->GetChildren().GetResult();

    ASSERT_EQ(scene_node_children.size(), 5);

    auto cameras = scene->GetCameras().GetResult();
    ASSERT_EQ(cameras.size(), 1);

    auto shell = scene->FindNode("//Scene/Shell").GetResult();
    ASSERT_TRUE(shell != nullptr);

    auto circle_parent_r1 = scene->FindNode("//Scene/Circle_Parent_R1").GetResult();
    ASSERT_TRUE(circle_parent_r1 != nullptr);

    auto celia_parent_r1 = scene->FindNode("//Scene/Celia_Parent_R1").GetResult();
    ASSERT_TRUE(celia_parent_r1 != nullptr);

    auto animations = scene->GetAnimations().GetResult();
    ASSERT_EQ(animations.size(), 1);
}

/**
 * @tc.name: CreateEmptyScene
 * @tc.desc: Tests for Create Empty Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, CreateEmptyScene, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene != nullptr);

    auto root = scene->GetRootNode().GetResult();
    ASSERT_TRUE(root);
    ASSERT_TRUE(!root->GetPath().GetResult().empty());

    auto root2 = scene->FindNode("").GetResult();
    EXPECT_EQ(root, root2);

    auto root3 = scene->FindNode("/").GetResult();
    EXPECT_EQ(root, root3);

    auto root4 = scene->FindNode("//").GetResult();
    EXPECT_EQ(root, root4);
}

/**
 * @tc.name: CreateEmptySceneWithDefaultContext
 * @tc.desc: Tests for Create Empty Scene With Default Context. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, CreateEmptySceneWithDefaultContext, testing::ext::TestSize.Level1)
{
    auto m = GetSceneManager();
    ASSERT_TRUE(m);
    scene = m->CreateScene().GetResult();
    ASSERT_TRUE(scene != nullptr);
}

/**
 * @tc.name: CreateNodeWithPositionToEmptyScene
 * @tc.desc: Tests for Create Node With Position To Empty Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, CreateNodeWithPositionToEmptyScene, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();

    // missing "scene" node
    auto test_node = scene->CreateNode("//scene/test").GetResult();
    ASSERT_FALSE(test_node);

    test_node = scene->CreateNode("//test").GetResult();
    auto test_node_ecs = interface_pointer_cast<IEcsObjectAccess>(test_node);
    ASSERT_TRUE(test_node_ecs);
    auto ecs_object = test_node_ecs->GetEcsObject();

    // check the native ecs path is the same
    EXPECT_EQ(scene->GetInternalScene()->GetEcsContext().GetPath(ecs_object->GetEntity()), "//test");

    BASE_NS::Math::Vec3 newPosition{1.5, 2.0, 3.0};
    test_node->Position()->SetValue(newPosition);
    auto position = test_node->Position()->GetValue();
    ASSERT_EQ(position, newPosition);

    UpdateScene();

    // Check metaobject
    auto eng_transform_position = ecs_object->GetEngineValueManager()->GetEngineValue("TransformComponent.position");
    auto& eng_position_value = eng_transform_position->GetValue();
    BASE_NS::Math::Vec3 eng_position_value_vec;
    auto ret_code = eng_position_value.GetValue(eng_position_value_vec);
    ASSERT_EQ(ret_code, META_NS::AnyReturn::SUCCESS);
    ASSERT_EQ(position, eng_position_value_vec);

    // Check engine
    auto entity = ecs_object->GetEntity();
    auto internal = scene->GetInternalScene();
    auto ecs = internal->GetEcsContext().GetNativeEcs();
    auto trans_m = ecs->GetComponentManager(CORE3D_NS::ITransformComponentManager::UID);
    auto trans_component_manager = (CORE3D_NS::ITransformComponentManager*)trans_m;
    auto test_transform_component = trans_component_manager->Get(entity);
    ASSERT_EQ(test_transform_component.position, newPosition);
}

/**
 * @tc.name: ReleaseNode
 * @tc.desc: Tests for Release Node. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, ReleaseNode, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();

    auto testNode = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(testNode);

    BASE_NS::Math::Vec3 newPosition{1.5, 2.0, 3.0};
    testNode->Position()->SetValue(newPosition);

    UpdateScene();

    auto position = testNode->Position()->GetValue();
    EXPECT_EQ(position, newPosition);

    {
        // make copy, ReleaseNode should not do anything
        auto copy = testNode;
        INode::WeakPtr w = testNode;
        EXPECT_FALSE(scene->ReleaseNode(BASE_NS::move(testNode), false).GetResult());
        copy.reset();
        EXPECT_FALSE(w.expired());
    }
    testNode = scene->FindNode("//test").GetResult();
    {
        INode::WeakPtr w = testNode;
        EXPECT_TRUE(scene->ReleaseNode(BASE_NS::move(testNode), false).GetResult());
        EXPECT_TRUE(w.expired());
    }

    testNode = scene->FindNode("//test").GetResult();
    ASSERT_TRUE(testNode);
    EXPECT_EQ(testNode->Position()->GetValue(), newPosition);
}

/**
 * @tc.name: ReleaseNodeFlushesPendingPropertyWrites
 * @tc.desc: Property writes made just before ReleaseNode must reach ECS even
 *          without an intervening Update/SyncProperties.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, ReleaseNodeFlushesPendingPropertyWrites, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();

    auto testNode = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(testNode);

    // Make sure the entity exists in ECS before the property write we care about.
    UpdateScene();

    BASE_NS::Math::Vec3 newPosition{1.5, 2.0, 3.0};
    testNode->Position()->SetValue(newPosition);

    // Deliberately do NOT call UpdateScene()/SyncProperties() here. ReleaseNode
    // is expected to flush this object's pending API->ECS writes itself.
    EXPECT_TRUE(scene->ReleaseNode(BASE_NS::move(testNode), false).GetResult());

    testNode = scene->FindNode("//test").GetResult();
    ASSERT_TRUE(testNode);
    EXPECT_EQ(testNode->Position()->GetValue(), newPosition);
}

/**
 * @tc.name: ReleaseNodeRecursive
 * @tc.desc: Tests for Release Node Recursive. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, ReleaseNodeRecursive, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();

    auto node1 = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(node1);
    INode::WeakPtr node1w = node1;
    auto node2 = scene->CreateNode("//test/node1").GetResult();
    ASSERT_TRUE(node2);
    INode::WeakPtr node2w = node2;
    auto node3 = scene->CreateNode("//test/node2").GetResult();
    ASSERT_TRUE(node3);
    INode::WeakPtr node3w = node3;
    auto node4 = scene->CreateNode("//test/node2/test").GetResult();
    ASSERT_TRUE(node4);
    INode::WeakPtr node4w = node4;

    // leave node 3 alive
    node2.reset();
    node4.reset();

    // scene caches the nodes
    EXPECT_FALSE(node2w.expired());
    EXPECT_FALSE(node4w.expired());

    EXPECT_TRUE(scene->ReleaseNode(BASE_NS::move(node1), true).GetResult());

    EXPECT_TRUE(node1w.expired());
    EXPECT_TRUE(node2w.expired());
    EXPECT_TRUE(node4w.expired());

    node3.reset();
    EXPECT_FALSE(node3w.expired());

    EXPECT_TRUE(scene->FindNode("//test").GetResult());
    EXPECT_TRUE(scene->FindNode("//test/node1").GetResult());
    EXPECT_TRUE(scene->FindNode("//test/node2").GetResult());
    EXPECT_TRUE(scene->FindNode("//test/node2/test").GetResult());
}

/**
 * @tc.name: ReleaseMeshNode
 * @tc.desc: Tests for Release Mesh Node. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, ReleaseMeshNode, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test_assets://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);
    {
        auto ext = interface_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(ext);

        auto child = ext->ImportChildScene("test_assets://AnimatedCube/AnimatedCube.gltf", "test").GetResult();
        ASSERT_TRUE(child);
    }
    UpdateScene();

    INode::Ptr node = scene->FindNode("//test//AnimatedCube").GetResult();
    EXPECT_TRUE(node);

    INode::WeakPtr wnode = node;
    IMesh::WeakPtr wmesh;

    CORE_NS::Entity ent;
    {
        auto mesh = interface_cast<IMesh>(node);
        ASSERT_TRUE(mesh);

        auto access = interface_cast<IMeshAccess>(mesh);
        ASSERT_TRUE(access);

        auto m = access->GetMesh().GetResult();
        ASSERT_TRUE(m);

        wmesh = m;
        auto acc = interface_cast<IEcsObjectAccess>(m);
        ASSERT_TRUE(acc);
        ent = acc->GetEcsObject()->GetEntity();
        ASSERT_TRUE(CORE_NS::EntityUtil::IsValid(ent));
    }

    auto entities = GetEntityCount();
    EXPECT_TRUE(scene->ReleaseNode(BASE_NS::move(node), true).GetResult());
    UpdateScene();
    EXPECT_EQ(entities, GetEntityCount());
    EXPECT_FALSE(wnode.lock());
    EXPECT_FALSE(wmesh.lock());
    {
        auto mesh = scene->FindNode<IMesh>("//test//AnimatedCube").GetResult();
        ASSERT_TRUE(mesh);

        auto access = interface_cast<IMeshAccess>(mesh);
        ASSERT_TRUE(access);

        auto m = access->GetMesh().GetResult();
        ASSERT_TRUE(m);
        auto acc = interface_cast<IEcsObjectAccess>(m);
        ASSERT_TRUE(acc);
        auto e = acc->GetEcsObject()->GetEntity();
        EXPECT_EQ(e, ent);
        auto& eman = scene->GetInternalScene()->GetEcsContext().GetNativeEcs()->GetEntityManager();
        EXPECT_TRUE(eman.IsAlive(e));
    }
}

/**
 * @tc.name: MoveNode
 * @tc.desc: Tests for Move Node. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, MoveNode, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();

    auto testNode = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(testNode);

    EXPECT_TRUE(testNode->Enabled()->GetValue());

    BASE_NS::Math::Vec3 newPosition{1.5, 2.0, 3.0};
    testNode->Position()->SetValue(newPosition);

    UpdateScene();

    EXPECT_TRUE(scene->GetRootNode().GetResult()->RemoveChild(testNode).GetResult());

    UpdateScene();

    EXPECT_EQ(testNode->GetPath().GetResult(), "");

    EXPECT_FALSE(scene->FindNode("//test").GetResult());

    auto test2Node = scene->CreateNode("//test2").GetResult();
    ASSERT_TRUE(test2Node);
    {
        auto testNodeEcs = interface_pointer_cast<IEcsObjectAccess>(test2Node);
        auto ecsObject = testNodeEcs->GetEcsObject();
        EXPECT_EQ(scene->GetInternalScene()->GetEcsContext().GetPath(ecsObject->GetEntity()), "//test2");
    }

    ASSERT_TRUE(test2Node->AddChild(testNode).GetResult());

    auto searchedNode = scene->FindNode("//test2/test").GetResult();
    ASSERT_EQ(searchedNode, testNode);

    UpdateScene();

    auto testNodeEcs = interface_pointer_cast<IEcsObjectAccess>(searchedNode);
    auto ecsObject = testNodeEcs->GetEcsObject();
    EXPECT_EQ(scene->GetInternalScene()->GetEcsContext().GetPath(ecsObject->GetEntity()), "//test2/test");

    // Check engine
    auto entity = ecsObject->GetEntity();
    auto internal = scene->GetInternalScene();
    auto ecs = internal->GetEcsContext().GetNativeEcs();
    auto trans = ecs->GetComponentManager(CORE3D_NS::ITransformComponentManager::UID);
    auto transComponentManager = (CORE3D_NS::ITransformComponentManager*)trans;
    auto testTransformComponent = transComponentManager->Get(entity);
    ASSERT_EQ(testTransformComponent.position, newPosition);
}

static bool IsEntityActive(INode::Ptr node)
{
    auto ecsObject = interface_pointer_cast<IEcsObjectAccess>(node)->GetEcsObject();
    return node->GetScene()->GetInternalScene()->GetEcsContext().GetNativeEcs()->GetEntityManager().IsAlive(
        ecsObject->GetEntity());
}

/**
 * @tc.name: MoveNodeActiveCheck
 * @tc.desc: Tests for Move Node Active Check. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, MoveNodeActiveCheck, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();

    auto root = scene->GetRootNode().GetResult();
    auto testNode = scene->CreateNode("//test").GetResult();
    auto someNode = scene->CreateNode("//test/some").GetResult();
    ASSERT_TRUE(testNode);
    ASSERT_TRUE(someNode);

    EXPECT_TRUE(IsEntityActive(testNode));
    EXPECT_TRUE(IsEntityActive(someNode));

    EXPECT_TRUE(root->RemoveChild(testNode).GetResult());
    EXPECT_FALSE(IsEntityActive(testNode));
    EXPECT_FALSE(IsEntityActive(someNode));

    EXPECT_FALSE(testNode->GetParent().GetResult());

    // cannot add/remove from inactive node
    EXPECT_FALSE(testNode->RemoveChild(someNode).GetResult());
    EXPECT_FALSE(testNode->AddChild(someNode).GetResult());

    ASSERT_TRUE(root->AddChild(testNode).GetResult());
    EXPECT_TRUE(IsEntityActive(testNode));
    EXPECT_TRUE(IsEntityActive(someNode));

    EXPECT_TRUE(testNode->GetParent().GetResult());
}

/**
 * @tc.name: RemoveNode
 * @tc.desc: Tests for Remove Node. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, RemoveNode, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto testNode = scene->CreateNode("//test").GetResult();

    EXPECT_TRUE(scene->FindNode("//test").GetResult());
    EXPECT_TRUE(scene->RemoveNode(BASE_NS::move(testNode)).GetResult());
    EXPECT_FALSE(scene->FindNode("//test").GetResult());

    UpdateScene();

    EXPECT_FALSE(scene->FindNode("//test").GetResult());
}

/**
 * @tc.name: RayCast
 * @tc.desc: Tests for Ray Cast. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, RayCast, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test", ClassId::MeshNode).GetResult();
    ASSERT_TRUE(node);
    node->Position()->SetValue({0, 0, 10});
    auto mesh = interface_pointer_cast<IMesh>(node);
    ASSERT_TRUE(mesh);
    AddSubMesh(mesh);
    auto submesh = mesh->SubMeshes()->GetValueAt(0);
    ASSERT_TRUE(submesh);
    submesh->AABBMin()->SetValue({-1, -1, -1});
    submesh->AABBMax()->SetValue({1, 1, 1});
    UpdateScene();
    EXPECT_EQ(mesh->AABBMin()->GetValue(), (BASE_NS::Math::Vec3{-1, -1, -1}));
    EXPECT_EQ(mesh->AABBMax()->GetValue(), (BASE_NS::Math::Vec3{1, 1, 1}));
    auto rc = interface_cast<IRayCast>(scene);
    ASSERT_TRUE(rc);
    auto res = rc->CastRay({0, 0, 0}, {0, 0, 1}, {}).GetResult();
    ASSERT_EQ(res.size(), 1);
    EXPECT_EQ(res[0].position, (BASE_NS::Math::Vec3{0, 0, 9}));
    EXPECT_EQ(res[0].distance, 9);
    EXPECT_EQ(res[0].distanceToCenter, 10);
    EXPECT_EQ(res[0].node, node);

    RayCastOptions opts;
    opts.layerMask = 42u;
    res = rc->CastRay({0, 0, 0}, {0, 0, 1}, opts).GetResult();
    ASSERT_EQ(res.size(), 0);
}

/**
 * @tc.name: RayCastVisible
 * @tc.desc: Test raycase for only visible nodes
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, RayCastVisible, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test", ClassId::MeshNode).GetResult();
    ASSERT_TRUE(node);
    node->Position()->SetValue({0, 0, 10});
    auto mesh = interface_pointer_cast<IMesh>(node);
    ASSERT_TRUE(mesh);
    AddSubMesh(mesh);
    auto submesh = mesh->SubMeshes()->GetValueAt(0);
    ASSERT_TRUE(submesh);
    submesh->AABBMin()->SetValue({-1, -1, -1});
    submesh->AABBMax()->SetValue({1, 1, 1});
    UpdateScene();

    auto rc = interface_cast<IRayCast>(scene);
    ASSERT_TRUE(rc);
    auto res = rc->CastRay({0, 0, 0}, {0, 0, 1}, {}).GetResult();
    ASSERT_EQ(res.size(), 1);

    META_NS::SetValue(node->Enabled(), false);
    RayCastOptions opts;
    opts.hitOptions = RayCastOptions::HIT_ENABLED;
    res = rc->CastRay({0, 0, 0}, {0, 0, 1}, opts).GetResult();
    EXPECT_EQ(res.size(), 1);  // Still returns 1 since Scene update has not been run

    UpdateScene();
    res = rc->CastRay({0, 0, 0}, {0, 0, 1}, opts).GetResult();
    EXPECT_EQ(res.size(), 0);  // No nodes should be hit after update

    META_NS::SetValue(node->Enabled(), true);
    UpdateScene();
    res = rc->CastRay({0, 0, 0}, {0, 0, 1}, opts).GetResult();
    EXPECT_EQ(res.size(), 1);  // Should return 1 again since the node was enabled
}

/**
 * @tc.name: AnimatedCube
 * @tc.desc: Tests for Animated Cube. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, AnimatedCube, testing::ext::TestSize.Level1)
{
    // this test scene has unnamed nodes which break the scene plugin
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);
    auto root = scene->GetRootNode().GetResult();
    ASSERT_TRUE(root);
    EXPECT_EQ(root->GetChildren().GetResult().size(), 1);
}

/**
 * @tc.name: NodeName
 * @tc.desc: Tests for Node Name. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, NodeName, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();

    auto node = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(node);
    auto snode1 = scene->FindNode("//test").GetResult();
    EXPECT_EQ(node, snode1);

    META_NS::SetName(node, "hops");
    UpdateScene();
    auto snode2 = scene->FindNode("//hops").GetResult();
    EXPECT_EQ(node, snode2);
    ASSERT_FALSE(scene->FindNode("//test").GetResult());

    auto child = scene->CreateNode("//hops/child").GetResult();
    ASSERT_TRUE(child);
    auto child1 = scene->FindNode("//hops/child").GetResult();
    EXPECT_EQ(child, child1);

    META_NS::SetName(node, "hips");
    UpdateScene();
    auto snode3 = scene->FindNode("//hips").GetResult();
    EXPECT_EQ(node, snode3);
    auto child2 = scene->FindNode("//hips/child").GetResult();
    EXPECT_EQ(child, child2);
    ASSERT_FALSE(scene->FindNode("//hops/child").GetResult());

    auto root = scene->GetRootNode().GetResult();
    auto r1 = scene->FindNode("").GetResult();
    EXPECT_EQ(root, r1);
    auto r2 = scene->FindNode("/").GetResult();
    EXPECT_EQ(root, r2);
    auto r3 = scene->FindNode("//").GetResult();
    EXPECT_EQ(root, r3);
}

/**
 * @tc.name: ImportNode
 * @tc.desc: Tests for Import Node. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, ImportNode, testing::ext::TestSize.Level1)
{
    auto scene1 = CreateEmptyScene();
    auto scene2 = CreateEmptyScene();
    ASSERT_TRUE(scene1);
    ASSERT_TRUE(scene2);

    auto n1 = scene1->CreateNode("//test").GetResult();
    ASSERT_TRUE(n1);

    auto e1 = scene2->CreateNode("//other").GetResult();
    ASSERT_TRUE(e1);

    auto ext = interface_cast<INodeImport>(n1);
    ASSERT_TRUE(ext);
    auto child = ext->ImportChild(e1).GetResult();
    ASSERT_TRUE(child);

    auto other = scene1->FindNode("//other").GetResult();
    ASSERT_FALSE(other);

    EXPECT_EQ(child->GetPath().GetResult(), "//test/other");
    EXPECT_EQ(child->GetParent().GetResult(), n1);
    auto ch = scene1->FindNode("//test/other").GetResult();
    ASSERT_TRUE(ch);
    EXPECT_EQ(ch, child);
}

/**
 * @tc.name: ImportScene
 * @tc.desc: Tests for Import Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, ImportScene, testing::ext::TestSize.Level1)
{
    auto scene1 = LoadScene("test://celia/Celia.gltf");
    auto scene2 = LoadScene("test://celia/Celia.gltf");
    ASSERT_TRUE(scene1);
    ASSERT_TRUE(scene2);

    ASSERT_EQ(scene1->GetCameras().GetResult().size(), 1);
    ASSERT_EQ(scene1->GetAnimations().GetResult().size(), 1);

    auto plane = scene1->FindNode("//Scene/Plane").GetResult();
    ASSERT_TRUE(plane);

    auto ext = interface_cast<INodeImport>(plane);
    ASSERT_TRUE(ext);
    auto node = ext->ImportChildScene(scene2, "Test").GetResult();
    ASSERT_TRUE(node);
    EXPECT_EQ(node->GetPath().GetResult(), "//Scene/Plane/Test");

    ASSERT_EQ(scene1->GetCameras().GetResult().size(), 2);
    ASSERT_EQ(scene1->GetAnimations().GetResult().size(), 2);

    auto plane2 = scene1->FindNode("//Scene/Plane/Test/Scene/Plane").GetResult();
    ASSERT_TRUE(plane2);

    auto shell = scene1->FindNode("//Scene/Plane/Test/Scene/Shell").GetResult();
    ASSERT_TRUE(shell);
}

/**
 * @tc.name: ManualMode
 * @tc.desc: Tests for Manual Mode. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, ManualMode, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    scene->SetRenderMode(RenderMode::MANUAL).Wait();
    auto camera = scene->CreateNode<ICamera>("//test", ClassId::CameraNode).GetResult();
    ASSERT_TRUE(camera);

    IImage::Ptr bmap = CreateTestBitmap();
    ASSERT_TRUE(camera->SetRenderTarget(interface_pointer_cast<IRenderTarget>(bmap)).GetResult());

    size_t count = 0;
    auto dr = interface_cast<META_NS::IDynamicResource>(bmap);
    ASSERT_TRUE(dr);
    dr->OnResourceChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([&] { ++count; }));

    UpdateScene();
    EXPECT_EQ(count, 0);

    UpdateScene();
    EXPECT_EQ(count, 0);

    scene->GetInternalScene()->RenderFrame();

    UpdateScene();
    EXPECT_EQ(count, 1);

    UpdateScene();
    EXPECT_EQ(count, 1);

    scene->GetInternalScene()->RenderFrame();

    EXPECT_EQ(count, 1);
    UpdateScene();
    EXPECT_EQ(count, 2);
}

/**
 * @tc.name: LoadDestroy
 * @tc.desc: Tests for Load Destroy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, LoadDestroy, testing::ext::TestSize.Level1)
{
    for (int i = 0; i != 10; ++i) {
        auto m = META_NS::GetObjectRegistry().Create<SCENE_NS::ISceneManager>(SCENE_NS::ClassId::SceneManager, params);
        auto scene = m->CreateScene("test://celia/Celia.gltf").GetResult();
        ASSERT_TRUE(scene);
        scene->GetInternalScene()->RunDirectlyOrInTask([=] { scene->GetInternalScene()->Update(); });
    }
}

/**
 * @tc.name: NodeEnabled
 * @tc.desc: Tests for Node Enabled. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, NodeEnabled, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();

    auto testNode = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(testNode);

    auto p = testNode->Enabled();
    EXPECT_TRUE(p->GetValue());
    EXPECT_TRUE(p->GetDefaultValue());
    p->SetValue(false);
    EXPECT_FALSE(p->GetValue());
    EXPECT_TRUE(p->GetDefaultValue());
}

/**
 * @tc.name: CreateGltfResources
 * @tc.desc: Tests for Create Gltf Resources. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, CreateGltfResources, testing::ext::TestSize.Level1)
{
    CORE_NS::ResourceIdContext imageId;
    {
        auto scene = LoadScene("test_assets://AnimatedCube/AnimatedCube.gltf");
        ASSERT_TRUE(scene);
        auto root = scene->GetRootNode().GetResult();
        ASSERT_TRUE(root);
        EXPECT_EQ(root->GetChildren().GetResult().size(), 1);

        for (auto&& v : context->GetResources()->GetResourceInfos(scene)) {
            CORE_LOG_W("%s", v.id.ToString().c_str());
        }

        auto n = scene->FindNode<IMesh>("///AnimatedCube").GetResult();
        ASSERT_TRUE(n);

        auto subs = n->SubMeshes()->GetValue();
        ASSERT_TRUE(!subs.empty());

        auto mat = subs[0]->Material()->GetValue();
        ASSERT_TRUE(mat);
        {
            auto res = interface_pointer_cast<CORE_NS::IResource>(mat);
            ASSERT_TRUE(res);
            EXPECT_EQ(res->GetResourceId().group, "test_assets://AnimatedCube/AnimatedCube.gltf");

            auto qres = resources->GetResource({res->GetResourceId(), res->GetContext()});
            EXPECT_EQ(qres, res);
        }

        auto texs = mat->Textures()->GetValue();
        ASSERT_TRUE(!texs.empty());

        auto image = texs[0]->Image()->GetValue();
        ASSERT_TRUE(image);

        {
            auto res = interface_pointer_cast<CORE_NS::IResource>(image);
            ASSERT_TRUE(res);
            EXPECT_EQ(res->GetResourceId().group, "test_assets://AnimatedCube/AnimatedCube.gltf");

            imageId = {res->GetResourceId(), res->GetContext()};
            auto qres = resources->GetResource(imageId);
            EXPECT_EQ(qres, res);
        }

        auto anims = scene->GetAnimations().GetResult();
        ASSERT_EQ(anims.size(), 1);
        auto anim = anims[0];
        ASSERT_TRUE(anim);

        {
            auto res = interface_pointer_cast<CORE_NS::IResource>(anim);
            ASSERT_TRUE(res);
            EXPECT_EQ(res->GetResourceId().group, "test_assets://AnimatedCube/AnimatedCube.gltf");

            auto qres = resources->GetResource({res->GetResourceId(), res->GetContext()});
            EXPECT_EQ(qres, res);
        }

        auto groups = scene->GetResourceGroups();
        ASSERT_EQ(groups.GetAllHandles().size(), 1);
        EXPECT_EQ(groups.PrimaryGroup(), "test_assets://AnimatedCube/AnimatedCube.gltf");

        EXPECT_TRUE(!resources->GetResourceInfos("test_assets://AnimatedCube/AnimatedCube.gltf", scene).empty());
    }
    this->scene.reset();

    EXPECT_TRUE(resources->GetResourceInfos("test_assets://AnimatedCube/AnimatedCube.gltf", scene).empty());
    EXPECT_FALSE(resources->GetResource(imageId));
}

/**
 * @tc.name: ExternalNode
 * @tc.desc: Tests for External Node. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, ExternalNode, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto n1 = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(n1);

    auto entities = GetEntityCount();
    {
        auto ext = interface_cast<INodeImport>(n1);
        ASSERT_TRUE(ext);

        auto child = ext->ImportChildScene("test_assets://AnimatedCube/AnimatedCube.gltf", "new").GetResult();
        ASSERT_TRUE(child);
    }
    UpdateScene();

    EXPECT_TRUE(!scene->GetAnimations().GetResult().empty());

    EXPECT_TRUE(resources->GetResource(
        {{"AnimatedCube.gltf/images/0", "test_assets://AnimatedCube/AnimatedCube.gltf"}, scene}));
    EXPECT_TRUE(resources->GetResource(
        {{"AnimatedCube.gltf/images/1", "test_assets://AnimatedCube/AnimatedCube.gltf"}, scene}));
    EXPECT_TRUE(
        resources->GetResource({{"animation_AnimatedCube", "test_assets://AnimatedCube/AnimatedCube.gltf"}, scene}));

    BASE_NS::string extGroup = "test_assets://AnimatedCube/AnimatedCube.gltf";

    auto ch = scene->FindNode("//test/new").GetResult();
    ASSERT_TRUE(ch);

    scene->RemoveNode(BASE_NS::move(ch));

    UpdateScene();

    EXPECT_FALSE(resources->GetResource(
        {{"AnimatedCube.gltf/images/0", "test_assets://AnimatedCube/AnimatedCube.gltf"}, scene}));
    EXPECT_FALSE(resources->GetResource(
        {{"AnimatedCube.gltf/images/1", "test_assets://AnimatedCube/AnimatedCube.gltf"}, scene}));
    EXPECT_FALSE(
        resources->GetResource({{"animation_AnimatedCube", "test_assets://AnimatedCube/AnimatedCube.gltf"}, scene}));

    EXPECT_TRUE(scene->GetAnimations().GetResult().empty());
    EXPECT_EQ(entities, GetEntityCount());
    LogAliveEntities(scene);
}

/**
 * @tc.name: ImportSameSceneTwice
 * @tc.desc: Tests for Import Same Scene Twice. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, ImportSameSceneTwice, testing::ext::TestSize.Level1)
{
    auto cube = LoadScene("test_assets://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(cube);

    auto scene = CreateEmptyScene();
    auto iScene = scene->GetInternalScene();
    auto node = scene->CreateNode("//test").GetResult();

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
        ASSERT_EQ(groups.GetAllHandles().size(), 1);
        EXPECT_TRUE(groups.GetHandle("Scene"));
    }

    EXPECT_TRUE(scene->FindNode("//test/some").GetResult());
    EXPECT_TRUE(
        resources
            ->GetResourceInfo({{"AnimatedCube.gltf/images/0", "test_assets://AnimatedCube/AnimatedCube.gltf"}, scene})
            .id.IsValid());

    auto secondNode = ext->ImportChildScene(cube, "other").GetResult();
    ASSERT_TRUE(secondNode);

    {
        auto groups = iScene->GetResourceGroups();
        ASSERT_EQ(groups.GetAllHandles().size(), 1);
        EXPECT_TRUE(groups.GetHandle("Scene"));
    }

    EXPECT_TRUE(scene->FindNode("//test/some").GetResult());
    EXPECT_TRUE(scene->FindNode("//test/other").GetResult());
    EXPECT_TRUE(
        resources
            ->GetResourceInfo({{"AnimatedCube.gltf/images/0", "test_assets://AnimatedCube/AnimatedCube.gltf"}, scene})
            .id.IsValid());
    EXPECT_FALSE(resources
                     ->GetResourceInfo(
                         {{"AnimatedCube.gltf/images/0", "test_assets://AnimatedCube/AnimatedCube.gltf (1)"}, scene})
                     .id.IsValid());
}

/**
 * @tc.name: RemoveInactiveNodes
 * @tc.desc: Tests for Remove Inactive Nodes. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, RemoveInactiveNodes, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto n1 = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(n1);

    auto entities = GetEntityCount();
    {
        auto n2 = scene->CreateNode("//test/new").GetResult();
        ASSERT_TRUE(n2);

        auto n3 = scene->CreateNode("//test/new/1").GetResult();
        ASSERT_TRUE(n3);

        auto n4 = scene->CreateNode("//test/new/2").GetResult();
        ASSERT_TRUE(n4);
    }

    UpdateScene();

    auto node = scene->FindNode("//test/new").GetResult();
    ASSERT_TRUE(node);

    ASSERT_TRUE(n1->RemoveChild(node).GetResult());
    ASSERT_TRUE(scene->RemoveNode(BASE_NS::move(node)).GetResult());

    UpdateScene();

    EXPECT_EQ(entities, GetEntityCount());
}

/**
 * @tc.name: FindImportedAnimations
 * @tc.desc: Tests for Find Imported Animations. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, FindImportedAnimations, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto n1 = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(n1);

    auto entities = GetEntityCount();

    auto ext = interface_cast<INodeImport>(n1);
    ASSERT_TRUE(ext);

    auto child = ext->ImportChildScene("test_assets://AnimatedCube/AnimatedCube.gltf", "new").GetResult();
    ASSERT_TRUE(child);

    auto anims = GetImportedAnimations(child);
    ASSERT_EQ(anims.size(), 1);
    EXPECT_TRUE(anims[0].Valid());
}

/**
 * @tc.name: FindImportedMaterials
 * @tc.desc: Tests for Find Imported Materials. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, FindImportedMaterials, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto n1 = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(n1);

    auto ext = interface_cast<INodeImport>(n1);
    ASSERT_TRUE(ext);

    auto child = ext->ImportChildScene("test_assets://AnimatedCube/AnimatedCube.gltf", "new").GetResult();
    ASSERT_TRUE(child);

    auto mats = GetImportedMaterials(child);
    ASSERT_EQ(mats.size(), 1);
    EXPECT_TRUE(mats[0]);
    EXPECT_EQ(mats[0].GetType(), SCENE_NS::MaterialType::METALLIC_ROUGHNESS);
}

/**
 * @tc.name: FindImportedResources
 * @tc.desc: Tests for Find Imported Resources. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, FindImportedResources, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto n1 = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(n1);

    auto ext = interface_cast<INodeImport>(n1);
    ASSERT_TRUE(ext);

    auto child = ext->ImportChildScene("test_assets://AnimatedCube/AnimatedCube.gltf", "new").GetResult();
    ASSERT_TRUE(child);

    auto res = GetImportedResources(child, {});
    ASSERT_EQ(res.size(), 4);

    child = ext->ImportChildScene("test_assets://AnimatedCube/AnimatedCube.gltf", "new").GetResult();
    ASSERT_TRUE(child);

    res = GetImportedResources(child, {});
    ASSERT_EQ(res.size(), 4);

    child = ext->ImportChildScene("test_assets://AnimatedCube/AnimatedCube.gltf", "other").GetResult();
    ASSERT_TRUE(child);

    res = GetImportedResources(child, {});
    ASSERT_EQ(res.size(), 4);
}

/**
 * @tc.name: ImportSceneTwiceWithDeactivatedNode
 * @tc.desc: Tests for Import Scene Twice With Deactivated Node. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, ImportSceneTwiceWithDeactivatedNode, testing::ext::TestSize.Level1)
{
    auto cube = LoadScene("test_assets://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(cube);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto n = scene->GetRootNode().GetResult();
    ASSERT_TRUE(n);

    auto ext = interface_cast<INodeImport>(n);
    ASSERT_TRUE(ext);

    INode::Ptr child;

    {
        child = ext->ImportChildScene(cube, "new").GetResult();
        ASSERT_TRUE(child);

        auto anims = scene->GetAnimations().GetResult();
        ASSERT_EQ(anims.size(), 1);
        auto anim = anims[0];

        interface_cast<META_NS::IStartableAnimation>(anim)->Start();
        UpdateScene();
        EXPECT_TRUE(anim->Running()->GetValue());
        UpdateScene(META_NS::TimeSpan::Milliseconds(15));
        EXPECT_TRUE(anim->Progress()->GetValue() > 0);

        n->RemoveChild(child).Wait();
    }
    {
        auto child = ext->ImportChildScene(cube, "new").GetResult();
        ASSERT_TRUE(child);

        auto anims = scene->GetAnimations().GetResult();
        // remove child does not remove resources
        ASSERT_EQ(anims.size(), 2);
        auto anim = anims[0];

        interface_cast<META_NS::IStartableAnimation>(anim)->Start();
        UpdateScene();
        EXPECT_TRUE(anim->Running()->GetValue());
        UpdateScene(META_NS::TimeSpan::Milliseconds(15));
        EXPECT_TRUE(anim->Progress()->GetValue() > 0);
    }
    {
        n->AddChild(child).Wait();
        auto anims = scene->GetAnimations().GetResult();
        ASSERT_EQ(anims.size(), 2);
        EXPECT_NE(anims[0], anims[1]);

        for (auto&& anim : anims) {
            interface_cast<META_NS::IStartableAnimation>(anim)->Start();
            UpdateScene();
            EXPECT_TRUE(anim->Running()->GetValue());
            UpdateScene(META_NS::TimeSpan::Milliseconds(15));
            EXPECT_TRUE(anim->Progress()->GetValue() > 0);
        }
    }
}

/**
 * @tc.name: EditorSceneLoad
 * @tc.desc: Tests for Editor Scene Load. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, EditorSceneLoad, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test_assets://editor/scene_test_project/assets/default.scene");
    ASSERT_TRUE(scene);

    auto iScene = scene->GetInternalScene();
    ASSERT_TRUE(iScene);

    auto info = iScene->GetDebugInfo();
    // both of the external nodes are still alive
    EXPECT_EQ(info.nodeObjectsAlive, 2);
    EXPECT_EQ(info.animationObjectsAlive, 0);

    auto res = iScene->GetContext()->GetResources();
    ASSERT_TRUE(res);

    BASE_NS::vector<CORE_NS::MatchingResourceId> select;
    auto groups = iScene->GetResourceGroups();
    for (auto&& g : groups.GetAllHandles()) {
        auto name = g->GetGroup();
        select.push_back(CORE_NS::MatchingResourceId{name});
        CORE_LOG_D("Group: %s", name.c_str());
    }

    auto di = interface_cast<META_NS::IResourceQuery>(res);
    ASSERT_TRUE(di);
    auto count = di->GetAliveCount(select, scene);
    EXPECT_EQ(count, 0);
    if (count > 0) {
        CORE_LOG_W("Resources still alive:");
        for (auto&& r : di->FindAliveResources(select, scene)) {
            CORE_LOG_W("  %s", r->GetResourceId().ToString().c_str());
        }
    }
}

#ifndef __OHOS__
/**
 * @tc.name: JsonImporterV1DispatchLoads
 * @tc.desc: project.json with importVersion "JsonImporter v1" routes to the new
 *           JSON scene importer, which parses a JSON-importer-format scene.
 *           The "resources" array in project.json lists a resource index
 *           (resources.res) that registers a material; verify it lands in
 *           the render context's resource manager before the scene parses.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, JsonImporterV1DispatchLoads, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test_assets://json_importer_dispatch/v1/assets/test.scene");
    ASSERT_TRUE(scene);
    auto root = scene->GetRootNode().GetResult();
    ASSERT_TRUE(root);
    EXPECT_TRUE(scene->FindNode("//child_a").GetResult());
    EXPECT_TRUE(scene->FindNode("//child_b").GetResult());
    EXPECT_EQ(root->GetChildren().GetResult().size(), 2u);

    // Resource index listed in project.json was loaded ahead of the scene.
    auto info = resources->GetResourceInfo({"test_material", nullptr});
    EXPECT_TRUE(info.id.IsValid());
    EXPECT_EQ(info.type, SCENE_NS::ClassId::MaterialResource.Id().ToUid());
}
#endif

/**
 * @tc.name: JsonImporterV1DispatchVersionMismatch
 * @tc.desc: Same JSON-importer-format scene, but project.json's importVersion
 *           is not "JsonImporter v1" — the legacy path is selected instead, and
 *           legacy can't parse JSON-importer format, so loading fails.
 *           Proves the importVersion field actually toggles which importer runs.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, JsonImporterV1DispatchVersionMismatch, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test_assets://json_importer_dispatch/legacy/assets/test.scene");
    EXPECT_FALSE(scene);
}

/**
 * @tc.name: ResourceCountTest
 * @tc.desc: Tests for resource counters.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, ResourceCountTest, testing::ext::TestSize.Level1)
{
    // Verifies the resource counter invariants rather than absolute values, so the test survives
    // resource-count changes in the underlying render libraries. The key properties tested are:
    //   - loading a scene makes handle/resource counts grow
    //   - repeated rendering without changes does not shift counts
    //   - each load/unload pair round-trips back to the state before it
    //   - after unloading everything the counters return to the initial baseline (no leaks)
    auto uri = "test_assets://AnimatedCube/AnimatedCube.gltf";
    IScene::Ptr cube;
    IScene::Ptr cube2;

    auto render = [&](int count = 6) {
        for (int i = 0; i < count; i++) {
            if (cube) {
                UpdateSceneAndRenderFrame(cube);
            }
            if (cube2) {
                UpdateSceneAndRenderFrame(cube2);
            }
        }
    };
    auto getCounters = [&]() { return context ? context->GetCounters() : IRenderContext::Counters{}; };
    auto expectHandlesEq = [](const IRenderContext::Counters& actual,
                               const IRenderContext::Counters& expected,
                               BASE_NS::string_view label) {
        auto s = BASE_NS::string(label);
        EXPECT_EQ(actual.handles.bufferHandleCount, expected.handles.bufferHandleCount) << s.c_str();
        EXPECT_EQ(actual.handles.imageHandleCount, expected.handles.imageHandleCount) << s.c_str();
        EXPECT_EQ(actual.handles.samplerHandleCount, expected.handles.samplerHandleCount) << s.c_str();
        EXPECT_EQ(actual.resourceCount, expected.resourceCount) << s.c_str();
        EXPECT_EQ(actual.scenes.size(), expected.scenes.size()) << s.c_str();
        EXPECT_EQ(actual.totalSceneCount, expected.totalSceneCount) << s.c_str();
    };

    // 1. Baseline: no user-loaded scenes.
    {
        auto warmup = GetSceneManager()->CreateScene(uri).GetResult();
        ASSERT_TRUE(warmup);
        cube = warmup;
        render();
        cube.reset();
        warmup.reset();
        render();
    }
    const auto initial = getCounters();

    // 2. Load one scene: handles and resources must all grow.
    cube = GetSceneManager()->CreateScene(uri).GetResult();
    ASSERT_TRUE(cube);
    render();
    const auto afterOne = getCounters();
    EXPECT_EQ(afterOne.scenes.size(), initial.scenes.size() + 1);
    EXPECT_EQ(afterOne.totalSceneCount, initial.totalSceneCount + 1);
    EXPECT_GT(afterOne.resourceCount, initial.resourceCount);
    EXPECT_GT(afterOne.handles.bufferHandleCount, initial.handles.bufferHandleCount);
    EXPECT_GT(afterOne.handles.imageHandleCount, initial.handles.imageHandleCount);
    // Samplers are cached globally; loading a scene may not add any.
    EXPECT_GE(afterOne.handles.samplerHandleCount, initial.handles.samplerHandleCount);

    // 2b. Rendering further frames without changes must not drift counters.
    render();
    expectHandlesEq(getCounters(), afterOne, "stable after idle frames");

    // 3. Load a second scene of the same URI.
    cube2 = GetSceneManager()->CreateScene(uri).GetResult();
    ASSERT_TRUE(cube2);
    render();
    const auto afterTwo = getCounters();
    EXPECT_EQ(afterTwo.scenes.size(), initial.scenes.size() + 2);
    EXPECT_EQ(afterTwo.totalSceneCount, initial.totalSceneCount + 2);
    EXPECT_GT(afterTwo.resourceCount, afterOne.resourceCount);
    EXPECT_GE(afterTwo.handles.bufferHandleCount, afterOne.handles.bufferHandleCount);
    EXPECT_GE(afterTwo.handles.imageHandleCount, afterOne.handles.imageHandleCount);
    EXPECT_GE(afterTwo.handles.samplerHandleCount, afterOne.handles.samplerHandleCount);

    // 4. Import cube into cube2. Scene count must stay unchanged.
    auto cube2root = cube2->GetRootNode().GetResult();
    auto import = interface_cast<INodeImport>(cube2root);
    auto imported = import->ImportChildScene(cube, "OriginalCube").GetResult();
    EXPECT_TRUE(imported);
    render();
    const auto afterImport = getCounters();
    EXPECT_EQ(afterImport.scenes.size(), afterTwo.scenes.size());
    EXPECT_EQ(afterImport.totalSceneCount, afterTwo.totalSceneCount);

    // 5. Remove the imported node. Handle/resource counts must return to the pre-import state.
    EXPECT_TRUE(cube2->RemoveNode(BASE_NS::move(imported)).GetResult());
    render();
    expectHandlesEq(getCounters(), afterTwo, "removed imported node");

    // 6. Drop cube2. Counts must collapse back to the single-scene state.
    cube2root.reset();
    cube2.reset();
    render();
    expectHandlesEq(getCounters(), afterOne, "removed cube2 scene");

    // 7. Drop cube too. Everything must return to the initial baseline (no leaks).
    cube.reset();
    render();
    expectHandlesEq(getCounters(), initial, "removed all scenes");
}

/**
 * @tc.name: ModifyRenderNodeGraph
 * @tc.desc: Tests for modifying render node graph.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, ModifyRenderNodeGraph, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    ASSERT_TRUE(scene);
    UpdateScene();

    BASE_NS::vector<RENDER_NS::RenderHandleReference> rng{};

    scene->GetInternalScene()->ModifyCustomRenderNodeGraph(
        IInternalScene::RenderNodeGraphModificationMode::PREPEND, rng);
    UpdateScene();

    scene->GetInternalScene()->ModifyCustomRenderNodeGraph(
        IInternalScene::RenderNodeGraphModificationMode::REPLACE, rng);
    UpdateScene();

    scene->GetInternalScene()->ModifyCustomRenderNodeGraph(
        IInternalScene::RenderNodeGraphModificationMode::INVALID, rng);
    UpdateScene();

    rng.push_back(RENDER_NS::RenderHandleReference{});
    scene->GetInternalScene()->ModifyCustomRenderNodeGraph(
        IInternalScene::RenderNodeGraphModificationMode::PREPEND, rng);
    UpdateScene();

    rng.push_back(RENDER_NS::RenderHandleReference{});
    scene->GetInternalScene()->ModifyCustomRenderNodeGraph(
        IInternalScene::RenderNodeGraphModificationMode::APPEND, rng);
    UpdateScene();

    rng.push_back(RENDER_NS::RenderHandleReference{});
    scene->GetInternalScene()->ModifyCustomRenderNodeGraph(
        IInternalScene::RenderNodeGraphModificationMode::REPLACE, rng);
    UpdateScene();

    scene->GetInternalScene()->ModifyCustomRenderNodeGraph(
        IInternalScene::RenderNodeGraphModificationMode::INVALID, rng);
    UpdateScene();
}

/**
 * @tc.name: RenderMode
 * @tc.desc: Tests for render mode.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, RenderMode, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    ASSERT_TRUE(scene);
    EXPECT_TRUE(scene->SetRenderMode(RenderMode::ALWAYS).GetResult());
    EXPECT_EQ(scene->GetRenderMode().GetResult(), RenderMode::ALWAYS);
    UpdateScene();
    EXPECT_EQ(scene->GetRenderMode().GetResult(), RenderMode::ALWAYS);
    EXPECT_TRUE(scene->SetRenderMode(RenderMode::IF_DIRTY).GetResult());
    EXPECT_EQ(scene->GetRenderMode().GetResult(), RenderMode::IF_DIRTY);
    UpdateScene();
    EXPECT_EQ(scene->GetRenderMode().GetResult(), RenderMode::IF_DIRTY);
    EXPECT_TRUE(scene->SetRenderMode(RenderMode::MANUAL).GetResult());
    EXPECT_EQ(scene->GetRenderMode().GetResult(), RenderMode::MANUAL);
    UpdateScene();
    EXPECT_EQ(scene->GetRenderMode().GetResult(), RenderMode::MANUAL);
}

/**
 * @tc.name: AutoUpdate
 * @tc.desc: Tests for auto update.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, AutoUpdate, testing::ext::TestSize.Level1)
{
    {
        auto sc = CreateStandaloneScene();
        ASSERT_TRUE(sc);
        sc->StartAutoUpdate(META_NS::TimeSpan::Milliseconds(16));
        UpdateScene(sc);
        sc->StartAutoUpdate(META_NS::TimeSpan::Milliseconds(10));
        UpdateScene(sc);
    }
}

/**
 * @tc.name: InvalidScene
 * @tc.desc: Tests for auto update.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, InvalidScene, testing::ext::TestSize.Level1)
{
    auto scene = META_NS::GetObjectRegistry().Create(ClassId::Scene);
    ASSERT_FALSE(scene);
}

/**
 * @tc.name: GetInvalidComponent
 * @tc.desc: Tests for invalid component.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, GetInvalidComponent, testing::ext::TestSize.Level1)
{
    CreateEmptyScene();
    ASSERT_TRUE(scene);
    scene->GetComponent(nullptr, "");
}

/**
 * @tc.name: SceneRenderContext
 * @tc.desc: Tests Scene's RenderContext
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, SceneRenderContext, testing::ext::TestSize.Level1)
{
    auto context = META_NS::GetObjectRegistry().Create<IRenderContext>(ClassId::RenderContext);
    ASSERT_TRUE(context);
    auto counters = context->GetCounters();
    EXPECT_EQ(counters.scenes.size(), 0);

    CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto ctx = scene->GetInternalScene()->GetContext()->GetRenderer();
    EXPECT_TRUE(context->Initialize(ctx, {}, {}, {}));
    EXPECT_FALSE(context->Initialize(ctx, {}, {}, {}));

    counters = context->GetCounters();
    EXPECT_EQ(counters.scenes.size(), 1);
}

/**
 * @tc.name: NodeImportNotShareResources
 * @tc.desc: Tests for Node Import Not Share Resources. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePlugin, NodeImportNotShareResources, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();

    auto m = GetSceneManager();
    ASSERT_TRUE(m);

    BASE_NS::string uri = "test_assets://AnimatedCube/AnimatedCube.gltf";

    auto imp1 = m->CreateScene(uri).GetResult();
    ASSERT_TRUE(imp1);

    auto imp2 = m->CreateScene(uri).GetResult();
    ASSERT_TRUE(imp2);

    auto getMat = [](IScene::Ptr scene, BASE_NS::string path) -> IMaterial::Ptr {
        auto n = scene->FindNode<IMesh>(path).GetResult();
        if (!n) {
            return nullptr;
        }
        auto subs = n->SubMeshes()->GetValue();
        if (subs.empty()) {
            return nullptr;
        }
        return subs[0]->Material()->GetValue();
    };

    auto gm1 = getMat(imp1, "///AnimatedCube");
    ASSERT_TRUE(gm1);
    auto gm2 = getMat(imp2, "///AnimatedCube");
    ASSERT_TRUE(gm2);

    {
        auto ext = interface_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(ext);
        auto n = ext->ImportChildScene(imp1, "1a").GetResult();
        ASSERT_TRUE(n);
    }
    {
        auto ext = interface_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(ext);
        auto n = ext->ImportChildScene(imp1, "1b").GetResult();
        ASSERT_TRUE(n);
    }
    {
        auto ext = interface_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(ext);
        auto n = ext->ImportChildScene(imp2, "2a").GetResult();
        ASSERT_TRUE(n);
    }
    {
        auto ext = interface_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(ext);
        INodeImport::ImportOptions opts;
        opts.nodeName = "2b";
        opts.shareResources = false;
        auto n = ext->ImportChildScene(imp2, opts).GetResult();
        ASSERT_TRUE(n);
    }

    auto mat1a = getMat(scene, "//1a//AnimatedCube");
    ASSERT_TRUE(mat1a);
    auto mat1b = getMat(scene, "//1b//AnimatedCube");
    ASSERT_TRUE(mat1b);
    auto mat2a = getMat(scene, "//2a//AnimatedCube");
    ASSERT_TRUE(mat2a);
    auto mat2b = getMat(scene, "//2b//AnimatedCube");
    ASSERT_TRUE(mat2b);

    EXPECT_EQ(mat1a, mat1b);
    EXPECT_EQ(mat1a, mat2a);
    EXPECT_NE(mat2a, mat2b);

    EXPECT_NE(mat1a, gm1);
    EXPECT_NE(mat2b, gm1);
    EXPECT_NE(mat1a, gm2);
    EXPECT_NE(mat2b, gm2);
}

}  // namespace UTest

SCENE_END_NAMESPACE()
