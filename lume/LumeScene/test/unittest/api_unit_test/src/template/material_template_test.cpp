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

#include <functional>
#include <scene/api/template/material_template.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/template/intf_resource_template.h>
#include <scene/interface/template/intf_template_options.h>

#include <3d/ecs/components/material_component.h>

#include <meta/api/metadata_util.h>
#include <meta/api/util.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/property/construct_array_property.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/resource/intf_resource.h>

#include "scene/scene_test.h"

using CORE3D_NS::MaterialComponent;

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class MaterialTemplateTest : public ScenePluginTest {
public:
    META_NS::IObject::Ptr CreateTemplateObj() const
    {
        return META_NS::GetObjectRegistry().Create<META_NS::IObject>(ClassId::MaterialTemplate);
    }

    static void AddScalarProperties(META_NS::IMetadata& meta)
    {
        meta.AddProperty(META_NS::ConstructProperty<MaterialType>("Type").GetProperty());
        meta.AddProperty(META_NS::ConstructProperty<float>("AlphaCutoff").GetProperty());
        meta.AddProperty(META_NS::ConstructProperty<LightingFlags>("LightingFlags").GetProperty());
        meta.AddProperty(META_NS::ConstructProperty<IShader::Ptr>("MaterialShader").GetProperty());
        meta.AddProperty(META_NS::ConstructProperty<IShader::Ptr>("DepthShader").GetProperty());
        meta.AddProperty(META_NS::ConstructProperty<RenderSort>("RenderSort").GetProperty());
    }

    static META_NS::IObject::Ptr CreateTemplateSampler()
    {
        auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(META_NS::ClassId::Object);
        if (auto m = interface_cast<META_NS::IMetadata>(obj.get())) {
            m->AddProperty(META_NS::ConstructProperty<SamplerFilter>("MagFilter").GetProperty());
            m->AddProperty(META_NS::ConstructProperty<SamplerFilter>("MinFilter").GetProperty());
            m->AddProperty(META_NS::ConstructProperty<SamplerFilter>("MipMapMode").GetProperty());
            m->AddProperty(META_NS::ConstructProperty<SamplerAddressMode>("AddressModeU").GetProperty());
            m->AddProperty(META_NS::ConstructProperty<SamplerAddressMode>("AddressModeV").GetProperty());
            m->AddProperty(META_NS::ConstructProperty<SamplerAddressMode>("AddressModeW").GetProperty());
        }
        return obj;
    }

    static META_NS::IObject::Ptr CreateTemplateTexture()
    {
        auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(META_NS::ClassId::Object);
        if (auto m = interface_cast<META_NS::IMetadata>(obj.get())) {
            m->AddProperty(META_NS::ConstructProperty<BASE_NS::string>("Name").GetProperty());
            m->AddProperty(META_NS::ConstructProperty<IImage::Ptr>("Image").GetProperty());
            m->AddProperty(META_NS::ConstructProperty<BASE_NS::Math::Vec4>("Factor").GetProperty());
            m->AddProperty(META_NS::ConstructProperty<BASE_NS::Math::Vec2>("Translation").GetProperty());
            m->AddProperty(META_NS::ConstructProperty<float>("Rotation").GetProperty());
            m->AddProperty(META_NS::ConstructProperty<BASE_NS::Math::Vec2>("Scale").GetProperty());
            auto sampler = CreateTemplateSampler();
            m->AddProperty(META_NS::ConstructProperty<META_NS::IObject::Ptr>("Sampler", sampler).GetProperty());
        }
        return obj;
    }

    static void AddTexturesProperty(META_NS::IMetadata& meta)
    {
        constexpr size_t textureCount = MaterialComponent::TextureIndex::TEXTURE_COUNT;
        // Slot names matching the standard PBR shader's materialMetadata.textures
        // declaration (test/unittest/assets/test_data/shaders/test.shader). Apply-time
        // texture routing matches against these names via ActiveTextureSlotInfo.
        static constexpr const char* SLOT_NAMES[textureCount] = {"baseColor",
            "normal",
            "material",
            "emissive",
            "ambientOcclusion",
            "clearcoat",
            "clearcoatRoughness",
            "clearcoatNormal",
            "sheen",
            "transmission",
            "specular"};
        BASE_NS::vector<META_NS::IObject::Ptr> textures;
        textures.reserve(textureCount);
        for (size_t ix = 0; ix < textureCount; ++ix) {
            auto tex = CreateTemplateTexture();
            if (auto m = interface_cast<META_NS::IMetadata>(tex.get())) {
                if (auto nameProp = m->GetProperty<BASE_NS::string>("Name")) {
                    META_NS::SetValue(nameProp, BASE_NS::string(SLOT_NAMES[ix]));
                }
            }
            textures.push_back(tex);
        }
        auto prop = META_NS::ConstructArrayProperty<META_NS::IObject::Ptr>("Textures", textures);
        meta.AddProperty(prop.GetProperty());
    }

    static void AddOptionsProperty(META_NS::IMetadata& meta)
    {
        auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(META_NS::ClassId::Object);
        if (auto m = interface_cast<META_NS::IMetadata>(obj.get())) {
            m->AddProperty(META_NS::ConstructProperty<CullModeFlags>("CullMode").GetProperty());
            m->AddProperty(META_NS::ConstructProperty<PolygonMode>("PolygonMode").GetProperty());
            m->AddProperty(META_NS::ConstructProperty<bool>("Blend").GetProperty());
        }
        meta.AddProperty(META_NS::ConstructProperty<META_NS::IObject::Ptr>("Options", obj).GetProperty());
    }

    MaterialTemplate CreateFullTemplate() const
    {
        auto obj = CreateTemplateObj();
        if (auto meta = interface_cast<META_NS::IMetadata>(obj.get())) {
            AddScalarProperties(*meta);
            AddTexturesProperty(*meta);
            AddOptionsProperty(*meta);
        }
        return MaterialTemplate(obj);
    }
};

/**
 * @tc.name: CreateMaterialTemplate
 * @tc.desc: MaterialTemplate can be created via object registry
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, CreateMaterialTemplate, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);
}

/**
 * @tc.name: PropertiesAccessible
 * @tc.desc: Dynamically added material template properties are accessible via API helper
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, PropertiesAccessible, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto named = interface_cast<META_NS::INamed>(tmpl.GetPtr().get());
    ASSERT_TRUE(named);
    EXPECT_TRUE(named->Name());

    EXPECT_TRUE(tmpl.Type());
    EXPECT_TRUE(tmpl.AlphaCutoff());
    EXPECT_TRUE(tmpl.LightingFlags());
    EXPECT_TRUE(tmpl.MaterialShader());
    EXPECT_TRUE(tmpl.DepthShader());
    EXPECT_TRUE(tmpl.RenderSort());
    EXPECT_TRUE(tmpl.Textures());
}

/**
 * @tc.name: TexturesPrePopulated
 * @tc.desc: Textures array has TEXTURE_COUNT entries after initialization
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, TexturesPrePopulated, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto textures = tmpl.Textures();
    ASSERT_TRUE(textures);

    constexpr size_t textureCount = MaterialComponent::TextureIndex::TEXTURE_COUNT;
    EXPECT_EQ(textures.GetSize(), textureCount);
}

/**
 * @tc.name: OptionsAccessible
 * @tc.desc: MaterialTemplate exposes the graphics-state Options sub-object via the API view
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, OptionsAccessible, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto options = tmpl.Options();
    ASSERT_TRUE(options);

    ASSERT_TRUE(options.CullMode());
    ASSERT_TRUE(options.PolygonMode());
    ASSERT_TRUE(options.Blend());

    META_NS::SetValue(options.CullMode(), CullModeFlags::FRONT_BIT);
    META_NS::SetValue(options.PolygonMode(), PolygonMode::LINE);
    META_NS::SetValue(options.Blend(), true);

    EXPECT_EQ(META_NS::GetValue(options.CullMode()), CullModeFlags::FRONT_BIT);
    EXPECT_EQ(META_NS::GetValue(options.PolygonMode()), PolygonMode::LINE);
    EXPECT_TRUE(META_NS::GetValue(options.Blend()));

    // Construct a live material instance from the template and verify the
    // options landed on its bound shader.
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto material = tmpl.CreateInstance(scene);
    ASSERT_TRUE(material);
    UpdateScene();

    auto shader = META_NS::GetValue(material->MaterialShader());
    ASSERT_TRUE(shader);
    EXPECT_EQ(META_NS::GetValue(shader->CullMode()), CullModeFlags::FRONT_BIT);
    EXPECT_EQ(META_NS::GetValue(shader->PolygonMode()), PolygonMode::LINE);
    EXPECT_TRUE(META_NS::GetValue(shader->Blend()));
}

/**
 * @tc.name: TexturesHaveSamplers
 * @tc.desc: Each texture has a valid Sampler sub-object
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, TexturesHaveSamplers, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto textures = tmpl.Textures();
    ASSERT_TRUE(textures);

    for (size_t i = 0; i < textures.GetSize(); ++i) {
        auto texView = textures.GetValueAt(i);
        ASSERT_TRUE(texView) << "Texture at index " << i << " is null";
        auto samplerView = texView.Sampler();
        EXPECT_TRUE(samplerView) << "Sampler at texture index " << i << " is null";
    }
}

/**
 * @tc.name: PropertiesDefaultNotSet
 * @tc.desc: Newly added properties are not marked as set
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, PropertiesDefaultNotSet, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto named = interface_cast<META_NS::INamed>(tmpl.GetPtr().get());
    ASSERT_TRUE(named);
    EXPECT_FALSE(META_NS::IsValueSet(*named->Name().GetProperty()));

    EXPECT_FALSE(META_NS::IsValueSet(*tmpl.AlphaCutoff().GetProperty()));
    EXPECT_FALSE(META_NS::IsValueSet(*tmpl.Type().GetProperty()));
    EXPECT_FALSE(META_NS::IsValueSet(*tmpl.LightingFlags().GetProperty()));
}

/**
 * @tc.name: NameProperty
 * @tc.desc: MaterialTemplate supports INamed and GetName returns the Name property value
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, NameProperty, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto named = interface_cast<META_NS::INamed>(obj.get());
    ASSERT_TRUE(named);

    META_NS::SetValue(named->Name(), BASE_NS::string("TestMaterial"));
    EXPECT_TRUE(META_NS::IsValueSet(*named->Name().GetProperty()));
    EXPECT_EQ(META_NS::GetValue(named->Name()), "TestMaterial");
    EXPECT_EQ(obj->GetName(), "TestMaterial");
}

/**
 * @tc.name: TextureNameProperty
 * @tc.desc: Template texture Name property is accessible via TemplateTextureView
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, TextureNameProperty, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto textures = tmpl.Textures();
    ASSERT_TRUE(textures);
    ASSERT_GT(textures.GetSize(), 0u);

    auto texView = textures.GetValueAt(0);
    ASSERT_TRUE(texView);
    ASSERT_TRUE(texView.Name());

    META_NS::SetValue(texView.Name(), BASE_NS::string("baseColor"));
    EXPECT_EQ(META_NS::GetValue(texView.Name()), "baseColor");
}

/**
 * @tc.name: SetPropertyMarksAsSet
 * @tc.desc: Setting a property value marks it as set
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, SetPropertyMarksAsSet, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    META_NS::SetValue(tmpl.AlphaCutoff(), 0.5f);
    EXPECT_TRUE(META_NS::IsValueSet(*tmpl.AlphaCutoff().GetProperty()));
    EXPECT_FLOAT_EQ(META_NS::GetValue(tmpl.AlphaCutoff()), 0.5f);
}

/**
 * @tc.name: ApplyToMaterial
 * @tc.desc: ApplyOptions copies only set properties to a live material
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, ApplyToMaterial, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    const float testAlpha = 0.75f;
    META_NS::SetValue(tmpl.AlphaCutoff(), testAlpha);

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);
    auto resource = interface_cast<CORE_NS::IResource>(material.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(options->ApplyOptions(*resource, scene));
    UpdateScene();

    EXPECT_FLOAT_EQ(META_NS::GetValue(material->AlphaCutoff()), testAlpha);
}

/**
 * @tc.name: ApplyToOnlyCopiesSetProperties
 * @tc.desc: ApplyOptions does not overwrite properties that are not set on the template
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, ApplyToOnlyCopiesSetProperties, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();

    const float originalAlpha = META_NS::GetValue(material->AlphaCutoff());

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);
    // Don't set AlphaCutoff on template

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);
    auto resource = interface_cast<CORE_NS::IResource>(material.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(options->ApplyOptions(*resource, scene));
    UpdateScene();

    EXPECT_FLOAT_EQ(META_NS::GetValue(material->AlphaCutoff()), originalAlpha);
}

/**
 * @tc.name: CopyFromMaterial
 * @tc.desc: UpdateOptions copies set properties from a live material
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, CopyFromMaterial, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();

    const float testAlpha = 0.33f;
    META_NS::SetValue(material->AlphaCutoff(), testAlpha);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);
    auto resource = interface_cast<CORE_NS::IResource>(material.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(options->UpdateOptions(*resource, scene));
    EXPECT_FLOAT_EQ(META_NS::GetValue(tmpl.AlphaCutoff()), testAlpha);
}

/**
 * @tc.name: CloneTemplate
 * @tc.desc: Clone produces an independent copy of the template
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, CloneTemplate, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto named = interface_cast<META_NS::INamed>(tmpl.GetPtr().get());
    ASSERT_TRUE(named);
    META_NS::SetValue(named->Name(), BASE_NS::string("OriginalMat"));
    META_NS::SetValue(tmpl.AlphaCutoff(), 0.5f);

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);

    auto cloneOptions = options->Clone();
    ASSERT_TRUE(cloneOptions);

    auto cloneMeta = interface_cast<META_NS::IMetadata>(cloneOptions.get());
    ASSERT_TRUE(cloneMeta);

    auto cloneAlpha = cloneMeta->GetProperty<float>("AlphaCutoff");
    ASSERT_TRUE(cloneAlpha);
    EXPECT_FLOAT_EQ(META_NS::GetValue(cloneAlpha), 0.5f);

    auto cloneNamed = interface_cast<META_NS::INamed>(cloneOptions.get());
    ASSERT_TRUE(cloneNamed);
    EXPECT_EQ(META_NS::GetValue(cloneNamed->Name()), "OriginalMat");

    // Verify independence: changing clone doesn't affect original
    META_NS::SetValue(cloneAlpha, 0.9f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(tmpl.AlphaCutoff()), 0.5f);
}

/**
 * @tc.name: MergeCopiesOtherSetValues
 * @tc.desc: Merge promotes this's set values to defaults, then copies other's set values
 *           as set values into this. This encodes the two-layer model: template defaults
 *           + scene overlay set values on a single MaterialTemplate.
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, MergeCopiesOtherSetValues, testing::ext::TestSize.Level1)
{
    auto dstObj = CreateTemplateObj();
    ASSERT_TRUE(dstObj);

    auto srcObj = CreateTemplateObj();
    ASSERT_TRUE(srcObj);
    if (auto srcMeta = interface_cast<META_NS::IMetadata>(srcObj.get())) {
        srcMeta->AddProperty(META_NS::ConstructProperty<float>("AlphaCutoff").GetProperty());
    }
    auto src = MaterialTemplate(srcObj);
    META_NS::SetValue(src.AlphaCutoff(), 0.6f);

    auto dstOptions = interface_cast<CORE_NS::IResourceOptions>(dstObj.get());
    auto srcOptions = interface_cast<CORE_NS::IResourceOptions>(srcObj.get());
    ASSERT_TRUE(dstOptions && srcOptions);

    EXPECT_TRUE(dstOptions->Merge(*srcOptions));

    // Other's set value is layered as a set value on dst.
    auto dstMeta = interface_cast<META_NS::IMetadata>(dstObj.get());
    ASSERT_TRUE(dstMeta);
    auto dstAlpha = dstMeta->GetProperty<float>("AlphaCutoff");
    ASSERT_TRUE(dstAlpha);
    EXPECT_TRUE(dstAlpha.GetProperty()->IsValueSet());
    EXPECT_FLOAT_EQ(META_NS::GetValue(dstAlpha), 0.6f);
}

/**
 * @tc.name: MergeLayersSceneOverTemplate
 * @tc.desc: When a scene-side options (tc=false) merges into a template options, the
 *           template values become defaults and scene values become set values. After
 *           ApplyOptions the live material shows the scene value as current and the
 *           template value as the property default.
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, MergeLayersSceneOverTemplate, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);
    META_NS::SetValue(tmpl.AlphaCutoff(), 0.5f);
    META_NS::SetValue(tmpl.LightingFlags(), static_cast<LightingFlags>(2));

    auto sceneObj = CreateTemplateObj();
    ASSERT_TRUE(sceneObj);
    if (auto sm = interface_cast<META_NS::IMetadata>(sceneObj.get())) {
        sm->AddProperty(META_NS::ConstructProperty<float>("AlphaCutoff").GetProperty());
    }
    auto sceneOpts = MaterialTemplate(sceneObj);
    META_NS::SetValue(sceneOpts.AlphaCutoff(), 0.75f);

    auto tmplOptions = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    auto sceneOptOptions = interface_cast<CORE_NS::IResourceOptions>(sceneObj.get());
    ASSERT_TRUE(tmplOptions && sceneOptOptions);
    EXPECT_TRUE(tmplOptions->Merge(*sceneOptOptions));

    // After merge: template alphaCutoff promoted to default (0.5), scene alphaCutoff is set value (0.75).
    EXPECT_TRUE(tmpl.AlphaCutoff().GetProperty()->IsValueSet());

    // Simulate PrepareResourceData: clone + SetTemplateContext(true).
    auto cloneOpts = tmplOptions->Clone();
    ASSERT_TRUE(cloneOpts);
    auto cloneTc = interface_cast<ITemplateOptions>(cloneOpts.get());
    ASSERT_TRUE(cloneTc);
    cloneTc->SetTemplateContext(true);

    auto resource = interface_cast<CORE_NS::IResource>(material.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(cloneOpts->ApplyOptions(*resource, scene));
    UpdateScene();

    EXPECT_FLOAT_EQ(META_NS::GetValue(material->AlphaCutoff()), 0.75f);
    EXPECT_FLOAT_EQ(material->AlphaCutoff()->GetDefaultValue(), 0.5f);
    EXPECT_EQ(META_NS::GetValue(material->LightingFlags()), static_cast<LightingFlags>(2));
}

/**
 * @tc.name: MergeReimportPreservesSceneValues
 * @tc.desc: When a node template is re-imported (template→template merge), scene-side set
 *           values carried by the prior template survive in the set-value slots so they
 *           still override the new template's defaults.
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, MergeReimportPreservesSceneValues, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();

    // v1 template with scene overlay merged in
    auto v1Obj = CreateTemplateObj();
    ASSERT_TRUE(v1Obj);
    if (auto m = interface_cast<META_NS::IMetadata>(v1Obj.get())) {
        m->AddProperty(META_NS::ConstructProperty<float>("AlphaCutoff").GetProperty());
    }
    auto v1 = MaterialTemplate(v1Obj);
    META_NS::SetValue(v1.AlphaCutoff(), 0.5f);
    auto v1Options = interface_cast<CORE_NS::IResourceOptions>(v1Obj.get());
    ASSERT_TRUE(v1Options);

    auto sceneObj = CreateTemplateObj();
    ASSERT_TRUE(sceneObj);
    if (auto m = interface_cast<META_NS::IMetadata>(sceneObj.get())) {
        m->AddProperty(META_NS::ConstructProperty<float>("AlphaCutoff").GetProperty());
    }
    auto sceneOpts = MaterialTemplate(sceneObj);
    META_NS::SetValue(sceneOpts.AlphaCutoff(), 0.3f);
    auto sceneOptOptions = interface_cast<CORE_NS::IResourceOptions>(sceneObj.get());
    ASSERT_TRUE(sceneOptOptions);
    EXPECT_TRUE(v1Options->Merge(*sceneOptOptions));

    // Simulate PrepareResourceData on v1.
    auto v1Clone = v1Options->Clone();
    ASSERT_TRUE(v1Clone);
    auto v1CloneTc = interface_cast<ITemplateOptions>(v1Clone.get());
    ASSERT_TRUE(v1CloneTc);
    v1CloneTc->SetTemplateContext(true);

    // v2 template re-import: merges over the stored v1 clone.
    auto v2Obj = CreateTemplateObj();
    ASSERT_TRUE(v2Obj);
    auto v2Options = interface_cast<CORE_NS::IResourceOptions>(v2Obj.get());
    ASSERT_TRUE(v2Options);
    EXPECT_TRUE(v2Options->Merge(*v1Clone));

    auto v2Clone = v2Options->Clone();
    ASSERT_TRUE(v2Clone);
    auto v2CloneTc = interface_cast<ITemplateOptions>(v2Clone.get());
    ASSERT_TRUE(v2CloneTc);
    v2CloneTc->SetTemplateContext(true);

    auto resource = interface_cast<CORE_NS::IResource>(material.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(v2Clone->ApplyOptions(*resource, scene));
    UpdateScene();

    // v2 has no alphaCutoff, but the scene value (0.3) survives across re-import.
    EXPECT_FLOAT_EQ(META_NS::GetValue(material->AlphaCutoff()), 0.3f);
}

/**
 * @tc.name: SetTemplateContextPromotesValues
 * @tc.desc: When SetTemplateContext(true) is called on a template that wasn't merged
 *           (no-merge path), its set values are promoted to defaults so ApplyOptions
 *           lands them as property defaults on the live material.
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, SetTemplateContextPromotesValues, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);
    META_NS::SetValue(tmpl.AlphaCutoff(), 0.5f);

    // Simulate PrepareResourceData without merge.
    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);
    auto cloneOpts = options->Clone();
    ASSERT_TRUE(cloneOpts);
    auto cloneTc = interface_cast<ITemplateOptions>(cloneOpts.get());
    ASSERT_TRUE(cloneTc);
    cloneTc->SetTemplateContext(true);

    auto resource = interface_cast<CORE_NS::IResource>(material.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(cloneOpts->ApplyOptions(*resource, scene));
    UpdateScene();

    // The template value should be the default, not a set value.
    EXPECT_FLOAT_EQ(META_NS::GetValue(material->AlphaCutoff()), 0.5f);
    EXPECT_FLOAT_EQ(material->AlphaCutoff()->GetDefaultValue(), 0.5f);
    EXPECT_FALSE(material->AlphaCutoff().GetProperty()->IsValueSet());
}

/**
 * @tc.name: MergePropagatesBaseResource
 * @tc.desc: Merge propagates a valid baseResource_ from source to destination.
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, MergePropagatesBaseResource, testing::ext::TestSize.Level1)
{
    auto dst = CreateTemplateObj();
    auto src = CreateTemplateObj();
    ASSERT_TRUE(dst && src);

    auto srcDerived = interface_cast<META_NS::IDerivedResourceOptions>(src.get());
    ASSERT_TRUE(srcDerived);
    CORE_NS::ResourceId base{"base-mat", "group-a"};
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
 * @tc.name: ClonePropagatesBaseAndTemplateContext
 * @tc.desc: Clone copies baseResource_ and templateContext_ along with property data.
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, ClonePropagatesBaseAndTemplateContext, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto derived = interface_cast<META_NS::IDerivedResourceOptions>(obj.get());
    auto tc = interface_cast<ITemplateOptions>(obj.get());
    ASSERT_TRUE(derived && tc);

    CORE_NS::ResourceId base{"base-mat", "group-a"};
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
 * @tc.name: TextureFactorApplyTo
 * @tc.desc: Setting a texture factor on template applies to material via ApplyOptions
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, TextureFactorApplyTo, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    auto tmplTextures = tmpl.Textures();
    ASSERT_TRUE(tmplTextures);
    ASSERT_GT(tmplTextures.GetSize(), 0u);

    auto tmplTex = tmplTextures.GetValueAt(0);
    ASSERT_TRUE(tmplTex);

    const BASE_NS::Math::Vec4 testFactor{0.1f, 0.2f, 0.3f, 0.4f};
    META_NS::SetValue(tmplTex.Factor(), testFactor);

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);
    auto resource = interface_cast<CORE_NS::IResource>(material.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(options->ApplyOptions(*resource, scene));
    UpdateScene();

    auto matTextures = material->Textures();
    ASSERT_TRUE(matTextures);
    ASSERT_GT(matTextures->GetSize(), 0u);

    auto matTex = matTextures->GetValueAt(0);
    ASSERT_TRUE(matTex);

    auto factor = META_NS::GetValue(matTex->Factor());
    EXPECT_FLOAT_EQ(factor.x, testFactor.x);
    EXPECT_FLOAT_EQ(factor.y, testFactor.y);
    EXPECT_FLOAT_EQ(factor.z, testFactor.z);
    EXPECT_FLOAT_EQ(factor.w, testFactor.w);
}

/**
 * @tc.name: ApplyToViaInterface
 * @tc.desc: IResourceTemplate::ApplyTo copies set properties to a live material
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, ApplyToViaInterface, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);
    META_NS::SetValue(tmpl.AlphaCutoff(), 0.42f);

    EXPECT_TRUE(tmpl.ApplyTo(*material));
    UpdateScene();

    EXPECT_FLOAT_EQ(META_NS::GetValue(material->AlphaCutoff()), 0.42f);
}

/**
 * @tc.name: CopyFromViaInterface
 * @tc.desc: IResourceTemplate::CopyFrom copies set properties from a live material
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, CopyFromViaInterface, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();

    META_NS::SetValue(material->AlphaCutoff(), 0.66f);
    UpdateScene();

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    EXPECT_TRUE(tmpl.CopyFrom(static_cast<const IMaterial&>(*material)));
    EXPECT_FLOAT_EQ(META_NS::GetValue(tmpl.AlphaCutoff()), 0.66f);
}

/**
 * @tc.name: CopyFromAnotherTemplate
 * @tc.desc: IResourceTemplate::CopyFrom copies set properties from another MaterialTemplate
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, CopyFromAnotherTemplate, testing::ext::TestSize.Level1)
{
    auto srcObj = CreateTemplateObj();
    ASSERT_TRUE(srcObj);
    if (auto m = interface_cast<META_NS::IMetadata>(srcObj.get())) {
        AddScalarProperties(*m);
    }
    auto src = MaterialTemplate(srcObj);
    META_NS::SetValue(src.AlphaCutoff(), 0.55f);
    META_NS::SetValue(src.Type(), MaterialType::UNLIT);

    auto dst = CreateFullTemplate();
    ASSERT_TRUE(dst);
    auto dstTempl = dst.GetPtr<IResourceTemplate>();
    ASSERT_TRUE(dstTempl);
    EXPECT_TRUE(dstTempl->CopyFrom(static_cast<const META_NS::IObject&>(*srcObj)));

    EXPECT_FLOAT_EQ(META_NS::GetValue(dst.AlphaCutoff()), 0.55f);
    EXPECT_EQ(META_NS::GetValue(dst.Type()), MaterialType::UNLIT);
}

/**
 * @tc.name: CopyFromWithDefaultsCopiesAll
 * @tc.desc: IResourceTemplate::CopyFrom with onlySetValues=false propagates
 *           source defaults and resets destination values that were not set on
 *           the source.
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, CopyFromWithDefaultsCopiesAll, testing::ext::TestSize.Level1)
{
    auto srcObj = CreateTemplateObj();
    ASSERT_TRUE(srcObj);
    if (auto m = interface_cast<META_NS::IMetadata>(srcObj.get())) {
        AddScalarProperties(*m);
    }
    auto src = MaterialTemplate(srcObj);
    // Leave AlphaCutoff unset on source; set Type only.
    META_NS::SetValue(src.Type(), MaterialType::UNLIT);

    auto dst = CreateFullTemplate();
    ASSERT_TRUE(dst);
    // Pre-set AlphaCutoff on destination so we can observe it being reset.
    META_NS::SetValue(dst.AlphaCutoff(), 0.11f);
    EXPECT_TRUE(dst.AlphaCutoff()->IsValueSet());

    auto dstTempl = dst.GetPtr<IResourceTemplate>();
    ASSERT_TRUE(dstTempl);
    EXPECT_TRUE(dstTempl->CopyFrom(static_cast<const META_NS::IObject&>(*srcObj), /*onlySetValues=*/false));

    // Set source property copied as a value.
    EXPECT_EQ(META_NS::GetValue(dst.Type()), MaterialType::UNLIT);
    // Unset source property reset the destination value (defaults-mode copy).
    EXPECT_FALSE(dst.AlphaCutoff()->IsValueSet());
}

/**
 * @tc.name: CopyFromOnlySetValuesLeavesDestUnset
 * @tc.desc: IResourceTemplate::CopyFrom default (onlySetValues=true) does not
 *           touch destination properties that were not set on source.
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, CopyFromOnlySetValuesLeavesDestUnset, testing::ext::TestSize.Level1)
{
    auto srcObj = CreateTemplateObj();
    ASSERT_TRUE(srcObj);
    if (auto m = interface_cast<META_NS::IMetadata>(srcObj.get())) {
        AddScalarProperties(*m);
    }
    auto src = MaterialTemplate(srcObj);
    // Leave AlphaCutoff unset on source.
    META_NS::SetValue(src.Type(), MaterialType::UNLIT);

    auto dst = CreateFullTemplate();
    ASSERT_TRUE(dst);
    META_NS::SetValue(dst.AlphaCutoff(), 0.11f);

    auto dstTempl = dst.GetPtr<IResourceTemplate>();
    ASSERT_TRUE(dstTempl);
    EXPECT_TRUE(dstTempl->CopyFrom(static_cast<const META_NS::IObject&>(*srcObj)));

    EXPECT_EQ(META_NS::GetValue(dst.Type()), MaterialType::UNLIT);
    // AlphaCutoff on dst should be untouched by set-values-only copy.
    EXPECT_TRUE(dst.AlphaCutoff()->IsValueSet());
    EXPECT_FLOAT_EQ(META_NS::GetValue(dst.AlphaCutoff()), 0.11f);
}

/**
 * @tc.name: IResourceTemplateInterfaceCast
 * @tc.desc: MaterialTemplate supports interface_cast to IResourceTemplate
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, IResourceTemplateInterfaceCast, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto rt = interface_cast<IResourceTemplate>(obj.get());
    EXPECT_TRUE(rt);
}

/**
 * @tc.name: ResourceType
 * @tc.desc: MaterialTemplate reports correct resource type
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, ResourceType, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto resource = interface_cast<CORE_NS::IResource>(obj.get());
    ASSERT_TRUE(resource);

    EXPECT_EQ(resource->GetResourceType(), ClassId::MaterialResourceTemplate.Id().ToUid());
}

/**
 * @tc.name: CreateInstanceRoundTrip
 * @tc.desc: CreateInstance returns a live material with the template's set properties applied
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, CreateInstanceRoundTrip, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);
    META_NS::SetValue(tmpl.AlphaCutoff(), 0.42f);
    META_NS::SetValue(tmpl.Type(), MaterialType::UNLIT);

    auto material = tmpl.CreateInstance(scene);
    ASSERT_TRUE(material);
    UpdateScene();

    EXPECT_FLOAT_EQ(META_NS::GetValue(material->AlphaCutoff()), 0.42f);
    EXPECT_EQ(META_NS::GetValue(material->Type()), MaterialType::UNLIT);
}

/**
 * @tc.name: CreateInstanceOcclusionType
 * @tc.desc: CreateInstance selects the OcclusionMaterial class for OCCLUSION type
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, CreateInstanceOcclusionType, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);
    META_NS::SetValue(tmpl.Type(), MaterialType::OCCLUSION);

    auto material = tmpl.CreateInstance(scene);
    ASSERT_TRUE(material);
    UpdateScene();

    auto obj = interface_cast<META_NS::IObject>(material.get());
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->GetClassId(), ClassId::OcclusionMaterial.Id());
}

/**
 * @tc.name: CreateInstanceNullScene
 * @tc.desc: CreateInstance with null scene returns nullptr
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, CreateInstanceNullScene, testing::ext::TestSize.Level1)
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
UNIT_TEST_F(MaterialTemplateTest, CreateInstanceUninitializedWrapper, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    MaterialTemplate tmpl(META_NS::IObject::Ptr{});
    EXPECT_FALSE(tmpl);
    EXPECT_FALSE(tmpl.CreateInstance(scene));
}

// Build a template that contains *only* a single sparse texture entry, so we
// can exercise apply-time slot routing without the synthetic 11-slot test
// fixture pre-shaping every entry. The entry is sparse — only the JSON-style
// fields the caller specifies are present.
static META_NS::IObject::Ptr MakeSparseTemplateWithOneTexture(
    BASE_NS::string_view slotName, std::function<void(META_NS::IMetadata&)> populate)
{
    auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(ClassId::MaterialTemplate);
    auto tmplMeta = interface_cast<META_NS::IMetadata>(obj.get());
    if (!tmplMeta) {
        return obj;
    }
    auto entry = META_NS::GetObjectRegistry().Create<META_NS::IObject>(META_NS::ClassId::Object);
    auto entryMeta = interface_cast<META_NS::IMetadata>(entry.get());
    if (entryMeta) {
        auto nameProp = META_NS::ConstructProperty<BASE_NS::string>("Name", BASE_NS::string(slotName));
        entryMeta->AddProperty(nameProp.GetProperty());
        if (populate) {
            populate(*entryMeta);
        }
    }
    BASE_NS::vector<META_NS::IObject::Ptr> entries{entry};
    auto prop = META_NS::ConstructArrayProperty<META_NS::IObject::Ptr>("Textures", entries);
    tmplMeta->AddProperty(prop.GetProperty());
    return obj;
}

/**
 * @tc.name: ApplyOnlyCopiesPresentFields
 * @tc.desc: A template entry with only `factor` set leaves the live material's
 *           translation / rotation / scale untouched on the same slot.
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, ApplyOnlyCopiesPresentFields, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();

    // Pre-set translation and rotation on the live material's BASE_COLOR slot.
    const BASE_NS::Math::Vec2 priorTranslation{0.7f, 0.8f};
    const float priorRotation = 1.25f;
    auto matTextures = material->Textures();
    ASSERT_TRUE(matTextures);
    auto liveTex = matTextures->GetValueAt(0);
    ASSERT_TRUE(liveTex);
    META_NS::SetValue(liveTex->Translation(), priorTranslation);
    META_NS::SetValue(liveTex->Rotation(), priorRotation);

    // Build a sparse template with only `factor` for baseColor.
    const BASE_NS::Math::Vec4 newFactor{0.4f, 0.3f, 0.2f, 0.1f};
    auto tmplObj = MakeSparseTemplateWithOneTexture("baseColor", [&newFactor](META_NS::IMetadata& m) {
        auto fp = META_NS::ConstructProperty<BASE_NS::Math::Vec4>("Factor");
        m.AddProperty(fp.GetProperty());
        META_NS::SetValue<BASE_NS::Math::Vec4>(fp.GetProperty(), newFactor);
    });
    auto options = interface_cast<CORE_NS::IResourceOptions>(tmplObj.get());
    ASSERT_TRUE(options);
    auto resource = interface_cast<CORE_NS::IResource>(material.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(options->ApplyOptions(*resource, scene));
    UpdateScene();

    // Factor was updated; translation and rotation must be left alone.
    EXPECT_EQ(META_NS::GetValue(liveTex->Factor()), newFactor);
    EXPECT_EQ(META_NS::GetValue(liveTex->Translation()), priorTranslation);
    EXPECT_FLOAT_EQ(META_NS::GetValue(liveTex->Rotation()), priorRotation);
}

/**
 * @tc.name: ApplyDoesNotTouchUnnamedSlots
 * @tc.desc: A template that names only "emissive" must not write anything into
 *           BASE_COLOR or other slots the JSON didn't mention.
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, ApplyDoesNotTouchUnnamedSlots, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();

    // Pre-set distinctive values on BASE_COLOR.
    auto matTextures = material->Textures();
    ASSERT_TRUE(matTextures);
    auto baseColor = matTextures->GetValueAt(0);
    ASSERT_TRUE(baseColor);
    const BASE_NS::Math::Vec4 priorBaseFactor{0.9f, 0.8f, 0.7f, 0.6f};
    META_NS::SetValue(baseColor->Factor(), priorBaseFactor);

    // Apply a template that only declares "emissive".
    const BASE_NS::Math::Vec4 emissiveFactor{0.1f, 0.1f, 0.1f, 0.1f};
    auto tmplObj = MakeSparseTemplateWithOneTexture("emissive", [&emissiveFactor](META_NS::IMetadata& m) {
        auto fp = META_NS::ConstructProperty<BASE_NS::Math::Vec4>("Factor");
        m.AddProperty(fp.GetProperty());
        META_NS::SetValue<BASE_NS::Math::Vec4>(fp.GetProperty(), emissiveFactor);
    });
    auto options = interface_cast<CORE_NS::IResourceOptions>(tmplObj.get());
    ASSERT_TRUE(options);
    auto resource = interface_cast<CORE_NS::IResource>(material.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(options->ApplyOptions(*resource, scene));
    UpdateScene();

    // BASE_COLOR is untouched.
    EXPECT_EQ(META_NS::GetValue(baseColor->Factor()), priorBaseFactor);
    // EMISSIVE got the new factor (slot index 3 in MaterialComponent::TextureIndex).
    auto emissive = matTextures->GetValueAt(MaterialComponent::TextureIndex::EMISSIVE);
    ASSERT_TRUE(emissive);
    EXPECT_EQ(META_NS::GetValue(emissive->Factor()), emissiveFactor);
}

/**
 * @tc.name: ApplyUnknownSlotIsNoOp
 * @tc.desc: A template entry whose name is not declared by the bound shader is
 *           skipped (warning logged) rather than mis-routed.
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, ApplyUnknownSlotIsNoOp, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();

    // Capture a snapshot of every slot's factor before apply.
    auto matTextures = material->Textures();
    ASSERT_TRUE(matTextures);
    BASE_NS::vector<BASE_NS::Math::Vec4> before;
    for (size_t i = 0; i < matTextures->GetSize(); ++i) {
        before.push_back(META_NS::GetValue(matTextures->GetValueAt(i)->Factor()));
    }

    const BASE_NS::Math::Vec4 unusedFactor{0.5f, 0.5f, 0.5f, 0.5f};
    auto tmplObj = MakeSparseTemplateWithOneTexture("notARealSlot", [&unusedFactor](META_NS::IMetadata& m) {
        auto fp = META_NS::ConstructProperty<BASE_NS::Math::Vec4>("Factor");
        m.AddProperty(fp.GetProperty());
        META_NS::SetValue<BASE_NS::Math::Vec4>(fp.GetProperty(), unusedFactor);
    });
    auto options = interface_cast<CORE_NS::IResourceOptions>(tmplObj.get());
    ASSERT_TRUE(options);
    auto resource = interface_cast<CORE_NS::IResource>(material.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(options->ApplyOptions(*resource, scene));
    UpdateScene();

    // No slot moved.
    for (size_t i = 0; i < matTextures->GetSize(); ++i) {
        EXPECT_EQ(META_NS::GetValue(matTextures->GetValueAt(i)->Factor()), before[i])
            << "slot " << i << " was unexpectedly modified by apply of unknown name";
    }
}

/**
 * @tc.name: ApplyTemplateContextLandsAsDefaults
 * @tc.desc: When ApplyOptions runs with templateContext_ set, the template's texture values
 *           should land on the live material's slots as *defaults* (overrideable) — not as
 *           set values. Mirrors the behaviour the rest of ApplyOptions has for non-texture
 *           properties (see MergeLayersSceneOverTemplate's defaults assertion on AlphaCutoff).
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, ApplyTemplateContextLandsAsDefaults, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();

    const BASE_NS::Math::Vec4 templFactor{0.2f, 0.4f, 0.6f, 0.8f};
    auto tmplObj = MakeSparseTemplateWithOneTexture("baseColor", [&templFactor](META_NS::IMetadata& m) {
        auto fp = META_NS::ConstructProperty<BASE_NS::Math::Vec4>("Factor");
        m.AddProperty(fp.GetProperty());
        META_NS::SetValue<BASE_NS::Math::Vec4>(fp.GetProperty(), templFactor);
    });
    auto tc = interface_cast<ITemplateOptions>(tmplObj.get());
    ASSERT_TRUE(tc);
    tc->SetTemplateContext(true);

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmplObj.get());
    ASSERT_TRUE(options);
    auto resource = interface_cast<CORE_NS::IResource>(material.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(options->ApplyOptions(*resource, scene));
    UpdateScene();

    auto matTextures = material->Textures();
    ASSERT_TRUE(matTextures);
    auto liveTex = matTextures->GetValueAt(0);
    ASSERT_TRUE(liveTex);
    auto factorProp = liveTex->Factor();
    ASSERT_TRUE(factorProp);

    // Template-context apply lands as defaults: the value is observable but IsValueSet() is
    // false, so a subsequent scene-side override can replace it.
    EXPECT_EQ(META_NS::GetValue(factorProp), templFactor);
    EXPECT_FALSE(factorProp.GetProperty()->IsValueSet())
        << "template-context apply should write defaults, not set values";
    EXPECT_EQ(factorProp->GetDefaultValue(), templFactor);
}

/**
 * @tc.name: ApplySamplerSubfieldSurgical
 * @tc.desc: A sparse sampler entry that declares only `magFilter` must update that subfield
 *           on the live texture's sampler and leave every other sampler subfield (e.g.
 *           addressModeW) at its previously-set value. Exercises the sub-object recursion
 *           in CopyEntryFieldsByName.
 * @tc.type: FUNC
 */
UNIT_TEST_F(MaterialTemplateTest, ApplySamplerSubfieldSurgical, testing::ext::TestSize.Level1)
{
    ScenePluginTest::SetUp();
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto material = CreateMaterial();
    ASSERT_TRUE(material);
    UpdateScene();

    auto matTextures = material->Textures();
    ASSERT_TRUE(matTextures);
    auto liveTex = matTextures->GetValueAt(0);
    ASSERT_TRUE(liveTex);
    auto liveSampler = META_NS::GetValue(liveTex->Sampler());
    ASSERT_TRUE(liveSampler);

    // Pre-set a sampler subfield on the live material that the sparse template will NOT
    // declare. It must survive the apply.
    META_NS::SetValue(liveSampler->AddressModeW(), SCENE_NS::SamplerAddressMode::MIRRORED_REPEAT);
    META_NS::SetValue(liveSampler->MinFilter(), SCENE_NS::SamplerFilter::LINEAR);

    // Build a template entry that declares only sampler.magFilter — nothing else.
    auto tmplObj = MakeSparseTemplateWithOneTexture("baseColor", [](META_NS::IMetadata& m) {
        auto samplerObj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(META_NS::ClassId::Object);
        if (auto sm = interface_cast<META_NS::IMetadata>(samplerObj.get())) {
            auto magProp = META_NS::ConstructProperty<SCENE_NS::SamplerFilter>("MagFilter");
            sm->AddProperty(magProp.GetProperty());
            META_NS::SetValue<SCENE_NS::SamplerFilter>(magProp.GetProperty(), SCENE_NS::SamplerFilter::NEAREST);
        }
        auto sp = META_NS::ConstructProperty<META_NS::IObject::Ptr>("Sampler", samplerObj);
        m.AddProperty(sp.GetProperty());
    });
    auto options = interface_cast<CORE_NS::IResourceOptions>(tmplObj.get());
    ASSERT_TRUE(options);
    auto resource = interface_cast<CORE_NS::IResource>(material.get());
    ASSERT_TRUE(resource);
    EXPECT_TRUE(options->ApplyOptions(*resource, scene));
    UpdateScene();

    // magFilter was updated; addressModeW and minFilter must be left alone.
    EXPECT_EQ(META_NS::GetValue(liveSampler->MagFilter()), SCENE_NS::SamplerFilter::NEAREST);
    EXPECT_EQ(META_NS::GetValue(liveSampler->AddressModeW()), SCENE_NS::SamplerAddressMode::MIRRORED_REPEAT)
        << "sibling sampler subfield must not be clobbered by sparse apply";
    EXPECT_EQ(META_NS::GetValue(liveSampler->MinFilter()), SCENE_NS::SamplerFilter::LINEAR)
        << "sibling sampler subfield must not be clobbered by sparse apply";
}

}  // namespace UTest
SCENE_END_NAMESPACE()
