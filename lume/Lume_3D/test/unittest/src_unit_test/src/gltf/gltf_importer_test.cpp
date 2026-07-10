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

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/post_process_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/gltf/gltf.h>
#include <3d/intf_graphics_context.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>
#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <core/ecs/intf_ecs.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/json/json.h>
#include <core/os/intf_platform.h>
#include <core/plugin/intf_plugin_register.h>
#include <render/render_data_structures.h>

#include "gltf/gltf2.h"
#include "gltf/gltf2_util.h"
#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace {
class ImportListener final : public IGLTF2Importer::Listener {
public:
    void OnImportStarted() override{};
    void OnImportFinished() override
    {
        done = true;
    };
    void OnImportProgressed(size_t taskIndex, size_t taskCount) override{};
    bool IsDone()
    {
        return done;
    }

private:
    bool done{false};
};
GLTFImportResult LoadAndImport(string_view filename, Gltf2& gltf2, IEcs& ecs, Entity& root)
{
    auto gltf = gltf2.LoadGLTF(filename);
    if (gltf.success) {
        auto importer = gltf2.CreateGLTF2Importer(ecs);
        ImportListener listener{};
        importer->ImportGLTFAsync(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL, &listener);
        while (!importer->Execute(0)) {
        }
        EXPECT_TRUE(listener.IsDone());

        GLTFImportResult result = importer->GetResult();
        EXPECT_TRUE(result.success);
        if (result.success) {
            root = gltf2.ImportGltfScene(gltf.data->GetDefaultSceneIndex(),
                *gltf.data,
                result.data,
                ecs,
                {},
                CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL);
            EXPECT_TRUE(EntityUtil::IsValid(root));
            GetManager<IRenderConfigurationComponentManager>(ecs)->Create(root);
        } else {
            PLUGIN_LOG_E("Import error: %s", result.error.c_str());
        }
        return result;
    }
    GLTFImportResult error;
    error.success = false;
    error.error = gltf.error;
    return error;
}
}  // namespace

#ifdef NDEBUG
/**
 * @tc.name: ImportCustomGLTFTest
 * @tc.desc: Tests for Import Custom Gltftest. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFImporterTest, ImportCustomGLTFTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    constexpr string_view filename = "test://gltf/Custom/Custom.gltf";

    Entity root;
    GLTFImportResult originalGltf = LoadAndImport(filename, *gltf2, *ecs, root);
    ASSERT_TRUE(originalGltf.success);
    EXPECT_EQ(1, originalGltf.data.images.size());
    EXPECT_EQ(1, originalGltf.data.specularRadianceCubemaps.size());
    EXPECT_EQ(1, originalGltf.data.textures.size());
    EXPECT_EQ(11, originalGltf.data.materials.size());
    EXPECT_EQ(4, originalGltf.data.samplers.size());
    EXPECT_EQ(10, originalGltf.data.meshes.size());
    EXPECT_EQ(6, originalGltf.data.animations.size());

    constexpr string_view exportFilename = "cache://Custom.gltf";
    ASSERT_TRUE(gltf2->SaveGLTF(*ecs, exportFilename));

    delete gltf2;
}
#endif  // NDEBUG

/**
 * @tc.name: ImportSparseAccessorTest
 * @tc.desc: Tests for Import Sparse Accessor Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFImporterTest, ImportSparseAccessorTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    constexpr string_view filename = "test://gltf/SimpleSparseAccessor/SimpleSparseAccessor.gltf";

    Entity root;
    GLTFImportResult originalGltf = LoadAndImport(filename, *gltf2, *ecs, root);
    ASSERT_TRUE(originalGltf.success);
    EXPECT_EQ(0, originalGltf.data.samplers.size());
    EXPECT_EQ(0, originalGltf.data.images.size());
    EXPECT_EQ(0, originalGltf.data.textures.size());
    EXPECT_EQ(0, originalGltf.data.materials.size());
    EXPECT_EQ(1, originalGltf.data.meshes.size());
    EXPECT_EQ(0, originalGltf.data.skins.size());
    EXPECT_EQ(0, originalGltf.data.animations.size());
    EXPECT_EQ(0, originalGltf.data.specularRadianceCubemaps.size());

    constexpr string_view exportFilename = "cache://SimpleSparseAccessor.gltf";
    ASSERT_TRUE(gltf2->SaveGLTF(*ecs, exportFilename));

    delete gltf2;
}

/**
 * @tc.name: InvalidImageTest
 * @tc.desc: Tests for Invalid Image Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFImporterTest, InvalidImageTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);
    auto importer = gltf2->CreateGLTF2Importer(*ecs);

    GLTF2::Data data{engine->GetFileManager()};
    data.images.push_back(unique_ptr<GLTF2::Image>{new GLTF2::Image{}});

    // invalid image shouldn't fail, but still report an error message.
    {
        data.images[0]->uri = "invalidImage.png";
        importer->ImportGLTF(data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL & (~CORE_GLTF_IMPORT_RESOURCE_SKIP_UNUSED));
        GLTFImportResult result = importer->GetResult();
        EXPECT_TRUE(result.success);
        EXPECT_FALSE(result.error.empty());
    }

    {
        data.images[0]->uri = "data:image/png;base64,----.fdkl1=";
        importer->ImportGLTF(data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL & (~CORE_GLTF_IMPORT_RESOURCE_SKIP_UNUSED));
        GLTFImportResult result = importer->GetResult();
        EXPECT_TRUE(result.success);
        EXPECT_FALSE(result.error.empty());
    }

    delete gltf2;
}

#ifdef NDEBUG
/**
 * @tc.name: InvalidMeshTest
 * @tc.desc: Tests for Invalid Mesh Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFImporterTest, InvalidMeshTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);
    auto importer = gltf2->CreateGLTF2Importer(*ecs);

    GLTF2::Buffer buffer;
    GLTF2::BufferView bufferView;
    bufferView.buffer = &buffer;
    GLTF2::Accessor accessor;
    accessor.bufferView = &bufferView;

    {
        GLTF2::Data data{engine->GetFileManager()};
        data.meshes.push_back(unique_ptr<GLTF2::Mesh>{new GLTF2::Mesh{}});

        data.meshes[0]->primitives.resize(1);
        data.meshes[0]->primitives[0].attributes.resize(1);
        data.meshes[0]->primitives[0].attributes[0].accessor = &accessor;
        data.meshes[0]->primitives[0].attributes[0].attribute.index = 0;
        data.meshes[0]->primitives[0].attributes[0].attribute.type = GLTF2::AttributeType::POSITION;

        importer->ImportGLTF(data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);
        GLTFImportResult result = importer->GetResult();
        EXPECT_FALSE(result.success);
    }

    {
        GLTF2::Data data{engine->GetFileManager()};
        data.meshes.push_back(unique_ptr<GLTF2::Mesh>{new GLTF2::Mesh{}});

        data.meshes[0]->primitives.resize(1);
        data.meshes[0]->primitives[0].indices = &accessor;

        importer->ImportGLTF(data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);
        GLTFImportResult result = importer->GetResult();
        EXPECT_FALSE(result.success);
    }

    {
        GLTF2::Data data{engine->GetFileManager()};
        data.meshes.push_back(unique_ptr<GLTF2::Mesh>{new GLTF2::Mesh{}});

        importer->ImportGLTF(data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);
        GLTFImportResult result = importer->GetResult();
        EXPECT_FALSE(result.success);
    }

    delete gltf2;
}
#endif  // NDEBUG

/**
 * @tc.name: ImportMeshoptTest
 * @tc.desc: Import a glTF which uses meshopt extension.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFImporterTest, ImportMeshoptTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    constexpr string_view filename = "test://gltf/BrainStem/glTF-Meshopt/BrainStem.gltf";

    Entity root;
    GLTFImportResult originalGltf = LoadAndImport(filename, *gltf2, *ecs, root);
    if (originalGltf.success) {
        EXPECT_EQ(0U, originalGltf.data.samplers.size());
        EXPECT_EQ(0U, originalGltf.data.images.size());
        EXPECT_EQ(0U, originalGltf.data.textures.size());
        EXPECT_EQ(49U, originalGltf.data.materials.size());
        EXPECT_EQ(1U, originalGltf.data.meshes.size());
        EXPECT_EQ(1U, originalGltf.data.skins.size());
        EXPECT_EQ(1U, originalGltf.data.animations.size());
        EXPECT_EQ(0U, originalGltf.data.specularRadianceCubemaps.size());
    } else {
        // allow failing if extension was not supported.
        EXPECT_NE(originalGltf.error.find("EXT_meshopt_compression"), BASE_NS::string::npos);
    }

    delete gltf2;
}

namespace {
// base64 of `numBytes` zero bytes (numBytes must be a multiple of 3, so there is no padding).
string ZeroBufferBase64(size_t numBytes)
{
    string out;
    for (size_t i = 0U; i < numBytes / 3U; ++i) {
        out += "AAAA";
    }
    return out;
}

GLTFLoadResult LoadFromJson(Gltf2& gltf2, const string& json)
{
    return gltf2.LoadGLTF(array_view<uint8_t const>(reinterpret_cast<uint8_t const*>(json.data()), json.size()));
}

// Single mesh with POSITION (256 verts, 3072 B) and NORMAL (1 vert, 12 B) sharing one zero-filled buffer.
string MakeMismatchedAttributeGltf()
{
    string json = R"({"asset":{"version":"2.0"},"buffers":[{"uri":"data:application/octet-stream;base64,)";
    json += ZeroBufferBase64(3084U);
    json += R"(","byteLength":3084}],)"
            R"("bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":3072},)"
            R"({"buffer":0,"byteOffset":3072,"byteLength":12}],)"
            R"("accessors":[{"bufferView":0,"componentType":5126,"count":256,"type":"VEC3"},)"
            R"({"bufferView":1,"componentType":5126,"count":1,"type":"VEC3"}],)"
            R"("meshes":[{"primitives":[{"attributes":{"POSITION":0,"NORMAL":1}}]}]})";
    return json;
}

// POSITION accessor declares 256 verts (3072 B) but the data-URI supplies only 12 B (1 vert).
string MakeTruncatedBufferGltf()
{
    string json = R"({"asset":{"version":"2.0"},"buffers":[{"uri":"data:application/octet-stream;base64,)";
    json += ZeroBufferBase64(12U);
    json += R"(","byteLength":3072}],)"
            R"("bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":3072}],)"
            R"("accessors":[{"bufferView":0,"componentType":5126,"count":256,"type":"VEC3"}],)"
            R"("meshes":[{"primitives":[{"attributes":{"POSITION":0}}]}]})";
    return json;
}

// Animation with SCALAR sampler input accessor but UNSIGNED_BYTE (componentType 5121), count 64.
string MakeNonFloatAnimationInputGltf()
{
    string json = R"({"asset":{"version":"2.0"},"buffers":[{"uri":"data:application/octet-stream;base64,)";
    json += ZeroBufferBase64(834U);  // 64 B input (BYTE scalar) + 768 B output (64 * VEC3 float), padded to *3
    json += R"(","byteLength":834}],)"
            R"("bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":64},)"
            R"({"buffer":0,"byteOffset":64,"byteLength":768}],)"
            R"("accessors":[{"bufferView":0,"componentType":5121,"count":64,"type":"SCALAR"},)"
            R"({"bufferView":1,"componentType":5126,"count":64,"type":"VEC3"}],"nodes":[{}],)"
            R"("animations":[{"samplers":[{"input":0,"output":1,"interpolation":"LINEAR"}],)"
            R"("channels":[{"sampler":0,"target":{"node":0,"path":"translation"}}]}]})";
    return json;
}
}  // namespace

/**
 * @tc.name: ImportMismatchedAttributeCount
 * @tc.desc: A primitive whose attribute accessors have differing element counts must be imported without
 *           reading past the shorter attribute.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFImporterTest, ImportMismatchedAttributeCount, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    const string json = MakeMismatchedAttributeGltf();
    auto gltf = LoadFromJson(*gltf2, json);
    ASSERT_TRUE(gltf.success);  // the loader tolerates mismatched attribute counts

    auto importer = gltf2->CreateGLTF2Importer(*ecs);
    importer->ImportGLTF(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);
    // Graceful failure or success are both fine; ASan must not report an out-of-bounds access.
    (void)importer->GetResult();

    delete gltf2;
}

/**
 * @tc.name: ImportNonFloatAnimationInput
 * @tc.desc: A non-FLOAT animation sampler input should fail.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFImporterTest, ImportNonFloatAnimationInput, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    const string json = MakeNonFloatAnimationInputGltf();
    auto gltf = LoadFromJson(*gltf2, json);
    // Loader should rejects this; only import if it was accepted.
    EXPECT_FALSE(gltf.success);
    if (gltf.success) {
        auto importer = gltf2->CreateGLTF2Importer(*ecs);
        importer->ImportGLTF(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);
        (void)importer->GetResult();
    }

    delete gltf2;
}

/**
 * @tc.name: ImportTruncatedBuffer
 * @tc.desc: An accessor that declares more elements than the backing buffer actually provides should fail.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFImporterTest, ImportTruncatedBuffer, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    const string json = MakeTruncatedBufferGltf();
    auto gltf = LoadFromJson(*gltf2, json);
    ASSERT_TRUE(gltf.success);

    auto importer = gltf2->CreateGLTF2Importer(*ecs);
    importer->ImportGLTF(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);
    EXPECT_FALSE(importer->GetResult().success);

    delete gltf2;
}

/**
 * @tc.name: ImportFailedSecondPrimitiveDoesNotOverread
 * @tc.desc: When a later primitive fails to load an attribute after an earlier one already populated the CPU
 *           position copy, generating missing attributes must not read from an invalid position pointer.
 *           The mesh is built directly so the failure is isolated to one accessor (its buffer view has no data
 *           pointer) without failing buffer loading globally. Import may fail or succeed; ASan must not report
 *           an out-of-bounds read.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFImporterTest, ImportFailedSecondPrimitiveDoesNotOverread, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);
    auto importer = gltf2->CreateGLTF2Importer(*ecs);

    // One buffer of three VEC3 float vertices (36 B) backing the POSITION attributes.
    GLTF2::Buffer buffer;
    buffer.byteLength = 36U;
    buffer.data.resize(36U);

    // badView has a null data pointer, so loading an accessor through it fails without failing buffer loading.
    GLTF2::BufferView goodView;
    goodView.buffer = &buffer;
    goodView.byteLength = 36U;
    goodView.data = buffer.data.data();
    GLTF2::BufferView badView;
    badView.buffer = &buffer;
    badView.byteLength = 36U;
    badView.data = nullptr;

    const auto makeVec3Accessor = [](GLTF2::BufferView& view) {
        GLTF2::Accessor accessor;
        accessor.bufferView = &view;
        accessor.componentType = GLTF2::ComponentType::FLOAT;
        accessor.type = GLTF2::DataType::VEC3;
        accessor.count = 3U;
        return accessor;
    };
    GLTF2::Accessor position0 = makeVec3Accessor(goodView);
    GLTF2::Accessor position1 = makeVec3Accessor(goodView);
    GLTF2::Accessor normalBad = makeVec3Accessor(badView);

    GLTF2::Data data{engine->GetFileManager()};
    data.meshes.push_back(unique_ptr<GLTF2::Mesh>{new GLTF2::Mesh{}});
    auto& primitives = data.meshes[0]->primitives;
    primitives.resize(2);

    // primitive 0: POSITION only -> fills the position copy and is marked as needing generated normals.
    primitives[0].attributes.resize(1);
    primitives[0].attributes[0].attribute.type = GLTF2::AttributeType::POSITION;
    primitives[0].attributes[0].accessor = &position0;

    // primitive 1: valid POSITION plus a NORMAL that fails to load, so it bails leaving positionOffset at -1.
    primitives[1].attributes.resize(2);
    primitives[1].attributes[0].attribute.type = GLTF2::AttributeType::POSITION;
    primitives[1].attributes[0].accessor = &position1;
    primitives[1].attributes[1].attribute.type = GLTF2::AttributeType::NORMAL;
    primitives[1].attributes[1].accessor = &normalBad;

    importer->ImportGLTF(data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);
    // Failure or success are both fine; ASan must not report an out-of-bounds access.
    (void)importer->GetResult();

    delete gltf2;
}
