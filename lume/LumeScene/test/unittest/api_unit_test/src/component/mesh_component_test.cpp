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

#include <scene/api/scene.h>
#include <scene/ext/component_util.h>
#include <scene/interface/intf_scene.h>
#include <test_framework.h>

#include <3d/ecs/components/mesh_component.h>
#include <3d/namespace.h>
#include <base/containers/array_view.h>
#include <base/containers/vector.h>

#include "scene/scene_component_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginMeshComponentTest : public ScenePluginComponentTest<CORE3D_NS::IMeshComponentManager> {
protected:
    template<class T>
    void TestIMeshPropertyGetters(T* mesh)
    {
        EXPECT_TRUE(mesh->AABBMin());
        EXPECT_TRUE(mesh->AABBMax());
        EXPECT_TRUE(mesh->SubMeshes());
    }

    void TestIMeshFunctions(IMesh* mesh)
    {
        ASSERT_TRUE(mesh);
        TestIMeshPropertyGetters<const IMesh>(mesh);
        TestIMeshPropertyGetters<IMesh>(mesh);
        TestIMeshPropertyGetters<const IMesh>(mesh);
        TestIMeshPropertyGetters<IMesh>(mesh);
        TestInvalidComponentPropertyInit(mesh);
    }
};

/**
 * @tc.name: Members
 * @tc.desc: Tests for Members. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshComponentTest, Members, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    const auto node = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(AddComponent<CORE3D_NS::IMeshComponentManager>(node));
    SetComponent(node, "MeshComponent");

    TestEngineProperty<BASE_NS::Math::Vec3>("AABBMax", { 1.0f, 2.0f, 3.0f }, nativeComponent.aabbMax);
    TestEngineProperty<BASE_NS::Math::Vec3>("AABBMin", { 4.0f, 5.0f, 6.0f }, nativeComponent.aabbMin);
    {
        auto prop = GetArrayProperty<CORE3D_NS::MeshComponent::Submesh>("SubMeshes");
        auto submeshes = BASE_NS::vector<CORE3D_NS::MeshComponent::Submesh> {};
        for (int i = 0; i < 3; ++i) {
            auto submesh = CORE3D_NS::MeshComponent::Submesh {};
            // Use any simple member to store data for differentiating between the submeshes.
            submesh.indexCount = i;
            submeshes.emplace_back(submesh);
        }
        prop->SetValue(submeshes);

        UpdateComponentMembers();

        const auto propValues = prop->GetValue();
        ASSERT_EQ(propValues.size(), 3);
        for (int i = 0; i < 3; ++i) {
            EXPECT_EQ(propValues[i].indexCount, nativeComponent.submeshes[i].indexCount);
        }
    }
    {
        const auto prop = GetArrayProperty<float>("JointBounds");
        const auto jointBounds = BASE_NS::vector<float> { 1.0f, 2.0f, 3.0f };
        prop->SetValue(jointBounds);

        UpdateComponentMembers();

        const auto propValues = prop->GetValue();
        ASSERT_EQ(propValues.size(), 3);
        for (int i = 0; i < 3; ++i) {
            EXPECT_EQ(propValues[i], nativeComponent.jointBounds[i]);
        }
    }
}

/**
 * @tc.name: Functions
 * @tc.desc: Tests for Functions. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshComponentTest, Functions, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    const auto node = scene->CreateNode("//test", ClassId::MeshNode).GetResult();
    ASSERT_TRUE(AddComponent<CORE3D_NS::IMeshComponentManager>(node));
    SetComponent(node, "MeshComponent");

    auto ma = interface_cast<IMeshAccess>(node);
    ASSERT_TRUE(ma);
    auto nodeMesh = ma->GetMesh().GetResult();
    TestIMeshFunctions(nodeMesh.get());
}

/**
 * @tc.name: SubMesh
 * @tc.desc: Tests for Sub Mesh. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshComponentTest, SubMesh, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    auto node = scene->CreateNode("//test", ClassId::MeshNode).GetResult();
    auto mesh = interface_pointer_cast<IMesh>(node);
    ASSERT_TRUE(mesh);
    auto meshacc = interface_cast<IMeshAccess>(mesh);
    ASSERT_TRUE(meshacc);
    object = interface_pointer_cast<META_NS::IObject>(meshacc->GetMesh().GetResult());
    ASSERT_TRUE(object);

    AddSubMesh(mesh);
    auto submesh = mesh->SubMeshes()->GetValueAt(0);
    ASSERT_TRUE(submesh);
    submesh->AABBMin()->SetValue({ -1, -1, -1 });
    submesh->AABBMax()->SetValue({ 1, 1, 1 });
    auto mat = CreateMaterial();
    submesh->Material()->SetValue(mat);
    EXPECT_EQ(submesh->Material()->GetValue(), mat);
    UpdateScene();
    UpdateComponentMembers();
    ASSERT_EQ(nativeComponent.submeshes.size(), 1);
    EXPECT_EQ(
        nativeComponent.submeshes[0].material, interface_cast<IEcsObjectAccess>(mat)->GetEcsObject()->GetEntity());
}

/**
 * @tc.name: MaterialOverride
 * @tc.desc: Tests for Material Override. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginMeshComponentTest, MaterialOverride, testing::ext::TestSize.Level1)
{
    auto getEntity = [](const BASE_NS::shared_ptr<CORE_NS::IInterface>& ptr) {
        auto ecso = interface_cast<IEcsObjectAccess>(ptr);
        return ecso ? ecso->GetEcsObject()->GetEntity() : CORE_NS::Entity {};
    };

    auto getSubMeshMaterialEntity = [](const BASE_NS::shared_ptr<CORE_NS::IInterface>& ptr, size_t index) {
        CORE_NS::Entity entity;
        if (auto ecsoa = interface_cast<IEcsObjectAccess>(ptr)) {
            if (auto ecso = ecsoa->GetEcsObject()) {
                auto ecs = ecso->GetScene()->GetEcsContext().GetNativeEcs();
                CORE3D_NS::IMeshComponentManager* mgr = static_cast<CORE3D_NS::IMeshComponentManager*>(
                    ecs->GetComponentManager(CORE3D_NS::IMeshComponentManager::UID));
                if (auto rh = mgr->Read(ecso->GetEntity())) {
                    entity = rh->submeshes[index].material;
                }
            }
        }
        return entity;
    };

    auto scene = Scene(LoadScene("test://AnimatedCube/AnimatedCube.gltf"));
    auto cube = Geometry(scene.FindNode("AnimatedCube"));
    ASSERT_TRUE(cube);
    auto mesh = cube.GetMesh();
    ASSERT_GT(mesh.GetSubMeshCount(), 0);
    auto submesh = mesh.GetSubMeshAt(0);
    ASSERT_TRUE(submesh);

    auto override = mesh.GetPtr<IMaterialOverride>();
    ASSERT_TRUE(override);

    UpdateScene();

    // Get original submesh material without touching the material property
    auto submeshMaterialEntityBeforeOverride = getSubMeshMaterialEntity(mesh, 0);
    EXPECT_TRUE(CORE_NS::EntityUtil::IsValid(submeshMaterialEntityBeforeOverride));

    // Make sure material property has not been instantiated to validate that override material gets correctly handled
    // even if submesh's Material property has not been initialized before setting the override
    EXPECT_FALSE(submesh.GetPtr<META_NS::IMetadata>()->GetProperty("Material", META_NS::MetadataQuery::EXISTING));

    auto overrideMaterial = CreateMaterial();
    auto overrideMaterial2 = CreateMaterial();

    // Set override
    override->SetMaterialOverride(overrideMaterial);
    auto overrideMaterialEntity = getEntity(overrideMaterial);

    UpdateScene();

    EXPECT_EQ(override->GetMaterialOverride(), overrideMaterial);

    // Verify that after override is set, submesh.GetMaterial() returns the same value as before override (as override
    // should not affect the submesh's material property value, only the value in ECS)
    auto submeshMaterialAfterOverride = submesh.GetMaterial();
    EXPECT_TRUE(submeshMaterialAfterOverride);
    EXPECT_EQ(submeshMaterialEntityBeforeOverride, getEntity(submeshMaterialAfterOverride));

    auto submeshMaterialEntity = getEntity(submeshMaterialAfterOverride);

    EXPECT_TRUE(CORE_NS::EntityUtil::IsValid(overrideMaterialEntity));
    EXPECT_TRUE(CORE_NS::EntityUtil::IsValid(submeshMaterialEntity));
    EXPECT_NE(overrideMaterialEntity, submeshMaterialEntity);
    EXPECT_EQ(getSubMeshMaterialEntity(mesh, 0), overrideMaterialEntity);

    // Change submesh material while override is set
    auto submeshChangedMaterial = Material(CreateMaterial());
    auto submeshChangedMaterial2 = Material(CreateMaterial());
    ASSERT_TRUE(submeshChangedMaterial);
    ASSERT_TRUE(submeshChangedMaterial2);
    submesh.SetMaterial(submeshChangedMaterial);
    // Verify that submesh returns the material that was set on the previous line
    EXPECT_EQ(submeshChangedMaterial, submesh.GetMaterial());
    submesh.SetMaterial(submeshChangedMaterial2);
    EXPECT_EQ(submeshChangedMaterial2, submesh.GetMaterial());

    EXPECT_EQ(getSubMeshMaterialEntity(mesh, 0), getEntity(overrideMaterial));

    // Switch material override
    override->SetMaterialOverride(overrideMaterial2);
    EXPECT_EQ(getSubMeshMaterialEntity(mesh, 0), getEntity(overrideMaterial2));

    EXPECT_EQ(override->GetMaterialOverride(), overrideMaterial2);

    EXPECT_EQ(submeshChangedMaterial2, submesh.GetMaterial());
    submesh.SetMaterial(submeshChangedMaterial);
    EXPECT_EQ(submeshChangedMaterial, submesh.GetMaterial());

    // Reset override
    override->SetMaterialOverride(nullptr);

    UpdateScene();

    EXPECT_EQ(override->GetMaterialOverride(), nullptr);
    auto submeshMaterial = submesh.GetMaterial();
    EXPECT_TRUE(submeshMaterial);
    EXPECT_EQ(submeshChangedMaterial, submeshMaterial);

    EXPECT_EQ(getSubMeshMaterialEntity(mesh, 0), getEntity(submeshChangedMaterial));
}

} // namespace UTest

SCENE_END_NAMESPACE()
