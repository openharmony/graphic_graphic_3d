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
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_texture.h>
#include <scene_importer/interface/intf_importer.h>

#include <gmock/gmock.h>

#include <3d/ecs/components/material_component.h>

#include <meta/api/metadata_util.h>
#include <meta/api/util.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/resource/intf_resource_manager_extension.h>

#include "import_test_helpers.h"
#include "scene/scene_test.h"

using CORE3D_NS::MaterialComponent;

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ResourceOptionsTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        ScenePluginTest::SetUp();
        imp = META_NS::GetObjectRegistry().Create<SCENE_IMP_NS::IImporter>(SCENE_IMP_NS::ClassId::Importer);
        ASSERT_TRUE(imp);
        ASSERT_TRUE(imp->Initialize(context, {}));
    }

    CORE_NS::IResourceManager::Ptr ImportIndex(BASE_NS::string_view path, const IScene::Ptr& sc = nullptr)
    {
        SCENE_IMP_NS::ImportParameters params;
        params.scene = sc;
        auto res = imp->Import(path, params);
        EXPECT_TRUE(res);
        if (!res) {
            return nullptr;
        }
        return interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
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

// ---------------------------------------------------------------------------
// Clone
// ---------------------------------------------------------------------------

/**
 * @tc.name: ClonePreservesProperties
 * @tc.desc: Cloning resource options produces a copy with the same property values.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, ClonePreservesProperties, testing::ext::TestSize.Level1)
{
    auto rman = ImportIndex("test://import/index/index_ro_clone.json");
    ASSERT_TRUE(rman);

    auto extRes = interface_cast<META_NS::IResourceManagerExtension>(rman);
    ASSERT_TRUE(extRes);

    auto data = extRes->GetResource({{"clone-mat", ""}, nullptr});
    ASSERT_TRUE(data.id.IsValid());
    ASSERT_TRUE(data.options);

    auto cloned = data.options->Clone();
    ASSERT_TRUE(cloned);

    // Both original and clone should produce the same material when applied
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    // Re-import with scene so we can instantiate
    auto rman2 = ImportIndex("test://import/index/index_ro_clone.json", scene);
    ASSERT_TRUE(rman2);

    auto mat = interface_pointer_cast<IMaterial>(rman2->GetResource({CORE_NS::ResourceId("clone-mat"), scene}));
    ASSERT_TRUE(mat);
    EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.5f);
    EXPECT_EQ(META_NS::GetValue(mat->LightingFlags()), static_cast<LightingFlags>(2));
}

/**
 * @tc.name: CloneIsIndependent
 * @tc.desc: Modifying a cloned options object does not affect the original.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, CloneIsIndependent, testing::ext::TestSize.Level1)
{
    auto rman = ImportIndex("test://import/index/index_ro_clone.json");
    ASSERT_TRUE(rman);

    auto extRes = interface_cast<META_NS::IResourceManagerExtension>(rman);
    ASSERT_TRUE(extRes);

    auto data = extRes->GetResource({{"clone-mat", ""}, nullptr});
    ASSERT_TRUE(data.options);

    auto cloned = data.options->Clone();
    ASSERT_TRUE(cloned);

    // Modify the clone's base resource
    auto clonedDerived = interface_cast<META_NS::IDerivedResourceOptions>(cloned);
    ASSERT_TRUE(clonedDerived);
    clonedDerived->SetBaseResource(CORE_NS::ResourceId("some-other-base", ""));

    // Original should remain unchanged
    auto origDerived = interface_cast<META_NS::IDerivedResourceOptions>(data.options);
    ASSERT_TRUE(origDerived);
    EXPECT_FALSE(origDerived->GetBaseResource().IsValid());
}

/**
 * @tc.name: ClonePreservesDerivedFrom
 * @tc.desc: Cloning an options that has derivedFrom preserves the base resource ID.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, ClonePreservesDerivedFrom, testing::ext::TestSize.Level1)
{
    auto rman = ImportIndex("test://import/index/index_ro_derived_override.json");
    ASSERT_TRUE(rman);

    auto extRes = interface_cast<META_NS::IResourceManagerExtension>(rman);
    ASSERT_TRUE(extRes);

    auto data = extRes->GetResource({{"derived-override-mat", ""}, nullptr});
    ASSERT_TRUE(data.options);

    auto derived = interface_cast<META_NS::IDerivedResourceOptions>(data.options);
    ASSERT_TRUE(derived);
    EXPECT_EQ(derived->GetBaseResource(), CORE_NS::ResourceId("base-mat", ""));

    auto cloned = data.options->Clone();
    ASSERT_TRUE(cloned);

    auto clonedDerived = interface_cast<META_NS::IDerivedResourceOptions>(cloned);
    ASSERT_TRUE(clonedDerived);
    EXPECT_EQ(clonedDerived->GetBaseResource(), CORE_NS::ResourceId("base-mat", ""));
}

// ---------------------------------------------------------------------------
// DerivedFrom with property override
// ---------------------------------------------------------------------------

/**
 * @tc.name: DerivedFromWithPropertyOverride
 * @tc.desc: A derived resource inherits base properties and overrides specified ones.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, DerivedFromWithPropertyOverride, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto rman = ImportIndex("test://import/index/index_ro_derived_override.json", scene);
    ASSERT_TRUE(rman);

    auto mat =
        interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("derived-override-mat"), scene}));
    ASSERT_TRUE(mat);

    // Inherited from base (material0.json)
    EXPECT_EQ(META_NS::GetValue(mat->Type()), MaterialType::METALLIC_ROUGHNESS);
    EXPECT_EQ(META_NS::GetValue(mat->LightingFlags()), static_cast<LightingFlags>(2));

    // Overridden by derived options
    EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.75f);

    // Characterization of the derivedFrom apply (ApplyBaseResource -> baseTmpl->ApplyTo at
    // resource_template_base.cpp:673). Captures CURRENT behaviour: these values land as OVERRIDES,
    // not defaults. Pins this path so the upcoming standalone-ApplyTo defaults change cannot alter
    // the ApplyOptions/derivedFrom behaviour (the fix must route :673 through asDefault=false).
    // Must stay true.
    EXPECT_TRUE(mat->Type().GetProperty()->IsValueSet());
    EXPECT_TRUE(mat->LightingFlags().GetProperty()->IsValueSet());
    EXPECT_TRUE(mat->AlphaCutoff().GetProperty()->IsValueSet());
}

// ---------------------------------------------------------------------------
// No options
// ---------------------------------------------------------------------------

/**
 * @tc.name: IndexEntryWithoutOptions
 * @tc.desc: A resource index entry without options is registered without options data.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, IndexEntryWithoutOptions, testing::ext::TestSize.Level1)
{
    auto rman = ImportIndex("test://import/index/index_ro_no_options.json");
    ASSERT_TRUE(rman);

    auto extRes = interface_cast<META_NS::IResourceManagerExtension>(rman);
    ASSERT_TRUE(extRes);

    auto data = extRes->GetResource({{"no-opts-mat", ""}, nullptr});
    ASSERT_TRUE(data.id.IsValid());
    EXPECT_FALSE(data.options);
}

// ---------------------------------------------------------------------------
// Template context (SetTemplateContext / ApplyOptions interaction)
// ---------------------------------------------------------------------------

/**
 * @tc.name: TemplateContextAppliesValues
 * @tc.desc: When resource options are in template context, ApplyOptions applies the template
 *           values onto the GLTF-loaded material.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, TemplateContextAppliesValues, testing::ext::TestSize.Level1)
{
    constexpr auto MAT_GROUP = "mat-group";
    auto t1 = LoadTestTemplate("test://import/node_template/node_template_mat_v1.json");
    ASSERT_TRUE(t1);

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

    // Template context applies alphaCutoff=0.5 and lightingFlags=2
    EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.5f);
    EXPECT_EQ(META_NS::GetValue(mat->LightingFlags()), static_cast<LightingFlags>(2));
    EXPECT_EQ(META_NS::GetValue(mat->Type()), MaterialType::METALLIC_ROUGHNESS);
}

inline void CheckLightingFlags(const IMaterial::Ptr& mat, LightingFlags expectedSet)
{
    ASSERT_TRUE(mat);
    uint32_t expectedFlags = static_cast<uint32_t>(expectedSet);
    uint32_t flags = static_cast<uint32_t>(META_NS::GetValue(mat->LightingFlags()));
    EXPECT_TRUE(flags & expectedFlags) << flags << " " << expectedFlags;
}
/**
 * @tc.name: TemplateContextResetOnUpdate
 * @tc.desc: Updating from a template that overrides a property to one that doesn't
 *           resets the property back to the type default.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, TemplateContextResetOnUpdate, testing::ext::TestSize.Level1)
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

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);
    CORE_NS::ResourceId matId{"testMat", MAT_GROUP};

    {
        auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
        ASSERT_TRUE(mat);
        EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.5f);
        CheckLightingFlags(mat, LightingFlags::SHADOW_CASTER_BIT);
    }

    // Update to v2 which doesn't override alphaCutoff or lightingFlags
    n = interface_pointer_cast<INode>(t2->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);

    {
        auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
        ASSERT_TRUE(mat);
        // Reset to GLTF base defaults
        EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 1.f);
        CheckLightingFlags(mat, LightingFlags::SHADOW_RECEIVER_BIT);
    }
}

/**
 * @tc.name: TemplateUpdateViaResetThenNewValues
 * @tc.desc: Updating through a reset template (v2, no overrides) then to a template with
 *           new values (v3) correctly applies the new values.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, TemplateUpdateViaResetThenNewValues, testing::ext::TestSize.Level1)
{
    auto t1 = LoadTestTemplate("test://import/node_template/node_template_mat_v1.json");
    auto t2 = LoadTestTemplate("test://import/node_template/node_template_mat_v2.json");
    auto t3 = LoadTestTemplate("test://import/node_template/node_template_mat_v3.json");
    ASSERT_TRUE(t1);
    ASSERT_TRUE(t2);
    ASSERT_TRUE(t3);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    constexpr auto MAT_GROUP = "mat-group";
    auto n = root->ImportTemplate(t1, MAT_GROUP).GetResult();
    ASSERT_TRUE(n);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);
    CORE_NS::ResourceId matId{"testMat", MAT_GROUP};

    {
        auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
        ASSERT_TRUE(mat);
        EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.5f);
        EXPECT_EQ(META_NS::GetValue(mat->LightingFlags()), static_cast<LightingFlags>(2));
    }

    // Reset via v2 (no property overrides)
    n = interface_pointer_cast<INode>(t2->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);

    {
        auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
        ASSERT_TRUE(mat);
        EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 1.f);
    }

    // Apply v3 with new values
    n = interface_pointer_cast<INode>(t3->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);

    {
        auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
        ASSERT_TRUE(mat);
        EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.75f);
        EXPECT_EQ(META_NS::GetValue(mat->LightingFlags()), static_cast<LightingFlags>(4));
    }
}

/**
 * @tc.name: TemplateUpdateResetCycleV1V2V1
 * @tc.desc: Cycling v1->v2(reset)->v1 correctly restores original template values.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, TemplateUpdateResetCycleV1V2V1, testing::ext::TestSize.Level1)
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

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);
    CORE_NS::ResourceId matId{"testMat", MAT_GROUP};

    // v1: alphaCutoff=0.5, lightingFlags=2
    {
        auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
        ASSERT_TRUE(mat);
        EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.5f);
        CheckLightingFlags(mat, LightingFlags::SHADOW_CASTER_BIT);
    }

    // v2: no overrides -> reset to base defaults
    n = interface_pointer_cast<INode>(t2->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);
    {
        auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
        ASSERT_TRUE(mat);
        EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 1.f);
        CheckLightingFlags(mat, LightingFlags::SHADOW_RECEIVER_BIT);
    }

    // Back to v1: alphaCutoff=0.5, lightingFlags=2
    n = interface_pointer_cast<INode>(t1->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);
    {
        auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
        ASSERT_TRUE(mat);
        EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.5f);
        CheckLightingFlags(mat, LightingFlags::SHADOW_CASTER_BIT);
    }
}

/**
 * @tc.name: TemplateUpdateV1V2V3Cycle
 * @tc.desc: Full cycle v1->v2(reset)->v3(new values) verifies reset then re-override.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, TemplateUpdateV1V2V3Cycle, testing::ext::TestSize.Level1)
{
    auto t1 = LoadTestTemplate("test://import/node_template/node_template_mat_v1.json");
    auto t2 = LoadTestTemplate("test://import/node_template/node_template_mat_v2.json");
    auto t3 = LoadTestTemplate("test://import/node_template/node_template_mat_v3.json");
    ASSERT_TRUE(t1);
    ASSERT_TRUE(t2);
    ASSERT_TRUE(t3);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto root = interface_pointer_cast<INodeImport>(scene->GetRootNode().GetResult());
    ASSERT_TRUE(root);

    constexpr auto MAT_GROUP = "mat-group";
    auto n = root->ImportTemplate(t1, MAT_GROUP).GetResult();
    ASSERT_TRUE(n);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);
    CORE_NS::ResourceId matId{"testMat", MAT_GROUP};

    // v1: alphaCutoff=0.5
    {
        auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
        ASSERT_TRUE(mat);
        EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.5f);
    }

    // v2: no alphaCutoff -> reset to base default
    n = interface_pointer_cast<INode>(t2->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);
    {
        auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
        ASSERT_TRUE(mat);
        EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 1.f);
    }

    // v3: alphaCutoff=0.75 -> re-override with new value
    n = interface_pointer_cast<INode>(t3->Update(*interface_cast<META_NS::IObject>(n), nullptr));
    ASSERT_TRUE(n);
    {
        auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({matId, scene}));
        ASSERT_TRUE(mat);
        EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.75f);
    }
}

// ---------------------------------------------------------------------------
// Merge (via template update with resources)
// ---------------------------------------------------------------------------

/**
 * @tc.name: MergeBaseResourceOverride
 * @tc.desc: When merging resource options, derivedFrom from the incoming options takes precedence.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, MergeBaseResourceOverride, testing::ext::TestSize.Level1)
{
    // Import two indices: one without derivedFrom, one with derivedFrom
    auto rman1 = ImportIndex("test://import/index/index_ro_clone.json");
    ASSERT_TRUE(rman1);
    auto rman2 = ImportIndex("test://import/index/index_ro_derived_override.json");
    ASSERT_TRUE(rman2);

    auto extRes1 = interface_cast<META_NS::IResourceManagerExtension>(rman1);
    auto extRes2 = interface_cast<META_NS::IResourceManagerExtension>(rman2);
    ASSERT_TRUE(extRes1);
    ASSERT_TRUE(extRes2);

    auto data1 = extRes1->GetResource({{"clone-mat", ""}, nullptr});
    auto data2 = extRes2->GetResource({{"derived-override-mat", ""}, nullptr});
    ASSERT_TRUE(data1.options);
    ASSERT_TRUE(data2.options);

    // Merge data2 (has derivedFrom) into data1 (no derivedFrom)
    auto result = data1.options->Merge(*data2.options);
    EXPECT_TRUE(result);

    // After merge, data1 should have the base resource from data2
    auto derived = interface_cast<META_NS::IDerivedResourceOptions>(data1.options);
    ASSERT_TRUE(derived);
    EXPECT_EQ(derived->GetBaseResource(), CORE_NS::ResourceId("base-mat", ""));
}

// ---------------------------------------------------------------------------
// Multiple instantiations from same options
// ---------------------------------------------------------------------------

/**
 * @tc.name: MultipleInstantiationsFromSameIndex
 * @tc.desc: The same index resource can be instantiated into multiple scenes independently.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, MultipleInstantiationsFromSameIndex, testing::ext::TestSize.Level1)
{
    // Import the same template into two separate scenes
    constexpr auto MAT_GROUP = "mat-group";
    auto t1 = LoadTestTemplate("test://import/node_template/node_template_mat_v1.json");
    ASSERT_TRUE(t1);

    auto scene1 = CreateEmptyScene();
    ASSERT_TRUE(scene1);
    auto root1 = interface_pointer_cast<INodeImport>(scene1->GetRootNode().GetResult());
    ASSERT_TRUE(root1);
    auto n1 = root1->ImportTemplate(t1, MAT_GROUP).GetResult();
    ASSERT_TRUE(n1);

    auto scene2 = CreateEmptyScene();
    ASSERT_TRUE(scene2);
    auto root2 = interface_pointer_cast<INodeImport>(scene2->GetRootNode().GetResult());
    ASSERT_TRUE(root2);
    auto n2 = root2->ImportTemplate(t1, MAT_GROUP).GetResult();
    ASSERT_TRUE(n2);

    // Both scenes should have independent materials with the same values
    auto rman1 = GetResourceManager(scene1);
    auto rman2 = GetResourceManager(scene2);
    ASSERT_TRUE(rman1);
    ASSERT_TRUE(rman2);

    CORE_NS::ResourceId matId{"testMat", MAT_GROUP};
    auto mat1 = interface_pointer_cast<IMaterial>(rman1->GetResource({matId, scene1}));
    auto mat2 = interface_pointer_cast<IMaterial>(rman2->GetResource({matId, scene2}));
    ASSERT_TRUE(mat1);
    ASSERT_TRUE(mat2);

    EXPECT_FLOAT_EQ(META_NS::GetValue(mat1->AlphaCutoff()), 0.5f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(mat2->AlphaCutoff()), 0.5f);
}

// ---------------------------------------------------------------------------
// Derived material with readonly sub-object inheritance
// ---------------------------------------------------------------------------

/**
 * @tc.name: DerivedFromInheritsMaterialType
 * @tc.desc: A bare derived resource (no inline overrides) inherits all base properties.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, DerivedFromInheritsMaterialType, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    // index3.json: base-mat (materialTemplate from material0.json) + derived-mat (derivedFrom only)
    auto rman = ImportIndex("test://import/index/index3.json", scene);
    ASSERT_TRUE(rman);

    auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("derived-mat"), scene}));
    ASSERT_TRUE(mat);

    // All values from base
    EXPECT_EQ(META_NS::GetValue(mat->Type()), MaterialType::METALLIC_ROUGHNESS);
    EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.5f);
    EXPECT_EQ(META_NS::GetValue(mat->LightingFlags()), static_cast<LightingFlags>(2));

    auto renderSort = META_NS::GetValue(mat->RenderSort());
    EXPECT_EQ(renderSort.renderSortLayer, 3);
    EXPECT_EQ(renderSort.renderSortLayerOrder, 1);
}

// ---------------------------------------------------------------------------
// DerivedFrom edge cases
// ---------------------------------------------------------------------------

/**
 * @tc.name: DerivedFromChainThreeLevels
 * @tc.desc: A 3-level derivedFrom chain (leaf -> mid -> base) inherits transitively: each
 *           layer's resolved values become the next layer's defaults. Leaf has no overrides
 *           of its own, so it sees mid's alphaCutoff (which itself overrides base's value).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, DerivedFromChainThreeLevels, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto rman = ImportIndex("test://import/index/index_derived_chain.json", scene);
    ASSERT_TRUE(rman);

    auto base = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("mat-base"), scene}));
    ASSERT_TRUE(base);
    EXPECT_FLOAT_EQ(META_NS::GetValue(base->AlphaCutoff()), 0.1f);
    EXPECT_EQ(META_NS::GetValue(base->Type()), MaterialType::METALLIC_ROUGHNESS);

    auto mid = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("mat-mid"), scene}));
    ASSERT_TRUE(mid);
    EXPECT_FLOAT_EQ(META_NS::GetValue(mid->AlphaCutoff()), 0.5f);
    EXPECT_EQ(META_NS::GetValue(mid->Type()), MaterialType::METALLIC_ROUGHNESS);

    // Leaf has no inline overrides; transitive inheritance brings in mid's alphaCutoff=0.5
    // and base's metallicRoughness type.
    auto leaf = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("mat-leaf"), scene}));
    ASSERT_TRUE(leaf);
    EXPECT_FLOAT_EQ(META_NS::GetValue(leaf->AlphaCutoff()), 0.5f);
    EXPECT_EQ(META_NS::GetValue(leaf->Type()), MaterialType::METALLIC_ROUGHNESS);
}

/**
 * @tc.name: ImportMaterialCustomShaderSlotName
 * @tc.desc: Custom.shader declares slot 0 as "Albedo" (not the standard "baseColor"). A
 *           JSON-imported material with a `textures` map containing both "Albedo" (the
 *           shader-declared name) and "baseColor" (the standard name the shader has
 *           redeclared) should:
 *             - route the "Albedo" entry's factor onto slot 0,
 *             - skip "baseColor" with a warning (the bound shader has no such slot).
 *           Exercises apply-time resolution against shader-declared slot names through
 *           the full JSON import pipeline.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, ImportMaterialCustomShaderSlotName, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto rman = ImportIndex("test://import/index/index_mat_custom_shader_slot.json", scene);
    ASSERT_TRUE(rman);

    auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("mat-custom-slot"), scene}));
    ASSERT_TRUE(mat);

    auto textures = mat->Textures();
    ASSERT_TRUE(textures);
    auto slot0 = textures->GetValueAt(0);
    ASSERT_TRUE(slot0);

    // "Albedo" — the shader's declared name for slot 0 — landed on slot 0.
    EXPECT_EQ(META_NS::GetValue(slot0->Factor()), BASE_NS::Math::Vec4(0.11f, 0.22f, 0.33f, 0.44f));

    // "baseColor" was *not* applied — Custom.shader has no such slot, so the entry was
    // skipped with a warning. Slot 0's factor must be the Albedo value, not baseColor's.
    EXPECT_NE(META_NS::GetValue(slot0->Factor()), BASE_NS::Math::Vec4(0.99f, 0.99f, 0.99f, 0.99f));
}

/**
 * @tc.name: DerivedFromMaterialTexturesByName
 * @tc.desc: Two-level material derivedFrom chain with named textures: base sets baseColor +
 *           emissive, leaf overrides baseColor and adds normal. Live leaf material should
 *           see leaf's overrides on baseColor and normal, base's emissive carried through
 *           as a default, and other slots untouched. Exercises ApplyBaseResource's
 *           applyBaseDefaults callback path in MaterialTemplate::ApplyOptions.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, DerivedFromMaterialTexturesByName, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto rman = ImportIndex("test://import/index/index_mat_derived_textures.json", scene);
    ASSERT_TRUE(rman);

    auto base = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("mat-base-tex"), scene}));
    ASSERT_TRUE(base);
    auto leaf = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("mat-leaf-tex"), scene}));
    ASSERT_TRUE(leaf);

    auto baseTextures = base->Textures();
    auto leafTextures = leaf->Textures();
    ASSERT_TRUE(baseTextures);
    ASSERT_TRUE(leafTextures);

    // Base: baseColor red, emissive grey.
    auto baseBaseColor = baseTextures->GetValueAt(MaterialComponent::TextureIndex::BASE_COLOR);
    auto baseEmissive = baseTextures->GetValueAt(MaterialComponent::TextureIndex::EMISSIVE);
    ASSERT_TRUE(baseBaseColor);
    ASSERT_TRUE(baseEmissive);
    EXPECT_EQ(META_NS::GetValue(baseBaseColor->Factor()), BASE_NS::Math::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    EXPECT_FLOAT_EQ(META_NS::GetValue(baseBaseColor->Rotation()), 0.25f);
    EXPECT_EQ(META_NS::GetValue(baseEmissive->Factor()), BASE_NS::Math::Vec4(0.5f, 0.5f, 0.5f, 1.0f));

    // Leaf: leaf's baseColor override (green) wins; base's emissive carries through; leaf
    // adds a normal override; rotation on baseColor came from base (leaf didn't set it).
    auto leafBaseColor = leafTextures->GetValueAt(MaterialComponent::TextureIndex::BASE_COLOR);
    auto leafNormal = leafTextures->GetValueAt(MaterialComponent::TextureIndex::NORMAL);
    auto leafEmissive = leafTextures->GetValueAt(MaterialComponent::TextureIndex::EMISSIVE);
    ASSERT_TRUE(leafBaseColor);
    ASSERT_TRUE(leafNormal);
    ASSERT_TRUE(leafEmissive);
    EXPECT_EQ(META_NS::GetValue(leafBaseColor->Factor()), BASE_NS::Math::Vec4(0.0f, 1.0f, 0.0f, 1.0f))
        << "leaf's baseColor override should win";
    EXPECT_FLOAT_EQ(META_NS::GetValue(leafBaseColor->Rotation()), 0.25f)
        << "rotation only set on base; should carry through as default";
    EXPECT_EQ(META_NS::GetValue(leafNormal->Factor()), BASE_NS::Math::Vec4(0.0f, 0.0f, 1.0f, 1.0f))
        << "leaf's own normal override";
    EXPECT_EQ(META_NS::GetValue(leafEmissive->Factor()), BASE_NS::Math::Vec4(0.5f, 0.5f, 0.5f, 1.0f))
        << "base's emissive should carry through as default";
}

/**
 * @tc.name: DerivedFromCrossTypeImports
 * @tc.desc: A material with derivedFrom pointing at an image (different resource type) imports
 *           successfully — derivedFrom resolution is by-id only, not type-checked at import.
 *           This documents current behavior so a future regression that changes it is visible.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, DerivedFromCrossTypeImports, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    // Even though "bad-mat" derives from an image, the index entries themselves register fine.
    // Whether the resource resolves cleanly when fetched is a separate concern handled by
    // the resource manager.
    auto rman = ImportIndex("test://import/index/index_derived_cross_type.json", scene);
    ASSERT_TRUE(rman);
    EXPECT_TRUE(rman->GetResourceInfo({"some-image", scene}).id.IsValid());
    EXPECT_TRUE(rman->GetResourceInfo({"bad-mat", scene}).id.IsValid());
}

/**
 * @tc.name: DerivedFromCircularImports
 * @tc.desc: Circular derivedFrom (mat-a -> mat-b -> mat-a) imports without an importer-level
 *           error — circularity is not detected during index import. This documents current
 *           behavior; if the importer adds cycle detection later, this test will fail and
 *           should be replaced with an explicit error-path test.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, DerivedFromCircularImports, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto rman = ImportIndex("test://import/index/index_derived_circular.json", scene);
    ASSERT_TRUE(rman);
    EXPECT_TRUE(rman->GetResourceInfo({"mat-a", scene}).id.IsValid());
    EXPECT_TRUE(rman->GetResourceInfo({"mat-b", scene}).id.IsValid());
}

/**
 * @tc.name: DuplicateResourceIdImport
 * @tc.desc: Two index entries sharing the same resourceId in the same group import without
 *           an error today — there is no duplicate-id detection in the index importer.
 *           This test documents that behavior. If duplicate detection is added later, this
 *           test should flip to expect a "Failed to add resource" diagnostic.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceOptionsTest, DuplicateResourceIdImport, testing::ext::TestSize.Level1)
{
    SCENE_IMP_NS::ImportParameters params;
    params.scene = CreateEmptyScene();
    auto res = imp->Import("test://import/index/index_duplicate_id.json", params);
    EXPECT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);
    EXPECT_TRUE(rman->GetResourceInfo({"dup", params.scene}).id.IsValid());
}

}  // namespace UTest
SCENE_END_NAMESPACE()
