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

#include <scene/interface/intf_create_mesh.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>

#include <3d/ecs/components/material_component.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginMeshNodeTest : public ScenePluginTest {
public:
    IMesh::Ptr mesh_;
    void CreateSubmeshes(IMesh::Ptr mesh)
    {
        for (int i = 0; i != 3; ++i) {
            AddSubMesh(mesh);
        }

        size_t i = 0;
        for (auto&& alphaCutoff : { 1.2345f, 2.3456f, 3.4567f }) {
            auto material = CreateMaterial();
            material->AlphaCutoff()->SetValue(alphaCutoff);
            mesh->SubMeshes()->GetValueAt(i++)->Material()->SetValue(material);
        }
    }

    void CheckEquals(const BASE_NS::vector<ISubMesh::Ptr>& a) const
    {
        BASE_NS::vector<float> alphaCutoff { 1.2345f, 2.3456f, 3.4567f };
        ASSERT_EQ(a.size(), alphaCutoff.size());
        for (auto i = 0; i < a.size(); i++) {
            auto actual = a.at(i)->Material()->GetValue()->AlphaCutoff()->GetValue();
            auto expected = alphaCutoff.at(i);
            EXPECT_EQ(actual, expected);
        }
    }

    void SetMaterialOverride(float magicAlphaCutoffValue)
    {
        const auto material = CreateMaterial();
        ASSERT_TRUE(material);
        material->AlphaCutoff()->SetValue(magicAlphaCutoffValue);
        UpdateScene();
        EXPECT_TRUE(SetMaterialForAllSubMeshes(mesh_, material));
    }

    void CheckMaterialOverride(float magicAlphaCutoffValue) const
    {
        const auto submeshes = mesh_->SubMeshes()->GetValue();
        EXPECT_FALSE(submeshes.empty());
        for (auto&& submesh : submeshes) {
            auto actualMaterial = submesh->Material()->GetValue();
            ASSERT_TRUE(actualMaterial);
            EXPECT_EQ(actualMaterial->AlphaCutoff()->GetValue(), magicAlphaCutoffValue);
        }
    }

protected:
    void SetUp() override
    {
        ScenePluginTest::SetUp();
        auto scene = CreateEmptyScene();
        auto node = scene->CreateNode("//test", ClassId::MeshNode).GetResult();
        ASSERT_TRUE(node);
        mesh_ = interface_pointer_cast<IMesh>(node);
        ASSERT_TRUE(mesh_);
    }

    void TearDown() override
    {
        mesh_.reset();
    }
};

/**
 * @tc.name: MaterialOverrideExistingSubmeshes
 * @tc.desc: Tests for Material Override Existing Submeshes. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshNodeTest, MaterialOverrideExistingSubmeshes, testing::ext::TestSize.Level1)
{
    CreateSubmeshes(mesh_);
    SetMaterialOverride(1.2345f);
    CheckMaterialOverride(1.2345f);
}

/**
 * @tc.name: GetSubmeshesEmpty
 * @tc.desc: Tests for Get Submeshes Empty. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshNodeTest, GetSubmeshesEmpty, testing::ext::TestSize.Level1)
{
    EXPECT_TRUE(mesh_->SubMeshes()->GetValue().empty());
}

/**
 * @tc.name: SubmeshRoundTrip
 * @tc.desc: Tests for Submesh Round Trip. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshNodeTest, SubmeshRoundTrip, testing::ext::TestSize.Level1)
{
    CreateSubmeshes(mesh_);
    auto submeshesOut = mesh_->SubMeshes()->GetValue();
    CheckEquals(submeshesOut);
}

/**
 * @tc.name: Uninitialized
 * @tc.desc: Tests for Uninitialized. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshNodeTest, Uninitialized, testing::ext::TestSize.Level1)
{
    auto mesh = META_NS::GetObjectRegistry().Create<IMesh>(ClassId::MeshNode);
    ASSERT_TRUE(mesh);
    auto submeshes = mesh_->SubMeshes()->GetValue();
    EXPECT_TRUE(submeshes.empty());
}

/**
 * @tc.name: SetMesh
 * @tc.desc: Tests for Set Mesh. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshNodeTest, SetMesh, testing::ext::TestSize.Level1)
{
    auto mc = scene->CreateObject<ICreateMesh>(ClassId::MeshCreator).GetResult();
    ASSERT_TRUE(mc);
    auto m = mc->CreatePlane({}, 10, 5).GetResult();
    ASSERT_TRUE(m);

    EXPECT_TRUE(mesh_->SubMeshes()->GetValue().empty());

    auto acc = interface_cast<IMeshAccess>(mesh_);
    ASSERT_TRUE(acc);

    EXPECT_TRUE(acc->SetMesh(m).GetResult());
    EXPECT_TRUE(!mesh_->SubMeshes()->GetValue().empty());

    EXPECT_EQ(acc->GetMesh().GetResult(), m);
}

/**
 * @tc.name: DestroyMesh
 * @tc.desc: Tests for Destroy Mesh. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshNodeTest, DestroyMesh, testing::ext::TestSize.Level1)
{
    auto& eman = scene->GetInternalScene()->GetEcsContext().GetNativeEcs()->GetEntityManager();
    {
        UpdateScene();
        CORE_NS::Entity ent;
        size_t entities = GetEntityCount();
        {
            auto mesh = scene->CreateObject(ClassId::Mesh).GetResult();
            ASSERT_TRUE(mesh);
            if (auto acc = interface_cast<IEcsObjectAccess>(mesh)) {
                ent = acc->GetEcsObject()->GetEntity();
            }
            EXPECT_TRUE(CORE_NS::EntityUtil::IsValid(ent));
        }
        UpdateScene();
        EXPECT_EQ(GetEntityCount(), entities);
        EXPECT_FALSE(eman.GetReferenceCounted(ent));
    }
    {
        CORE_NS::Entity ent;
        size_t entities = GetEntityCount();
        {
            auto mc = scene->CreateObject<ICreateMesh>(ClassId::MeshCreator).GetResult();
            ASSERT_TRUE(mc);
            auto m = mc->CreatePlane({}, 10, 5).GetResult();
            ASSERT_TRUE(m);
            if (auto acc = interface_cast<IEcsObjectAccess>(m)) {
                ent = acc->GetEcsObject()->GetEntity();
            }
            EXPECT_TRUE(CORE_NS::EntityUtil::IsValid(ent));
        }
        UpdateScene();
        EXPECT_EQ(GetEntityCount(), entities);
        EXPECT_FALSE(eman.GetReferenceCounted(ent));
    }
    {
        CORE_NS::Entity ent;
        {
            auto node = scene->CreateNode("//mesh1", ClassId::MeshNode).GetResult();
            ASSERT_TRUE(node);
            {
                auto mesh = interface_pointer_cast<IMesh>(node);

                auto mc = scene->CreateObject<ICreateMesh>(ClassId::MeshCreator).GetResult();
                ASSERT_TRUE(mc);
                auto m = mc->CreatePlane({}, 10, 5).GetResult();
                ASSERT_TRUE(m);
                if (auto acc = interface_cast<IEcsObjectAccess>(m)) {
                    ent = acc->GetEcsObject()->GetEntity();
                }
                EXPECT_TRUE(CORE_NS::EntityUtil::IsValid(ent));

                auto acc = interface_cast<IMeshAccess>(mesh);
                ASSERT_TRUE(acc);
                EXPECT_TRUE(acc->SetMesh(m).GetResult());
            }
            size_t entities = GetEntityCount();
            EXPECT_TRUE(scene->ReleaseNode(BASE_NS::move(node), false).GetResult());
            UpdateScene();
            EXPECT_EQ(GetEntityCount(), entities);
        }
        EXPECT_TRUE(eman.GetReferenceCounted(ent));
    }
    {
        CORE_NS::Entity ent;
        size_t entities = GetEntityCount();
        {
            auto node = scene->CreateNode("//mesh2", ClassId::MeshNode).GetResult();
            ASSERT_TRUE(node);
            auto mesh = interface_pointer_cast<IMesh>(node);

            auto mc = scene->CreateObject<ICreateMesh>(ClassId::MeshCreator).GetResult();
            ASSERT_TRUE(mc);
            auto m = mc->CreatePlane({}, 10, 5).GetResult();
            ASSERT_TRUE(m);
            if (auto acc = interface_cast<IEcsObjectAccess>(m)) {
                ent = acc->GetEcsObject()->GetEntity();
            }
            EXPECT_TRUE(CORE_NS::EntityUtil::IsValid(ent));

            auto acc = interface_cast<IMeshAccess>(mesh);
            ASSERT_TRUE(acc);
            EXPECT_TRUE(acc->SetMesh(m).GetResult());

            EXPECT_TRUE(scene->RemoveNode(BASE_NS::move(node)).GetResult());
        }
        UpdateScene();
        EXPECT_EQ(GetEntityCount(), entities);
        EXPECT_FALSE(eman.GetReferenceCounted(ent));
    }
    {
        CORE_NS::Entity ent;
        size_t entities = GetEntityCount();
        INode::Ptr node;
        {
            node = scene->CreateNode("//mesh3", ClassId::MeshNode).GetResult();
            ASSERT_TRUE(node);
            auto mesh = interface_pointer_cast<IMesh>(node);

            auto mc = scene->CreateObject<ICreateMesh>(ClassId::MeshCreator).GetResult();
            ASSERT_TRUE(mc);
            auto m = mc->CreatePlane({}, 10, 5).GetResult();
            ASSERT_TRUE(m);
            if (auto acc = interface_cast<IEcsObjectAccess>(m)) {
                ent = acc->GetEcsObject()->GetEntity();
            }
            EXPECT_TRUE(CORE_NS::EntityUtil::IsValid(ent));

            auto acc = interface_cast<IMeshAccess>(mesh);
            ASSERT_TRUE(acc);
            EXPECT_TRUE(acc->SetMesh(m).GetResult());

            auto other = node;
            EXPECT_TRUE(scene->RemoveNode(BASE_NS::move(other)).GetResult());
        }
        UpdateScene();
        EXPECT_TRUE(eman.GetReferenceCounted(ent));
        node.reset();
        UpdateScene();
        EXPECT_EQ(GetEntityCount(), entities);
        EXPECT_FALSE(eman.GetReferenceCounted(ent));
    }
}

/**
 * @tc.name: ImportMeshNode
 * @tc.desc: Tests for Import Mesh Node. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshNodeTest, ImportMeshNode, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto child = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(child);

    {
        auto scene2 = CreateEmptyScene();
        ASSERT_TRUE(scene2);
        auto meshCreator = scene2->CreateObject<ICreateMesh>(ClassId::MeshCreator).GetResult();
        ASSERT_TRUE(meshCreator);
        auto n = scene2->CreateNode<IMesh>("//mesh", ClassId::MeshNode).GetResult();
        ASSERT_TRUE(n);

        auto meshacc = interface_cast<IMeshAccess>(n);
        ASSERT_TRUE(meshacc);
        auto cube = meshCreator->CreateCube({}, 10, 10, 10).GetResult();
        ASSERT_TRUE(cube);
        meshacc->SetMesh(cube).Wait();

        auto subs = n->SubMeshes()->GetValue();
        ASSERT_TRUE(!subs.empty());

        if (auto i = interface_cast<INodeImport>(child)) {
            ASSERT_TRUE(i->ImportChildScene(scene2, "ext"));
        }
    }

    auto n = scene->FindNode<IMesh>("//test/ext/mesh").GetResult();
    ASSERT_TRUE(n);

    auto subs = n->SubMeshes()->GetValue();
    ASSERT_TRUE(!subs.empty());
}

} // namespace UTest

SCENE_END_NAMESPACE()
