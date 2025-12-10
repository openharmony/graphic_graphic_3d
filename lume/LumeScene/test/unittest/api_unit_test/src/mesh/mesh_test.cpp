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

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginMeshTest : public ScenePluginTest {
public:
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

    META_NS::ArrayProperty<CORE3D_NS::MeshComponent::Submesh> GetSubmeshes(IMesh::Ptr mesh)
    {
        if (auto i = interface_pointer_cast<IMeshAccess>(mesh)) {
            mesh = i->GetMesh().GetResult();
        }
        auto acc = interface_cast<IEcsObjectAccess>(mesh);
        if (acc) {
            auto ecsObj = acc->GetEcsObject();
            if (ecsObj) {
                return META_NS::ArrayProperty<CORE3D_NS::MeshComponent::Submesh>(
                    ecsObj->CreateProperty("MeshComponent.submeshes").GetResult());
            }
        }
        return {};
    }

    void AddSubMeshToFront(IMesh::Ptr mesh)
    {
        auto arr = GetSubmeshes(mesh);
        ASSERT_TRUE(arr);
        arr->InsertValueAt(0, {});
        UpdateScene();
        // wait for running notifications on changed engine properties
        WaitForUserQueue();
    }

    void ClearSubMeshes(IMesh::Ptr mesh)
    {
        auto arr = GetSubmeshes(mesh);
        ASSERT_TRUE(arr);
        arr->SetValue({});
        UpdateScene();
        // wait for running notifications on changed engine properties
        WaitForUserQueue();
    }

    IMesh::Ptr mesh_;
};

/**
 * @tc.name: CreatePlaneMesh
 * @tc.desc: Tests for Create Plane Mesh. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshTest, CreatePlaneMesh, testing::ext::TestSize.Level1)
{
    auto mc = scene->CreateObject<ICreateMesh>(ClassId::MeshCreator).GetResult();
    ASSERT_TRUE(mc);
    auto m = mc->CreatePlane({}, 10, 5).GetResult();
    ASSERT_TRUE(m);
    EXPECT_NE(m->AABBMin()->GetValue(), (BASE_NS::Math::Vec3 {}));
    EXPECT_TRUE(!m->SubMeshes()->GetValue().empty());
}

/**
 * @tc.name: CreateCubeMesh
 * @tc.desc: Tests for Create Cube Mesh. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshTest, CreateCubeMesh, testing::ext::TestSize.Level1)
{
    auto mc = scene->CreateObject<ICreateMesh>(ClassId::MeshCreator).GetResult();
    ASSERT_TRUE(mc);
    auto m = mc->CreateCube({}, 10, 10, 5).GetResult();
    ASSERT_TRUE(m);
    EXPECT_NE(m->AABBMin()->GetValue(), (BASE_NS::Math::Vec3 {}));
    EXPECT_TRUE(!m->SubMeshes()->GetValue().empty());
}
/**
 * @tc.name: CreateSpherMesh
 * @tc.desc: Tests for Create Spher Mesh. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshTest, CreateSpherMesh, testing::ext::TestSize.Level1)
{
    auto mc = scene->CreateObject<ICreateMesh>(ClassId::MeshCreator).GetResult();
    ASSERT_TRUE(mc);
    auto m = mc->CreateSphere({}, 10, 5, 5).GetResult();
    ASSERT_TRUE(m);
    EXPECT_NE(m->AABBMin()->GetValue(), (BASE_NS::Math::Vec3 {}));
    EXPECT_TRUE(!m->SubMeshes()->GetValue().empty());
}

/**
 * @tc.name: CreateConeMesh
 * @tc.desc: Tests for Create Cone Mesh. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshTest, CreateConeMesh, testing::ext::TestSize.Level1)
{
    auto mc = scene->CreateObject<ICreateMesh>(ClassId::MeshCreator).GetResult();
    ASSERT_TRUE(mc);
    auto m = mc->CreateCone({}, 10, 5, 5).GetResult();
    ASSERT_TRUE(m);
    EXPECT_NE(m->AABBMin()->GetValue(), (BASE_NS::Math::Vec3 {}));
    EXPECT_TRUE(!m->SubMeshes()->GetValue().empty());
}

/**
 * @tc.name: CreateCustomMesh
 * @tc.desc: Tests for Create Custom Mesh. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshTest, CreateCustomMesh, testing::ext::TestSize.Level1)
{
    auto mc = scene->CreateObject<ICreateMesh>(ClassId::MeshCreator).GetResult();
    ASSERT_TRUE(mc);

    CustomMeshData data;
    data.vertices = { { -1, -1, -1 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
    data.indices = { 0, 1, 2, 2, 1, 3 };

    auto m = mc->Create({}, data).GetResult();
    ASSERT_TRUE(m);
    EXPECT_NE(m->AABBMin()->GetValue(), (BASE_NS::Math::Vec3 {}));
    EXPECT_TRUE(!m->SubMeshes()->GetValue().empty());
}

/**
 * @tc.name: CreateCylinderMesh
 * @tc.desc: Tests for Create Cylinder Mesh.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshTest, CreateCylinderMesh, testing::ext::TestSize.Level1)
{
    auto mc = scene->CreateObject<ICreateMesh>(ClassId::MeshCreator).GetResult();
    ASSERT_TRUE(mc);
    auto m = mc->CreateCylinder({}, 1, 1, 25).GetResult();
    ASSERT_TRUE(m);
    EXPECT_NE(m->AABBMin()->GetValue(), (BASE_NS::Math::Vec3 {}));
    EXPECT_TRUE(!m->SubMeshes()->GetValue().empty());
}

/**
 * @tc.name: SubMesh
 * @tc.desc: Tests for Sub Mesh. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshTest, SubMesh, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test", ClassId::MeshNode).GetResult();
    ASSERT_TRUE(node);
    auto mesh = interface_pointer_cast<IMesh>(node);
    ASSERT_TRUE(mesh);
    AddSubMesh(mesh);
    auto submesh = mesh->SubMeshes()->GetValueAt(0);
    ASSERT_TRUE(submesh);
    submesh->AABBMin()->SetValue({ -1, -1, -1 });
    submesh->AABBMax()->SetValue({ 1, 1, 1 });
    auto mat1 = CreateMaterial();
    submesh->Material()->SetValue(mat1);
    EXPECT_EQ(submesh->Material()->GetValue(), mat1);
    UpdateScene();
    EXPECT_EQ(submesh->Material()->GetValue(), mat1);
    auto mat2 = CreateMaterial();
    submesh->Material()->SetValue(mat2);
    EXPECT_EQ(submesh->Material()->GetValue(), mat2);
    UpdateScene();
    EXPECT_EQ(submesh->Material()->GetValue(), mat2);
}

/**
 * @tc.name: AddSubMesh
 * @tc.desc: Tests for Add Sub Mesh. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshTest, AddSubMesh, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test", ClassId::MeshNode).GetResult();
    ASSERT_TRUE(node);
    auto mesh = interface_pointer_cast<IMesh>(node);
    ASSERT_TRUE(mesh);

    EXPECT_EQ(mesh->SubMeshes()->GetValue().size(), 0);
    AddSubMesh(mesh);
    auto submesh = mesh->SubMeshes()->GetValueAt(0);
    ASSERT_TRUE(submesh);
    submesh->AABBMin()->SetValue({ -1, -1, -1 });
    submesh->AABBMax()->SetValue({ 1, 1, 1 });
    UpdateScene();
    EXPECT_EQ(mesh->AABBMin()->GetValue(), (BASE_NS::Math::Vec3 { -1, -1, -1 }));
    EXPECT_EQ(mesh->AABBMax()->GetValue(), (BASE_NS::Math::Vec3 { 1, 1, 1 }));

    EXPECT_EQ(mesh->SubMeshes()->GetValue().size(), 1);
    AddSubMesh(mesh);
    EXPECT_EQ(mesh->SubMeshes()->GetValue().size(), 2);
    AddSubMeshToFront(mesh);
    EXPECT_EQ(mesh->SubMeshes()->GetValue().size(), 3);
    EXPECT_EQ(submesh, mesh->SubMeshes()->GetValueAt(0));
    EXPECT_EQ(submesh->AABBMin()->GetValue(), (BASE_NS::Math::Vec3 { 0, 0, 0 }));
    EXPECT_EQ(submesh->AABBMax()->GetValue(), (BASE_NS::Math::Vec3 { 0, 0, 0 }));
    {
        auto submesh = mesh->SubMeshes()->GetValueAt(0);
        ASSERT_TRUE(submesh);
        EXPECT_EQ(submesh->AABBMin()->GetValue(), (BASE_NS::Math::Vec3 { 0, 0, 0 }));
        EXPECT_EQ(submesh->AABBMax()->GetValue(), (BASE_NS::Math::Vec3 { 0, 0, 0 }));
    }
    {
        auto submesh = mesh->SubMeshes()->GetValueAt(1);
        ASSERT_TRUE(submesh);
        EXPECT_EQ(submesh->AABBMin()->GetValue(), (BASE_NS::Math::Vec3 { -1, -1, -1 }));
        EXPECT_EQ(submesh->AABBMax()->GetValue(), (BASE_NS::Math::Vec3 { 1, 1, 1 }));
    }
    ClearSubMeshes(mesh);
    EXPECT_EQ(mesh->SubMeshes()->GetValue().size(), 0);
}

} // namespace UTest
SCENE_END_NAMESPACE()
