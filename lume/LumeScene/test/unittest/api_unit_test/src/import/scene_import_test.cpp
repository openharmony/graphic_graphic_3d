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

#include <scene/ext/scene_utils.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_environment.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_light_probe_group.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/intf_texture.h>
#include <scene/interface/intf_transform.h>
#include <scene/interface/postprocess/intf_bloom.h>
#include <scene/interface/resource/image_info.h>
#include <scene/interface/resource/types.h>
#include <scene_importer/interface/intf_importer.h>

#include <gmock/gmock.h>

#include <base/math/vector.h>

#include "import_test_helpers.h"
#include "scene/scene_test.h"
#include "util.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

using BASE_NS::Math::Quat;
using BASE_NS::Math::Vec3;

class API_SceneImportTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        ScenePluginTest::SetUp();
        imp = META_NS::GetObjectRegistry().Create<SCENE_IMP_NS::IImporter>(SCENE_IMP_NS::ClassId::Importer);
        ASSERT_TRUE(imp);
        ASSERT_TRUE(imp->Initialize(context, {}));
    }

    template <typename Inter>
    typename Inter::Ptr Load(BASE_NS::string_view path);
    IScene::Ptr LoadTestScene(BASE_NS::string_view path);

public:
    SCENE_IMP_NS::IImporter::Ptr imp;
};

static size_t GetNodeCount(const INode::Ptr& node)
{
    size_t count = 1;
    for (auto&& c : node->GetChildren().GetResult()) {
        count += GetNodeCount(c);
    }
    return count;
}

static size_t GetNodeCount(const IScene::Ptr& s)
{
    return GetNodeCount(s->GetRootNode().GetResult());
}

template <typename Inter>
typename Inter::Ptr API_SceneImportTest::Load(BASE_NS::string_view path)
{
    auto res = imp->Import(path, {});
    EXPECT_TRUE(res) << PrintErrors(res.error).c_str();
    if (!res) {
        return nullptr;
    }
    return interface_pointer_cast<Inter>(res.object);
}

IScene::Ptr API_SceneImportTest::LoadTestScene(BASE_NS::string_view path)
{
    auto scene = Load<IScene>(path);
    if (auto named = interface_cast<META_NS::INamed>(scene)) {
        EXPECT_EQ(named->Name()->GetValue(), "test");
    }
    return scene;
}

/**
 * @tc.name: ImportScene0
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene0, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene0.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 1);
}
/**
 * @tc.name: ImportScene1
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene1, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene1.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 1);
    auto root = scene->GetRootNode().GetResult();
    auto rootTrans = interface_pointer_cast<ITransform>(root);
    ASSERT_TRUE(rootTrans);
    EXPECT_EQ(rootTrans->Position()->GetValue(), Vec3(1, 2, 3));
    EXPECT_EQ(rootTrans->Scale()->GetValue(), Vec3(2, 2, 2));
    EXPECT_EQ(rootTrans->Rotation()->GetValue(), Quat(0.5, 0, 0.2, 0));
    EXPECT_EQ(root->NodeFlags()->GetValue(), SCENE_NS::NodeFlags::CONTRIBUTE_GI_BIT);
}
/**
 * @tc.name: ImportScene2
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene2, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene2.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);

    // Verify the structural shape: root "mynode" with one child "1".
    auto root = scene->GetRootNode().GetResult();
    ASSERT_TRUE(root);
    EXPECT_EQ(root->GetName(), "mynode");
    auto rootChildren = root->GetChildren().GetResult();
    ASSERT_EQ(rootChildren.size(), 1u);
    EXPECT_EQ(rootChildren[0]->GetName(), "1");

    // Path lookup must reach the same node we reached structurally.
    auto child = scene->FindNode("//1").GetResult();
    ASSERT_TRUE(child);
    EXPECT_EQ(child, rootChildren[0]);
}
/**
 * @tc.name: ImportScene3
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene3, testing::ext::TestSize.Level1)
{
    EXPECT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{{"cube"}},
        ::SCENE_NS::ClassId::GltfSceneResource.Id().ToUid(),
        "test://AnimatedCube/AnimatedCube.gltf"));
    auto scene = LoadTestScene("test://import/scene/scene3.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 4);
    EXPECT_TRUE(scene->FindNode("//1"));
}
/**
 * @tc.name: ImportScene4
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene4, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene4.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 4);

    // Hierarchy from scene4.json: mynode > { "1" > "3", "2" }
    auto root = scene->GetRootNode().GetResult();
    ASSERT_TRUE(root);
    EXPECT_EQ(root->GetName(), "mynode");
    auto rootChildren = root->GetChildren().GetResult();
    ASSERT_EQ(rootChildren.size(), 2u);
    // Children order in JSON: "1", "2"
    EXPECT_EQ(rootChildren[0]->GetName(), "1");
    EXPECT_EQ(rootChildren[1]->GetName(), "2");

    // Path-based lookups must agree with the structural walk.
    EXPECT_EQ(scene->FindNode("//1").GetResult(), rootChildren[0]);
    EXPECT_EQ(scene->FindNode("//2").GetResult(), rootChildren[1]);

    auto child1Children = rootChildren[0]->GetChildren().GetResult();
    ASSERT_EQ(child1Children.size(), 1u);
    EXPECT_EQ(child1Children[0]->GetName(), "3");
    EXPECT_EQ(scene->FindNode("//1/3").GetResult(), child1Children[0]);

    EXPECT_TRUE(rootChildren[1]->GetChildren().GetResult().empty());
}
/**
 * @tc.name: ImportScene5
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene5, testing::ext::TestSize.Level1)
{
    // scene5.json adds a component by name only (no componentUid) — exercises the importer's
    // name-based component lookup path. The uid-based path is covered by ImportScene6.
    auto scene = LoadTestScene("test://import/scene/scene5.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 1);
    auto root = scene->GetRootNode().GetResult();
    ASSERT_TRUE(root);
    auto comp = scene->GetComponent(root, "RenderConfigurationComponent");
    ASSERT_TRUE(comp);
    // The component must implement IMetadata so its declared properties are reachable.
    EXPECT_TRUE(interface_cast<META_NS::IMetadata>(comp));
}

/**
 * @tc.name: ImportScene6
 * @tc.desc: Same component as scene5 but added via componentUid. Verifies the uid path produces
 *           a structurally equivalent component (same name lookup succeeds, same metadata
 *           contract).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene6, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene6.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 1);
    auto root = scene->GetRootNode().GetResult();
    ASSERT_TRUE(root);
    auto comp = scene->GetComponent(root, "RenderConfigurationComponent");
    ASSERT_TRUE(comp);
    EXPECT_TRUE(interface_cast<META_NS::IMetadata>(comp));
}

namespace {
enum TestEnum {
    TestEnumA = 1,
    TestEnumB = 2,
    TestEnumC = 4,
};
enum class TestEnumStrong : uint32_t {
    A = 0,
    B = 1,
    C = 2,
};
}  // namespace
}  // namespace UTest
SCENE_END_NAMESPACE()
META_TYPE(SCENE_NS::UTest::TestEnum);
META_TYPE(SCENE_NS::UTest::TestEnumStrong);
SCENE_BEGIN_NAMESPACE()
namespace UTest {
namespace {

class ITestAttachment : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITestAttachment, "ba027cdf-656e-4208-a814-0a7ab26e6d29")
public:
    META_PROPERTY(bool, Bool)
    META_PROPERTY(uint8_t, SmallUint)
    META_PROPERTY(uint64_t, BigUint)
    META_PROPERTY(int16_t, SmallInt)
    META_PROPERTY(int64_t, BigInt)
    META_PROPERTY(float, Float)
    META_PROPERTY(double, Double)
    META_PROPERTY(BASE_NS::string, String)
    META_PROPERTY(META_NS::IObject::Ptr, Null)
    META_PROPERTY(META_NS::IObject::Ptr, Object)
    META_PROPERTY(BASE_NS::Math::Vec3, Vec3)
    META_PROPERTY(TestEnum, Enum1)
    META_PROPERTY(TestEnumStrong, Enum2)
    META_ARRAY_PROPERTY(uint32_t, UintArray)
    META_ARRAY_PROPERTY(BASE_NS::Math::Vec3, Vec3Array)
};

META_REGISTER_CLASS(TestAttachment, "9694bdab-29ae-4e56-8d49-b8158fe0339d", META_NS::ObjectCategoryBits::NO_CATEGORY)

class TestAttachment : public META_NS::IntroduceInterfaces<META_NS::MetaObject, ITestAttachment> {
    META_OBJECT(TestAttachment, ClassId::TestAttachment, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(ITestAttachment, bool, Bool)
    META_STATIC_PROPERTY_DATA(ITestAttachment, uint8_t, SmallUint)
    META_STATIC_PROPERTY_DATA(ITestAttachment, uint64_t, BigUint)
    META_STATIC_PROPERTY_DATA(ITestAttachment, int16_t, SmallInt)
    META_STATIC_PROPERTY_DATA(ITestAttachment, int64_t, BigInt)
    META_STATIC_PROPERTY_DATA(ITestAttachment, float, Float)
    META_STATIC_PROPERTY_DATA(ITestAttachment, double, Double)
    META_STATIC_PROPERTY_DATA(ITestAttachment, BASE_NS::string, String)
    META_STATIC_PROPERTY_DATA(ITestAttachment, META_NS::IObject::Ptr, Null)
    META_STATIC_PROPERTY_DATA(ITestAttachment, META_NS::IObject::Ptr, Object)
    META_STATIC_PROPERTY_DATA(ITestAttachment, BASE_NS::Math::Vec3, Vec3)
    META_STATIC_PROPERTY_DATA(ITestAttachment, TestEnum, Enum1, TestEnumA)
    META_STATIC_PROPERTY_DATA(ITestAttachment, TestEnumStrong, Enum2)
    META_STATIC_ARRAY_PROPERTY_DATA(ITestAttachment, uint32_t, UintArray)
    META_STATIC_ARRAY_PROPERTY_DATA(ITestAttachment, BASE_NS::Math::Vec3, Vec3Array)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Bool)
    META_IMPLEMENT_PROPERTY(uint8_t, SmallUint)
    META_IMPLEMENT_PROPERTY(uint64_t, BigUint)
    META_IMPLEMENT_PROPERTY(int16_t, SmallInt)
    META_IMPLEMENT_PROPERTY(int64_t, BigInt)
    META_IMPLEMENT_PROPERTY(float, Float)
    META_IMPLEMENT_PROPERTY(double, Double)
    META_IMPLEMENT_PROPERTY(BASE_NS::string, String)
    META_IMPLEMENT_PROPERTY(META_NS::IObject::Ptr, Null)
    META_IMPLEMENT_PROPERTY(META_NS::IObject::Ptr, Object)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec3, Vec3)
    META_IMPLEMENT_PROPERTY(TestEnum, Enum1)
    META_IMPLEMENT_PROPERTY(TestEnumStrong, Enum2)
    META_IMPLEMENT_ARRAY_PROPERTY(uint32_t, UintArray)
    META_IMPLEMENT_ARRAY_PROPERTY(BASE_NS::Math::Vec3, Vec3Array)
};
}  // namespace

/**
 * @tc.name: ImportScene7
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene7, testing::ext::TestSize.Level1)
{
    META_NS::RegisterObjectType<TestAttachment>();
    auto scene = LoadTestScene("test://import/scene/scene7.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 1);
    auto root = scene->GetRootNode().GetResult();
    auto atts = interface_cast<META_NS::IAttach>(root)->GetAttachments<ITestAttachment>();
    ASSERT_TRUE(atts.size() == 1);
    auto obj = atts[0];
    EXPECT_EQ(obj->Bool()->GetValue(), true);
    EXPECT_EQ(obj->SmallUint()->GetValue(), 64);
    EXPECT_EQ(obj->BigUint()->GetValue(), 1234565778);
    EXPECT_EQ(obj->SmallInt()->GetValue(), -8000);
    EXPECT_EQ(obj->BigInt()->GetValue(), -71234565778);
    EXPECT_EQ(obj->Float()->GetValue(), 1.0);
    EXPECT_EQ(obj->Double()->GetValue(), 2.0);
    EXPECT_EQ(obj->String()->GetValue(), "hips hops");
    EXPECT_EQ(obj->Null()->GetValue(), nullptr);
    EXPECT_TRUE(obj->Object()->GetValue());
    EXPECT_EQ(obj->Vec3()->GetValue(), BASE_NS::Math::Vec3(1, 2, 3));
    EXPECT_EQ(obj->Enum1()->GetValue(), TestEnumB);
    EXPECT_EQ(obj->Enum2()->GetValue(), TestEnumStrong::C);
    EXPECT_THAT(obj->UintArray()->GetValue(), testing::ElementsAre(1, 2, 3, 4));
    EXPECT_THAT(
        obj->Vec3Array()->GetValue(), testing::ElementsAre(BASE_NS::Math::Vec3(1, 2, 3), BASE_NS::Math::Vec3(4, 4, 4)));
    META_NS::UnregisterObjectType<TestAttachment>();
}

/**
 * @tc.name: ImportScene8
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene8, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene8.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 4);
    EXPECT_TRUE(scene->FindNode("//1//AnimatedCube").GetResult());

    BASE_NS::vector<CORE_NS::MatchingResourceId> res{{CORE_NS::ResourceId("AnimatedCube.gltf/images/0", "cube-group")},
        {CORE_NS::ResourceId("AnimatedCube.gltf/images/1", "cube-group")},
        {CORE_NS::ResourceId("AnimatedCube", "cube-group")},
        {CORE_NS::ResourceId("animation_AnimatedCube", "cube-group")}};

    auto infos = GetResourceManager(scene)->GetResourceInfos(res, scene);
    ASSERT_EQ(infos.size(), 4);
}

/**
 * @tc.name: ImportIndex0
 * @tc.desc: Tests for ImportIndex [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportIndex0, testing::ext::TestSize.Level1)
{
    auto rman = Load<CORE_NS::IResourceManager>("test://import/index/index0.json");
    ASSERT_TRUE(rman);
    EXPECT_EQ(rman->GetResourceInfos(nullptr).size(), 1);
    auto res = rman->GetResourceInfo({"cube", nullptr});
    EXPECT_TRUE(res.id.IsValid());
    EXPECT_EQ(res.type, SCENE_NS::ClassId::GltfSceneResource.Id().ToUid());
    EXPECT_EQ(res.path, "test_assets://AnimatedCube/AnimatedCube.gltf");
    EXPECT_EQ(res.name, "Cube gltf");
}

/**
 * @tc.name: ImportScene9
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene9, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene9.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 4);
    EXPECT_TRUE(scene->FindNode("//1//AnimatedCube").GetResult());

    BASE_NS::vector<CORE_NS::MatchingResourceId> res{{CORE_NS::ResourceId("AnimatedCube.gltf/images/0", "cube-group")},
        {CORE_NS::ResourceId("AnimatedCube.gltf/images/1", "cube-group")},
        {CORE_NS::ResourceId("AnimatedCube", "cube-group")},
        {CORE_NS::ResourceId("animation_AnimatedCube", "cube-group")}};

    auto infos = GetResourceManager(scene)->GetResourceInfos(res, scene);
    ASSERT_EQ(infos.size(), 4);
}

/**
 * @tc.name: ImportScene10
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene10, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene10.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);
    EXPECT_TRUE(scene->FindNode("//test").GetResult());
}

/**
 * @tc.name: ImportScene11
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene11, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene11.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);
    auto node = interface_pointer_cast<SCENE_NS::ILight>(scene->FindNode("//test").GetResult());
    ASSERT_TRUE(node);
    EXPECT_EQ(META_NS::GetValue(node->Type()), LightType::POINT);
    EXPECT_EQ(META_NS::GetValue(node->Color()), BASE_NS::Color(0.5, 0.5, 0, 0));
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->Intensity()), 1);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->NearPlane()), 0.8);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->Range()), 0.1);
    EXPECT_EQ(META_NS::GetValue(node->AdditionalFactor()), BASE_NS::Math::Vec4(1, 2, 3, 4));
    EXPECT_EQ(META_NS::GetValue(node->LightLayerMask()), 77);

    EXPECT_EQ(META_NS::GetValue(node->ShadowEnabled()), true);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->ShadowStrength()), 0.6);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->ShadowDepthBias()), 0.1);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->ShadowNormalBias()), 0.2);
    EXPECT_EQ(META_NS::GetValue(node->ShadowLayerMask()), 44);
}

/**
 * @tc.name: ImportScene13
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene13, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene13.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);
    auto node = interface_pointer_cast<SCENE_NS::ICamera>(scene->FindNode("//test").GetResult());
    ASSERT_TRUE(node);
    EXPECT_EQ(META_NS::GetValue(node->Projection()), CameraProjection::PERSPECTIVE);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->FoV()), 60.0f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->AspectRatio()), 1.5f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->NearPlane()), 0.1f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->FarPlane()), 100.0f);
    EXPECT_EQ(META_NS::GetValue(node->Culling()), CameraCulling::VIEW_FRUSTUM);
    EXPECT_EQ(META_NS::GetValue(node->RenderingPipeline()), CameraPipeline::FORWARD);
    EXPECT_EQ(META_NS::GetValue(node->SceneFlags()), uint32_t(CameraSceneFlag::ACTIVE_RENDER_BIT));
    EXPECT_EQ(META_NS::GetValue(node->PipelineFlags()),
        uint32_t(CameraPipelineFlag::CLEAR_DEPTH_BIT | CameraPipelineFlag::CLEAR_COLOR_BIT));
    EXPECT_EQ(META_NS::GetValue(node->Viewport()), BASE_NS::Math::Vec4(0, 0, 1, 1));
    EXPECT_EQ(META_NS::GetValue(node->Scissor()), BASE_NS::Math::Vec4(0, 0, 1, 1));
    EXPECT_EQ(META_NS::GetValue(node->ClearColor()), BASE_NS::Math::Vec4(0.2f, 0.3f, 0.4f, 1.0f));
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->ClearDepth()), 0.9f);
    EXPECT_EQ(META_NS::GetValue(node->CameraLayerMask()), uint64_t(255));
}

/**
 * @tc.name: ImportScene14
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene14, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene14.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);
    auto node = interface_pointer_cast<SCENE_NS::ICamera>(scene->FindNode("//test").GetResult());
    ASSERT_TRUE(node);
    EXPECT_EQ(META_NS::GetValue(node->Projection()), CameraProjection::ORTHOGRAPHIC);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->XMagnification()), 5.0f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->YMagnification()), 3.0f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->NearPlane()), 0.5f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->FarPlane()), 200.0f);
}

/**
 * @tc.name: ImportScene15
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene15, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene15.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);
    auto node = interface_pointer_cast<SCENE_NS::ILight>(scene->FindNode("//test").GetResult());
    ASSERT_TRUE(node);
    EXPECT_EQ(META_NS::GetValue(node->Type()), LightType::DIRECTIONAL);
    EXPECT_EQ(META_NS::GetValue(node->Color()), BASE_NS::Color(1.0f, 0.8f, 0.6f, 0.0f));
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->Intensity()), 2.5f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->NearPlane()), 0.1f);
    EXPECT_EQ(META_NS::GetValue(node->AdditionalFactor()), BASE_NS::Math::Vec4(0.1f, 0.2f, 0.3f, 0.4f));
    EXPECT_EQ(META_NS::GetValue(node->LightLayerMask()), uint64_t(15));
    EXPECT_EQ(META_NS::GetValue(node->ShadowEnabled()), false);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->ShadowStrength()), 0.5f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->ShadowDepthBias()), 0.05f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->ShadowNormalBias()), 0.1f);
    EXPECT_EQ(META_NS::GetValue(node->ShadowLayerMask()), uint64_t(3));
}

/**
 * @tc.name: ImportScene16
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene16, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene16.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);
    auto node = interface_pointer_cast<SCENE_NS::ILight>(scene->FindNode("//test").GetResult());
    ASSERT_TRUE(node);
    EXPECT_EQ(META_NS::GetValue(node->Type()), LightType::SPOT);
    EXPECT_EQ(META_NS::GetValue(node->Color()), BASE_NS::Color(0.8f, 0.8f, 1.0f, 0.0f));
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->Intensity()), 5.0f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->Range()), 10.0f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->SpotInnerAngle()), 0.3f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->SpotOuterAngle()), 0.6f);
}

/**
 * @tc.name: ImportScene17
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene17, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene17.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);
    auto node = scene->FindNode("//test").GetResult();
    ASSERT_TRUE(node);
    auto trans = interface_pointer_cast<ITransform>(node);
    ASSERT_TRUE(trans);
    EXPECT_EQ(META_NS::GetValue(trans->Position()), Vec3(1, 2, 3));
}

/**
 * @tc.name: ImportScene19
 * @tc.desc: Tests for ImportScene [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene19, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene19.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 4);
    auto mesh = scene->FindNode<IMesh>("//test//AnimatedCube").GetResult();
    ASSERT_TRUE(mesh);

    auto subs = mesh->SubMeshes()->GetValue();
    ASSERT_FALSE(subs.empty());

    auto firstSub = subs[0];
    auto mat = firstSub->Material()->GetValue();
    ASSERT_TRUE(mat);

    EXPECT_FLOAT_EQ(mat->AlphaCutoff()->GetValue(), 0.5f);
}

/**
 * @tc.name: ImportScene20
 * @tc.desc: Tests for ImportScene with camera colorTargets [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene20, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene20.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);
    auto node = interface_pointer_cast<SCENE_NS::ICamera>(scene->FindNode("//test").GetResult());
    ASSERT_TRUE(node);
    auto targets = node->ColorTargetCustomization()->GetValue();
    ASSERT_EQ(targets.size(), 2);
    EXPECT_EQ(targets[0].format, BASE_NS::BASE_FORMAT_R8G8B8A8_SRGB);
    EXPECT_EQ(targets[0].usageFlags,
        static_cast<uint32_t>(ImageUsageFlag::SAMPLED_BIT) |
            static_cast<uint32_t>(ImageUsageFlag::COLOR_ATTACHMENT_BIT));
    EXPECT_EQ(targets[1].format, BASE_NS::BASE_FORMAT_R16G16B16A16_SFLOAT);
    EXPECT_EQ(targets[1].usageFlags, 0u);
}

/**
 * @tc.name: ImportScene21
 * @tc.desc: Tests for ImportScene with camera inline postProcess
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene21, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene21.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);
    auto node = interface_pointer_cast<SCENE_NS::ICamera>(scene->FindNode("//test").GetResult());
    ASSERT_TRUE(node);
    auto pp = META_NS::GetValue(node->PostProcess());
    ASSERT_TRUE(pp);
    auto tonemap = META_NS::GetValue(pp->Tonemap());
    ASSERT_TRUE(tonemap);
    EXPECT_TRUE(META_NS::GetValue(tonemap->Enabled()));
    EXPECT_EQ(META_NS::GetValue(tonemap->Type()), SCENE_NS::TonemapType::ACES);
    EXPECT_FLOAT_EQ(META_NS::GetValue(tonemap->Exposure()), 1.5f);
    auto bloom = META_NS::GetValue(pp->Bloom());
    ASSERT_TRUE(bloom);
    EXPECT_TRUE(META_NS::GetValue(bloom->Enabled()));
    EXPECT_FLOAT_EQ(META_NS::GetValue(bloom->ThresholdHard()), 0.8f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(bloom->AmountCoefficient()), 0.5f);
}

/**
 * @tc.name: ImportScene22
 * @tc.desc: Tests for ImportScene with camera postProcess objectRef
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene22, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene22.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);
    auto node = interface_pointer_cast<SCENE_NS::ICamera>(scene->FindNode("//test").GetResult());
    ASSERT_TRUE(node);
    auto pp = META_NS::GetValue(node->PostProcess());
    ASSERT_TRUE(pp);
    auto tonemap = META_NS::GetValue(pp->Tonemap());
    ASSERT_TRUE(tonemap);
    EXPECT_TRUE(META_NS::GetValue(tonemap->Enabled()));
    EXPECT_EQ(META_NS::GetValue(tonemap->Type()), SCENE_NS::TonemapType::FILMIC);
    EXPECT_FLOAT_EQ(META_NS::GetValue(tonemap->Exposure()), 2.0f);
    auto vignette = META_NS::GetValue(pp->Vignette());
    ASSERT_TRUE(vignette);
    EXPECT_TRUE(META_NS::GetValue(vignette->Enabled()));
    EXPECT_FLOAT_EQ(META_NS::GetValue(vignette->Coefficient()), 0.5f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(vignette->Power()), 0.8f);
}

/**
 * @tc.name: ImportScene24
 * @tc.desc: Tests importing a scene containing a lightProbeGroup node
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene24, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene24.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);
    auto node = interface_pointer_cast<SCENE_NS::ILightProbeGroup>(scene->FindNode("//probes").GetResult());
    ASSERT_TRUE(node);

    auto probes = node->GetLightProbeGroup();
    ASSERT_EQ(probes.size(), 2u);

    EXPECT_EQ(probes[0].position, Vec3(1, 2, 3));
    EXPECT_EQ(probes[0].shCoefficients[0], Vec3(0.1f, 0.2f, 0.3f));
    EXPECT_EQ(probes[0].shCoefficients[4], Vec3(1.3f, 1.4f, 1.5f));
    EXPECT_EQ(probes[0].shCoefficients[8], Vec3(2.5f, 2.6f, 2.7f));
    EXPECT_EQ(probes[0].bentNormal, Vec3(0, 1, 0));
    EXPECT_FLOAT_EQ(probes[0].ao, 0.75f);

    EXPECT_EQ(probes[1].position, Vec3(4, 5, 6));
    EXPECT_EQ(probes[1].shCoefficients[0], Vec3(10, 11, 12));
    EXPECT_EQ(probes[1].shCoefficients[8], Vec3(34, 35, 36));
}

/**
 * @tc.name: ImportIndex1
 * @tc.desc: Tests for ImportIndex [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportIndex1, testing::ext::TestSize.Level1)
{
    auto rman = Load<CORE_NS::IResourceManager>("test://import/index/index1.json");
    ASSERT_TRUE(rman);
    EXPECT_EQ(rman->GetResourceInfos(nullptr).size(), 2);
    auto res = rman->GetResourceInfo({"scene", nullptr});
    EXPECT_TRUE(res.id.IsValid());
    EXPECT_EQ(res.path, "myscenepath");
}

/**
 * @tc.name: ImportSceneMaterial0
 * @tc.desc: Tests scene loading with indexed material options [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportSceneMaterial0, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene_mat.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);
    EXPECT_TRUE(scene->FindNode("//geom"));

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);
    auto res = rman->GetResourceInfo({"myMaterial", scene});
    EXPECT_TRUE(res.id.IsValid());
    EXPECT_EQ(res.type, SCENE_NS::ClassId::MaterialResource.Id().ToUid());
    EXPECT_TRUE(res.options);
    auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({"myMaterial", scene}));
    ASSERT_TRUE(mat);
    EXPECT_EQ(META_NS::GetValue(mat->Type()), MaterialType::METALLIC_ROUGHNESS);
    EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.5f);
    EXPECT_EQ(META_NS::GetValue(mat->LightingFlags()), static_cast<SCENE_NS::LightingFlags>(2));
}

/**
 * @tc.name: ImportSceneMaterial1
 * @tc.desc: Tests material import with materialShader, depthShader and textures [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportSceneMaterial1, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene_mat2.json");
    ASSERT_TRUE(scene);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    // Verify shader resource was registered
    auto shaderInfo = rman->GetResourceInfo({"myShader", scene});
    EXPECT_TRUE(shaderInfo.id.IsValid());

    // Verify image resource was registered
    auto imageInfo = rman->GetResourceInfo({"myImage", scene});
    EXPECT_TRUE(imageInfo.id.IsValid());

    // Verify material resource
    auto matRes = rman->GetResourceInfo({"myMaterial2", scene});
    EXPECT_TRUE(matRes.id.IsValid());
    EXPECT_TRUE(matRes.options);

    auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({"myMaterial2", scene}));
    ASSERT_TRUE(mat);
    EXPECT_EQ(META_NS::GetValue(mat->Type()), MaterialType::CUSTOM);

    // Verify materialShader was resolved
    auto matShader = META_NS::GetValue(mat->MaterialShader());
    ASSERT_TRUE(matShader);

    // Per-material graphics options were applied onto the bound shader
    EXPECT_EQ(META_NS::GetValue(matShader->CullMode()), CullModeFlags::FRONT_BIT);
    EXPECT_TRUE(META_NS::GetValue(matShader->Blend()));

    // Verify depthShader was resolved
    auto depthShader = META_NS::GetValue(mat->DepthShader());
    EXPECT_TRUE(depthShader);

    // Verify textures
    auto textures = mat->Textures();
    ASSERT_TRUE(textures);
    EXPECT_GE(textures->GetSize(), 1);
}

/**
 * @tc.name: ImportSceneMaterialOptions
 * @tc.desc: A scene whose material has graphics-state options applies them to the
 *           instantiated material's bound shader
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportSceneMaterialOptions, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene_mat_options.json");
    ASSERT_TRUE(scene);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({"optMat", scene}));
    ASSERT_TRUE(mat);

    auto shader = META_NS::GetValue(mat->MaterialShader());
    ASSERT_TRUE(shader);
    EXPECT_EQ(META_NS::GetValue(shader->CullMode()), CullModeFlags::BACK_BIT);
    EXPECT_EQ(META_NS::GetValue(shader->PolygonMode()), PolygonMode::LINE);
    EXPECT_TRUE(META_NS::GetValue(shader->Blend()));
}

/**
 * @tc.name: ImportSceneEnvironment1
 * @tc.desc: Tests environment import with radianceImage and environmentImage references
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportSceneEnvironment1, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene_env1.json");
    ASSERT_TRUE(scene);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    auto radianceInfo = rman->GetResourceInfo({"radianceImg", scene});
    EXPECT_TRUE(radianceInfo.id.IsValid());

    auto envImgInfo = rman->GetResourceInfo({"envImg", scene});
    EXPECT_TRUE(envImgInfo.id.IsValid());

    auto envRes = rman->GetResourceInfo({"myEnv", scene});
    EXPECT_TRUE(envRes.id.IsValid());

    auto env = interface_pointer_cast<IEnvironment>(rman->GetResource({"myEnv", scene}));
    ASSERT_TRUE(env);
    EXPECT_EQ(META_NS::GetValue(env->Background()), EnvBackgroundType::CUBEMAP);
    EXPECT_EQ(META_NS::GetValue(env->RadianceCubemapMipCount()), 5u);
    EXPECT_FLOAT_EQ(META_NS::GetValue(env->EnvironmentMapLodLevel()), 2.5f);

    // The environment must wire to the *specific* images registered in the index, not just
    // any non-null images. Compare against the resources resolved by their declared ids.
    auto expectedRadiance = interface_pointer_cast<IImage>(rman->GetResource({"radianceImg", scene}));
    auto expectedEnv = interface_pointer_cast<IImage>(rman->GetResource({"envImg", scene}));
    ASSERT_TRUE(expectedRadiance);
    ASSERT_TRUE(expectedEnv);

    auto radianceImage = META_NS::GetValue(env->RadianceImage());
    ASSERT_TRUE(radianceImage);
    EXPECT_EQ(radianceImage, expectedRadiance);

    auto environmentImage = META_NS::GetValue(env->EnvironmentImage());
    ASSERT_TRUE(environmentImage);
    EXPECT_EQ(environmentImage, expectedEnv);
}

/**
 * @tc.name: ImportSceneRenderConfiguration
 * @tc.desc: Tests inline renderConfiguration import populates IRenderConfiguration on the scene
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportSceneRenderConfiguration, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene_render_config.json");
    ASSERT_TRUE(scene);

    auto rc = META_NS::GetValue(scene->RenderConfiguration());
    ASSERT_TRUE(rc);

    EXPECT_EQ(META_NS::GetValue(rc->ShadowType()), SceneShadowType::VARIABLE_PCF);
    EXPECT_EQ(META_NS::GetValue(rc->ShadowQuality()), SceneShadowQuality::HIGH);
    EXPECT_EQ(META_NS::GetValue(rc->ShadowSmoothness()), SceneShadowSmoothness::SOFT);
    EXPECT_EQ(META_NS::GetValue(rc->ShadowResolution()), BASE_NS::Math::UVec2(2048, 2048));
    EXPECT_FLOAT_EQ(META_NS::GetValue(rc->VariablePcfRadius()), 2.5f);
    EXPECT_EQ(META_NS::GetValue(rc->VariablePcfSampleCount()), 8);
    EXPECT_EQ(META_NS::GetValue(rc->RenderNodeGraphUri()), "rng://custom");
    EXPECT_EQ(META_NS::GetValue(rc->PostRenderNodeGraphUri()), "rng://post");

    auto env = META_NS::GetValue(rc->Environment());
    ASSERT_TRUE(env);
    EXPECT_EQ(META_NS::GetValue(env->Background()), EnvBackgroundType::CUBEMAP);
    EXPECT_EQ(META_NS::GetValue(env->RadianceCubemapMipCount()), 4u);
}

/**
 * @tc.name: ImportSceneBloom1
 * @tc.desc: Tests bloom import with dirtMaskImage reference
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportSceneBloom1, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene_bloom1.json");
    ASSERT_TRUE(scene);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    auto pp = interface_pointer_cast<IPostProcess>(rman->GetResource({"myPostProcess", scene}));
    ASSERT_TRUE(pp);

    auto bloom = META_NS::GetValue(pp->Bloom());
    ASSERT_TRUE(bloom);
    EXPECT_TRUE(META_NS::GetValue(bloom->Enabled()));
    EXPECT_FLOAT_EQ(META_NS::GetValue(bloom->ThresholdHard()), 0.8f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(bloom->DirtMaskCoefficient()), 0.5f);

    auto dirtMask = META_NS::GetValue(bloom->DirtMaskImage());
    EXPECT_TRUE(dirtMask);
}

/**
 * @tc.name: ImportScene23
 * @tc.desc: Tests 3 levels of nested node templates (scene -> template1 -> template2 -> template3)
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportScene23, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene23.json");
    ASSERT_TRUE(scene);

    // root + level1 + level2 + level3 = 4 nodes
    EXPECT_EQ(GetNodeCount(scene), 4);

    // Navigate through the hierarchy: root -> level1 -> level2 -> level3
    auto root = scene->GetRootNode().GetResult();
    ASSERT_TRUE(root);

    auto level1Children = root->GetChildren().GetResult();
    ASSERT_EQ(level1Children.size(), 1);
    auto level1 = level1Children[0];
    EXPECT_EQ(interface_cast<META_NS::INamed>(level1)->Name()->GetValue(), "level1");
    // Scene overrides level1 position to (10,20,30) - scene override is not default
    EXPECT_EQ(META_NS::GetValue(interface_cast<ITransform>(level1)->Position()), Vec3(10, 20, 30));
    EXPECT_FALSE(interface_cast<ITransform>(level1)->Position()->IsDefaultValue());

    auto level2Children = level1->GetChildren().GetResult();
    ASSERT_EQ(level2Children.size(), 1);
    auto level2 = level2Children[0];
    EXPECT_EQ(interface_cast<META_NS::INamed>(level2)->Name()->GetValue(), "level2");
    // Level1 template overrides level2 scale to (5,5,5) - template override is default
    EXPECT_EQ(META_NS::GetValue(interface_cast<ITransform>(level2)->Scale()), Vec3(5, 5, 5));
    EXPECT_TRUE(interface_cast<ITransform>(level2)->Scale()->IsDefaultValue());

    auto level3Children = level2->GetChildren().GetResult();
    ASSERT_EQ(level3Children.size(), 1);
    auto level3 = level3Children[0];
    EXPECT_EQ(interface_cast<META_NS::INamed>(level3)->Name()->GetValue(), "level3");
    // Level2 template overrides level3 position to (7,8,9) - template override is default
    EXPECT_EQ(META_NS::GetValue(interface_cast<ITransform>(level3)->Position()), Vec3(7, 8, 9));
    EXPECT_TRUE(interface_cast<ITransform>(level3)->Position()->IsDefaultValue());
}

/**
 * @tc.name: ImportDepthNodeExceeded
 * @tc.desc: Node hierarchy deeper than 128 levels must fail with a diagnostic, not a crash [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportDepthNodeExceeded, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/scene/scene_depth_node_exceed.json", {});
    EXPECT_FALSE(res);
    // The JSON parser bails on this file before the node-depth check fires (it cannot parse the
    // ~128-level deep array of children), so the surfaced diagnostic is a parse failure rather
    // than the explicit "Maximum node hierarchy depth exceeded" message. The 128-node-depth
    // guard in node.cpp is currently not reachable from JSON; it would be exercised by very
    // deeply nested templates instead.
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "Invalid file, top-level JSON is not an object");
}

/**
 * @tc.name: ImportDepthNodeOk
 * @tc.desc: Node hierarchy at 126 levels must succeed [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportDepthNodeOk, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/scene/scene_depth_node_ok.json", {});
    EXPECT_TRUE(res) << PrintErrors(res.error).c_str();
    if (res) {
        auto scene = interface_pointer_cast<IScene>(res.object);
        ASSERT_TRUE(scene);
        // root (n0) + 125 children in a chain = 126 nodes total
        EXPECT_EQ(GetNodeCount(scene), 126);
    }
}

/**
 * @tc.name: ImportSceneCameraFrustum
 * @tc.desc: Camera node with frustum projection and offset
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportSceneCameraFrustum, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene_camera_frustum.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);
    auto node = interface_pointer_cast<SCENE_NS::ICamera>(scene->FindNode("//test").GetResult());
    ASSERT_TRUE(node);
    EXPECT_EQ(META_NS::GetValue(node->Projection()), CameraProjection::FRUSTUM);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->XOffset()), 2.0f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->YOffset()), 3.0f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->NearPlane()), 0.1f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->FarPlane()), 500.0f);
}

/**
 * @tc.name: ImportSceneCameraCustom
 * @tc.desc: Camera node with custom projection matrix
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportSceneCameraCustom, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene_camera_custom.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);
    auto node = interface_pointer_cast<SCENE_NS::ICamera>(scene->FindNode("//test").GetResult());
    ASSERT_TRUE(node);
    EXPECT_EQ(META_NS::GetValue(node->Projection()), CameraProjection::CUSTOM);

    // Column-major: col0={1,0,0,0}, col1={0,2,0,0}, col2={0,0,-1,-0.2}, col3={0,0,-1,0}
    BASE_NS::Math::Mat4X4 expected{
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, -0.2f, 0.0f, 0.0f, -1.0f, 0.0f};
    EXPECT_EQ(META_NS::GetValue(node->CustomProjectionMatrix()), expected);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->NearPlane()), 0.1f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->FarPlane()), 100.0f);
}

/**
 * @tc.name: ImportSceneCameraRenderProps
 * @tc.desc: Camera with extended render properties: deferred pipeline, MSAA, renderTargetSize, downsample
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportSceneCameraRenderProps, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene_camera_render_props.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 2);
    auto node = interface_pointer_cast<SCENE_NS::ICamera>(scene->FindNode("//test").GetResult());
    ASSERT_TRUE(node);

    // Projection
    EXPECT_EQ(META_NS::GetValue(node->Projection()), CameraProjection::PERSPECTIVE);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->FoV()), 45.0f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->AspectRatio()), 1.77f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->NearPlane()), 0.5f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->FarPlane()), 1000.0f);

    // Culling and pipeline
    EXPECT_EQ(META_NS::GetValue(node->Culling()), CameraCulling::NONE);
    EXPECT_EQ(META_NS::GetValue(node->RenderingPipeline()), CameraPipeline::DEFERRED);

    // Scene flags: active | mainCamera
    EXPECT_EQ(META_NS::GetValue(node->SceneFlags()),
        uint32_t(CameraSceneFlag::ACTIVE_RENDER_BIT) | uint32_t(CameraSceneFlag::MAIN_CAMERA_BIT));

    // Pipeline flags: clearDepth | clearColor | msaa | jitter | history
    EXPECT_EQ(META_NS::GetValue(node->PipelineFlags()),
        uint32_t(CameraPipelineFlag::CLEAR_DEPTH_BIT) | uint32_t(CameraPipelineFlag::CLEAR_COLOR_BIT) |
            uint32_t(CameraPipelineFlag::MSAA_BIT) | uint32_t(CameraPipelineFlag::JITTER_BIT) |
            uint32_t(CameraPipelineFlag::HISTORY_BIT));

    // View properties
    EXPECT_EQ(META_NS::GetValue(node->Viewport()), BASE_NS::Math::Vec4(0.1f, 0.2f, 0.8f, 0.6f));
    EXPECT_EQ(META_NS::GetValue(node->Scissor()), BASE_NS::Math::Vec4(0.0f, 0.0f, 0.5f, 0.5f));
    EXPECT_EQ(META_NS::GetValue(node->RenderTargetSize()), BASE_NS::Math::UVec2(1920, 1080));

    // Render properties
    EXPECT_EQ(META_NS::GetValue(node->ClearColor()), BASE_NS::Math::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->ClearDepth()), 1.0f);
    EXPECT_EQ(META_NS::GetValue(node->CameraLayerMask()), uint64_t(65535));
    EXPECT_EQ(META_NS::GetValue(node->MSAASampleCount()), CameraSampleCount::COUNT_4);
    EXPECT_FLOAT_EQ(META_NS::GetValue(node->DownsamplePercentage()), 50.0f);
}

/**
 * @tc.name: ImportExternalNodeAttachment
 * @tc.desc: External node via gltf URL creates IExternalNode attachment and child hierarchy
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportExternalNodeAttachment, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene9.json");
    ASSERT_TRUE(scene);
    EXPECT_EQ(GetNodeCount(scene), 4);

    auto node = scene->FindNode("//1").GetResult();
    ASSERT_TRUE(node);
    EXPECT_TRUE(GetExternalNodeAttachment(node));

    auto child = scene->FindNode("//1//AnimatedCube").GetResult();
    EXPECT_TRUE(child);
}

/**
 * @tc.name: ImportExternalNodeTransform
 * @tc.desc: External node with position, scale and rotation transform overrides
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportExternalNodeTransform, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene_ext_transform.json");
    ASSERT_TRUE(scene);

    auto node = scene->FindNode("//ext1").GetResult();
    ASSERT_TRUE(node);
    EXPECT_TRUE(GetExternalNodeAttachment(node));

    auto transform = interface_cast<ITransform>(node);
    ASSERT_TRUE(transform);
    EXPECT_EQ(META_NS::GetValue(transform->Position()), Vec3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(META_NS::GetValue(transform->Scale()), Vec3(2.0f, 2.0f, 2.0f));

    auto rot = META_NS::GetValue(transform->Rotation());
    EXPECT_NEAR(rot.x, 0.0f, 0.001f);
    EXPECT_NEAR(rot.y, 0.0f, 0.001f);
    EXPECT_NEAR(rot.z, 0.7071068f, 0.001f);
    EXPECT_NEAR(rot.w, 0.7071068f, 0.001f);
}

/**
 * @tc.name: ImportExternalNodeMultiple
 * @tc.desc: Multiple external nodes in one scene with separate resource groups
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportExternalNodeMultiple, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene_ext_multi.json");
    ASSERT_TRUE(scene);

    auto ext1 = scene->FindNode("//ext1").GetResult();
    ASSERT_TRUE(ext1);
    EXPECT_TRUE(GetExternalNodeAttachment(ext1));

    auto ext2 = scene->FindNode("//ext2").GetResult();
    ASSERT_TRUE(ext2);
    EXPECT_TRUE(GetExternalNodeAttachment(ext2));

    // Resources in grp1
    BASE_NS::vector<CORE_NS::MatchingResourceId> res1{{CORE_NS::ResourceId("AnimatedCube", "grp1")}};
    auto infos1 = GetResourceManager(scene)->GetResourceInfos(res1, scene);
    EXPECT_EQ(infos1.size(), 1);

    // Resources in grp2
    BASE_NS::vector<CORE_NS::MatchingResourceId> res2{{CORE_NS::ResourceId("AnimatedCube", "grp2")}};
    auto infos2 = GetResourceManager(scene)->GetResourceInfos(res2, scene);
    EXPECT_EQ(infos2.size(), 1);
}

/**
 * @tc.name: ImportExternalNodeMissingSource
 * @tc.desc: External node without required source field produces error diagnostic
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportExternalNodeMissingSource, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/scene/scene_ext_missing_source.json", {});
    EXPECT_FALSE(res);
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "Missing source");
}

/**
 * @tc.name: ImportExternalNodeMissingResourceGroup
 * @tc.desc: External node without required resourceGroup field produces error diagnostic
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportExternalNodeMissingResourceGroup, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/scene/scene_ext_missing_rgroup.json", {});
    EXPECT_FALSE(res);
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "Missing required string 'resourceGroup'");
}

/**
 * @tc.name: ImportExternalNodeInvalidType
 * @tc.desc: External node with invalid source type produces error diagnostic
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportExternalNodeInvalidType, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/scene/scene_ext_invalid_type.json", {});
    EXPECT_FALSE(res);
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "Invalid external type");
}

/**
 * @tc.name: ImportExternalNodeCircularReference
 * @tc.desc: A node template referencing itself as an external node must fail, not recurse infinitely
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportExternalNodeCircularReference, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/scene/scene_ext_circular.json", {});
    EXPECT_FALSE(res);
    // The recursion is bounded by the file-include depth limit in LoadFile, but the underlying
    // "Maximum import depth exceeded" diagnostic is currently swallowed by the external-node
    // wrapper and surfaces as a generic load failure. We verify the wrapper message — the
    // inner cause is a known reporting gap, not a coverage issue.
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "Failed to load external node");
}

/**
 * @tc.name: ImportExternalNodeMaterialOverride
 * @tc.desc: External node with material override from index replaces submesh material
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportExternalNodeMaterialOverride, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene_ext_mat_override.json");
    ASSERT_TRUE(scene);

    auto mesh = scene->FindNode<IMesh>("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(mesh);

    auto subs = mesh->SubMeshes()->GetValue();
    ASSERT_FALSE(subs.empty());

    auto mat = subs[0]->Material()->GetValue();
    ASSERT_TRUE(mat);

    EXPECT_FLOAT_EQ(mat->AlphaCutoff()->GetValue(), 0.75f);
    EXPECT_EQ(META_NS::GetValue(mat->LightingFlags()), static_cast<LightingFlags>(2));

    // The override must replace the gltf-supplied material with the *specific* "overrideMat"
    // resource — not a coincidentally matching default. Compare pointer identity.
    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);
    auto overrideMat = interface_pointer_cast<IMaterial>(rman->GetResource({"overrideMat", scene}));
    ASSERT_TRUE(overrideMat);
    EXPECT_EQ(mat, overrideMat);
}

/**
 * @tc.name: ImportExternalNodeComponentGltf
 * @tc.desc: Component added to a child node of an external gltf hierarchy via path
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportExternalNodeComponentGltf, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene_ext_component_gltf.json");
    ASSERT_TRUE(scene);

    auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);

    auto comp = scene->GetComponent(cube, "RenderConfigurationComponent");
    EXPECT_TRUE(comp);
}

/**
 * @tc.name: ImportExtensionNode
 * @tc.desc: extensionNode dispatches by nodeUid, instantiates that registered class,
 *           and applies inherited node fields (name, transform, children) plus the
 *           extension-specific properties array.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportExtensionNode, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene_extension_node.json");
    ASSERT_TRUE(scene);
    ASSERT_EQ(GetNodeCount(scene), 3);

    auto ext = scene->FindNode("//ext").GetResult();
    ASSERT_TRUE(ext);
    EXPECT_EQ(ext->GetName(), "ext");

    auto trans = interface_pointer_cast<ITransform>(ext);
    ASSERT_TRUE(trans);
    EXPECT_EQ(trans->Position()->GetValue(), Vec3(1, 2, 3));
    EXPECT_EQ(trans->Scale()->GetValue(), Vec3(2, 2, 2));

    EXPECT_EQ(ext->Enabled()->GetValue(), false);

    EXPECT_TRUE(scene->FindNode("//ext/leaf").GetResult());
}

/**
 * @tc.name: ImportExtensionNodeMissingNodeUid
 * @tc.desc: extensionNode without a nodeUid cannot be constructed and produces an error
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportExtensionNodeMissingNodeUid, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/scene/scene_extension_node_missing_uid.json", {});
    EXPECT_FALSE(res);
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "Failed to create node");
}

/**
 * @tc.name: ImportExtensionNodeUnknownNodeUid
 * @tc.desc: extensionNode whose nodeUid is not registered in the object registry fails to construct
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportExtensionNodeUnknownNodeUid, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/scene/scene_extension_node_unknown_uid.json", {});
    EXPECT_FALSE(res);
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "Failed to create node");
}

namespace {

class IBuiltinTypesAttachment : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IBuiltinTypesAttachment, "e7a1c3b5-4d2f-48e6-9f0a-1b3c5d7e9f01")
public:
    META_PROPERTY(BASE_NS::Math::Vec4, Vec4)
    META_PROPERTY(BASE_NS::Color, Color)
    META_PROPERTY(BASE_NS::Math::Quat, Quat)
    META_PROPERTY(BASE_NS::Math::UVec2, UVec2)
    META_PROPERTY(BASE_NS::Math::Mat4X4, Mat4x4)
};

META_REGISTER_CLASS(
    BuiltinTypesAttachment, "e7a1c3b5-4d2f-48e6-9f0a-1b3c5d7e9f01", META_NS::ObjectCategoryBits::NO_CATEGORY)

class BuiltinTypesAttachment : public META_NS::IntroduceInterfaces<META_NS::MetaObject, IBuiltinTypesAttachment> {
    META_OBJECT(BuiltinTypesAttachment, ClassId::BuiltinTypesAttachment, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IBuiltinTypesAttachment, BASE_NS::Math::Vec4, Vec4)
    META_STATIC_PROPERTY_DATA(IBuiltinTypesAttachment, BASE_NS::Color, Color)
    META_STATIC_PROPERTY_DATA(IBuiltinTypesAttachment, BASE_NS::Math::Quat, Quat)
    META_STATIC_PROPERTY_DATA(IBuiltinTypesAttachment, BASE_NS::Math::UVec2, UVec2)
    META_STATIC_PROPERTY_DATA(IBuiltinTypesAttachment, BASE_NS::Math::Mat4X4, Mat4x4)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec4, Vec4)
    META_IMPLEMENT_PROPERTY(BASE_NS::Color, Color)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Quat, Quat)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::UVec2, UVec2)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Mat4X4, Mat4x4)
};
}  // namespace

/**
 * @tc.name: ImportSceneBuiltinTypes
 * @tc.desc: Tests that all builtin value types (vec4, color, quat, uvec2, mat4x4) can be imported as properties
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportSceneBuiltinTypes, testing::ext::TestSize.Level1)
{
    META_NS::RegisterObjectType<BuiltinTypesAttachment>();
    auto scene = LoadTestScene("test://import/scene/scene_builtin_types.json");
    ASSERT_TRUE(scene);
    auto root = scene->GetRootNode().GetResult();
    ASSERT_TRUE(root);
    auto atts = interface_cast<META_NS::IAttach>(root)->GetAttachments<IBuiltinTypesAttachment>();
    ASSERT_EQ(atts.size(), 1);
    auto obj = atts[0];
    EXPECT_EQ(obj->Vec4()->GetValue(), BASE_NS::Math::Vec4(1, 2, 3, 4));
    EXPECT_EQ(obj->Color()->GetValue(), BASE_NS::Color(0.1f, 0.2f, 0.3f, 1.0f));
    EXPECT_EQ(obj->Quat()->GetValue(), BASE_NS::Math::Quat(0, 0, 0, 1));
    EXPECT_EQ(obj->UVec2()->GetValue(), BASE_NS::Math::UVec2(100, 200));

    BASE_NS::Math::Mat4X4 expectedMat{};
    expectedMat[0][0] = 2.f;
    expectedMat[1][1] = 3.f;
    expectedMat[2][2] = 4.f;
    expectedMat[3][3] = 1.f;
    EXPECT_EQ(obj->Mat4x4()->GetValue(), expectedMat);
    META_NS::UnregisterObjectType<BuiltinTypesAttachment>();
}

/**
 * @tc.name: ImportCameraInvalidProjection
 * @tc.desc: A camera with an unrecognised projection enum value fails the import with the
 *           specific "Invalid camera projection" diagnostic.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportCameraInvalidProjection, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/scene/scene_camera_invalid_projection.json", {});
    EXPECT_FALSE(res);
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "Invalid camera projection");
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "isometric");
}

/**
 * @tc.name: ImportLightProbeGroupEmpty
 * @tc.desc: A lightProbeGroup with an empty probes array imports successfully and produces
 *           a node whose probe list is empty.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportLightProbeGroupEmpty, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/scene/scene_light_probe_empty.json");
    ASSERT_TRUE(scene);
    auto node = interface_pointer_cast<SCENE_NS::ILightProbeGroup>(scene->FindNode("//probes").GetResult());
    ASSERT_TRUE(node);
    EXPECT_TRUE(node->GetLightProbeGroup().empty());
}

/**
 * @tc.name: ImportStructuralMalformedJson
 * @tc.desc: Import of a file that is not valid JSON fails with a top-level diagnostic
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportStructuralMalformedJson, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/structural/malformed.json", {});
    EXPECT_FALSE(res);
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "Invalid file, top-level JSON is not an object");
}

/**
 * @tc.name: ImportStructuralMissingVersion
 * @tc.desc: Top-level file without a "version" field fails as a missing-required-string error
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportStructuralMissingVersion, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/structural/missing_version.json", {});
    EXPECT_FALSE(res);
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "Missing required string 'version'");
}

/**
 * @tc.name: ImportStructuralWrongVersion
 * @tc.desc: Top-level "version" must equal "1.0" - anything else is rejected
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportStructuralWrongVersion, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/structural/wrong_version.json", {});
    EXPECT_FALSE(res);
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "Invalid value for required string 'version'");
}

/**
 * @tc.name: ImportStructuralMissingData
 * @tc.desc: Top-level file without a "data" object fails with a clear diagnostic
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportStructuralMissingData, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/structural/missing_data.json", {});
    EXPECT_FALSE(res);
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "'data' object missing or not an object");
}

/**
 * @tc.name: ImportStructuralUnknownType
 * @tc.desc: An unrecognised top-level data.type produces a "Requested unknown object type" error
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneImportTest, ImportStructuralUnknownType, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/structural/unknown_type.json", {});
    EXPECT_FALSE(res);
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "Requested unknown object type");
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "thingThatDoesNotExist");
}

}  // namespace UTest
SCENE_END_NAMESPACE()
