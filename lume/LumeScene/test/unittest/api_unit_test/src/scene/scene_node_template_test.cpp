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

#include <scene/api/node.h>
#include <scene/api/resource_utils.h>
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
#include <scene/interface/resource/intf_render_resource_manager.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/resource/util.h>
#include <scene_metadata_importer/interface/util.h>

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

    IImage::Ptr CreateTestBitmap(CORE_NS::ResourceIdContext id = {})
    {
        auto bitmap = renderman_->LoadImage("test://images/logo.png").GetResult();
        EXPECT_TRUE(bitmap);
        if (!id.id.IsValid()) {
            id = CORE_NS::ResourceIdContext{"image"};
        }
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(bitmap)) {
            i->SetResourceId(id);
        }
        if (bitmap) {
            EXPECT_TRUE(resources->AddResource(id, ClassId::ImageResource.Id().ToUid(), "test://images/logo.png"));
        }
        return bitmap;
    }

    IShader::Ptr CreateTestShader(CORE_NS::ResourceIdContext id = {})
    {
        auto shader = renderman_->LoadShader("test://shaders/test.shader").GetResult();
        EXPECT_TRUE(shader);
        if (!id.id.IsValid()) {
            id = CORE_NS::ResourceIdContext{"shader"};
        }
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(shader)) {
            i->SetResourceId(id);
        }
        if (shader) {
            EXPECT_TRUE(resources->AddResource(id, ClassId::ShaderResource.Id().ToUid(), "test://shaders/test.shader"));
        }
        return shader;
    }

    IMaterial::Ptr CreateTestMaterial(CORE_NS::ResourceId id)
    {
        auto material = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
        EXPECT_TRUE(material);
        if (auto r = interface_cast<CORE_NS::ISetResourceId>(material)) {
            r->SetResourceId({id, scene});
        }
        material->LightingFlags()->SetValue(LightingFlags::SHADOW_CASTER_BIT);
        material->MaterialShader()->SetValue(CreateTestShader(CORE_NS::ResourceIdContext{{"shader", id.group}}));
        material->Textures()->GetValue()[0]->Factor()->SetValue({0, 1, 0, 0});
        META_NS::SetName(material, id.name);
        EXPECT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(material), ""));
        return material;
    }

    IMaterial::Ptr CreateTestDerivedMaterial(CORE_NS::ResourceId id)
    {
        auto material = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
        EXPECT_TRUE(material);
        if (auto r = interface_cast<CORE_NS::ISetResourceId>(material)) {
            r->SetResourceId({id, scene});
        }
        material->LightingFlags()->SetValue(LightingFlags::SHADOW_CASTER_BIT);
        // currently resource template cannot point to a resource from node template group
        material->MaterialShader()->SetValue(CreateTestShader(CORE_NS::ResourceIdContext{{"shader"}}));

        auto mres = interface_pointer_cast<META_NS::IDerivedFromTemplate>(material);
        if (mres) {
            auto mt = mres->CreateTemplate();
            if (auto r = interface_cast<CORE_NS::ISetResourceId>(mt)) {
                r->SetResourceId(CORE_NS::ResourceIdContext{{id.name + ".rt", id.group}});
            }
            mres->SetTemplate(mt);

            META_NS::SetName(material, id.name);
            EXPECT_TRUE(resources->AddResource(mt, "app://" + id.name + ".rt"));
            EXPECT_EQ(resources->ExportResourcePayload(mt), CORE_NS::IResourceManager::Result::OK);
            EXPECT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(material), ""));
        }
        return material;
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
        auto n2 = scene->CreateNode("//" + name + "/some").GetResult();
        auto imp = interface_pointer_cast<INodeImport>(n2);
        if (!imp) {
            return nullptr;
        }
        auto group = scene->GetResourceGroups().PrimaryGroup();
        // add cube external node
        {
            EXPECT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{{"cube", group}},
                ::SCENE_NS::ClassId::GltfSceneResource.Id().ToUid(),
                "test://AnimatedCube/AnimatedCube.gltf"));

            if (auto i = interface_cast<INodeImport>(n1)) {
                auto node = i->ImportChildScene(interface_pointer_cast<IScene>(resources->GetResource(
                                                    CORE_NS::ResourceIdContext{{"cube", group}})),
                                 "cube")
                                .GetResult();
                EXPECT_TRUE(node);
                if (auto n = scene->FindNode<IMesh>("//" + name + "/cube//AnimatedCube").GetResult()) {
                    auto subs = n->SubMeshes()->GetValue();
                    EXPECT_FALSE(subs.empty());
                    if (!subs.empty()) {
                        if (embedded) {
                            subs[0]->Material()->SetValue(CreateTestDerivedMaterial({"material_" + name, group}));
                        } else {
                            subs[0]->Material()->SetValue(CreateTestMaterial({"material_" + name, group}));
                        }
                    }
                }
            }
        }
        if (embedded && !imp->ImportTemplate(embedded).GetResult()) {
            return nullptr;
        }
        return scene;
    }

    META_NS::IObjectTemplate::Ptr CreateTestTemplate(BASE_NS::string name, INode::Ptr node)
    {
        auto templ = CreateNodeTemplate(CORE_NS::ResourceIdContext{name}, node);
        if (!templ) {
            return nullptr;
        }
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(templ)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"template_" + name});
        }

        EXPECT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(templ), "app://node_template_" + name + ".template"));
        EXPECT_EQ(resources->ExportResourcePayload(interface_pointer_cast<CORE_NS::IResource>(templ)),
            CORE_NS::IResourceManager::Result::OK);
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

    IScene::Ptr CreateSceneWithNewContext()
    {
        auto c = META_NS::GetObjectRegistry().Create<IRenderContext>(ClassId::RenderContext);
        auto resources =
            META_NS::GetObjectRegistry().Create<CORE_NS::IResourceManager>(META_NS::ClassId::FileResourceManager);
        if (!c || !resources) {
            return nullptr;
        }
        resources->SetFileManager(CORE_NS::IFileManager::Ptr(&GetTestEnv()->engine->GetFileManager()));
        for (auto&& i : context->GetResources()->GetResourceTypes()) {
            resources->AddResourceType(i);
        }
        c->Initialize(context->GetRenderer(), context->GetRenderQueue(), context->GetApplicationQueue(), resources);
        auto man = META_NS::GetObjectRegistry().Create<ISceneManager>(ClassId::SceneManager, CreateRenderContextArg(c));
        return man ? man->CreateScene().GetResult() : nullptr;
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
        n2->Scale()->SetValue({2.0, 2.0, 2.0});
        auto n3 = scene->CreateNode("//test/other").GetResult();
        ASSERT_TRUE(n3);
        n3->Position()->SetValue({3.0, 3.0, 3.0});

        // add resource that is not used
        CreateTestMaterial({"material"});

        auto templ = CreateNodeTemplate(CORE_NS::ResourceIdContext{"mynodes"}, n1);
        ASSERT_TRUE(templ);

        resources->RemoveAllResources();

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(templ), "app://node_template.template"));

        ASSERT_EQ(resources->Export("app://node_template.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }

    {
        ASSERT_EQ(resources->Import("app://node_template.resources"), CORE_NS::IResourceManager::Result::OK);

        auto res = resources->GetResource(CORE_NS::ResourceIdContext{"mynodes"});
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

        EXPECT_TRUE(resources->GetResource(CORE_NS::ResourceIdContext{"material", scene}));

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }
        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://node_template.scene"));

        ASSERT_EQ(resources->Export("app://node_template.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }

    ASSERT_EQ(resources->Import("app://node_template.resources"), CORE_NS::IResourceManager::Result::OK);
    auto res = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(res);

    auto n1 = res->FindNode("//test").GetResult();
    ASSERT_TRUE(n1);
    auto n2 = res->FindNode("//test/some").GetResult();
    ASSERT_TRUE(n2);
    auto n3 = res->FindNode("//test/other").GetResult();
    ASSERT_TRUE(n3);

    EXPECT_EQ(n2->Scale()->GetValue(), BASE_NS::Math::Vec3(2.0, 2.0, 2.0));
    EXPECT_EQ(n3->Position()->GetValue(), BASE_NS::Math::Vec3(3.0, 3.0, 3.0));

    EXPECT_TRUE(resources->GetResource(CORE_NS::ResourceIdContext{"material", res}));

    EXPECT_TRUE(GetExternalNodeAttachment(n1));
}

void TestNodeTemplateExternalResult(IScene::Ptr scene, META_NS::IObjectTemplate::Ptr templ)
{
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
        EXPECT_EQ(i->GetResourceId(), CORE_NS::ResourceId{"shader"});
    }

    {
        auto inputs = mat->CustomProperties()->GetValue();
        ASSERT_TRUE(inputs);
        auto prop = inputs->GetProperty<BASE_NS::Math::Vec4>("v4_");
        ASSERT_TRUE(prop);
        EXPECT_EQ(prop->GetValue(), (BASE_NS::Math::Vec4{1.0, 1.0, 1.0, 1.0}));
    }

    auto t1 = mat->Textures()->GetValueAt(0);
    ASSERT_TRUE(t1);
    EXPECT_EQ(t1->Rotation()->GetValue(), 1.0f);
    auto bitmap = t1->Image()->GetValue();
    {
        auto i = interface_cast<CORE_NS::IResource>(bitmap);
        ASSERT_TRUE(i);
        EXPECT_EQ(i->GetResourceId(), CORE_NS::ResourceId{"image"});
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
            ASSERT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"scene2"},
                ::SCENE_NS::ClassId::GltfSceneResource.Id().ToUid(),
                "test://AnimatedCube/AnimatedCube.gltf"));

            INode::Ptr node;
            if (auto i = interface_cast<INodeImport>(root)) {
                node = i->ImportChildScene(interface_pointer_cast<IScene>(
                                               resources->GetResource(CORE_NS::ResourceIdContext{"scene2"})),
                            "ext")
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

        {  // material
            auto material = scene->CreateObject<IMaterial>(::SCENE_NS::ClassId::Material).GetResult();
            ASSERT_TRUE(material);

            META_NS::SetName(material, "hipshops");

            if (auto r = interface_cast<CORE_NS::ISetResourceId>(material)) {
                r->SetResourceId({CORE_NS::ResourceId{"material", "app://nt_ext_test.scene"}, scene});
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
            prop->SetValue({1.0, 1.0, 1.0, 1.0});

            ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(material), ""));
            subs[0]->Material()->SetValue(material);
        }
        auto extNode = scene->FindNode("//ext").GetResult();
        ASSERT_TRUE(extNode);

        auto templ = CreateNodeTemplate(CORE_NS::ResourceIdContext{"mynodes"}, extNode);
        ASSERT_TRUE(templ);

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(templ), "app://nt_ext_test.template"));

        ASSERT_EQ(resources->Export("app://nt_ext_test.resources"), CORE_NS::IResourceManager::Result::OK);

        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://nt_ext_test.resources"), CORE_NS::IResourceManager::Result::OK);

    auto res = resources->GetResource(CORE_NS::ResourceIdContext{"mynodes"});
    ASSERT_TRUE(res);

    auto templ = interface_pointer_cast<META_NS::IObjectTemplate>(res);
    ASSERT_TRUE(templ);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    TestNodeTemplateExternalResult(scene, templ);

    //----- use separate scene and resource manager
    auto newscene = CreateSceneWithNewContext();
    ASSERT_TRUE(newscene);
    TestNodeTemplateExternalResult(newscene, templ);
}

void TestNodeTemplateRecursiveResult(IScene::Ptr scene)
{
    ASSERT_TRUE(scene->FindNode("//1").GetResult());
    ASSERT_TRUE(scene->FindNode("//1/some").GetResult());
    ASSERT_TRUE(scene->FindNode("//1/some/2").GetResult());
    ASSERT_TRUE(scene->FindNode("//1/some/2/some").GetResult());
    ASSERT_TRUE(scene->FindNode("//1/some/2/some/3").GetResult());
    ASSERT_TRUE(scene->FindNode("//1/some/2/some/3/some").GetResult());

    // PrintHierarchy(scene->GetRootNode().GetResult());

    auto c1 = scene->FindNode("//1/cube").GetResult();
    auto c2 = scene->FindNode("//1/some/2/cube").GetResult();
    auto c3 = scene->FindNode("//1/some/2/some/3/cube").GetResult();
    ASSERT_TRUE(c1);
    ASSERT_TRUE(c2);
    ASSERT_TRUE(c3);

    auto checkMesh = [](auto n, BASE_NS::string name) {
        auto subs = n->SubMeshes()->GetValue();
        ASSERT_TRUE(!subs.empty());
        auto mat = subs[0]->Material()->GetValue();
        ASSERT_TRUE(mat);
        EXPECT_EQ(META_NS::GetName(mat), "material_" + name);
        EXPECT_EQ(mat->LightingFlags()->GetValue(), LightingFlags::SHADOW_CASTER_BIT) << "name: " << name.c_str();
        auto shader = mat->MaterialShader()->GetValue();
        {
            auto i = interface_cast<CORE_NS::IResource>(shader);
            ASSERT_TRUE(i);
            EXPECT_EQ(i->GetResourceId().name, "shader")
                << "name: " << name.c_str() << " gid: " << i->GetResourceId().ToString().c_str();
        }
    };

    auto n1 = scene->FindNode<IMesh>("//1/cube//AnimatedCube").GetResult();
    ASSERT_TRUE(n1);
    checkMesh(n1, "1");
    auto n2 = scene->FindNode<IMesh>("//1/some/2/cube//AnimatedCube").GetResult();
    ASSERT_TRUE(n2);
    checkMesh(n2, "2");
    auto n3 = scene->FindNode<IMesh>("//1/some/2/some/3/cube//AnimatedCube").GetResult();
    ASSERT_TRUE(n3);
    checkMesh(n3, "3");
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
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://node_template_rec.scene"));

        // PrintHierarchy(scene->GetRootNode().GetResult());

        ASSERT_EQ(resources->Export("app://node_template_rec.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }

    ASSERT_EQ(resources->Import("app://node_template_rec.resources"), CORE_NS::IResourceManager::Result::OK);
    auto res = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(res);

    TestNodeTemplateRecursiveResult(res);

    //----- use separate scene and resource manager
    {
        SCOPED_TRACE("separate scene");
        auto newscene = CreateSceneWithNewContext();
        ASSERT_TRUE(newscene);

        auto root = interface_pointer_cast<INodeImport>(newscene->GetRootNode().GetResult());
        ASSERT_TRUE(root);
        auto n = root->ImportTemplate(interface_pointer_cast<META_NS::IObjectTemplate>(
                                          resources->GetResource(CORE_NS::ResourceIdContext{"template_1"})))
                     .GetResult();
        ASSERT_TRUE(n);

        TestNodeTemplateRecursiveResult(newscene);

        auto newTempl =
            CreateNodeTemplate(CORE_NS::ResourceIdContext{"new_template"}, newscene->GetRootNode().GetResult());
        ASSERT_TRUE(newTempl);

        auto resources = GetResourceManager(newscene);

        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(newTempl), "app://node_new_template.template"));
        // so we can check the files
        ASSERT_EQ(resources->Export("app://node_new_template.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }
    resources->RemoveAllResources();
    {
        ASSERT_EQ(resources->Import("app://node_new_template.resources"), CORE_NS::IResourceManager::Result::OK);
        auto templ = interface_pointer_cast<META_NS::IObjectTemplate>(
            resources->GetResource(CORE_NS::ResourceIdContext{"new_template"}));

        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(root);

        auto n = root->ImportTemplate(templ).GetResult();
        ASSERT_TRUE(n);

        TestNodeTemplateRecursiveResult(res);

        n = interface_pointer_cast<INode>(templ->Update(*interface_cast<META_NS::IObject>(n), nullptr));

        TestNodeTemplateRecursiveResult(res);

        auto newTempl =
            CreateNodeTemplate(CORE_NS::ResourceIdContext{"new_new_template"}, scene->GetRootNode().GetResult());
        ASSERT_TRUE(newTempl);

        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(newTempl), "app://node_new_new_template.template"));
        // so we can check the files
        ASSERT_EQ(resources->Export("app://node_new_new_template.resources"), CORE_NS::IResourceManager::Result::OK);
    }
    resources->RemoveAllResources();
    {
        ASSERT_EQ(resources->Import("app://node_new_new_template.resources"), CORE_NS::IResourceManager::Result::OK);
        auto templ = interface_pointer_cast<META_NS::IObjectTemplate>(
            resources->GetResource(CORE_NS::ResourceIdContext{"new_new_template"}));

        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(root);

        auto n = root->ImportTemplate(templ).GetResult();
        ASSERT_TRUE(n);

        TestNodeTemplateRecursiveResult(res);
    }
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
        n2->Scale()->SetValue({2.0, 2.0, 2.0});
        n2->Rotation()->SetValue({2.0, 2.0, 2.0, 2.0});
        auto n3 = scene->CreateNode("//test/other").GetResult();
        ASSERT_TRUE(n3);

        META_NS::Object obj = META_NS::CreateObjectInstance<META_NS::IObject>();
        META_NS::SetName(obj, "MyAttachment");
        META_NS::Metadata md(obj);
        md.AddProperty<BASE_NS::string>("test", "myvalue", META_NS::ObjectFlagBits::SERIALIZE);
        interface_cast<META_NS::IAttach>(n2)->Attach(obj.GetPtr());

        templ1 = CreateNodeTemplate(CORE_NS::ResourceIdContext{"mynodes"}, n1);
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
        n1->Scale()->SetValue({4.0, 4.0, 4.0});
        auto n2 = scene->CreateNode("//test/some").GetResult();
        ASSERT_TRUE(n2);
        n2->Rotation()->SetValue({3.0, 3.0, 3.0, 3.0});

        META_NS::Object obj = META_NS::CreateObjectInstance<META_NS::IObject>();
        META_NS::SetName(obj, "MyAttachment");
        META_NS::Metadata md(obj);
        md.AddProperty<BASE_NS::string>("test", "myvalue2", META_NS::ObjectFlagBits::SERIALIZE);
        interface_cast<META_NS::IAttach>(n2)->Attach(obj.GetPtr());

        templ2 = CreateNodeTemplate(CORE_NS::ResourceIdContext{"mynodes"}, n1);
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

        n2->Position()->SetValue({3.0, 3.0, 3.0});
        n2->Rotation()->SetValue({4.0, 4.0, 4.0, 4.0});

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
        ASSERT_TRUE(n);

        EXPECT_TRUE(GetExternalNodeAttachment(n));
        EXPECT_TRUE(interface_cast<META_NS::IAttach>(n)->GetAttachmentContainer()->FindByName("Something"));

        EXPECT_FALSE(scene->FindNode("//test/other").GetResult());
        auto tn1 = scene->FindNode("//test").GetResult();
        ASSERT_TRUE(tn1);
        EXPECT_EQ(tn1->Scale()->GetValue(), (BASE_NS::Math::Vec3{4.0, 4.0, 4.0}));
        auto tn2 = scene->FindNode("//test/some").GetResult();
        ASSERT_TRUE(tn2);
        EXPECT_EQ(tn2->Position()->GetValue(), (BASE_NS::Math::Vec3{3.0, 3.0, 3.0}));
        EXPECT_EQ(tn2->Scale()->GetValue(), (BASE_NS::Math::Vec3{1.0, 1.0, 1.0}));
        EXPECT_EQ(tn2->Rotation()->GetValue(), (BASE_NS::Math::Quat{4.0, 4.0, 4.0, 4.0}));

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
        sn->Scale()->SetValue({2.0, 2.0, 2.0});

        t2 = CreateTestTemplate("templ", n);
    }
    {
        auto scene = CreateTestNodes("templ");
        ASSERT_TRUE(scene);
        auto n = scene->FindNode("//templ").GetResult();
        ASSERT_TRUE(n);

        auto sn = scene->FindNode("//templ/some").GetResult();
        ASSERT_TRUE(sn);
        sn->Scale()->SetValue({3.0, 3.0, 3.0});

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

        EXPECT_EQ(sn->Scale()->GetValue(), (BASE_NS::Math::Vec3{1.0, 1.0, 1.0}));
    }
    {
        n = interface_pointer_cast<INode>(t2->Update(*interface_cast<META_NS::IObject>(n), nullptr));
        ASSERT_TRUE(n);

        EXPECT_TRUE(GetExternalNodeAttachment(n));

        auto sn = scene->FindNode("//templ/some").GetResult();
        ASSERT_TRUE(sn);

        EXPECT_EQ(sn->Scale()->GetValue(), (BASE_NS::Math::Vec3{2.0, 2.0, 2.0}));
    }
    {
        n = interface_pointer_cast<INode>(t3->Update(*interface_cast<META_NS::IObject>(n), nullptr));
        ASSERT_TRUE(n);

        EXPECT_TRUE(GetExternalNodeAttachment(n));

        auto sn = scene->FindNode("//templ/some").GetResult();
        ASSERT_TRUE(sn);

        EXPECT_EQ(sn->Scale()->GetValue(), (BASE_NS::Math::Vec3{3.0, 3.0, 3.0}));
    }
    {
        n = interface_pointer_cast<INode>(t1->Update(*interface_cast<META_NS::IObject>(n), nullptr));
        ASSERT_TRUE(n);

        EXPECT_TRUE(GetExternalNodeAttachment(n));

        auto sn = scene->FindNode("//templ/some").GetResult();
        ASSERT_TRUE(sn);

        EXPECT_EQ(sn->Scale()->GetValue(), (BASE_NS::Math::Vec3{1.0, 1.0, 1.0}));
    }
}

/**
 * @tc.name: UpdateAttachment
 * @tc.desc: Tests for Update Attachment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, UpdateAttachment, testing::ext::TestSize.Level1)
{
    META_NS::IObjectTemplate::Ptr templ;
    {
        auto scene = CreateTestNodes("test");
        ASSERT_TRUE(scene);

        auto ext = scene->FindNode("//test/cube").GetResult();
        ASSERT_TRUE(ext);

        META_NS::Object obj = META_NS::CreateObjectInstance<META_NS::IObject>();
        META_NS::SetName(obj, "MyAttachment");
        interface_cast<META_NS::IAttach>(ext)->Attach(obj.GetPtr());

        auto n = scene->FindNode("//test").GetResult();
        ASSERT_TRUE(n);
        templ = CreateTestTemplate("test", n);
        ASSERT_TRUE(templ);
    }

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(templ).GetResult();
    ASSERT_TRUE(n);

    {
        auto ext = scene->FindNode("//test/cube").GetResult();
        ASSERT_TRUE(ext);

        auto vec = interface_cast<META_NS::IAttach>(ext)->GetAttachmentContainer()->FindAll(
            META_NS::IContainer::FindOptions{"MyAttachment"});
        EXPECT_EQ(vec.size(), 1);
    }

    n = interface_pointer_cast<INode>(templ->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);

    {
        auto ext = scene->FindNode("//test/cube").GetResult();
        ASSERT_TRUE(ext);

        auto vec = interface_cast<META_NS::IAttach>(ext)->GetAttachmentContainer()->FindAll(
            META_NS::IContainer::FindOptions{"MyAttachment"});
        EXPECT_EQ(vec.size(), 1);
    }
}

/**
 * @tc.name: SaveAttachment
 * @tc.desc: Tests for Save Attachment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, SaveAttachment, testing::ext::TestSize.Level1)
{
    META_NS::IObjectTemplate::Ptr templ;
    {
        auto scene = CreateTestNodes("test");
        ASSERT_TRUE(scene);

        auto ext = scene->FindNode("//test/cube").GetResult();
        ASSERT_TRUE(ext);

        META_NS::Object obj = META_NS::CreateObjectInstance<META_NS::IObject>();
        META_NS::SetName(obj, "MyAttachment");
        META_NS::Metadata md(obj);
        md.AddProperty<uint32_t>("test", 0, META_NS::ObjectFlagBits::SERIALIZE);
        md.GetProperty<uint32_t>("test")->SetValue(1);
        interface_cast<META_NS::IAttach>(ext)->Attach(obj.GetPtr());

        auto n = scene->FindNode("//test").GetResult();
        ASSERT_TRUE(n);
        templ = CreateTestTemplate("test", n);
        ASSERT_TRUE(templ);
    }

    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(root);

        auto n = root->ImportTemplate(templ).GetResult();
        ASSERT_TRUE(n);

        {
            auto ext = scene->FindNode("//test/cube").GetResult();
            ASSERT_TRUE(ext);

            auto vec = interface_cast<META_NS::IAttach>(ext)->GetAttachmentContainer()->FindAll(
                META_NS::IContainer::FindOptions{"MyAttachment"});
            EXPECT_EQ(vec.size(), 1);
            if (!vec.empty()) {
                META_NS::Metadata md(vec[0]);
                auto tprop = md.GetProperty<uint32_t>("test");
                ASSERT_TRUE(tprop);
                EXPECT_EQ(tprop->GetValue(), 1);

                // override the value
                tprop->SetValue(2);
            }
        }

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(templ), "app://save_att.template"));

        ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://save_att.scene"));

        ASSERT_EQ(resources->Export("app://save_att.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://save_att.g-res"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }

    ASSERT_EQ(resources->Import("app://save_att.g-res"), CORE_NS::IResourceManager::Result::OK);
    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    {
        auto ext = scene->FindNode("//test/cube").GetResult();
        ASSERT_TRUE(ext);

        auto vec = interface_cast<META_NS::IAttach>(ext)->GetAttachmentContainer()->FindAll(
            META_NS::IContainer::FindOptions{"MyAttachment"});
        ASSERT_EQ(vec.size(), 1);

        META_NS::Metadata md(vec[0]);
        auto tprop = md.GetProperty<uint32_t>("test");
        ASSERT_TRUE(tprop);
        EXPECT_EQ(tprop->GetValue(), 2);
        EXPECT_EQ(tprop->GetDefaultValue(), 1);
    }
}

/**
 * @tc.name: UpdateWithOverride
 * @tc.desc: Tests for Update with override. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, UpdateWithOverride, testing::ext::TestSize.Level1)
{
    {
        auto t = CreateTestNodeTemplate("test");
        ASSERT_TRUE(t);

        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(root);

        auto n = root->ImportTemplate(t).GetResult();
        ASSERT_TRUE(n);

        auto cube = scene->FindNode<IMesh>("//test/cube//AnimatedCube").GetResult();
        ASSERT_TRUE(cube);
        auto subs = cube->SubMeshes()->GetValue();
        ASSERT_FALSE(subs.empty());
        // set override
        subs[0]->Material()->GetValue()->Textures()->GetValue()[0]->Factor()->SetValue({1, 0, 0, 0});

        // reapply template
        t->Update(*interface_pointer_cast<META_NS::IObject>(n), scene);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }
        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://update_override.scene"));

        ASSERT_EQ(resources->Export("app://update_override.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://update_override.g-res"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }

    ASSERT_EQ(resources->Import("app://update_override.g-res"), CORE_NS::IResourceManager::Result::OK);
    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    auto cube = scene->FindNode<IMesh>("//test/cube//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);
    auto subs = cube->SubMeshes()->GetValue();
    ASSERT_FALSE(subs.empty());

    auto p = subs[0]->Material()->GetValue()->Textures()->GetValue()[0]->Factor();
    EXPECT_EQ(p->GetValue(), BASE_NS::Math::Vec4(1, 0, 0, 0));
    EXPECT_EQ(p->GetDefaultValue(), BASE_NS::Math::Vec4(0, 1, 0, 0));
}

/**
 * @tc.name: UpdateResetTemplateNodePropertyOverride
 * @tc.desc: Tests for Update Reset Template Node Property Override. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, UpdateResetTemplateNodePropertyOverride, testing::ext::TestSize.Level1)
{
    META_NS::IObjectTemplate::Ptr t1;
    META_NS::IObjectTemplate::Ptr t2;
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto n1 = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(n1);
        n1->Scale()->SetValue({2.0, 2.0, 2.0});

        t1 = CreateNodeTemplate(CORE_NS::ResourceIdContext{"mynodes"}, n1);
        ASSERT_TRUE(t1);

        n1->Scale()->ResetValue();
        t2 = CreateNodeTemplate(CORE_NS::ResourceIdContext{"mynodes"}, n1);
        ASSERT_TRUE(t2);
    }

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(t1).GetResult();
    ASSERT_TRUE(n);

    {
        auto sn = scene->FindNode("//test").GetResult();
        ASSERT_TRUE(sn);

        EXPECT_EQ(sn->Scale()->GetValue(), (BASE_NS::Math::Vec3{2.0, 2.0, 2.0}));
    }
    {
        t2->Update(*interface_pointer_cast<META_NS::IObject>(n), scene);

        auto sn = scene->FindNode("//test").GetResult();
        ASSERT_TRUE(sn);

        EXPECT_EQ(sn->Scale()->GetValue(), (BASE_NS::Math::Vec3{1.0, 1.0, 1.0}));
    }
}

// This is relevant for windows and LumeEditor only
#ifdef _WIN32
/**
 * @tc.name: UpdateResetTemplateMaterialPropertyOverride
 * @tc.desc: Tests for Update Reset Template Material Property Override. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(
    API_SceneNodeTemplateTest, DISABLED_UpdateResetTemplateMaterialPropertyOverride, testing::ext::TestSize.Level1)
{
    META_NS::IObjectTemplate::Ptr t1;
    META_NS::IObjectTemplate::Ptr t2;
    {
        auto scene = CreateTestNodes("test");
        ASSERT_TRUE(scene);

        auto n = scene->FindNode("//test").GetResult();
        ASSERT_TRUE(n);

        t1 = CreateTestTemplate("test", n);
        ASSERT_TRUE(t1);

        auto cube = scene->FindNode<IMesh>("//test/cube//AnimatedCube").GetResult();
        ASSERT_TRUE(cube);
        auto subs = cube->SubMeshes()->GetValue();
        ASSERT_FALSE(subs.empty());
        // reset override
        auto p = subs[0]->Material()->GetValue()->Textures()->GetValue()[0]->Factor();
        p->ResetValue();

        t2 = CreateTestTemplate("test", n);
        ASSERT_TRUE(t2);
    }

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(t1).GetResult();
    ASSERT_TRUE(n);

    {
        auto cube = scene->FindNode<IMesh>("//test/cube//AnimatedCube").GetResult();
        ASSERT_TRUE(cube);
        auto subs = cube->SubMeshes()->GetValue();
        ASSERT_FALSE(subs.empty());

        auto p = subs[0]->Material()->GetValue()->Textures()->GetValue()[0]->Factor();
        EXPECT_EQ(p->GetValue(), BASE_NS::Math::Vec4(0, 1, 0, 0));
    }
    {
        t2->Update(*interface_pointer_cast<META_NS::IObject>(n), scene);

        auto cube = scene->FindNode<IMesh>("//test/cube//AnimatedCube").GetResult();
        ASSERT_TRUE(cube);
        auto subs = cube->SubMeshes()->GetValue();
        ASSERT_FALSE(subs.empty());

        auto p = subs[0]->Material()->GetValue()->Textures()->GetValue()[0]->Factor();
        EXPECT_EQ(p->GetValue(), BASE_NS::Math::Vec4(1, 1, 1, 0));
    }
}
#endif

/**
 * @tc.name: LoadNodeTemplate
 * @tc.desc: Tests for LoadNodeTemplate. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, LoadNodeTemplate, testing::ext::TestSize.Level1)
{
    {
        auto t = CreateTestNodeTemplate("test");
        ASSERT_TRUE(t);

        ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(t), "app://load_node.template"));
        // so we can check the files
        ASSERT_EQ(resources->Export("app://load_node.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }
    auto scene = CreateEmptyScene();

    auto templ = LoadNodeTemplate(*resources, "app://load_node.template");
    ASSERT_TRUE(templ);

    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(templ).GetResult();
    ASSERT_TRUE(n);

    ASSERT_TRUE(scene->FindNode("//test/cube").GetResult());

    scene->RemoveNode(BASE_NS::move(n)).GetResult();

    ASSERT_FALSE(scene->FindNode("//test/cube").GetResult());
}
/**
 * @tc.name: LoadNodeTemplateAPI
 * @tc.desc: Tests for LoadNodeTemplate API.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, LoadNodeTemplateAPI, testing::ext::TestSize.Level1)
{
    {
        auto t = CreateTestNodeTemplate("test");
        ASSERT_TRUE(t);

        ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(t), "app://load_node.template"));
        // so we can check the files
        ASSERT_EQ(resources->Export("app://load_node.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }
    auto s = Scene(CreateEmptyScene());
    auto root = s.GetRootNode();
    ASSERT_TRUE(root);

    auto imported = root.ImportTemplate("app://load_node.template");
    EXPECT_TRUE(imported);
    EXPECT_TRUE(s.GetNode("//test/cube"));
    EXPECT_TRUE(s.RemoveNode(imported));
    EXPECT_FALSE(s.GetNode("//test/cube"));

    imported = root.ImportTemplate<META_NS::Async>("app://load_node.template").GetResult();
    EXPECT_TRUE(imported);
    EXPECT_TRUE(s.GetNode("//test/cube"));
}
/**
 * @tc.name: ExternalNode
 * @tc.desc: Tests for External Node. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, ExternalNode, testing::ext::TestSize.Level1)
{
    auto templ = CreateTestNodeTemplate("templ");
    ASSERT_TRUE(templ);

    resources->RemoveAllResources();
    // auto scene = CreateSceneWithNewContext();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto resources = GetResourceManager(scene);

    auto entities = GetEntityCount(scene);
    //    LogAliveEntities(scene);

    EXPECT_FALSE(resources->GetResource({{"AnimatedCube.gltf/images/0", "cube"}, scene}));
    EXPECT_FALSE(resources->GetResource({{"AnimatedCube.gltf/images/1", "cube"}, scene}));
    EXPECT_FALSE(resources->GetResource({{"animation_AnimatedCube", "cube"}, scene}));
    EXPECT_FALSE(scene->FindNode("//templ/some").GetResult());

    auto root = scene->GetRootNode().GetResult();
    auto rootImp = interface_pointer_cast<INodeImport>(root);
    ASSERT_TRUE(rootImp);

    auto n = rootImp->ImportTemplate(templ).GetResult();
    ASSERT_TRUE(n);

    UpdateScene(scene);

    EXPECT_TRUE(!scene->GetAnimations().GetResult().empty());

    EXPECT_TRUE(scene->GetResource({"AnimatedCube.gltf/images/0", "cube"}));
    EXPECT_TRUE(scene->GetResource({"AnimatedCube.gltf/images/1", "cube"}));
    EXPECT_TRUE(scene->GetResource({"animation_AnimatedCube", "cube"}));
    EXPECT_TRUE(scene->GetResource({"material_templ", "template_templ"}));
    EXPECT_TRUE(scene->FindNode("//templ/some").GetResult());

    ASSERT_TRUE(root->RemoveChild(n).GetResult());

    UpdateScene(scene);

    // remove child does not remove the used resources
    EXPECT_TRUE(resources->GetResource({{"AnimatedCube.gltf/images/0", "cube"}, scene}));
    EXPECT_TRUE(resources->GetResource({{"AnimatedCube.gltf/images/1", "cube"}, scene}));
    EXPECT_TRUE(resources->GetResource({{"animation_AnimatedCube", "cube"}, scene}));
    EXPECT_TRUE(resources->GetResource({{"material_templ", "template_templ"}, scene}));
    EXPECT_FALSE(scene->FindNode("//templ/some").GetResult());

    ASSERT_TRUE(root->AddChild(n).GetResult());

    EXPECT_TRUE(resources->GetResource({{"AnimatedCube.gltf/images/0", "cube"}, scene}));
    EXPECT_TRUE(resources->GetResource({{"AnimatedCube.gltf/images/1", "cube"}, scene}));
    EXPECT_TRUE(resources->GetResource({{"animation_AnimatedCube", "cube"}, scene}));
    EXPECT_TRUE(resources->GetResource({{"material_templ", "template_templ"}, scene}));
    EXPECT_TRUE(scene->FindNode("//templ/some").GetResult());

    scene->RemoveNode(BASE_NS::move(n)).GetResult();

    UpdateScene(scene);

    EXPECT_FALSE(resources->GetResource({{"AnimatedCube.gltf/images/0", "cube"}, scene}));
    EXPECT_FALSE(resources->GetResource({{"AnimatedCube.gltf/images/1", "cube"}, scene}));
    EXPECT_FALSE(resources->GetResource({{"animation_AnimatedCube", "cube"}, scene}));
    EXPECT_FALSE(resources->GetResource({{"material_templ", "template_templ"}, scene}));
    EXPECT_FALSE(scene->FindNode("//templ/some").GetResult());

    auto& m = scene->GetInternalScene()->GetEcsContext().GetNativeEcs()->GetEntityManager();

    EXPECT_TRUE(scene->GetAnimations().GetResult().empty());
    EXPECT_EQ(entities, GetEntityCount(scene));
    // LogAliveEntities(scene);
}

/**
 * @tc.name: RemakeTemplate
 * @tc.desc: Tests for Remake Template [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, RemakeTemplate, testing::ext::TestSize.Level1)
{
    auto templ = CreateTestNodeTemplate("templ");
    ASSERT_TRUE(templ);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto root = scene->GetRootNode().GetResult();
    auto rootImp = interface_pointer_cast<INodeImport>(root);
    ASSERT_TRUE(rootImp);

    // auto n = rootImp->ImportTemplate(templ).GetResult();
    auto n = ApplyAsNonTemplate(templ, root);
    ASSERT_TRUE(n);

    auto checkMesh = [](auto n) {
        auto subs = n->SubMeshes()->GetValue();
        ASSERT_TRUE(!subs.empty());
        auto mat = subs[0]->Material()->GetValue();
        ASSERT_TRUE(mat);
        EXPECT_EQ(META_NS::GetName(mat), "material_templ");
        EXPECT_EQ(mat->LightingFlags()->GetValue(), LightingFlags::SHADOW_CASTER_BIT);
        auto shader = mat->MaterialShader()->GetValue();
        {
            auto i = interface_cast<CORE_NS::IResource>(shader);
            ASSERT_TRUE(i);
            EXPECT_EQ(i->GetResourceId().name, "shader");
        }
    };

    auto n1 = scene->FindNode<IMesh>("//templ/cube//AnimatedCube").GetResult();
    ASSERT_TRUE(n1);
    checkMesh(n1);

    // attempt to remake the template
    auto newTempl = CreateNodeTemplate(CORE_NS::ResourceIdContext{"newTempl"}, n);
    ASSERT_TRUE(newTempl);

    resources->RemoveAllResources();
    {
        auto newScene = CreateEmptyScene();
        auto root = newScene->GetRootNode().GetResult();
        auto rootImp = interface_pointer_cast<INodeImport>(root);
        ASSERT_TRUE(rootImp);

        auto n = rootImp->ImportTemplate(newTempl).GetResult();
        ASSERT_TRUE(n);

        auto n1 = newScene->FindNode<IMesh>("//templ/cube//AnimatedCube").GetResult();
        ASSERT_TRUE(n1);
        checkMesh(n1);
    }
}

/**
 * @tc.name: NodeTemplateUpdateAttachment
 * @tc.desc: Tests for Node Template Update Attachment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, NodeTemplateUpdateAttachment, testing::ext::TestSize.Level1)
{
    META_NS::IObjectTemplate::Ptr templ1;
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto n1 = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(n1);

        META_NS::Object obj = META_NS::CreateObjectInstance<META_NS::IObject>();
        META_NS::SetName(obj, "MyAttachment");
        META_NS::Metadata md(obj);
        md.AddProperty<BASE_NS::string>("test", "myvalue", META_NS::ObjectFlagBits::SERIALIZE);
        md.AddProperty<BASE_NS::string>("user", "hihi", META_NS::ObjectFlagBits::SERIALIZE);
        interface_cast<META_NS::IAttach>(n1)->Attach(obj.GetPtr());

        templ1 = CreateNodeTemplate(CORE_NS::ResourceIdContext{"mynodes"}, n1);
        ASSERT_TRUE(templ1);

        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(templ1), "app://nt_update_att_1.template"));
        // so we can check the files
        ASSERT_EQ(resources->Export("app://nt_update_att.resources"), CORE_NS::IResourceManager::Result::OK);
    }
    META_NS::IObjectTemplate::Ptr templ2;
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto n1 = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(n1);

        META_NS::Object obj = META_NS::CreateObjectInstance<META_NS::IObject>();
        META_NS::SetName(obj, "MyAttachment");
        META_NS::Metadata md(obj);
        md.AddProperty<BASE_NS::string>("test", "myvalue2", META_NS::ObjectFlagBits::SERIALIZE);
        md.AddProperty<BASE_NS::string>("user", "hihi-update", META_NS::ObjectFlagBits::SERIALIZE);
        md.AddProperty<BASE_NS::string>("test2", "value", META_NS::ObjectFlagBits::SERIALIZE);
        interface_cast<META_NS::IAttach>(n1)->Attach(obj.GetPtr());

        templ2 = CreateNodeTemplate(CORE_NS::ResourceIdContext{"mynodes"}, n1);
        ASSERT_TRUE(templ2);

        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(templ2), "app://nt_update_att_2.template"));
        // so we can check the files
        ASSERT_EQ(resources->Export("app://nt_update_att.resources"), CORE_NS::IResourceManager::Result::OK);
    }
    META_NS::IObjectTemplate::Ptr templ3;
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto n1 = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(n1);

        META_NS::Object obj{META_NS::CreateObjectInstance<META_NS::IContainer>().GetPtr<META_NS::IObject>()};
        META_NS::SetName(obj, "MyAttachment");
        interface_cast<META_NS::IAttach>(n1)->Attach(obj.GetPtr());

        templ3 = CreateNodeTemplate(CORE_NS::ResourceIdContext{"mynodes"}, n1);
        ASSERT_TRUE(templ3);

        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(templ3), "app://nt_update_att_3.template"));
        // so we can check the files
        ASSERT_EQ(resources->Export("app://nt_update_att.resources"), CORE_NS::IResourceManager::Result::OK);
    }
    META_NS::IObjectTemplate::Ptr templ4;
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto n1 = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(n1);

        templ4 = CreateNodeTemplate(CORE_NS::ResourceIdContext{"mynodes"}, n1);
        ASSERT_TRUE(templ4);

        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(templ4), "app://nt_update_att_4.template"));
        // so we can check the files
        ASSERT_EQ(resources->Export("app://nt_update_att.resources"), CORE_NS::IResourceManager::Result::OK);
    }
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(root);

        auto n = root->ImportTemplate(templ1).GetResult();
        ASSERT_TRUE(n);

        {
            auto n1 = scene->FindNode("//test").GetResult();
            ASSERT_TRUE(n1);

            auto myatt = interface_cast<META_NS::IAttach>(n1)->GetAttachmentContainer()->FindByName("MyAttachment");
            ASSERT_TRUE(myatt);
            META_NS::Metadata md(myatt);

            auto p1 = md.GetProperty<BASE_NS::string>("test");
            ASSERT_TRUE(p1);
            EXPECT_EQ(p1->GetValue(), "myvalue");

            auto p2 = md.GetProperty<BASE_NS::string>("user");
            ASSERT_TRUE(p2);
            EXPECT_EQ(p2->GetValue(), "hihi");
            // user overwrite the value
            p2->SetValue("my-hihi");

            META_NS::Object obj = META_NS::CreateObjectInstance<META_NS::IObject>();
            META_NS::SetName(obj, "Other");
            interface_cast<META_NS::IAttach>(n1)->Attach(obj.GetPtr());
        }

        n = interface_pointer_cast<INode>(templ2->Update(*interface_cast<META_NS::IObject>(n), nullptr));
        ASSERT_TRUE(n);

        {
            auto n1 = scene->FindNode("//test").GetResult();
            ASSERT_TRUE(n1);

            EXPECT_TRUE(interface_cast<META_NS::IAttach>(n1)->GetAttachmentContainer()->FindByName("Other"));

            auto myatt = interface_cast<META_NS::IAttach>(n1)->GetAttachmentContainer()->FindByName("MyAttachment");
            ASSERT_TRUE(myatt);
            META_NS::Metadata md(myatt);

            auto p1 = md.GetProperty<BASE_NS::string>("test");
            ASSERT_TRUE(p1);
            EXPECT_EQ(p1->GetValue(), "myvalue2");

            auto p2 = md.GetProperty<BASE_NS::string>("user");
            ASSERT_TRUE(p2);
            EXPECT_EQ(p2->GetValue(), "my-hihi");

            p2->ResetValue();
            EXPECT_EQ(p2->GetValue(), "hihi-update");

            auto p3 = md.GetProperty<BASE_NS::string>("test2");
            ASSERT_TRUE(p3);
            EXPECT_EQ(p3->GetValue(), "value");
        }

        n = interface_pointer_cast<INode>(templ3->Update(*interface_cast<META_NS::IObject>(n), nullptr));
        ASSERT_TRUE(n);

        {
            auto n1 = scene->FindNode("//test").GetResult();
            ASSERT_TRUE(n1);

            EXPECT_TRUE(interface_cast<META_NS::IAttach>(n1)->GetAttachmentContainer()->FindByName("Other"));

            auto myatt = interface_cast<META_NS::IAttach>(n1)->GetAttachmentContainer()->FindByName("MyAttachment");
            ASSERT_TRUE(myatt);
            META_NS::Metadata md(myatt);

            EXPECT_FALSE(md.GetProperty<BASE_NS::string>("test"));
            EXPECT_FALSE(md.GetProperty<BASE_NS::string>("user"));
            EXPECT_FALSE(md.GetProperty<BASE_NS::string>("test2"));
        }

        n = interface_pointer_cast<INode>(templ4->Update(*interface_cast<META_NS::IObject>(n), nullptr));
        ASSERT_TRUE(n);

        {
            auto n1 = scene->FindNode("//test").GetResult();
            ASSERT_TRUE(n1);

            EXPECT_TRUE(interface_cast<META_NS::IAttach>(n1)->GetAttachmentContainer()->FindByName("Other"));

            EXPECT_FALSE(interface_cast<META_NS::IAttach>(n1)->GetAttachmentContainer()->FindByName("MyAttachment"));
        }
    }
}

/**
 * @tc.name: ImportedResources
 * @tc.desc: Tests for imported resources [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, ImportedResources, testing::ext::TestSize.Level1)
{
    auto t = CreateTestNodeTemplate("test");
    ASSERT_TRUE(t);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    auto n = root->ImportTemplate(t).GetResult();
    ASSERT_TRUE(n);

    auto res = GetImportedResources(n, {});
    ASSERT_EQ(res.size(), 7);

    auto nn = interface_pointer_cast<INodeImport>(scene->CreateNode("/some").GetResult());
    ASSERT_TRUE(nn);

    n = nn->ImportTemplate(t).GetResult();
    ASSERT_TRUE(n);

    res = GetImportedResources(n, {});
    ASSERT_EQ(res.size(), 7);

    n = interface_pointer_cast<INode>(t->Update(*interface_pointer_cast<META_NS::IObject>(n), nullptr));

    res = GetImportedResources(n, {});
    ASSERT_EQ(res.size(), 7);
}

/**
 * @tc.name: AttachmentNodePointer
 * @tc.desc: Tests for Attachment Node Pointer. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, AttachmentNodePointer, testing::ext::TestSize.Level1)
{
    META_NS::IObjectTemplate::Ptr templ;
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto n1 = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(n1);
        auto n2 = scene->CreateNode("//test/some").GetResult();
        ASSERT_TRUE(n2);

        META_NS::Object obj = META_NS::CreateObjectInstance<META_NS::IObject>();
        META_NS::SetName(obj, "MyAttachment");
        META_NS::Metadata md(obj);
        md.AddProperty<INode::WeakPtr>("test", n2, META_NS::ObjectFlagBits::SERIALIZE);
        interface_cast<META_NS::IAttach>(n1)->Attach(obj.GetPtr());

        templ = CreateNodeTemplate(CORE_NS::ResourceIdContext{"mynodes"}, n1);
        ASSERT_TRUE(templ);

        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(templ), "app://att_node_pointer.template"));
        // so we can check the files
        ASSERT_EQ(resources->Export("app://att_node_pointer.resources"), CORE_NS::IResourceManager::Result::OK);
    }
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(root);

        auto n = root->ImportTemplate(templ).GetResult();
        ASSERT_TRUE(n);

        auto n1 = scene->FindNode("//test").GetResult();
        ASSERT_TRUE(n1);
        auto n2 = scene->FindNode("//test/some").GetResult();
        ASSERT_TRUE(n2);

        auto myatt = interface_cast<META_NS::IAttach>(n1)->GetAttachmentContainer()->FindByName("MyAttachment");
        ASSERT_TRUE(myatt);
        META_NS::Metadata md(myatt);

        auto p = md.GetProperty<INode::WeakPtr>("test");
        ASSERT_TRUE(p);
        auto node = p->GetValue();
        EXPECT_EQ(node.lock(), n2);

        auto n3 = scene->CreateNode("//whatever").GetResult();
        ASSERT_TRUE(n3);

        EXPECT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"cube"},
            ::SCENE_NS::ClassId::GltfSceneResource.Id().ToUid(),
            "test://AnimatedCube/AnimatedCube.gltf"));

        auto i = interface_cast<INodeImport>(n3);
        ASSERT_TRUE(i);
        auto cubeNode =
            i->ImportChildScene(
                 interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"cube"})), "cube")
                .GetResult();

        auto acube = scene->FindNode("//whatever/cube//AnimatedCube").GetResult();

        p->SetValue(acube);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }
        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://att_node_pointer.scene"));

        ASSERT_EQ(resources->Export("app://att_node_pointer.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://att_node_pointer.g-res"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }

    ASSERT_EQ(resources->Import("app://att_node_pointer.g-res"), CORE_NS::IResourceManager::Result::OK);
    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    auto n1 = scene->FindNode("//test").GetResult();
    ASSERT_TRUE(n1);
    auto acube = scene->FindNode("//whatever/cube//AnimatedCube").GetResult();
    ASSERT_TRUE(acube);

    auto myatt = interface_cast<META_NS::IAttach>(n1)->GetAttachmentContainer()->FindByName("MyAttachment");
    ASSERT_TRUE(myatt);
    META_NS::Metadata md(myatt);

    auto p = md.GetProperty<INode::WeakPtr>("test");
    ASSERT_TRUE(p);
    auto node = p->GetValue();
    EXPECT_EQ(node.lock(), acube);
}

/**
 * @tc.name: ImportTemplateHandleCount
 * @tc.desc: Tests for handle counts. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, ImportTemplateHandleCount, testing::ext::TestSize.Level1)
{
    auto t = CreateTestNodeTemplate("test");
    {
        auto resources = GetResourceManager(scene);

        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(t), "app://import_handle_count.template"));
        // so we can check the files
        ASSERT_EQ(resources->Export("app://import_handle_count.resources"), CORE_NS::IResourceManager::Result::OK);

        resources->RemoveAllResources();
    }

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());

    auto is = scene->GetInternalScene();
    ASSERT_TRUE(is);
    auto rc = is->GetContext();
    ASSERT_TRUE(rc);
    auto resources = GetResourceManager(scene);

    UpdateSceneAndRenderFrame(scene);
    auto counters = rc->GetCounters();

    auto n = root->ImportTemplate(t).GetResult();
    ASSERT_TRUE(n);

    auto betweenCounters = rc->GetCounters();

    UpdateSceneAndRenderFrame(scene);
    auto impCounters = rc->GetCounters();

    scene->RemoveNode(BASE_NS::move(n)).GetResult();

    UpdateSceneAndRenderFrame(scene);

    auto counters2 = rc->GetCounters();
    EXPECT_EQ(counters.resourceCount, counters2.resourceCount);
    EXPECT_EQ(counters.handles.imageHandleCount, counters2.handles.imageHandleCount);
    EXPECT_EQ(counters.handles.bufferHandleCount, counters2.handles.bufferHandleCount);
    EXPECT_EQ(counters.handles.samplerHandleCount, counters2.handles.samplerHandleCount);

    ASSERT_EQ(counters.scenes.size(), counters2.scenes.size());
    for (size_t i = 0; i != counters.scenes.size(); ++i) {
        EXPECT_EQ(counters.scenes[i].entityCount, counters2.scenes[i].entityCount);
        EXPECT_EQ(counters.scenes[i].nodeCount, counters2.scenes[i].nodeCount);
        EXPECT_EQ(counters.scenes[i].nodeObjectCount, counters2.scenes[i].nodeObjectCount);
    }
}

/**
 * @tc.name: ImportTemplateHandleCount
 * @tc.desc: Tests for handle counts. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneNodeTemplateTest, ImportResourceOptions, testing::ext::TestSize.Level1)
{
    META_NS::IObjectTemplate::Ptr templ;
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = scene->GetRootNode().GetResult();
        ASSERT_TRUE(root);

        INode::Ptr node;
        {
            ASSERT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"cube"},
                ::SCENE_NS::ClassId::GltfSceneResource.Id().ToUid(),
                "test://AnimatedCube/AnimatedCube.gltf"));

            auto i = interface_cast<INodeImport>(root);
            node =
                i->ImportChildScene(
                     interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"cube"})), "ext")
                    .GetResult();
            ASSERT_TRUE(node);
        }

        IMaterial::Ptr material;
        {
            material = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
            ASSERT_TRUE(material);
            if (auto r = interface_cast<CORE_NS::ISetResourceId>(material)) {
                r->SetResourceId({{"material", "Scene"}, scene});
            }
            material->MaterialShader()->SetValue(CreateTestShader(CORE_NS::ResourceIdContext{{"shader", "Scene"}}));
            EXPECT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(material), ""));
        }

        auto cube = scene->FindNode<IMesh>("//ext//AnimatedCube").GetResult();
        ASSERT_TRUE(cube);
        auto subs = cube->SubMeshes()->GetValue();
        ASSERT_FALSE(subs.empty());
        subs[0]->Material()->SetValue(material);

        templ = CreateNodeTemplate(CORE_NS::ResourceIdContext{"templ"}, node);
        ASSERT_TRUE(templ);

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(templ), "app://update_opts.template"));
        // so we can check the files
        ASSERT_EQ(resources->Export("app://update_opts.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto i = interface_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(i->ImportTemplate(templ).GetResult());
    auto resources = GetResourceManager(scene);
    auto rs = resources->GetResource(CORE_NS::ResourceIdContext{{"shader", "templ"}, scene});
    EXPECT_TRUE(rs);

    auto cube = scene->FindNode<IMesh>("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(cube);
    auto subs = cube->SubMeshes()->GetValue();
    ASSERT_FALSE(subs.empty());
    auto shader = subs[0]->Material()->GetValue()->MaterialShader()->GetValue();
    EXPECT_EQ(shader, interface_pointer_cast<IShader>(rs));
}

UNIT_TEST_F(API_SceneNodeTemplateTest, GenericComponentNodeTemplate, testing::ext::TestSize.Level1)
{
    META_NS::IObjectTemplate::Ptr templ;
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = scene->GetRootNode().GetResult();
        ASSERT_TRUE(root);

        {
            ASSERT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"cube"},
                ::SCENE_NS::ClassId::GltfSceneResource.Id().ToUid(),
                "test://AnimatedCube/AnimatedCube.gltf"));

            auto i = interface_cast<INodeImport>(root);
            auto node =
                i->ImportChildScene(
                     interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"cube"})), "test")
                    .GetResult();
            ASSERT_TRUE(node);
        }

        auto node = scene->FindNode("//test//AnimatedCube").GetResult();
        ASSERT_TRUE(node);

        // use component that is not built-in to scene
        ASSERT_FALSE(scene->GetInternalScene()->GetEcsContext().FindComponent("FogComponent"));
        auto component = scene->CreateComponent(node, "FogComponent").GetResult();
        ASSERT_TRUE(component);
        auto md = interface_cast<META_NS::IMetadata>(component);
        ASSERT_TRUE(md);
        auto p = md->GetProperty<float>("FogComponent.density");
        ASSERT_TRUE(p);
        p->SetValue(1.0);
        auto p2 = md->GetProperty<float>("FogComponent.heightFogOffset");
        ASSERT_TRUE(p2);
        p2->SetValue(1.0);

        templ = CreateNodeTemplate(CORE_NS::ResourceIdContext{"templ"}, root);
        ASSERT_TRUE(templ);

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(templ), "app://generic_comp.template"));
        // so we can check the files
        ASSERT_EQ(resources->Export("app://generic_comp.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);
        auto i = interface_cast<INodeImport>(scene->GetRootNode().GetResult());
        ASSERT_TRUE(i->ImportTemplate(templ).GetResult());
        PrintHierarchy(scene->GetRootNode().GetResult());
        auto node = scene->FindNode("///test//AnimatedCube").GetResult();
        ASSERT_TRUE(node);
        auto component = scene->GetComponent(node, "FogComponent");
        ASSERT_TRUE(component);
        auto md = interface_cast<META_NS::IMetadata>(component);
        ASSERT_TRUE(md);
        auto p = md->GetProperty<float>("FogComponent.density");
        ASSERT_TRUE(p);
        p->SetValue(0.5);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }
        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(scene), "app://generic_comp_scene.scene"));
        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(templ), "app://generic_comp.template"));
        ASSERT_EQ(resources->Export("app://generic_comp_scene.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://generic_comp_scene.g-res"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://generic_comp_scene.g-res"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    {
        auto n = scene->FindNode("///test//AnimatedCube").GetResult();
        ASSERT_TRUE(n);

        auto component = scene->GetComponent(n, "FogComponent");
        ASSERT_TRUE(component);
        auto md = interface_cast<META_NS::IMetadata>(component);
        ASSERT_TRUE(md);
        {
            auto p = md->GetProperty<float>("FogComponent.density");
            ASSERT_TRUE(p);
            EXPECT_FLOAT_EQ(p->GetValue(), 0.5);
            EXPECT_FALSE(p->IsDefaultValue());
            EXPECT_FLOAT_EQ(p->GetDefaultValue(), 1.0);
        }
        {
            auto p = md->GetProperty<float>("FogComponent.heightFogOffset");
            ASSERT_TRUE(p);
            EXPECT_TRUE(p->IsDefaultValue());
            EXPECT_FLOAT_EQ(p->GetValue(), 1.0);
        }
    }
}

}  // namespace UTest
SCENE_END_NAMESPACE()
