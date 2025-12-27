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

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace {
class ImportListener final : public IGLTF2Importer::Listener {
public:
    void OnImportStarted() override {};
    void OnImportFinished() override
    {
        done = true;
    };
    void OnImportProgressed(size_t taskIndex, size_t taskCount) override {};
    bool IsDone()
    {
        return done;
    }

private:
    bool done { false };
};
GLTFImportResult LoadAndImport(string_view filename, Gltf2& gltf2, IEcs& ecs, Entity& root)
{
    auto gltf = gltf2.LoadGLTF(filename);
    if (gltf.success) {
        auto importer = gltf2.CreateGLTF2Importer(ecs);
        ImportListener listener {};
        importer->ImportGLTFAsync(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL, &listener);
        while (!importer->Execute(0)) {
        }
        EXPECT_TRUE(listener.IsDone());

        GLTFImportResult result = importer->GetResult();
        EXPECT_TRUE(result.success);
        if (result.success) {
            root = gltf2.ImportGltfScene(gltf.data->GetDefaultSceneIndex(), *gltf.data, result.data, ecs, {},
                CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL);
            EXPECT_TRUE(EntityUtil::IsValid(root));
            GetManager<IRenderConfigurationComponentManager>(ecs)->Create(root);
        } else {
            CORE_LOG_E("Import error: %s", result.error.c_str());
        }
        return result;
    }
    GLTFImportResult error;
    error.success = false;
    error.error = gltf.error;
    return error;
}
} // namespace

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
#endif // NDEBUG

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

    GLTF2::Data data { engine->GetFileManager() };
    data.images.push_back(unique_ptr<GLTF2::Image> { new GLTF2::Image {} });

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
        GLTF2::Data data { engine->GetFileManager() };
        data.meshes.push_back(unique_ptr<GLTF2::Mesh> { new GLTF2::Mesh {} });

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
        GLTF2::Data data { engine->GetFileManager() };
        data.meshes.push_back(unique_ptr<GLTF2::Mesh> { new GLTF2::Mesh {} });

        data.meshes[0]->primitives.resize(1);
        data.meshes[0]->primitives[0].indices = &accessor;

        importer->ImportGLTF(data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);
        GLTFImportResult result = importer->GetResult();
        EXPECT_FALSE(result.success);
    }

    {
        GLTF2::Data data { engine->GetFileManager() };
        data.meshes.push_back(unique_ptr<GLTF2::Mesh> { new GLTF2::Mesh {} });

        importer->ImportGLTF(data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);
        GLTFImportResult result = importer->GetResult();
        EXPECT_FALSE(result.success);
    }

    delete gltf2;
}
#endif // NDEBUG

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
