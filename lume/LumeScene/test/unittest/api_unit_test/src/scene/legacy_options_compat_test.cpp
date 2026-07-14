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

#include <scene/api/resource_utils.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_environment.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/postprocess/intf_bloom.h>
#include <scene/interface/resource/intf_render_resource_manager.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/template/intf_resource_template.h>

#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/resources/intf_resource_manager.h>

#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/resource/intf_resource.h>

#include "scene/scene_test.h"

META_TYPE(CORE_NS::IResourceType::Ptr)

SCENE_BEGIN_NAMESPACE()
namespace UTest {

// Guards the on-disk shape of CORE_NS::IResourceOptions produced by the legacy
// metadata-importer plugin. The options metadata uses a flat dot-prefixed path
// grammar (e.g. ".MaterialShader", ".Textures[0].Image") which is the
// committed on-disk format. Any refactor that preserves back-compat must keep
// these names and round-trip semantics intact.
class API_LegacyOptionsCompatTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        ScenePluginTest::SetUp();
        renderman_ =
            META_NS::GetObjectRegistry().Create<IRenderResourceManager>(ClassId::RenderResourceManager, params);
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

    BASE_NS::vector<BASE_NS::string> SavedOptionPropertyNames(const CORE_NS::IResourceOptions::Ptr& opts) const
    {
        BASE_NS::vector<BASE_NS::string> names;
        if (auto md = interface_cast<META_NS::IMetadata>(opts)) {
            for (auto&& d : md->GetAllMetadatas(META_NS::MetadataType::PROPERTY)) {
                names.push_back(d.name);
            }
        }
        return names;
    }

    static bool Contains(const BASE_NS::vector<BASE_NS::string>& v, BASE_NS::string_view s)
    {
        for (auto&& n : v) {
            if (n == s) {
                return true;
            }
        }
        return false;
    }

private:
    IRenderResourceManager::Ptr renderman_;
};

/**
 * @tc.name: MaterialOptionsPropertyNameSnapshot
 * @tc.desc: Saved material options must carry the legacy flat dot-prefixed
 *           property names. This locks the on-disk shape.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_LegacyOptionsCompatTest, MaterialOptionsPropertyNameSnapshot, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto material = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
    ASSERT_TRUE(material);
    material->LightingFlags()->SetValue(LightingFlags::SHADOW_CASTER_BIT);
    material->MaterialShader()->SetValue(CreateTestShader());
    auto t0 = material->Textures()->GetValueAt(0);
    ASSERT_TRUE(t0);
    t0->Image()->SetValue(CreateTestBitmap());
    t0->Rotation()->SetValue(1.0f);

    auto res = interface_pointer_cast<CORE_NS::IResource>(material);
    auto type = scene->CreateObject<CORE_NS::IResourceType>(ClassId::MaterialResource).GetResult();
    ASSERT_TRUE(type);

    auto opts =
        META_NS::GetObjectRegistry().Create<META_NS::IObjectResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
    ASSERT_TRUE(opts);
    CORE_NS::IResourceType::StorageInfo info{
        opts, nullptr, CORE_NS::ResourceId{"mat"}, "", nullptr, interface_pointer_cast<CORE_NS::IInterface>(scene)};
    ASSERT_TRUE(type->SaveResource(res, info));

    auto names = SavedOptionPropertyNames(opts);
    EXPECT_TRUE(Contains(names, ".MaterialShader")) << "missing .MaterialShader";
    EXPECT_TRUE(Contains(names, ".LightingFlags")) << "missing .LightingFlags";
    EXPECT_TRUE(Contains(names, ".Textures[0].Image")) << "missing .Textures[0].Image";
    EXPECT_TRUE(Contains(names, ".Textures[0].Rotation")) << "missing .Textures[0].Rotation";

    // No property name may drop the leading dot — it is part of the legacy
    // flat-path grammar consumed by FindObject on load.
    for (auto&& n : names) {
        if (!n.empty()) {
            EXPECT_EQ(n[0], '.') << "property without leading dot: " << n.c_str();
        }
    }
}

/**
 * @tc.name: MaterialOptionsRoundTrip
 * @tc.desc: Save+Load through MaterialResourceType preserves property values.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_LegacyOptionsCompatTest, MaterialOptionsRoundTrip, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto shader = CreateTestShader();
    auto bitmap = CreateTestBitmap();

    auto material = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
    ASSERT_TRUE(material);
    material->LightingFlags()->SetValue(LightingFlags::SHADOW_CASTER_BIT);
    material->MaterialShader()->SetValue(shader);
    auto t0 = material->Textures()->GetValueAt(0);
    ASSERT_TRUE(t0);
    t0->Image()->SetValue(bitmap);
    t0->Rotation()->SetValue(1.0f);

    auto res = interface_pointer_cast<CORE_NS::IResource>(material);
    auto type = scene->CreateObject<CORE_NS::IResourceType>(ClassId::MaterialResource).GetResult();
    ASSERT_TRUE(type);

    auto opts =
        META_NS::GetObjectRegistry().Create<META_NS::IObjectResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
    ASSERT_TRUE(opts);
    CORE_NS::IResourceType::StorageInfo info{
        opts, nullptr, CORE_NS::ResourceId{"mat"}, "", nullptr, interface_pointer_cast<CORE_NS::IInterface>(scene)};
    ASSERT_TRUE(type->SaveResource(res, info));

    auto loaded = type->LoadResource(info);
    ASSERT_TRUE(loaded);
    auto mat = interface_cast<IMaterial>(loaded);
    ASSERT_TRUE(mat);
    EXPECT_EQ(mat->LightingFlags()->GetValue(), LightingFlags::SHADOW_CASTER_BIT);
    {
        auto s = mat->MaterialShader()->GetValue();
        auto r = interface_cast<CORE_NS::IResource>(s);
        ASSERT_TRUE(r);
        EXPECT_EQ(r->GetResourceId(), CORE_NS::ResourceId{"shader"});
    }
    auto lt = mat->Textures()->GetValueAt(0);
    ASSERT_TRUE(lt);
    EXPECT_EQ(lt->Rotation()->GetValue(), 1.0f);
    {
        auto img = lt->Image()->GetValue();
        auto r = interface_cast<CORE_NS::IResource>(img);
        ASSERT_TRUE(r);
        EXPECT_EQ(r->GetResourceId(), CORE_NS::ResourceId{"image"});
    }
}

/**
 * @tc.name: CreateTemplateForMaterialResource
 * @tc.desc: CreateTemplateForResource builds a MaterialTemplate populated from
 *           the live material's property values.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_LegacyOptionsCompatTest, CreateTemplateForMaterialResource, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto shader = CreateTestShader();
    auto material = scene->CreateObject<IMaterial>(ClassId::Material).GetResult();
    ASSERT_TRUE(material);
    material->LightingFlags()->SetValue(LightingFlags::SHADOW_CASTER_BIT);
    material->MaterialShader()->SetValue(shader);
    material->AlphaCutoff()->SetValue(0.25f);

    auto res = interface_pointer_cast<CORE_NS::IResource>(material);
    ASSERT_TRUE(res);

    auto templ = CreateTemplateForResource(res);
    ASSERT_TRUE(templ);

    auto meta = interface_cast<META_NS::IMetadata>(templ);
    ASSERT_TRUE(meta);
    auto alpha = meta->GetProperty<float>("AlphaCutoff");
    ASSERT_TRUE(alpha);
    EXPECT_EQ(alpha->GetValue(), 0.25f);
    auto flags = meta->GetProperty<LightingFlags>("LightingFlags");
    ASSERT_TRUE(flags);
    EXPECT_EQ(flags->GetValue(), LightingFlags::SHADOW_CASTER_BIT);
    // Templates carry resource refs as ResourceId, not as typed pointers; the apply path
    // resolves them against the live scene's resource manager.
    auto sh = meta->GetProperty<CORE_NS::ResourceId>("MaterialShader");
    ASSERT_TRUE(sh);
    auto shRes = interface_pointer_cast<CORE_NS::IResource>(shader);
    ASSERT_TRUE(shRes);
    EXPECT_EQ(sh->GetValue(), shRes->GetResourceId());
}

/**
 * @tc.name: CreateTemplateForEnvironmentResource
 * @tc.desc: CreateTemplateForResource builds an EnvironmentTemplate whose RadianceImage and
 *           EnvironmentImage are captured as CORE_NS::ResourceId, read off the bound live
 *           IImage's GetResourceId(). Exercises the generic CopyOnePropertyExcept capture
 *           path (EnvironmentTemplate has no CopyFromTyped override).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_LegacyOptionsCompatTest, CreateTemplateForEnvironmentResource, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);
    const auto imgId = interface_cast<CORE_NS::IResource>(bitmap.get())->GetResourceId();
    ASSERT_TRUE(imgId.IsValid());

    auto env = scene->CreateObject<IEnvironment>(ClassId::Environment).GetResult();
    ASSERT_TRUE(env);
    env->RadianceImage()->SetValue(bitmap);
    env->EnvironmentImage()->SetValue(bitmap);

    auto res = interface_pointer_cast<CORE_NS::IResource>(env);
    ASSERT_TRUE(res);

    auto templ = CreateTemplateForResource(res);
    ASSERT_TRUE(templ);

    auto meta = interface_cast<META_NS::IMetadata>(templ);
    ASSERT_TRUE(meta);
    auto radId = meta->GetProperty<CORE_NS::ResourceId>("RadianceImage");
    ASSERT_TRUE(radId);
    EXPECT_EQ(radId->GetValue(), imgId);
    auto envId = meta->GetProperty<CORE_NS::ResourceId>("EnvironmentImage");
    ASSERT_TRUE(envId);
    EXPECT_EQ(envId->GetValue(), imgId);
}

/**
 * @tc.name: CreateTemplateForPostProcessResource
 * @tc.desc: CreateTemplateForResource builds a PostProcessTemplate whose Bloom sub-object
 *           is captured as a fresh sub-object on the template (not a shared pointer back
 *           into the live IPostProcess), and the sub-object's DirtMaskImage resource ref
 *           is captured as CORE_NS::ResourceId. Exercises CopyOnePropertyExcept's owned-
 *           sub-object capture branch and its recursive descent.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_LegacyOptionsCompatTest, CreateTemplateForPostProcessResource, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);
    const auto imgId = interface_cast<CORE_NS::IResource>(bitmap.get())->GetResourceId();
    ASSERT_TRUE(imgId.IsValid());

    auto pp = scene->CreateObject<IPostProcess>(ClassId::PostProcess).GetResult();
    ASSERT_TRUE(pp);
    auto bloom = META_NS::GetValue(pp->Bloom());
    ASSERT_TRUE(bloom);
    bloom->Enabled()->SetValue(true);
    bloom->DirtMaskImage()->SetValue(bitmap);

    auto res = interface_pointer_cast<CORE_NS::IResource>(pp);
    ASSERT_TRUE(res);

    auto templ = CreateTemplateForResource(res);
    ASSERT_TRUE(templ);

    auto meta = interface_cast<META_NS::IMetadata>(templ);
    ASSERT_TRUE(meta);
    auto bloomProp = meta->GetProperty<META_NS::IObject::Ptr>("Bloom");
    ASSERT_TRUE(bloomProp);
    auto bloomSub = META_NS::GetPointer<META_NS::IMetadata>(bloomProp);
    ASSERT_TRUE(bloomSub);
    // The captured template owns a fresh sub-object — not a shared pointer back into the
    // live IBloom — so the captured Bloom should NOT also implement IBloom.
    EXPECT_FALSE(interface_cast<IBloom>(bloomSub.get()));
    auto dirtId = bloomSub->GetProperty<CORE_NS::ResourceId>("DirtMaskImage");
    ASSERT_TRUE(dirtId);
    EXPECT_EQ(dirtId->GetValue(), imgId);
}

/**
 * @tc.name: CreateTemplateForResource_NullAndUnmappedTypes
 * @tc.desc: CreateTemplateForResource returns nullptr for null input or for a
 *           resource type that has no mapped template class.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_LegacyOptionsCompatTest, CreateTemplateForResource_NullAndUnmappedTypes, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(CreateTemplateForResource({}));
    EXPECT_FALSE(TemplateClassIdForResourceType(BASE_NS::Uid{}).IsValid());
}

}  // namespace UTest
SCENE_END_NAMESPACE()
