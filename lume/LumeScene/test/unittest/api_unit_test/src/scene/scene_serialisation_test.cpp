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
#include <scene/interface/resource/intf_render_resource_manager.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/resource/util.h>
#include <scene_metadata_importer/interface/intf_scene_exporter.h>
#include <scene_metadata_importer/interface/intf_scene_importer.h>

#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_filesystem_api.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/resources/intf_resource_manager.h>

#include <meta/api/metadata_util.h>
#include <meta/api/object.h>
#include <meta/api/resource/resource_template_access.h>
#include <meta/ext/attachment/behavior.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/serialization/intf_exporter.h>
#include <meta/interface/serialization/intf_importer.h>

#include "scene/scene_test.h"

META_TYPE(CORE_NS::IResourceType::Ptr)

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_SceneSerialisationTest : public ScenePluginTest {
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

    template<typename Obj>
    bool Export(const Obj& obj, BASE_NS::string_view file)
    {
        auto exporter = META_NS::GetObjectRegistry().Create<META_NS::IFileExporter>(META_NS::ClassId::JsonExporter);
        exporter->SetResourceManager(resources);
        auto f = GetTestEnv()->engine->GetFileManager().CreateFile(file);
        return exporter->Export(*f, interface_pointer_cast<META_NS::IObject>(obj));
    }
    template<typename Interface>
    typename Interface::Ptr Import(BASE_NS::string_view file, META_NS::SharedPtrIInterface userContext = nullptr)
    {
        if (auto f = GetTestEnv()->engine->GetFileManager().OpenFile(file)) {
            auto importer = META_NS::GetObjectRegistry().Create<META_NS::IFileImporter>(META_NS::ClassId::JsonImporter);
            importer->SetResourceManager(resources);
            importer->SetUserContext(interface_pointer_cast<META_NS::IObject>(userContext));
            return interface_pointer_cast<Interface>(importer->Import(*f));
        }
        return nullptr;
    }

    IImage::Ptr CreateTestBitmap()
    {
        auto bitmap = renderman_->LoadImage("test://images/logo.png").GetResult();
        EXPECT_TRUE(bitmap);
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(bitmap)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"image"});
        }
        if (bitmap) {
            EXPECT_TRUE(resources->AddResource(
                CORE_NS::ResourceIdContext{"image"}, ClassId::ImageResource.Id().ToUid(), "test://images/logo.png"));
        }
        return bitmap;
    }

    IShader::Ptr CreateTestShader()
    {
        auto shader = renderman_->LoadShader("test://shaders/test.shader").GetResult();
        EXPECT_TRUE(shader);
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(shader)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"shader"});
        }
        if (shader) {
            EXPECT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"shader"},
                ClassId::ShaderResource.Id().ToUid(),
                "test://shaders/test.shader"));
        }
        return shader;
    }

    template<typename Res>
    bool CreateAndSetTemplate(const META_NS::ClassInfo& acc, const CORE_NS::ResourceIdContext& id, const Res& resource)
    {
        if (auto res = interface_pointer_cast<CORE_NS::IResource>(resource)) {
            if (auto i = interface_cast<META_NS::IDerivedFromTemplate>(resource)) {
                if (auto access = META_NS::GetObjectRegistry().Create<META_NS::IResourceTemplateAccess>(acc)) {
                    auto templ = access->CreateTemplateFromResource(res);
                    if (auto r = interface_cast<CORE_NS::ISetResourceId>(templ)) {
                        r->SetResourceId(id);
                    }
                    i->SetTemplateId(templ->GetResourceId());
                    return resources->AddResource(templ);
                }
            }
        }
        return false;
    }
    template<typename Res>
    bool SetTemplate(const META_NS::ClassInfo& acc, const CORE_NS::IResource::Ptr& templ, const Res& resource)
    {
        if (auto res = interface_pointer_cast<CORE_NS::IResource>(resource)) {
            if (auto access = META_NS::GetObjectRegistry().Create<META_NS::IResourceTemplateAccess>(acc)) {
                return access->SetValuesFromTemplate(templ, res);
            }
        }
        return false;
    }

private:
    IRenderResourceManager::Ptr renderman_;
};

/**
 * @tc.name: Basic
 * @tc.desc: Tests for Basic. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, Basic, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = scene->GetRootNode().GetResult();
        auto n1 = scene->CreateNode("//test").GetResult();
        auto n2 = scene->CreateNode("//test/hips").GetResult();
        auto n3 = scene->CreateNode("//test/some").GetResult();
        auto n4 = scene->CreateNode("//test/some/disabled").GetResult();

        n1->Scale()->SetValue({2.0, 2.0, 2.0});
        n1->Position();
        n4->Enabled()->SetValue(false);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"app://test.scene"});
        }

        auto res =
            META_NS::GetObjectRegistry().Create<CORE_NS::IResourceManager>(META_NS::ClassId::FileResourceManager);
        ASSERT_TRUE(res);

        res->SetFileManager(CORE_NS::IFileManager::Ptr(&GetTestEnv()->engine->GetFileManager()));

        ASSERT_TRUE(res->AddResourceType(
            META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::SceneResource, params)));
        ASSERT_TRUE(res->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene)));

        ASSERT_EQ(res->Export("app://test.resources"), CORE_NS::IResourceManager::Result::OK);

        auto exporter = META_NS::GetObjectRegistry().Create<META_NS::IFileExporter>(META_NS::ClassId::JsonExporter);
        exporter->SetResourceManager(res);

        auto f = GetTestEnv()->engine->GetFileManager().CreateFile("app://test.json");
        ASSERT_TRUE(exporter->Export(*f, interface_pointer_cast<META_NS::IObject>(scene)));
    }

    auto res = META_NS::GetObjectRegistry().Create<CORE_NS::IResourceManager>(META_NS::ClassId::FileResourceManager);
    ASSERT_TRUE(res);

    res->SetFileManager(CORE_NS::IFileManager::Ptr(&GetTestEnv()->engine->GetFileManager()));

    ASSERT_TRUE(res->AddResourceType(
        META_NS::GetObjectRegistry().Create<CORE_NS::IResourceType>(ClassId::SceneResource, params)));

    ASSERT_EQ(res->Import("app://test.resources"), CORE_NS::IResourceManager::Result::OK);

    auto f = GetTestEnv()->engine->GetFileManager().OpenFile("app://test.json");
    ASSERT_TRUE(f);
    auto importer = META_NS::GetObjectRegistry().Create<META_NS::IFileImporter>(META_NS::ClassId::JsonImporter);
    ASSERT_TRUE(importer);

    importer->SetResourceManager(res);

    auto scene = interface_pointer_cast<IScene>(importer->Import(*f));
    ASSERT_TRUE(scene);
    auto n1 = scene->FindNode("//test").GetResult();
    ASSERT_TRUE(n1);
    ASSERT_TRUE(scene->FindNode("//test/hips").GetResult());
    ASSERT_TRUE(scene->FindNode("//test/some").GetResult());

    EXPECT_EQ(n1->Scale()->GetValue(), BASE_NS::Math::Vec3(2.0, 2.0, 2.0));

    auto dis = scene->FindNode("//test/some/disabled").GetResult();
    ASSERT_TRUE(dis);
    EXPECT_FALSE(dis->Enabled()->GetValue());
}

/**
 * @tc.name: Bitmap
 * @tc.desc: Tests for Bitmap. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, Bitmap, testing::ext::TestSize.Level1)
{
    {
        auto renderman =
            META_NS::GetObjectRegistry().Create<IRenderResourceManager>(ClassId::RenderResourceManager, params);
        ASSERT_TRUE(renderman);

        ASSERT_TRUE(resources->AddResource(
            CORE_NS::ResourceIdContext{"image"}, ClassId::ImageResource.Id().ToUid(), "test://images/logo.png"));
        ASSERT_EQ(resources->Export("app://bitmap_test.resources"), CORE_NS::IResourceManager::Result::OK);

        auto bitmap = CreateTestBitmap();
        ASSERT_TRUE(bitmap);

        ASSERT_TRUE(Export(bitmap, "app://bitmap.json"));
        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://bitmap_test.resources"), CORE_NS::IResourceManager::Result::OK);

    auto bitmap = Import<IImage>("app://bitmap.json");
    ASSERT_TRUE(bitmap);

    auto rresu = interface_cast<IRenderResource>(bitmap);
    ASSERT_TRUE(rresu);
    EXPECT_TRUE(rresu->GetRenderHandle());
}

/**
 * @tc.name: Shader
 * @tc.desc: Tests for Shader. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, Shader, testing::ext::TestSize.Level1)
{
    {
        auto renderman =
            META_NS::GetObjectRegistry().Create<IRenderResourceManager>(ClassId::RenderResourceManager, params);
        ASSERT_TRUE(renderman);

        ASSERT_TRUE(resources->AddResource(
            CORE_NS::ResourceIdContext{"shader"}, ClassId::ShaderResource.Id().ToUid(), "test://shaders/test.shader"));
        ASSERT_EQ(resources->Export("app://shader_test.resources"), CORE_NS::IResourceManager::Result::OK);

        auto shader = CreateTestShader();
        ASSERT_TRUE(shader);

        ASSERT_TRUE(Export(shader, "app://shader.json"));
        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://shader_test.resources"), CORE_NS::IResourceManager::Result::OK);

    auto shader = Import<IShader>("app://shader.json");
    ASSERT_TRUE(shader);

    auto rresu = interface_cast<IRenderResource>(shader);
    ASSERT_TRUE(rresu);
    EXPECT_TRUE(rresu->GetRenderHandle());
}

/**
 * @tc.name: Attachment
 * @tc.desc: Tests for Attachment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, Attachment, testing::ext::TestSize.Level1)
{
    {
        auto cont = META_NS::GetObjectRegistry().Create<META_NS::IContainer>(META_NS::ClassId::ObjectContainer);
        ASSERT_TRUE(cont);

        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        META_NS::SetName(scene, "TestScene");

        META_NS::Object obj1 = META_NS::CreateObjectInstance<META_NS::IObject>();
        META_NS::SetName(obj1, "TestAttachment1");
        interface_cast<META_NS::IAttach>(scene)->Attach(obj1.GetPtr());

        auto root = scene->GetRootNode().GetResult();
        auto n1 = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(n1);

        META_NS::Object obj2 = META_NS::CreateObjectInstance<META_NS::IObject>();
        META_NS::SetName(obj2, "TestAttachment2");
        interface_cast<META_NS::IAttach>(n1)->Attach(obj2.GetPtr());

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"app://att_test.scene"});
        }

        ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene)));
        cont->Add(interface_pointer_cast<META_NS::IObject>(scene));

        ASSERT_EQ(resources->Export("app://att_test.resources"), CORE_NS::IResourceManager::Result::OK);

        ASSERT_TRUE(Export(cont, "app://att_test.json"));
        resources->PurgeResource(CORE_NS::ResourceIdContext{"app://att_test.scene"});
    }

    auto obj = Import<META_NS::IContainer>("app://att_test.json");
    ASSERT_TRUE(obj);

    auto scene = interface_pointer_cast<IScene>(obj->FindByName("TestScene"));
    ASSERT_TRUE(scene);

    auto att1 = interface_cast<META_NS::IAttach>(scene)->GetAttachmentContainer()->FindByName("TestAttachment1");
    ASSERT_TRUE(att1);

    auto n1 = scene->FindNode("//test").GetResult();
    ASSERT_TRUE(n1);

    auto att2 = interface_cast<META_NS::IAttach>(n1)->GetAttachmentContainer()->FindByName("TestAttachment2");
    ASSERT_TRUE(att2);
}

/**
 * @tc.name: ExtNodeNotSerAttachment
 * @tc.desc: Tests for Ext Node not serialised attachment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, ExtNodeNotSerAttachment, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = scene->GetRootNode().GetResult();
        ASSERT_TRUE(root);

        {
            ASSERT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"scene2"},
                ClassId::GltfSceneResource.Id().ToUid(),
                "test://AnimatedCube/AnimatedCube.gltf"));

            INode::Ptr node;
            if (auto i = interface_cast<INodeImport>(root)) {
                node = i->ImportChildScene(interface_pointer_cast<IScene>(
                                               resources->GetResource(CORE_NS::ResourceIdContext{"scene2"})),
                            "ext")
                           .GetResult();
                ASSERT_TRUE(node);
            }
        }

        auto n = scene->FindNode<IMesh>("//ext//AnimatedCube").GetResult();
        ASSERT_TRUE(n);

        {
            META_NS::Object obj = META_NS::CreateObjectInstance<META_NS::IObject>();
            META_NS::SetName(obj, "TestAttachment");
            META_NS::SetObjectFlags(obj.GetPtr(), META_NS::ObjectFlagBits::SERIALIZE, false);
            interface_cast<META_NS::IAttach>(n)->Attach(obj.GetPtr());
        }

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"app://ext_node_not_ser_att.scene"});
        }

        ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene)));
        ASSERT_EQ(resources->Export("app://ext_node_not_ser_att.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://ext_node_not_ser_att.g-res"), CORE_NS::IResourceManager::Result::OK);

        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://ext_node_not_ser_att.g-res"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(
        resources->GetResource(CORE_NS::ResourceIdContext{"app://ext_node_not_ser_att.scene"}));
    ASSERT_TRUE(scene);

    auto n = scene->FindNode<IMesh>("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(n);

    auto att2 = interface_cast<META_NS::IAttach>(n)->GetAttachmentContainer()->FindByName("TestAttachment");
    ASSERT_FALSE(att2);
}

/**
 * @tc.name: ReadOnlyProps
 * @tc.desc: Tests for Read Only Props. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, ReadOnlyProps, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        scene->RenderConfiguration()->GetValue()->RenderNodeGraphUri()->SetValue("hips hops haps");

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"app://readonly_test.scene"});
        }

        ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene)));
        ASSERT_EQ(resources->Export("app://readonly_test.resources"), CORE_NS::IResourceManager::Result::OK);

        ASSERT_TRUE(Export(scene, "app://readonly_test.json"));
        resources->PurgeResource(CORE_NS::ResourceIdContext{"app://readonly_test.scene"});
    }
    auto scene = Import<IScene>("app://readonly_test.json");
    ASSERT_TRUE(scene);

    EXPECT_EQ(scene->RenderConfiguration()->GetValue()->RenderNodeGraphUri()->GetValue(), "hips hops haps");
}

/**
 * @tc.name: ExternalNode
 * @tc.desc: Tests for External Node. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, ExternalNode, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = scene->GetRootNode().GetResult();
        ASSERT_TRUE(root);
        auto n1 = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(n1);

        ASSERT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"test://celia/Celia.gltf"},
            ClassId::GltfSceneResource.Id().ToUid(),
            "test://celia/Celia.gltf"));

        if (auto i = interface_cast<INodeImport>(n1)) {
            ASSERT_TRUE(i->ImportChildScene(interface_pointer_cast<IScene>(resources->GetResource(
                                                CORE_NS::ResourceIdContext{"test://celia/Celia.gltf"})),
                             "ext")
                            .GetResult());
        }

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"app://external_test.scene"});
        }

        ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene)));
        ASSERT_EQ(resources->Export("app://external_test.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://external_test.g-res"), CORE_NS::IResourceManager::Result::OK);

        ASSERT_TRUE(Export(scene, "app://external_test.json"));
        resources->PurgeResource(CORE_NS::ResourceIdContext{"app://external_test.scene"});
    }

    auto scene = Import<IScene>("app://external_test.json");
    ASSERT_TRUE(scene);

    ASSERT_TRUE(scene->FindNode("//test").GetResult());
    ASSERT_TRUE(scene->FindNode("//test/ext").GetResult());
    ASSERT_TRUE(scene->FindNode("//test/ext/Scene/Plane").GetResult());
    ASSERT_TRUE(scene->FindNode("//test/ext/Scene/Shell").GetResult());

    auto n = scene->FindNode("//test/ext").GetResult();
    auto v = interface_cast<META_NS::IAttach>(n)->GetAttachments<IExternalNode>();
    EXPECT_EQ(v.size(), 1);
}

/**
 * @tc.name: ExternalNodeWithChanges
 * @tc.desc: Tests for External Node With Changes. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, ExternalNodeWithChanges, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = scene->GetRootNode().GetResult();
        ASSERT_TRUE(root);
        auto n1 = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(n1);

        {
            auto scene2 = CreateEmptyScene();
            ASSERT_TRUE(scene2);
            auto n = scene2->CreateNode("//mesh", ClassId::MeshNode).GetResult();
            ASSERT_TRUE(n);

            if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene2)) {
                i->SetResourceId(CORE_NS::ResourceIdContext{"scene2"});
            }

            ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene2), ""));
        }

        if (auto i = interface_cast<INodeImport>(n1)) {
            ASSERT_TRUE(i->ImportChildScene(interface_pointer_cast<IScene>(
                                                resources->GetResource(CORE_NS::ResourceIdContext{"scene2"})),
                             "ext")
                            .GetResult());
        }

        auto n = scene->FindNode("//test/ext/mesh").GetResult();
        ASSERT_TRUE(n);
        n->Scale()->SetValue({2.0, 2.0, 2.0});

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"app://external_test_c.scene"});
        }

        ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene)));
        ASSERT_EQ(resources->Export("app://external_test_c.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://external_test_c.g-res"), CORE_NS::IResourceManager::Result::OK);

        ASSERT_TRUE(Export(scene, "app://external_test_c.json"));
        resources->PurgeResource(CORE_NS::ResourceIdContext{"app://external_test_c.scene"});
    }

    auto scene = Import<IScene>("app://external_test_c.json");
    ASSERT_TRUE(scene);

    ASSERT_TRUE(scene->FindNode("//test").GetResult());
    ASSERT_TRUE(scene->FindNode("//test/ext").GetResult());
    ASSERT_TRUE(scene->FindNode("//test/ext/mesh").GetResult());

    auto n = scene->FindNode("//test/ext/mesh").GetResult();
    EXPECT_EQ(n->Scale()->GetValue(), BASE_NS::Math::Vec3(2.0, 2.0, 2.0));
}

/**
 * @tc.name: Environment
 * @tc.desc: Tests for Environment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, Environment, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto renderman =
            META_NS::GetObjectRegistry().Create<IRenderResourceManager>(ClassId::RenderResourceManager, params);
        ASSERT_TRUE(renderman);

        auto env = scene->CreateObject<IEnvironment>(ClassId::Environment).GetResult();
        ASSERT_TRUE(env);

        auto bitmap = CreateTestBitmap();
        ASSERT_TRUE(bitmap);

        env->RadianceCubemapMipCount()->SetValue(2);
        env->Background()->SetValue(EnvBackgroundType::IMAGE);
        env->EnvironmentImage()->SetValue(bitmap);

        ASSERT_TRUE(CreateAndSetTemplate(
            ClassId::EnvironmentTemplateAccess, CORE_NS::ResourceIdContext{"app://env_res.resource"}, env));

        // change it after making the pure resource
        env->RadianceCubemapMipCount()->SetValue(4);
        env->IrradianceCoefficients()->SetValue({BASE_NS::Math::Vec3(1, 1, 1), BASE_NS::Math::Vec3(1, 2, 3)});

        if (auto r = interface_cast<CORE_NS::ISetResourceId>(env)) {
            r->SetResourceId({CORE_NS::ResourceId{"env", "app://env_test.scene"}, scene});
            ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(env), ""));
        }

        scene->RenderConfiguration()->GetValue()->Environment()->SetValue(env);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"app://env_test.scene"});
        }

        ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene)));
        ASSERT_EQ(resources->Export("app://env_test.res", scene), CORE_NS::IResourceManager::Result::OK);

        ASSERT_TRUE(Export(scene, "app://env_test.json"));
        resources->RemoveAllResources(scene);
    }
    auto scene = Import<IScene>("app://env_test.json");
    ASSERT_TRUE(scene);

    auto env = scene->RenderConfiguration()->GetValue()->Environment()->GetValue();
    ASSERT_TRUE(env);

    auto der = interface_cast<META_NS::IDerivedFromTemplate>(env);
    ASSERT_TRUE(der);
    EXPECT_EQ(der->GetTemplateId(), CORE_NS::ResourceId("app://env_res.resource"));

    EXPECT_EQ(env->Background()->GetValue(), EnvBackgroundType::IMAGE);
    EXPECT_TRUE(env->EnvironmentImage()->GetValue());
    EXPECT_EQ(env->RadianceCubemapMipCount()->GetValue(), 4);
    auto vec = env->IrradianceCoefficients()->GetValue();
    ASSERT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], BASE_NS::Math::Vec3(1, 1, 1));
    EXPECT_EQ(vec[1], BASE_NS::Math::Vec3(1, 2, 3));
}

/**
 * @tc.name: CreateEnvironmentResource
 * @tc.desc: Tests for Create Environment Resource. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, CreateEnvironmentResource, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    {
        auto bitmap = CreateTestBitmap();
        ASSERT_TRUE(bitmap);

        auto res = META_NS::CreateObjectResource(ClassId::Environment, ClassId::EnvironmentResourceTemplate);
        ASSERT_TRUE(res);

        auto m = interface_cast<META_NS::IMetadata>(res);
        ASSERT_TRUE(m);
        {
            auto p = m->GetProperty<uint32_t>("RadianceCubemapMipCount", META_NS::MetadataQuery::EXISTING);
            ASSERT_TRUE(p);
            p->SetValue(69);
        }
        {
            auto p = m->GetProperty<IImage::Ptr>("RadianceImage", META_NS::MetadataQuery::EXISTING);
            ASSERT_TRUE(p);
            p->SetValue(bitmap);
        }
        {
            auto p = m->GetProperty<IImage::Ptr>("EnvironmentImage", META_NS::MetadataQuery::EXISTING);
            ASSERT_TRUE(p);
            p->SetValue(bitmap);
        }

        if (auto r = interface_cast<CORE_NS::ISetResourceId>(res)) {
            r->SetResourceId(CORE_NS::ResourceIdContext{"app://create_env.resource", scene});
        }
        ASSERT_TRUE(resources->AddResource(res));

        ASSERT_EQ(resources->Export("app://create_env.res", scene), CORE_NS::IResourceManager::Result::OK);
        resources->PurgeResource(CORE_NS::ResourceIdContext{"app://create_env.resource", scene});
        resources->PurgeResource(CORE_NS::ResourceIdContext{"image"});
    }

    auto env = scene->CreateObject<IEnvironment>(ClassId::Environment).GetResult();
    ASSERT_TRUE(env);

    auto res = resources->GetResource(CORE_NS::ResourceIdContext{"app://create_env.resource", scene});
    ASSERT_TRUE(res);
    ASSERT_TRUE(SetTemplate(ClassId::EnvironmentTemplateAccess, res, env));

    EXPECT_EQ(env->RadianceCubemapMipCount()->GetValue(), 69);
    EXPECT_TRUE(env->RadianceImage()->GetValue());
    EXPECT_TRUE(env->EnvironmentImage()->GetValue());
}

/**
 * @tc.name: EnvironmentTemplatePreservesOverride
 * @tc.desc: Applying an environment template updates the default but keeps an explicit user override.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, EnvironmentTemplatePreservesOverride, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();

    auto res = META_NS::CreateObjectResource(ClassId::Environment, ClassId::EnvironmentResourceTemplate);
    ASSERT_TRUE(res);
    {
        auto m = interface_cast<META_NS::IMetadata>(res);
        ASSERT_TRUE(m);
        auto p = m->GetProperty<uint32_t>("RadianceCubemapMipCount", META_NS::MetadataQuery::EXISTING);
        ASSERT_TRUE(p);
        p->SetValue(69);
    }

    auto env = scene->CreateObject<IEnvironment>(ClassId::Environment).GetResult();
    ASSERT_TRUE(env);

    // Explicit user override set before the template is applied.
    env->RadianceCubemapMipCount()->SetValue(1);

    ASSERT_TRUE(SetTemplate(ClassId::EnvironmentTemplateAccess, interface_pointer_cast<CORE_NS::IResource>(res), env));

    // The override stays as the effective value; only the default slot is updated.
    EXPECT_EQ(env->RadianceCubemapMipCount()->GetValue(), 1);
    EXPECT_FALSE(env->RadianceCubemapMipCount()->IsDefaultValue());
    env->RadianceCubemapMipCount()->ResetValue();
    EXPECT_EQ(env->RadianceCubemapMipCount()->GetValue(), 69);
}

/**
 * @tc.name: Material
 * @tc.desc: Tests for Material. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, Material, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    {
        auto material = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
        ASSERT_TRUE(material);

        if (auto r = interface_cast<CORE_NS::ISetResourceId>(material)) {
            r->SetResourceId(
                CORE_NS::ResourceIdContext{CORE_NS::ResourceId{"material", "app://mat_test.scene"}, scene});
        }

        material->LightingFlags()->SetValue(LightingFlags::SHADOW_CASTER_BIT);
        material->MaterialShader()->SetValue(CreateTestShader());

        auto renderSort = interface_cast<IRenderSort>(material);
        ASSERT_TRUE(renderSort);
        renderSort->RenderSortLayer()->SetValue(2);
        renderSort->RenderSortLayerOrder()->SetValue(3);

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
        ASSERT_EQ(resources->Export("app://mat_test.res", scene), CORE_NS::IResourceManager::Result::OK);

        ASSERT_TRUE(Export(material, "app://mat_test.json"));
        resources->RemoveAllResources(scene);
    }
    ASSERT_EQ(resources->Import("app://mat_test.res", scene), CORE_NS::IResourceManager::Result::OK);

    auto mat = Import<IMaterial>("app://mat_test.json", scene);
    ASSERT_TRUE(mat);
    EXPECT_EQ(mat->LightingFlags()->GetValue(), LightingFlags::SHADOW_CASTER_BIT);
    auto shader = mat->MaterialShader()->GetValue();
    {
        auto i = interface_cast<CORE_NS::IResource>(shader);
        ASSERT_TRUE(i);
        EXPECT_EQ(i->GetResourceId(), CORE_NS::ResourceId{"shader"});
    }

    auto rSort = interface_cast<IRenderSort>(mat);
    ASSERT_TRUE(rSort);
    EXPECT_EQ(rSort->RenderSortLayer()->GetValue(), 2);
    EXPECT_EQ(rSort->RenderSortLayerOrder()->GetValue(), 3);
    auto res = mat->RenderSort()->GetValue();
    EXPECT_EQ(res.renderSortLayer, 2);
    EXPECT_EQ(res.renderSortLayerOrder, 3);

    auto inputs = mat->CustomProperties()->GetValue();
    ASSERT_TRUE(inputs);
    auto prop = inputs->GetProperty<BASE_NS::Math::Vec4>("v4_");
    ASSERT_TRUE(prop);
    EXPECT_EQ(prop->GetValue(), (BASE_NS::Math::Vec4{1.0, 1.0, 1.0, 1.0}));

    auto t1 = mat->Textures()->GetValueAt(0);
    ASSERT_TRUE(t1);
    EXPECT_EQ(t1->Rotation()->GetValue(), 1.0f);
    auto bitmap = t1->Image()->GetValue();
    {
        auto i = interface_cast<CORE_NS::IResource>(bitmap);
        ASSERT_TRUE(i);
        EXPECT_EQ(i->GetResourceId(), CORE_NS::ResourceId{"image"});
    }
}

/**
 * @tc.name: MaterialType
 * @tc.desc: Tests for Material Type. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, MaterialType, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto material = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
    ASSERT_TRUE(material);
    material->LightingFlags()->SetValue(LightingFlags::SHADOW_CASTER_BIT);
    auto res = interface_pointer_cast<CORE_NS::IResource>(material);

    auto type = scene->CreateObject<CORE_NS::IResourceType>(ClassId::MaterialResource).GetResult();
    ASSERT_TRUE(type);

    // auto f = GetTestEnv()->engine->GetFileManager().CreateFile("app://material_type.json");
    // ASSERT_TRUE(f);

    auto opts =
        META_NS::GetObjectRegistry().Create<META_NS::IObjectResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
    ASSERT_TRUE(opts);

    CORE_NS::IResourceType::StorageInfo info{
        opts, nullptr, CORE_NS::ResourceId{"some"}, "", nullptr, interface_pointer_cast<CORE_NS::IInterface>(scene)};

    // f->Seek(0);
    ASSERT_TRUE(type->SaveResource(res, info));
    {
        // f->Seek(0);
        res = type->LoadResource(info);
        ASSERT_TRUE(res);
        auto mat = interface_cast<IMaterial>(res);
        EXPECT_EQ(mat->LightingFlags()->GetValue(), LightingFlags::SHADOW_CASTER_BIT);
    }
    // f->Seek(0);
    ASSERT_TRUE(type->SaveResource(res, info));
    {
        // f->Seek(0);
        res = type->LoadResource(info);
        ASSERT_TRUE(res);
        auto mat = interface_cast<IMaterial>(res);
        EXPECT_EQ(mat->LightingFlags()->GetValue(), LightingFlags::SHADOW_CASTER_BIT);
    }
}

/**
 * @tc.name: DerivedMaterial
 * @tc.desc: Tests for Derived Material. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, DerivedMaterial, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    {
        auto material = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
        ASSERT_TRUE(material);

        if (auto r = interface_cast<CORE_NS::ISetResourceId>(material)) {
            r->SetResourceId(
                CORE_NS::ResourceIdContext{CORE_NS::ResourceId{"material", "app://der_mat_test.scene"}, scene});
        }

        material->LightingFlags()->SetValue(LightingFlags::SHADOW_CASTER_BIT);
        material->MaterialShader()->SetValue(CreateTestShader());
        auto t1 = material->Textures()->GetValueAt(0);
        ASSERT_TRUE(t1);
        t1->Image()->SetValue(CreateTestBitmap());
        t1->Rotation()->SetValue(1.0f);

        ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(material), ""));
        ASSERT_TRUE(CreateAndSetTemplate(
            ClassId::MaterialTemplateAccess, CORE_NS::ResourceIdContext{"app://der_mat_res.resource"}, material));

        t1->Rotation()->SetValue(2.0f);

        ASSERT_EQ(resources->Export("app://der_mat_test.res", scene), CORE_NS::IResourceManager::Result::OK);

        ASSERT_TRUE(Export(material, "app://der_mat_test.json"));
        resources->RemoveAllResources(scene);
    }
    ASSERT_EQ(resources->Import("app://der_mat_test.res", scene), CORE_NS::IResourceManager::Result::OK);

    auto mat = Import<IMaterial>("app://der_mat_test.json", scene);
    ASSERT_TRUE(mat);
    EXPECT_EQ(mat->LightingFlags()->GetValue(), LightingFlags::SHADOW_CASTER_BIT);
    auto shader = mat->MaterialShader()->GetValue();
    {
        auto i = interface_cast<CORE_NS::IResource>(shader);
        ASSERT_TRUE(i);
        EXPECT_EQ(i->GetResourceId(), CORE_NS::ResourceId{"shader"});
    }

    auto t1 = mat->Textures()->GetValueAt(0);
    ASSERT_TRUE(t1);
    EXPECT_EQ(t1->Rotation()->GetValue(), 2.0f);
    auto bitmap = t1->Image()->GetValue();
    {
        auto i = interface_cast<CORE_NS::IResource>(bitmap);
        ASSERT_TRUE(i);
        EXPECT_EQ(i->GetResourceId(), CORE_NS::ResourceId{"image"});
    }
}

/**
 * @tc.name: DerivedMaterialFromTemplate
 * @tc.desc: Tests for Derived Material From Template. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, DerivedMaterialFromTemplate, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = scene->GetRootNode().GetResult();
        ASSERT_TRUE(root);

        {
            ASSERT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"scene2"},
                ClassId::GltfSceneResource.Id().ToUid(),
                "test://AnimatedCube/AnimatedCube.gltf"));

            if (auto i = interface_cast<INodeImport>(root)) {
                ASSERT_TRUE(i->ImportChildScene(interface_pointer_cast<IScene>(
                                                    resources->GetResource(CORE_NS::ResourceIdContext{"scene2"})),
                                 "ext")
                                .GetResult());
            }
        }

        auto n = scene->FindNode<IMesh>("//ext//AnimatedCube").GetResult();
        ASSERT_TRUE(n);

        auto subs = n->SubMeshes()->GetValue();
        ASSERT_TRUE(!subs.empty());

        auto access =
            META_NS::GetObjectRegistry().Create<META_NS::IResourceTemplateAccess>(ClassId::MaterialTemplateAccess);
        ASSERT_TRUE(access);
        auto res = access->CreateEmptyTemplate();
        ASSERT_TRUE(res);

        auto m = interface_cast<META_NS::IMetadata>(res);
        ASSERT_TRUE(m);
        {
            auto p = m->GetProperty<float>("AlphaCutoff", META_NS::MetadataQuery::EXISTING);
            ASSERT_TRUE(p);
            p->SetValue(0.5f);
        }
        auto material = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
        ASSERT_TRUE(material);

        if (auto r = interface_cast<CORE_NS::ISetResourceId>(material)) {
            r->SetResourceId({CORE_NS::ResourceId{"material", "app://der_mat_template_test.scene"}, scene});
        }

        if (auto i = interface_pointer_cast<CORE_NS::IResource>(material)) {
            if (auto r = interface_cast<CORE_NS::ISetResourceId>(res)) {
                r->SetResourceId(CORE_NS::ResourceIdContext{"app://der_mat_template_res.resource"});
            }
            access->SetValuesFromTemplate(res, i);
            ASSERT_TRUE(resources->AddResource(res));
        }
        subs[0]->Material()->SetValue(material);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(material), ""));
        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(scene), "app://der_mat_template_test.scene"));
        ASSERT_EQ(resources->Export("app://der_mat_template_test.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://der_mat_template_test.g-res"), CORE_NS::IResourceManager::Result::OK);

        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://der_mat_template_test.g-res"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);
    ASSERT_TRUE(scene->FindNode("//ext").GetResult());

    auto n = scene->FindNode<IMesh>("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(n);

    auto subs = n->SubMeshes()->GetValue();
    ASSERT_TRUE(!subs.empty());

    auto firstSub = subs[0];
    auto mat = firstSub->Material()->GetValue();
    ASSERT_TRUE(mat);

    EXPECT_EQ(mat->AlphaCutoff()->GetValue(), 0.5f);

    auto t1 = mat->Textures()->GetValueAt(0);
    ASSERT_TRUE(t1);
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
 * @tc.name: MaterialTemplateReload
 * @tc.desc: Tests for Material Template Reload. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, MaterialTemplateReload, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto access =
        META_NS::GetObjectRegistry().Create<META_NS::IResourceTemplateAccess>(ClassId::MaterialTemplateAccess);
    ASSERT_TRUE(access);
    auto templ = access->CreateEmptyTemplate();
    ASSERT_TRUE(templ);

    auto m = interface_cast<META_NS::IMetadata>(templ);
    ASSERT_TRUE(m);
    {
        auto p = m->GetProperty<float>("AlphaCutoff", META_NS::MetadataQuery::EXISTING);
        ASSERT_TRUE(p);
        p->SetValue(0.5f);
    }

    auto material = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
    ASSERT_TRUE(material);

    if (auto r = interface_cast<CORE_NS::ISetResourceId>(material)) {
        r->SetResourceId({CORE_NS::ResourceId{"material"}, scene});
    }

    if (auto r = interface_cast<CORE_NS::ISetResourceId>(templ)) {
        r->SetResourceId(CORE_NS::ResourceIdContext{"app://mat_template_reload.resource"});
    }
    if (auto i = interface_cast<META_NS::IDerivedFromTemplate>(material)) {
        i->SetTemplate(templ);
    }
    ASSERT_TRUE(resources->AddResource(templ));
    material->RenderSort()->SetValue(RenderSort{23});

    ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(material), ""));

    EXPECT_EQ(material->AlphaCutoff()->GetValue(), 0.5f);
    {
        auto p = m->GetProperty<float>("AlphaCutoff", META_NS::MetadataQuery::EXISTING);
        ASSERT_TRUE(p);
        p->SetValue(1.0f);
    }
    if (auto i = interface_cast<META_NS::IDerivedFromTemplate>(material)) {
        i->SetTemplate(templ);
    }
    resources->ReloadResource(interface_pointer_cast<CORE_NS::IResource>(material));
    EXPECT_EQ(material->AlphaCutoff()->GetValue(), 1.0f);
    EXPECT_EQ(material->RenderSort()->GetValue().renderSortLayer, 23);
}

/**
 * @tc.name: ExtNodeMaterial
 * @tc.desc: Tests for Ext Node Material. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, ExtNodeMaterial, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = scene->GetRootNode().GetResult();
        ASSERT_TRUE(root);

        {
            ASSERT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"scene2"},
                ClassId::GltfSceneResource.Id().ToUid(),
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
            auto material = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
            ASSERT_TRUE(material);

            META_NS::SetName(material, "hipshops");

            if (auto r = interface_cast<CORE_NS::ISetResourceId>(material)) {
                r->SetResourceId({CORE_NS::ResourceId{"material", "app://ext_mat_test.scene"}, scene});
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

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"app://ext_mat_test.scene"});
        }

        ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene)));
        ASSERT_EQ(resources->Export("app://ext_mat_test.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://ext_mat_test.g-res"), CORE_NS::IResourceManager::Result::OK);

        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://ext_mat_test.g-res"), CORE_NS::IResourceManager::Result::OK);

    auto scene =
        interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"app://ext_mat_test.scene"}));
    ASSERT_TRUE(scene);
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
 * @tc.name: CreateMaterialResource
 * @tc.desc: Tests for Create Material Resource. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, CreateMaterialResource, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    {
        auto access =
            META_NS::GetObjectRegistry().Create<META_NS::IResourceTemplateAccess>(ClassId::MaterialTemplateAccess);
        ASSERT_TRUE(access);
        auto resource = access->CreateEmptyTemplate();
        ASSERT_TRUE(resource);

        auto m = interface_cast<META_NS::IMetadata>(resource);
        ASSERT_TRUE(m);
        {
            auto p = m->GetProperty<float>("AlphaCutoff", META_NS::MetadataQuery::EXISTING);
            ASSERT_TRUE(p);
            p->SetValue(2.0);
        }

        if (auto r = interface_cast<CORE_NS::ISetResourceId>(resource)) {
            r->SetResourceId(CORE_NS::ResourceIdContext{"app://create_material.resource"});
        }
        resources->AddResource(resource);
        resources->ExportResourcePayload(resource);

        ASSERT_EQ(resources->Export("app://create_material.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->PurgeResource(CORE_NS::ResourceIdContext{"app://create_material.resource"});
    }

    auto mat = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
    ASSERT_TRUE(mat);

    auto res = resources->GetResource(CORE_NS::ResourceIdContext{"app://create_material.resource"});
    ASSERT_TRUE(res);

    ASSERT_TRUE(SetTemplate(ClassId::MaterialTemplateAccess, res, mat));
    EXPECT_EQ(mat->AlphaCutoff()->GetValue(), 2.0);
}

/**
 * @tc.name: MaterialTemplatePreservesOverride
 * @tc.desc: Applying a material template updates defaults but keeps explicit user overrides,
 *           for both scalar properties and per-texture-slot factors.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, MaterialTemplatePreservesOverride, testing::ext::TestSize.Level1)
{
    const BASE_NS::Math::Vec4 templateFactor{0.1f, 0.2f, 0.3f, 0.4f};
    const BASE_NS::Math::Vec4 overrideFactor{0.9f, 0.9f, 0.9f, 0.9f};

    auto scene = CreateEmptyScene();

    auto access =
        META_NS::GetObjectRegistry().Create<META_NS::IResourceTemplateAccess>(ClassId::MaterialTemplateAccess);
    ASSERT_TRUE(access);

    // Build the template from a live source material so the Textures array is populated.
    auto srcMat = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
    ASSERT_TRUE(srcMat);
    srcMat->AlphaCutoff()->SetValue(2.0f);
    {
        auto st = srcMat->Textures()->GetValueAt(0);
        ASSERT_TRUE(st);
        st->Factor()->SetValue(templateFactor);
    }
    auto templ = access->CreateTemplateFromResource(interface_pointer_cast<CORE_NS::IResource>(srcMat));
    ASSERT_TRUE(templ);

    auto mat = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
    ASSERT_TRUE(mat);

    // Explicit user overrides set before the template is applied.
    mat->AlphaCutoff()->SetValue(0.25f);
    auto tex = mat->Textures()->GetValueAt(0);
    ASSERT_TRUE(tex);
    tex->Factor()->SetValue(overrideFactor);

    ASSERT_TRUE(SetTemplate(ClassId::MaterialTemplateAccess, templ, mat));

    // Scalar override is preserved; only the default slot is updated.
    EXPECT_EQ(mat->AlphaCutoff()->GetValue(), 0.25f);
    EXPECT_FALSE(mat->AlphaCutoff()->IsDefaultValue());
    mat->AlphaCutoff()->ResetValue();
    EXPECT_EQ(mat->AlphaCutoff()->GetValue(), 2.0f);

    // Texture-slot factor override is preserved; only the default slot is updated.
    EXPECT_EQ(tex->Factor()->GetValue(), overrideFactor);
    EXPECT_FALSE(tex->Factor()->IsDefaultValue());
    tex->Factor()->ResetValue();
    EXPECT_EQ(tex->Factor()->GetValue(), templateFactor);
}

/**
 * @tc.name: SaveEnvironmentOptions
 * @tc.desc: Tests for Save Environment Options. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, SaveEnvironmentOptions, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    {
        auto bitmap = CreateTestBitmap();
        ASSERT_TRUE(bitmap);

        auto res = META_NS::CreateObjectResource(ClassId::Environment, ClassId::EnvironmentResourceTemplate);
        ASSERT_TRUE(res);

        auto m = interface_cast<META_NS::IMetadata>(res);
        ASSERT_TRUE(m);
        {
            auto p = m->GetProperty<uint32_t>("RadianceCubemapMipCount", META_NS::MetadataQuery::EXISTING);
            ASSERT_TRUE(p);
            p->SetValue(69);
        }
        {
            auto p = m->GetProperty<IImage::Ptr>("RadianceImage", META_NS::MetadataQuery::EXISTING);
            ASSERT_TRUE(p);
            p->SetValue(bitmap);
        }
        {
            auto p = m->GetProperty<IImage::Ptr>("EnvironmentImage", META_NS::MetadataQuery::EXISTING);
            ASSERT_TRUE(p);
            p->SetValue(bitmap);
        }

        if (auto r = interface_cast<CORE_NS::ISetResourceId>(res)) {
            r->SetResourceId(CORE_NS::ResourceIdContext{"app://save_env_opts.resource"});
        }
        ASSERT_TRUE(resources->AddResource(res));

        ASSERT_EQ(resources->Export("app://save_env_opts.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }

    ASSERT_EQ(resources->Import("app://save_env_opts.resources"), CORE_NS::IResourceManager::Result::OK);

    auto res = resources->GetResource(CORE_NS::ResourceIdContext{"app://save_env_opts.resource"});
    ASSERT_TRUE(res);

    auto env = interface_cast<META_NS::IMetadata>(res);
    ASSERT_TRUE(env);

    EXPECT_EQ(env->GetProperty<uint32_t>("RadianceCubemapMipCount", META_NS::MetadataQuery::EXISTING)->GetValue(), 69);
    EXPECT_TRUE(env->GetProperty<IImage::Ptr>("RadianceImage", META_NS::MetadataQuery::EXISTING)->GetValue());
    EXPECT_TRUE(env->GetProperty<IImage::Ptr>("EnvironmentImage", META_NS::MetadataQuery::EXISTING)->GetValue());
}

/**
 * @tc.name: Camera
 * @tc.desc: Tests for Camera. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, Camera, testing::ext::TestSize.Level1)
{
    {
        auto obj = META_NS::GetObjectRegistry().Create<META_NS::IMetadata>(META_NS::ClassId::Object);
        ASSERT_TRUE(META_NS::CreatePropertiesFromStaticMeta(::SCENE_NS::ClassId::CameraNode, *obj));
        ASSERT_TRUE(Export(obj, "app://camera.json"));
    }
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto cam = scene->CreateNode<ICamera>("//test", ::SCENE_NS::ClassId::CameraNode).GetResult();
        cam->CameraLayerMask()->SetValue(111);
        ASSERT_TRUE(cam->SceneFlags()->GetValue() != int(CameraSceneFlag::MAIN_CAMERA_BIT));
        cam->SceneFlags()->SetValue(int(CameraSceneFlag::MAIN_CAMERA_BIT));
        cam->MSAASampleCount()->SetValue(CameraSampleCount::COUNT_2);

        {
            auto pp = scene->CreateObject<IPostProcess>(ClassId::PostProcess).GetResult();
            ASSERT_TRUE(pp);

            if (auto r = interface_cast<CORE_NS::ISetResourceId>(pp)) {
                r->SetResourceId({CORE_NS::ResourceId{"postprocess"}, scene});
            }

            auto tm = pp->Tonemap()->GetValue();
            ASSERT_TRUE(tm);
            tm->Enabled()->SetValue(false);
            tm->Type()->SetValue(TonemapType::FILMIC);
            tm->Exposure()->SetValue(1.0);

            auto b = pp->Bloom()->GetValue();
            ASSERT_TRUE(b);
            b->Enabled()->SetValue(true);
            b->Type()->SetValue(BloomType::BILATERAL);
            b->Quality()->SetValue(EffectQualityType::HIGH);
            b->ThresholdHard()->SetValue(0.5);
            b->ThresholdSoft()->SetValue(1.0);
            b->AmountCoefficient()->SetValue(0.5);
            b->DirtMaskCoefficient()->SetValue(0.5);
            auto bitmap = CreateTestBitmap();
            b->DirtMaskImage()->SetValue(bitmap);
            b->UseCompute()->SetValue(true);
            b->Scatter()->SetValue(0.5);
            b->ScaleFactor()->SetValue(0.5);

            ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(pp), ""));
            cam->PostProcess()->SetValue(pp);
        }
        if (auto r = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            r->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://cam_test.resource"));
        ASSERT_EQ(resources->Export("app://cam_test.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://cam_test.g-res"), CORE_NS::IResourceManager::Result::OK);

        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://cam_test.g-res"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    auto cam = scene->FindNode<ICamera>("//test").GetResult();
    ASSERT_TRUE(cam);
    EXPECT_EQ(cam->CameraLayerMask()->GetValue(), 111);
    EXPECT_EQ(cam->SceneFlags()->GetValue(), int(CameraSceneFlag::MAIN_CAMERA_BIT));
    EXPECT_EQ(cam->MSAASampleCount()->GetValue(), CameraSampleCount::COUNT_2);

    auto pp = cam->PostProcess()->GetValue();

    auto tm = pp->Tonemap()->GetValue();
    ASSERT_TRUE(tm);
    EXPECT_FALSE(tm->Enabled()->GetValue());
    EXPECT_EQ(tm->Type()->GetValue(), TonemapType::FILMIC);
    EXPECT_FLOAT_EQ(tm->Exposure()->GetValue(), 1.0);

    auto b = pp->Bloom()->GetValue();
    ASSERT_TRUE(b);
    EXPECT_TRUE(b->Enabled()->GetValue());
    EXPECT_EQ(b->Type()->GetValue(), BloomType::BILATERAL);
    EXPECT_EQ(b->Quality()->GetValue(), EffectQualityType::HIGH);
    EXPECT_FLOAT_EQ(b->ThresholdHard()->GetValue(), 0.5);
    EXPECT_FLOAT_EQ(b->ThresholdSoft()->GetValue(), 1.0);
    EXPECT_FLOAT_EQ(b->AmountCoefficient()->GetValue(), 0.5);
    EXPECT_FLOAT_EQ(b->DirtMaskCoefficient()->GetValue(), 0.5);
    EXPECT_TRUE(b->DirtMaskImage()->GetValue());
    EXPECT_TRUE(b->UseCompute()->GetValue());
    EXPECT_FLOAT_EQ(b->Scatter()->GetValue(), 0.5);
    EXPECT_FLOAT_EQ(b->ScaleFactor()->GetValue(), 0.5);

    {
        auto acc = interface_cast<IEcsObjectAccess>(pp);
        ASSERT_TRUE(acc);
        auto ecsobj = acc->GetEcsObject();
        ASSERT_TRUE(ecsobj);
        EXPECT_TRUE(CORE_NS::EntityUtil::IsValid(ecsobj->GetEntity()));
    }
}

/**
 * @tc.name: Light
 * @tc.desc: Tests for Light. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, Light, testing::ext::TestSize.Level1)
{
    {
        auto obj = META_NS::GetObjectRegistry().Create<META_NS::IMetadata>(META_NS::ClassId::Object);
        ASSERT_TRUE(META_NS::CreatePropertiesFromStaticMeta(::SCENE_NS::ClassId::LightNode, *obj));
        ASSERT_TRUE(Export(obj, "app://light.json"));
    }
}

/**
 * @tc.name: EscapedNames
 * @tc.desc: Tests for Escaped Names. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, EscapedNames, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = scene->GetRootNode().GetResult();
        ASSERT_TRUE(root);
        {
            auto scene2 = CreateEmptyScene();
            ASSERT_TRUE(scene2);
            auto n1 = scene2->CreateNode("//..").GetResult();
            ASSERT_TRUE(scene2);
            auto n2 = scene2->CreateNode("//../.te.st[1]").GetResult();
            ASSERT_TRUE(n2);

            if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene2)) {
                i->SetResourceId(CORE_NS::ResourceIdContext{"scene2"});
            }
            ASSERT_TRUE(resources->AddResource(
                interface_pointer_cast<CORE_NS::IResource>(scene2), "app://escaped_names.scene2"));
        }

        if (auto i = interface_cast<INodeImport>(root)) {
            ASSERT_TRUE(i->ImportChildScene(interface_pointer_cast<IScene>(
                                                resources->GetResource(CORE_NS::ResourceIdContext{"scene2"})),
                             "ext")
                            .GetResult());
        }

        auto n = scene->FindNode("//ext/../.te.st[1]").GetResult();
        ASSERT_TRUE(n);
        n->Scale()->SetValue({2.0, 2.0, 2.0});

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://escaped_names.scene"));
        ASSERT_EQ(resources->Export("app://escaped_names.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->PurgeResource(CORE_NS::ResourceIdContext{"scene"});
    }
    ASSERT_EQ(resources->Import("app://escaped_names.resources"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    auto n = scene->FindNode("//ext/../.te.st[1]").GetResult();
    ASSERT_TRUE(n);

    EXPECT_EQ(n->Scale()->GetValue(), (BASE_NS::Math::Vec3{2.0, 2.0, 2.0}));
}

/**
 * @tc.name: SaveImageOptions
 * @tc.desc: Tests for Save Image Options. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, SaveImageOptions, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    {
        auto opts = META_NS::GetObjectRegistry().Create<META_NS::IObjectResourceOptions>(
            META_NS::ClassId::ObjectResourceOptions);
        ASSERT_TRUE(opts);

        ImageLoadInfo info = DEFAULT_IMAGE_LOAD_INFO;
        info.loadFlags = ImageLoadFlags::FORCE_GRAYSCALE_BIT;

        opts->SetProperty("ImageLoadInfo", info);

        EXPECT_TRUE(resources->AddResource(
            CORE_NS::ResourceIdContext{"image"}, ClassId::ImageResource.Id().ToUid(), "test://images/logo.png", opts));

        ASSERT_EQ(resources->Export("app://save_image_opts.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }

    ASSERT_EQ(resources->Import("app://save_image_opts.resources"), CORE_NS::IResourceManager::Result::OK);

    auto res = resources->GetResource(CORE_NS::ResourceIdContext{"image"});
    ASSERT_TRUE(res);

    ImageLoadInfo flags{};
    auto att = interface_cast<META_NS::IAttach>(res);
    ASSERT_TRUE(att);
    auto cont = att->GetAttachmentContainer(true);
    auto opts = cont->FindAny<META_NS::IObjectResourceOptions>("", META_NS::TraversalType::NO_HIERARCHY);
    ASSERT_TRUE(opts);
    auto p = opts->GetProperty<ImageLoadInfo>("ImageLoadInfo");
    ASSERT_TRUE(p);
    flags = p->GetValue();
    EXPECT_EQ(flags.loadFlags, ImageLoadFlags::FORCE_GRAYSCALE_BIT);
    EXPECT_EQ(flags.info.usageFlags, DEFAULT_IMAGE_LOAD_INFO.info.usageFlags);
    EXPECT_EQ(flags.info.memoryFlags, DEFAULT_IMAGE_LOAD_INFO.info.memoryFlags);
    EXPECT_EQ(flags.info.creationFlags, DEFAULT_IMAGE_LOAD_INFO.info.creationFlags);
}

/**
 * @tc.name: Animation
 * @tc.desc: Tests for Animation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, Animation, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);

    auto anims = scene->GetAnimations().GetResult();
    ASSERT_TRUE(!anims.empty());

    auto anim = anims[0];

    if (auto i = interface_cast<CORE_NS::ISetResourceId>(anim)) {
        i->SetResourceId(CORE_NS::ResourceIdContext{"animation", scene});
    }

    ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(anim), ""));

    auto res = resources->GetResource(CORE_NS::ResourceIdContext{"animation", scene});
    ASSERT_TRUE(res);
}

class ITestBehavior : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITestBehavior, "2e4d4345-4c33-4599-96cb-189c6ef83735")
public:
    META_ARRAY_PROPERTY(float, Array)
    META_PROPERTY(INode::WeakPtr, Pointer)
};

META_REGISTER_CLASS(TestBehavior, "67620692-df84-40ef-8fae-f25993375f2f", META_NS::ObjectCategoryBits::NO_CATEGORY, "")

class TestBehavior : public META_NS::Behavior<ITestBehavior> {
    META_OBJECT(TestBehavior, ClassId::TestBehavior, Behavior)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_ARRAY_PROPERTY_DATA(ITestBehavior, float, Array)
    META_STATIC_PROPERTY_DATA(ITestBehavior, INode::WeakPtr, Pointer)
    META_END_STATIC_DATA()

    META_IMPLEMENT_ARRAY_PROPERTY(float, Array)
    META_IMPLEMENT_PROPERTY(INode::WeakPtr, Pointer)

    bool OnAttach() override
    {
        EXPECT_TRUE(Pointer()->GetValue().lock());
        return true;
    }
};

/**
 * @tc.name: Behavior
 * @tc.desc: Tests for Behavior. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, Behavior, testing::ext::TestSize.Level1)
{
    auto& registry = META_NS::GetObjectRegistry();
    registry.RegisterObjectType(TestBehavior::GetFactory());
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto node = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(node);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        auto obj = registry.Create<ITestBehavior>(ClassId::TestBehavior);
        ASSERT_TRUE(obj);
        obj->Array()->SetValue({1.0, 2.0, 4.0});
        obj->Pointer()->SetValue(node);

        if (auto i = interface_cast<META_NS::IAttach>(node)) {
            i->Attach(obj);
        }

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://behaviour_test.scene"));
        ASSERT_EQ(resources->Export("app://behaviour_test.resources"), CORE_NS::IResourceManager::Result::OK);

        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://behaviour_test.resources"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    auto n = scene->FindNode("//test").GetResult();
    ASSERT_TRUE(n);

    auto i = interface_cast<META_NS::IAttach>(n);
    ASSERT_TRUE(i);
    auto vec = i->GetAttachments<ITestBehavior>();
    ASSERT_EQ(vec.size(), 1);
    auto cont = vec.front()->Array()->GetValue();
    ASSERT_EQ(cont.size(), 3);
    EXPECT_FLOAT_EQ(cont[0], 1.0);
    EXPECT_FLOAT_EQ(cont[1], 2.0);
    EXPECT_FLOAT_EQ(cont[2], 4.0);

    registry.UnregisterObjectType(TestBehavior::GetFactory());
}

/**
 * @tc.name: MetaKeyframeAnimation
 * @tc.desc: Tests for Meta Keyframe Animation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, MetaKeyframeAnimation, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto node = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(node);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        auto delay = META_NS::TimeSpan::Milliseconds(1);
        auto property = node->Position();
        auto value = property->GetValue();

        META_NS::KeyframeAnimation<BASE_NS::Math::Vec3> anim(META_NS::CreateNew);

        anim.SetFrom(value).SetTo({10, 10, 10}).SetProperty(property).SetDuration(META_NS::TimeSpan::Milliseconds(100));
        EXPECT_TRUE(anim.GetValid());

        if (auto i = interface_cast<META_NS::IAttach>(node)) {
            i->Attach(anim);
        }

        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(scene), "app://keyframe_animation_test.scene"));
        ASSERT_EQ(resources->Export("app://keyframe_animation_test.resources"), CORE_NS::IResourceManager::Result::OK);

        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://keyframe_animation_test.resources"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    auto n = scene->FindNode("//test").GetResult();
    ASSERT_TRUE(n);

    EXPECT_EQ(n->Position()->GetValue(), (BASE_NS::Math::Vec3{0, 0, 0}));

    auto i = interface_cast<META_NS::IAttach>(n);
    ASSERT_TRUE(i);
    auto vec = i->GetAttachments<META_NS::IAnimation>();
    ASSERT_EQ(vec.size(), 1);
    auto anim = vec[0];
    ASSERT_TRUE(anim);
    interface_cast<META_NS::IStartableAnimation>(anim)->Start();
    UpdateScene();
    UpdateScene(META_NS::TimeSpan::Milliseconds(100));
    EXPECT_EQ(n->Position()->GetValue(), (BASE_NS::Math::Vec3{10, 10, 10}));
}

/**
 * @tc.name: MetaTrackAnimation
 * @tc.desc: Tests for Meta Track Animation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, MetaTrackAnimation, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto node = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(node);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        auto delay = META_NS::TimeSpan::Milliseconds(1);
        auto property = node->Scale();
        auto value = property->GetValue();

        BASE_NS::vector<float> timestamps = {0.0f, 0.5f, 1.f};
        BASE_NS::vector<BASE_NS::Math::Vec3> keyframes = {{10, 10, 10}, {50, 50, 50}, {100, 100, 100}};

        META_NS::TrackAnimation<BASE_NS::Math::Vec3> anim(META_NS::CreateNew);
        anim.SetKeyframes(keyframes)
            .SetTimestamps(timestamps)
            .SetProperty(property)
            .SetDuration(META_NS::TimeSpan::Milliseconds(100));

        ASSERT_TRUE(anim.GetValid());

        if (auto i = interface_cast<META_NS::IAttach>(node)) {
            i->Attach(anim);
        }

        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(scene), "app://track_animation_test.scene"));
        ASSERT_EQ(resources->Export("app://track_animation_test.resources"), CORE_NS::IResourceManager::Result::OK);

        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://track_animation_test.resources"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    auto n = scene->FindNode("//test").GetResult();
    ASSERT_TRUE(n);

    EXPECT_EQ(n->Position()->GetValue(), (BASE_NS::Math::Vec3{0, 0, 0}));

    auto i = interface_cast<META_NS::IAttach>(n);
    ASSERT_TRUE(i);
    auto vec = i->GetAttachments<META_NS::IAnimation>();
    ASSERT_EQ(vec.size(), 1);
    auto anim = vec[0];
    ASSERT_TRUE(anim);
    EXPECT_TRUE(anim->Valid()->GetValue());
    interface_cast<META_NS::IStartableAnimation>(anim)->Start();
    UpdateScene();
    UpdateScene(META_NS::TimeSpan::Milliseconds(100));
    EXPECT_EQ(n->Scale()->GetValue(), (BASE_NS::Math::Vec3{100, 100, 100}));
}

/**
 * @tc.name: MetaTrackAnimationAsResource
 * @tc.desc: Tests for Meta Track Animation As Resource. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, MetaTrackAnimationAsResource, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto node = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(node);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        auto delay = META_NS::TimeSpan::Milliseconds(1);
        auto property = node->Scale();
        auto value = property->GetValue();

        BASE_NS::vector<float> timestamps = {0.0f, 0.5f, 1.f};
        BASE_NS::vector<BASE_NS::Math::Vec3> keyframes = {{10, 10, 10}, {50, 50, 50}, {100, 100, 100}};

        META_NS::TrackAnimation<BASE_NS::Math::Vec3> anim(META_NS::CreateNew);
        anim.SetKeyframes(keyframes)
            .SetTimestamps(timestamps)
            .SetProperty(property)
            .SetDuration(META_NS::TimeSpan::Milliseconds(100));

        ASSERT_TRUE(anim.GetValid());

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(anim)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"animation", scene});
        }

        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(scene), "app://track_animation_res_test.scene"));
        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(anim), "app://track_animation_res_test.anim"));
        ASSERT_EQ(
            resources->Export("app://track_animation_res_test.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://track_animation_res_test.g-res"), CORE_NS::IResourceManager::Result::OK);

        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://track_animation_res_test.g-res"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    auto n = scene->FindNode("//test").GetResult();
    ASSERT_TRUE(n);

    EXPECT_EQ(n->Position()->GetValue(), (BASE_NS::Math::Vec3{0, 0, 0}));

    auto anim = interface_pointer_cast<META_NS::IAnimation>(
        resources->GetResource(CORE_NS::ResourceIdContext{"animation", scene}));
    ASSERT_TRUE(anim);
    EXPECT_TRUE(anim->Valid()->GetValue());
    interface_cast<META_NS::IStartableAnimation>(anim)->Start();
    UpdateScene();
    UpdateScene(META_NS::TimeSpan::Milliseconds(100));
    EXPECT_EQ(n->Scale()->GetValue(), (BASE_NS::Math::Vec3{100, 100, 100}));
}

/**
 * @tc.name: WeakRefFromOutside
 * @tc.desc: Tests for Weak Ref From Outside. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, WeakRefFromOutside, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto node = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(node);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://weakref_test.scene"));
        ASSERT_EQ(resources->Export("app://weakref_test.resources"), CORE_NS::IResourceManager::Result::OK);

        META_NS::Object obj(META_NS::CreateNew);
        META_NS::AttachmentContainer(obj).Attach(scene);
        auto prop = META_NS::ConstructProperty<INode::WeakPtr>("node", node);
        META_NS::Metadata(obj).AddProperty(prop);

        ASSERT_TRUE(Export(obj, "app://weakref_test.json"));
        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://weakref_test.resources"), CORE_NS::IResourceManager::Result::OK);
    auto obj = Import<META_NS::IObject>("app://weakref_test.json");
    ASSERT_TRUE(obj);

    auto m = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(m);
    auto p = m->GetProperty<INode::WeakPtr>("node");
    ASSERT_TRUE(p);
    EXPECT_TRUE(p->GetValue().lock());

    auto i = interface_cast<META_NS::IAttach>(obj);
    ASSERT_TRUE(i);
    auto vec = i->GetAttachments<IScene>();
    ASSERT_EQ(vec.size(), 1);

    EXPECT_EQ(vec.front()->FindNode("//test").GetResult(), p->GetValue().lock());
}

/**
 * @tc.name: ReferenceToTexture
 * @tc.desc: Tests for Reference To Texture. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, ReferenceToTexture, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = scene->GetRootNode().GetResult();
        ASSERT_TRUE(root);

        auto camera =
            interface_pointer_cast<ICamera>(scene->CreateNode("//camera", ::SCENE_NS::ClassId::CameraNode).GetResult());
        ASSERT_TRUE(camera);

        {
            ASSERT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"scene2"},
                ::SCENE_NS::ClassId::GltfSceneResource.Id().ToUid(),
                "test://AnimatedCube/AnimatedCube.gltf"));

            if (auto i = interface_cast<INodeImport>(root)) {
                ASSERT_TRUE(i->ImportChildScene(interface_pointer_cast<IScene>(
                                                    resources->GetResource(CORE_NS::ResourceIdContext{"scene2"})),
                                 "test")
                                .GetResult());
            }
        }

        auto n = scene->FindNode<IMesh>("//test//AnimatedCube").GetResult();
        ASSERT_TRUE(n);

        auto subs = n->SubMeshes()->GetValue();
        ASSERT_TRUE(!subs.empty());

        auto material = scene->CreateObject<IMaterial>(::SCENE_NS::ClassId::Material).GetResult();
        ASSERT_TRUE(material);

        if (auto r = interface_cast<CORE_NS::ISetResourceId>(material)) {
            r->SetResourceId(CORE_NS::ResourceIdContext{CORE_NS::ResourceId{"material"}, scene});
        }

        material->MaterialShader()->SetValue(CreateTestShader());
        auto t1 = material->Textures()->GetValueAt(0);
        ASSERT_TRUE(t1);

        t1->Rotation()->SetValue(1.0f);

        auto inputs = material->CustomProperties()->GetValue();
        ASSERT_TRUE(inputs);
        auto prop = inputs->GetProperty<BASE_NS::Math::Vec4>("v4_");
        ASSERT_TRUE(prop);
        prop->SetValue({1.0, 1.0, 1.0, 1.0});

        subs[0]->Material()->SetValue(material);

        auto rconfig = scene->RenderConfiguration()->GetValue();
        ASSERT_TRUE(rconfig);
        // Workaround for restoring RenderConfigurationComponent::RenderNodeGraphUri at deserialization time
        rconfig->RenderNodeGraphUri()->SetValue(".");

        auto pp = scene->CreateObject<IPostProcess>(::SCENE_NS::ClassId::PostProcess).GetResult();
        ASSERT_TRUE(pp);

        if (auto r = interface_cast<CORE_NS::ISetResourceId>(pp)) {
            r->SetResourceId({CORE_NS::ResourceId{"postprocess"}, scene});
        }
        camera->PostProcess()->SetValue(pp);

        auto tonemap = pp->Tonemap()->GetValue();
        auto bloom = pp->Bloom()->GetValue();

        META_NS::Object obj(META_NS::CreateNew);
        auto prop1 = META_NS::ConstructProperty<META_NS::IProperty::WeakPtr>("tex", t1->Rotation());
        auto prop2 = META_NS::ConstructProperty<META_NS::IProperty::WeakPtr>("custom", prop);
        auto prop3 = META_NS::ConstructProperty<META_NS::IProperty::WeakPtr>("rconfig", rconfig->RenderNodeGraphUri());
        auto prop4 = META_NS::ConstructProperty<META_NS::IProperty::WeakPtr>("tonemap", tonemap->Type());
        auto prop5 = META_NS::ConstructProperty<META_NS::IProperty::WeakPtr>("bloom", bloom->Scatter());
        META_NS::Metadata(obj).AddProperty(prop1);
        META_NS::Metadata(obj).AddProperty(prop2);
        META_NS::Metadata(obj).AddProperty(prop3);
        META_NS::Metadata(obj).AddProperty(prop4);
        META_NS::Metadata(obj).AddProperty(prop5);

        META_NS::AttachmentContainer(scene).Attach(obj);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(material), ""));
        ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(pp), ""));
        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://ref_tex_test.scene"));
        ASSERT_EQ(resources->Export("app://ref_tex_test.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://ref_tex_test.g-res"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://ref_tex_test.g-res"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    auto i = interface_cast<META_NS::IAttach>(scene);
    ASSERT_TRUE(i);
    auto vec = i->GetAttachments();
    META_NS::IObject::Ptr obj;
    for (auto&& v : i->GetAttachments()) {
        if (!interface_cast<META_NS::IProperty>(v)) {
            obj = v;
        }
    }
    ASSERT_TRUE(obj);

    auto m = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(m);
    auto p1 = m->GetProperty<META_NS::IProperty::WeakPtr>("tex");
    ASSERT_TRUE(p1);
    EXPECT_TRUE(p1->GetValue().lock());

    auto p2 = m->GetProperty<META_NS::IProperty::WeakPtr>("custom");
    ASSERT_TRUE(p2);
    EXPECT_TRUE(p2->GetValue().lock());

    auto p3 = m->GetProperty<META_NS::IProperty::WeakPtr>("rconfig");
    ASSERT_TRUE(p3);
    EXPECT_TRUE(p3->GetValue().lock());

    auto p4 = m->GetProperty<META_NS::IProperty::WeakPtr>("tonemap");
    ASSERT_TRUE(p4);
    EXPECT_TRUE(p4->GetValue().lock());

    auto p5 = m->GetProperty<META_NS::IProperty::WeakPtr>("bloom");
    ASSERT_TRUE(p5);
    EXPECT_TRUE(p5->GetValue().lock());

    auto n = scene->FindNode<IMesh>("//test//AnimatedCube").GetResult();
    ASSERT_TRUE(n);

    auto subs = n->SubMeshes()->GetValue();
    ASSERT_TRUE(!subs.empty());
    auto mat = subs[0]->Material()->GetValue();
    ASSERT_TRUE(mat);

    auto inputs = mat->CustomProperties()->GetValue();
    ASSERT_TRUE(inputs);
    auto prop = inputs->GetProperty<BASE_NS::Math::Vec4>("v4_");
    ASSERT_TRUE(prop);
    EXPECT_EQ(prop->GetValue(), (BASE_NS::Math::Vec4{1.0, 1.0, 1.0, 1.0}));
    EXPECT_EQ(prop.GetProperty(), p2->GetValue().lock());

    auto t1 = mat->Textures()->GetValueAt(0);
    ASSERT_TRUE(t1);
    EXPECT_EQ(t1->Rotation()->GetValue(), 1.0f);
    EXPECT_EQ(t1->Rotation().GetProperty(), p1->GetValue().lock());

    auto rconfig = scene->RenderConfiguration()->GetValue();
    ASSERT_TRUE(rconfig);
    EXPECT_EQ(rconfig->RenderNodeGraphUri().GetProperty(), p3->GetValue().lock());

    auto cam = scene->FindNode<ICamera>("//camera").GetResult();
    ASSERT_TRUE(cam);

    auto tonemap = cam->PostProcess()->GetValue()->Tonemap()->GetValue();
    EXPECT_EQ(tonemap->Type().GetProperty(), p4->GetValue().lock());
    auto bloom = cam->PostProcess()->GetValue()->Bloom()->GetValue();
    EXPECT_EQ(bloom->Scatter().GetProperty(), p5->GetValue().lock());
}

/**
 * @tc.name: DupNodeNames
 * @tc.desc: Tests for Dup Node Names. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, DupNodeNames, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto node1 = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(node1);

        auto node2 = scene->CreateNode("//other").GetResult();
        ASSERT_TRUE(node2);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        auto named = interface_cast<META_NS::INamed>(node2);
        named->Name()->SetValue("test");

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://dup_node_names.scene"));
        ASSERT_EQ(resources->Export("app://dup_node_names.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://dup_node_names.resources"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    auto root = scene->GetRootNode().GetResult();
    auto cont = interface_cast<META_NS::IContainer>(root);
    auto vec = cont->GetAll();

    ASSERT_EQ(vec.size(), 2);
}

/**
 * @tc.name: MaterialOptionsFromImported
 * @tc.desc: Tests for Material Options From Imported. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, MaterialOptionsFromImported, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = scene->GetRootNode().GetResult();
        {
            ASSERT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"scene2"},
                ::SCENE_NS::ClassId::GltfSceneResource.Id().ToUid(),
                "test://AnimatedCube/AnimatedCube.gltf"));

            if (auto i = interface_cast<INodeImport>(root)) {
                ASSERT_TRUE(i->ImportChildScene(interface_pointer_cast<IScene>(
                                                    resources->GetResource(CORE_NS::ResourceIdContext{"scene2"})),
                                 "ext")
                                .GetResult());
            }

            EXPECT_TRUE(!resources->GetResourceInfos({"scene2"}, scene).empty());
        }

        auto n = scene->FindNode<IMesh>("//ext//AnimatedCube").GetResult();
        ASSERT_TRUE(n);

        auto subs = n->SubMeshes()->GetValue();
        ASSERT_TRUE(!subs.empty());

        auto material = subs[0]->Material()->GetValue();
        ASSERT_TRUE(material);

        EXPECT_EQ(META_NS::GetName(material), "AnimatedCube");

        // this will serialise in to the resource options
        material->Type()->SetValue(MaterialType::CUSTOM);
        material->MaterialShader()->SetValue(CreateTestShader());
        material->Textures()->GetValue()[0]->Factor()->SetValue({1, 0, 0, 0});

        subs[0]->Material()->SetValue(material);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }
        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(scene), "app://mat_opt_imported_test.scene"));
        ASSERT_EQ(resources->Export("app://mat_opt_imported_test.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://mat_opt_imported_test.g-res"), CORE_NS::IResourceManager::Result::OK);

        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://mat_opt_imported_test.g-res"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    // make sure the import purges the external scene used for external node
    EXPECT_TRUE(resources->GetResourceInfos({"scene2"}, nullptr).empty());

    auto n = scene->FindNode<IMesh>("//ext//AnimatedCube").GetResult();
    ASSERT_TRUE(n);

    UpdateScene(scene);

    // instantiating resource applies the options to it, so we have to check it straight from ecs
    {
        auto meshacc = interface_cast<IMeshAccess>(n);
        auto ecsObject = interface_cast<IEcsObjectAccess>(meshacc->GetMesh().GetResult())->GetEcsObject();
        auto ecs = scene->GetInternalScene()->GetEcsContext().GetNativeEcs();
        auto meshM = CORE_NS::GetManager<CORE3D_NS::IMeshComponentManager>(*ecs);
        auto materialM = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(*ecs);
        auto rmanager = CORE_NS::GetManager<CORE3D_NS::IRenderHandleComponentManager>(*ecs);

        auto m = meshM->Read(ecsObject->GetEntity());
        ASSERT_TRUE(m);
        ASSERT_TRUE(!m->submeshes.empty());

        auto mat = materialM->Read(m->submeshes[0].material);
        ASSERT_TRUE(mat);
        EXPECT_EQ(mat->type, CORE3D_NS::MaterialComponent::Type::CUSTOM);

        auto shader = resources->GetResource({"shader", nullptr});
        ASSERT_TRUE(shader);
        auto renderRes = interface_cast<IRenderResource>(shader);
        ASSERT_TRUE(renderRes);
        auto graphicsState = interface_cast<IGraphicsState>(shader);
        ASSERT_TRUE(graphicsState);
        EXPECT_EQ(renderRes->GetRenderHandle().GetHandle(),
            rmanager->GetRenderHandleReference(mat->materialShader.shader).GetHandle());
        EXPECT_EQ(graphicsState->GetGraphicsState().GetHandle(),
            rmanager->GetRenderHandleReference(mat->materialShader.graphicsState).GetHandle());

        auto nameM = CORE_NS::GetManager<CORE3D_NS::INameComponentManager>(*ecs);
        auto n = nameM->Read(m->submeshes[0].material);
        ASSERT_TRUE(n);
        EXPECT_EQ(n->name, "AnimatedCube");
    }

    auto sm = n->SubMeshes()->GetValueAt(0);
    ASSERT_TRUE(sm);
    auto mat = sm->Material()->GetValue();
    ASSERT_TRUE(mat);
    EXPECT_EQ(mat->Type()->GetValue(), MaterialType::CUSTOM);
    EXPECT_EQ(META_NS::GetName(mat), "AnimatedCube");
}

/**
 * @tc.name: MaterialOptionsFromImportedTwice
 * @tc.desc: Tests for Material Options From Imported Twice. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, MaterialOptionsFromImportedTwice, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto root = scene->GetRootNode().GetResult();
        {
            ASSERT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"scene2"},
                ::SCENE_NS::ClassId::GltfSceneResource.Id().ToUid(),
                "test://AnimatedCube/AnimatedCube.gltf"));

            if (auto i = interface_cast<INodeImport>(root)) {
                ASSERT_TRUE(i->ImportChildScene(interface_pointer_cast<IScene>(
                                                    resources->GetResource(CORE_NS::ResourceIdContext{"scene2"})),
                                 "ext1")
                                .GetResult());

                ASSERT_TRUE(i->ImportChildScene(interface_pointer_cast<IScene>(
                                                    resources->GetResource(CORE_NS::ResourceIdContext{"scene2"})),
                                 "ext2")
                                .GetResult());
            }

            EXPECT_TRUE(!resources->GetResourceInfos({"scene2"}, scene).empty());
        }

        {
            auto n = scene->FindNode<IMesh>("//ext1//AnimatedCube").GetResult();
            ASSERT_TRUE(n);

            auto subs = n->SubMeshes()->GetValue();
            ASSERT_TRUE(!subs.empty());

            auto material = subs[0]->Material()->GetValue();
            ASSERT_TRUE(material);

            // this will serialise in to the resource options
            material->Type()->SetValue(MaterialType::CUSTOM);
        }
        {
            auto n = scene->FindNode<IMesh>("//ext2//AnimatedCube").GetResult();
            ASSERT_TRUE(n);

            auto subs = n->SubMeshes()->GetValue();
            ASSERT_TRUE(!subs.empty());

            auto material = subs[0]->Material()->GetValue();
            ASSERT_TRUE(material);

            // this will serialise in to the resource options
            material->Type()->SetValue(MaterialType::CUSTOM);
        }

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }
        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(scene), "app://mat_opt_imported_twice_test.scene"));
        ASSERT_EQ(
            resources->Export("app://mat_opt_imported_twice_test.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://mat_opt_imported_twice_test.g-res"), CORE_NS::IResourceManager::Result::OK);

        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://mat_opt_imported_twice_test.g-res"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    UpdateScene(scene);
    {
        auto n = scene->FindNode<IMesh>("//ext1//AnimatedCube").GetResult();
        ASSERT_TRUE(n);

        // instantiating resource applies the options to it, so we have to check it straight from ecs
        {
            auto meshacc = interface_cast<IMeshAccess>(n);
            auto ecsObject = interface_cast<IEcsObjectAccess>(meshacc->GetMesh().GetResult())->GetEcsObject();
            auto ecs = scene->GetInternalScene()->GetEcsContext().GetNativeEcs();
            auto meshM = CORE_NS::GetManager<CORE3D_NS::IMeshComponentManager>(*ecs);
            auto materialM = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(*ecs);
            auto m = meshM->Read(ecsObject->GetEntity());
            ASSERT_TRUE(m);
            ASSERT_TRUE(!m->submeshes.empty());

            auto mat = materialM->Read(m->submeshes[0].material);
            ASSERT_TRUE(mat);
            EXPECT_EQ(mat->type, CORE3D_NS::MaterialComponent::Type::CUSTOM);
        }

        auto sm = n->SubMeshes()->GetValueAt(0);
        ASSERT_TRUE(sm);
        auto mat = sm->Material()->GetValue();
        ASSERT_TRUE(mat);
        EXPECT_EQ(mat->Type()->GetValue(), MaterialType::CUSTOM);
    }
    {
        auto n = scene->FindNode<IMesh>("//ext2//AnimatedCube").GetResult();
        ASSERT_TRUE(n);

        // instantiating resource applies the options to it, so we have to check it straight from ecs
        {
            auto meshacc = interface_cast<IMeshAccess>(n);
            auto ecsObject = interface_cast<IEcsObjectAccess>(meshacc->GetMesh().GetResult())->GetEcsObject();
            auto ecs = scene->GetInternalScene()->GetEcsContext().GetNativeEcs();
            auto meshM = CORE_NS::GetManager<CORE3D_NS::IMeshComponentManager>(*ecs);
            auto materialM = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(*ecs);
            auto m = meshM->Read(ecsObject->GetEntity());
            ASSERT_TRUE(m);
            ASSERT_TRUE(!m->submeshes.empty());

            auto mat = materialM->Read(m->submeshes[0].material);
            ASSERT_TRUE(mat);
            EXPECT_EQ(mat->type, CORE3D_NS::MaterialComponent::Type::CUSTOM);
        }

        auto sm = n->SubMeshes()->GetValueAt(0);
        ASSERT_TRUE(sm);
        auto mat = sm->Material()->GetValue();
        ASSERT_TRUE(mat);
        EXPECT_EQ(mat->Type()->GetValue(), MaterialType::CUSTOM);
    }
}

/**
 * @tc.name: NodeHierarchyDuplicateImport
 * @tc.desc: Tests for Node Hierarchy Duplicate Import. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, NodeHierarchyDuplicateImport, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto test = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(test);
    auto some = scene->CreateNode("//test/some").GetResult();
    ASSERT_TRUE(some);
    ASSERT_TRUE(scene->CreateNode("//test/some/other").GetResult());

    auto f = GetTestEnv()->engine->GetFileManager().CreateFile("app://dup_test.json");
    ASSERT_TRUE(f);

    auto exporter = META_NS::GetObjectRegistry().Create<ISceneExporter>(::SCENE_NS::ClassId::SceneExporter);
    ASSERT_TRUE(exporter->ExportNode(*f, some));

    f->Seek(0);

    auto importer = META_NS::GetObjectRegistry().Create<ISceneImporter>(::SCENE_NS::ClassId::SceneImporter);
    auto n = importer->ImportNode(*f, test);
    ASSERT_TRUE(n);
    ASSERT_TRUE(n->GetNode());

    EXPECT_EQ(test->GetChildren().GetResult().size(), 2);
}

/**
 * @tc.name: NodeHierarchyImportRename
 * @tc.desc: Tests for Node Hierarchy Import Rename. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, NodeHierarchyImportRename, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto test = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(test);
    auto some = scene->CreateNode("//test/some").GetResult();
    ASSERT_TRUE(some);
    ASSERT_TRUE(scene->CreateNode("//test/some/other").GetResult());

    auto f = GetTestEnv()->engine->GetFileManager().CreateFile("app://dup_test.json");
    ASSERT_TRUE(f);

    auto exporter = META_NS::GetObjectRegistry().Create<ISceneExporter>(::SCENE_NS::ClassId::SceneExporter);
    ASSERT_TRUE(exporter->ExportNode(*f, some));

    f->Seek(0);

    auto importer = META_NS::GetObjectRegistry().Create<ISceneImporter>(::SCENE_NS::ClassId::SceneImporter);
    NodeImportOptions opts;
    opts.name = test->GetUniqueChildName(some->GetName()).GetResult();
    auto n = importer->ImportNode(*f, test, opts);
    ASSERT_TRUE(n);
    ASSERT_TRUE(n->GetNode());

    EXPECT_EQ(test->GetChildren().GetResult().size(), 2);
    EXPECT_TRUE(scene->FindNode("//test/" + opts.name).GetResult());
    // PrintHierarchy(scene->GetRootNode().GetResult());
}

/**
 * @tc.name: NodeHierarchyImportExtRename
 * @tc.desc: Tests for Node Hierarchy Import Ext Rename. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, NodeHierarchyImportExtRename, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto test = scene->CreateNode("//test").GetResult();
    ASSERT_TRUE(test);

    ASSERT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"scene2"},
        ::SCENE_NS::ClassId::GltfSceneResource.Id().ToUid(),
        "test://AnimatedCube/AnimatedCube.gltf"));

    INode::Ptr node;
    if (auto i = interface_cast<INodeImport>(test)) {
        node = i->ImportChildScene(
                    interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene2"})), "ext")
                   .GetResult();
        ASSERT_TRUE(node);
    }

    auto f = GetTestEnv()->engine->GetFileManager().CreateFile("app://dup_test.json");
    ASSERT_TRUE(f);

    auto exporter = META_NS::GetObjectRegistry().Create<ISceneExporter>(::SCENE_NS::ClassId::SceneExporter);
    ASSERT_TRUE(exporter->ExportNode(*f, node));

    f->Seek(0);

    auto importer = META_NS::GetObjectRegistry().Create<ISceneImporter>(::SCENE_NS::ClassId::SceneImporter);
    NodeImportOptions opts;
    opts.name = test->GetUniqueChildName(node->GetName()).GetResult();
    auto n = importer->ImportNode(*f, test, opts);
    ASSERT_TRUE(n);
    ASSERT_TRUE(n->GetNode());

    EXPECT_EQ(test->GetChildren().GetResult().size(), 2);
    EXPECT_TRUE(scene->FindNode("//test/" + opts.name).GetResult());
    // PrintHierarchy(scene->GetRootNode().GetResult());
}

static BASE_NS::vector<CORE_NS::ResourceIdContext> GetSceneAnimations(const IScene::Ptr& scene)
{
    BASE_NS::vector<CORE_NS::ResourceIdContext> anims;
    auto resources = GetResourceManager(scene);
    for (auto&& v : resources->GetResourceInfos(scene)) {
        if (v.type == ::SCENE_NS::ClassId::AnimationResource.Id().ToUid()) {
            anims.push_back({v.id, scene});
        }
    }
    return anims;
}

/**
 * @tc.name: AnimationResourcesWithSer
 * @tc.desc: Tests count of animation resources after res. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, AnimationResourcesWithSer, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);
        auto test1 = scene->CreateNode("//test1").GetResult();
        ASSERT_TRUE(test1);
        auto test2 = scene->CreateNode("//test2").GetResult();
        ASSERT_TRUE(test2);

        ASSERT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"scene2"},
            ::SCENE_NS::ClassId::GltfSceneResource.Id().ToUid(),
            "test://AnimatedCube/AnimatedCube.gltf"));

        {
            auto i = interface_cast<INodeImport>(test1);
            i->ImportChildScene(
                 interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene2"})), "ext")
                .GetResult();
        }
        {
            auto i = interface_cast<INodeImport>(test2);
            i->ImportChildScene(
                 interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene2"})), "ext")
                .GetResult();
        }

        ASSERT_EQ(GetSceneAnimations(scene).size(), 2);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }
        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://anim_res_with_ser.scene"));
        ASSERT_EQ(resources->Export("app://anim_res_with_ser.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://anim_res_with_ser.g-res"), CORE_NS::IResourceManager::Result::OK);

        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://anim_res_with_ser.g-res"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    ASSERT_EQ(GetSceneAnimations(scene).size(), 2);
}

UNIT_TEST_F(API_SceneSerialisationTest, GenericComponent, testing::ext::TestSize.Level1)
{
    float defaultValue{};
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        auto node = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(node);

        {
            // use component that is not built-in to scene
            ASSERT_FALSE(scene->GetInternalScene()->GetEcsContext().FindComponent("FogComponent"));
            auto component = scene->CreateComponent(node, "FogComponent").GetResult();
            ASSERT_TRUE(component);
            auto md = interface_cast<META_NS::IMetadata>(component);
            ASSERT_TRUE(md);
            auto p = md->GetProperty<float>("FogComponent.density");
            ASSERT_TRUE(p);
            defaultValue = p->GetDefaultValue();
            p->SetValue(1.0);
        }
        {
            auto root = scene->GetRootNode().GetResult();
            // use component that is not built-in to scene
            auto component = scene->CreateComponent(root, "FogComponent").GetResult();
            ASSERT_TRUE(component);
            auto md = interface_cast<META_NS::IMetadata>(component);
            ASSERT_TRUE(md);
            auto p = md->GetProperty<float>("FogComponent.density");
            ASSERT_TRUE(p);
            p->SetValue(1.0);
        }

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://generic_comp.scene"));
        ASSERT_EQ(resources->Export("app://generic_comp.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }
    // first import
    {
        ASSERT_EQ(resources->Import("app://generic_comp.resources"), CORE_NS::IResourceManager::Result::OK);

        auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
        ASSERT_TRUE(scene);

        auto n = scene->FindNode("//test").GetResult();
        ASSERT_TRUE(n);

        auto component = scene->GetComponent(n, "FogComponent");
        ASSERT_TRUE(component);
        auto md = interface_cast<META_NS::IMetadata>(component);
        ASSERT_TRUE(md);
        auto p = md->GetProperty<float>("FogComponent.density");
        ASSERT_TRUE(p);
        EXPECT_FLOAT_EQ(p->GetValue(), 1.0);
        EXPECT_FALSE(p->IsDefaultValue());

        component->PopulateAllProperties();
        EXPECT_FLOAT_EQ(p->GetValue(), 1.0);
        EXPECT_FLOAT_EQ(p->GetDefaultValue(), defaultValue);
        EXPECT_FALSE(p->IsDefaultValue());
    }
    {
        // export again
        ASSERT_EQ(resources->Export("app://generic_comp.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }
    // second import
    {
        ASSERT_EQ(resources->Import("app://generic_comp.resources"), CORE_NS::IResourceManager::Result::OK);

        auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
        ASSERT_TRUE(scene);

        {
            auto n = scene->FindNode("//test").GetResult();
            ASSERT_TRUE(n);

            auto component = scene->GetComponent(n, "FogComponent");
            ASSERT_TRUE(component);
            auto md = interface_cast<META_NS::IMetadata>(component);
            ASSERT_TRUE(md);
            auto p = md->GetProperty<float>("FogComponent.density");
            ASSERT_TRUE(p);
            EXPECT_FLOAT_EQ(p->GetValue(), 1.0);
            EXPECT_FALSE(p->IsDefaultValue());
        }
        {
            auto root = scene->GetRootNode().GetResult();
            ASSERT_TRUE(root);
            auto component = scene->GetComponent(root, "FogComponent");
            ASSERT_TRUE(component);
            auto md = interface_cast<META_NS::IMetadata>(component);
            ASSERT_TRUE(md);
            auto p = md->GetProperty<float>("FogComponent.density");
            ASSERT_TRUE(p);
            EXPECT_FLOAT_EQ(p->GetValue(), 1.0);
            EXPECT_FALSE(p->IsDefaultValue());
        }
    }
}

UNIT_TEST_F(API_SceneSerialisationTest, GenericComponentExternalNode, testing::ext::TestSize.Level1)
{
    float defaultValue{};
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        ASSERT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"scene2"},
            ::SCENE_NS::ClassId::GltfSceneResource.Id().ToUid(),
            "test://AnimatedCube/AnimatedCube.gltf"));

        {
            auto i = interface_cast<INodeImport>(scene->GetRootNode().GetResult());
            auto node = i->ImportChildScene(interface_pointer_cast<IScene>(
                                                resources->GetResource(CORE_NS::ResourceIdContext{"scene2"})),
                             "test")
                            .GetResult();

            ASSERT_TRUE(node);
            // use component that is not built-in to scene
            ASSERT_FALSE(scene->GetInternalScene()->GetEcsContext().FindComponent("FogComponent"));
            auto component = scene->CreateComponent(node, "FogComponent").GetResult();
            ASSERT_TRUE(component);
            auto md = interface_cast<META_NS::IMetadata>(component);
            ASSERT_TRUE(md);
            auto p = md->GetProperty<float>("FogComponent.density");
            ASSERT_TRUE(p);
            defaultValue = p->GetDefaultValue();
            p->SetValue(1.0);
        }
        {
            auto node = scene->FindNode("//test//AnimatedCube").GetResult();
            ASSERT_TRUE(node);
            auto component = scene->CreateComponent(node, "FogComponent").GetResult();
            ASSERT_TRUE(component);
            auto md = interface_cast<META_NS::IMetadata>(component);
            ASSERT_TRUE(md);
            auto p = md->GetProperty<float>("FogComponent.density");
            ASSERT_TRUE(p);
            p->SetValue(1.0);
        }

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://generic_comp_ext.scene"));
        ASSERT_EQ(resources->Export("app://generic_comp_ext.res", scene), CORE_NS::IResourceManager::Result::OK);
        ASSERT_EQ(resources->Export("app://generic_comp_ext.g-res"), CORE_NS::IResourceManager::Result::OK);

        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://generic_comp_ext.g-res"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    {
        auto n = scene->FindNode("//test").GetResult();
        ASSERT_TRUE(n);

        auto component = scene->GetComponent(n, "FogComponent");
        ASSERT_TRUE(component);
        auto md = interface_cast<META_NS::IMetadata>(component);
        ASSERT_TRUE(md);
        auto p = md->GetProperty<float>("FogComponent.density");
        ASSERT_TRUE(p);
        EXPECT_FLOAT_EQ(p->GetValue(), 1.0);
        EXPECT_FALSE(p->IsDefaultValue());
    }
    {
        auto n = scene->FindNode("//test//AnimatedCube").GetResult();
        ASSERT_TRUE(n);

        auto component = scene->GetComponent(n, "FogComponent");
        ASSERT_TRUE(component);
        auto md = interface_cast<META_NS::IMetadata>(component);
        ASSERT_TRUE(md);
        auto p = md->GetProperty<float>("FogComponent.density");
        ASSERT_TRUE(p);
        EXPECT_FLOAT_EQ(p->GetValue(), 1.0);
        EXPECT_FALSE(p->IsDefaultValue());
    }
}

/**
 * @tc.name: NodeSerializeFlag
 * @tc.desc: Tests for Object Serialize flag for node.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, NodeSerializeFlag, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);
        auto test = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(test);
        auto some = scene->CreateNode("//test/some").GetResult();
        ASSERT_TRUE(some);
        ASSERT_TRUE(scene->CreateNode("//test/some/other").GetResult());

        EXPECT_TRUE(META_NS::IsFlagSet(some, META_NS::ObjectFlagBits::SERIALIZE));
        META_NS::SetObjectFlags(some, META_NS::ObjectFlagBits::SERIALIZE, false);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://ser_node_test.scene"));
        ASSERT_EQ(resources->Export("app://ser_node_test.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://ser_node_test.resources"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    EXPECT_TRUE(scene->FindNode("//test").GetResult());
    EXPECT_FALSE(scene->FindNode("//test/some").GetResult());
    EXPECT_FALSE(scene->FindNode("//test/some/other").GetResult());
}

/**
 * @tc.name: NodeSerializeFlagExternal
 * @tc.desc: Tests for Object Serialize flag for node.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, NodeSerializeFlagExternal, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);

        ASSERT_TRUE(resources->AddResource(CORE_NS::ResourceIdContext{"scene2"},
            ::SCENE_NS::ClassId::GltfSceneResource.Id().ToUid(),
            "test://AnimatedCube/AnimatedCube.gltf"));

        auto i = interface_cast<INodeImport>(scene->GetRootNode().GetResult());
        auto node =
            i->ImportChildScene(
                 interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene2"})), "test")
                .GetResult();

        ASSERT_TRUE(node);

        EXPECT_TRUE(META_NS::IsFlagSet(node, META_NS::ObjectFlagBits::SERIALIZE));
        META_NS::SetObjectFlags(node, META_NS::ObjectFlagBits::SERIALIZE, false);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        ASSERT_TRUE(
            resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(scene), "app://ser_node_test_ext.scene"));
        ASSERT_EQ(resources->Export("app://ser_node_test_ext.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://ser_node_test_ext.resources"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    EXPECT_FALSE(scene->FindNode("//test").GetResult());
}

/**
 * @tc.name: NodeSerializeFlagRoot
 * @tc.desc: Tests for Object Serialize flag for node.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneSerialisationTest, NodeSerializeFlagRoot, testing::ext::TestSize.Level1)
{
    {
        auto scene = CreateEmptyScene();
        ASSERT_TRUE(scene);
        auto test = scene->CreateNode("//test").GetResult();
        ASSERT_TRUE(test);

        auto root = scene->GetRootNode().GetResult();
        root->Position()->SetValue({1.f, 2.f, 3.f});

        META_NS::SetObjectFlags(root, META_NS::ObjectFlagBits::SERIALIZE, false);

        if (auto i = interface_cast<CORE_NS::ISetResourceId>(scene)) {
            i->SetResourceId(CORE_NS::ResourceIdContext{"scene"});
        }

        ASSERT_TRUE(resources->AddResource(
            interface_pointer_cast<CORE_NS::IResource>(scene), "app://ser_node_test_root.scene"));
        ASSERT_EQ(resources->Export("app://ser_node_test_root.resources"), CORE_NS::IResourceManager::Result::OK);
        resources->RemoveAllResources();
    }
    ASSERT_EQ(resources->Import("app://ser_node_test_root.resources"), CORE_NS::IResourceManager::Result::OK);

    auto scene = interface_pointer_cast<IScene>(resources->GetResource(CORE_NS::ResourceIdContext{"scene"}));
    ASSERT_TRUE(scene);

    // root is there always even if you say to not serialise it (just the data you might have added is not serialised)
    auto root = scene->GetRootNode().GetResult();
    ASSERT_TRUE(root);
    EXPECT_EQ(root->Position()->GetValue(), BASE_NS::Math::Vec3());

    EXPECT_FALSE(scene->FindNode("//test").GetResult());
}

}  // namespace UTest
SCENE_END_NAMESPACE()
