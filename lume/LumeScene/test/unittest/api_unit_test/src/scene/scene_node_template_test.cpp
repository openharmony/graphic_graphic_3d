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

// clang-format off
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
// clang-format on

#include <scene/api/scene.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/ext/scene_utils.h>
#include <scene/interface/intf_create_mesh.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_raycast.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/intf_text.h>
#include <scene/interface/resource/intf_render_resource_manager.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/resource/util.h>

#include <core/resources/intf_resource_manager.h>

#include <meta/api/metadata_util.h>
#include <meta/api/object.h>
#include <meta/ext/attachment/behavior.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/resource/intf_resource.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_SceneNodeTemplateTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        ScenePluginTest::SetUp();
        // construct scene manager which registers the resources types
        renderman_ =
            META_NS::GetObjectRegistry().Create<IRenderResourceManager>(ClassId::RenderResourceManager, params);

        // Get access to protocols defined in engine file manager
        resources->SetFileManager(CORE_NS::IFileManager::Ptr(&GetTestEnv()->engine->GetFileManager()));

        ASSERT_TRUE(renderman_);
    }

    void TearDown() override
    {
        ScenePluginTest::TearDown();
    }

    IImage::Ptr CreateTestBitmap()
    {
        auto bitmap = renderman_->LoadImage("test://images/logo.png").GetResult();
        EXPECT_TRUE(bitmap);
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(bitmap)) {
            i->SetResourceId("image");
        }
        if (bitmap) {
            EXPECT_TRUE(resources->AddResource("image", ClassId::ImageResource.Id().ToUid(), "test://images/logo.png"));
        }
        return bitmap;
    }

    IShader::Ptr CreateTestShader()
    {
        auto shader = renderman_->LoadShader("test://shaders/test.shader").GetResult();
        EXPECT_TRUE(shader);
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(shader)) {
            i->SetResourceId("shader");
        }
        if (shader) {
            EXPECT_TRUE(
                resources->AddResource("shader", ClassId::ShaderResource.Id().ToUid(), "test://shaders/test.shader"));
        }
        return shader;
    }

    IScene::Ptr CreateTestNodes(BASE_NS::string name, META_NS::IObjectTemplate::Ptr embedded = nullptr)
    {
        auto scene = CreateEmptyScene();
        if (!scene) {
            return nullptr;
        }
        auto n1 = scene->CreateNode("//" + name).GetResult();
        if (!n1) {
            return nullptr;
        }
        auto n2 = scene->CreateNode("//" + name + "//some").GetResult();
        auto imp = interface_pointer_cast<INodeImport>(n2);
        if (!imp) {
            return nullptr;
        }
        if (embedded && !imp->ImportTemplate(embedded).GetResult()) {
            return nullptr;
        }
        return scene;
    }

    META_NS::IObjectTemplate::Ptr CreateTestTemplate(BASE_NS::string name, INode::Ptr node)
    {
        auto templ = CreateNodeTemplate(name, node);
        if (!templ) {
            return nullptr;
        }
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(templ)) {
            i->SetResourceId("template_" + name);
        }
        EXPECT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(templ), "app://node_template_" + name + ".template"));
        return templ;
    }

    META_NS::IObjectTemplate::Ptr CreateTestNodeTemplate(
        BASE_NS::string name, META_NS::IObjectTemplate::Ptr embedded = nullptr)
    {
        auto scene = CreateTestNodes(name, embedded);
        if (!scene) {
            return nullptr;
        }
        auto n1 = scene->FindNode("//" + name).GetResult();
        if (!n1) {
            return nullptr;
        }
        return CreateTestTemplate(name, n1);
    }

private:
    IRenderResourceManager::Ptr renderman_;
};

/**
 * @tc.name: NodeTemplate
 * @tc.desc: Tests for Node Template. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, NodeTemplate, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto n1 = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(n1);
        auto n2 = scene->CreateNode("//test/some").GetResult();
        ASSERT_TRUE(n2);
        n2->Scale()->SetValue({ 2.0, 2.0, 2.0 });
        auto n3 = scene->CreateNode("//test/other").GetResult();
        ASSERT_TRUE(n3);
        n3->Position()->SetValue({ 3.0, 3.0, 3.0 });

        auto templ = CreateNodeTemplate("mynodes", n1);
        ASSERT_TRUE(templ);

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(templ), "app://node_template.template"));

        ASSERT_EQ(resources->Export("app://node_template.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }

    {
        ASSERT_EQ(resources->Import("app://node_template.resources"), CORE_NS::IResourceManager::Result::OK);

        auto res = resources->GetResource("mynodes");
        ASSERT_TRUE(res);

        auto templ = interface_pointer_cast<META_NS::IObjectTemplate>(res);
        ASSERT_TRUE(templ);

        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(root);

        auto n = root->ImportTemplate(templ).GetResult();
        ASSERT_TRUE(n);
        auto n1 = scene->FindNode("//test").GetResult();
        ASSERT_TRUE(n1);
        EXPECT_EQ(n1, n);
        auto n2 = scene->FindNode("//test/some").GetResult();
        ASSERT_TRUE(n2);
        auto n3 = scene->FindNode("//test/other").GetResult();
        ASSERT_TRUE(n3);

        EXPECT_EQ(n2->Scale()->GetValue(), BASE_NS::Math::Vec3(2.0, 2.0, 2.0));
        EXPECT_EQ(n3->Position()->GetValue(), BASE_NS::Math::Vec3(3.0, 3.0, 3.0));

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId("scene");
        }
        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://node_template.scene"));

        ASSERT_EQ(resources->Export("app://node_template.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }

    ASSERT_EQ(resources->Import("app://node_template.resources"), CORE_NS::IResourceManager::Result::OK);
    auto res = interface_pointer_cast<IScene>(resources->GetResource("scene"));
    ASSERT_TRUE(res);

    auto n1 = res->FindNode("//test").GetResult();
    ASSERT_TRUE(n1);
    auto n2 = res->FindNode("//test/some").GetResult();
    ASSERT_TRUE(n2);
    auto n3 = res->FindNode("//test/other").GetResult();
    ASSERT_TRUE(n3);

    EXPECT_EQ(n2->Scale()->GetValue(), BASE_NS::Math::Vec3(2.0, 2.0, 2.0));
    EXPECT_EQ(n3->Position()->GetValue(), BASE_NS::Math::Vec3(3.0, 3.0, 3.0));

    EXPECT_TRUE(GetExternalNodeAttachment(n1));
}

/**
 * @tc.name: NodeTemplateExternal
 * @tc.desc: Tests for Node Template External. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, NodeTemplateExternal, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = scene->GetRootNode().GetResult();
        ASSERT_TRUE(root);

        {
            ASSERT_TRUE(resources->AddResource("scene2", ::SCENE_NS::ClassId::GltfSceneResource.Id().ToUid(),
                "test://AnimatedCube/AnimatedCube.gltf"));

            INode::Ptr node;
            if (auto i = interface_cast<INodeImport>(root)) {
                node = i->ImportChildScene(interface_pointer_cast<IScene>(resources->GetResource("scene2")), "ext")
                           .GetResult();
                ASSERT_TRUE(node);
            }

            META_NS::Object obj = META_NS::CreateObjectInstance<META_NS::IObject>();
            META_NS::SetName(obj, "OtherTestAttachment");
            interface_cast<META_NS::IAttach>(node)->Attach(obj.GetPtr());
        }

        auto n = scene->FindNode<IMesh>("//ext//AnimatedCube").GetResult();
        ASSERT_TRUE(n);

        {
            META_NS::Object obj = META_NS::CreateObjectInstance<META_NS::IObject>();
            META_NS::SetName(obj, "TestAttachment");
            interface_cast<META_NS::IAttach>(n)->Attach(obj.GetPtr());
        }

        auto subs = n->SubMeshes()->GetValue();
        ASSERT_TRUE(!subs.empty());

        { // material
            auto material = scene->CreateObject<IMaterial>(::SCENE_NS::ClassId::Material).GetResult();
            ASSERT_TRUE(material);

            META_NS::SetName(material, "hipshops");

            if (auto r = interface_cast<CORE_NS::ISetResourceId>(material)) {
                r->SetResourceId(CORE_NS::ResourceId { "material", "app://nt_ext_test.scene" });
            }

            material->LightingFlags()->SetValue(LightingFlags::SHADOW_CASTER_BIT);
            material->MaterialShader()->SetValue(CreateTestShader());
            auto t1 = material->Textures()->GetValueAt(0);
            ASSERT_TRUE(t1);
            t1->Image()->SetValue(CreateTestBitmap());
            t1->Rotation()->SetValue(1.0f);

            auto inputs = material->CustomProperties()->GetValue();
            ASSERT_TRUE(inputs);
            auto prop = inputs->GetProperty<BASE_NS::Math::Vec4>("v4_");
            ASSERT_TRUE(prop);
            prop->SetValue({ 1.0, 1.0, 1.0, 1.0 });

            ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(material), ""));
            subs[0]->Material()->SetValue(material);
        }

        /*     if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
                 i->SetResourceId("app://nt_ext_test.scene");
             }

             ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene)));
     */
        auto extNode = scene->FindNode("//ext").GetResult();
        ASSERT_TRUE(extNode);

        auto templ = CreateNodeTemplate("mynodes", extNode);
        ASSERT_TRUE(templ);

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(templ), "app://nt_ext_test.template"));

        ASSERT_EQ(resources->Export("app://nt_ext_test.resources"), CORE_NS::IResourceManager::Result::OK);

        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://nt_ext_test.resources"), CORE_NS::IResourceManager::Result::OK);

    auto res = resources->GetResource("mynodes");
    ASSERT_TRUE(res);

    auto templ = interface_pointer_cast<META_NS::IObjectTemplate>(res);
    ASSERT_TRUE(templ);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    ASSERT_TRUE(root->ImportTemplate(templ).GetResult());

    auto extNode = scene->FindNode("//ext").GetResult();
    ASSERT_TRUE(extNode);

    auto n = scene->FindNode<IMesh>("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(n);

    auto att1 = interface_cast<META_NS::IAttach>(extNode)->GetAttachmentContainer()->FindByName("OtherTestAttachment");
    ASSERT_TRUE(att1);

    auto att2 = interface_cast<META_NS::IAttach>(n)->GetAttachmentContainer()->FindByName("TestAttachment");
    ASSERT_TRUE(att2);

    auto subs = n->SubMeshes()->GetValue();
    ASSERT_TRUE(!subs.empty());

    auto firstSub = subs[0];
    auto mat = firstSub->Material()->GetValue();
    ASSERT_TRUE(mat);
    EXPECT_EQ(META_NS::GetName(mat), "hipshops");

    EXPECT_EQ(mat->LightingFlags()->GetValue(), LightingFlags::SHADOW_CASTER_BIT);
    auto shader = mat->MaterialShader()->GetValue();
    {
        auto i = interface_cast<CORE_NS::IResource>(shader);
        ASSERT_TRUE(i);
        EXPECT_EQ(i->GetResourceId(), CORE_NS::ResourceId { "shader" });
    }

    {
        auto inputs = mat->CustomProperties()->GetValue();
        ASSERT_TRUE(inputs);
        auto prop = inputs->GetProperty<BASE_NS::Math::Vec4>("v4_");
        ASSERT_TRUE(prop);
        EXPECT_EQ(prop->GetValue(), (BASE_NS::Math::Vec4 { 1.0, 1.0, 1.0, 1.0 }));
    }

    auto t1 = mat->Textures()->GetValueAt(0);
    ASSERT_TRUE(t1);
    EXPECT_EQ(t1->Rotation()->GetValue(), 1.0f);
    auto bitmap = t1->Image()->GetValue();
    {
        auto i = interface_cast<CORE_NS::IResource>(bitmap);
        ASSERT_TRUE(i);
        EXPECT_EQ(i->GetResourceId(), CORE_NS::ResourceId { "image" });
    }

    {
        auto acc = interface_cast<IEcsObjectAccess>(firstSub);
        ASSERT_TRUE(acc);
        auto ecsobj = acc->GetEcsObject();
        ASSERT_TRUE(ecsobj);
        EXPECT_TRUE(CORE_NS::EntityUtil::IsValid(ecsobj->GetEntity()));
    }
    {
        auto acc = interface_cast<IEcsObjectAccess>(t1);
        ASSERT_TRUE(acc);
        auto ecsobj = acc->GetEcsObject();
        ASSERT_TRUE(ecsobj);
        EXPECT_TRUE(CORE_NS::EntityUtil::IsValid(ecsobj->GetEntity()));
    }
}

/**
 * @tc.name: NodeTemplateRecursive
 * @tc.desc: Tests for Node Template Recursive. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, NodeTemplateRecursive, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(root);

        auto t3 = CreateTestNodeTemplate("3");
        ASSERT_TRUE(t3);
        auto t2 = CreateTestNodeTemplate("2", t3);
        ASSERT_TRUE(t2);
        auto t1 = CreateTestNodeTemplate("1", t2);
        ASSERT_TRUE(t1);

        auto n = root->ImportTemplate(t1).GetResult();
        ASSERT_TRUE(n);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId("scene");
        }

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://node_template_rec.scene"));

        ASSERT_EQ(resources->Export("app://node_template_rec.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }

    ASSERT_EQ(resources->Import("app://node_template_rec.resources"), CORE_NS::IResourceManager::Result::OK);
    auto res = interface_pointer_cast<IScene>(resources->GetResource("scene"));
    ASSERT_TRUE(res);

    ASSERT_TRUE(res->FindNode("//1").GetResult());
    ASSERT_TRUE(res->FindNode("//1/some").GetResult());
    ASSERT_TRUE(res->FindNode("//1/some/2").GetResult());
    ASSERT_TRUE(res->FindNode("//1/some/2/some").GetResult());
    ASSERT_TRUE(res->FindNode("//1/some/2/some/3").GetResult());
    ASSERT_TRUE(res->FindNode("//1/some/2/some/3/some").GetResult());

    // PrintHierarchy(res->GetRootNode().GetResult());
}

/**
 * @tc.name: NodeTemplateUpdate
 * @tc.desc: Tests for Node Template Update. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, NodeTemplateUpdate, testing::ext::TestSize.Level1)
{
    META_NS::IObjectTemplate::Ptr templ1;
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto n1 = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(n1);
        auto n2 = scene->CreateNode("//test/some").GetResult();
        ASSERT_TRUE(n2);
        n2->Scale()->SetValue({ 2.0, 2.0, 2.0 });
        n2->Rotation()->SetValue({ 2.0, 2.0, 2.0, 2.0 });
        auto n3 = scene->CreateNode("//test/other").GetResult();
        ASSERT_TRUE(n3);

        META_NS::Object obj = META_NS::CreateObjectInstance<META_NS::IObject>();
        META_NS::SetName(obj, "MyAttachment");
        META_NS::Metadata md(obj);
        md.AddProperty<BASE_NS::string>("test", "myvalue", META_NS::ObjectFlagBits::SERIALIZE);
        interface_cast<META_NS::IAttach>(n2)->Attach(obj.GetPtr());

        templ1 = CreateNodeTemplate("mynodes", n1);
        ASSERT_TRUE(templ1);

        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(templ1), "app://node_template_update_1.template"));
        // so we can check the files
        ASSERT_EQ(resources->Export("app://node_template_update.resources"), CORE_NS::IResourceManager::Result::OK);
    }
    META_NS::IObjectTemplate::Ptr templ2;
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto n1 = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(n1);
        n1->Scale()->SetValue({ 4.0, 4.0, 4.0 });
        auto n2 = scene->CreateNode("//test/some").GetResult();
        ASSERT_TRUE(n2);
        n2->Rotation()->SetValue({ 3.0, 3.0, 3.0, 3.0 });

        META_NS::Object obj = META_NS::CreateObjectInstance<META_NS::IObject>();
        META_NS::SetName(obj, "MyAttachment");
        META_NS::Metadata md(obj);
        md.AddProperty<BASE_NS::string>("test", "myvalue2", META_NS::ObjectFlagBits::SERIALIZE);
        interface_cast<META_NS::IAttach>(n2)->Attach(obj.GetPtr());

        templ2 = CreateNodeTemplate("mynodes", n1);
        ASSERT_TRUE(templ2);

        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(templ2), "app://node_template_update_2.template"));
        // so we can check the files
        ASSERT_EQ(resources->Export("app://node_template_update.resources"), CORE_NS::IResourceManager::Result::OK);
    }
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(root);

        auto n = root->ImportTemplate(templ1).GetResult();
        ASSERT_TRUE(n);

        auto n2 = scene->FindNode("//test/some").GetResult();
        ASSERT_TRUE(n2);

        auto myatt = interface_cast<META_NS::IAttach>(n2)->GetAttachmentContainer()->FindByName("MyAttachment");
        ASSERT_TRUE(myatt);
        META_NS::Metadata md(myatt);
        auto prop = md.GetProperty<BASE_NS::string>("test");
        ASSERT_TRUE(prop);
        prop->SetValue("hipshops");

        n2->Position()->SetValue({ 3.0, 3.0, 3.0 });
        n2->Rotation()->SetValue({ 4.0, 4.0, 4.0, 4.0 });

        {
            META_NS::Object obj = META_NS::CreateObjectInstance<META_NS::IObject>();
            META_NS::SetName(obj, "Other");
            interface_cast<META_NS::IAttach>(n2)->Attach(obj.GetPtr());
        }
        {
            META_NS::Object obj = META_NS::CreateObjectInstance<META_NS::IObject>();
            META_NS::SetName(obj, "Something");
            interface_cast<META_NS::IAttach>(n)->Attach(obj.GetPtr());
        }

        n = interface_pointer_cast<INode>(templ2->Update(*interface_cast<META_NS::IObject>(n), nullptr));

        EXPECT_TRUE(GetExternalNodeAttachment(n));
        EXPECT_TRUE(interface_cast<META_NS::IAttach>(n)->GetAttachmentContainer()->FindByName("Something"));

        EXPECT_FALSE(scene->FindNode("//test/other").GetResult());
        auto tn1 = scene->FindNode("//test").GetResult();
        ASSERT_TRUE(tn1);
        EXPECT_EQ(tn1->Scale()->GetValue(), (BASE_NS::Math::Vec3 { 4.0, 4.0, 4.0 }));
        auto tn2 = scene->FindNode("//test/some").GetResult();
        ASSERT_TRUE(tn2);
        EXPECT_EQ(tn2->Position()->GetValue(), (BASE_NS::Math::Vec3 { 3.0, 3.0, 3.0 }));
        EXPECT_EQ(tn2->Scale()->GetValue(), (BASE_NS::Math::Vec3 { 1.0, 1.0, 1.0 }));
        EXPECT_EQ(tn2->Rotation()->GetValue(), (BASE_NS::Math::Quat { 4.0, 4.0, 4.0, 4.0 }));

        EXPECT_TRUE(interface_cast<META_NS::IAttach>(tn2)->GetAttachmentContainer()->FindByName("Other"));
        auto tmyatt = interface_cast<META_NS::IAttach>(tn2)->GetAttachmentContainer()->FindByName("MyAttachment");
        ASSERT_TRUE(tmyatt);
        META_NS::Metadata tmd(tmyatt);
        auto tprop = md.GetProperty<BASE_NS::string>("test");
        ASSERT_TRUE(tprop);
        EXPECT_EQ(tprop->GetValue(), "hipshops");
    }
}

/**
 * @tc.name: MultiUpdate
 * @tc.desc: Tests for Multi Update. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, MultiUpdate, testing::ext::TestSize.Level1)
{
    auto t1 = CreateTestNodeTemplate("templ");
    ASSERT_TRUE(t1);

    META_NS::IObjectTemplate::Ptr t2, t3;
    {
        auto scene = CreateTestNodes("templ");
        ASSERT_TRUE(scene);
        auto n = scene->FindNode("//templ").GetResult();
        ASSERT_TRUE(n);

        auto sn = scene->FindNode("//templ/some").GetResult();
        ASSERT_TRUE(sn);
        sn->Scale()->SetValue({ 2.0, 2.0, 2.0 });

        t2 = CreateTestTemplate("templ", n);
    }
    {
        auto scene = CreateTestNodes("templ");
        ASSERT_TRUE(scene);
        auto n = scene->FindNode("//templ").GetResult();
        ASSERT_TRUE(n);

        auto sn = scene->FindNode("//templ/some").GetResult();
        ASSERT_TRUE(sn);
        sn->Scale()->SetValue({ 3.0, 3.0, 3.0 });

        t3 = CreateTestTemplate("templ", n);
    }

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(t1).GetResult();
    ASSERT_TRUE(n);

    {
        n = interface_pointer_cast<INode>(t1->Update(*interface_cast<META_NS::IObject>(n), nullptr));
        ASSERT_TRUE(n);

        EXPECT_TRUE(GetExternalNodeAttachment(n));

        auto sn = scene->FindNode("//templ/some").GetResult();
        ASSERT_TRUE(sn);

        EXPECT_EQ(sn->Scale()->GetValue(), (BASE_NS::Math::Vec3 { 1.0, 1.0, 1.0 }));
    }
    {
        n = interface_pointer_cast<INode>(t2->Update(*interface_cast<META_NS::IObject>(n), nullptr));
        ASSERT_TRUE(n);

        EXPECT_TRUE(GetExternalNodeAttachment(n));

        auto sn = scene->FindNode("//templ/some").GetResult();
        ASSERT_TRUE(sn);

        EXPECT_EQ(sn->Scale()->GetValue(), (BASE_NS::Math::Vec3 { 2.0, 2.0, 2.0 }));
    }
    {
        n = interface_pointer_cast<INode>(t3->Update(*interface_cast<META_NS::IObject>(n), nullptr));
        ASSERT_TRUE(n);

        EXPECT_TRUE(GetExternalNodeAttachment(n));

        auto sn = scene->FindNode("//templ/some").GetResult();
        ASSERT_TRUE(sn);

        EXPECT_EQ(sn->Scale()->GetValue(), (BASE_NS::Math::Vec3 { 3.0, 3.0, 3.0 }));
    }
    {
        n = interface_pointer_cast<INode>(t1->Update(*interface_cast<META_NS::IObject>(n), nullptr));
        ASSERT_TRUE(n);

        EXPECT_TRUE(GetExternalNodeAttachment(n));

        auto sn = scene->FindNode("//templ/some").GetResult();
        ASSERT_TRUE(sn);

        EXPECT_EQ(sn->Scale()->GetValue(), (BASE_NS::Math::Vec3 { 1.0, 1.0, 1.0 }));
    }
}

} // namespace UTest
SCENE_END_NAMESPACE()
