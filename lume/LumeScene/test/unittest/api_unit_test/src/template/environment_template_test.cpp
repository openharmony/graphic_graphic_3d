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

#include <scene/api/template/environment_template.h>
#include <scene/interface/intf_environment.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/template/intf_resource_template.h>
#include <scene/interface/template/intf_template_options.h>

#include <meta/api/metadata_util.h>
#include <meta/api/util.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/property/construct_array_property.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/resource/intf_resource.h>

#include <core/resources/intf_resource.h>
#include <core/resources/intf_resource_manager.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class EnvironmentTemplateTest : public ScenePluginTest {
public:
    META_NS::IObject::Ptr CreateTemplateObj() const
    {
        return META_NS::GetObjectRegistry().Create<META_NS::IObject>(ClassId::EnvironmentTemplate);
    }

    static void AddScalarProperties(META_NS::IMetadata& meta)
    {
        meta.AddProperty(META_NS::ConstructProperty<EnvBackgroundType>("Background").GetProperty());
        meta.AddProperty(META_NS::ConstructProperty<BASE_NS::Math::Vec4>("IndirectDiffuseFactor").GetProperty());
        meta.AddProperty(META_NS::ConstructProperty<BASE_NS::Math::Vec4>("IndirectSpecularFactor").GetProperty());
        meta.AddProperty(META_NS::ConstructProperty<BASE_NS::Math::Vec4>("EnvMapFactor").GetProperty());
        meta.AddProperty(META_NS::ConstructProperty<uint32_t>("RadianceCubemapMipCount").GetProperty());
        meta.AddProperty(META_NS::ConstructProperty<float>("EnvironmentMapLodLevel").GetProperty());
        meta.AddProperty(META_NS::ConstructProperty<BASE_NS::Math::Quat>("EnvironmentRotation").GetProperty());
    }

    static void AddIrradianceProperty(META_NS::IMetadata& meta)
    {
        BASE_NS::vector<BASE_NS::Math::Vec3> coefficients;
        coefficients.push_back({1.0f, 0.5f, 0.25f});
        coefficients.push_back({0.1f, 0.2f, 0.3f});
        coefficients.push_back({0.4f, 0.5f, 0.6f});
        auto prop = META_NS::ConstructArrayProperty<BASE_NS::Math::Vec3>("IrradianceCoefficients", coefficients);
        meta.AddProperty(prop.GetProperty());
    }

    EnvironmentTemplate CreateFullTemplate() const
    {
        auto obj = CreateTemplateObj();
        if (auto meta = interface_cast<META_NS::IMetadata>(obj.get())) {
            AddScalarProperties(*meta);
            AddIrradianceProperty(*meta);
        }
        return EnvironmentTemplate(obj);
    }
};

/**
 * @tc.name: CreateEnvironmentTemplate
 * @tc.desc: EnvironmentTemplate can be created via object registry
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, CreateEnvironmentTemplate, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);
}

/**
 * @tc.name: PropertiesAccessible
 * @tc.desc: Dynamically added environment template properties are accessible via API helper
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, PropertiesAccessible, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    EXPECT_TRUE(tmpl.Name());
    EXPECT_TRUE(tmpl.Background());
    EXPECT_TRUE(tmpl.IndirectDiffuseFactor());
    EXPECT_TRUE(tmpl.IndirectSpecularFactor());
    EXPECT_TRUE(tmpl.EnvMapFactor());
    EXPECT_TRUE(tmpl.RadianceCubemapMipCount());
    EXPECT_TRUE(tmpl.EnvironmentMapLodLevel());
    EXPECT_TRUE(tmpl.EnvironmentRotation());
    EXPECT_TRUE(tmpl.IrradianceCoefficients());
}

/**
 * @tc.name: PropertiesDefaultNotSet
 * @tc.desc: Newly added properties are not marked as set
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, PropertiesDefaultNotSet, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    EXPECT_FALSE(META_NS::IsValueSet(*tmpl.Background().GetProperty()));
    EXPECT_FALSE(META_NS::IsValueSet(*tmpl.EnvironmentMapLodLevel().GetProperty()));
    EXPECT_FALSE(META_NS::IsValueSet(*tmpl.RadianceCubemapMipCount().GetProperty()));
}

/**
 * @tc.name: NameProperty
 * @tc.desc: EnvironmentTemplate supports INamed and GetName returns the Name property value
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, NameProperty, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto named = interface_cast<META_NS::INamed>(obj.get());
    ASSERT_TRUE(named);

    META_NS::SetValue(named->Name(), BASE_NS::string("TestEnv"));
    EXPECT_TRUE(META_NS::IsValueSet(*named->Name().GetProperty()));
    EXPECT_EQ(META_NS::GetValue(named->Name()), "TestEnv");
    EXPECT_EQ(obj->GetName(), "TestEnv");

    // The API wrapper exposes the same Name property.
    EnvironmentTemplate tmpl(obj);
    ASSERT_TRUE(tmpl.Name());
    EXPECT_EQ(META_NS::GetValue(tmpl.Name()), "TestEnv");
}

/**
 * @tc.name: SetPropertyMarksAsSet
 * @tc.desc: Setting a property value marks it as set
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, SetPropertyMarksAsSet, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    META_NS::SetValue(tmpl.EnvironmentMapLodLevel(), 2.5f);
    EXPECT_TRUE(META_NS::IsValueSet(*tmpl.EnvironmentMapLodLevel().GetProperty()));
    EXPECT_FLOAT_EQ(META_NS::GetValue(tmpl.EnvironmentMapLodLevel()), 2.5f);
}

/**
 * @tc.name: ApplyToEnvironment
 * @tc.desc: ApplyOptions copies only set properties to a live environment
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, ApplyToEnvironment, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto env = scene->CreateObject<IEnvironment>(ClassId::Environment).GetResult();
    ASSERT_TRUE(env);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    const float testLod = 3.5f;
    META_NS::SetValue(tmpl.EnvironmentMapLodLevel(), testLod);

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);
    auto resource = interface_cast<CORE_NS::IResource>(env.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(options->ApplyOptions(*resource, scene));
    UpdateScene();

    EXPECT_FLOAT_EQ(META_NS::GetValue(env->EnvironmentMapLodLevel()), testLod);
    // Characterization: non-template-context ApplyOptions lands set values as OVERRIDES.
    // Pins the ApplyOptions path against the upcoming standalone-ApplyTo defaults change.
    EXPECT_TRUE(env->EnvironmentMapLodLevel().GetProperty()->IsValueSet());
}

/**
 * @tc.name: ApplyToOnlyCopiesSetProperties
 * @tc.desc: ApplyOptions does not overwrite properties that are not set on the template
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, ApplyToOnlyCopiesSetProperties, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto env = scene->CreateObject<IEnvironment>(ClassId::Environment).GetResult();
    ASSERT_TRUE(env);
    UpdateScene();

    const float originalLod = META_NS::GetValue(env->EnvironmentMapLodLevel());

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);
    // Don't set EnvironmentMapLodLevel on template

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);
    auto resource = interface_cast<CORE_NS::IResource>(env.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(options->ApplyOptions(*resource, scene));
    UpdateScene();

    EXPECT_FLOAT_EQ(META_NS::GetValue(env->EnvironmentMapLodLevel()), originalLod);
}

/**
 * @tc.name: CopyFromEnvironment
 * @tc.desc: UpdateOptions copies set properties from a live environment
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, CopyFromEnvironment, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto env = scene->CreateObject<IEnvironment>(ClassId::Environment).GetResult();
    ASSERT_TRUE(env);
    UpdateScene();

    const float testLod = 4.2f;
    META_NS::SetValue(env->EnvironmentMapLodLevel(), testLod);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);
    auto resource = interface_cast<CORE_NS::IResource>(env.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(options->UpdateOptions(*resource, scene));
    EXPECT_FLOAT_EQ(META_NS::GetValue(tmpl.EnvironmentMapLodLevel()), testLod);
}

/**
 * @tc.name: ApplyToViaInterface
 * @tc.desc: IResourceTemplate::ApplyTo copies set properties to a live environment
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, ApplyToViaInterface, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto env = scene->CreateObject<IEnvironment>(ClassId::Environment).GetResult();
    ASSERT_TRUE(env);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);
    META_NS::SetValue(tmpl.EnvironmentMapLodLevel(), 1.5f);

    EXPECT_TRUE(tmpl.ApplyTo(*env));
    UpdateScene();

    EXPECT_FLOAT_EQ(META_NS::GetValue(env->EnvironmentMapLodLevel()), 1.5f);
}

/**
 * @tc.name: ApplyToPromotesScalarsToDefaults
 * @tc.desc: IResourceTemplate::ApplyTo writes template scalars (EnvironmentMapLodLevel, a Vec4
 *          factor) onto the live environment as DEFAULTS: IsValueSet stays false and
 *          GetDefaultValue equals the template value. Pins the standalone ApplyTo path.
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, ApplyToPromotesScalarsToDefaults, testing::ext::TestSize.Level1)
{
    constexpr float LOD = 2.5f;
    constexpr BASE_NS::Math::Vec4 FACTOR{0.1f, 0.2f, 0.3f, 0.4f};

    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto env = scene->CreateObject<IEnvironment>(ClassId::Environment).GetResult();
    ASSERT_TRUE(env);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);
    META_NS::SetValue(tmpl.EnvironmentMapLodLevel(), LOD);
    META_NS::SetValue(tmpl.IndirectDiffuseFactor(), FACTOR);

    EXPECT_TRUE(tmpl.ApplyTo(*env));
    UpdateScene();

    EXPECT_FLOAT_EQ(META_NS::GetValue(env->EnvironmentMapLodLevel()), LOD);
    EXPECT_FLOAT_EQ(env->EnvironmentMapLodLevel()->GetDefaultValue(), LOD);
    EXPECT_FALSE(env->EnvironmentMapLodLevel().GetProperty()->IsValueSet());

    EXPECT_EQ(META_NS::GetValue(env->IndirectDiffuseFactor()), FACTOR);
    EXPECT_EQ(env->IndirectDiffuseFactor()->GetDefaultValue(), FACTOR);
    EXPECT_FALSE(env->IndirectDiffuseFactor().GetProperty()->IsValueSet());
}

/**
 * @tc.name: ApplyToResolvesSceneScopedRadianceImage
 * @tc.desc: IResourceTemplate::ApplyTo resolves an image ResourceId (RadianceImage) against a
 *          resource registered under the target scene's context — exercising the
 *          context-qualified rm->GetResource({id, context}) path from a standalone ApplyTo
 *          (no explicit context). The scene-scoped image is invisible to the context-less
 *          fallback, so success proves the derived scene context matches the registration.
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, ApplyToResolvesSceneScopedRadianceImage, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto env = scene->CreateObject<IEnvironment>(ClassId::Environment).GetResult();
    ASSERT_TRUE(env);
    UpdateScene();

    auto image = CreateTestBitmap();
    ASSERT_TRUE(image);
    CORE_NS::ResourceIdContext imgId{
        CORE_NS::ResourceId{"env-image"}, interface_pointer_cast<CORE_NS::IInterface>(scene)};
    auto setId = interface_cast<CORE_NS::ISetResourceId>(image);
    ASSERT_TRUE(setId);
    setId->SetResourceId(imgId);
    ASSERT_TRUE(resources->AddResource(interface_pointer_cast<CORE_NS::IResource>(image)));

    // The context-less fallback must MISS a scene-scoped resource.
    EXPECT_FALSE(resources->GetResource(CORE_NS::ResourceIdContext{CORE_NS::ResourceId{"env-image"}}));
    auto expected = resources->GetResource(imgId);
    ASSERT_TRUE(expected);

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);
    auto meta = interface_cast<META_NS::IMetadata>(tmpl.GetPtr().get());
    ASSERT_TRUE(meta);
    meta->AddProperty(META_NS::ConstructProperty<CORE_NS::ResourceId>("RadianceImage").GetProperty());
    auto radianceProp = meta->GetProperty<CORE_NS::ResourceId>("RadianceImage");
    ASSERT_TRUE(radianceProp);
    META_NS::SetValue(radianceProp, imgId.id);

    EXPECT_TRUE(tmpl.ApplyTo(*env));
    UpdateScene();

    auto resolved = META_NS::GetValue(env->RadianceImage());
    ASSERT_TRUE(resolved);
    EXPECT_EQ(interface_pointer_cast<CORE_NS::IResource>(resolved), expected);
}

/**
 * @tc.name: CopyFromViaInterface
 * @tc.desc: IResourceTemplate::CopyFrom copies set properties from a live environment
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, CopyFromViaInterface, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto env = scene->CreateObject<IEnvironment>(ClassId::Environment).GetResult();
    ASSERT_TRUE(env);
    UpdateScene();

    META_NS::SetValue(env->EnvironmentMapLodLevel(), 2.0f);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    EXPECT_TRUE(tmpl.CopyFrom(static_cast<const IEnvironment&>(*env)));
    EXPECT_FLOAT_EQ(META_NS::GetValue(tmpl.EnvironmentMapLodLevel()), 2.0f);
}

/**
 * @tc.name: CopyFromAnotherTemplate
 * @tc.desc: IResourceTemplate::CopyFrom copies set properties from another EnvironmentTemplate
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, CopyFromAnotherTemplate, testing::ext::TestSize.Level1)
{
    auto srcObj = CreateTemplateObj();
    ASSERT_TRUE(srcObj);
    if (auto m = interface_cast<META_NS::IMetadata>(srcObj.get())) {
        AddScalarProperties(*m);
    }
    auto src = EnvironmentTemplate(srcObj);
    META_NS::SetValue(src.EnvironmentMapLodLevel(), 5.0f);
    META_NS::SetValue(src.Background(), EnvBackgroundType::CUBEMAP);

    auto dst = CreateFullTemplate();
    ASSERT_TRUE(dst);
    auto dstTempl = dst.GetPtr<IResourceTemplate>();
    ASSERT_TRUE(dstTempl);
    EXPECT_TRUE(dstTempl->CopyFrom(static_cast<const META_NS::IObject&>(*srcObj)));

    EXPECT_FLOAT_EQ(META_NS::GetValue(dst.EnvironmentMapLodLevel()), 5.0f);
    EXPECT_EQ(META_NS::GetValue(dst.Background()), EnvBackgroundType::CUBEMAP);
}

/**
 * @tc.name: CloneTemplate
 * @tc.desc: Clone produces an independent copy of the template
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, CloneTemplate, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto named = interface_cast<META_NS::INamed>(tmpl.GetPtr().get());
    ASSERT_TRUE(named);
    META_NS::SetValue(named->Name(), BASE_NS::string("OriginalEnv"));
    META_NS::SetValue(tmpl.EnvironmentMapLodLevel(), 2.5f);

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);

    auto cloneOptions = options->Clone();
    ASSERT_TRUE(cloneOptions);

    auto cloneMeta = interface_cast<META_NS::IMetadata>(cloneOptions.get());
    ASSERT_TRUE(cloneMeta);

    auto cloneLod = cloneMeta->GetProperty<float>("EnvironmentMapLodLevel");
    ASSERT_TRUE(cloneLod);
    EXPECT_FLOAT_EQ(META_NS::GetValue(cloneLod), 2.5f);

    auto cloneNamed = interface_cast<META_NS::INamed>(cloneOptions.get());
    ASSERT_TRUE(cloneNamed);
    EXPECT_EQ(META_NS::GetValue(cloneNamed->Name()), "OriginalEnv");

    // Verify independence: changing clone doesn't affect original
    META_NS::SetValue(cloneLod, 9.0f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(tmpl.EnvironmentMapLodLevel()), 2.5f);
}

/**
 * @tc.name: ClonePropagatesBaseAndTemplateContext
 * @tc.desc: Clone copies baseResource_ and templateContext_ along with property data
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, ClonePropagatesBaseAndTemplateContext, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto derived = interface_cast<META_NS::IDerivedResourceOptions>(obj.get());
    auto tc = interface_cast<ITemplateOptions>(obj.get());
    ASSERT_TRUE(derived && tc);

    CORE_NS::ResourceId base{"base-env", "group-a"};
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
UNIT_TEST_F(EnvironmentTemplateTest, MergeCopiesOtherSetValues, testing::ext::TestSize.Level1)
{
    auto dstObj = CreateTemplateObj();
    ASSERT_TRUE(dstObj);

    auto srcObj = CreateTemplateObj();
    ASSERT_TRUE(srcObj);
    if (auto srcMeta = interface_cast<META_NS::IMetadata>(srcObj.get())) {
        srcMeta->AddProperty(META_NS::ConstructProperty<float>("EnvironmentMapLodLevel").GetProperty());
    }
    auto src = EnvironmentTemplate(srcObj);
    META_NS::SetValue(src.EnvironmentMapLodLevel(), 3.0f);

    auto dstOptions = interface_cast<CORE_NS::IResourceOptions>(dstObj.get());
    auto srcOptions = interface_cast<CORE_NS::IResourceOptions>(srcObj.get());
    ASSERT_TRUE(dstOptions && srcOptions);

    EXPECT_TRUE(dstOptions->Merge(*srcOptions));

    auto dstMeta = interface_cast<META_NS::IMetadata>(dstObj.get());
    ASSERT_TRUE(dstMeta);
    auto dstLod = dstMeta->GetProperty<float>("EnvironmentMapLodLevel");
    ASSERT_TRUE(dstLod);
    EXPECT_TRUE(dstLod.GetProperty()->IsValueSet());
    EXPECT_FLOAT_EQ(META_NS::GetValue(dstLod), 3.0f);
}

/**
 * @tc.name: MergeLayersSceneOverTemplate
 * @tc.desc: When scene-side options merges into template, template values become defaults
 *           and scene values become set values
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, MergeLayersSceneOverTemplate, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto env = scene->CreateObject<IEnvironment>(ClassId::Environment).GetResult();
    ASSERT_TRUE(env);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);
    META_NS::SetValue(tmpl.EnvironmentMapLodLevel(), 1.0f);
    META_NS::SetValue(tmpl.Background(), EnvBackgroundType::EQUIRECTANGULAR);

    auto sceneObj = CreateTemplateObj();
    ASSERT_TRUE(sceneObj);
    if (auto sm = interface_cast<META_NS::IMetadata>(sceneObj.get())) {
        sm->AddProperty(META_NS::ConstructProperty<float>("EnvironmentMapLodLevel").GetProperty());
    }
    auto sceneOpts = EnvironmentTemplate(sceneObj);
    META_NS::SetValue(sceneOpts.EnvironmentMapLodLevel(), 4.0f);

    auto tmplOptions = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    auto sceneOptOptions = interface_cast<CORE_NS::IResourceOptions>(sceneObj.get());
    ASSERT_TRUE(tmplOptions && sceneOptOptions);
    EXPECT_TRUE(tmplOptions->Merge(*sceneOptOptions));

    EXPECT_TRUE(tmpl.EnvironmentMapLodLevel().GetProperty()->IsValueSet());

    // Simulate PrepareResourceData: clone + SetTemplateContext(true).
    auto cloneOpts = tmplOptions->Clone();
    ASSERT_TRUE(cloneOpts);
    auto cloneTc = interface_cast<ITemplateOptions>(cloneOpts.get());
    ASSERT_TRUE(cloneTc);
    cloneTc->SetTemplateContext(true);

    auto resource = interface_cast<CORE_NS::IResource>(env.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(cloneOpts->ApplyOptions(*resource, scene));
    UpdateScene();

    EXPECT_FLOAT_EQ(META_NS::GetValue(env->EnvironmentMapLodLevel()), 4.0f);
    EXPECT_FLOAT_EQ(env->EnvironmentMapLodLevel()->GetDefaultValue(), 1.0f);
    EXPECT_EQ(META_NS::GetValue(env->Background()), EnvBackgroundType::EQUIRECTANGULAR);
}

/**
 * @tc.name: MergePropagatesBaseResource
 * @tc.desc: Merge propagates a valid baseResource_ from source to destination
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, MergePropagatesBaseResource, testing::ext::TestSize.Level1)
{
    auto dst = CreateTemplateObj();
    auto src = CreateTemplateObj();
    ASSERT_TRUE(dst && src);

    auto srcDerived = interface_cast<META_NS::IDerivedResourceOptions>(src.get());
    ASSERT_TRUE(srcDerived);
    CORE_NS::ResourceId base{"base-env", "group-a"};
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
 * @tc.name: SetTemplateContextPromotesValues
 * @tc.desc: SetTemplateContext(true) promotes set values to defaults so ApplyOptions
 *           lands them as property defaults on the live environment
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, SetTemplateContextPromotesValues, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto env = scene->CreateObject<IEnvironment>(ClassId::Environment).GetResult();
    ASSERT_TRUE(env);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);
    META_NS::SetValue(tmpl.EnvironmentMapLodLevel(), 2.0f);

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);
    auto cloneOpts = options->Clone();
    ASSERT_TRUE(cloneOpts);
    auto cloneTc = interface_cast<ITemplateOptions>(cloneOpts.get());
    ASSERT_TRUE(cloneTc);
    cloneTc->SetTemplateContext(true);

    auto resource = interface_cast<CORE_NS::IResource>(env.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(cloneOpts->ApplyOptions(*resource, scene));
    UpdateScene();

    EXPECT_FLOAT_EQ(META_NS::GetValue(env->EnvironmentMapLodLevel()), 2.0f);
    EXPECT_FLOAT_EQ(env->EnvironmentMapLodLevel()->GetDefaultValue(), 2.0f);
    EXPECT_FALSE(env->EnvironmentMapLodLevel().GetProperty()->IsValueSet());
}

/**
 * @tc.name: IResourceTemplateInterfaceCast
 * @tc.desc: EnvironmentTemplate supports interface_cast to IResourceTemplate
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, IResourceTemplateInterfaceCast, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto rt = interface_cast<IResourceTemplate>(obj.get());
    EXPECT_TRUE(rt);
}

/**
 * @tc.name: ResourceType
 * @tc.desc: EnvironmentTemplate reports correct resource type
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, ResourceType, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto resource = interface_cast<CORE_NS::IResource>(obj.get());
    ASSERT_TRUE(resource);

    EXPECT_EQ(resource->GetResourceType(), ClassId::EnvironmentResourceTemplate.Id().ToUid());
}

/**
 * @tc.name: CreateInstanceRoundTrip
 * @tc.desc: CreateInstance returns a live environment with the template's set properties applied
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, CreateInstanceRoundTrip, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);
    META_NS::SetValue(tmpl.EnvironmentMapLodLevel(), 3.25f);
    META_NS::SetValue(tmpl.Background(), EnvBackgroundType::CUBEMAP);

    auto env = tmpl.CreateInstance(scene);
    ASSERT_TRUE(env);
    UpdateScene();

    EXPECT_FLOAT_EQ(META_NS::GetValue(env->EnvironmentMapLodLevel()), 3.25f);
    EXPECT_EQ(META_NS::GetValue(env->Background()), EnvBackgroundType::CUBEMAP);
}

/**
 * @tc.name: CreateInstanceNullScene
 * @tc.desc: CreateInstance with null scene returns nullptr
 * @tc.type: FUNC
 */
UNIT_TEST_F(EnvironmentTemplateTest, CreateInstanceNullScene, testing::ext::TestSize.Level1)
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
UNIT_TEST_F(EnvironmentTemplateTest, CreateInstanceUninitializedWrapper, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    EnvironmentTemplate tmpl(META_NS::IObject::Ptr{});
    EXPECT_FALSE(tmpl);
    EXPECT_FALSE(tmpl.CreateInstance(scene));
}

}  // namespace UTest
SCENE_END_NAMESPACE()
