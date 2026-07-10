/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

// clang-format off
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
// clang-format on

#include <scene/api/node.h>
#include <scene/api/scene.h>
#include <scene/ext/intf_component.h>
#include <scene/interface/intf_environment.h>
#include <scene/interface/postprocess/intf_bloom.h>
#include <scene/interface/intf_image.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/intf_texture.h>
#include <scene_importer/interface/intf_importer.h>

#include <gmock/gmock.h>

#include <3d/ecs/components/material_component.h>
#include <base/math/vector.h>

#include <meta/api/metadata_util.h>
#include <meta/api/object.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/resource/intf_resource_manager_extension.h>

#include "import_test_helpers.h"
#include "scene/scene_test.h"
#include "test_attachment.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

using CORE3D_NS::MaterialComponent;

class API_NodeTemplateImportTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        META_NS::RegisterObjectType<TestAttachment>();
        ScenePluginTest::SetUp();
        imp = META_NS::GetObjectRegistry().Create<SCENE_IMP_NS::IImporter>(SCENE_IMP_NS::ClassId::Importer);
        ASSERT_TRUE(imp);
        ASSERT_TRUE(imp->Initialize(context, {}));
    }
    void TearDown() override
    {
        META_NS::UnregisterObjectType<TestAttachment>();
        ScenePluginTest::TearDown();
    }

    IScene::Ptr LoadTestScene(BASE_NS::string_view path)
    {
        auto res = imp->Import(path, {});
        EXPECT_TRUE(res);
        if (!res) {
            return nullptr;
        }
        return interface_pointer_cast<IScene>(res.object);
    }

    META_NS::IObjectTemplate::Ptr LoadTestTemplate(BASE_NS::string_view path)
    {
        auto res = imp->Import(path, {});
        EXPECT_TRUE(res);
        if (!res) {
            return nullptr;
        }
        return interface_pointer_cast<META_NS::IObjectTemplate>(res.object);
    }

public:
    SCENE_IMP_NS::IImporter::Ptr imp;
};

/**
 * @tc.name: ImportNodeTemplatePropertyOverride
 * @tc.desc: Template overrides set property defaults; scene overrides stack on top as explicit values. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, ImportNodeTemplatePropertyOverride, testing::ext::TestSize.Level1)
{
    constexpr BASE_NS::Math::Vec3 TEMPLATE_POS{1.0f, 2.0f, 3.0f};
    constexpr BASE_NS::Math::Vec3 TEMPLATE_SCALE{4.0f, 5.0f, 6.0f};
    constexpr BASE_NS::Math::Vec3 SCENE_POS{7.0f, 8.0f, 9.0f};

    auto scene = LoadTestScene("test://import/node_template/scene_nt0.json");
    ASSERT_TRUE(scene);

    auto n = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(n);

    // Position was overridden by scene on top of template default
    EXPECT_TRUE(n->Position()->IsValueSet());
    EXPECT_FALSE(n->Position()->IsDefaultValue());
    EXPECT_EQ(n->Position()->GetValue(), SCENE_POS);
    EXPECT_EQ(n->Position()->GetDefaultValue(), TEMPLATE_POS);

    // Scale was only overridden by template — still at default
    EXPECT_TRUE(n->Scale()->IsDefaultValue());
    EXPECT_EQ(n->Scale()->GetValue(), TEMPLATE_SCALE);
}

/**
 * @tc.name: ImportNodeTemplateNoSceneOverridePreservesDefault
 * @tc.desc: When a scene wraps a nodeTemplate via an external node and does NOT
 *          override a property, the imported node's property stays at its
 *          default (IsDefaultValue==true, !IsValueSet) with the template value.
 *          [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(
    API_NodeTemplateImportTest, ImportNodeTemplateNoSceneOverridePreservesDefault, testing::ext::TestSize.Level1)
{
    constexpr BASE_NS::Math::Vec3 TEMPLATE_POS{1.0f, 2.0f, 3.0f};
    constexpr BASE_NS::Math::Vec3 TEMPLATE_SCALE{4.0f, 5.0f, 6.0f};

    auto scene = LoadTestScene("test://import/node_template/scene_nt0_no_override.json");
    ASSERT_TRUE(scene);

    auto n = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(n);

    EXPECT_TRUE(n->Position()->IsDefaultValue());
    EXPECT_FALSE(n->Position()->IsValueSet());
    EXPECT_EQ(n->Position()->GetValue(), TEMPLATE_POS);
    EXPECT_EQ(n->Position()->GetDefaultValue(), TEMPLATE_POS);

    EXPECT_TRUE(n->Scale()->IsDefaultValue());
    EXPECT_FALSE(n->Scale()->IsValueSet());
    EXPECT_EQ(n->Scale()->GetValue(), TEMPLATE_SCALE);
    EXPECT_EQ(n->Scale()->GetDefaultValue(), TEMPLATE_SCALE);
}

/**
 * @tc.name: UpdateAppliesNewTemplateOverrides
 * @tc.desc: After update with a new template version, the new template's overrides become active defaults.
 * [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, UpdateAppliesNewTemplateOverrides, testing::ext::TestSize.Level1)
{
    auto t1 = LoadTestTemplate("test://import/node_template/node_template_upd_v1.json");
    auto t2 = LoadTestTemplate("test://import/node_template/node_template_upd_v2.json");
    ASSERT_TRUE(t1);
    ASSERT_TRUE(t2);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(t1).GetResult();
    ASSERT_TRUE(n);

    auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);
    // After v1 import: Position default=[1,2,3], Scale not overridden
    EXPECT_TRUE(cube->Position()->IsDefaultValue());
    EXPECT_EQ(cube->Position()->GetValue(), (BASE_NS::Math::Vec3{1.f, 2.f, 3.f}));
    EXPECT_FALSE(cube->Scale()->IsValueSet());

    n = interface_pointer_cast<INode>(t2->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);

    cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);
    // After v2 update: Position default=[10,20,30], Scale default=[2,2,2]
    EXPECT_TRUE(cube->Position()->IsDefaultValue());
    EXPECT_EQ(cube->Position()->GetValue(), (BASE_NS::Math::Vec3{10.f, 20.f, 30.f}));
    EXPECT_TRUE(cube->Scale()->IsDefaultValue());
    EXPECT_EQ(cube->Scale()->GetValue(), (BASE_NS::Math::Vec3{2.f, 2.f, 2.f}));
}

/**
 * @tc.name: UpdatePreservesUserPropertyOverride
 * @tc.desc: A property explicitly set by the user survives the update; the new template value becomes the default.
 * [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, UpdatePreservesUserPropertyOverride, testing::ext::TestSize.Level1)
{
    constexpr BASE_NS::Math::Vec3 USER_POS{7.f, 8.f, 9.f};
    constexpr BASE_NS::Math::Vec3 V2_POS{10.f, 20.f, 30.f};

    auto t1 = LoadTestTemplate("test://import/node_template/node_template_upd_v1.json");
    auto t2 = LoadTestTemplate("test://import/node_template/node_template_upd_v2.json");
    ASSERT_TRUE(t1);
    ASSERT_TRUE(t2);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(t1).GetResult();
    ASSERT_TRUE(n);

    auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);
    // User explicitly overrides Position
    cube->Position()->SetValue(USER_POS);

    n = interface_pointer_cast<INode>(t2->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);

    cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);
    // User value preserved as current, v2 value is the new default
    EXPECT_FALSE(cube->Position()->IsDefaultValue());
    EXPECT_EQ(cube->Position()->GetValue(), USER_POS);
    EXPECT_EQ(cube->Position()->GetDefaultValue(), V2_POS);
    // Scale came from v2 template
    EXPECT_TRUE(cube->Scale()->IsDefaultValue());
    EXPECT_EQ(cube->Scale()->GetValue(), (BASE_NS::Math::Vec3{2.f, 2.f, 2.f}));
}

/**
 * @tc.name: UpdatePreservesUserAttachment
 * @tc.desc: A user-added attachment on the root node survives the update. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, UpdatePreservesUserAttachment, testing::ext::TestSize.Level1)
{
    auto t1 = LoadTestTemplate("test://import/node_template/node_template_upd_v1.json");
    auto t2 = LoadTestTemplate("test://import/node_template/node_template_upd_v2.json");
    ASSERT_TRUE(t1);
    ASSERT_TRUE(t2);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(t1).GetResult();
    ASSERT_TRUE(n);

    // User attaches an object to the root (ext) node
    META_NS::Object userAtt = META_NS::CreateObjectInstance<META_NS::IObject>();
    META_NS::SetName(userAtt, "UserAttachment");
    interface_cast<META_NS::IAttach>(n)->Attach(userAtt.GetPtr());

    n = interface_pointer_cast<INode>(t2->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);

    // User attachment survived the update
    EXPECT_TRUE(interface_cast<META_NS::IAttach>(n)->GetAttachmentContainer()->FindByName("UserAttachment"));
    // Template's IExternalNode attachment is present
    EXPECT_TRUE(GetExternalNodeAttachment(n));
}

UNIT_TEST_F(API_NodeTemplateImportTest, ImportAttachmentName, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/attachment_named.json");
    ASSERT_TRUE(scene);
    auto root = scene->GetRootNode().GetResult();
    ASSERT_TRUE(root);
    auto att = interface_cast<META_NS::IAttach>(root)->GetAttachmentContainer()->FindByName("MyAttachment");
    ASSERT_TRUE(att);
    EXPECT_EQ(att->GetName(), "MyAttachment");
}

/**
 * @tc.name: MultiUpdate
 * @tc.desc: Multiple sequential updates switch template defaults back and forth correctly. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, MultiUpdate, testing::ext::TestSize.Level1)
{
    auto t1 = LoadTestTemplate("test://import/node_template/node_template_upd_v1.json");
    auto t2 = LoadTestTemplate("test://import/node_template/node_template_upd_v2.json");
    ASSERT_TRUE(t1);
    ASSERT_TRUE(t2);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(t1).GetResult();
    ASSERT_TRUE(n);

    {
        auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
        ASSERT_TRUE(cube);
        EXPECT_TRUE(cube->Position()->IsDefaultValue());
        EXPECT_EQ(cube->Position()->GetValue(), (BASE_NS::Math::Vec3{1.f, 2.f, 3.f}));
        EXPECT_FALSE(cube->Scale()->IsValueSet());
        EXPECT_TRUE(GetExternalNodeAttachment(n));
    }

    {
        n = interface_pointer_cast<INode>(t1->Update(*interface_cast<META_NS::IObject>(n), nullptr));
        ASSERT_TRUE(n);
        EXPECT_TRUE(GetExternalNodeAttachment(n));

        auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
        ASSERT_TRUE(cube);
        EXPECT_TRUE(cube->Position()->IsDefaultValue());
        EXPECT_EQ(cube->Position()->GetValue(), (BASE_NS::Math::Vec3{1.f, 2.f, 3.f}));
        EXPECT_FALSE(cube->Scale()->IsValueSet());
    }

    {
        n = interface_pointer_cast<INode>(t2->Update(*interface_cast<META_NS::IObject>(n), nullptr));
        ASSERT_TRUE(n);
        EXPECT_TRUE(GetExternalNodeAttachment(n));

        auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
        ASSERT_TRUE(cube);
        EXPECT_TRUE(cube->Position()->IsDefaultValue());
        EXPECT_EQ(cube->Position()->GetValue(), (BASE_NS::Math::Vec3{10.f, 20.f, 30.f}));
        EXPECT_TRUE(cube->Scale()->IsDefaultValue());
        EXPECT_EQ(cube->Scale()->GetValue(), (BASE_NS::Math::Vec3{2.f, 2.f, 2.f}));
    }

    {
        n = interface_pointer_cast<INode>(t1->Update(*interface_cast<META_NS::IObject>(n), nullptr));
        ASSERT_TRUE(n);
        EXPECT_TRUE(GetExternalNodeAttachment(n));

        auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
        ASSERT_TRUE(cube);
        EXPECT_TRUE(cube->Position()->IsDefaultValue());
        EXPECT_EQ(cube->Position()->GetValue(), (BASE_NS::Math::Vec3{1.f, 2.f, 3.f}));
        EXPECT_FALSE(cube->Scale()->IsValueSet());
    }
}

/**
 * @tc.name: UpdateUserOverrideAttachment
 * @tc.desc: A user override of a template-defined attachment property survives the update. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, UpdateUserOverrideAttachment, testing::ext::TestSize.Level1)
{
    auto findTemplateAtt = [](const INode::Ptr& nd) -> META_NS::IObject::Ptr {
        for (auto&& a : interface_cast<META_NS::IAttach>(nd)->GetAttachments()) {
            if (!interface_cast<::SCENE_NS::IExternalNode>(a) && !interface_cast<::SCENE_NS::IComponent>(a)) {
                return a;
            }
        }
        return nullptr;
    };

    auto t = LoadTestTemplate("test://import/node_template/node_template_upd_v1_att.json");
    ASSERT_TRUE(t);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(t).GetResult();
    ASSERT_TRUE(n);

    {
        auto att = findTemplateAtt(n);
        ASSERT_TRUE(att);
        auto floatProp = META_NS::Metadata(att).GetProperty<float>("Float");
        ASSERT_TRUE(floatProp);
        EXPECT_TRUE(floatProp->IsDefaultValue());
        EXPECT_EQ(floatProp->GetValue(), 1.f);
        floatProp->SetValue(2.f);
    }

    n = interface_pointer_cast<INode>(t->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);

    {
        auto att = findTemplateAtt(n);
        ASSERT_TRUE(att);
        auto floatProp = META_NS::Metadata(att).GetProperty<float>("Float");
        ASSERT_TRUE(floatProp);
        EXPECT_FALSE(floatProp->IsDefaultValue());
        EXPECT_EQ(floatProp->GetValue(), 2.f);
        EXPECT_EQ(floatProp->GetDefaultValue(), 1.f);
    }
}

/**
 * @tc.name: UpdateResetTemplateNodePropertyOverride
 * @tc.desc: Switching to a template without a property override resets it to the type default. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, UpdateResetTemplateNodePropertyOverride, testing::ext::TestSize.Level1)
{
    auto t1 = LoadTestTemplate("test://import/node_template/node_template_upd_v1.json");
    auto t2 = LoadTestTemplate("test://import/node_template/node_template_upd_v2.json");
    ASSERT_TRUE(t1);
    ASSERT_TRUE(t2);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    // Import v2 first (has Scale override)
    auto n = root->ImportTemplate(t2).GetResult();
    ASSERT_TRUE(n);

    {
        auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
        ASSERT_TRUE(cube);
        EXPECT_TRUE(cube->Scale()->IsDefaultValue());
        EXPECT_EQ(cube->Scale()->GetValue(), (BASE_NS::Math::Vec3{2.f, 2.f, 2.f}));
    }

    // Update to v1 (no Scale override) — Scale should reset to type default
    n = interface_pointer_cast<INode>(t1->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);

    {
        auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
        ASSERT_TRUE(cube);
        EXPECT_TRUE(cube->Scale()->IsDefaultValue());
        EXPECT_EQ(cube->Scale()->GetValue(), (BASE_NS::Math::Vec3{1.f, 1.f, 1.f}));
    }
}

/**
 * @tc.name: NodeTemplateUpdateAttachment
 * @tc.desc: Template attachment lifecycle across updates: user value preserved, template removal clears it.
 * [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, NodeTemplateUpdateAttachment, testing::ext::TestSize.Level1)
{
    // Find the TestAttachment by the presence of its typed "Float" property
    auto findTestAtt = [](const INode::Ptr& nd) -> META_NS::IObject::Ptr {
        for (auto&& a : interface_cast<META_NS::IAttach>(nd)->GetAttachments()) {
            if (META_NS::Metadata(a).GetProperty<float>("Float")) {
                return a;
            }
        }
        return nullptr;
    };

    auto tAtt = LoadTestTemplate("test://import/node_template/node_template_upd_v1_att.json");
    auto tNoAtt = LoadTestTemplate("test://import/node_template/node_template_upd_v2.json");
    ASSERT_TRUE(tAtt);
    ASSERT_TRUE(tNoAtt);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(tAtt).GetResult();
    ASSERT_TRUE(n);

    // Set user override on template attachment and add a user attachment
    {
        auto att = findTestAtt(n);
        ASSERT_TRUE(att);
        META_NS::Metadata(att).GetProperty<float>("Float")->SetValue(2.f);

        META_NS::Object userAtt = META_NS::CreateObjectInstance<META_NS::IObject>();
        META_NS::SetName(userAtt, "Other");
        interface_cast<META_NS::IAttach>(n)->Attach(userAtt.GetPtr());
    }

    // Update v1_att: template attachment present with user value preserved, user attachment survives
    n = interface_pointer_cast<INode>(tAtt->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);
    {
        EXPECT_TRUE(GetExternalNodeAttachment(n));
        EXPECT_TRUE(interface_cast<META_NS::IAttach>(n)->GetAttachmentContainer()->FindByName("Other"));

        auto att = findTestAtt(n);
        ASSERT_TRUE(att);
        auto floatProp = META_NS::Metadata(att).GetProperty<float>("Float");
        ASSERT_TRUE(floatProp);
        EXPECT_FALSE(floatProp->IsDefaultValue());
        EXPECT_EQ(floatProp->GetValue(), 2.f);
        EXPECT_EQ(floatProp->GetDefaultValue(), 1.f);
    }

    // Update v2 (no attachment in template): template attachment gone, user attachment survives
    n = interface_pointer_cast<INode>(tNoAtt->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);
    {
        EXPECT_TRUE(GetExternalNodeAttachment(n));
        EXPECT_TRUE(interface_cast<META_NS::IAttach>(n)->GetAttachmentContainer()->FindByName("Other"));
        EXPECT_FALSE(findTestAtt(n));
    }

    // Update back to v1_att: template attachment re-created with fresh default (user value gone)
    n = interface_pointer_cast<INode>(tAtt->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);
    {
        EXPECT_TRUE(GetExternalNodeAttachment(n));
        EXPECT_TRUE(interface_cast<META_NS::IAttach>(n)->GetAttachmentContainer()->FindByName("Other"));

        auto att = findTestAtt(n);
        ASSERT_TRUE(att);
        auto floatProp = META_NS::Metadata(att).GetProperty<float>("Float");
        ASSERT_TRUE(floatProp);
        EXPECT_TRUE(floatProp->IsDefaultValue());
        EXPECT_EQ(floatProp->GetValue(), 1.f);
    }
}

/**
 * @tc.name: SceneOverrideSurvivesTemplateUpdate
 * @tc.desc: Property overrides specified in a scene JSON survive a template update. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, SceneOverrideSurvivesTemplateUpdate, testing::ext::TestSize.Level1)
{
    constexpr BASE_NS::Math::Vec3 SCENE_POS{7.f, 8.f, 9.f};
    constexpr BASE_NS::Math::Vec3 V2_POS{10.f, 20.f, 30.f};
    constexpr BASE_NS::Math::Vec3 V2_SCALE{2.f, 2.f, 2.f};

    auto scene = LoadTestScene("test://import/node_template/scene_nt0.json");
    ASSERT_TRUE(scene);

    auto ext = scene->FindNode("//ext").GetResult();
    ASSERT_TRUE(ext);

    {
        auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
        ASSERT_TRUE(cube);
        EXPECT_FALSE(cube->Position()->IsDefaultValue());
        EXPECT_EQ(cube->Position()->GetValue(), SCENE_POS);
    }

    auto t2 = LoadTestTemplate("test://import/node_template/node_template_upd_v2.json");
    ASSERT_TRUE(t2);

    auto updated = interface_pointer_cast<INode>(t2->Update(*interface_cast<META_NS::IObject>(ext), nullptr));
    ASSERT_TRUE(updated);

    {
        auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
        ASSERT_TRUE(cube);
        EXPECT_FALSE(cube->Position()->IsDefaultValue());
        EXPECT_EQ(cube->Position()->GetValue(), SCENE_POS);
        EXPECT_EQ(cube->Position()->GetDefaultValue(), V2_POS);
        EXPECT_TRUE(cube->Scale()->IsDefaultValue());
        EXPECT_EQ(cube->Scale()->GetValue(), V2_SCALE);
    }
}

/**
 * @tc.name: PrimaryGroupResourceReGrouping
 * @tc.desc: Resources with primaryGroup are re-grouped to runtime group during template import;
 *           options and resource templates also use the new group.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, PrimaryGroupResourceReGrouping, testing::ext::TestSize.Level1)
{
    auto templ = LoadTestTemplate("test://import/node_template/node_template_pg.json");
    ASSERT_TRUE(templ);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    // Import with a different runtime group than the template's primaryGroup
    constexpr auto RUNTIME_GROUP = "runtime-group";
    auto n = root->ImportTemplate(templ, RUNTIME_GROUP).GetResult();
    ASSERT_TRUE(n);

    // Node should be created
    auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);
    EXPECT_EQ(cube->Position()->GetValue(), (BASE_NS::Math::Vec3{1.f, 2.f, 3.f}));

    // Verify resources were re-grouped to runtime group
    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);
    auto extRes = interface_cast<META_NS::IResourceManagerExtension>(rman);
    ASSERT_TRUE(extRes);

    // Resources under original group should NOT exist
    auto oldBase = extRes->GetResource({{"base-mat", "tmpl-group"}, scene});
    EXPECT_FALSE(oldBase.id.IsValid());

    // Resources under new runtime group SHOULD exist
    auto newBase = extRes->GetResource({{"base-mat", RUNTIME_GROUP}, scene});
    EXPECT_TRUE(newBase.id.IsValid());

    auto newDerived = extRes->GetResource({{"derived-mat", RUNTIME_GROUP}, scene});
    EXPECT_TRUE(newDerived.id.IsValid());

    // Verify derived resource has correct derivedFrom with re-grouped base
    auto derived = interface_cast<META_NS::IDerivedResourceOptions>(newDerived.options);
    ASSERT_TRUE(derived);
    EXPECT_EQ(derived->GetBaseResource(), CORE_NS::ResourceId("base-mat", RUNTIME_GROUP));

    // Instantiate the derived material and verify options were applied
    auto mat = interface_pointer_cast<IMaterial>(
        rman->GetResource({CORE_NS::ResourceId("derived-mat", RUNTIME_GROUP), scene}));
    ASSERT_TRUE(mat);

    // Should inherit from base (material0.json: metallicRoughness, alphaCutoff=0.5, lightingFlags=2)
    EXPECT_EQ(META_NS::GetValue(mat->Type()), MaterialType::METALLIC_ROUGHNESS);
    EXPECT_EQ(META_NS::GetValue(mat->LightingFlags()), static_cast<LightingFlags>(2));

    // Options override should be applied
    EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.75f);
}

/**
 * @tc.name: ScenePrimaryGroupResourceReGrouping
 * @tc.desc: Resources with primaryGroup are re-grouped when the scene JSON specifies a resourceGroup
 *           on the external node reference — tests the full end-to-end scene import path.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, ScenePrimaryGroupResourceReGrouping, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_nt_pg.json");
    ASSERT_TRUE(scene);

    // Node should be created with position override from the template
    auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);
    EXPECT_EQ(cube->Position()->GetValue(), (BASE_NS::Math::Vec3{1.f, 2.f, 3.f}));

    // Verify resources were re-grouped to the scene's runtime group
    constexpr auto RUNTIME_GROUP = "scene-runtime-group";
    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);
    auto extRes = interface_cast<META_NS::IResourceManagerExtension>(rman);
    ASSERT_TRUE(extRes);

    // Resources under original group should NOT exist
    auto oldBase = extRes->GetResource({{"base-mat", "tmpl-group"}, scene});
    EXPECT_FALSE(oldBase.id.IsValid());

    // Resources under new runtime group SHOULD exist
    auto newBase = extRes->GetResource({{"base-mat", RUNTIME_GROUP}, scene});
    EXPECT_TRUE(newBase.id.IsValid());

    auto newDerived = extRes->GetResource({{"derived-mat", RUNTIME_GROUP}, scene});
    EXPECT_TRUE(newDerived.id.IsValid());

    // Verify derived resource has correct derivedFrom with re-grouped base
    auto derived = interface_cast<META_NS::IDerivedResourceOptions>(newDerived.options);
    ASSERT_TRUE(derived);
    EXPECT_EQ(derived->GetBaseResource(), CORE_NS::ResourceId("base-mat", RUNTIME_GROUP));

    // Instantiate the derived material and verify options were applied
    auto mat = interface_pointer_cast<IMaterial>(
        rman->GetResource({CORE_NS::ResourceId("derived-mat", RUNTIME_GROUP), scene}));
    ASSERT_TRUE(mat);

    // Should inherit from base (material0.json: metallicRoughness, alphaCutoff=0.5, lightingFlags=2)
    EXPECT_EQ(META_NS::GetValue(mat->Type()), MaterialType::METALLIC_ROUGHNESS);
    EXPECT_EQ(META_NS::GetValue(mat->LightingFlags()), static_cast<LightingFlags>(2));

    // Options override should be applied
    EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.75f);
}

inline void CheckLightingFlags(const IMaterial::Ptr& mat, LightingFlags expectedSet)
{
    ASSERT_TRUE(mat);
    uint32_t expectedFlags = static_cast<uint32_t>(expectedSet);
    uint32_t flags = static_cast<uint32_t>(META_NS::GetValue(mat->LightingFlags()));
    EXPECT_TRUE(flags & expectedFlags) << flags << " " << expectedFlags;
}

/**
 * @tc.name: TemplateMaterialPropertyOverride
 * @tc.desc: Material properties from template index are correctly set;
 *           a template without a property leaves it at the type default.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, TemplateMaterialPropertyOverride, testing::ext::TestSize.Level1)
{
    constexpr auto MAT_GROUP = "mat-group";
    auto t1 = LoadTestTemplate("test://import/node_template/node_template_mat_v1.json");
    ASSERT_TRUE(t1);

    // v1 template has material with alphaCutoff=0.5 and lightingFlags=2
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);
        auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(root);

        auto n = root->ImportTemplate(t1, MAT_GROUP).GetResult();
        ASSERT_TRUE(n);

        auto rman = GetResourceManager(scene);
        ASSERT_TRUE(rman);
        CORE_NS::ResourceId matId{"testMat", MAT_GROUP};
        auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
        ASSERT_TRUE(mat);
        EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.5f);
        EXPECT_EQ(META_NS::GetValue(mat->LightingFlags()), static_cast<LightingFlags>(2));
    }

    // v2 template has material without alphaCutoff or lightingFlags — should be at type defaults
    auto t2 = LoadTestTemplate("test://import/node_template/node_template_mat_v2.json");
    ASSERT_TRUE(t2);

    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);
        auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(root);

        auto n = root->ImportTemplate(t2, MAT_GROUP).GetResult();
        ASSERT_TRUE(n);

        auto rman = GetResourceManager(scene);
        ASSERT_TRUE(rman);
        CORE_NS::ResourceId matId{"testMat", MAT_GROUP};
        auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
        ASSERT_TRUE(mat);
        // v2 does not override alphaCutoff or lightingFlags, so they come from the GLTF base material
        EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 1.f);
        CheckLightingFlags(mat, LightingFlags::SHADOW_RECEIVER_BIT);
    }
}

/**
 * @tc.name: UpdateResetTemplateMaterialPropertyOverride
 * @tc.desc: Switching to a template whose material lacks a property override resets it to default.
 *           (New-importer equivalent of the old DISABLED_UpdateResetTemplateMaterialPropertyOverride test.)
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, UpdateResetTemplateMaterialPropertyOverride, testing::ext::TestSize.Level1)
{
    auto t1 = LoadTestTemplate("test://import/node_template/node_template_mat_v1.json");
    auto t2 = LoadTestTemplate("test://import/node_template/node_template_mat_v2.json");
    ASSERT_TRUE(t1);
    ASSERT_TRUE(t2);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    constexpr auto MAT_GROUP = "mat-group";
    auto n = root->ImportTemplate(t1, MAT_GROUP).GetResult();
    ASSERT_TRUE(n);

    {
        auto rman = GetResourceManager(scene);
        ASSERT_TRUE(rman);
        CORE_NS::ResourceId matId{"testMat", MAT_GROUP};
        auto info = rman->GetResourceInfo({matId, scene});
        EXPECT_TRUE(info.id.IsValid()) << "testMat not in resource manager after first import";
        auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
        ASSERT_TRUE(mat);
        EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.5f);
        CheckLightingFlags(mat, LightingFlags::SHADOW_CASTER_BIT);
    }

    // Update to v2 which does NOT specify alphaCutoff or lightingFlags
    n = interface_pointer_cast<INode>(t2->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);

    {
        auto rman = GetResourceManager(scene);
        ASSERT_TRUE(rman);
        CORE_NS::ResourceId matId{"testMat", MAT_GROUP};
        auto info = rman->GetResourceInfo({matId, scene});
        EXPECT_TRUE(info.id.IsValid()) << "testMat not in resource manager after update";
        auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
        ASSERT_TRUE(mat) << "Material resource 'testMat' not found after template update";
        // After update to v2 (no alphaCutoff/lightingFlags overrides), values reset to GLTF base
        EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 1.f);
        CheckLightingFlags(mat, LightingFlags::SHADOW_RECEIVER_BIT);
    }
}

/**
 * @tc.name: SceneIndexAndTemplateIndexShareMaterial
 * @tc.desc: When the same material resource id appears in BOTH the scene's own index and a node
 *           template's index, the template's values should land on the live material as defaults
 *           and the scene's values as set values on top.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, SceneIndexAndTemplateIndexShareMaterial, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_nt_mat_overlap.json");
    ASSERT_TRUE(scene);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);
    CORE_NS::ResourceId matId{"testMat", "mat-group"};
    auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
    ASSERT_TRUE(mat);

    // Template (node_template_mat_v1_index.json): alphaCutoff=0.5, lightingFlags=shadowCaster (=2)
    // Scene  (scene_nt_mat_overlap.index):        alphaCutoff=0.75, materialType=metallicRoughness
    // Expected layering:
    //   - alphaCutoff current = 0.75 (scene overrides template as a set value)
    //   - alphaCutoff default = 0.5  (template contributes the property default)
    //   - lightingFlags = shadowCaster (template-only contribution)
    //   - materialType  = metallicRoughness (scene-only contribution)
    EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.75f);
    EXPECT_FLOAT_EQ(mat->AlphaCutoff()->GetDefaultValue(), 0.5f);
    EXPECT_EQ(META_NS::GetValue(mat->LightingFlags()), static_cast<LightingFlags>(2));
    EXPECT_EQ(META_NS::GetValue(mat->Type()), MaterialType::METALLIC_ROUGHNESS);
}

/**
 * @tc.name: SceneLoadsNodeTemplateGltfMaterialOverride
 * @tc.desc: Scene JSON references a node template whose own index overrides a material that
 *           comes from a gltf loaded inside that template (the index entry id matches the
 *           gltf-defined material name "AnimatedCube"). The override must land both in the
 *           resource manager and on the live submesh material when the scene is imported
 *           via the scene importer path.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, SceneLoadsNodeTemplateGltfMaterialOverride, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_nt_gltf_mat_override.json");
    ASSERT_TRUE(scene);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    // The gltf defines a material named "AnimatedCube". With resourceGroup=mat-group the gltf
    // loader places it under mat-group. The template index overrides alphaCutoff and
    // lightingFlags for that same id.
    CORE_NS::ResourceId matId{"AnimatedCube", "mat-group"};
    auto rmMat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
    ASSERT_TRUE(rmMat);

    // node_template_gltf_mat_override_index.json: alphaCutoff=0.5, lightingFlags=shadowCaster (=2)
    EXPECT_FLOAT_EQ(META_NS::GetValue(rmMat->AlphaCutoff()), 0.5f);
    EXPECT_EQ(META_NS::GetValue(rmMat->LightingFlags()), static_cast<LightingFlags>(2));

    // And the override must also have landed on the gltf-loaded mesh's submesh material —
    // not just the resource manager entry.
    auto mesh = scene->FindNode<IMesh>("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(mesh);
    auto subs = mesh->SubMeshes()->GetValue();
    ASSERT_FALSE(subs.empty());
    auto subMat = subs[0]->Material()->GetValue();
    ASSERT_TRUE(subMat);
    EXPECT_FLOAT_EQ(META_NS::GetValue(subMat->AlphaCutoff()), 0.5f);
    EXPECT_EQ(META_NS::GetValue(subMat->LightingFlags()), static_cast<LightingFlags>(2));
    EXPECT_EQ(subMat, rmMat);

    // The override also assigns a baseColor texture pointing at an image defined in the same
    // template index ("ntLogo" in mat-group, path test://images/logo.png — 1200x1219). The
    // gltf's own baseColor image is 512x512, so a successful override is observable by the
    // image dimensions on the live material's baseColor slot.
    auto ntLogo =
        interface_pointer_cast<IImage>(rman->GetResource({CORE_NS::ResourceId("ntLogo", "mat-group"), scene}));
    ASSERT_TRUE(ntLogo);
    const auto ntLogoSize = META_NS::GetValue(ntLogo->Size());

    auto textures = subMat->Textures();
    ASSERT_TRUE(textures);
    auto baseColor = textures->GetValueAt(MaterialComponent::TextureIndex::BASE_COLOR);
    ASSERT_TRUE(baseColor);
    auto baseImg = META_NS::GetValue(baseColor->Image());
    ASSERT_TRUE(baseImg) << "baseColor image was not resolved from the node template's index";
    EXPECT_EQ(META_NS::GetValue(baseImg->Size()), ntLogoSize)
        << "baseColor image should be the override (ntLogo / logo.png) but its dimensions "
           "match the gltf's own baseColor image — the template index override did not apply";
}

/**
 * @tc.name: SceneLoadsNodeTemplateGltfImageRef
 * @tc.desc: A material override in a node template's index points its baseColor image at a
 *           gltf-loaded image (no separate file path). The index forward-declares the image
 *           id with `type: image` and no path; the gltf load is expected to supply the
 *           instance for that id. The override must land on the live material's baseColor
 *           slot when the scene is imported. Detection: the override targets images/1
 *           (MetallicRoughness, distinct dimensions from the gltf's own BaseColor image),
 *           so the swap is observable.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, SceneLoadsNodeTemplateGltfImageRef, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_nt_gltf_img_override.json");
    ASSERT_TRUE(scene);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    auto gltfMR = interface_pointer_cast<IImage>(
        rman->GetResource({CORE_NS::ResourceId("AnimatedCube.gltf/images/1", "mat-group"), scene}));
    ASSERT_TRUE(gltfMR);
    const auto mrSize = META_NS::GetValue(gltfMR->Size());

    auto mesh = scene->FindNode<IMesh>("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(mesh);
    auto subs = mesh->SubMeshes()->GetValue();
    ASSERT_FALSE(subs.empty());
    auto subMat = subs[0]->Material()->GetValue();
    ASSERT_TRUE(subMat);

    auto textures = subMat->Textures();
    ASSERT_TRUE(textures);
    auto baseColor = textures->GetValueAt(MaterialComponent::TextureIndex::BASE_COLOR);
    ASSERT_TRUE(baseColor);
    auto baseImg = META_NS::GetValue(baseColor->Image());
    ASSERT_TRUE(baseImg) << "baseColor image was not assigned";

    EXPECT_EQ(META_NS::GetValue(baseImg->Size()), mrSize)
        << "baseColor image should be the MetallicRoughness image (images/1, referenced by "
           "the template's index override) but its dimensions don't match — the override "
           "did not land on the gltf-loaded baseColor slot";
}

/**
 * @tc.name: NodeTemplateIndexMaterialShaderLazyRef
 * @tc.desc: A node template's index defines a shader and a material that references it via
 *           materialShader and depthShader. After scene import, instantiating the material
 *           via the resource manager must bind both shader slots to the index-defined
 *           shader — verifies lazy resolution of MaterialShader$ref / DepthShader$ref at
 *           apply time against the live scene's resource manager.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, NodeTemplateIndexMaterialShaderLazyRef, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_nt_mat_shader.json");
    ASSERT_TRUE(scene);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    auto expected = interface_pointer_cast<IShader>(rman->GetResource({"ntShader", scene}));
    ASSERT_TRUE(expected);

    auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({"ntMat", scene}));
    ASSERT_TRUE(mat);

    auto matShader = META_NS::GetValue(mat->MaterialShader());
    ASSERT_TRUE(matShader);
    EXPECT_EQ(matShader, expected);

    auto depthShader = META_NS::GetValue(mat->DepthShader());
    ASSERT_TRUE(depthShader);
    EXPECT_EQ(depthShader, expected);
}

/**
 * @tc.name: NodeTemplateIndexEnvironmentImageLazyRefs
 * @tc.desc: A node template's index defines two images and an environment that references
 *           them via radianceImage and environmentImage. Instantiating the environment
 *           after scene import must bind both image slots — verifies lazy resolution of
 *           RadianceImage$ref / EnvironmentImage$ref at apply time on an Environment
 *           resource.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, NodeTemplateIndexEnvironmentImageLazyRefs, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_nt_env.json");
    ASSERT_TRUE(scene);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    auto radiance = interface_pointer_cast<IImage>(rman->GetResource({"ntRadiance", scene}));
    ASSERT_TRUE(radiance);
    auto envImg = interface_pointer_cast<IImage>(rman->GetResource({"ntEnvImg", scene}));
    ASSERT_TRUE(envImg);

    auto env = interface_pointer_cast<IEnvironment>(rman->GetResource({"ntEnv", scene}));
    ASSERT_TRUE(env);

    auto boundRadiance = META_NS::GetValue(env->RadianceImage());
    ASSERT_TRUE(boundRadiance);
    EXPECT_EQ(boundRadiance, radiance);

    auto boundEnvImg = META_NS::GetValue(env->EnvironmentImage());
    ASSERT_TRUE(boundEnvImg);
    EXPECT_EQ(boundEnvImg, envImg);
}

/**
 * @tc.name: NodeTemplateIndexPostProcessImageLazyRef
 * @tc.desc: A node template's index defines an image and a postProcess whose bloom
 *           dirtMaskImage references that image. Instantiating the postProcess after scene
 *           import must bind the nested DirtMaskImage slot — verifies lazy resolution of
 *           refs that live on an owned sub-object (Bloom) inside the resource template.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, NodeTemplateIndexPostProcessImageLazyRef, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_nt_pp.json");
    ASSERT_TRUE(scene);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    auto dirt = interface_pointer_cast<IImage>(rman->GetResource({"ntDirt", scene}));
    ASSERT_TRUE(dirt);

    auto pp = interface_pointer_cast<IPostProcess>(rman->GetResource({"ntPP", scene}));
    ASSERT_TRUE(pp);

    auto bloom = META_NS::GetValue(pp->Bloom());
    ASSERT_TRUE(bloom);
    EXPECT_TRUE(META_NS::GetValue(bloom->Enabled()));

    auto boundDirt = META_NS::GetValue(bloom->DirtMaskImage());
    ASSERT_TRUE(boundDirt);
    EXPECT_EQ(boundDirt, dirt);
}

/**
 * @tc.name: AttachmentToChildNodeViaPath
 * @tc.desc: An external node attachment with a path targets a child node, not the root.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, AttachmentToChildNodeViaPath, testing::ext::TestSize.Level1)
{
    auto findTestAtt = [](const META_NS::IObject::Ptr& obj) -> META_NS::IObject::Ptr {
        auto attach = interface_cast<META_NS::IAttach>(obj);
        if (!attach) {
            return nullptr;
        }
        for (auto&& a : attach->GetAttachments()) {
            if (META_NS::Metadata(a).GetProperty<float>("Float")) {
                return a;
            }
        }
        return nullptr;
    };

    auto templ = LoadTestTemplate("test://import/node_template/node_template_att_child.json");
    ASSERT_TRUE(templ);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(templ).GetResult();
    ASSERT_TRUE(n);

    // Root external node should NOT have the TestAttachment
    EXPECT_FALSE(findTestAtt(interface_pointer_cast<META_NS::IObject>(n)));

    // Child node should have the TestAttachment
    auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);

    auto att = findTestAtt(interface_pointer_cast<META_NS::IObject>(cube));
    ASSERT_TRUE(att);

    auto floatProp = META_NS::Metadata(att).GetProperty<float>("Float");
    ASSERT_TRUE(floatProp);
    EXPECT_EQ(floatProp->GetValue(), 3.f);

    auto nameProp = META_NS::Metadata(att).GetProperty<BASE_NS::string>("Name");
    ASSERT_TRUE(nameProp);
    EXPECT_EQ(nameProp->GetValue(), "ChildAtt");
}

/**
 * @tc.name: AttachmentToRootAndChildViaPath
 * @tc.desc: Two attachments: one on root (no path), one on child (with path). Each lands on correct node.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, AttachmentToRootAndChildViaPath, testing::ext::TestSize.Level1)
{
    auto findTestAtt = [](const META_NS::IObject::Ptr& obj) -> META_NS::IObject::Ptr {
        auto attach = interface_cast<META_NS::IAttach>(obj);
        if (!attach) {
            return nullptr;
        }
        for (auto&& a : attach->GetAttachments()) {
            if (META_NS::Metadata(a).GetProperty<float>("Float")) {
                return a;
            }
        }
        return nullptr;
    };

    auto templ = LoadTestTemplate("test://import/node_template/node_template_att_root_and_child.json");
    ASSERT_TRUE(templ);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(templ).GetResult();
    ASSERT_TRUE(n);

    // Root should have the TestAttachment with Float=1.0 and Name="RootAtt"
    {
        auto att = findTestAtt(interface_pointer_cast<META_NS::IObject>(n));
        ASSERT_TRUE(att);
        auto floatProp = META_NS::Metadata(att).GetProperty<float>("Float");
        ASSERT_TRUE(floatProp);
        EXPECT_EQ(floatProp->GetValue(), 1.f);
        auto nameProp = META_NS::Metadata(att).GetProperty<BASE_NS::string>("Name");
        ASSERT_TRUE(nameProp);
        EXPECT_EQ(nameProp->GetValue(), "RootAtt");
    }

    // Child should have the TestAttachment with Float=2.0 and Name="ChildAtt"
    {
        auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
        ASSERT_TRUE(cube);
        auto att = findTestAtt(interface_pointer_cast<META_NS::IObject>(cube));
        ASSERT_TRUE(att);
        auto floatProp = META_NS::Metadata(att).GetProperty<float>("Float");
        ASSERT_TRUE(floatProp);
        EXPECT_EQ(floatProp->GetValue(), 2.f);
        auto nameProp = META_NS::Metadata(att).GetProperty<BASE_NS::string>("Name");
        ASSERT_TRUE(nameProp);
        EXPECT_EQ(nameProp->GetValue(), "ChildAtt");
    }
}

/**
 * @tc.name: ComponentOnExternalNodeTemplateChild
 * @tc.desc: Component added to a child node of an external gltf hierarchy via path in a node template
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, ComponentOnExternalNodeTemplateChild, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_ext_component.json");
    ASSERT_TRUE(scene);

    auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);

    auto comp = scene->GetComponent(cube, "RenderConfigurationComponent");
    ASSERT_TRUE(comp);

    auto rc = interface_pointer_cast<IRenderConfiguration>(comp);
    ASSERT_TRUE(rc);

    // ShadowQuality: template sets HIGH(2) as default, scene overrides to ULTRA(3)
    EXPECT_TRUE(rc->ShadowQuality()->IsValueSet());
    EXPECT_FALSE(rc->ShadowQuality()->IsDefaultValue());
    EXPECT_EQ(rc->ShadowQuality()->GetValue(), SceneShadowQuality::ULTRA);
    EXPECT_EQ(rc->ShadowQuality()->GetDefaultValue(), SceneShadowQuality::HIGH);

    // ShadowType: template sets VSM(1) as default, scene does not override
    EXPECT_TRUE(rc->ShadowType()->IsDefaultValue());
    EXPECT_EQ(rc->ShadowType()->GetValue(), SceneShadowType::VSM);
}

/**
 * @tc.name: ComponentOnExternalNodeSurvivesUpdate
 * @tc.desc: Template-imported component property overrides survive template update; new defaults applied.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, ComponentOnExternalNodeSurvivesUpdate, testing::ext::TestSize.Level1)
{
    auto t1 = LoadTestTemplate("test://import/node_template/node_template_ext_component.json");
    auto t2 = LoadTestTemplate("test://import/node_template/node_template_ext_component_v2.json");
    ASSERT_TRUE(t1);
    ASSERT_TRUE(t2);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(t1).GetResult();
    ASSERT_TRUE(n);

    auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);

    auto comp = scene->GetComponent(cube, "RenderConfigurationComponent");
    ASSERT_TRUE(comp);
    auto rc = interface_pointer_cast<IRenderConfiguration>(comp);
    ASSERT_TRUE(rc);

    // v1 defaults: ShadowType=VSM(1), ShadowQuality=HIGH(2)
    EXPECT_TRUE(rc->ShadowType()->IsDefaultValue());
    EXPECT_EQ(rc->ShadowType()->GetValue(), SceneShadowType::VSM);
    EXPECT_TRUE(rc->ShadowQuality()->IsDefaultValue());
    EXPECT_EQ(rc->ShadowQuality()->GetValue(), SceneShadowQuality::HIGH);

    // User overrides ShadowQuality
    rc->ShadowQuality()->SetValue(SceneShadowQuality::LOW);

    // Update to v2: ShadowType=PCF(0), ShadowQuality=ULTRA(3)
    n = interface_pointer_cast<INode>(t2->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);

    cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);

    comp = scene->GetComponent(cube, "RenderConfigurationComponent");
    ASSERT_TRUE(comp);
    rc = interface_pointer_cast<IRenderConfiguration>(comp);
    ASSERT_TRUE(rc);

    // ShadowType: not overridden by user, should pick up v2 default PCF(0)
    EXPECT_TRUE(rc->ShadowType()->IsDefaultValue());
    EXPECT_EQ(rc->ShadowType()->GetValue(), SceneShadowType::PCF);

    // ShadowQuality: user override LOW preserved, v2 default is ULTRA(3)
    EXPECT_FALSE(rc->ShadowQuality()->IsDefaultValue());
    EXPECT_EQ(rc->ShadowQuality()->GetValue(), SceneShadowQuality::LOW);
    EXPECT_EQ(rc->ShadowQuality()->GetDefaultValue(), SceneShadowQuality::ULTRA);
}

/**
 * @tc.name: UserAddedComponentSurvivesUpdate
 * @tc.desc: A component added by the user (not from template) survives template update.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, UserAddedComponentSurvivesUpdate, testing::ext::TestSize.Level1)
{
    auto t1 = LoadTestTemplate("test://import/node_template/node_template_upd_v1.json");
    auto t2 = LoadTestTemplate("test://import/node_template/node_template_upd_v2.json");
    ASSERT_TRUE(t1);
    ASSERT_TRUE(t2);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(t1).GetResult();
    ASSERT_TRUE(n);

    auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);

    // User adds a component (not defined in template)
    auto comp = scene->CreateComponent(cube, "RenderConfigurationComponent").GetResult();
    ASSERT_TRUE(comp);
    auto rc = interface_pointer_cast<IRenderConfiguration>(comp);
    ASSERT_TRUE(rc);
    rc->ShadowType()->SetValue(SceneShadowType::VSM);

    // Update to v2
    n = interface_pointer_cast<INode>(t2->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);

    cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);

    // User-added component should still be present
    comp = scene->GetComponent(cube, "RenderConfigurationComponent");
    ASSERT_TRUE(comp);
    rc = interface_pointer_cast<IRenderConfiguration>(comp);
    ASSERT_TRUE(rc);
    EXPECT_EQ(rc->ShadowType()->GetValue(), SceneShadowType::VSM);
}

/**
 * @tc.name: AttachmentToChildViaPathSurvivesUpdate
 * @tc.desc: Path-based child attachment survives template update; user override preserved.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, AttachmentToChildViaPathSurvivesUpdate, testing::ext::TestSize.Level1)
{
    auto findTestAtt = [](const META_NS::IObject::Ptr& obj) -> META_NS::IObject::Ptr {
        auto attach = interface_cast<META_NS::IAttach>(obj);
        if (!attach) {
            return nullptr;
        }
        for (auto&& a : attach->GetAttachments()) {
            if (META_NS::Metadata(a).GetProperty<float>("Float")) {
                return a;
            }
        }
        return nullptr;
    };

    auto t1 = LoadTestTemplate("test://import/node_template/node_template_att_child.json");
    auto t2 = LoadTestTemplate("test://import/node_template/node_template_att_child_v2.json");
    ASSERT_TRUE(t1);
    ASSERT_TRUE(t2);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(t1).GetResult();
    ASSERT_TRUE(n);

    // After v1 import: child has attachment with Float=3.0
    auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);
    {
        auto att = findTestAtt(interface_pointer_cast<META_NS::IObject>(cube));
        ASSERT_TRUE(att);
        auto floatProp = META_NS::Metadata(att).GetProperty<float>("Float");
        ASSERT_TRUE(floatProp);
        EXPECT_TRUE(floatProp->IsDefaultValue());
        EXPECT_EQ(floatProp->GetValue(), 3.f);

        // User overrides Float
        floatProp->SetValue(5.f);
    }

    // Update to v2 (Float=7.0)
    n = interface_pointer_cast<INode>(t2->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);

    cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);
    {
        auto att = findTestAtt(interface_pointer_cast<META_NS::IObject>(cube));
        ASSERT_TRUE(att);
        auto floatProp = META_NS::Metadata(att).GetProperty<float>("Float");
        ASSERT_TRUE(floatProp);
        // User value preserved, new template value is the default
        EXPECT_FALSE(floatProp->IsDefaultValue());
        EXPECT_EQ(floatProp->GetValue(), 5.f);
        EXPECT_EQ(floatProp->GetDefaultValue(), 7.f);
    }
}

/**
 * @tc.name: SceneMultiLevelAttachments
 * @tc.desc: Attachments distributed across a mixed hierarchy land on correct nodes and don't leak.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, SceneMultiLevelAttachments, testing::ext::TestSize.Level1)
{
    auto findAtt = [](const META_NS::IObject::Ptr& obj, BASE_NS::string_view attName) -> META_NS::IObject::Ptr {
        auto attach = interface_cast<META_NS::IAttach>(obj);
        if (!attach) {
            return nullptr;
        }
        for (auto&& a : attach->GetAttachments()) {
            auto n = META_NS::Metadata(a).GetProperty<BASE_NS::string>("Name");
            if (n && n->GetValue() == attName) {
                return a;
            }
        }
        return nullptr;
    };

    auto scene = LoadTestScene("test://import/node_template/scene_multi_level_att.json");
    ASSERT_TRUE(scene);

    auto parent = interface_pointer_cast<META_NS::IObject>(scene->FindNode("//parent").GetResult());
    auto ext = interface_pointer_cast<META_NS::IObject>(scene->FindNode("//ext").GetResult());
    auto cube = interface_pointer_cast<META_NS::IObject>(scene->FindNode("//ext//AnimatedCube").GetResult());
    ASSERT_TRUE(parent);
    ASSERT_TRUE(ext);
    ASSERT_TRUE(cube);

    // Parent has Float=10, no other test attachments
    auto pAtt = findAtt(parent, "ParentAtt");
    ASSERT_TRUE(pAtt);
    EXPECT_EQ(META_NS::Metadata(pAtt).GetProperty<float>("Float")->GetValue(), 10.f);
    EXPECT_FALSE(findAtt(parent, "ExtAtt"));
    EXPECT_FALSE(findAtt(parent, "CubeAtt"));

    // Ext root has Float=20, no other test attachments
    auto eAtt = findAtt(ext, "ExtAtt");
    ASSERT_TRUE(eAtt);
    EXPECT_EQ(META_NS::Metadata(eAtt).GetProperty<float>("Float")->GetValue(), 20.f);
    EXPECT_FALSE(findAtt(ext, "ParentAtt"));
    EXPECT_FALSE(findAtt(ext, "CubeAtt"));

    // AnimatedCube has Float=30, no other test attachments
    auto cAtt = findAtt(cube, "CubeAtt");
    ASSERT_TRUE(cAtt);
    EXPECT_EQ(META_NS::Metadata(cAtt).GetProperty<float>("Float")->GetValue(), 30.f);
    EXPECT_FALSE(findAtt(cube, "ParentAtt"));
    EXPECT_FALSE(findAtt(cube, "ExtAtt"));
}

/**
 * @tc.name: SceneOverrideAttachmentPropertyViaPath
 * @tc.desc: Scene overrides a template attachment's property using ! path syntax.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, SceneOverrideAttachmentPropertyViaPath, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_nt_att_override.json");
    ASSERT_TRUE(scene);

    auto ext = scene->FindNode("//ext").GetResult();
    ASSERT_TRUE(ext);

    // Find the TestAttachment on ext
    META_NS::IObject::Ptr testAtt;
    for (auto&& a : interface_cast<META_NS::IAttach>(ext)->GetAttachments()) {
        if (META_NS::Metadata(a).GetProperty<float>("Float")) {
            testAtt = a;
            break;
        }
    }
    ASSERT_TRUE(testAtt);

    auto floatProp = META_NS::Metadata(testAtt).GetProperty<float>("Float");
    ASSERT_TRUE(floatProp);
    EXPECT_EQ(floatProp->GetValue(), 99.f);
    EXPECT_EQ(floatProp->GetDefaultValue(), 1.f);
    EXPECT_FALSE(floatProp->IsDefaultValue());
}

/**
 * @tc.name: SceneOverrideAttachmentPropertyWithForwardObjectRef
 * @tc.desc: Scene overrides a template attachment property with an objectRef pointing
 *           to a node declared later in the JSON. The override goes through
 *           ImportProperty(IProperty&), which must defer the objectRef resolution
 *           until the hierarchy is fully built.
 * @tc.type: FUNC
 */
UNIT_TEST_F(
    API_NodeTemplateImportTest, SceneOverrideAttachmentPropertyWithForwardObjectRef, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_nt_att_override_objref_forward.json");
    ASSERT_TRUE(scene);

    auto ext = scene->FindNode("//ext").GetResult();
    ASSERT_TRUE(ext);

    auto target = scene->FindNode("//target").GetResult();
    ASSERT_TRUE(target);

    META_NS::IObject::Ptr testAtt;
    for (auto&& a : interface_cast<META_NS::IAttach>(ext)->GetAttachments()) {
        if (META_NS::Metadata(a).GetProperty<float>("Float")) {
            testAtt = a;
            break;
        }
    }
    ASSERT_TRUE(testAtt);

    auto nodeRefProp = META_NS::Metadata(testAtt).GetProperty<INode::WeakPtr>("NodeRef");
    ASSERT_TRUE(nodeRefProp);

    // Forward reference: target is declared after ext, so the override objectRef
    // must be deferred until the hierarchy is built.
    auto locked = nodeRefProp->GetValue().lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(locked, target);
}

/**
 * @tc.name: SceneOverrideChildAttachmentPropertyViaPath
 * @tc.desc: Scene overrides a child node's attachment property using chained child+attachment+property path.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, SceneOverrideChildAttachmentPropertyViaPath, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_nt_child_att_override.json");
    ASSERT_TRUE(scene);

    auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);

    // Find the ChildAtt attachment on AnimatedCube
    META_NS::IObject::Ptr childAtt;
    for (auto&& a : interface_cast<META_NS::IAttach>(cube)->GetAttachments()) {
        if (META_NS::Metadata(a).GetProperty<float>("Float")) {
            childAtt = a;
            break;
        }
    }
    ASSERT_TRUE(childAtt);

    auto floatProp = META_NS::Metadata(childAtt).GetProperty<float>("Float");
    ASSERT_TRUE(floatProp);
    EXPECT_EQ(floatProp->GetValue(), 55.f);
    EXPECT_EQ(floatProp->GetDefaultValue(), 3.f);
    EXPECT_FALSE(floatProp->IsDefaultValue());
}

/**
 * @tc.name: NestedNodeTemplateWithDeepAttachment
 * @tc.desc: Nested node templates creating a 3-level hierarchy with attachment on the deepest node.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, NestedNodeTemplateWithDeepAttachment, testing::ext::TestSize.Level1)
{
    auto findTestAtt = [](const META_NS::IObject::Ptr& obj) -> META_NS::IObject::Ptr {
        auto attach = interface_cast<META_NS::IAttach>(obj);
        if (!attach) {
            return nullptr;
        }
        for (auto&& a : attach->GetAttachments()) {
            if (META_NS::Metadata(a).GetProperty<float>("Float")) {
                return a;
            }
        }
        return nullptr;
    };

    auto templ = LoadTestTemplate("test://import/node_template/node_template_nested_outer.json");
    ASSERT_TRUE(templ);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(templ).GetResult();
    ASSERT_TRUE(n);

    // The outer's result (n) is the ext node itself — it should NOT have the TestAttachment
    EXPECT_FALSE(findTestAtt(interface_pointer_cast<META_NS::IObject>(n)));

    // AnimatedCube SHOULD have it with Float=42.0
    auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);
    auto att = findTestAtt(interface_pointer_cast<META_NS::IObject>(cube));
    ASSERT_TRUE(att);
    auto floatProp = META_NS::Metadata(att).GetProperty<float>("Float");
    ASSERT_TRUE(floatProp);
    EXPECT_EQ(floatProp->GetValue(), 42.f);
}

/**
 * @tc.name: NestedTemplateOverrideDeepAttachmentProperty
 * @tc.desc: Nested template uses override to modify an inner template's attachment property via deep path.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, NestedTemplateOverrideDeepAttachmentProperty, testing::ext::TestSize.Level1)
{
    auto templ = LoadTestTemplate("test://import/node_template/node_template_nested_att_override.json");
    ASSERT_TRUE(templ);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(templ).GetResult();
    ASSERT_TRUE(n);

    auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);

    // Find the ChildAtt attachment on AnimatedCube
    META_NS::IObject::Ptr childAtt;
    for (auto&& a : interface_cast<META_NS::IAttach>(cube)->GetAttachments()) {
        if (META_NS::Metadata(a).GetProperty<float>("Float")) {
            childAtt = a;
            break;
        }
    }
    ASSERT_TRUE(childAtt);

    // Inner template set Float default=3.0, outer override changed it to 77.0
    auto floatProp = META_NS::Metadata(childAtt).GetProperty<float>("Float");
    ASSERT_TRUE(floatProp);
    EXPECT_EQ(floatProp->GetValue(), 77.f);
}

/**
 * @tc.name: NodeTemplateMissingIndex
 * @tc.desc: A nodeTemplate that references a non-existent index file fails the import with
 *           an explicit "Failed to open file" diagnostic — pinning the file-open error path.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, NodeTemplateMissingIndex, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/node_template/node_template_missing_index.json", {});
    EXPECT_FALSE(res);
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "Failed to open file");
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "this_file_does_not_exist.index");
}

/**
 * @tc.name: LoadNodeTemplateResourceViaIndex
 * @tc.desc: A node template declared as a resource-index entry is loaded lazily through the
 *           resource manager (NodeTemplateResourceType::LoadResource) and can be instantiated.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, LoadNodeTemplateResourceViaIndex, testing::ext::TestSize.Level1)
{
    constexpr BASE_NS::Math::Vec3 TEMPLATE_POS{1.0f, 2.0f, 3.0f};
    constexpr BASE_NS::Math::Vec3 TEMPLATE_SCALE{4.0f, 5.0f, 6.0f};

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    SCENE_IMP_NS::ImportParameters params;
    params.scene = scene;
    auto res = imp->Import("test://import/node_template/index_node_template.json", params);
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto resource = rman->GetResource({CORE_NS::ResourceId("ext-node-template"), scene});
    ASSERT_TRUE(resource);
    auto templ = interface_pointer_cast<META_NS::IObjectTemplate>(resource);
    ASSERT_TRUE(templ);

    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);
    auto n = root->ImportTemplate(templ).GetResult();
    ASSERT_TRUE(n);

    auto cube = scene->FindNode("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);
    EXPECT_EQ(cube->Position()->GetValue(), TEMPLATE_POS);
    EXPECT_EQ(cube->Scale()->GetValue(), TEMPLATE_SCALE);
}

/**
 * @tc.name: NodeTemplateResourceCachedViaIndex
 * @tc.desc: Requesting the same node template resource id twice returns the same cached
 *           instance — caching is owned by the resource manager, not the loader.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, NodeTemplateResourceCachedViaIndex, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    SCENE_IMP_NS::ImportParameters params;
    params.scene = scene;
    auto res = imp->Import("test://import/node_template/index_node_template.json", params);
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto r1 = rman->GetResource({CORE_NS::ResourceId("ext-node-template"), scene});
    auto r2 = rman->GetResource({CORE_NS::ResourceId("ext-node-template"), scene});
    ASSERT_TRUE(r1);
    ASSERT_TRUE(r2);
    EXPECT_EQ(r1, r2);
}

/**
 * @tc.name: ExternalNodeReferencesNodeTemplateResource
 * @tc.desc: An external scene node referencing a node template by resourceId resolves it
 *           through the resource manager and instantiates the external GLTF subtree.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, ExternalNodeReferencesNodeTemplateResource, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_nt_resource.json");
    ASSERT_TRUE(scene);

    // The outer external entry's "name" ("ext-instance") renames the template's root
    // node on instantiation, overriding the template-authored "ext".
    auto cube = scene->FindNode("//ext-instance//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);
    EXPECT_EQ(cube->Position()->GetValue(), (BASE_NS::Math::Vec3{1.f, 2.f, 3.f}));
    EXPECT_EQ(cube->Scale()->GetValue(), (BASE_NS::Math::Vec3{4.f, 5.f, 6.f}));
}

/**
 * @tc.name: TypedResourceIdOverrideRemapsPrimaryGroup
 * @tc.desc: A property override whose value is the typed `{type:"resourceId", name, group}`
 *           form must apply the template's primaryGroup → runtime-group remap, just like the
 *           helper-parsed `GetOptResourceId` path. Regression test for
 *           ImportResourceId::Import not consulting FindResourceGroupOverride.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_NodeTemplateImportTest, TypedResourceIdOverrideRemapsPrimaryGroup, testing::ext::TestSize.Level1)
{
    auto templ = LoadTestTemplate("test://import/node_template/node_template_resid_override.json");
    ASSERT_TRUE(templ);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    constexpr auto RUNTIME_GROUP = "runtime-group";
    auto n = root->ImportTemplate(templ, RUNTIME_GROUP).GetResult();
    ASSERT_TRUE(n);

    // ImportResources moved the template's overrideMat from tmpl-group to RUNTIME_GROUP.
    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);
    auto overrideMat = interface_pointer_cast<IMaterial>(
        rman->GetResource({CORE_NS::ResourceId{"overrideMat", RUNTIME_GROUP}, scene}));
    ASSERT_TRUE(overrideMat);

    // The override authored against group="tmpl-group" must resolve to that relocated resource.
    // If ImportResourceId::Import skips FindResourceGroupOverride, the lookup misses and the
    // submesh material stays unset.
    auto mesh = scene->FindNode<IMesh>("//meshNode").GetResult();
    ASSERT_TRUE(mesh);
    auto subs = mesh->SubMeshes()->GetValue();
    ASSERT_FALSE(subs.empty());
    auto mat = subs[0]->Material()->GetValue();
    ASSERT_TRUE(mat);
    EXPECT_EQ(mat, overrideMat);
}

}  // namespace UTest
SCENE_END_NAMESPACE()
