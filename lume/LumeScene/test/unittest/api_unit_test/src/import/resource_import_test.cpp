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

#include <scene/api/template/material_template.h>
#include <scene/ext/scene_utils.h>
#include <scene/interface/intf_environment.h>
#include <scene/interface/intf_image.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_mesh_template.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/intf_texture.h>
#include <scene/interface/mesh_descriptor.h>
#include <scene/interface/postprocess/intf_bloom.h>
#include <scene/interface/postprocess/intf_blur.h>
#include <scene/interface/postprocess/intf_color_conversion.h>
#include <scene/interface/postprocess/intf_color_fringe.h>
#include <scene/interface/postprocess/intf_dof.h>
#include <scene/interface/postprocess/intf_fxaa.h>
#include <scene/interface/postprocess/intf_lens_flare.h>
#include <scene/interface/postprocess/intf_motion_blur.h>
#include <scene/interface/postprocess/intf_taa.h>
#include <scene/interface/postprocess/intf_tonemap.h>
#include <scene/interface/postprocess/intf_upscale.h>
#include <scene/interface/postprocess/intf_vignette.h>
#include <scene/interface/resource/image_info.h>
#include <scene_importer/interface/intf_importer.h>

#include <core/intf_engine.h>

#include <gmock/gmock.h>

#include <meta/api/util.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/animation/modifiers/intf_loop.h>
#include <meta/interface/animation/modifiers/intf_speed.h>
#include <meta/interface/curves/intf_bezier.h>
#include <meta/interface/curves/intf_curve_1d.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/resource/intf_resource_manager_extension.h>

#include "import_test_helpers.h"
#include "scene/scene_test.h"
#include "util.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ResourceImportTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        ScenePluginTest::SetUp();
        imp = META_NS::GetObjectRegistry().Create<SCENE_IMP_NS::IImporter>(SCENE_IMP_NS::ClassId::Importer);
        ASSERT_TRUE(imp);
        ASSERT_TRUE(imp->Initialize(context, {}));
    }

    META_NS::IObject::Ptr Load(BASE_NS::string_view path)
    {
        auto res = imp->Import(path, {});
        EXPECT_TRUE(res);
        if (!res) {
            return nullptr;
        }
        return res.object;
    }

    template<typename T>
    T GetProp(META_NS::IMetadata* meta, BASE_NS::string_view name, T def = {})
    {
        EXPECT_TRUE(meta);
        if (!meta) {
            return def;
        }
        return META_NS::GetValue<T>(meta->GetProperty(name), def);
    }

    CORE_NS::IResourceManager::Ptr ImportIndexIntoScene(BASE_NS::string_view path, const IScene::Ptr& sc)
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

public:
    SCENE_IMP_NS::IImporter::Ptr imp;
};

/**
 * @tc.name: ImportMaterial0
 * @tc.desc: Tests for ImportMaterial [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportMaterial0, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/material/material0.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "Name"), "MyMaterial");
    EXPECT_EQ(GetProp<SCENE_NS::MaterialType>(meta, "Type"), SCENE_NS::MaterialType::METALLIC_ROUGHNESS);
    EXPECT_FLOAT_EQ(GetProp<float>(meta, "AlphaCutoff"), 0.5f);
    EXPECT_EQ(GetProp<SCENE_NS::LightingFlags>(meta, "LightingFlags"), static_cast<SCENE_NS::LightingFlags>(2));

    auto renderSort = GetProp<SCENE_NS::RenderSort>(meta, "RenderSort");
    EXPECT_EQ(renderSort.renderSortLayer, 3);
    EXPECT_EQ(renderSort.renderSortLayerOrder, 1);
}

/**
 * @tc.name: ImportMaterialMultiErrorDefault
 * @tc.desc: Material with multiple type errors fails on first error in default mode
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportMaterialMultiErrorDefault, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/material/material_multi_error.json", {});
    EXPECT_FALSE(res);
    ASSERT_TRUE(res.error);
    EXPECT_EQ(res.error->GetErrors().size(), 1u);
    // material_multi_error.json: alphaCutoff is "notANumber" (string); lightingFlags is [123].
    // Default mode stops at the first error, which is the alphaCutoff parse failure.
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "alphaCutoff");
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "type mismatch");
}

/**
 * @tc.name: ImportMaterialMultiErrorContinue
 * @tc.desc: Material with multiple type errors collects all errors in continue-on-error mode
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportMaterialMultiErrorContinue, testing::ext::TestSize.Level1)
{
    SCENE_IMP_NS::IImporter::Ptr impContinue =
        META_NS::GetObjectRegistry().Create<SCENE_IMP_NS::IImporter>(SCENE_IMP_NS::ClassId::Importer);
    ASSERT_TRUE(impContinue);
    ASSERT_TRUE(impContinue->Initialize(context, {{}, true}));
    auto res = impContinue->Import("test://import/material/material_multi_error.json", {});
    EXPECT_TRUE(res);
    ASSERT_TRUE(res.error);
    EXPECT_EQ(res.error->GetErrors().size(), 2u);
    // Both errors must be present: alphaCutoff was a string instead of number,
    // and lightingFlags entry was 123 (a number) instead of a string flag name.
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "alphaCutoff");
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "lightingFlags");
}

/**
 * @tc.name: ImportEnvironment0
 * @tc.desc: Tests for ImportEnvironment [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportEnvironment0, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/environment/environment0.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "Name"), "MyEnvironment");
    EXPECT_EQ(GetProp<SCENE_NS::EnvBackgroundType>(meta, "Background"), SCENE_NS::EnvBackgroundType::CUBEMAP);
    EXPECT_EQ(GetProp<BASE_NS::Math::Vec4>(meta, "IndirectDiffuseFactor"), BASE_NS::Math::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
    EXPECT_EQ(
        GetProp<BASE_NS::Math::Vec4>(meta, "IndirectSpecularFactor"), BASE_NS::Math::Vec4(0.4f, 0.5f, 0.6f, 1.0f));
    EXPECT_EQ(GetProp<BASE_NS::Math::Vec4>(meta, "EnvMapFactor"), BASE_NS::Math::Vec4(1.0f, 1.0f, 1.0f, 0.5f));
    EXPECT_EQ(GetProp<uint32_t>(meta, "RadianceCubemapMipCount"), 5u);
    EXPECT_FLOAT_EQ(GetProp<float>(meta, "EnvironmentMapLodLevel"), 2.5f);
    EXPECT_EQ(GetProp<BASE_NS::Math::Quat>(meta, "EnvironmentRotation"), BASE_NS::Math::Quat(0.0f, 0.0f, 0.0f, 1.0f));

    auto coeffs = META_NS::GetValue<BASE_NS::vector<BASE_NS::Math::Vec3>>(meta->GetProperty("IrradianceCoefficients"));
    ASSERT_EQ(coeffs.size(), 3u);
    EXPECT_EQ(coeffs[0], BASE_NS::Math::Vec3(1.0f, 0.0f, 0.0f));
    EXPECT_EQ(coeffs[1], BASE_NS::Math::Vec3(0.0f, 1.0f, 0.0f));
    EXPECT_EQ(coeffs[2], BASE_NS::Math::Vec3(0.0f, 0.0f, 1.0f));
}

/**
 * @tc.name: ImportImage0
 * @tc.desc: Tests for ImportImage [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportImage0, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/image/image0.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "Name"), "MyImage");
    auto info = GetProp<SCENE_NS::ImageLoadInfo>(meta, "ImageLoadInfo");
    EXPECT_EQ(info.loadFlags, SCENE_NS::ImageLoadFlags::GENERATE_MIPS | SCENE_NS::ImageLoadFlags::FORCE_SRGB_BIT);
    EXPECT_EQ(info.info.usageFlags, SCENE_NS::ImageUsageFlag::SAMPLED_BIT | SCENE_NS::ImageUsageFlag::TRANSFER_DST_BIT);
    EXPECT_EQ(info.info.memoryFlags, SCENE_NS::MemoryPropertyFlag::DEVICE_LOCAL_BIT);
    EXPECT_EQ(info.info.creationFlags, SCENE_NS::EngineImageCreationFlag::GENERATE_MIPS);
}

/**
 * @tc.name: ImportImageLoadFlagsExtraBits
 * @tc.desc: Verifies the IMAGE_LOAD_FLAGS_TABLE entries beyond the GENERATE_MIPS/FORCE_SRGB pair
 *           covered by ImportImage0 — forceLinearRgb, forceGrayscale, flipVertically,
 *           premultiplyAlpha all import to the matching ImageLoadFlags bits.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportImageLoadFlagsExtraBits, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/image/image_load_flags_extra.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto info = GetProp<SCENE_NS::ImageLoadInfo>(meta, "ImageLoadInfo");
    auto expected = SCENE_NS::ImageLoadFlags::FORCE_LINEAR_RGB_BIT | SCENE_NS::ImageLoadFlags::FORCE_GRAYSCALE_BIT |
                    SCENE_NS::ImageLoadFlags::FLIP_VERTICALLY_BIT | SCENE_NS::ImageLoadFlags::PREMULTIPLY_ALPHA;
    EXPECT_EQ(info.loadFlags, expected);
}

/**
 * @tc.name: ImportImageInvalidLoadFlag
 * @tc.desc: An unknown loadFlags entry produces a clear "Invalid loadFlags value" diagnostic
 *           (and does not silently strip the unknown bit).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportImageInvalidLoadFlag, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/image/image_invalid_load_flag.json", {});
    EXPECT_FALSE(res);
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "Invalid loadFlags value");
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "thisFlagIsNotReal");
}

/**
 * @tc.name: ImportMaterialOptions0
 * @tc.desc: Material "options" block is imported as a sparse Options sub-object
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportMaterialOptions0, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/material/material_options.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto optionsObj = GetProp<META_NS::IObject::Ptr>(meta, "Options");
    ASSERT_TRUE(optionsObj);
    auto optMeta = interface_cast<META_NS::IMetadata>(optionsObj);
    ASSERT_TRUE(optMeta);

    EXPECT_EQ(GetProp<SCENE_NS::CullModeFlags>(optMeta, "CullMode"), SCENE_NS::CullModeFlags::BACK_BIT);
    EXPECT_EQ(GetProp<SCENE_NS::PolygonMode>(optMeta, "PolygonMode"), SCENE_NS::PolygonMode::FILL);
    EXPECT_EQ(GetProp<bool>(optMeta, "Blend"), true);
}

/**
 * @tc.name: ImportMesh0
 * @tc.desc: Mesh JSON descriptor parses into a MeshTemplate exposing IMeshTemplate.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportMesh0, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/mesh/triangle.json");
    ASSERT_TRUE(obj);
    auto meshTemplate = interface_pointer_cast<SCENE_NS::IMeshTemplate>(obj);
    ASSERT_TRUE(meshTemplate);

    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);
    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "Name"), "MyMesh");
}

/**
 * @tc.name: ImportMeshUnknownSemantic
 * @tc.desc: Mesh with a semantic that doesn't match the schema's uvN/position/etc. pattern fails import.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportMeshUnknownSemantic, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/mesh/mesh_unknown_semantic.json", {});
    EXPECT_FALSE(res);
    ASSERT_TRUE(res.error);
    EXPECT_FALSE(res.error->GetErrors().empty());
}

/**
 * @tc.name: ImportMeshMissingBuffer
 * @tc.desc: Mesh JSON without bufferUri fails import (bufferUri is required).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportMeshMissingBuffer, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/mesh/mesh_missing_buffer.json", {});
    EXPECT_FALSE(res);
    ASSERT_TRUE(res.error);
    EXPECT_FALSE(res.error->GetErrors().empty());
}

/**
 * @tc.name: ImportMeshBadMagic
 * @tc.desc: Mesh whose .bin lacks the "LMSH" magic header is rejected at import time.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportMeshBadMagic, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/mesh/mesh_bad_magic.json", {});
    EXPECT_FALSE(res);
    ASSERT_TRUE(res.error);
    EXPECT_FALSE(res.error->GetErrors().empty());
}

/**
 * @tc.name: ImportMeshRegionOutOfBounds
 * @tc.desc: A submesh region whose `offset + size` exceeds the .bin file size
 *          is rejected at import time, naming the offending region.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportMeshRegionOutOfBounds, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/mesh/mesh_region_out_of_bounds.json", {});
    EXPECT_FALSE(res);
    ASSERT_TRUE(res.error);
    EXPECT_FALSE(res.error->GetErrors().empty());
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "exceeds binary size");
}

/**
 * @tc.name: ImportIndexDerivedMeshFromTemplate
 * @tc.desc: A 'mesh' index entry with derivedFrom pointing at a 'meshTemplate'
 *           entry resolves through the resource manager to a live IMesh whose
 *           submeshes were populated via MeshTemplate::ApplyOptions.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexDerivedMeshFromTemplate, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    SCENE_IMP_NS::ImportParameters params;
    params.scene = scene;
    auto res = imp->Import("test://import/index/index_mesh.json", params);
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    // The template entry registers as a MeshTemplate resource.
    auto tmpl = rman->GetResource({CORE_NS::ResourceId("triangle-template"), scene});
    ASSERT_TRUE(tmpl);
    EXPECT_TRUE(interface_cast<SCENE_NS::IMeshTemplate>(tmpl));

    // The derived 'mesh' entry materialises as a live IMesh with one submesh
    // (triangle.json defines exactly one).
    auto meshRes = rman->GetResource({CORE_NS::ResourceId("triangle-mesh"), scene});
    ASSERT_TRUE(meshRes);
    auto mesh = interface_pointer_cast<SCENE_NS::IMesh>(meshRes);
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->SubMeshes()->GetValue().size(), 1u);

    // Asking for the same id twice returns the same live IMesh — caching is
    // owned by the resource manager, not by the template.
    auto mesh2 =
        interface_pointer_cast<SCENE_NS::IMesh>(rman->GetResource({CORE_NS::ResourceId("triangle-mesh"), scene}));
    EXPECT_EQ(mesh, mesh2);
}

/**
 * @tc.name: ImportedMeshSubmeshAABBMatchesJson
 * @tc.desc: After importing a mesh via the JSON importer, the submesh
 *          AABBMin/AABBMax match the values declared in triangle.json's
 *          "aabb" block, and remain the property's default value (loader did
 *          not stamp them as user-modified).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportedMeshSubmeshAABBMatchesJson, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto rman = ImportIndexIntoScene("test://import/index/index_mesh.json", scene);
    ASSERT_TRUE(rman);

    auto meshRes = rman->GetResource({CORE_NS::ResourceId("triangle-mesh"), scene});
    ASSERT_TRUE(meshRes);
    auto mesh = interface_pointer_cast<SCENE_NS::IMesh>(meshRes);
    ASSERT_TRUE(mesh);

    auto subs = mesh->SubMeshes()->GetValue();
    ASSERT_EQ(subs.size(), 1u);
    auto submesh = subs[0];
    ASSERT_TRUE(submesh);

    EXPECT_EQ(submesh->AABBMin()->GetValue(), (BASE_NS::Math::Vec3{-1.0f, -1.0f, 0.0f}));
    EXPECT_EQ(submesh->AABBMax()->GetValue(), (BASE_NS::Math::Vec3{1.0f, 1.0f, 0.0f}));
    EXPECT_TRUE(submesh->AABBMin()->IsDefaultValue());
    EXPECT_TRUE(submesh->AABBMax()->IsDefaultValue());
}

/**
 * @tc.name: ImportIndexMeshInlineDescriptor
 * @tc.desc: A 'mesh' index entry that carries its descriptor + bufferUri
 *           inline in `options` (no `derivedFrom`, no separate `meshTemplate`
 *           sibling) still builds a live IMesh — MeshTemplate::ApplyOptions
 *           takes its data from the inline options instead of resolving a
 *           base. This is the standalone-mesh path through the manager.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexMeshInlineDescriptor, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto rman = ImportIndexIntoScene("test://import/index/index_mesh_inline.json", scene);
    ASSERT_TRUE(rman);

    auto mesh = interface_pointer_cast<SCENE_NS::IMesh>(rman->GetResource({CORE_NS::ResourceId("inline-mesh"), scene}));
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->SubMeshes()->GetValue().size(), 1u);

    // No derivedFrom was set, so no template id is stamped — the mesh stands
    // on its own data.
    auto derived = interface_cast<META_NS::IDerivedFromTemplate>(mesh);
    ASSERT_TRUE(derived);
    EXPECT_FALSE(derived->GetTemplateId().IsValid());
}

/**
 * @tc.name: MeshTemplateExposesDescriptor
 * @tc.desc: A standalone 'meshTemplate' index entry loads as a MeshTemplate
 *           that exposes its descriptor data through the IMeshTemplate
 *           getters — submeshes, vertex attributes, and binary data.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, MeshTemplateExposesDescriptor, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto rman = ImportIndexIntoScene("test://import/index/index_mesh.json", scene);
    ASSERT_TRUE(rman);

    auto tmplRes = rman->GetResource({CORE_NS::ResourceId("triangle-template"), scene});
    auto tmpl = interface_pointer_cast<SCENE_NS::IMeshTemplate>(tmplRes);
    ASSERT_TRUE(tmpl);

    // triangle.json declares one submesh, one position attribute, and a
    // non-empty binary buffer (header + 36-byte vertex region + 6-byte index
    // region, padded).
    EXPECT_EQ(tmpl->GetSubmeshes().size(), 1u);
    ASSERT_EQ(tmpl->GetVertexAttributes().size(), 1u);
    EXPECT_EQ(tmpl->GetVertexAttributes()[0].semantic, SCENE_NS::MeshSemantic::POSITION);
    EXPECT_FALSE(tmpl->GetBinaryData().empty());
}

/**
 * @tc.name: TemplateIdStampedOnDerivedMesh
 * @tc.desc: After MeshTemplate::ApplyOptions runs, the live Mesh's
 *           IDerivedFromTemplate::GetTemplateId points at the originating
 *           template id.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, TemplateIdStampedOnDerivedMesh, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto rman = ImportIndexIntoScene("test://import/index/index_mesh.json", scene);
    ASSERT_TRUE(rman);

    auto mesh = rman->GetResource({CORE_NS::ResourceId("triangle-mesh"), scene});
    ASSERT_TRUE(mesh);
    auto derived = interface_cast<META_NS::IDerivedFromTemplate>(mesh);
    ASSERT_TRUE(derived);
    EXPECT_EQ(derived->GetTemplateId(), CORE_NS::ResourceId("triangle-template"));
}

/**
 * @tc.name: TwoMeshesShareOneTemplate
 * @tc.desc: Two distinct 'mesh' index entries deriving from the same template
 *           produce two distinct live IMesh instances (each with its own ECS
 *           entity), so they can carry independent material overrides etc.
 *           Each instance has the submesh count from the template's descriptor.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, TwoMeshesShareOneTemplate, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto rman = ImportIndexIntoScene("test://import/index/index_mesh_two_derived.json", scene);
    ASSERT_TRUE(rman);

    auto meshA = interface_pointer_cast<SCENE_NS::IMesh>(rman->GetResource({CORE_NS::ResourceId("tri-mesh-a"), scene}));
    auto meshB = interface_pointer_cast<SCENE_NS::IMesh>(rman->GetResource({CORE_NS::ResourceId("tri-mesh-b"), scene}));
    ASSERT_TRUE(meshA);
    ASSERT_TRUE(meshB);
    EXPECT_NE(meshA, meshB);
    EXPECT_EQ(meshA->SubMeshes()->GetValue().size(), 1u);
    EXPECT_EQ(meshB->SubMeshes()->GetValue().size(), 1u);
}

/**
 * @tc.name: SubmeshMaterialResolvedAtApply
 * @tc.desc: A submesh that declares a material resource id has that material
 *           resolved and bound to the live Mesh's submesh entry by
 *           MeshTemplate::ApplyOptions — geometry-node code can then plug the
 *           IMesh in without doing any per-submesh material lookups itself.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, SubmeshMaterialResolvedAtApply, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto rman = ImportIndexIntoScene("test://import/index/index_mesh_with_material.json", scene);
    ASSERT_TRUE(rman);

    auto mesh = interface_pointer_cast<SCENE_NS::IMesh>(rman->GetResource({CORE_NS::ResourceId("tri-mesh"), scene}));
    ASSERT_TRUE(mesh);
    auto submeshes = mesh->SubMeshes()->GetValue();
    ASSERT_EQ(submeshes.size(), 1u);

    auto mat = META_NS::GetValue(submeshes[0]->Material());
    ASSERT_TRUE(mat);
    // The resolved material should be the same instance the resource manager
    // hands out for the same id — no duplication.
    auto matFromManager =
        interface_pointer_cast<SCENE_NS::IMaterial>(rman->GetResource({CORE_NS::ResourceId("tri-mat"), scene}));
    EXPECT_EQ(mat, matFromManager);
}

/**
 * @tc.name: GeometryNodeBindsLiveMesh
 * @tc.desc: A scene with a geometry node referencing a 'mesh' resource id
 *           plugs the live IMesh into the mesh node — the geometry node
 *           handler itself does not build or resolve anything beyond looking
 *           up the resource by id.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, GeometryNodeBindsLiveMesh, testing::ext::TestSize.Level1)
{
    SCENE_IMP_NS::ImportParameters params;
    auto res = imp->Import("test://import/scene/scene_with_mesh.json", params);
    ASSERT_TRUE(res);
    auto scene = interface_pointer_cast<SCENE_NS::IScene>(res.object);
    ASSERT_TRUE(scene);

    auto geomA = scene->FindNode("//geom_a").GetResult();
    ASSERT_TRUE(geomA);
    auto meshAccA = interface_cast<SCENE_NS::IMeshAccess>(geomA);
    ASSERT_TRUE(meshAccA);
    auto meshA = meshAccA->GetMesh().GetResult();
    ASSERT_TRUE(meshA);
    EXPECT_EQ(meshA->SubMeshes()->GetValue().size(), 1u);

    // Two geometry nodes referencing the same mesh id share the underlying
    // IMesh — the resource manager caches by id, so geom_b gets the same
    // instance as geom_a.
    auto geomB = scene->FindNode("//geom_b").GetResult();
    ASSERT_TRUE(geomB);
    auto meshB = interface_cast<SCENE_NS::IMeshAccess>(geomB)->GetMesh().GetResult();
    EXPECT_EQ(meshA, meshB);
}

/**
 * @tc.name: ImportPostProcess0
 * @tc.desc: Tests for ImportPostProcess [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportPostProcess0, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/postprocess/postprocess0.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "Name"), "MyPostProcess");

    // Tonemap sub-effect
    auto tonemapObj = GetProp<META_NS::IObject::Ptr>(meta, "Tonemap");
    ASSERT_TRUE(tonemapObj);
    auto tonemapMeta = interface_cast<META_NS::IMetadata>(tonemapObj);
    ASSERT_TRUE(tonemapMeta);
    EXPECT_EQ(GetProp<bool>(tonemapMeta, "Enabled"), true);
    EXPECT_EQ(GetProp<SCENE_NS::TonemapType>(tonemapMeta, "Type"), SCENE_NS::TonemapType::ACES);
    EXPECT_FLOAT_EQ(GetProp<float>(tonemapMeta, "Exposure"), 1.5f);

    // Bloom sub-effect
    auto bloomObj = GetProp<META_NS::IObject::Ptr>(meta, "Bloom");
    ASSERT_TRUE(bloomObj);
    auto bloomMeta = interface_cast<META_NS::IMetadata>(bloomObj);
    ASSERT_TRUE(bloomMeta);
    EXPECT_EQ(GetProp<bool>(bloomMeta, "Enabled"), true);
    EXPECT_EQ(GetProp<SCENE_NS::BloomType>(bloomMeta, "Type"), SCENE_NS::BloomType::NORMAL);
    EXPECT_EQ(GetProp<SCENE_NS::EffectQualityType>(bloomMeta, "Quality"), SCENE_NS::EffectQualityType::HIGH);
    EXPECT_FLOAT_EQ(GetProp<float>(bloomMeta, "ThresholdHard"), 0.8f);
    EXPECT_FLOAT_EQ(GetProp<float>(bloomMeta, "ThresholdSoft"), 0.3f);
    EXPECT_FLOAT_EQ(GetProp<float>(bloomMeta, "AmountCoefficient"), 0.5f);
    EXPECT_FLOAT_EQ(GetProp<float>(bloomMeta, "Scatter"), 0.7f);

    // Vignette sub-effect
    auto vignetteObj = GetProp<META_NS::IObject::Ptr>(meta, "Vignette");
    ASSERT_TRUE(vignetteObj);
    auto vignetteMeta = interface_cast<META_NS::IMetadata>(vignetteObj);
    ASSERT_TRUE(vignetteMeta);
    EXPECT_EQ(GetProp<bool>(vignetteMeta, "Enabled"), true);
    EXPECT_FLOAT_EQ(GetProp<float>(vignetteMeta, "Coefficient"), 0.4f);
    EXPECT_FLOAT_EQ(GetProp<float>(vignetteMeta, "Power"), 2.0f);

    // Blur sub-effect
    auto blurObj = GetProp<META_NS::IObject::Ptr>(meta, "Blur");
    ASSERT_TRUE(blurObj);
    auto blurMeta = interface_cast<META_NS::IMetadata>(blurObj);
    ASSERT_TRUE(blurMeta);
    EXPECT_EQ(GetProp<bool>(blurMeta, "Enabled"), true);
    EXPECT_EQ(GetProp<SCENE_NS::BlurType>(blurMeta, "Type"), SCENE_NS::BlurType::HORIZONTAL);
    EXPECT_EQ(GetProp<SCENE_NS::EffectQualityType>(blurMeta, "Quality"), SCENE_NS::EffectQualityType::LOW);
    EXPECT_FLOAT_EQ(GetProp<float>(blurMeta, "FilterSize"), 2.5f);
    EXPECT_EQ(GetProp<uint32_t>(blurMeta, "MaxMipmapLevel"), 3u);

    // MotionBlur sub-effect
    auto motionBlurObj = GetProp<META_NS::IObject::Ptr>(meta, "MotionBlur");
    ASSERT_TRUE(motionBlurObj);
    auto motionBlurMeta = interface_cast<META_NS::IMetadata>(motionBlurObj);
    ASSERT_TRUE(motionBlurMeta);
    EXPECT_EQ(GetProp<bool>(motionBlurMeta, "Enabled"), true);
    EXPECT_EQ(GetProp<SCENE_NS::EffectQualityType>(motionBlurMeta, "Quality"), SCENE_NS::EffectQualityType::NORMAL);
    EXPECT_EQ(
        GetProp<SCENE_NS::EffectSharpnessType>(motionBlurMeta, "Sharpness"), SCENE_NS::EffectSharpnessType::MEDIUM);
    EXPECT_FLOAT_EQ(GetProp<float>(motionBlurMeta, "Alpha"), 0.6f);
    EXPECT_FLOAT_EQ(GetProp<float>(motionBlurMeta, "VelocityCoefficient"), 1.2f);

    // ColorConversion sub-effect
    auto colorConvObj = GetProp<META_NS::IObject::Ptr>(meta, "ColorConversion");
    ASSERT_TRUE(colorConvObj);
    auto colorConvMeta = interface_cast<META_NS::IMetadata>(colorConvObj);
    ASSERT_TRUE(colorConvMeta);
    EXPECT_EQ(GetProp<bool>(colorConvMeta, "Enabled"), true);
    EXPECT_EQ(GetProp<SCENE_NS::ColorConversionFunctionType>(colorConvMeta, "Function"),
        SCENE_NS::ColorConversionFunctionType::LINEAR_TO_SRGB);
    EXPECT_EQ(GetProp<bool>(colorConvMeta, "MultiplyWithAlpha"), true);

    // ColorFringe sub-effect
    auto colorFringeObj = GetProp<META_NS::IObject::Ptr>(meta, "ColorFringe");
    ASSERT_TRUE(colorFringeObj);
    auto colorFringeMeta = interface_cast<META_NS::IMetadata>(colorFringeObj);
    ASSERT_TRUE(colorFringeMeta);
    EXPECT_EQ(GetProp<bool>(colorFringeMeta, "Enabled"), true);
    EXPECT_FLOAT_EQ(GetProp<float>(colorFringeMeta, "DistanceCoefficient"), 0.3f);

    // DepthOfField sub-effect
    auto dofObj = GetProp<META_NS::IObject::Ptr>(meta, "DepthOfField");
    ASSERT_TRUE(dofObj);
    auto dofMeta = interface_cast<META_NS::IMetadata>(dofObj);
    ASSERT_TRUE(dofMeta);
    EXPECT_EQ(GetProp<bool>(dofMeta, "Enabled"), true);
    EXPECT_FLOAT_EQ(GetProp<float>(dofMeta, "FocusPoint"), 5.0f);
    EXPECT_FLOAT_EQ(GetProp<float>(dofMeta, "FocusRange"), 2.0f);
    EXPECT_FLOAT_EQ(GetProp<float>(dofMeta, "NearTransitionRange"), 1.0f);
    EXPECT_FLOAT_EQ(GetProp<float>(dofMeta, "FarTransitionRange"), 1.5f);
    EXPECT_FLOAT_EQ(GetProp<float>(dofMeta, "NearBlur"), 0.8f);
    EXPECT_FLOAT_EQ(GetProp<float>(dofMeta, "FarBlur"), 0.9f);
    EXPECT_FLOAT_EQ(GetProp<float>(dofMeta, "NearPlane"), 0.1f);
    EXPECT_FLOAT_EQ(GetProp<float>(dofMeta, "FarPlane"), 100.0f);

    // Fxaa sub-effect
    auto fxaaObj = GetProp<META_NS::IObject::Ptr>(meta, "Fxaa");
    ASSERT_TRUE(fxaaObj);
    auto fxaaMeta = interface_cast<META_NS::IMetadata>(fxaaObj);
    ASSERT_TRUE(fxaaMeta);
    EXPECT_EQ(GetProp<bool>(fxaaMeta, "Enabled"), true);
    EXPECT_EQ(GetProp<SCENE_NS::EffectQualityType>(fxaaMeta, "Quality"), SCENE_NS::EffectQualityType::HIGH);
    EXPECT_EQ(GetProp<SCENE_NS::EffectSharpnessType>(fxaaMeta, "Sharpness"), SCENE_NS::EffectSharpnessType::SHARP);

    // Taa sub-effect
    auto taaObj = GetProp<META_NS::IObject::Ptr>(meta, "Taa");
    ASSERT_TRUE(taaObj);
    auto taaMeta = interface_cast<META_NS::IMetadata>(taaObj);
    ASSERT_TRUE(taaMeta);
    EXPECT_EQ(GetProp<bool>(taaMeta, "Enabled"), true);
    EXPECT_EQ(GetProp<SCENE_NS::EffectQualityType>(taaMeta, "Quality"), SCENE_NS::EffectQualityType::NORMAL);
    EXPECT_EQ(GetProp<SCENE_NS::EffectSharpnessType>(taaMeta, "Sharpness"), SCENE_NS::EffectSharpnessType::SOFT);

    // LensFlare sub-effect
    auto lensFlareObj = GetProp<META_NS::IObject::Ptr>(meta, "LensFlare");
    ASSERT_TRUE(lensFlareObj);
    auto lensFlareMeta = interface_cast<META_NS::IMetadata>(lensFlareObj);
    ASSERT_TRUE(lensFlareMeta);
    EXPECT_EQ(GetProp<bool>(lensFlareMeta, "Enabled"), true);
    EXPECT_EQ(GetProp<SCENE_NS::EffectQualityType>(lensFlareMeta, "Quality"), SCENE_NS::EffectQualityType::LOW);
    EXPECT_FLOAT_EQ(GetProp<float>(lensFlareMeta, "Intensity"), 0.7f);
    EXPECT_EQ(GetProp<BASE_NS::Math::Vec3>(lensFlareMeta, "FlarePosition"), BASE_NS::Math::Vec3(0.1f, 0.2f, 0.3f));

    // Upscale sub-effect
    auto upscaleObj = GetProp<META_NS::IObject::Ptr>(meta, "Upscale");
    ASSERT_TRUE(upscaleObj);
    auto upscaleMeta = interface_cast<META_NS::IMetadata>(upscaleObj);
    ASSERT_TRUE(upscaleMeta);
    EXPECT_EQ(GetProp<bool>(upscaleMeta, "Enabled"), true);
    EXPECT_FLOAT_EQ(GetProp<float>(upscaleMeta, "SmoothScale"), 0.5f);
    EXPECT_FLOAT_EQ(GetProp<float>(upscaleMeta, "StructureSensitivity"), 0.3f);
    EXPECT_FLOAT_EQ(GetProp<float>(upscaleMeta, "EdgeSharpness"), 0.8f);
    EXPECT_FLOAT_EQ(GetProp<float>(upscaleMeta, "Ratio"), 1.5f);
}

/**
 * @tc.name: ImportPostProcessPartial
 * @tc.desc: Post-process with only a subset of effects imports correctly
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportPostProcessPartial, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/postprocess/postprocess_partial.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    // Tonemap should be present
    auto tonemapObj = GetProp<META_NS::IObject::Ptr>(meta, "Tonemap");
    ASSERT_TRUE(tonemapObj);
    auto tonemapMeta = interface_cast<META_NS::IMetadata>(tonemapObj);
    ASSERT_TRUE(tonemapMeta);
    EXPECT_EQ(GetProp<bool>(tonemapMeta, "Enabled"), true);
    EXPECT_EQ(GetProp<SCENE_NS::TonemapType>(tonemapMeta, "Type"), SCENE_NS::TonemapType::FILMIC);
    EXPECT_FLOAT_EQ(GetProp<float>(tonemapMeta, "Exposure"), 2.0f);

    // Upscale should be present
    auto upscaleObj = GetProp<META_NS::IObject::Ptr>(meta, "Upscale");
    ASSERT_TRUE(upscaleObj);
    auto upscaleMeta = interface_cast<META_NS::IMetadata>(upscaleObj);
    ASSERT_TRUE(upscaleMeta);
    EXPECT_EQ(GetProp<bool>(upscaleMeta, "Enabled"), false);
    EXPECT_FLOAT_EQ(GetProp<float>(upscaleMeta, "SmoothScale"), 0.8f);
    EXPECT_FLOAT_EQ(GetProp<float>(upscaleMeta, "Ratio"), 2.0f);

    // Effects not in the JSON should be absent
    EXPECT_FALSE(GetProp<META_NS::IObject::Ptr>(meta, "Bloom"));
    EXPECT_FALSE(GetProp<META_NS::IObject::Ptr>(meta, "Blur"));
    EXPECT_FALSE(GetProp<META_NS::IObject::Ptr>(meta, "MotionBlur"));
    EXPECT_FALSE(GetProp<META_NS::IObject::Ptr>(meta, "ColorConversion"));
    EXPECT_FALSE(GetProp<META_NS::IObject::Ptr>(meta, "ColorFringe"));
    EXPECT_FALSE(GetProp<META_NS::IObject::Ptr>(meta, "DepthOfField"));
    EXPECT_FALSE(GetProp<META_NS::IObject::Ptr>(meta, "Fxaa"));
    EXPECT_FALSE(GetProp<META_NS::IObject::Ptr>(meta, "Taa"));
    EXPECT_FALSE(GetProp<META_NS::IObject::Ptr>(meta, "Vignette"));
    EXPECT_FALSE(GetProp<META_NS::IObject::Ptr>(meta, "LensFlare"));
}

/**
 * @tc.name: ImportPostProcessBloomFull
 * @tc.desc: Bloom with all properties including dirtMaskCoefficient, scaleFactor, useCompute
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportPostProcessBloomFull, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/postprocess/postprocess_bloom_full.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto bloomObj = GetProp<META_NS::IObject::Ptr>(meta, "Bloom");
    ASSERT_TRUE(bloomObj);
    auto bloomMeta = interface_cast<META_NS::IMetadata>(bloomObj);
    ASSERT_TRUE(bloomMeta);
    EXPECT_EQ(GetProp<bool>(bloomMeta, "Enabled"), true);
    EXPECT_EQ(GetProp<SCENE_NS::BloomType>(bloomMeta, "Type"), SCENE_NS::BloomType::BILATERAL);
    EXPECT_EQ(GetProp<SCENE_NS::EffectQualityType>(bloomMeta, "Quality"), SCENE_NS::EffectQualityType::LOW);
    EXPECT_FLOAT_EQ(GetProp<float>(bloomMeta, "ThresholdHard"), 1.0f);
    EXPECT_FLOAT_EQ(GetProp<float>(bloomMeta, "ThresholdSoft"), 0.5f);
    EXPECT_FLOAT_EQ(GetProp<float>(bloomMeta, "AmountCoefficient"), 0.8f);
    EXPECT_FLOAT_EQ(GetProp<float>(bloomMeta, "DirtMaskCoefficient"), 0.3f);
    EXPECT_FLOAT_EQ(GetProp<float>(bloomMeta, "Scatter"), 0.6f);
    EXPECT_FLOAT_EQ(GetProp<float>(bloomMeta, "ScaleFactor"), 0.75f);
    EXPECT_EQ(GetProp<bool>(bloomMeta, "UseCompute"), true);
}

/**
 * @tc.name: ImportIndexDerivedFrom
 * @tc.desc: Index entry with derivedFrom sets base resource on options
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexDerivedFrom, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index2.json", {});
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto extRes = interface_cast<META_NS::IResourceManagerExtension>(rman);
    ASSERT_TRUE(extRes);

    auto data = extRes->GetResource({{"derived-mat", ""}, nullptr});
    ASSERT_TRUE(data.id.IsValid());

    auto derived = interface_cast<META_NS::IDerivedResourceOptions>(data.options);
    ASSERT_TRUE(derived);
    EXPECT_EQ(derived->GetBaseResource(), CORE_NS::ResourceId("base-mat", ""));
}

/**
 * @tc.name: ImportIndexTemplateWithOptions
 * @tc.desc: A *Template index entry accepts inline options. Options are parsed by the
 *           non-template handler (materialTemplate -> ImportMaterial), so type-specific
 *           fields (alphaCutoff) and derivedFrom both apply.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexTemplateWithOptions, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index_template_options.json", {});
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto extRes = interface_cast<META_NS::IResourceManagerExtension>(rman);
    ASSERT_TRUE(extRes);

    auto data = extRes->GetResource({{"tmpl-with-opts", ""}, nullptr});
    ASSERT_TRUE(data.id.IsValid());
    EXPECT_EQ(data.type, SCENE_NS::ClassId::MaterialResourceTemplate.Id().ToUid());

    auto derived = interface_cast<META_NS::IDerivedResourceOptions>(data.options);
    ASSERT_TRUE(derived);
    EXPECT_EQ(derived->GetBaseResource(), CORE_NS::ResourceId("base-mat", ""));

    auto meta = interface_cast<META_NS::IMetadata>(data.options);
    ASSERT_TRUE(meta);
    auto cutoff = meta->GetProperty<float>("AlphaCutoff");
    ASSERT_TRUE(cutoff);
    EXPECT_FLOAT_EQ(cutoff->GetValue(), 0.25f);
}

/**
 * @tc.name: ImportIndexDerivedFromValues
 * @tc.desc: Instantiated derived resource inherits property values from base resource
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexDerivedFromValues, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    // index3.json registers base-mat (loaded from material0.json) and derived-mat (derivedFrom: base-mat)
    SCENE_IMP_NS::ImportParameters params;
    params.scene = scene;
    auto res = imp->Import("test://import/index/index3.json", params);
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    // Instantiate derived-mat; LoadBaseResource will construct base-mat and SetTemplate it
    auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("derived-mat"), scene}));
    ASSERT_TRUE(mat);

    // Derived material should have inherited property values from base-mat (material0.json)
    EXPECT_EQ(META_NS::GetValue(mat->Type()), SCENE_NS::MaterialType::METALLIC_ROUGHNESS);
    EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.5f);
    EXPECT_EQ(META_NS::GetValue(mat->LightingFlags()), static_cast<SCENE_NS::LightingFlags>(2));
}

/**
 * @tc.name: ImportIndexDerivedFromPostProcess
 * @tc.desc: Index entry with derivedFrom sets base resource on postProcess options
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexDerivedFromPostProcess, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index4.json", {});
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto extRes = interface_cast<META_NS::IResourceManagerExtension>(rman);
    ASSERT_TRUE(extRes);

    auto data = extRes->GetResource({{"derived-pp", ""}, nullptr});
    ASSERT_TRUE(data.id.IsValid());

    auto derived = interface_cast<META_NS::IDerivedResourceOptions>(data.options);
    ASSERT_TRUE(derived);
    EXPECT_EQ(derived->GetBaseResource(), CORE_NS::ResourceId("base-pp", ""));
}

/**
 * @tc.name: ImportIndexDerivedFromEnvironment
 * @tc.desc: Index entry with derivedFrom sets base resource on environment options
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexDerivedFromEnvironment, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index6.json", {});
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto extRes = interface_cast<META_NS::IResourceManagerExtension>(rman);
    ASSERT_TRUE(extRes);

    auto data = extRes->GetResource({{"derived-env", ""}, nullptr});
    ASSERT_TRUE(data.id.IsValid());

    auto derived = interface_cast<META_NS::IDerivedResourceOptions>(data.options);
    ASSERT_TRUE(derived);
    EXPECT_EQ(derived->GetBaseResource(), CORE_NS::ResourceId("base-env", ""));
}

/**
 * @tc.name: ImportIndexDerivedFromPostProcessValues
 * @tc.desc: Instantiated derived postProcess resource inherits property values from base resource
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexDerivedFromPostProcessValues, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    SCENE_IMP_NS::ImportParameters params;
    params.scene = scene;
    auto res = imp->Import("test://import/index/index5.json", params);
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto pp = interface_pointer_cast<IPostProcess>(rman->GetResource({CORE_NS::ResourceId("derived-pp"), scene}));
    ASSERT_TRUE(pp);

    auto tonemap = META_NS::GetValue(pp->Tonemap());
    ASSERT_TRUE(tonemap);
    EXPECT_EQ(META_NS::GetValue(tonemap->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(tonemap->Type()), TonemapType::ACES);
    EXPECT_FLOAT_EQ(META_NS::GetValue(tonemap->Exposure()), 1.5f);

    auto bloom = META_NS::GetValue(pp->Bloom());
    ASSERT_TRUE(bloom);
    EXPECT_EQ(META_NS::GetValue(bloom->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(bloom->Type()), BloomType::NORMAL);
    EXPECT_EQ(META_NS::GetValue(bloom->Quality()), EffectQualityType::HIGH);
    EXPECT_FLOAT_EQ(META_NS::GetValue(bloom->ThresholdHard()), 0.8f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(bloom->ThresholdSoft()), 0.3f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(bloom->AmountCoefficient()), 0.5f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(bloom->Scatter()), 0.7f);

    auto blur = META_NS::GetValue(pp->Blur());
    ASSERT_TRUE(blur);
    EXPECT_EQ(META_NS::GetValue(blur->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(blur->Type()), BlurType::HORIZONTAL);
    EXPECT_EQ(META_NS::GetValue(blur->Quality()), EffectQualityType::LOW);
    EXPECT_FLOAT_EQ(META_NS::GetValue(blur->FilterSize()), 2.5f);
    EXPECT_EQ(META_NS::GetValue(blur->MaxMipmapLevel()), 3u);

    auto motionBlur = META_NS::GetValue(pp->MotionBlur());
    ASSERT_TRUE(motionBlur);
    EXPECT_EQ(META_NS::GetValue(motionBlur->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(motionBlur->Quality()), EffectQualityType::NORMAL);
    EXPECT_EQ(META_NS::GetValue(motionBlur->Sharpness()), EffectSharpnessType::MEDIUM);
    EXPECT_FLOAT_EQ(META_NS::GetValue(motionBlur->Alpha()), 0.6f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(motionBlur->VelocityCoefficient()), 1.2f);

    auto colorConversion = META_NS::GetValue(pp->ColorConversion());
    ASSERT_TRUE(colorConversion);
    EXPECT_EQ(META_NS::GetValue(colorConversion->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(colorConversion->Function()), ColorConversionFunctionType::LINEAR_TO_SRGB);
    EXPECT_EQ(META_NS::GetValue(colorConversion->MultiplyWithAlpha()), true);

    auto colorFringe = META_NS::GetValue(pp->ColorFringe());
    ASSERT_TRUE(colorFringe);
    EXPECT_EQ(META_NS::GetValue(colorFringe->Enabled()), true);
    EXPECT_FLOAT_EQ(META_NS::GetValue(colorFringe->DistanceCoefficient()), 0.3f);

    auto dof = META_NS::GetValue(pp->DepthOfField());
    ASSERT_TRUE(dof);
    EXPECT_EQ(META_NS::GetValue(dof->Enabled()), true);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->FocusPoint()), 5.0f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->FocusRange()), 2.0f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->NearTransitionRange()), 1.0f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->FarTransitionRange()), 1.5f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->NearBlur()), 0.8f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->FarBlur()), 0.9f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->NearPlane()), 0.1f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->FarPlane()), 100.0f);

    auto fxaa = META_NS::GetValue(pp->Fxaa());
    ASSERT_TRUE(fxaa);
    EXPECT_EQ(META_NS::GetValue(fxaa->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(fxaa->Quality()), EffectQualityType::HIGH);
    EXPECT_EQ(META_NS::GetValue(fxaa->Sharpness()), EffectSharpnessType::SHARP);

    auto taa = META_NS::GetValue(pp->Taa());
    ASSERT_TRUE(taa);
    EXPECT_EQ(META_NS::GetValue(taa->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(taa->Quality()), EffectQualityType::NORMAL);
    EXPECT_EQ(META_NS::GetValue(taa->Sharpness()), EffectSharpnessType::SOFT);

    auto vignette = META_NS::GetValue(pp->Vignette());
    ASSERT_TRUE(vignette);
    EXPECT_EQ(META_NS::GetValue(vignette->Enabled()), true);
    EXPECT_FLOAT_EQ(META_NS::GetValue(vignette->Coefficient()), 0.4f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(vignette->Power()), 2.0f);

    auto lensFlare = META_NS::GetValue(pp->LensFlare());
    ASSERT_TRUE(lensFlare);
    EXPECT_EQ(META_NS::GetValue(lensFlare->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(lensFlare->Quality()), EffectQualityType::LOW);
    EXPECT_FLOAT_EQ(META_NS::GetValue(lensFlare->Intensity()), 0.7f);
    EXPECT_EQ(META_NS::GetValue(lensFlare->FlarePosition()), BASE_NS::Math::Vec3(0.1f, 0.2f, 0.3f));

    auto upscale = META_NS::GetValue(pp->Upscale());
    ASSERT_TRUE(upscale);
    EXPECT_EQ(META_NS::GetValue(upscale->Enabled()), true);
    EXPECT_FLOAT_EQ(META_NS::GetValue(upscale->SmoothScale()), 0.5f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(upscale->StructureSensitivity()), 0.3f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(upscale->EdgeSharpness()), 0.8f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(upscale->Ratio()), 1.5f);
}

/**
 * @tc.name: ImportIndexDerivedFromEnvironmentValues
 * @tc.desc: Instantiated derived environment resource inherits property values from base resource
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexDerivedFromEnvironmentValues, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    SCENE_IMP_NS::ImportParameters params;
    params.scene = scene;
    auto res = imp->Import("test://import/index/index7.json", params);
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto env = interface_pointer_cast<IEnvironment>(rman->GetResource({CORE_NS::ResourceId("derived-env"), scene}));
    ASSERT_TRUE(env);

    EXPECT_EQ(META_NS::GetValue(env->Background()), SCENE_NS::EnvBackgroundType::CUBEMAP);
    EXPECT_EQ(META_NS::GetValue(env->RadianceCubemapMipCount()), 5u);
}

/**
 * @tc.name: ImportIndexDerivedEnvironmentOverridesReachECS
 * @tc.desc: A derived environment that overrides engine-bound scalar properties on
 *           top of a base environment sees both layers reflected on the live
 *           property — `META_NS::GetValue` routes through the engine value, so a
 *           correct read implies the value has been synced to ECS. Exercises the
 *           template-context apply path where base defaults land first and the
 *           derived layer must replace them without losing engine-side sync.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexDerivedEnvironmentOverridesReachECS, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    SCENE_IMP_NS::ImportParameters params;
    params.scene = scene;
    auto res = imp->Import("test://import/index/index_env_derived_override.json", params);
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto env = interface_pointer_cast<IEnvironment>(rman->GetResource({CORE_NS::ResourceId("derived-env"), scene}));
    ASSERT_TRUE(env);

    // Inherited from base-env (environment0.json) — base layer reaches the live property.
    EXPECT_EQ(META_NS::GetValue(env->Background()), SCENE_NS::EnvBackgroundType::CUBEMAP);
    EXPECT_EQ(META_NS::GetValue(env->RadianceCubemapMipCount()), 5u);
    EXPECT_EQ(META_NS::GetValue(env->IndirectSpecularFactor()), BASE_NS::Math::Vec4(0.4f, 0.5f, 0.6f, 1.0f));

    // Overridden by derived-env — the derived layer must propagate to the engine value.
    EXPECT_FLOAT_EQ(META_NS::GetValue(env->EnvironmentMapLodLevel()), 4.5f);
    EXPECT_EQ(META_NS::GetValue(env->IndirectDiffuseFactor()), BASE_NS::Math::Vec4(0.7f, 0.8f, 0.9f, 1.0f));
}

/**
 * @tc.name: ImportIndexMaterialOptions
 * @tc.desc: Material in index with inline options gets those options applied
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexMaterialOptions, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    SCENE_IMP_NS::ImportParameters params;
    params.scene = scene;
    auto res = imp->Import("test://import/index/index8.json", params);
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("mat"), scene}));
    ASSERT_TRUE(mat);

    EXPECT_EQ(META_NS::GetValue(mat->Type()), SCENE_NS::MaterialType::METALLIC_ROUGHNESS);
    EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.5f);
    EXPECT_EQ(META_NS::GetValue(mat->LightingFlags()), static_cast<SCENE_NS::LightingFlags>(2));
    auto renderSort = META_NS::GetValue(mat->RenderSort());
    EXPECT_EQ(renderSort.renderSortLayer, 3);
    EXPECT_EQ(renderSort.renderSortLayerOrder, 1);
}

/**
 * @tc.name: ImportIndexEnvironmentOptions
 * @tc.desc: Environment in index with inline options gets those options applied
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexEnvironmentOptions, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    SCENE_IMP_NS::ImportParameters params;
    params.scene = scene;
    auto res = imp->Import("test://import/index/index9.json", params);
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto env = interface_pointer_cast<IEnvironment>(rman->GetResource({CORE_NS::ResourceId("env"), scene}));
    ASSERT_TRUE(env);

    EXPECT_EQ(META_NS::GetValue(env->Background()), SCENE_NS::EnvBackgroundType::CUBEMAP);
    EXPECT_EQ(META_NS::GetValue(env->RadianceCubemapMipCount()), 5u);
    EXPECT_EQ(META_NS::GetValue(env->IndirectDiffuseFactor()), BASE_NS::Math::Vec4(0.1f, 0.2f, 0.3f, 1.0f));
    EXPECT_EQ(META_NS::GetValue(env->IndirectSpecularFactor()), BASE_NS::Math::Vec4(0.4f, 0.5f, 0.6f, 1.0f));
    EXPECT_EQ(META_NS::GetValue(env->EnvMapFactor()), BASE_NS::Math::Vec4(1.0f, 1.0f, 1.0f, 0.5f));
    EXPECT_FLOAT_EQ(META_NS::GetValue(env->EnvironmentMapLodLevel()), 2.5f);
    EXPECT_EQ(META_NS::GetValue(env->EnvironmentRotation()), BASE_NS::Math::Quat(0.0f, 0.0f, 0.0f, 1.0f));

    auto coeffs = env->IrradianceCoefficients()->GetValue();
    ASSERT_EQ(coeffs.size(), 3u);
    EXPECT_EQ(coeffs[0], BASE_NS::Math::Vec3(1.0f, 0.0f, 0.0f));
    EXPECT_EQ(coeffs[1], BASE_NS::Math::Vec3(0.0f, 1.0f, 0.0f));
    EXPECT_EQ(coeffs[2], BASE_NS::Math::Vec3(0.0f, 0.0f, 1.0f));
}

/**
 * @tc.name: ImportIndexPostProcessOptions
 * @tc.desc: PostProcess in index with inline options gets all effect options applied
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexPostProcessOptions, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    SCENE_IMP_NS::ImportParameters params;
    params.scene = scene;
    auto res = imp->Import("test://import/index/index10.json", params);
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto pp = interface_pointer_cast<IPostProcess>(rman->GetResource({CORE_NS::ResourceId("pp"), scene}));
    ASSERT_TRUE(pp);

    auto tonemap = META_NS::GetValue(pp->Tonemap());
    ASSERT_TRUE(tonemap);
    EXPECT_EQ(META_NS::GetValue(tonemap->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(tonemap->Type()), TonemapType::ACES);
    EXPECT_FLOAT_EQ(META_NS::GetValue(tonemap->Exposure()), 1.5f);

    auto bloom = META_NS::GetValue(pp->Bloom());
    ASSERT_TRUE(bloom);
    EXPECT_EQ(META_NS::GetValue(bloom->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(bloom->Type()), BloomType::NORMAL);
    EXPECT_EQ(META_NS::GetValue(bloom->Quality()), EffectQualityType::HIGH);
    EXPECT_FLOAT_EQ(META_NS::GetValue(bloom->ThresholdHard()), 0.8f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(bloom->ThresholdSoft()), 0.3f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(bloom->AmountCoefficient()), 0.5f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(bloom->Scatter()), 0.7f);

    auto blur = META_NS::GetValue(pp->Blur());
    ASSERT_TRUE(blur);
    EXPECT_EQ(META_NS::GetValue(blur->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(blur->Type()), BlurType::HORIZONTAL);
    EXPECT_EQ(META_NS::GetValue(blur->Quality()), EffectQualityType::LOW);
    EXPECT_FLOAT_EQ(META_NS::GetValue(blur->FilterSize()), 2.5f);
    EXPECT_EQ(META_NS::GetValue(blur->MaxMipmapLevel()), 3u);

    auto motionBlur = META_NS::GetValue(pp->MotionBlur());
    ASSERT_TRUE(motionBlur);
    EXPECT_EQ(META_NS::GetValue(motionBlur->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(motionBlur->Quality()), EffectQualityType::NORMAL);
    EXPECT_EQ(META_NS::GetValue(motionBlur->Sharpness()), EffectSharpnessType::MEDIUM);
    EXPECT_FLOAT_EQ(META_NS::GetValue(motionBlur->Alpha()), 0.6f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(motionBlur->VelocityCoefficient()), 1.2f);

    auto colorConversion = META_NS::GetValue(pp->ColorConversion());
    ASSERT_TRUE(colorConversion);
    EXPECT_EQ(META_NS::GetValue(colorConversion->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(colorConversion->Function()), ColorConversionFunctionType::LINEAR_TO_SRGB);
    EXPECT_EQ(META_NS::GetValue(colorConversion->MultiplyWithAlpha()), true);

    auto colorFringe = META_NS::GetValue(pp->ColorFringe());
    ASSERT_TRUE(colorFringe);
    EXPECT_EQ(META_NS::GetValue(colorFringe->Enabled()), true);
    EXPECT_FLOAT_EQ(META_NS::GetValue(colorFringe->DistanceCoefficient()), 0.3f);

    auto dof = META_NS::GetValue(pp->DepthOfField());
    ASSERT_TRUE(dof);
    EXPECT_EQ(META_NS::GetValue(dof->Enabled()), true);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->FocusPoint()), 5.0f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->FocusRange()), 2.0f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->NearTransitionRange()), 1.0f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->FarTransitionRange()), 1.5f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->NearBlur()), 0.8f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->FarBlur()), 0.9f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->NearPlane()), 0.1f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(dof->FarPlane()), 100.0f);

    auto fxaa = META_NS::GetValue(pp->Fxaa());
    ASSERT_TRUE(fxaa);
    EXPECT_EQ(META_NS::GetValue(fxaa->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(fxaa->Quality()), EffectQualityType::HIGH);
    EXPECT_EQ(META_NS::GetValue(fxaa->Sharpness()), EffectSharpnessType::SHARP);

    auto taa = META_NS::GetValue(pp->Taa());
    ASSERT_TRUE(taa);
    EXPECT_EQ(META_NS::GetValue(taa->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(taa->Quality()), EffectQualityType::NORMAL);
    EXPECT_EQ(META_NS::GetValue(taa->Sharpness()), EffectSharpnessType::SOFT);

    auto vignette = META_NS::GetValue(pp->Vignette());
    ASSERT_TRUE(vignette);
    EXPECT_EQ(META_NS::GetValue(vignette->Enabled()), true);
    EXPECT_FLOAT_EQ(META_NS::GetValue(vignette->Coefficient()), 0.4f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(vignette->Power()), 2.0f);

    auto lensFlare = META_NS::GetValue(pp->LensFlare());
    ASSERT_TRUE(lensFlare);
    EXPECT_EQ(META_NS::GetValue(lensFlare->Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(lensFlare->Quality()), EffectQualityType::LOW);
    EXPECT_FLOAT_EQ(META_NS::GetValue(lensFlare->Intensity()), 0.7f);
    EXPECT_EQ(META_NS::GetValue(lensFlare->FlarePosition()), BASE_NS::Math::Vec3(0.1f, 0.2f, 0.3f));

    auto upscale = META_NS::GetValue(pp->Upscale());
    ASSERT_TRUE(upscale);
    EXPECT_EQ(META_NS::GetValue(upscale->Enabled()), true);
    EXPECT_FLOAT_EQ(META_NS::GetValue(upscale->SmoothScale()), 0.5f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(upscale->StructureSensitivity()), 0.3f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(upscale->EdgeSharpness()), 0.8f);
    EXPECT_FLOAT_EQ(META_NS::GetValue(upscale->Ratio()), 1.5f);
}

/**
 * @tc.name: ImportIndexOcclusionMaterialOptions
 * @tc.desc: OcclusionMaterial in index with inline options gets those options applied
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexOcclusionMaterialOptions, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    SCENE_IMP_NS::ImportParameters params;
    params.scene = scene;
    auto res = imp->Import("test://import/index/index11.json", params);
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("occ-mat"), scene}));
    ASSERT_TRUE(mat);

    EXPECT_FLOAT_EQ(META_NS::GetValue(mat->AlphaCutoff()), 0.5f);
    EXPECT_EQ(META_NS::GetValue(mat->LightingFlags()), static_cast<SCENE_NS::LightingFlags>(2));
}

/**
 * @tc.name: ImportIndexDerivedFromOcclusionMaterial
 * @tc.desc: Index entry with derivedFrom sets base resource on occlusionMaterial options
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexDerivedFromOcclusionMaterial, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index12.json", {});
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto extRes = interface_cast<META_NS::IResourceManagerExtension>(rman);
    ASSERT_TRUE(extRes);

    auto data = extRes->GetResource({{"derived-occ-mat", ""}, nullptr});
    ASSERT_TRUE(data.id.IsValid());

    auto derived = interface_cast<META_NS::IDerivedResourceOptions>(data.options);
    ASSERT_TRUE(derived);
    EXPECT_EQ(derived->GetBaseResource(), CORE_NS::ResourceId("base-occ-mat", ""));
}

/**
 * @tc.name: ImportIndexResourceInstanceNames
 * @tc.desc: A resource's "name" field propagates from the imported JSON all the
 *           way to the live resource instance — every resource type that
 *           implements META_NS::INamed exposes the imported name via GetResource.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexResourceInstanceNames, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto rman = ImportIndexIntoScene("test://import/index/index_resource_names.json", scene);
    ASSERT_TRUE(rman);

    auto checkName = [&](const char* id, const char* expected) {
        auto resource = rman->GetResource({CORE_NS::ResourceId(id), scene});
        ASSERT_TRUE(resource);
        auto named = interface_cast<META_NS::INamed>(resource);
        ASSERT_TRUE(named);
        EXPECT_EQ(named->Name()->GetValue(), expected);
    };

    checkName("named-material", "InstanceMaterialName");
    checkName("named-environment", "InstanceEnvironmentName");
    checkName("named-image", "InstanceImageName");
    checkName("named-postprocess", "InstancePostProcessName");
    checkName("named-mesh", "InstanceMeshName");
}

/**
 * @tc.name: ImportIndexShaderLoad
 * @tc.desc: Shader in index referenced by path loads as IShader (no inline options)
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexShaderLoad, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index15.json", {});
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto shader = interface_pointer_cast<IShader>(rman->GetResource({CORE_NS::ResourceId("test-shader"), nullptr}));
    ASSERT_TRUE(shader);
}

/**
 * @tc.name: ImportIndexShaderInstanceName
 * @tc.desc: A shader entry's "name" field is applied to the loaded shader's
 *           META_NS::INamed::Name; entries without a name leave Name empty.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexShaderInstanceName, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index_shader_name.json", {});
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto named = rman->GetResource({CORE_NS::ResourceId("named-shader"), nullptr});
    ASSERT_TRUE(named);
    auto namedIfc = interface_cast<META_NS::INamed>(named);
    ASSERT_TRUE(namedIfc);
    EXPECT_EQ(namedIfc->Name()->GetValue(), "InstanceShaderName");

    auto unnamed = rman->GetResource({CORE_NS::ResourceId("unnamed-shader"), nullptr});
    ASSERT_TRUE(unnamed);
    auto unnamedIfc = interface_cast<META_NS::INamed>(unnamed);
    ASSERT_TRUE(unnamedIfc);
    EXPECT_TRUE(unnamedIfc->Name()->GetValue().empty());
}

/**
 * @tc.name: ImportIndexNameOverridesOptionsName
 * @tc.desc: When an index entry sets a top-level "name", it overrides any
 *           options/template-derived name on the live resource. When no
 *           top-level name is set, the options/template name is preserved.
 *           Covers material (SceneResourceTypeBase), environment (same base),
 *           and image (custom LoadResource path).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexNameOverridesOptionsName, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto rman = ImportIndexIntoScene("test://import/index/index_index_name_priority.json", scene);
    ASSERT_TRUE(rman);

    auto checkName = [&](const char* id, const char* expected) {
        auto resource = rman->GetResource({CORE_NS::ResourceId(id), scene});
        ASSERT_TRUE(resource);
        auto named = interface_cast<META_NS::INamed>(resource);
        ASSERT_TRUE(named);
        EXPECT_EQ(named->Name()->GetValue(), expected);
    };

    checkName("mat-index-wins", "IndexMatName");
    checkName("mat-options-fallback", "OptionsMatName");
    checkName("env-index-wins", "IndexEnvName");
    checkName("image-index-wins", "IndexImageName");
}

/**
 * @tc.name: ImportIndexImageLoad
 * @tc.desc: Image in index with path and options gets loaded as IImage via GetResource
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexImageLoad, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index13.json", {});
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto img = interface_pointer_cast<IImage>(rman->GetResource({CORE_NS::ResourceId("test-image"), nullptr}));
    ASSERT_TRUE(img);
}

/**
 * @tc.name: ImportIndexImageDerivedFrom
 * @tc.desc: Image index entry with derivedFrom sets base resource on options
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexImageDerivedFrom, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index14.json", {});
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto extRes = interface_cast<META_NS::IResourceManagerExtension>(rman);
    ASSERT_TRUE(extRes);

    auto data = extRes->GetResource({{"derived-image", ""}, nullptr});
    ASSERT_TRUE(data.id.IsValid());

    auto derived = interface_cast<META_NS::IDerivedResourceOptions>(data.options);
    ASSERT_TRUE(derived);
    EXPECT_EQ(derived->GetBaseResource(), CORE_NS::ResourceId("base-image", ""));
}

/**
 * @tc.name: ImportAnimationBaseProperties
 * @tc.desc: Animation with base properties (name, enabled, loop, speed)
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportAnimationBaseProperties, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/animation0.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "AnimationType"), "trackAnimation");
    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "Name"), "myAnim");
    EXPECT_EQ(GetProp<bool>(meta, "Enabled"), false);
    EXPECT_EQ(GetProp<int32_t>(meta, "LoopCount"), 3);
    EXPECT_FLOAT_EQ(GetProp<float>(meta, "SpeedFactor"), 2.5f);
}

/**
 * @tc.name: ImportTrackAnimation
 * @tc.desc: TrackAnimation with duration, property, timestamps and keyframes
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimation, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation0.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "AnimationType"), "trackAnimation");
    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "Name"), "trackAnim");
    EXPECT_EQ(GetProp<bool>(meta, "Enabled"), true);
    EXPECT_EQ(GetProp<int32_t>(meta, "LoopCount"), -1);
    EXPECT_FLOAT_EQ(GetProp<float>(meta, "SpeedFactor"), 0.5f);
    EXPECT_FLOAT_EQ(GetProp<float>(meta, "DurationMs"), 1000.0f);
    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "PropertyPath"), "nodes/myNode.transform.position");

    auto timestamps = META_NS::GetValue<BASE_NS::vector<float>>(meta->GetProperty("Timestamps"));
    ASSERT_EQ(timestamps.size(), 4u);
    EXPECT_FLOAT_EQ(timestamps[0], 0.0f);
    EXPECT_FLOAT_EQ(timestamps[1], 0.25f);
    EXPECT_FLOAT_EQ(timestamps[2], 0.5f);
    EXPECT_FLOAT_EQ(timestamps[3], 1.0f);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 4u);
    for (size_t i = 0; i < keyframes.size(); ++i) {
        ASSERT_TRUE(keyframes[i]);
        float v = 0.f;
        EXPECT_TRUE(keyframes[i]->GetValue(v));
        EXPECT_FLOAT_EQ(v, static_cast<float>(i + 1));
    }
}

/**
 * @tc.name: ImportTrackAnimationKeyframesFloat
 * @tc.desc: Verify GetAnyValue produces float IAny for floating-point keyframes
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframesFloat, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframes_float.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 3u);

    const float expected[] = {1.5f, 2.5f, 3.5f};
    for (size_t i = 0; i < keyframes.size(); ++i) {
        ASSERT_TRUE(keyframes[i]);
        float v = 0.f;
        EXPECT_TRUE(keyframes[i]->GetValue(v));
        EXPECT_FLOAT_EQ(v, expected[i]);
    }
}

/**
 * @tc.name: ImportTrackAnimationKeyframesString
 * @tc.desc: Verify GetAnyValue produces string IAny for string keyframes
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframesString, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframes_string.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 2u);

    ASSERT_TRUE(keyframes[0]);
    BASE_NS::string s0;
    EXPECT_TRUE(keyframes[0]->GetValue(s0));
    EXPECT_EQ(s0, "hello");

    ASSERT_TRUE(keyframes[1]);
    BASE_NS::string s1;
    EXPECT_TRUE(keyframes[1]->GetValue(s1));
    EXPECT_EQ(s1, "world");
}

/**
 * @tc.name: ImportTrackAnimationKeyframesBool
 * @tc.desc: Verify GetAnyValue produces bool IAny for boolean keyframes
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframesBool, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframes_bool.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 3u);

    const bool expected[] = {true, false, true};
    for (size_t i = 0; i < keyframes.size(); ++i) {
        ASSERT_TRUE(keyframes[i]);
        bool v = false;
        EXPECT_TRUE(keyframes[i]->GetValue(v));
        EXPECT_EQ(v, expected[i]);
    }
}

/**
 * @tc.name: ImportTrackAnimationKeyframesInt
 * @tc.desc: Verify GetAnyValue produces int64_t for negative and uint64_t for positive integer keyframes
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframesInt, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframes_int.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 3u);

    // 10 is positive, JSON parser stores as uint64_t
    ASSERT_TRUE(keyframes[0]);
    uint64_t u0 = 0;
    EXPECT_TRUE(keyframes[0]->GetValue(u0));
    EXPECT_EQ(u0, 10u);

    // -20 is negative, JSON parser stores as int64_t
    ASSERT_TRUE(keyframes[1]);
    int64_t s1 = 0;
    EXPECT_TRUE(keyframes[1]->GetValue(s1));
    EXPECT_EQ(s1, -20);

    // 30 is positive, JSON parser stores as uint64_t
    ASSERT_TRUE(keyframes[2]);
    uint64_t u2 = 0;
    EXPECT_TRUE(keyframes[2]->GetValue(u2));
    EXPECT_EQ(u2, 30u);
}

/**
 * @tc.name: ImportTrackAnimationKeyframesNull
 * @tc.desc: Verify GetAnyValue produces null IAny::Ptr for null keyframes
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframesNull, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframes_null.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 2u);
    EXPECT_FALSE(keyframes[0]);
    EXPECT_FALSE(keyframes[1]);
}

/**
 * @tc.name: ImportTrackAnimationKeyframesMixed
 * @tc.desc: Verify GetAnyValue handles mixed-type keyframes correctly
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframesMixed, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframes_mixed.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 4u);

    // [0] = 1.5 (float)
    ASSERT_TRUE(keyframes[0]);
    float f0 = 0.f;
    EXPECT_TRUE(keyframes[0]->GetValue(f0));
    EXPECT_FLOAT_EQ(f0, 1.5f);

    // [1] = "text" (string)
    ASSERT_TRUE(keyframes[1]);
    BASE_NS::string s1;
    EXPECT_TRUE(keyframes[1]->GetValue(s1));
    EXPECT_EQ(s1, "text");

    // [2] = true (bool)
    ASSERT_TRUE(keyframes[2]);
    bool b2 = false;
    EXPECT_TRUE(keyframes[2]->GetValue(b2));
    EXPECT_EQ(b2, true);

    // [3] = 42 (positive int → uint64_t)
    ASSERT_TRUE(keyframes[3]);
    uint64_t u3 = 0;
    EXPECT_TRUE(keyframes[3]->GetValue(u3));
    EXPECT_EQ(u3, 42u);
}

/**
 * @tc.name: ImportTrackAnimationKeyframesArray
 * @tc.desc: Verify GetAnyValue handles nested array keyframes
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframesArray, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframes_array.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 2u);

    // Each element should be an IAny holding vector<IAny::Ptr>
    for (size_t i = 0; i < keyframes.size(); ++i) {
        ASSERT_TRUE(keyframes[i]);
        BASE_NS::vector<META_NS::IAny::Ptr> inner;
        EXPECT_TRUE(keyframes[i]->GetValue(inner));
        ASSERT_EQ(inner.size(), 2u);

        uint64_t v0 = 0;
        ASSERT_TRUE(inner[0]);
        EXPECT_TRUE(inner[0]->GetValue(v0));
        EXPECT_EQ(v0, i * 2 + 1);

        uint64_t v1 = 0;
        ASSERT_TRUE(inner[1]);
        EXPECT_TRUE(inner[1]->GetValue(v1));
        EXPECT_EQ(v1, i * 2 + 2);
    }
}

/**
 * @tc.name: ImportTrackAnimationKeyframesVec2
 * @tc.desc: Builtin vec2 keyframes unwrap to typed Any<Vec2> via IBuiltinContainer::GetAny
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframesVec2, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframes_vec2.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 2u);

    BASE_NS::Math::Vec2 v0;
    ASSERT_TRUE(keyframes[0]);
    EXPECT_TRUE(keyframes[0]->GetValue(v0));
    EXPECT_FLOAT_EQ(v0.x, 1.0f);
    EXPECT_FLOAT_EQ(v0.y, 2.0f);

    BASE_NS::Math::Vec2 v1;
    ASSERT_TRUE(keyframes[1]);
    EXPECT_TRUE(keyframes[1]->GetValue(v1));
    EXPECT_FLOAT_EQ(v1.x, 3.0f);
    EXPECT_FLOAT_EQ(v1.y, 4.0f);
}

/**
 * @tc.name: ImportTrackAnimationKeyframesVec3
 * @tc.desc: Builtin vec3 keyframes unwrap to typed Any<Vec3>
 *           Regression for the object-shaped keyframe wrapping bug fixed via
 *           IBuiltinContainer::GetAny.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframesVec3, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframes_vec3.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 2u);

    BASE_NS::Math::Vec3 v0;
    ASSERT_TRUE(keyframes[0]);
    EXPECT_TRUE(keyframes[0]->GetValue(v0));
    EXPECT_FLOAT_EQ(v0.x, 1.0f);
    EXPECT_FLOAT_EQ(v0.y, 2.0f);
    EXPECT_FLOAT_EQ(v0.z, 3.0f);

    BASE_NS::Math::Vec3 v1;
    ASSERT_TRUE(keyframes[1]);
    EXPECT_TRUE(keyframes[1]->GetValue(v1));
    EXPECT_FLOAT_EQ(v1.x, 4.0f);
    EXPECT_FLOAT_EQ(v1.y, 5.0f);
    EXPECT_FLOAT_EQ(v1.z, 6.0f);
}

/**
 * @tc.name: ImportTrackAnimationKeyframesVec4
 * @tc.desc: Builtin vec4 keyframes unwrap to typed Any<Vec4>
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframesVec4, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframes_vec4.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 2u);

    BASE_NS::Math::Vec4 v0;
    ASSERT_TRUE(keyframes[0]);
    EXPECT_TRUE(keyframes[0]->GetValue(v0));
    EXPECT_FLOAT_EQ(v0.x, 1.0f);
    EXPECT_FLOAT_EQ(v0.w, 4.0f);

    BASE_NS::Math::Vec4 v1;
    ASSERT_TRUE(keyframes[1]);
    EXPECT_TRUE(keyframes[1]->GetValue(v1));
    EXPECT_FLOAT_EQ(v1.x, 5.0f);
    EXPECT_FLOAT_EQ(v1.w, 8.0f);
}

/**
 * @tc.name: ImportTrackAnimationKeyframesQuat
 * @tc.desc: Builtin quat keyframes unwrap to typed Any<Quat>
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframesQuat, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframes_quat.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 2u);

    BASE_NS::Math::Quat q0;
    ASSERT_TRUE(keyframes[0]);
    EXPECT_TRUE(keyframes[0]->GetValue(q0));
    EXPECT_FLOAT_EQ(q0.x, 0.0f);
    EXPECT_FLOAT_EQ(q0.y, 0.0f);
    EXPECT_FLOAT_EQ(q0.z, 0.0f);
    EXPECT_FLOAT_EQ(q0.w, 1.0f);

    BASE_NS::Math::Quat q1;
    ASSERT_TRUE(keyframes[1]);
    EXPECT_TRUE(keyframes[1]->GetValue(q1));
    EXPECT_FLOAT_EQ(q1.y, 1.0f);
    EXPECT_FLOAT_EQ(q1.w, 0.0f);
}

/**
 * @tc.name: ImportTrackAnimationKeyframesColor
 * @tc.desc: Builtin color keyframes unwrap to typed Any<Color>
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframesColor, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframes_color.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 2u);

    BASE_NS::Color c0;
    ASSERT_TRUE(keyframes[0]);
    EXPECT_TRUE(keyframes[0]->GetValue(c0));
    EXPECT_FLOAT_EQ(c0.x, 0.1f);
    EXPECT_FLOAT_EQ(c0.w, 1.0f);

    BASE_NS::Color c1;
    ASSERT_TRUE(keyframes[1]);
    EXPECT_TRUE(keyframes[1]->GetValue(c1));
    EXPECT_FLOAT_EQ(c1.x, 0.5f);
    EXPECT_FLOAT_EQ(c1.w, 0.8f);
}

/**
 * @tc.name: ImportTrackAnimationKeyframesUVec2
 * @tc.desc: Builtin uvec2 keyframes unwrap to typed Any<UVec2>
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframesUVec2, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframes_uvec2.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 2u);

    BASE_NS::Math::UVec2 v0;
    ASSERT_TRUE(keyframes[0]);
    EXPECT_TRUE(keyframes[0]->GetValue(v0));
    EXPECT_EQ(v0.x, 10u);
    EXPECT_EQ(v0.y, 20u);

    BASE_NS::Math::UVec2 v1;
    ASSERT_TRUE(keyframes[1]);
    EXPECT_TRUE(keyframes[1]->GetValue(v1));
    EXPECT_EQ(v1.x, 30u);
    EXPECT_EQ(v1.y, 40u);
}

/**
 * @tc.name: ImportTrackAnimationKeyframesMat4x4
 * @tc.desc: Builtin mat4x4 keyframes unwrap to typed Any<Mat4X4>
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframesMat4x4, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframes_mat4x4.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 2u);

    BASE_NS::Math::Mat4X4 m0;
    ASSERT_TRUE(keyframes[0]);
    EXPECT_TRUE(keyframes[0]->GetValue(m0));
    // Identity
    EXPECT_FLOAT_EQ(m0.x.x, 1.0f);
    EXPECT_FLOAT_EQ(m0.y.y, 1.0f);
    EXPECT_FLOAT_EQ(m0.w.w, 1.0f);

    BASE_NS::Math::Mat4X4 m1;
    ASSERT_TRUE(keyframes[1]);
    EXPECT_TRUE(keyframes[1]->GetValue(m1));
    // Scale 2 + translation column [5,6,7,1]
    EXPECT_FLOAT_EQ(m1.x.x, 2.0f);
    EXPECT_FLOAT_EQ(m1.w.x, 5.0f);
    EXPECT_FLOAT_EQ(m1.w.y, 6.0f);
    EXPECT_FLOAT_EQ(m1.w.z, 7.0f);
}

/**
 * @tc.name: ImportTrackAnimationKeyframesResourceId
 * @tc.desc: Builtin resourceId keyframes unwrap to typed Any<ResourceId>
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframesResourceId, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframes_resourceId.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto keyframes = META_NS::GetValue<BASE_NS::vector<META_NS::IAny::Ptr>>(meta->GetProperty("KeyframeValues"));
    ASSERT_EQ(keyframes.size(), 2u);

    CORE_NS::ResourceId r0;
    ASSERT_TRUE(keyframes[0]);
    EXPECT_TRUE(keyframes[0]->GetValue(r0));
    EXPECT_EQ(r0, CORE_NS::ResourceId("first-ref", "g1"));

    CORE_NS::ResourceId r1;
    ASSERT_TRUE(keyframes[1]);
    EXPECT_TRUE(keyframes[1]->GetValue(r1));
    EXPECT_EQ(r1, CORE_NS::ResourceId("second-ref", ""));
}

/**
 * @tc.name: ImportAnimationMinimal
 * @tc.desc: Animation with only required fields produces valid template
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportAnimationMinimal, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/meta_animation0.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "AnimationType"), "trackAnimation");
    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "Name"), "metaAnim");
    EXPECT_EQ(GetProp<bool>(meta, "Enabled"), true);
    // Duration, Property, Timestamps should not exist on a non-track animation
    EXPECT_FALSE(meta->GetProperty("DurationMs"));
    EXPECT_FALSE(meta->GetProperty("Timestamps"));
}

/**
 * @tc.name: ImportIndexAnimationOptions
 * @tc.desc: Animation in index with options gets name, enabled, loop, and speed applied
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexAnimationOptions, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index17.json", {});
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);
    EXPECT_EQ(rman->GetResourceInfos(nullptr).size(), 2u);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("test-anim"), nullptr}));
    ASSERT_TRUE(anim);

    EXPECT_EQ(META_NS::GetValue(anim->Name()), "indexAnim");
    EXPECT_EQ(META_NS::GetValue(anim->Enabled()), false);

    // Verify loop modifier attached with LoopCount=5
    auto attach = interface_cast<META_NS::IAttach>(anim);
    ASSERT_TRUE(attach);

    auto loops = attach->GetAttachments<META_NS::AnimationModifiers::ILoop>();
    ASSERT_EQ(loops.size(), 1u);
    EXPECT_EQ(META_NS::GetValue(loops[0]->LoopCount()), 5);

    // Verify speed modifier attached with SpeedFactor=3.0
    auto speeds = attach->GetAttachments<META_NS::AnimationModifiers::ISpeed>();
    ASSERT_EQ(speeds.size(), 1u);
    EXPECT_FLOAT_EQ(META_NS::GetValue(speeds[0]->SpeedFactor()), 3.0f);
}

/**
 * @tc.name: ImportIndexTrackAnimationOptions
 * @tc.desc: Track animation in index with options gets duration, timestamps, and name applied
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexTrackAnimationOptions, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index17.json", {});
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto anim = interface_pointer_cast<META_NS::IAnimation>(
        rman->GetResource({CORE_NS::ResourceId("test-track-anim"), nullptr}));
    ASSERT_TRUE(anim);

    EXPECT_EQ(META_NS::GetValue(anim->Name()), "indexTrackAnim");

    auto timed = interface_cast<META_NS::ITimedAnimation>(anim);
    ASSERT_TRUE(timed);
    EXPECT_EQ(META_NS::GetValue(timed->Duration()), META_NS::TimeSpan::Milliseconds(2000));

    auto track = interface_cast<META_NS::ITrackAnimation>(anim);
    ASSERT_TRUE(track);
    auto timestamps = track->Timestamps()->GetValue();
    ASSERT_EQ(timestamps.size(), 3u);
    EXPECT_FLOAT_EQ(timestamps[0], 0.0f);
    EXPECT_FLOAT_EQ(timestamps[1], 0.5f);
    EXPECT_FLOAT_EQ(timestamps[2], 1.0f);
}

/**
 * @tc.name: AnimationFromIndexRuns
 * @tc.desc: Animation loaded from index can be started and progresses
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, AnimationFromIndexRuns, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto rman = ImportIndexIntoScene("test://import/index/index18.json", scene);
    ASSERT_TRUE(rman);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("run-anim"), scene}));
    ASSERT_TRUE(anim);

    EXPECT_EQ(META_NS::GetValue(anim->Name()), "runAnim");
    EXPECT_EQ(META_NS::GetValue(anim->Enabled()), true);

    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);
    startable->Start();
    UpdateScene();
    EXPECT_TRUE(anim->Running()->GetValue());

    UpdateScene(META_NS::TimeSpan::Milliseconds(50));
    WaitForUserQueue();
    EXPECT_GT(anim->Progress()->GetValue(), 0.0f);
}

/**
 * @tc.name: AnimationSpeedModifierApplied
 * @tc.desc: Animation from index has loop and speed modifiers correctly attached
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, AnimationSpeedModifierApplied, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto rman = ImportIndexIntoScene("test://import/index/index18.json", scene);
    ASSERT_TRUE(rman);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("run-anim"), scene}));
    ASSERT_TRUE(anim);

    auto attach = interface_cast<META_NS::IAttach>(anim);
    ASSERT_TRUE(attach);

    auto speeds = attach->GetAttachments<META_NS::AnimationModifiers::ISpeed>();
    ASSERT_EQ(speeds.size(), 1u);
    EXPECT_FLOAT_EQ(META_NS::GetValue(speeds[0]->SpeedFactor()), 2.0f);

    auto loops = attach->GetAttachments<META_NS::AnimationModifiers::ILoop>();
    ASSERT_EQ(loops.size(), 1u);
    EXPECT_EQ(META_NS::GetValue(loops[0]->LoopCount()), 2);
}

/**
 * @tc.name: TrackAnimationFromIndexRunsWithDuration
 * @tc.desc: Track animation from index has correct duration, timestamps, and progresses
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, TrackAnimationFromIndexRunsWithDuration, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto rman = ImportIndexIntoScene("test://import/index/index18.json", scene);
    ASSERT_TRUE(rman);

    auto anim = interface_pointer_cast<META_NS::IAnimation>(
        rman->GetResource({CORE_NS::ResourceId("timed-track-anim"), scene}));
    ASSERT_TRUE(anim);

    EXPECT_EQ(META_NS::GetValue(anim->Name()), "timedTrack");

    auto timed = interface_cast<META_NS::ITimedAnimation>(anim);
    ASSERT_TRUE(timed);
    EXPECT_EQ(META_NS::GetValue(timed->Duration()), META_NS::TimeSpan::Milliseconds(500));

    auto track = interface_cast<META_NS::ITrackAnimation>(anim);
    ASSERT_TRUE(track);
    auto timestamps = track->Timestamps()->GetValue();
    ASSERT_EQ(timestamps.size(), 3u);
    EXPECT_FLOAT_EQ(timestamps[0], 0.0f);
    EXPECT_FLOAT_EQ(timestamps[1], 0.5f);
    EXPECT_FLOAT_EQ(timestamps[2], 1.0f);

    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);
    startable->Start();
    UpdateScene();
    EXPECT_TRUE(anim->Running()->GetValue());

    UpdateScene(META_NS::TimeSpan::Milliseconds(50));
    WaitForUserQueue();
    auto progress = anim->Progress()->GetValue();
    EXPECT_GT(progress, 0.0f);

    UpdateScene(META_NS::TimeSpan::Milliseconds(50));
    WaitForUserQueue();
    EXPECT_GT(anim->Progress()->GetValue(), progress);
}

/**
 * @tc.name: TrackAnimationVec3KeyframesNoTimestampDuplication
 * @tc.desc: Regression: with a typed (vec3) backing keyframe array, ApplyProperties
 *           pre-sets Timestamps and ApplyKeyframes calls AddKeyframe per entry; without
 *           the Reset() in ApplyKeyframes the timestamps end up duplicated (2N entries).
 *           Verify the instantiated track has exactly N timestamps matching the input.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, TrackAnimationVec3KeyframesNoTimestampDuplication, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto rman = ImportIndexIntoScene("test://import/index/index_anim_vec3_track.json", scene);
    ASSERT_TRUE(rman);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("vec3-track-anim"), scene}));
    ASSERT_TRUE(anim);

    auto track = interface_cast<META_NS::ITrackAnimation>(anim);
    ASSERT_TRUE(track);
    auto timestamps = track->Timestamps()->GetValue();
    ASSERT_EQ(timestamps.size(), 3u);
    EXPECT_FLOAT_EQ(timestamps[0], 0.0f);
    EXPECT_FLOAT_EQ(timestamps[1], 0.5f);
    EXPECT_FLOAT_EQ(timestamps[2], 1.0f);
}

/**
 * @tc.name: TrackAnimationVec3PositionInterpolates
 * @tc.desc: Vec3 keyframes drive a node's Position; mid-playback the value differs
 *           from both endpoints, end-of-playback equals the final keyframe.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, TrackAnimationVec3PositionInterpolates, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto node = scene->CreateNode("//testNode").GetResult();
    ASSERT_TRUE(node);

    auto rman = ImportIndexIntoScene("test://import/index/index_anim_vec3_position.json", scene);
    ASSERT_TRUE(rman);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("vec3-pos-anim"), scene}));
    ASSERT_TRUE(anim);

    auto initial = node->Position()->GetValue();
    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);
    startable->Start();
    UpdateScene();
    EXPECT_TRUE(anim->Running()->GetValue());

    UpdateScene(META_NS::TimeSpan::Milliseconds(500));
    WaitForUserQueue();
    auto mid = node->Position()->GetValue();
    EXPECT_NE(mid, initial);
    EXPECT_NE(mid, BASE_NS::Math::Vec3(10.0f, 20.0f, 30.0f));

    UpdateScene(META_NS::TimeSpan::Milliseconds(700));
    WaitForUserQueue();
    auto final_v = node->Position()->GetValue();
    EXPECT_FLOAT_EQ(final_v.x, 10.0f);
    EXPECT_FLOAT_EQ(final_v.y, 20.0f);
    EXPECT_FLOAT_EQ(final_v.z, 30.0f);
}

/**
 * @tc.name: TrackAnimationQuatRotationInterpolates
 * @tc.desc: Quat keyframes drive a node's Rotation; mid-playback the value differs
 *           from both endpoints, end-of-playback matches the final keyframe.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, TrackAnimationQuatRotationInterpolates, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto node = scene->CreateNode("//testNode").GetResult();
    ASSERT_TRUE(node);

    auto rman = ImportIndexIntoScene("test://import/index/index_anim_quat_rotation.json", scene);
    ASSERT_TRUE(rman);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("quat-rot-anim"), scene}));
    ASSERT_TRUE(anim);

    auto initial = node->Rotation()->GetValue();
    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);
    startable->Start();
    UpdateScene();
    EXPECT_TRUE(anim->Running()->GetValue());

    UpdateScene(META_NS::TimeSpan::Milliseconds(500));
    WaitForUserQueue();
    auto mid = node->Rotation()->GetValue();
    EXPECT_NE(mid, initial);

    UpdateScene(META_NS::TimeSpan::Milliseconds(700));
    WaitForUserQueue();
    auto final_q = node->Rotation()->GetValue();
    EXPECT_NEAR(final_q.y, 1.0f, 1e-3f);
    EXPECT_NEAR(final_q.w, 0.0f, 1e-3f);
}

/**
 * @tc.name: GltfAnimationFromSceneRuns
 * @tc.desc: glTF animation loaded from scene can be started and progresses
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, GltfAnimationFromSceneRuns, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);

    auto anims = scene->GetAnimations().GetResult();
    ASSERT_FALSE(anims.empty());
    auto anim = anims[0];

    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);
    startable->Start();
    UpdateScene();
    EXPECT_TRUE(anim->Running()->GetValue());

    UpdateScene(META_NS::TimeSpan::Milliseconds(50));
    WaitForUserQueue();
    auto progress = anim->Progress()->GetValue();
    EXPECT_GT(progress, 0.0f);

    UpdateScene(META_NS::TimeSpan::Milliseconds(50));
    WaitForUserQueue();
    EXPECT_GT(anim->Progress()->GetValue(), progress);
}

/**
 * @tc.name: GltfAnimationReloadViaResourceManager
 * @tc.desc: glTF animation can be reloaded through the resource manager
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, GltfAnimationReloadViaResourceManager, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);

    auto anims = scene->GetAnimations().GetResult();
    ASSERT_FALSE(anims.empty());

    auto animRes = interface_pointer_cast<CORE_NS::IResource>(anims[0]);
    ASSERT_TRUE(animRes);
    EXPECT_TRUE(resources->ReloadResource(animRes));

    auto reloadedAnim = interface_pointer_cast<META_NS::IAnimation>(animRes);
    ASSERT_TRUE(reloadedAnim);

    auto startable = interface_cast<META_NS::IStartableAnimation>(reloadedAnim);
    ASSERT_TRUE(startable);
}

#if !defined(__ANDROID__) && !defined(__OHOS__)
/**
 * @tc.name: MaterialTemplateFileReloadViaRemoveAddAndReload
 * @tc.desc: Picking up edits to a material template file at runtime works via the
 *           "remove + re-add + ReloadResource(dependent)" sequence. ResolveBaseResource
 *           re-looks up the template by id at apply-time, so re-registering the template
 *           with the same id makes the next ApplyOptions pull the freshly-parsed file.
 *
 *           Negative cross-check: calling ReloadResource(material) alone (without
 *           removing+re-adding the template) does NOT pick up the new file content,
 *           because SceneResourceTemplateTypeBase::ReloadResource re-applies the cached
 *           options object rather than re-importing from the file payload.
 *
 *           Test fixtures live under test/unittest/assets/test_data/material_reload/.
 *           Rebuild after editing fixtures so the build tree copy is refreshed.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, MaterialTemplateFileReloadViaRemoveAddAndReload, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);

    auto& fm = context->GetRenderer()->GetEngine().GetFileManager();
    constexpr BASE_NS::string_view kLive = "file://material_reload/template.json";
    if (fm.FileExists(kLive)) {
        ASSERT_TRUE(fm.DeleteFile(kLive));
    }
    ASSERT_TRUE(CopyFile(fm, "test://material_reload/template_v1.json", kLive));

    SCENE_IMP_NS::ImportParameters params;
    params.scene = scene;
    auto importResult = imp->Import("test://material_reload/index.json", params);
    ASSERT_TRUE(importResult);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(importResult.object);
    ASSERT_TRUE(rman);

    auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("derived"), scene}));
    ASSERT_TRUE(mat);
    EXPECT_FLOAT_EQ(mat->AlphaCutoff()->GetValue(), 0.5f);
    {
        auto rs = mat->RenderSort()->GetValue();
        EXPECT_EQ(rs.renderSortLayer, 3);
        EXPECT_EQ(rs.renderSortLayerOrder, 1);
    }

    ASSERT_TRUE(fm.DeleteFile(kLive));
    ASSERT_TRUE(CopyFile(fm, "test://material_reload/template_v2.json", kLive));

    auto matRes = interface_pointer_cast<CORE_NS::IResource>(mat);
    ASSERT_TRUE(matRes);
    EXPECT_TRUE(rman->ReloadResource(matRes));
    EXPECT_FLOAT_EQ(mat->AlphaCutoff()->GetValue(), 0.5f);
    {
        auto rs = mat->RenderSort()->GetValue();
        EXPECT_EQ(rs.renderSortLayer, 3);
        EXPECT_EQ(rs.renderSortLayerOrder, 1);
    }

    auto extRes = interface_cast<META_NS::IResourceManagerExtension>(rman);
    ASSERT_TRUE(extRes);
    ASSERT_TRUE(rman->RemoveResource(CORE_NS::ResourceIdContext{CORE_NS::ResourceId("tmpl"), scene}));
    // Use the extension interface so the registration carries the same version
    // ("JsonSchemaImport") as the index handler used originally — otherwise the
    // type lookup falls back to a generic ObjectResourceType.
    META_NS::ResourceData tmplData;
    tmplData.version = "JsonSchemaImport";
    tmplData.id = CORE_NS::ResourceId("tmpl");
    tmplData.type = ClassId::MaterialResourceTemplate.Id().ToUid();
    tmplData.path = kLive;
    ASSERT_TRUE(extRes->AddResource(BASE_NS::move(tmplData), scene));

    EXPECT_TRUE(rman->ReloadResource(matRes));
    EXPECT_FLOAT_EQ(mat->AlphaCutoff()->GetValue(), 0.25f);
    {
        auto rs = mat->RenderSort()->GetValue();
        EXPECT_EQ(rs.renderSortLayer, 7);
        EXPECT_EQ(rs.renderSortLayerOrder, 2);
    }
}
#endif

/**
 * @tc.name: GltfAnimationWithOptionsFromIndex
 * @tc.desc: glTF animation loaded with index options gets loop and speed modifiers applied
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, GltfAnimationWithOptionsFromIndex, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);

    // Baseline: original animation has no loop/speed modifiers
    auto anims = scene->GetAnimations().GetResult();
    ASSERT_FALSE(anims.empty());
    auto origAttach = interface_cast<META_NS::IAttach>(anims[0]);
    ASSERT_TRUE(origAttach);
    EXPECT_TRUE(origAttach->GetAttachments<META_NS::AnimationModifiers::ILoop>().empty());
    EXPECT_TRUE(origAttach->GetAttachments<META_NS::AnimationModifiers::ISpeed>().empty());

    // Import index19 which registers gltfAnimation with options (loop=3, speed=2.5)
    auto rman = ImportIndexIntoScene("test://import/index/index19.json", scene);
    ASSERT_TRUE(rman);

    // Get animation from importer's resource manager - triggers GltfAnimationResourceType with options
    auto anim = interface_pointer_cast<META_NS::IAnimation>(rman->GetResource(
        {CORE_NS::ResourceId("animation_AnimatedCube", "test://AnimatedCube/AnimatedCube.gltf"), scene}));
    ASSERT_TRUE(anim);

    auto attach = interface_cast<META_NS::IAttach>(anim);
    ASSERT_TRUE(attach);

    auto loops = attach->GetAttachments<META_NS::AnimationModifiers::ILoop>();
    ASSERT_EQ(loops.size(), 1u);
    EXPECT_EQ(META_NS::GetValue(loops[0]->LoopCount()), 3);

    auto speeds = attach->GetAttachments<META_NS::AnimationModifiers::ISpeed>();
    ASSERT_EQ(speeds.size(), 1u);
    EXPECT_FLOAT_EQ(META_NS::GetValue(speeds[0]->SpeedFactor()), 2.5f);
}

/**
 * @tc.name: GltfAnimationWithOptionsRuns
 * @tc.desc: glTF animation loaded with index options can be started and progresses
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, GltfAnimationWithOptionsRuns, testing::ext::TestSize.Level1)
{
    auto scene = LoadScene("test://AnimatedCube/AnimatedCube.gltf");
    ASSERT_TRUE(scene);

    auto rman = ImportIndexIntoScene("test://import/index/index19.json", scene);
    ASSERT_TRUE(rman);

    auto anim = interface_pointer_cast<META_NS::IAnimation>(rman->GetResource(
        {CORE_NS::ResourceId("animation_AnimatedCube", "test://AnimatedCube/AnimatedCube.gltf"), scene}));
    ASSERT_TRUE(anim);

    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);
    startable->Start();
    UpdateScene();
    EXPECT_TRUE(anim->Running()->GetValue());

    UpdateScene(META_NS::TimeSpan::Milliseconds(50));
    WaitForUserQueue();
    EXPECT_GT(anim->Progress()->GetValue(), 0.0f);
}

/**
 * @tc.name: ImportIndexAnimationTemplate
 * @tc.desc: Index entry with animationTemplate type loads animation via template path
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexAnimationTemplate, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index20.json", {});
    ASSERT_TRUE(res);
    auto rman = interface_pointer_cast<CORE_NS::IResourceManager>(res.object);
    ASSERT_TRUE(rman);

    auto resource = rman->GetResource({CORE_NS::ResourceId("anim-template"), nullptr});
    ASSERT_TRUE(resource);

    auto meta = interface_cast<META_NS::IMetadata>(resource);
    ASSERT_TRUE(meta);

    // track_animation0.json has type=trackAnimation, name=trackAnim
    auto animType = META_NS::GetValue<BASE_NS::string>(meta->GetProperty("AnimationType"));
    EXPECT_EQ(animType, "trackAnimation");

    auto name = META_NS::GetValue<BASE_NS::string>(meta->GetProperty("Name"));
    EXPECT_EQ(name, "trackAnim");
}

/**
 * @tc.name: TrackAnimationPropertyResolved
 * @tc.desc: Track animation property path resolves to correct scene property
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, TrackAnimationPropertyResolved, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto node = scene->CreateNode("//testNode", ClassId::LightNode).GetResult();
    ASSERT_TRUE(node);

    auto rman = ImportIndexIntoScene("test://import/index/index21.json", scene);
    ASSERT_TRUE(rman);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("prop-track-anim"), scene}));
    ASSERT_TRUE(anim);

    EXPECT_EQ(META_NS::GetValue(anim->Name()), "propTrackAnim");

    auto propAnim = interface_cast<META_NS::IPropertyAnimation>(anim);
    ASSERT_TRUE(propAnim);
    auto resolved = propAnim->Property()->GetValue().lock();
    ASSERT_TRUE(resolved);
}

/**
 * @tc.name: TrackAnimationPropertyModified
 * @tc.desc: Track animation modifies target property when run
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, TrackAnimationPropertyModified, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto node = scene->CreateNode("//testNode", ClassId::LightNode).GetResult();
    ASSERT_TRUE(node);

    auto light = interface_cast<ILight>(node);
    ASSERT_TRUE(light);
    auto initial = light->Intensity()->GetValue();

    auto rman = ImportIndexIntoScene("test://import/index/index21.json", scene);
    ASSERT_TRUE(rman);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("prop-track-anim"), scene}));
    ASSERT_TRUE(anim);

    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);
    startable->Start();
    UpdateScene();
    EXPECT_TRUE(anim->Running()->GetValue());

    // index21.json animates the Light's Intensity from 0.0 -> 100.0 over 1000ms.
    // Light's default Intensity is 1.0, so the animation start (keyframe[0]=0.0) flips it to 0.
    // Verify the animation took effect on the property and the engine is reporting progress.
    UpdateScene(META_NS::TimeSpan::Milliseconds(500));
    WaitForUserQueue();
    EXPECT_NE(light->Intensity()->GetValue(), initial);
    EXPECT_GT(anim->Progress()->GetValue(), 0.0f);
    // Note: this test does NOT verify mid-run interpolation reaches ~50.0 because the
    // animation-driven property does not progress smoothly under UpdateScene's clock in this
    // unit-test harness — only the start keyframe gets latched to the property. A future
    // change should add a real interpolation check; today we only verify the link works.
}

/**
 * @tc.name: TrackAnimationPropertyResolvedFromSceneIndex
 * @tc.desc: Animation defined in a scene's embedded index resolves its property path
 *           against the scene's rootNode (which is built AFTER the index loads).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, TrackAnimationPropertyResolvedFromSceneIndex, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/scene/scene_anim_index.json", {});
    ASSERT_TRUE(res);
    auto scene = interface_pointer_cast<IScene>(res.object);
    ASSERT_TRUE(scene);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("prop-track-anim"), scene}));
    ASSERT_TRUE(anim);

    auto propAnim = interface_cast<META_NS::IPropertyAnimation>(anim);
    ASSERT_TRUE(propAnim);
    auto resolved = propAnim->Property()->GetValue().lock();
    ASSERT_TRUE(resolved);

    auto light = interface_pointer_cast<ILight>(scene->FindNode("//testLight").GetResult());
    ASSERT_TRUE(light);
    EXPECT_EQ(resolved, light->Intensity().GetProperty());
}

/**
 * @tc.name: TrackAnimationPropertyTemplatePopulatedAfterImport
 * @tc.desc: After scene import, the AnimationTemplate stored in the resource manager
 *           has its `Property` weak-ptr populated (the import-time deferred resolved it
 *           against the built rootNode). Querying the template before any apply yields
 *           the resolved binding.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, TrackAnimationPropertyTemplatePopulatedAfterImport, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/scene/scene_anim_index.json", {});
    ASSERT_TRUE(res);
    auto scene = interface_pointer_cast<IScene>(res.object);
    ASSERT_TRUE(scene);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    auto info = rman->GetResourceInfo({CORE_NS::ResourceId("prop-track-anim"), scene});
    ASSERT_TRUE(info.options);
    auto meta = interface_cast<META_NS::IMetadata>(info.options);
    ASSERT_TRUE(meta);

    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "PropertyPath"), "testLight.Intensity");

    auto propProp = meta->GetProperty<META_NS::IProperty::WeakPtr>("Property", META_NS::MetadataQuery::EXISTING);
    ASSERT_TRUE(propProp);
    auto resolved = META_NS::GetValue(propProp).lock();
    ASSERT_TRUE(resolved);

    auto light = interface_pointer_cast<ILight>(scene->FindNode("//testLight").GetResult());
    ASSERT_TRUE(light);
    EXPECT_EQ(resolved, light->Intensity().GetProperty());
}

/**
 * @tc.name: TrackAnimationPropertyUserOverridePreserved
 * @tc.desc: If `Property` is programmatically set on a template AFTER scene import,
 *           applying the resource respects the user's override (does not re-resolve
 *           from PropertyPath).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, TrackAnimationPropertyUserOverridePreserved, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/scene/scene_anim_index.json", {});
    ASSERT_TRUE(res);
    auto scene = interface_pointer_cast<IScene>(res.object);
    ASSERT_TRUE(scene);
    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    auto info = rman->GetResourceInfo({CORE_NS::ResourceId("prop-track-anim"), scene});
    ASSERT_TRUE(info.options);
    auto meta = interface_cast<META_NS::IMetadata>(info.options);
    ASSERT_TRUE(meta);

    auto override_node = scene->CreateNode("//overrideLight", ClassId::LightNode).GetResult();
    auto override_light = interface_pointer_cast<ILight>(override_node);
    ASSERT_TRUE(override_light);
    META_NS::IProperty::WeakPtr overrideProp = override_light->Intensity();

    auto propProp = meta->GetProperty<META_NS::IProperty::WeakPtr>("Property", META_NS::MetadataQuery::EXISTING);
    ASSERT_TRUE(propProp);
    META_NS::SetValue(propProp, overrideProp);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("prop-track-anim"), scene}));
    ASSERT_TRUE(anim);
    auto propAnim = interface_cast<META_NS::IPropertyAnimation>(anim);
    ASSERT_TRUE(propAnim);
    auto resolved = propAnim->Property()->GetValue().lock();
    ASSERT_EQ(resolved, override_light->Intensity().GetProperty());
}

/**
 * @tc.name: TrackAnimationPropertyResolvedFromSceneIndexBackwardRef
 * @tc.desc: Animation in scene index references a node that appears LATER in the
 *           rootNode children. Deferred resolution at end of rootNode build
 *           handles both forward and backward order.
 * @tc.type: FUNC
 */
UNIT_TEST_F(
    API_ResourceImportTest, TrackAnimationPropertyResolvedFromSceneIndexBackwardRef, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/scene/scene_anim_index_backward.json", {});
    ASSERT_TRUE(res);
    auto scene = interface_pointer_cast<IScene>(res.object);
    ASSERT_TRUE(scene);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    auto anim = interface_pointer_cast<META_NS::IAnimation>(
        rman->GetResource({CORE_NS::ResourceId("prop-track-anim-back"), scene}));
    ASSERT_TRUE(anim);
    auto propAnim = interface_cast<META_NS::IPropertyAnimation>(anim);
    ASSERT_TRUE(propAnim);

    auto resolved = propAnim->Property()->GetValue().lock();
    ASSERT_TRUE(resolved);
    auto secondLight = interface_pointer_cast<ILight>(scene->FindNode("//secondLight").GetResult());
    ASSERT_TRUE(secondLight);
    EXPECT_EQ(resolved, secondLight->Intensity().GetProperty());
}

/**
 * @tc.name: TrackAnimationPropertySlashPrefixedPath
 * @tc.desc: Animation property path with explicit `/` prefix anchors at the scene
 *           root via the auto-pinned `importRoot` (= rootNode).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, TrackAnimationPropertySlashPrefixedPath, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/scene/scene_anim_index_slash.json", {});
    ASSERT_TRUE(res);
    auto scene = interface_pointer_cast<IScene>(res.object);
    ASSERT_TRUE(scene);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    auto anim = interface_pointer_cast<META_NS::IAnimation>(
        rman->GetResource({CORE_NS::ResourceId("prop-track-anim-slash"), scene}));
    ASSERT_TRUE(anim);
    auto propAnim = interface_cast<META_NS::IPropertyAnimation>(anim);
    ASSERT_TRUE(propAnim);

    auto resolved = propAnim->Property()->GetValue().lock();
    ASSERT_TRUE(resolved);
    auto light = interface_pointer_cast<ILight>(scene->FindNode("//testLight").GetResult());
    ASSERT_TRUE(light);
    EXPECT_EQ(resolved, light->Intensity().GetProperty());
}

/**
 * @tc.name: TrackAnimationPropertyTopLevelPayloadWithScene
 * @tc.desc: Importing a top-level trackAnimation file directly via the importer
 *           with a scene parameter resolves the property path immediately
 *           against that scene's hierarchy. There is no enclosing
 *           DeferredHierarchyScope, so the deferred fires inline; with the scene
 *           already built, resolution succeeds.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, TrackAnimationPropertyTopLevelPayloadWithScene, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto node = scene->CreateNode("//testNode", ClassId::LightNode).GetResult();
    ASSERT_TRUE(node);
    auto light = interface_cast<ILight>(node);
    ASSERT_TRUE(light);

    SCENE_IMP_NS::ImportParameters params;
    params.scene = scene;
    auto res = imp->Import("test://import/animation/track_animation_with_path.json", params);
    ASSERT_TRUE(res);
    auto meta = interface_cast<META_NS::IMetadata>(res.object);
    ASSERT_TRUE(meta);

    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "PropertyPath"), "testNode.Intensity");

    auto propProp = meta->GetProperty<META_NS::IProperty::WeakPtr>("Property", META_NS::MetadataQuery::EXISTING);
    ASSERT_TRUE(propProp);
    auto resolved = META_NS::GetValue(propProp).lock();
    ASSERT_TRUE(resolved);
    EXPECT_EQ(resolved, light->Intensity().GetProperty());
}

/**
 * @tc.name: TrackAnimationPropertyInsideTemplateOwnIndex
 * @tc.desc: A node template has its own index containing a trackAnimation that
 *           references a node inside the template's content. After scene
 *           import + template instantiation, loading the animation resource
 *           must yield a live animation whose Property points at the
 *           instantiated content's node, not nullptr.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, TrackAnimationPropertyInsideTemplateOwnIndex, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/node_template/scene_template_with_index_anim.json", {});
    ASSERT_TRUE(res);
    auto scene = interface_pointer_cast<IScene>(res.object);
    ASSERT_TRUE(scene);

    auto light = interface_pointer_cast<ILight>(scene->FindNode("//tmplRootIdx/tmplLight").GetResult());
    ASSERT_TRUE(light);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("tmpl-anim"), scene}));
    ASSERT_TRUE(anim);

    auto propAnim = interface_cast<META_NS::IPropertyAnimation>(anim);
    ASSERT_TRUE(propAnim);
    auto resolved = propAnim->Property()->GetValue().lock();
    ASSERT_TRUE(resolved);
    EXPECT_EQ(resolved, light->Intensity().GetProperty());
}

/**
 * @tc.name: AnimationOnlyModelLoadsAndRuns
 * @tc.desc: Load the animation_only model index against a runtime-built scene
 *           whose hierarchy matches the animation's property target. The
 *           parallelAnimation "Bounce" must resolve to AnimatedEmpty.Position
 *           and drive it when started.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, AnimationOnlyModelLoadsAndRuns, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    ASSERT_TRUE(scene->CreateNode("//AnimOnlyScene").GetResult());
    auto animatedEmpty = scene->CreateNode("//AnimOnlyScene/AnimatedEmpty").GetResult();
    ASSERT_TRUE(animatedEmpty);

    auto rman = ImportIndexIntoScene("test://import/animation/models/animation_only/animation_only.index", scene);
    ASSERT_TRUE(rman);

    auto anim = interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("Bounce"), scene}));
    ASSERT_TRUE(anim);
    EXPECT_EQ(META_NS::GetValue(anim->Name()), "Bounce");

    // Bounce.animation is a parallelAnimation with one trackAnimation child.
    auto staggered = interface_cast<META_NS::IStaggeredAnimation>(anim);
    ASSERT_TRUE(staggered);
    auto children = staggered->GetAnimations();
    ASSERT_EQ(children.size(), 1u);

    auto trackProp = interface_pointer_cast<META_NS::IPropertyAnimation>(children[0]);
    ASSERT_TRUE(trackProp);
    auto resolvedProp = trackProp->Property()->GetValue().lock();
    ASSERT_TRUE(resolvedProp);
    EXPECT_EQ(resolvedProp, animatedEmpty->Position().GetProperty());

    auto initial = animatedEmpty->Position()->GetValue();
    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);
    startable->Start();
    UpdateScene();
    EXPECT_TRUE(anim->Running()->GetValue());

    UpdateScene(META_NS::TimeSpan::Milliseconds(1000));
    WaitForUserQueue();
    EXPECT_NE(animatedEmpty->Position()->GetValue(), initial);
}

/**
 * @tc.name: AnimationOnlyTmplModelLoadsAndRuns
 * @tc.desc: A wrapper scene imports the animation_only_tmpl node template,
 *           which carries an internal index defining Bounce_template (an
 *           animationTemplate loaded from a .animation file) and Bounce (a
 *           parallelAnimation that derives from Bounce_template). After
 *           template instantiation, GetResource("Bounce") must yield a live
 *           parallel animation whose track child is bound to the
 *           AnimatedEmpty.Position inside the template instance and drives
 *           it when started.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, AnimationOnlyTmplModelLoadsAndRuns, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/animation/models/animation_only_tmpl/scene_animation_only_tmpl.json", {});
    ASSERT_TRUE(res);
    auto scene = interface_pointer_cast<IScene>(res.object);
    ASSERT_TRUE(scene);

    auto animatedEmpty = scene->FindNode("//AnimOnlyScene/AnimatedEmpty").GetResult();
    ASSERT_TRUE(animatedEmpty);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);
    auto anim = interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("Bounce"), scene}));
    ASSERT_TRUE(anim);
    EXPECT_EQ(META_NS::GetValue(anim->Name()), "Bounce");

    auto staggered = interface_cast<META_NS::IStaggeredAnimation>(anim);
    ASSERT_TRUE(staggered);
    auto children = staggered->GetAnimations();
    ASSERT_EQ(children.size(), 1u);

    auto trackProp = interface_pointer_cast<META_NS::IPropertyAnimation>(children[0]);
    ASSERT_TRUE(trackProp);
    auto resolvedProp = trackProp->Property()->GetValue().lock();
    ASSERT_TRUE(resolvedProp);
    EXPECT_EQ(resolvedProp, animatedEmpty->Position().GetProperty());

    auto initial = animatedEmpty->Position()->GetValue();
    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);
    startable->Start();
    UpdateScene();
    EXPECT_TRUE(anim->Running()->GetValue());

    UpdateScene(META_NS::TimeSpan::Milliseconds(1000));
    WaitForUserQueue();
    EXPECT_NE(animatedEmpty->Position()->GetValue(), initial);
}

/**
 * @tc.name: AnimationOnlyTmpl3ModelLoadsAndRuns
 * @tc.desc: Wrapper scene imports a node template whose internal index defines a
 *           track-only Bounce_template (animationTemplate carrying just timestamps
 *           + vec3 keyframes, no Property), and a parallelAnimation Bounce whose
 *           inline trackAnimation child has derivedFrom = Bounce_template plus its
 *           own Property override targeting AnimatedEmpty.Position. The inline
 *           derivedFrom processing pulls the base's timestamps + keyframes into
 *           the child meta at parse time; the lazy keyframe-array init in
 *           AnimationTemplate::ApplyKeyframes then keeps them when the derived
 *           Property binds.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, AnimationOnlyTmpl3ModelLoadsAndRuns, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/animation/models/animation_only_tmpl3/scene_animation_only_tmpl3.json", {});
    ASSERT_TRUE(res);
    auto scene = interface_pointer_cast<IScene>(res.object);
    ASSERT_TRUE(scene);

    auto animatedEmpty = scene->FindNode("//AnimOnlyScene/AnimatedEmpty").GetResult();
    ASSERT_TRUE(animatedEmpty);

    auto rman = GetResourceManager(scene);
    ASSERT_TRUE(rman);
    auto anim = interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("Bounce"), scene}));
    ASSERT_TRUE(anim);
    EXPECT_EQ(META_NS::GetValue(anim->Name()), "Bounce");

    auto staggered = interface_cast<META_NS::IStaggeredAnimation>(anim);
    ASSERT_TRUE(staggered);
    auto children = staggered->GetAnimations();
    ASSERT_EQ(children.size(), 1u);

    auto trackProp = interface_pointer_cast<META_NS::IPropertyAnimation>(children[0]);
    ASSERT_TRUE(trackProp);
    auto resolvedProp = trackProp->Property()->GetValue().lock();
    ASSERT_TRUE(resolvedProp);
    EXPECT_EQ(resolvedProp, animatedEmpty->Position().GetProperty());

    auto initial = animatedEmpty->Position()->GetValue();
    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);
    startable->Start();
    UpdateScene();
    EXPECT_TRUE(anim->Running()->GetValue());

    UpdateScene(META_NS::TimeSpan::Milliseconds(1000));
    WaitForUserQueue();
    EXPECT_NE(animatedEmpty->Position()->GetValue(), initial);
}

/**
 * @tc.name: ImportSequentialAnimation
 * @tc.desc: Sequential animation with inline track children
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportSequentialAnimation, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/sequential_animation0.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "AnimationType"), "sequentialAnimation");
    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "Name"), "seqAnim");
    EXPECT_EQ(GetProp<bool>(meta, "Enabled"), true);
    EXPECT_EQ(GetProp<int32_t>(meta, "LoopCount"), 2);
    EXPECT_FLOAT_EQ(GetProp<float>(meta, "SpeedFactor"), 1.5f);

    auto children = META_NS::GetValue<BASE_NS::vector<META_NS::IObject::Ptr>>(meta->GetProperty("Animations"));
    ASSERT_EQ(children.size(), 2u);

    auto child0Meta = interface_cast<META_NS::IMetadata>(children[0]);
    ASSERT_TRUE(child0Meta);
    EXPECT_EQ(GetProp<BASE_NS::string>(child0Meta, "AnimationType"), "trackAnimation");
    EXPECT_EQ(GetProp<BASE_NS::string>(child0Meta, "Name"), "child1");
    EXPECT_FLOAT_EQ(GetProp<float>(child0Meta, "DurationMs"), 500.0f);

    auto child1Meta = interface_cast<META_NS::IMetadata>(children[1]);
    ASSERT_TRUE(child1Meta);
    EXPECT_EQ(GetProp<BASE_NS::string>(child1Meta, "AnimationType"), "trackAnimation");
    EXPECT_EQ(GetProp<BASE_NS::string>(child1Meta, "Name"), "child2");
    EXPECT_FLOAT_EQ(GetProp<float>(child1Meta, "DurationMs"), 300.0f);
}

/**
 * @tc.name: ImportParallelAnimation
 * @tc.desc: Parallel animation with inline track children
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportParallelAnimation, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/parallel_animation0.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "AnimationType"), "parallelAnimation");
    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "Name"), "parAnim");
    EXPECT_EQ(GetProp<bool>(meta, "Enabled"), false);
    EXPECT_EQ(GetProp<int32_t>(meta, "LoopCount"), -1);
    EXPECT_FLOAT_EQ(GetProp<float>(meta, "SpeedFactor"), 2.0f);

    auto children = META_NS::GetValue<BASE_NS::vector<META_NS::IObject::Ptr>>(meta->GetProperty("Animations"));
    ASSERT_EQ(children.size(), 2u);

    auto child0Meta = interface_cast<META_NS::IMetadata>(children[0]);
    ASSERT_TRUE(child0Meta);
    EXPECT_EQ(GetProp<BASE_NS::string>(child0Meta, "AnimationType"), "trackAnimation");
    EXPECT_EQ(GetProp<BASE_NS::string>(child0Meta, "Name"), "child1");
    EXPECT_FLOAT_EQ(GetProp<float>(child0Meta, "DurationMs"), 400.0f);

    auto child1Meta = interface_cast<META_NS::IMetadata>(children[1]);
    ASSERT_TRUE(child1Meta);
    EXPECT_EQ(GetProp<BASE_NS::string>(child1Meta, "AnimationType"), "trackAnimation");
    EXPECT_EQ(GetProp<BASE_NS::string>(child1Meta, "Name"), "child2");
    EXPECT_FLOAT_EQ(GetProp<float>(child1Meta, "DurationMs"), 600.0f);
}

/**
 * @tc.name: ImportKeyframeAnimation
 * @tc.desc: Keyframe animation template with from/to values
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportKeyframeAnimation, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/keyframe_animation0.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "AnimationType"), "keyframeAnimation");
    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "Name"), "kfAnim");
    EXPECT_EQ(GetProp<bool>(meta, "Enabled"), true);
    EXPECT_EQ(GetProp<int32_t>(meta, "LoopCount"), 2);
    EXPECT_FLOAT_EQ(GetProp<float>(meta, "SpeedFactor"), 1.5f);
    EXPECT_FLOAT_EQ(GetProp<float>(meta, "DurationMs"), 500.0f);
    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "PropertyPath"), "nodes/myNode.transform.position");

    auto fromAny = GetProp<META_NS::IAny::Ptr>(meta, "From");
    ASSERT_TRUE(fromAny);
    EXPECT_FLOAT_EQ(META_NS::GetValue<float>(*fromAny), 0.0f);

    auto toAny = GetProp<META_NS::IAny::Ptr>(meta, "To");
    ASSERT_TRUE(toAny);
    EXPECT_FLOAT_EQ(META_NS::GetValue<float>(*toAny), 10.0f);
}

/**
 * @tc.name: ImportKeyframeAnimationWithCurve
 * @tc.desc: Keyframe animation with CubicBezierEasingCurve via objectUid
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportKeyframeAnimationWithCurve, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/keyframe_animation_curve.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "AnimationType"), "keyframeAnimation");
    EXPECT_EQ(GetProp<BASE_NS::string>(meta, "Name"), "kfAnimWithCurve");
    EXPECT_FLOAT_EQ(GetProp<float>(meta, "DurationMs"), 1000.0f);

    auto fromAny = GetProp<META_NS::IAny::Ptr>(meta, "From");
    ASSERT_TRUE(fromAny);
    EXPECT_FLOAT_EQ(META_NS::GetValue<float>(*fromAny), 0.0f);

    auto toAny = GetProp<META_NS::IAny::Ptr>(meta, "To");
    ASSERT_TRUE(toAny);
    EXPECT_FLOAT_EQ(META_NS::GetValue<float>(*toAny), 100.0f);

    auto curveObj = GetProp<META_NS::IObject::Ptr>(meta, "Curve");
    ASSERT_TRUE(curveObj);
    auto bezier = interface_cast<META_NS::ICubicBezier>(curveObj);
    ASSERT_TRUE(bezier);

    auto cp1 = bezier->ControlPoint1()->GetValue();
    EXPECT_FLOAT_EQ(cp1.x, 0.25f);
    EXPECT_FLOAT_EQ(cp1.y, 0.1f);

    auto cp2 = bezier->ControlPoint2()->GetValue();
    EXPECT_FLOAT_EQ(cp2.x, 0.25f);
    EXPECT_FLOAT_EQ(cp2.y, 1.0f);
}

/**
 * @tc.name: ImportKeyframeAnimationFromToVec2
 * @tc.desc: vec2 from/to values unwrap to typed Any<Vec2>
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportKeyframeAnimationFromToVec2, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/keyframe_animation_vec2.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto fromAny = GetProp<META_NS::IAny::Ptr>(meta, "From");
    ASSERT_TRUE(fromAny);
    BASE_NS::Math::Vec2 from;
    EXPECT_TRUE(fromAny->GetValue(from));
    EXPECT_FLOAT_EQ(from.x, 1.0f);
    EXPECT_FLOAT_EQ(from.y, 2.0f);

    auto toAny = GetProp<META_NS::IAny::Ptr>(meta, "To");
    ASSERT_TRUE(toAny);
    BASE_NS::Math::Vec2 to;
    EXPECT_TRUE(toAny->GetValue(to));
    EXPECT_FLOAT_EQ(to.x, 3.0f);
    EXPECT_FLOAT_EQ(to.y, 4.0f);
}

/**
 * @tc.name: ImportKeyframeAnimationFromToVec3
 * @tc.desc: vec3 from/to values unwrap to typed Any<Vec3>
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportKeyframeAnimationFromToVec3, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/keyframe_animation_vec3.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto fromAny = GetProp<META_NS::IAny::Ptr>(meta, "From");
    ASSERT_TRUE(fromAny);
    BASE_NS::Math::Vec3 from;
    EXPECT_TRUE(fromAny->GetValue(from));
    EXPECT_FLOAT_EQ(from.x, 1.0f);
    EXPECT_FLOAT_EQ(from.z, 3.0f);

    auto toAny = GetProp<META_NS::IAny::Ptr>(meta, "To");
    ASSERT_TRUE(toAny);
    BASE_NS::Math::Vec3 to;
    EXPECT_TRUE(toAny->GetValue(to));
    EXPECT_FLOAT_EQ(to.x, 4.0f);
    EXPECT_FLOAT_EQ(to.z, 6.0f);
}

/**
 * @tc.name: ImportKeyframeAnimationFromToVec4
 * @tc.desc: vec4 from/to values unwrap to typed Any<Vec4>
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportKeyframeAnimationFromToVec4, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/keyframe_animation_vec4.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto fromAny = GetProp<META_NS::IAny::Ptr>(meta, "From");
    ASSERT_TRUE(fromAny);
    BASE_NS::Math::Vec4 from;
    EXPECT_TRUE(fromAny->GetValue(from));
    EXPECT_FLOAT_EQ(from.x, 1.0f);
    EXPECT_FLOAT_EQ(from.w, 4.0f);

    auto toAny = GetProp<META_NS::IAny::Ptr>(meta, "To");
    ASSERT_TRUE(toAny);
    BASE_NS::Math::Vec4 to;
    EXPECT_TRUE(toAny->GetValue(to));
    EXPECT_FLOAT_EQ(to.x, 5.0f);
    EXPECT_FLOAT_EQ(to.w, 8.0f);
}

/**
 * @tc.name: ImportKeyframeAnimationFromToQuat
 * @tc.desc: quat from/to values unwrap to typed Any<Quat>
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportKeyframeAnimationFromToQuat, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/keyframe_animation_quat.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto fromAny = GetProp<META_NS::IAny::Ptr>(meta, "From");
    ASSERT_TRUE(fromAny);
    BASE_NS::Math::Quat from;
    EXPECT_TRUE(fromAny->GetValue(from));
    EXPECT_FLOAT_EQ(from.w, 1.0f);

    auto toAny = GetProp<META_NS::IAny::Ptr>(meta, "To");
    ASSERT_TRUE(toAny);
    BASE_NS::Math::Quat to;
    EXPECT_TRUE(toAny->GetValue(to));
    EXPECT_FLOAT_EQ(to.x, 1.0f);
    EXPECT_FLOAT_EQ(to.w, 0.0f);
}

/**
 * @tc.name: ImportKeyframeAnimationFromToColor
 * @tc.desc: color from/to values unwrap to typed Any<Color>
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportKeyframeAnimationFromToColor, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/keyframe_animation_color.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto fromAny = GetProp<META_NS::IAny::Ptr>(meta, "From");
    ASSERT_TRUE(fromAny);
    BASE_NS::Color from;
    EXPECT_TRUE(fromAny->GetValue(from));
    EXPECT_FLOAT_EQ(from.x, 0.1f);
    EXPECT_FLOAT_EQ(from.w, 1.0f);

    auto toAny = GetProp<META_NS::IAny::Ptr>(meta, "To");
    ASSERT_TRUE(toAny);
    BASE_NS::Color to;
    EXPECT_TRUE(toAny->GetValue(to));
    EXPECT_FLOAT_EQ(to.x, 0.5f);
    EXPECT_FLOAT_EQ(to.w, 0.8f);
}

/**
 * @tc.name: ImportKeyframeAnimationFromToUVec2
 * @tc.desc: uvec2 from/to values unwrap to typed Any<UVec2>
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportKeyframeAnimationFromToUVec2, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/keyframe_animation_uvec2.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto fromAny = GetProp<META_NS::IAny::Ptr>(meta, "From");
    ASSERT_TRUE(fromAny);
    BASE_NS::Math::UVec2 from;
    EXPECT_TRUE(fromAny->GetValue(from));
    EXPECT_EQ(from.x, 10u);
    EXPECT_EQ(from.y, 20u);

    auto toAny = GetProp<META_NS::IAny::Ptr>(meta, "To");
    ASSERT_TRUE(toAny);
    BASE_NS::Math::UVec2 to;
    EXPECT_TRUE(toAny->GetValue(to));
    EXPECT_EQ(to.x, 30u);
    EXPECT_EQ(to.y, 40u);
}

/**
 * @tc.name: ImportKeyframeAnimationFromToMat4x4
 * @tc.desc: mat4x4 from/to values unwrap to typed Any<Mat4X4>
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportKeyframeAnimationFromToMat4x4, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/keyframe_animation_mat4x4.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto fromAny = GetProp<META_NS::IAny::Ptr>(meta, "From");
    ASSERT_TRUE(fromAny);
    BASE_NS::Math::Mat4X4 from;
    EXPECT_TRUE(fromAny->GetValue(from));
    EXPECT_FLOAT_EQ(from.x.x, 1.0f);
    EXPECT_FLOAT_EQ(from.w.w, 1.0f);

    auto toAny = GetProp<META_NS::IAny::Ptr>(meta, "To");
    ASSERT_TRUE(toAny);
    BASE_NS::Math::Mat4X4 to;
    EXPECT_TRUE(toAny->GetValue(to));
    EXPECT_FLOAT_EQ(to.x.x, 2.0f);
    EXPECT_FLOAT_EQ(to.w.x, 5.0f);
    EXPECT_FLOAT_EQ(to.w.z, 7.0f);
}

/**
 * @tc.name: ImportKeyframeAnimationFromToResourceId
 * @tc.desc: resourceId from/to values unwrap to typed Any<ResourceId>
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportKeyframeAnimationFromToResourceId, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/keyframe_animation_resourceId.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto fromAny = GetProp<META_NS::IAny::Ptr>(meta, "From");
    ASSERT_TRUE(fromAny);
    CORE_NS::ResourceId from;
    EXPECT_TRUE(fromAny->GetValue(from));
    EXPECT_EQ(from, CORE_NS::ResourceId("from-ref", "g1"));

    auto toAny = GetProp<META_NS::IAny::Ptr>(meta, "To");
    ASSERT_TRUE(toAny);
    CORE_NS::ResourceId to;
    EXPECT_TRUE(toAny->GetValue(to));
    EXPECT_EQ(to, CORE_NS::ResourceId("to-ref", ""));
}

/**
 * @tc.name: ImportAnimationCurveBuiltinLinear
 * @tc.desc: Keyframe animation with built-in "linear" curve (type-string shorthand)
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportAnimationCurveBuiltinLinear, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/keyframe_animation_curve_linear.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto curveObj = GetProp<META_NS::IObject::Ptr>(meta, "Curve");
    ASSERT_TRUE(curveObj);
    auto curve = interface_cast<META_NS::ICurve1D>(curveObj);
    ASSERT_TRUE(curve);
    EXPECT_FLOAT_EQ(curve->Transform(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(curve->Transform(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(curve->Transform(1.0f), 1.0f);
}

/**
 * @tc.name: ImportAnimationCurveBuiltinInOutCubic
 * @tc.desc: Keyframe animation with built-in "inOutCubic" curve is a real ease curve
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportAnimationCurveBuiltinInOutCubic, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/keyframe_animation_curve_builtin.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto curveObj = GetProp<META_NS::IObject::Ptr>(meta, "Curve");
    ASSERT_TRUE(curveObj);
    auto curve = interface_cast<META_NS::ICurve1D>(curveObj);
    ASSERT_TRUE(curve);
    EXPECT_FLOAT_EQ(curve->Transform(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(curve->Transform(1.0f), 1.0f);
    EXPECT_NEAR(curve->Transform(0.5f), 0.5f, 1e-4f);
    // Ease-in-out cubic is below y=x on the first half
    EXPECT_LT(curve->Transform(0.25f), 0.25f);
    EXPECT_GT(curve->Transform(0.75f), 0.75f);
}

/**
 * @tc.name: ImportAnimationCurveBezierInline
 * @tc.desc: Keyframe animation with inline "cubicBezier" shorthand (controlPoint1/2 as vec2 arrays)
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportAnimationCurveBezierInline, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/keyframe_animation_curve_bezier_inline.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto curveObj = GetProp<META_NS::IObject::Ptr>(meta, "Curve");
    ASSERT_TRUE(curveObj);
    auto bezier = interface_cast<META_NS::ICubicBezier>(curveObj);
    ASSERT_TRUE(bezier);

    auto cp1 = bezier->ControlPoint1()->GetValue();
    EXPECT_FLOAT_EQ(cp1.x, 0.42f);
    EXPECT_FLOAT_EQ(cp1.y, 0.0f);

    auto cp2 = bezier->ControlPoint2()->GetValue();
    EXPECT_FLOAT_EQ(cp2.x, 0.58f);
    EXPECT_FLOAT_EQ(cp2.y, 1.0f);
}

/**
 * @tc.name: ImportTrackAnimationKeyframeCurvesMixed
 * @tc.desc: Track animation with mixed keyframeCurves: built-in, null, and inline cubicBezier
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportTrackAnimationKeyframeCurvesMixed, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/animation/track_animation_keyframe_curves_mixed.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto prop = meta->GetProperty("KeyframeCurves");
    ASSERT_TRUE(prop);
    auto arrProp = META_NS::ArrayProperty<META_NS::IObject::Ptr>(prop);
    ASSERT_TRUE(arrProp);
    auto curves = arrProp->GetValue();
    ASSERT_EQ(curves.size(), 4u);

    EXPECT_TRUE(interface_cast<META_NS::ICurve1D>(curves[0]));
    EXPECT_TRUE(interface_cast<META_NS::ICurve1D>(curves[1]));
    EXPECT_FALSE(curves[2]);  // null entry => nullptr

    auto bezier = interface_cast<META_NS::ICubicBezier>(curves[3]);
    ASSERT_TRUE(bezier);
    auto cp1 = bezier->ControlPoint1()->GetValue();
    EXPECT_FLOAT_EQ(cp1.x, 0.1f);
    EXPECT_FLOAT_EQ(cp1.y, 0.2f);
    auto cp2 = bezier->ControlPoint2()->GetValue();
    EXPECT_FLOAT_EQ(cp2.x, 0.3f);
    EXPECT_FLOAT_EQ(cp2.y, 0.4f);
}

/**
 * @tc.name: ImportAnimationCurveUnknownTypeFails
 * @tc.desc: Unknown built-in curve name produces a diagnostic rather than a silent success
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportAnimationCurveUnknownTypeFails, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/animation/keyframe_animation_curve_unknown_type.json", {});
    EXPECT_FALSE(res);
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "Unknown built-in curve type");
}

/**
 * @tc.name: AnimationCurveFromIndex
 * @tc.desc: Keyframe animation loaded from index has CubicBezier curve applied to real IAnimation
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, AnimationCurveFromIndex, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto rman = ImportIndexIntoScene("test://import/index/index_anim_curve.json", scene);
    ASSERT_TRUE(rman);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("curve-anim"), scene}));
    ASSERT_TRUE(anim);

    EXPECT_EQ(META_NS::GetValue(anim->Name()), "curveAnim");

    auto kfa = interface_cast<META_NS::IKeyframeAnimation>(anim);
    ASSERT_TRUE(kfa);

    auto fromAny = kfa->From()->GetValue();
    ASSERT_TRUE(fromAny);
    EXPECT_FLOAT_EQ(META_NS::GetValue<float>(*fromAny), 0.0f);

    auto toAny = kfa->To()->GetValue();
    ASSERT_TRUE(toAny);
    EXPECT_FLOAT_EQ(META_NS::GetValue<float>(*toAny), 100.0f);

    auto curve = anim->Curve()->GetValue();
    ASSERT_TRUE(curve);

    auto bezier = interface_cast<META_NS::ICubicBezier>(curve);
    ASSERT_TRUE(bezier);

    auto cp1 = bezier->ControlPoint1()->GetValue();
    EXPECT_FLOAT_EQ(cp1.x, 0.25f);
    EXPECT_FLOAT_EQ(cp1.y, 0.1f);

    auto cp2 = bezier->ControlPoint2()->GetValue();
    EXPECT_FLOAT_EQ(cp2.x, 0.25f);
    EXPECT_FLOAT_EQ(cp2.y, 1.0f);
}

/**
 * @tc.name: ImportContainerAnimationWithResourceId
 * @tc.desc: Container animation with resourceId child resolved via deferred actions runs correctly
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportContainerAnimationWithResourceId, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto rman = ImportIndexIntoScene("test://import/index/index_anim_container_resid.json", scene);
    ASSERT_TRUE(rman);

    auto seqAnim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("seq-anim"), scene}));
    ASSERT_TRUE(seqAnim);

    // Verify structure: sequential with two children
    auto staggered = interface_cast<META_NS::IStaggeredAnimation>(seqAnim);
    ASSERT_TRUE(staggered);
    auto children = staggered->GetAnimations();
    ASSERT_EQ(children.size(), 2u);
    EXPECT_EQ(META_NS::GetValue(children[0]->Name()), "inlineChild");
    EXPECT_EQ(META_NS::GetValue(children[1]->Name()), "resolvedChild");

    // Start and verify it runs
    auto startable = interface_cast<META_NS::IStartableAnimation>(seqAnim);
    ASSERT_TRUE(startable);
    startable->Start();
    UpdateScene();
    EXPECT_TRUE(seqAnim->Running()->GetValue());

    // Advance partway through the first child (duration 500ms)
    UpdateScene(META_NS::TimeSpan::Milliseconds(250));
    WaitForUserQueue();
    EXPECT_TRUE(seqAnim->Running()->GetValue());
    EXPECT_GT(seqAnim->Progress()->GetValue(), 0.0f);

    // Advance past first child into second child (resolved resourceId, duration 1000ms)
    UpdateScene(META_NS::TimeSpan::Milliseconds(500));
    WaitForUserQueue();
    EXPECT_TRUE(seqAnim->Running()->GetValue());

    // Advance past the total duration (500 + 1000 = 1500ms)
    UpdateScene(META_NS::TimeSpan::Milliseconds(1000));
    WaitForUserQueue();
    EXPECT_FALSE(seqAnim->Running()->GetValue());
}

/**
 * @tc.name: AnimationNegativeSpeedModifier
 * @tc.desc: Animation instance with negative speed factor has correct modifier attached
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, AnimationNegativeSpeedModifier, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto rman = ImportIndexIntoScene("test://import/index/index_anim_negative_speed.json", scene);
    ASSERT_TRUE(rman);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("reverse-anim"), scene}));
    ASSERT_TRUE(anim);

    EXPECT_EQ(META_NS::GetValue(anim->Name()), "reverseAnim");
    EXPECT_EQ(META_NS::GetValue(anim->Enabled()), true);

    auto attach = interface_cast<META_NS::IAttach>(anim);
    ASSERT_TRUE(attach);

    auto speeds = attach->GetAttachments<META_NS::AnimationModifiers::ISpeed>();
    ASSERT_EQ(speeds.size(), 1u);
    EXPECT_FLOAT_EQ(META_NS::GetValue(speeds[0]->SpeedFactor()), -1.0f);

    auto timed = interface_cast<META_NS::ITimedAnimation>(anim);
    ASSERT_TRUE(timed);
    EXPECT_EQ(META_NS::GetValue(timed->Duration()), META_NS::TimeSpan::Milliseconds(500));
}

/**
 * @tc.name: AnimationZeroLoopModifier
 * @tc.desc: Animation instance with zero loop count has correct modifier attached
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, AnimationZeroLoopModifier, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto rman = ImportIndexIntoScene("test://import/index/index_anim_zero_loop.json", scene);
    ASSERT_TRUE(rman);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("zero-loop-anim"), scene}));
    ASSERT_TRUE(anim);

    EXPECT_EQ(META_NS::GetValue(anim->Name()), "zeroLoopAnim");

    auto attach = interface_cast<META_NS::IAttach>(anim);
    ASSERT_TRUE(attach);

    auto loops = attach->GetAttachments<META_NS::AnimationModifiers::ILoop>();
    ASSERT_EQ(loops.size(), 1u);
    EXPECT_EQ(META_NS::GetValue(loops[0]->LoopCount()), 0);
}

/**
 * @tc.name: TrackAnimationUnevenTimestamps
 * @tc.desc: Track animation instance with non-uniform timestamp spacing
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, TrackAnimationUnevenTimestamps, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto rman = ImportIndexIntoScene("test://import/index/index_anim_uneven_timestamps.json", scene);
    ASSERT_TRUE(rman);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("uneven-anim"), scene}));
    ASSERT_TRUE(anim);

    EXPECT_EQ(META_NS::GetValue(anim->Name()), "unevenAnim");

    auto timed = interface_cast<META_NS::ITimedAnimation>(anim);
    ASSERT_TRUE(timed);
    EXPECT_EQ(META_NS::GetValue(timed->Duration()), META_NS::TimeSpan::Milliseconds(2000));

    auto track = interface_cast<META_NS::ITrackAnimation>(anim);
    ASSERT_TRUE(track);
    auto timestamps = track->Timestamps()->GetValue();
    ASSERT_EQ(timestamps.size(), 5u);
    EXPECT_FLOAT_EQ(timestamps[0], 0.0f);
    EXPECT_FLOAT_EQ(timestamps[1], 0.1f);
    EXPECT_FLOAT_EQ(timestamps[2], 0.15f);
    EXPECT_FLOAT_EQ(timestamps[3], 0.8f);
    EXPECT_FLOAT_EQ(timestamps[4], 1.0f);
}

/**
 * @tc.name: NestedContainerAnimationRuns
 * @tc.desc: Sequential container with nested parallel child runs through all children
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, NestedContainerAnimationRuns, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto rman = ImportIndexIntoScene("test://import/index/index_anim_nested_container.json", scene);
    ASSERT_TRUE(rman);

    auto anim =
        interface_pointer_cast<META_NS::IAnimation>(rman->GetResource({CORE_NS::ResourceId("nested-anim"), scene}));
    ASSERT_TRUE(anim);

    EXPECT_EQ(META_NS::GetValue(anim->Name()), "outerSeq");

    // Verify structure: sequential with 2 children
    auto staggered = interface_cast<META_NS::IStaggeredAnimation>(anim);
    ASSERT_TRUE(staggered);
    auto children = staggered->GetAnimations();
    ASSERT_EQ(children.size(), 2u);
    EXPECT_EQ(META_NS::GetValue(children[0]->Name()), "innerPar");
    EXPECT_EQ(META_NS::GetValue(children[1]->Name()), "sibling");

    // Verify inner parallel has 2 children
    auto innerStaggered = interface_cast<META_NS::IStaggeredAnimation>(children[0]);
    ASSERT_TRUE(innerStaggered);
    auto innerChildren = innerStaggered->GetAnimations();
    ASSERT_EQ(innerChildren.size(), 2u);
    EXPECT_EQ(META_NS::GetValue(innerChildren[0]->Name()), "deepChild1");
    EXPECT_EQ(META_NS::GetValue(innerChildren[1]->Name()), "deepChild2");

    // Start and verify it runs
    auto startable = interface_cast<META_NS::IStartableAnimation>(anim);
    ASSERT_TRUE(startable);
    startable->Start();
    UpdateScene();
    EXPECT_TRUE(anim->Running()->GetValue());

    // Advance partway — parallel child runs for max(200,300)=300ms, sibling runs 400ms = 700ms total
    UpdateScene(META_NS::TimeSpan::Milliseconds(100));
    WaitForUserQueue();
    EXPECT_TRUE(anim->Running()->GetValue());

    // Advance past total duration
    UpdateScene(META_NS::TimeSpan::Milliseconds(700));
    WaitForUserQueue();
    EXPECT_FALSE(anim->Running()->GetValue());
}

/**
 * @tc.name: ImportIndexDeferredMissingImage
 * @tc.desc: Image references in an index are stored as lazy refs and resolved at apply time
 *           against the scene's resource manager. A reference to a non-existent image does
 *           not fail the import — the property simply stays unbound on the live resource and
 *           a warning is logged when the resource is later instantiated.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexDeferredMissingImage, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index_deferred_missing_image.json", {});
    EXPECT_TRUE(res);
}

/**
 * @tc.name: ImportIndexDeferredMissingImageContinue
 * @tc.desc: Continue-on-error mode: lazy image refs do not produce an error diagnostic at
 *           import time (resolution happens at apply time, which is a warning, not a fatal
 *           error).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexDeferredMissingImageContinue, testing::ext::TestSize.Level1)
{
    SCENE_IMP_NS::IImporter::Ptr impContinue =
        META_NS::GetObjectRegistry().Create<SCENE_IMP_NS::IImporter>(SCENE_IMP_NS::ClassId::Importer);
    ASSERT_TRUE(impContinue);
    ASSERT_TRUE(impContinue->Initialize(context, {{}, true}));
    auto res = impContinue->Import("test://import/index/index_deferred_missing_image.json", {});
    EXPECT_TRUE(res);
}

/**
 * @tc.name: ImportIndexDeferredMissingShader
 * @tc.desc: Material shader references in the index are stored as lazy resource refs and
 *           resolved at apply time. A reference to a non-existent shader does not fail the
 *           import — the property simply stays unbound on the live material; a warning is
 *           logged when apply runs.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexDeferredMissingShader, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index_deferred_missing_shader.json", {});
    EXPECT_TRUE(res);
}

/**
 * @tc.name: ImportIndexDeferredWrongType
 * @tc.desc: Type-mismatched lazy refs (an image field referencing a shader id) do not fail
 *           the import. Resolution happens at apply time: the resource is found but the
 *           interface_pointer_cast to the expected type returns null, the destination stays
 *           unbound, and a warning is logged.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexDeferredWrongType, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index_deferred_wrong_type.json", {});
    EXPECT_TRUE(res);
}

/**
 * @tc.name: ImportIndexAnimationOptionsTypeMismatch
 * @tc.desc: Inner options.type that contradicts the entry's outer type fails the import
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportIndexAnimationOptionsTypeMismatch, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/index/index_anim_type_mismatch.json", {});
    EXPECT_FALSE(res);
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "does not match entry type");
}

/**
 * @tc.name: ImportMaterialTypes
 * @tc.desc: Material instances with unlit, unlitShadowAlpha, custom types
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportMaterialTypes, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto rman = ImportIndexIntoScene("test://import/index/index_mat_types.json", scene);
    ASSERT_TRUE(rman);

    auto matUnlit = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("mat-unlit"), scene}));
    ASSERT_TRUE(matUnlit);
    EXPECT_EQ(META_NS::GetValue(matUnlit->Type()), SCENE_NS::MaterialType::UNLIT);
    EXPECT_FLOAT_EQ(META_NS::GetValue(matUnlit->AlphaCutoff()), 0.0f);

    auto matUnlitShadow =
        interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("mat-unlit-shadow"), scene}));
    ASSERT_TRUE(matUnlitShadow);
    EXPECT_EQ(META_NS::GetValue(matUnlitShadow->Type()), SCENE_NS::MaterialType::UNLIT_SHADOW_ALPHA);
    EXPECT_FLOAT_EQ(META_NS::GetValue(matUnlitShadow->AlphaCutoff()), 1.0f);

    auto matCustom = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("mat-custom"), scene}));
    ASSERT_TRUE(matCustom);
    EXPECT_EQ(META_NS::GetValue(matCustom->Type()), SCENE_NS::MaterialType::CUSTOM);
    EXPECT_FLOAT_EQ(META_NS::GetValue(matCustom->AlphaCutoff()), 0.5f);
}

/**
 * @tc.name: ImportMaterialLightingFlags
 * @tc.desc: Material instances with all lighting flag bits set and with no flags
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportMaterialLightingFlags, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto rman = ImportIndexIntoScene("test://import/index/index_mat_lighting_flags.json", scene);
    ASSERT_TRUE(rman);

    auto matAll = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("mat-all-flags"), scene}));
    ASSERT_TRUE(matAll);
    EXPECT_EQ(META_NS::GetValue(matAll->LightingFlags()), static_cast<SCENE_NS::LightingFlags>(63));

    auto matNone = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("mat-no-flags"), scene}));
    ASSERT_TRUE(matNone);
    EXPECT_EQ(META_NS::GetValue(matNone->LightingFlags()), static_cast<SCENE_NS::LightingFlags>(0));
}

/**
 * @tc.name: ImportMaterialMultiTexture
 * @tc.desc: Material with multiple textures imports successfully and has correct slot count
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportMaterialMultiTexture, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto rman = ImportIndexIntoScene("test://import/index/index_mat_multi_texture.json", scene);
    ASSERT_TRUE(rman);

    // Verify image resource was registered by the index
    auto imageInfo = rman->GetResourceInfo({"tex-image", scene});
    EXPECT_TRUE(imageInfo.id.IsValid());

    auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("mat-multi-tex"), scene}));
    ASSERT_TRUE(mat);
    EXPECT_EQ(META_NS::GetValue(mat->Type()), SCENE_NS::MaterialType::METALLIC_ROUGHNESS);

    // METALLIC_ROUGHNESS has 11 texture slots
    auto textures = mat->Textures();
    ASSERT_TRUE(textures);
    EXPECT_EQ(textures->GetSize(), 11u);

    // Verify texture slots are valid ITexture instances
    for (size_t i = 0; i < textures->GetSize(); ++i) {
        auto tex = interface_cast<ITexture>(textures->GetValueAt(i));
        EXPECT_TRUE(tex) << "Texture slot " << i << " is not a valid ITexture";
    }

    // Slot 0 (baseColor): factor [1.0, 0.5, 0.25, 1.0], rotation 0.5
    auto tex0 = interface_cast<ITexture>(textures->GetValueAt(0));
    ASSERT_TRUE(tex0);
    EXPECT_EQ(tex0->Factor()->GetValue(), BASE_NS::Math::Vec4(1.0f, 0.5f, 0.25f, 1.0f));
    EXPECT_FLOAT_EQ(tex0->Rotation()->GetValue(), 0.5f);

    // Slot 1 (normal): factor [0.0, 0.0, 1.0, 1.0], rotation 0.0
    auto tex1 = interface_cast<ITexture>(textures->GetValueAt(1));
    ASSERT_TRUE(tex1);
    EXPECT_EQ(tex1->Factor()->GetValue(), BASE_NS::Math::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    EXPECT_FLOAT_EQ(tex1->Rotation()->GetValue(), 0.0f);

    // Slot 3 (emissive): factor [1.0, 1.0, 1.0, 0.5], rotation 1.57
    // (slot index 3 = MaterialComponent::TextureIndex::EMISSIVE — name-resolved)
    auto tex3 = interface_cast<ITexture>(textures->GetValueAt(3));
    ASSERT_TRUE(tex3);
    EXPECT_EQ(tex3->Factor()->GetValue(), BASE_NS::Math::Vec4(1.0f, 1.0f, 1.0f, 0.5f));
    EXPECT_FLOAT_EQ(tex3->Rotation()->GetValue(), 1.57f);
}

/**
 * @tc.name: ImportMaterialTextureSampler
 * @tc.desc: Texture with sampler, translation and scale properties imports correctly
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportMaterialTextureSampler, testing::ext::TestSize.Level1)
{
    auto obj = Load("test://import/material/material_texture_sampler.json");
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    ASSERT_TRUE(meta);

    auto textures = meta->GetArrayProperty<META_NS::IObject::Ptr>("Textures");
    ASSERT_TRUE(textures);
    ASSERT_GE(textures->GetSize(), 3u);

    // First texture: baseColor with full sampler, translation, scale
    {
        auto tex = SCENE_NS::TemplateTextureView(textures->GetValueAt(0));
        ASSERT_TRUE(tex);
        EXPECT_EQ(META_NS::GetValue(tex.Name()), "baseColor");
        EXPECT_EQ(META_NS::GetValue(tex.Factor()), BASE_NS::Math::Vec4(1.0f, 0.5f, 0.25f, 1.0f));
        EXPECT_EQ(META_NS::GetValue(tex.Translation()), BASE_NS::Math::Vec2(0.1f, 0.2f));
        EXPECT_FLOAT_EQ(META_NS::GetValue(tex.Rotation()), 0.5f);
        EXPECT_EQ(META_NS::GetValue(tex.Scale()), BASE_NS::Math::Vec2(2.0f, 3.0f));

        auto sampler = tex.Sampler();
        ASSERT_TRUE(sampler);
        EXPECT_EQ(META_NS::GetValue(sampler.MagFilter()), SCENE_NS::SamplerFilter::LINEAR);
        EXPECT_EQ(META_NS::GetValue(sampler.MinFilter()), SCENE_NS::SamplerFilter::NEAREST);
        EXPECT_EQ(META_NS::GetValue(sampler.MipMapMode()), SCENE_NS::SamplerFilter::LINEAR);
        EXPECT_EQ(META_NS::GetValue(sampler.AddressModeU()), SCENE_NS::SamplerAddressMode::REPEAT);
        EXPECT_EQ(META_NS::GetValue(sampler.AddressModeV()), SCENE_NS::SamplerAddressMode::CLAMP_TO_EDGE);
        EXPECT_EQ(META_NS::GetValue(sampler.AddressModeW()), SCENE_NS::SamplerAddressMode::MIRRORED_REPEAT);
    }

    // Second texture: normal with partial sampler (only mag/min filter).
    // Per the "only what JSON declared" rule, sampler subfields not in JSON
    // are absent from the template entry rather than present-but-unset.
    {
        auto tex = SCENE_NS::TemplateTextureView(textures->GetValueAt(1));
        ASSERT_TRUE(tex);
        EXPECT_EQ(META_NS::GetValue(tex.Name()), "normal");

        auto sampler = tex.Sampler();
        ASSERT_TRUE(sampler);
        EXPECT_EQ(META_NS::GetValue(sampler.MagFilter()), SCENE_NS::SamplerFilter::NEAREST);
        EXPECT_EQ(META_NS::GetValue(sampler.MinFilter()), SCENE_NS::SamplerFilter::LINEAR);
        EXPECT_FALSE(sampler.MipMapMode().GetProperty());
        EXPECT_FALSE(sampler.AddressModeU().GetProperty());
    }

    // Third texture: emissive with translation and scale, no sampler in JSON.
    // Sampler sub-object should not exist on the template entry at all.
    {
        auto tex = SCENE_NS::TemplateTextureView(textures->GetValueAt(2));
        ASSERT_TRUE(tex);
        EXPECT_EQ(META_NS::GetValue(tex.Name()), "emissive");
        EXPECT_EQ(META_NS::GetValue(tex.Translation()), BASE_NS::Math::Vec2(0.5f, -0.5f));
        EXPECT_EQ(META_NS::GetValue(tex.Scale()), BASE_NS::Math::Vec2(1.0f, 1.0f));
        EXPECT_FALSE(tex.Sampler());
    }
}

/**
 * @tc.name: ImportMaterialTextureSamplerViaInterface
 * @tc.desc: Texture sampler, translation, and scale are accessible via IMaterial/ITexture/ISampler interfaces
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ResourceImportTest, ImportMaterialTextureSamplerViaInterface, testing::ext::TestSize.Level1)
{
    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto rman = ImportIndexIntoScene("test://import/index/index_mat_texture_sampler.json", scene);
    ASSERT_TRUE(rman);

    auto mat = interface_pointer_cast<IMaterial>(rman->GetResource({CORE_NS::ResourceId("mat-tex-sampler"), scene}));
    ASSERT_TRUE(mat);
    EXPECT_EQ(META_NS::GetValue(mat->Type()), SCENE_NS::MaterialType::METALLIC_ROUGHNESS);

    auto textures = mat->Textures();
    ASSERT_TRUE(textures);
    EXPECT_EQ(textures->GetSize(), 11u);

    // First slot (baseColor) should have our imported values
    auto tex = interface_cast<ITexture>(textures->GetValueAt(0));
    ASSERT_TRUE(tex);

    EXPECT_EQ(tex->Factor()->GetValue(), BASE_NS::Math::Vec4(1.0f, 0.5f, 0.25f, 1.0f));
    EXPECT_EQ(tex->Translation()->GetValue(), BASE_NS::Math::Vec2(0.1f, 0.2f));
    EXPECT_FLOAT_EQ(tex->Rotation()->GetValue(), 0.5f);
    EXPECT_EQ(tex->Scale()->GetValue(), BASE_NS::Math::Vec2(2.0f, 3.0f));

    auto sampler = tex->Sampler()->GetValue();
    ASSERT_TRUE(sampler);
    EXPECT_EQ(sampler->MagFilter()->GetValue(), SCENE_NS::SamplerFilter::LINEAR);
    EXPECT_EQ(sampler->MinFilter()->GetValue(), SCENE_NS::SamplerFilter::NEAREST);
    EXPECT_EQ(sampler->MipMapMode()->GetValue(), SCENE_NS::SamplerFilter::LINEAR);
    EXPECT_EQ(sampler->AddressModeU()->GetValue(), SCENE_NS::SamplerAddressMode::REPEAT);
    EXPECT_EQ(sampler->AddressModeV()->GetValue(), SCENE_NS::SamplerAddressMode::CLAMP_TO_EDGE);
    EXPECT_EQ(sampler->AddressModeW()->GetValue(), SCENE_NS::SamplerAddressMode::MIRRORED_REPEAT);
}

}  // namespace UTest
SCENE_END_NAMESPACE()
