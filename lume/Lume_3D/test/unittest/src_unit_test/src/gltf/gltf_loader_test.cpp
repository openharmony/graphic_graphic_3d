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

#include <3d/gltf/gltf.h>
#include <core/image/intf_image_container.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/io/intf_file_manager.h>
#include <core/os/intf_platform.h>

#include "gltf/gltf2.h"
#include "gltf/gltf2_loader.h"
#include "gltf/gltf2_util.h"
#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;

namespace {
bool isExpectedAttribute(const GLTF2::AttributeBase& aAttribute, const GLTF2::AttributeType* aExpectedAttributes,
    int aNumberOfExpectedAttributes)
{
    bool result = false;

    for (int i = 0; i < aNumberOfExpectedAttributes; ++i) {
        if (aExpectedAttributes[i] == aAttribute.type) {
            result = true;
            break;
        }
    }

    return result;
}
void validateWaterBottle(GLTF2::Data const& aData)
{
    ASSERT_EQ(aData.accessors.size(), 5);
    ASSERT_EQ(aData.animations.size(), 0);
    ASSERT_EQ(aData.materials.size(), 1);
    ASSERT_EQ(aData.meshes.size(), 1);
    ASSERT_EQ(aData.buffers.size(), 1);
    ASSERT_EQ(aData.textures.size(), 4);
    ASSERT_EQ(aData.nodes.size(), 1);
    ASSERT_EQ(aData.images.size(), 4);
    ASSERT_EQ(aData.bufferViews.size(), 9);
    ASSERT_EQ(aData.scenes.size(), 1);
}

void validateMonster(GLTF2::Data const& aData)
{
    ASSERT_EQ(aData.scenes.size(), 1);
    ASSERT_EQ(aData.nodes.size(), 35);
    ASSERT_EQ(aData.meshes.size(), 1);
    ASSERT_EQ(aData.animations.size(), 1);
    ASSERT_EQ(aData.skins.size(), 1);
    ASSERT_EQ(aData.buffers.size(), 1);
    ASSERT_EQ(aData.materials.size(), 1);
    ASSERT_EQ(aData.textures.size(), 1);
    ASSERT_EQ(aData.images.size(), 1);
    ASSERT_EQ(aData.samplers.size(), 1);
    ASSERT_EQ(aData.bufferViews.size(), 8);
    ASSERT_EQ(aData.accessors.size(), 135);

    // Every primitive should have these attributes.
    GLTF2::AttributeType expectedAttributes[] = {
        GLTF2::AttributeType::NORMAL,
        GLTF2::AttributeType::POSITION,
        GLTF2::AttributeType::TEXCOORD,
        GLTF2::AttributeType::JOINTS,
        GLTF2::AttributeType::WEIGHTS,
    };

    int numberOfExpectedAttributes = sizeof(expectedAttributes) / sizeof(expectedAttributes[0]);

    // Go through meshes.
    for (auto& mesh : aData.meshes) {
        // Every mesh should have one primitive.
        ASSERT_EQ(mesh->primitives.size(), 1);

        const auto& primitive = mesh->primitives[0];
        for (const auto& attribute : primitive.attributes) {
            bool isExpected = isExpectedAttribute(attribute.attribute, expectedAttributes, numberOfExpectedAttributes);

            // Try to load attribute data and see if it matches the 'expected' state.
            GLTF2::GLTFLoadDataResult result = GLTF2::LoadData(*attribute.accessor);
            ASSERT_EQ(result.success, isExpected);

            if (isExpected) {
                // See that there is data.
                ASSERT_TRUE(result.elementCount > 0);
                ASSERT_TRUE(result.elementSize > 0);
                ASSERT_TRUE(result.data.size() > 0);
            }
        }

        // Load indices.
        GLTF2::GLTFLoadDataResult indicesLoadResult = GLTF2::LoadData(*primitive.indices);
        ASSERT_TRUE(indicesLoadResult.success);

        // See that there is data.
        ASSERT_TRUE(indicesLoadResult.elementCount > 0);
        ASSERT_TRUE(indicesLoadResult.elementSize > 0);
        ASSERT_TRUE(indicesLoadResult.data.size() > 0);
    }

    const auto& animation = *(aData.animations[0]);

    ASSERT_EQ(animation.samplers.size(), 96);
    ASSERT_EQ(animation.tracks.size(), 96);

    ASSERT_EQ(animation.samplers[0]->input, aData.accessors[6].get());
    ASSERT_EQ(animation.samplers[0]->interpolation, GLTF2::AnimationInterpolation::LINEAR);
    ASSERT_EQ(animation.samplers[0]->output, aData.accessors[7].get());

    ASSERT_EQ(animation.tracks[0].sampler, animation.samplers[0].get());
    ASSERT_EQ(animation.tracks[0].channel.node, aData.nodes[2].get());
    ASSERT_EQ(animation.tracks[0].channel.path, GLTF2::AnimationPath::TRANSLATION);
}
} // namespace

/**
 * @tc.name: loadGlbFile
 * @tc.desc: Tests for Load Glb File. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, loadGlbFile, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();

    string_view filename = "test://gltf/WaterBottle/WaterBottle.glb";

    auto gltf = GLTF2::LoadGLTF(files, filename);
    ASSERT_TRUE(gltf.success);

    // Ensure proper counts.
    validateWaterBottle(*gltf.data);
}

/**
 * @tc.name: loadInvalidGltfFile
 * @tc.desc: Tests for Load Invalid Gltf File. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, loadInvalidGltfFile, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();

    {
        string filename = "test://gltf/Invalid/Monster_invalid_bufferview.gltf";
        auto gltf = GLTF2::LoadGLTF(files, filename);

        ASSERT_FALSE(gltf.success);
    }

    {
        string filename = "test://gltf/Invalid/Duck_GLTFv1.gltf";
        auto gltf = GLTF2::LoadGLTF(files, filename);

        ASSERT_FALSE(gltf.success);
    }
}

/**
 * @tc.name: loadInvalidGlbFile
 * @tc.desc: Tests for Load Invalid Glb File. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, loadInvalidGlbFile, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();

    {
        string_view filename = "test://gltf/Invalid/Duck_invalid_bufferview.glb";
        auto gltf = GLTF2::LoadGLTF(files, filename);

        ASSERT_FALSE(gltf.success);
    }

    {
        string_view filename = "test://gltf/Invalid/Duck_GLTFv1.glb";
        auto gltf = GLTF2::LoadGLTF(files, filename);

        ASSERT_FALSE(gltf.success);
    }
}

/**
 * @tc.name: loadGltfWithExternallyReferencedData
 * @tc.desc: Tests for Load Gltf With Externally Referenced Data. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, loadGltfWithExternallyReferencedData, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();

    string_view filename = "test://gltf/FlightHelmet/FlightHelmet.gltf";

    auto gltf = GLTF2::LoadGLTF(files, filename);
    ASSERT_TRUE(gltf.success);

    ASSERT_TRUE(gltf.data->LoadBuffers());

    // Ensure proper counts.
    ASSERT_EQ(gltf.data->animations.size(), 0);
    ASSERT_EQ(gltf.data->materials.size(), 5);
    ASSERT_EQ(gltf.data->meshes.size(), 5);
    ASSERT_EQ(gltf.data->nodes.size(), 6);
    ASSERT_EQ(gltf.data->textures.size(), 15);

    // Every primitive should have these attributes.
    GLTF2::AttributeType expectedAttributes[] = {
        GLTF2::AttributeType::NORMAL,
        GLTF2::AttributeType::POSITION,
        GLTF2::AttributeType::TANGENT,
        GLTF2::AttributeType::TEXCOORD,
    };

    int numberOfExpectedAttributes = sizeof(expectedAttributes) / sizeof(expectedAttributes[0]);

    // Go through meshes.
    for (auto& mesh : gltf.data->meshes) {
        // Every mesh should have one primitive.
        ASSERT_EQ(mesh->primitives.size(), 1);

        const auto& primitive = mesh->primitives[0];
        for (const auto& attribute : primitive.attributes) {
            bool isExpected = isExpectedAttribute(attribute.attribute, expectedAttributes, numberOfExpectedAttributes);

            // Try to load attribute data and see if it matches the 'expected' state.
            GLTF2::GLTFLoadDataResult result = GLTF2::LoadData(*attribute.accessor);
            ASSERT_EQ(result.success, isExpected);

            if (isExpected) {
                // See that there is data.
                ASSERT_TRUE(result.elementCount > 0);
                ASSERT_TRUE(result.elementSize > 0);
                ASSERT_TRUE(result.data.size() > 0);
            }
        }

        // Load indices.
        GLTF2::GLTFLoadDataResult indicesLoadResult = GLTF2::LoadData(*primitive.indices);
        ASSERT_TRUE(indicesLoadResult.success);

        // See that there is data.
        ASSERT_TRUE(indicesLoadResult.elementCount > 0);
        ASSERT_TRUE(indicesLoadResult.elementSize > 0);
        ASSERT_TRUE(indicesLoadResult.data.size() > 0);
    }
}

/**
 * @tc.name: loadGltfWithBase64EncodedData
 * @tc.desc: Tests for Load Gltf With Base64Encoded Data. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, loadGltfWithBase64EncodedData, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();

    string_view filename = "test://gltf/Monster/Monster.gltf";

    auto gltf = GLTF2::LoadGLTF(files, filename);
    ASSERT_TRUE(gltf.success);

    ASSERT_TRUE(gltf.data->LoadBuffers());

    // Ensure proper counts.
    validateMonster(*gltf.data);
}

/**
 * @tc.name: loadGltfWithExtensions
 * @tc.desc: Tests for Load Gltf With Extensions. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, loadGltfWithExtensions, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();

    string_view filename = "test://gltf/Extensions/laohu.glb";

    auto gltf = GLTF2::LoadGLTF(files, filename);

    ASSERT_TRUE(gltf.success);

    // Ensure proper counts.
    ASSERT_EQ(gltf.data->scenes.size(), 1);
    ASSERT_EQ(gltf.data->nodes.size(), 20);
    ASSERT_EQ(gltf.data->meshes.size(), 3);
    ASSERT_EQ(gltf.data->samplers.size(), 4);
    ASSERT_EQ(gltf.data->buffers.size(), 1);
    ASSERT_EQ(gltf.data->materials.size(), 1);
    ASSERT_EQ(gltf.data->textures.size(), 4);
    ASSERT_EQ(gltf.data->images.size(), 4);
    ASSERT_EQ(gltf.data->bufferViews.size(), 81);
    ASSERT_EQ(gltf.data->accessors.size(), 77);

#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
    ASSERT_EQ(gltf.data->lights.size(), 0); // IGFX Lights extension is not supported anylonger so light count is zero.
#endif
}

/**
 * @tc.name: testGltfLoadingApi
 * @tc.desc: Tests for Test Gltf Loading Api. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, testGltfLoadingApi, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();

    string_view filename = "test://gltf/FlightHelmet/FlightHelmet.gltf";
    Gltf2 g(files);

    auto gltf = g.LoadGLTF(filename);
    ASSERT_TRUE(gltf.success);
}

/**
 * @tc.name: loadFromMemoryGlbFile
 * @tc.desc: Tests for Load From Memory Glb File. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, loadFromMemoryGlbFile, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();

    string_view filename = "test://gltf/WaterBottle/WaterBottle.glb";
    Gltf2 g(files);

    auto const file = files.OpenFile(filename);
    auto const fileSize = file->GetLength();
    auto fileBuffer = std::vector<uint8_t>(fileSize);
    file->Read(fileBuffer.data(), fileBuffer.size());
    auto igltf = g.LoadGLTF(array_view<uint8_t const>(fileBuffer.data(), fileBuffer.size()));

    ASSERT_TRUE(igltf.success);
    const auto& data = (GLTF2::Data&)(*igltf.data);

    // Ensure proper counts.
    validateWaterBottle(data);
}

/**
 * @tc.name: loadFromMemoryGltfWithBase64EncodedData
 * @tc.desc: Tests for Load From Memory Gltf With Base64Encoded Data. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, loadFromMemoryGltfWithBase64EncodedData, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();

    string_view filename = "test://gltf/Monster/Monster.gltf";
    Gltf2 g(files);

    auto const file = files.OpenFile(filename);
    auto const fileSize = file->GetLength();
    auto fileBuffer = std::vector<uint8_t>(fileSize);
    file->Read(fileBuffer.data(), fileBuffer.size());
    auto igltf = g.LoadGLTF(array_view<uint8_t const>(fileBuffer.data(), fileBuffer.size()));

    ASSERT_TRUE(igltf.success);

    ASSERT_TRUE(igltf.data->LoadBuffers());

    const auto& data = (GLTF2::Data&)(*igltf.data);

    // Ensure proper counts.
    validateMonster(data);
}

/**
 * @tc.name: loadThumbnails
 * @tc.desc: Tests for Load Thumbnails. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, loadThumbnails, testing::ext::TestSize.Level1)
{
    auto& files = UTest::GetTestContext()->engine->GetFileManager();
    auto* imageManager = &(UTest::GetTestContext()->engine->GetImageLoaderManager());
    string_view filename = "test://gltf/Thumbnail/thumbnail.glb";
    Gltf2 g(files);

    auto gltf = g.LoadGLTF(filename);
    ASSERT_TRUE(gltf.success);
    ASSERT_TRUE(gltf.data->GetThumbnailImageCount() > 0);

    // Check for thumbs.
    for (size_t thumbIndex = 0; thumbIndex < gltf.data->GetThumbnailImageCount(); ++thumbIndex) {
        auto thumbnail = gltf.data->GetThumbnailImage(thumbIndex);
        ASSERT_TRUE(thumbnail.extension == "png");

        if (!thumbnail.data.empty()) {
            auto result = imageManager->LoadImage(thumbnail.data, 0);
            ASSERT_TRUE(result.success);

            ASSERT_TRUE(result.image->GetImageDesc().width > 0);
            ASSERT_TRUE(result.image->GetImageDesc().height > 0);
        }
    }
}

/**
 * @tc.name: NonExistingFileTest
 * @tc.desc: Tests for Non Existing File Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, NonExistingFileTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    auto gltf = gltf2->LoadGLTF("nonExistingFile.gltf");
    EXPECT_FALSE(gltf.success);

    delete gltf2;
}

/**
 * @tc.name: InvalidJsonTest
 * @tc.desc: Tests for Invalid Json Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, InvalidJsonTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\",}}";
    auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
    tmpFile->Write(jsonStr.data(), jsonStr.size());
    tmpFile->Close();

    auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
    EXPECT_FALSE(gltf.success);

    engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    delete gltf2;
}

/**
 * @tc.name: InvalidSceneTest
 * @tc.desc: Tests for Invalid Scene Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, InvalidSceneTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"scene\": []}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"scenes\": [{\"extensions\": {\"EXT_lights_image_based\": -1}}]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    delete gltf2;
}

/**
 * @tc.name: InvalidAccessorTest
 * @tc.desc: Tests for Invalid Accessor Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, InvalidAccessorTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"accessors\": [{\"componentType\": 0, \"count\": 0, \"type\": "
            "\"abcd\", \"bufferView\": 15, \"byteOffset\": {}, \"normalized\": [], \"min\": \"xyz\", \"max\": "
            "\"xyz\"}]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"accessors\": [{\"componentType\": 5126, \"count\": 1, \"type\": "
            "\"VEC3\", \"min\": [0, 0], \"max\": [1, 1]}]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"accessors\": [{\"componentType\": 5126, \"count\": 1, \"type\": "
            "\"VEC3\", \"min\": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], \"max\": [1, 1, {}, []]}]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"accessors\": [{\"componentType\": 5126, \"count\": 1, \"type\": "
            "\"VEC3\", \"min\": [0, 0, 0], \"max\": [1, 1]}]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"accessors\": [{\"sparse\": {\"count\": []} }]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"accessors\": [{\"sparse\": {\"count\": 1} }]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"accessors\": [{\"sparse\": {\"count\": 1, \"indices\": {}} }]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"accessors\": [{\"sparse\": "
                                              "{\"count\": 1, \"indices\": {\"bufferView\": 15}} }]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"buffers\": [{\"byteLength\": 128, \"uri\": \"buffer.bin\"}], "
            "\"bufferViews\": [{\"buffer\": 0, \"byteLength\": 128, \"byteStride\": 4}], \"accessors\": [{\"sparse\": "
            "{\"count\": 1, \"indices\": {\"bufferView\": 0, \"componentType\": []}} }]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"buffers\": [{\"byteLength\": 128, \"uri\": \"buffer.bin\"}], "
            "\"bufferViews\": [{\"buffer\": 0, \"byteLength\": 128, \"byteStride\": 4}], \"accessors\": [{\"sparse\": "
            "{\"count\": 1, \"indices\": {\"bufferView\": 0, \"componentType\": 5126}} }]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"buffers\": [{\"byteLength\": 128, \"uri\": \"buffer.bin\"}], "
            "\"bufferViews\": [{\"buffer\": 0, \"byteLength\": 128, \"byteStride\": 4}], \"accessors\": [{\"sparse\": "
            "{\"count\": 1, \"indices\": {\"bufferView\": 0, \"componentType\": 5126}, \"values\": {}} }]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"buffers\": [{\"byteLength\": 128, \"uri\": \"buffer.bin\"}], "
            "\"bufferViews\": [{\"buffer\": 0, \"byteLength\": 128, \"byteStride\": 4}], \"accessors\": [{\"sparse\": "
            "{\"count\": 1, \"indices\": {\"bufferView\": 0, \"componentType\": 5126}, \"values\": {\"bufferView\": "
            "15}} }]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"accessors\": [{\"componentType\": \"abcd\"}]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"accessors\": -1}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    delete gltf2;
}

/**
 * @tc.name: InvalidMeshTest
 * @tc.desc: Tests for Invalid Mesh Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, InvalidMeshTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"meshes\": [{\"primitives\": [{\"mode\": 8}]}]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"meshes\": [{\"primitives\": [{\"mode\": 8, \"attributes\": {}}]}]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"buffers\": [{\"byteLength\": 128, \"uri\": "
            "\"buffer.bin\"}],\"bufferViews\": [{\"buffer\": 0, \"byteLength\": 128, \"byteStride\": 4}], "
            "\"accessors\": [{\"bufferView\": 0, \"count\": 1, \"componentType\": 5126, \"type\": \"VEC3\", \"min\": "
            "[0, 0, 0], \"max\": [1, 1, 1]}], \"meshes\": [{\"primitives\": [{\"mode\": 8, \"attributes\": "
            "{\"POSITION\": 0}}]}]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"extensionsRequired\": [\"KHR_mesh_quantization\"], \"accessors\": "
            "[{}], \"meshes\": [{\"primitives\": [{\"attributes\": {\"POSITION\": 0}}]}]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"extensionsRequired\": [\"KHR_mesh_quantization\"], \"accessors\": "
            "[{}], \"meshes\": [{\"primitives\": [{\"attributes\": {\"POSITION\": 0}}]}]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"extensionsRequired\": [\"KHR_mesh_quantization\"], \"buffers\": "
            "[{\"byteLength\": 128, \"uri\": \"buffer.bin\"}],\"bufferViews\": [{\"buffer\": 0, \"byteLength\": 128, "
            "\"byteStride\": 4}], \"accessors\": [{\"bufferView\": 0, \"count\": 1, \"componentType\": 5126, \"type\": "
            "\"VEC3\", \"min\": [0, 0, 0], \"max\": [1, 1, 1]}, {}], \"meshes\": [{\"primitives\": [{\"attributes\": "
            "{\"POSITION\": 0}, \"targets\": [{\"POSITION\": 1}]}]}]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"extensionsRequired\": [\"KHR_mesh_quantization\"], \"buffers\": "
            "[{\"byteLength\": 128, \"uri\": \"buffer.bin\"}],\"bufferViews\": [{\"buffer\": 0, \"byteLength\": 128, "
            "\"byteStride\": 4}], \"accessors\": [{\"bufferView\": 0, \"count\": 1, \"componentType\": 5126, \"type\": "
            "\"VEC3\", \"min\": [0, 0, 0], \"max\": [1, 1, 1]}, {}], \"meshes\": [{\"primitives\": [{\"attributes\": "
            "{\"POSITION\": 0}, \"targets\": [{\"INVALID_ATTR\": 0}]}]}]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"extensionsRequired\": [\"KHR_mesh_quantization\"], \"buffers\": "
            "[{\"byteLength\": 128, \"uri\": \"buffer.bin\"}],\"bufferViews\": [{\"buffer\": 0, \"byteLength\": 128, "
            "\"byteStride\": 4}], \"accessors\": [{\"bufferView\": 0, \"count\": 1, \"componentType\": 5126, \"type\": "
            "\"VEC3\", \"min\": [0, 0, 0], \"max\": [1, 1, 1]}, {}], \"meshes\": [{\"primitives\": [{\"attributes\": "
            "{\"POSITION\": 0}, \"targets\": [{\"POSITION\": 0}]}, {\"attributes\": {\"POSITION\": 0}, \"targets\": "
            "[]}]}]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"meshes\": [{\"name\": \"\\\"\"}, {\"name\": {}}]}";
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    delete gltf2;
}

/**
 * @tc.name: InvalidBufferTest
 * @tc.desc: Tests for Invalid Buffer Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, InvalidBufferTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"buffers\": [{\"byteLength\": {}, \"uri\": \"buffer.bin\"}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"buffers\": [{\"byteLength\": -1, \"uri\": \"buffer.bin\"}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    delete gltf2;
}

/**
 * @tc.name: InvalidBufferViewTest
 * @tc.desc: Tests for Invalid Buffer View Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, InvalidBufferViewTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"bufferViews\": [{\"buffer\": {}, \"byteLength\": 0, \"byteOffset\": "
            "-1, \"byteStride\": 3, \"target\": 15}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"bufferViews\": [{\"buffer\": {}, \"byteLength\": [], "
            "\"byteOffset\": -1, \"byteStride\": 3, \"target\": 15}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    delete gltf2;
}

/**
 * @tc.name: InvalidCameraTest
 * @tc.desc: Tests for Invalid Camera Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, InvalidCameraTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"cameras\": [{\"type\": \"invalid\"}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"cameras\": [{\"type\": "
                                              "\"perspective\", \"perspective\": {\"znear\": -6}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"cameras\": [{\"type\": "
                                              "\"orthographic\", \"orthographic\": {\"xmag\": 0}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    delete gltf2;
}

/**
 * @tc.name: InvalidNodeTest
 * @tc.desc: Tests for Invalid Node Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, InvalidNodeTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"nodes\": [{\"children\": [1, 2, 100]}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"nodes\": [{\"extensions\": []}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"nodes\": [{\"weights\": [1, 5]}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"nodes\": [{\"extensions\": "
                                              "{\"KHR_lights_punctual\": {\"light\": 5}}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"nodes\": [{\"camera\": 16}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    delete gltf2;
}

/**
 * @tc.name: InvalidAssetTest
 * @tc.desc: Tests for Invalid Asset Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, InvalidAssetTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    {
        constexpr const string_view jsonStr = "{}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"minVersion\": \"3.1\", \"version\": \"1.0\"}}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"minVersion\": \"9321\", \"version\": \"1.0\"}}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"7.3\"}}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": []}}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    delete gltf2;
}

/**
 * @tc.name: InvalidAnimationTest
 * @tc.desc: Tests for Invalid Animation Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, DISABLED_InvalidAnimationTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"animations\": [{}, {\"name\": {}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"animations\": [{\"channels\": [{\"target\": {}}]}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"animations\": [{\"channels\": "
                                              "[{\"target\": {\"path\": \"invalid\"}}]}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    delete gltf2;
}

/**
 * @tc.name: InvalidLightTest
 * @tc.desc: Tests for Invalid Light Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, InvalidLightTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"extensions\": "
                                              "{\"KHR_lights_punctual\": {\"lights\": [{\"type\": \"invalid\"}]}}}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"extensions\": "
                                              "{\"EXT_lights_image_based\": {\"lights\": [{\"rotation\": [0, 0]}]}}}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    delete gltf2;
}

/**
 * @tc.name: InvalidTextureTest
 * @tc.desc: Tests for Invalid Texture Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, InvalidTextureTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"textures\": [{\"sampler\": 32, \"source\": 43}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    delete gltf2;
}

/**
 * @tc.name: InvalidMaterialTest
 * @tc.desc: Tests for Invalid Material Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, InvalidMaterialTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"materials\": [{\"emissiveFactor\": {}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"materials\": [{\"alphaMode\": \"invalid\"}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"materials\": [{\"extensions\": {\"KHR_materials_unlit\": []}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"materials\": [{\"extensions\": "
                                              "{\"KHR_materials_transmission\": {\"transmissionTexture\": []}}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"materials\": [{\"extensions\": "
                                              "{\"KHR_materials_specular\": {\"specularColorTexture\": []}}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"materials\": [{\"extensions\": "
                                              "{\"KHR_materials_sheen\": {\"sheenRoughnessTexture\": []}}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"materials\": [{\"extensions\": "
            "{\"KHR_materials_pbrSpecularGlossiness\": {\"specularGlossinessTexture\": []}}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"materials\": [{\"extensions\": "
            "{\"KHR_materials_clearcoat\": {\"clearcoatNormalTexture\": {\"extensions\": []}}}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"materials\": [{\"extras\": {\"clearCoat\": []}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"materials\": [{\"emissiveTexture\": {\"index\": {}}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"materials\": [{\"occlusionTexture\": {\"strength\": {}}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"materials\": [{\"normalTexture\": {\"scale\": {}}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"materials\": "
                                              "[{\"pbrMetallicRoughness\": {\"roughnessFactor\": {}}}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    delete gltf2;
}

/**
 * @tc.name: InvalidSamplerTest
 * @tc.desc: Tests for Invalid Sampler Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, InvalidSamplerTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    {
        constexpr const string_view jsonStr =
            "{\"asset\": {\"version\": \"1.0\"}, \"samplers\": [{\"magFilter\": 33}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    {
        constexpr const string_view jsonStr = "{\"asset\": {\"version\": \"1.0\"}, \"samplers\": [{\"wrapS\": 88}]}";

        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.gltf");
        tmpFile->Write(jsonStr.data(), jsonStr.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.gltf");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.gltf");
    }

    delete gltf2;
}

/**
 * @tc.name: InvalidGlbTest
 * @tc.desc: Tests for Invalid Glb Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFLoaderTest, InvalidGlbTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    {
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.glb");
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.glb");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.glb");
    }

    {
        vector<uint8_t> data;
        data.resize(100);
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.glb");
        tmpFile->Write(data.data(), data.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.glb");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.glb");
    }

    {
        GLTF2::GLBHeader header;
        header.magic = GLTF2::GLTF_MAGIC;
        header.version = 2;
        header.length = 1000;
        vector<uint8_t> data;
        data.resize(100);
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.glb");
        tmpFile->Write(reinterpret_cast<uint8_t*>(&header), sizeof(header));
        tmpFile->Write(data.data(), data.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.glb");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.glb");
    }

    {
        GLTF2::GLBHeader header;
        header.magic = GLTF2::GLTF_MAGIC;
        header.version = 2;
        header.length = 2;
        vector<uint8_t> data;
        data.resize(2);
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.glb");
        tmpFile->Write(reinterpret_cast<uint8_t*>(&header), sizeof(header));
        tmpFile->Write(data.data(), data.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.glb");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.glb");
    }

    {
        GLTF2::GLBHeader header;
        header.magic = GLTF2::GLTF_MAGIC;
        header.version = 2;
        header.length = 2;
        GLTF2::GLBChunk chunk;
        chunk.chunkType = 7;
        chunk.chunkLength = 100;
        vector<uint8_t> data;
        data.resize(100);
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.glb");
        tmpFile->Write(reinterpret_cast<uint8_t*>(&header), sizeof(header));
        tmpFile->Write(reinterpret_cast<uint8_t*>(&chunk), sizeof(chunk));
        tmpFile->Write(data.data(), data.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.glb");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.glb");
    }

    {
        GLTF2::GLBHeader header;
        header.magic = GLTF2::GLTF_MAGIC;
        header.version = 2;
        header.length = 2;
        GLTF2::GLBChunk chunk;
        chunk.chunkType = static_cast<uint32_t>(GLTF2::ChunkType::JSON);
        chunk.chunkLength = 100;
        vector<uint8_t> data;
        data.resize(100);
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.glb");
        tmpFile->Write(reinterpret_cast<uint8_t*>(&header), sizeof(header));
        tmpFile->Write(reinterpret_cast<uint8_t*>(&chunk), sizeof(chunk));
        tmpFile->Write(data.data(), data.size());
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.glb");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.glb");
    }

    {
        GLTF2::GLBHeader header;
        header.magic = GLTF2::GLTF_MAGIC;
        header.version = 2;
        header.length = 128;
        GLTF2::GLBChunk chunk;
        chunk.chunkType = static_cast<uint32_t>(GLTF2::ChunkType::JSON);
        chunk.chunkLength = 100;
        vector<uint8_t> data;
        data.resize(100);
        auto tmpFile = engine->GetFileManager().CreateFile("cache://tmp.glb");
        tmpFile->Write(reinterpret_cast<uint8_t*>(&header), sizeof(header));
        tmpFile->Write(reinterpret_cast<uint8_t*>(&chunk), sizeof(chunk));
        tmpFile->Write(data.data(), data.size());
        tmpFile->Write(reinterpret_cast<uint8_t*>(&chunk), sizeof(chunk));
        tmpFile->Close();

        auto gltf = gltf2->LoadGLTF("cache://tmp.glb");
        EXPECT_FALSE(gltf.success);

        engine->GetFileManager().DeleteFile("cache://tmp.glb");
    }

    delete gltf2;
}
