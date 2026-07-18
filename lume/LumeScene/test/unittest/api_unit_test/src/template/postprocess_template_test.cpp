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

#include <scene/api/template/postprocess_template.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/template/intf_resource_template.h>
#include <scene/interface/template/intf_template_options.h>

#include <meta/api/metadata_util.h>
#include <meta/api/util.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/resource/intf_resource.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class PostProcessTemplateTest : public ScenePluginTest {
public:
    META_NS::IObject::Ptr CreateTemplateObj() const
    {
        return META_NS::GetObjectRegistry().Create<META_NS::IObject>(ClassId::PostProcessTemplate);
    }

    static META_NS::IObject::Ptr CreateEffectSubObject()
    {
        return META_NS::GetObjectRegistry().Create<META_NS::IObject>(META_NS::ClassId::Object);
    }

    template <typename T>
    static void AddSetProperty(META_NS::IMetadata& meta, BASE_NS::string_view name, const T& value)
    {
        auto prop = META_NS::ConstructProperty<T>(name);
        meta.AddProperty(prop.GetProperty());
        META_NS::SetValue<T>(prop.GetProperty(), value);
    }

    static META_NS::IObject::Ptr CreateTonemapEffect()
    {
        auto effect = CreateEffectSubObject();
        if (auto m = interface_cast<META_NS::IMetadata>(effect.get())) {
            AddSetProperty(*m, "Enabled", true);
            AddSetProperty(*m, "Type", TonemapType::ACES);
            AddSetProperty(*m, "Exposure", 1.5f);
        }
        return effect;
    }

    PostProcessTemplate CreateFullTemplate() const
    {
        auto obj = CreateTemplateObj();
        if (auto meta = interface_cast<META_NS::IMetadata>(obj.get())) {
            AddSetProperty(*meta, "Tonemap", CreateTonemapEffect());
        }
        return PostProcessTemplate(obj);
    }

    IPostProcess::Ptr CreatePostProcess(const IScene::Ptr& scene)
    {
        return scene->CreateObject<IPostProcess>(ClassId::PostProcess).GetResult();
    }
};

/**
 * @tc.name: CreatePostProcessTemplate
 * @tc.desc: PostProcessTemplate can be created via object registry
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, CreatePostProcessTemplate, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);
}

/**
 * @tc.name: EffectViewAccessible
 * @tc.desc: TemplateEffectView provides access to effect sub-object properties
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, EffectViewAccessible, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto tonemap = tmpl.Tonemap();
    ASSERT_TRUE(tonemap);
    EXPECT_TRUE(tonemap.Enabled());
    EXPECT_TRUE(tonemap.GetProperty<TonemapType>("Type"));
    EXPECT_TRUE(tonemap.GetProperty<float>("Exposure"));
}

/**
 * @tc.name: EffectViewValues
 * @tc.desc: TemplateEffectView returns correct property values
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, EffectViewValues, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto tonemap = tmpl.Tonemap();
    ASSERT_TRUE(tonemap);
    EXPECT_EQ(META_NS::GetValue(tonemap.Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(tonemap.GetProperty<TonemapType>("Type")), TonemapType::ACES);
    EXPECT_FLOAT_EQ(META_NS::GetValue(tonemap.GetProperty<float>("Exposure")), 1.5f);
}

/**
 * @tc.name: MissingEffectViewIsFalse
 * @tc.desc: TemplateEffectView for an effect not added to template evaluates to false
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, MissingEffectViewIsFalse, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    EXPECT_FALSE(tmpl.Bloom());
    EXPECT_FALSE(tmpl.Blur());
    EXPECT_FALSE(tmpl.Vignette());
}

/**
 * @tc.name: NameProperty
 * @tc.desc: PostProcessTemplate supports INamed and GetName returns the Name property value
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, NameProperty, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto named = interface_cast<META_NS::INamed>(obj.get());
    ASSERT_TRUE(named);

    META_NS::SetValue(named->Name(), BASE_NS::string("TestPP"));
    EXPECT_TRUE(META_NS::IsValueSet(*named->Name().GetProperty()));
    EXPECT_EQ(META_NS::GetValue(named->Name()), "TestPP");
    EXPECT_EQ(obj->GetName(), "TestPP");
}

/**
 * @tc.name: ApplyToPostProcess
 * @tc.desc: ApplyOptions copies set effect properties to a live postprocess
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, ApplyToPostProcess, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto pp = CreatePostProcess(scene);
    ASSERT_TRUE(pp);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);
    auto resource = interface_cast<CORE_NS::IResource>(pp.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(options->ApplyOptions(*resource, scene));
    UpdateScene();

    auto tonemap = META_NS::GetValue(pp->Tonemap());
    ASSERT_TRUE(tonemap);
    EXPECT_EQ(META_NS::GetValue(tonemap->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(tonemap->Type()), TonemapType::ACES);
    EXPECT_FLOAT_EQ(META_NS::GetValue(tonemap->Exposure()), 1.5f);
}

/**
 * @tc.name: ApplyToOnlyCopiesSetEffects
 * @tc.desc: ApplyOptions does not overwrite effects that are not on the template
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, ApplyToOnlyCopiesSetEffects, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto pp = CreatePostProcess(scene);
    ASSERT_TRUE(pp);
    UpdateScene();

    auto bloom = META_NS::GetValue(pp->Bloom());
    ASSERT_TRUE(bloom);
    const bool origBloomEnabled = META_NS::GetValue(bloom->Enabled());

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);
    // Template only has Tonemap, no Bloom

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);
    auto resource = interface_cast<CORE_NS::IResource>(pp.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(options->ApplyOptions(*resource, scene));
    UpdateScene();

    bloom = META_NS::GetValue(pp->Bloom());
    ASSERT_TRUE(bloom);
    EXPECT_EQ(META_NS::GetValue(bloom->Enabled()), origBloomEnabled);
}

/**
 * @tc.name: CopyFromPostProcess
 * @tc.desc: UpdateOptions copies set properties from a live postprocess
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, CopyFromPostProcess, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto pp = CreatePostProcess(scene);
    ASSERT_TRUE(pp);
    UpdateScene();

    auto tonemap = META_NS::GetValue(pp->Tonemap());
    ASSERT_TRUE(tonemap);
    META_NS::SetValue(tonemap->Exposure(), 2.5f);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);
    auto resource = interface_cast<CORE_NS::IResource>(pp.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(options->UpdateOptions(*resource, scene));

    auto tmplTonemap = tmpl.Tonemap();
    ASSERT_TRUE(tmplTonemap);
    EXPECT_FLOAT_EQ(META_NS::GetValue(tmplTonemap.GetProperty<float>("Exposure")), 2.5f);
}

/**
 * @tc.name: ApplyToViaInterface
 * @tc.desc: PostProcessTemplate::ApplyTo copies set properties to a live postprocess
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, ApplyToViaInterface, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto pp = CreatePostProcess(scene);
    ASSERT_TRUE(pp);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    EXPECT_TRUE(tmpl.ApplyTo(*pp));
    UpdateScene();

    auto tonemap = META_NS::GetValue(pp->Tonemap());
    ASSERT_TRUE(tonemap);
    EXPECT_FLOAT_EQ(META_NS::GetValue(tonemap->Exposure()), 1.5f);
}

/**
 * @tc.name: CopyFromViaInterface
 * @tc.desc: PostProcessTemplate::CopyFrom copies set properties from a live postprocess
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, CopyFromViaInterface, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto pp = CreatePostProcess(scene);
    ASSERT_TRUE(pp);
    UpdateScene();

    auto tonemap = META_NS::GetValue(pp->Tonemap());
    ASSERT_TRUE(tonemap);
    META_NS::SetValue(tonemap->Exposure(), 3.0f);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    EXPECT_TRUE(tmpl.CopyFrom(static_cast<const IPostProcess&>(*pp)));

    auto tmplTonemap = tmpl.Tonemap();
    ASSERT_TRUE(tmplTonemap);
    EXPECT_FLOAT_EQ(META_NS::GetValue(tmplTonemap.GetProperty<float>("Exposure")), 3.0f);
}

/**
 * @tc.name: CloneTemplate
 * @tc.desc: Clone produces an independent copy of the template
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, CloneTemplate, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto named = interface_cast<META_NS::INamed>(tmpl.GetPtr().get());
    ASSERT_TRUE(named);
    META_NS::SetValue(named->Name(), BASE_NS::string("OriginalPP"));

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);

    auto cloneOptions = options->Clone();
    ASSERT_TRUE(cloneOptions);

    auto cloneNamed = interface_cast<META_NS::INamed>(cloneOptions.get());
    ASSERT_TRUE(cloneNamed);
    EXPECT_EQ(META_NS::GetValue(cloneNamed->Name()), "OriginalPP");

    auto cloneMeta = interface_cast<META_NS::IMetadata>(cloneOptions.get());
    ASSERT_TRUE(cloneMeta);
    auto cloneTonemap = cloneMeta->GetProperty<META_NS::IObject::Ptr>("Tonemap");
    ASSERT_TRUE(cloneTonemap);

    // Verify independence
    META_NS::SetValue(cloneNamed->Name(), BASE_NS::string("ModifiedPP"));
    EXPECT_EQ(META_NS::GetValue(named->Name()), "OriginalPP");
}

/**
 * @tc.name: ClonePropagatesBaseAndTemplateContext
 * @tc.desc: Clone copies baseResource_ and templateContext_ along with property data
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, ClonePropagatesBaseAndTemplateContext, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto derived = interface_cast<META_NS::IDerivedResourceOptions>(obj.get());
    auto tc = interface_cast<ITemplateOptions>(obj.get());
    ASSERT_TRUE(derived && tc);

    CORE_NS::ResourceId base{"base-pp", "group-a"};
    derived->SetBaseResource(base);
    tc->SetTemplateContext(true);

    auto options = interface_cast<CORE_NS::IResourceOptions>(obj.get());
    ASSERT_TRUE(options);
    auto cloneOptions = options->Clone();
    ASSERT_TRUE(cloneOptions);

    auto cloneDerived = interface_cast<META_NS::IDerivedResourceOptions>(cloneOptions.get());
    auto cloneTc = interface_cast<ITemplateOptions>(cloneOptions.get());
    ASSERT_TRUE(cloneDerived && cloneTc);
    EXPECT_EQ(cloneDerived->GetBaseResource(), base);
    EXPECT_TRUE(cloneTc->IsTemplateContext());
}

/**
 * @tc.name: MergeCopiesOtherSetValues
 * @tc.desc: Merge promotes this's set values to defaults, then copies other's set values
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, MergeCopiesOtherSetValues, testing::ext::TestSize.Level1)
{
    auto dstObj = CreateTemplateObj();
    ASSERT_TRUE(dstObj);

    auto srcObj = CreateTemplateObj();
    ASSERT_TRUE(srcObj);
    if (auto srcMeta = interface_cast<META_NS::IMetadata>(srcObj.get())) {
        AddSetProperty(*srcMeta, "Tonemap", CreateTonemapEffect());
    }

    auto dstOptions = interface_cast<CORE_NS::IResourceOptions>(dstObj.get());
    auto srcOptions = interface_cast<CORE_NS::IResourceOptions>(srcObj.get());
    ASSERT_TRUE(dstOptions && srcOptions);

    EXPECT_TRUE(dstOptions->Merge(*srcOptions));

    auto dstMeta = interface_cast<META_NS::IMetadata>(dstObj.get());
    ASSERT_TRUE(dstMeta);
    auto dstTonemap = dstMeta->GetProperty<META_NS::IObject::Ptr>("Tonemap");
    ASSERT_TRUE(dstTonemap);
    EXPECT_TRUE(dstTonemap.GetProperty()->IsValueSet());
}

/**
 * @tc.name: MergePropagatesBaseResource
 * @tc.desc: Merge propagates a valid baseResource_ from source to destination
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, MergePropagatesBaseResource, testing::ext::TestSize.Level1)
{
    auto dst = CreateTemplateObj();
    auto src = CreateTemplateObj();
    ASSERT_TRUE(dst && src);

    auto srcDerived = interface_cast<META_NS::IDerivedResourceOptions>(src.get());
    ASSERT_TRUE(srcDerived);
    CORE_NS::ResourceId base{"base-pp", "group-a"};
    srcDerived->SetBaseResource(base);

    auto dstOptions = interface_cast<CORE_NS::IResourceOptions>(dst.get());
    auto srcOptions = interface_cast<CORE_NS::IResourceOptions>(src.get());
    ASSERT_TRUE(dstOptions && srcOptions);
    EXPECT_TRUE(dstOptions->Merge(*srcOptions));

    auto dstDerived = interface_cast<META_NS::IDerivedResourceOptions>(dst.get());
    ASSERT_TRUE(dstDerived);
    EXPECT_EQ(dstDerived->GetBaseResource(), base);
}

/**
 * @tc.name: IResourceTemplateInterfaceCast
 * @tc.desc: PostProcessTemplate supports interface_cast to IResourceTemplate
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, IResourceTemplateInterfaceCast, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto rt = interface_cast<IResourceTemplate>(obj.get());
    EXPECT_TRUE(rt);
}

/**
 * @tc.name: ResourceType
 * @tc.desc: PostProcessTemplate reports correct resource type
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, ResourceType, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto resource = interface_cast<CORE_NS::IResource>(obj.get());
    ASSERT_TRUE(resource);

    EXPECT_EQ(resource->GetResourceType(), ClassId::PostProcessResourceTemplate.Id().ToUid());
}

/**
 * @tc.name: CreateInstanceRoundTrip
 * @tc.desc: CreateInstance returns a live postprocess with the template's set effects applied
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, CreateInstanceRoundTrip, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto pp = tmpl.CreateInstance(scene);
    ASSERT_TRUE(pp);
    UpdateScene();

    auto tonemap = META_NS::GetValue(pp->Tonemap());
    ASSERT_TRUE(tonemap);
    EXPECT_EQ(META_NS::GetValue(tonemap->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(tonemap->Type()), TonemapType::ACES);
    EXPECT_FLOAT_EQ(META_NS::GetValue(tonemap->Exposure()), 1.5f);
}

/**
 * @tc.name: CreateInstanceNullScene
 * @tc.desc: CreateInstance with null scene returns nullptr
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, CreateInstanceNullScene, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);
    EXPECT_FALSE(tmpl.CreateInstance(nullptr));
}

/**
 * @tc.name: CreateInstanceUninitializedWrapper
 * @tc.desc: CreateInstance on a wrapper that holds no object returns nullptr
 * @tc.type: FUNC
 */
UNIT_TEST_F(PostProcessTemplateTest, CreateInstanceUninitializedWrapper, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    PostProcessTemplate tmpl(META_NS::IObject::Ptr{});
    EXPECT_FALSE(tmpl);
    EXPECT_FALSE(tmpl.CreateInstance(scene));
}

}  // namespace UTest
SCENE_END_NAMESPACE()
