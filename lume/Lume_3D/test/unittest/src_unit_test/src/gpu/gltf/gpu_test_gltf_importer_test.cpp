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

#include <3d/ecs/components/render_configuration_component.h>
#include <3d/gltf/gltf.h>
#include <3d/intf_graphics_context.h>
#include <core/ecs/intf_ecs.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/os/intf_platform.h>
#include <core/plugin/intf_plugin_register.h>

#include "gltf/gltf2.h"
#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;

/**
 * @tc.name: importGltfObject
 * @tc.desc: Tests for Import Gltf Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_GLTFImporterTest, importGltfObject, testing::ext::TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    constexpr string_view filename = "test://gltf/WaterBottle/WaterBottle.glb";

    auto gltf = gltf2->LoadGLTF(filename);
    ASSERT_TRUE(gltf.success);

    auto importer = gltf2->CreateGLTF2Importer(*ecs);
    importer->ImportGLTF(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);

    auto importResult = importer->GetResult();
    ASSERT_TRUE(importResult.success);

    ASSERT_EQ(importResult.data.textures.size(), 4);
    ASSERT_EQ(importResult.data.materials.size(), 1);
    ASSERT_EQ(importResult.data.meshes.size(), 1);

    // by default no cpu access to mesh data.
    const auto& meshData = importer->GetMeshData();
    EXPECT_TRUE(meshData.meshes.empty());

    delete gltf2;
}

/**
 * @tc.name: meshData
 * @tc.desc: Tests for Mesh Data. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_GLTFImporterTest, meshData, testing::ext::TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE_NS::GetLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    constexpr string_view filename = "test://gltf/WaterBottle/WaterBottle.glb";

    auto gltf = gltf2->LoadGLTF(filename);
    ASSERT_TRUE(gltf.success);

    auto importer = gltf2->CreateGLTF2Importer(*ecs);
    // import with CORE_GLTF_IMPORT_RESOURCE_MESH_CPU_ACCESS
    importer->ImportGLTF(
        *gltf.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL | CORE_GLTF_IMPORT_RESOURCE_MESH_CPU_ACCESS);

    auto importResult = importer->GetResult();
    ASSERT_TRUE(importResult.success);

    const auto& meshData = importer->GetMeshData();
    EXPECT_EQ(meshData.meshes.size(), 1U);
    for (const auto& mesh : meshData.meshes) {
        EXPECT_EQ(mesh.subMeshes.size(), 1U);
        for (const auto& submesh : mesh.subMeshes) {
            EXPECT_EQ(submesh.indices, 13530);
            EXPECT_EQ(submesh.vertices, 2549);
            EXPECT_EQ(submesh.indexBuffer.size(), sizeof(uint16_t) * submesh.indices);
            for (auto i = 0U; i < RENDER_NS::PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT; ++i) {
                EXPECT_TRUE(submesh.attributeBuffers[i].empty() ||
                            submesh.attributeBuffers[i].size() ==
                                meshData.vertexInputDeclaration.bindingDescriptions[i].stride * submesh.vertices);
            }
        }
    }

    delete gltf2;
}

namespace {
GLTFImportResult LoadAndImport(string_view filename, Gltf2& gltf2, IEcs& ecs)
{
    auto gltf = gltf2.LoadGLTF(filename);
    EXPECT_TRUE(gltf.success);
    if (gltf.success) {
        auto importer = gltf2.CreateGLTF2Importer(ecs);
        importer->ImportGLTF(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);

        GLTFImportResult result = importer->GetResult();
        EXPECT_TRUE(result.success);
        if (result.success) {
            auto root = gltf2.ImportGltfScene(gltf.data->GetDefaultSceneIndex(), *gltf.data, result.data, ecs, {},
                CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL);
            EXPECT_TRUE(EntityUtil::IsValid(root));
            GetManager<IRenderConfigurationComponentManager>(ecs)->Create(root);
        }
        return result;
    }
    GLTFImportResult error;
    error.success = false;
    return error;
}
} // namespace

/**
 * @tc.name: exportGltfObject
 * @tc.desc: Tests for Export Gltf Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_GLTFImporterTest, exportGltfObject, testing::ext::TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.
    //    test::LogLevelScope level(GetLogger(), ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    constexpr string_view filename = "test://gltf/WaterBottle/WaterBottle.glb";

    GLTFImportResult originalGlb = LoadAndImport(filename, *gltf2, *ecs);
    ASSERT_TRUE(originalGlb.success);
    ASSERT_EQ(originalGlb.data.textures.size(), 4);
    ASSERT_EQ(originalGlb.data.materials.size(), 1);
    ASSERT_EQ(originalGlb.data.meshes.size(), 1);

    constexpr string_view exportFilename = "cache://WaterBottle.glb";

    ASSERT_TRUE(gltf2->SaveGLTF(*ecs, exportFilename));

    GLTFImportResult exportedGlb = LoadAndImport(exportFilename, *gltf2, *ecs);
    ASSERT_TRUE(exportedGlb.success);
    ASSERT_EQ(originalGlb.data.textures.size(), exportedGlb.data.textures.size());
    ASSERT_EQ(originalGlb.data.materials.size(), exportedGlb.data.materials.size());
    ASSERT_EQ(originalGlb.data.meshes.size(), exportedGlb.data.meshes.size());
    ASSERT_EQ(originalGlb.data.images.size(), exportedGlb.data.images.size());

    delete gltf2;
}
