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
#include <3d/ecs/components/render_handle_component.h>
#include <3d/gltf/gltf.h>
#include <3d/gltf/gltf_data.h>
#include <3d/intf_graphics_context.h>
#include <3d/render/default_material_constants.h>
#include <core/ecs/intf_ecs.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/os/intf_platform.h>
#include <core/plugin/intf_plugin_register.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>

#include "gltf/gltf2.h"
#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;
using namespace RENDER_NS;

namespace {
constexpr string_view WATER_BOTTLE_GLTF = "test://gltf/WaterBottle/WaterBottle.glb";

class ImportListener final : public IGLTF2Importer::Listener {
public:
    void OnImportStarted() override
    {}
    void OnImportFinished() override
    {
        done = true;
    }
    void OnImportProgressed(size_t taskIndex, size_t taskCount) override
    {}

    bool IsDone() const
    {
        return done;
    }

private:
    bool done{false};
};

size_t FindImageIndex(const GLTF2::GltfData& data, const GLTF2::Image* image)
{
    for (size_t i = 0U; i < data.images.size(); ++i) {
        if (data.images[i].get() == image) {
            return i;
        }
    }
    return GLTF2::GLTF_INVALID_INDEX;
}

size_t FindBaseColorImageIndex(const GLTF2::GltfData& data)
{
    if (data.materials.empty()) {
        return GLTF2::GLTF_INVALID_INDEX;
    }
    const auto& baseColorTexture = data.materials[0]->metallicRoughness.baseColorTexture;
    if (!baseColorTexture.texture || !baseColorTexture.texture->image) {
        return GLTF2::GLTF_INVALID_INDEX;
    }
    return FindImageIndex(data, baseColorTexture.texture->image);
}

RenderHandleReference GetImportedImageHandle(IEcs& ecs, Entity imageEntity)
{
    auto* renderHandleManager = GetManager<IRenderHandleComponentManager>(ecs);
    if (!renderHandleManager || !EntityUtil::IsValid(imageEntity)) {
        return {};
    }
    if (!renderHandleManager->HasComponent(imageEntity)) {
        return {};
    }

    return renderHandleManager->Get(imageEntity).reference;
}

GpuImageDesc GetImportedImageDesc(IRenderContext& renderContext, const RenderHandleReference& imageHandle)
{
    return renderContext.GetDevice().GetGpuResourceManager().GetImageDescriptor(imageHandle);
}

GLTFImportResult ImportWithOptions(Gltf2& gltf2, IEcs& ecs, const IGLTFData& data, const GltfImportOptions& options)
{
    auto importer = gltf2.CreateGLTF2Importer(ecs);
    importer->ImportGLTF(data, options);
    return importer->GetResult();
}

uint32_t GetBindingStrideForLocation(const VertexInputDeclarationView& vertexInputDeclaration, uint32_t location)
{
    for (const auto& attribute : vertexInputDeclaration.attributeDescriptions) {
        if (attribute.location != location) {
            continue;
        }
        for (const auto& binding : vertexInputDeclaration.bindingDescriptions) {
            if (binding.binding == attribute.binding) {
                return binding.stride;
            }
        }
    }
    return 0U;
}

void ExpectVidBindings(
    const VertexInputDeclarationView& expectedVertexInput, const VertexInputDeclarationData& actualVertexInput)
{
    for (size_t i = 0U; i < expectedVertexInput.bindingDescriptions.size(); ++i) {
        const auto& expected = expectedVertexInput.bindingDescriptions[i];
        const auto& actual = actualVertexInput.bindingDescriptions[i];
        EXPECT_EQ(expected.binding, actual.binding);
        EXPECT_EQ(expected.stride, actual.stride);
        EXPECT_EQ(expected.vertexInputRate, actual.vertexInputRate);
    }
}

void ExpectVidAttributes(
    const VertexInputDeclarationView& expectedVertexInput, const VertexInputDeclarationData& actualVertexInput)
{
    for (size_t i = 0U; i < expectedVertexInput.attributeDescriptions.size(); ++i) {
        const auto& expected = expectedVertexInput.attributeDescriptions[i];
        const auto& actual = actualVertexInput.attributeDescriptions[i];
        EXPECT_EQ(expected.location, actual.location);
        EXPECT_EQ(expected.binding, actual.binding);
        EXPECT_EQ(expected.format, actual.format);
        EXPECT_EQ(expected.offset, actual.offset);
    }
}

void ExpectInvalidVidFails(Gltf2& gltf2, IEcs& ecs, const IGLTFData& data, GltfImportOptions& options)
{
    options.vertexInputDeclarationPath = "3dvertexinputdeclarations://missing_importer_test.shadervid";
    auto invalidImporter = gltf2.CreateGLTF2Importer(ecs);
    invalidImporter->ImportGLTF(data, options);
    const GLTFImportResult invalidResult = invalidImporter->GetResult();
    EXPECT_FALSE(invalidResult.success);
    EXPECT_FALSE(invalidResult.error.empty());

    ImportListener invalidListener{};
    auto invalidAsyncImporter = gltf2.CreateGLTF2Importer(ecs);
    invalidAsyncImporter->ImportGLTFAsync(data, options, &invalidListener);
    EXPECT_TRUE(invalidAsyncImporter->Execute(0));
    EXPECT_TRUE(invalidListener.IsDone());
    const GLTFImportResult invalidAsyncResult = invalidAsyncImporter->GetResult();
    EXPECT_FALSE(invalidAsyncResult.success);
    EXPECT_FALSE(invalidAsyncResult.error.empty());
}
}  // namespace

/**
 * @tc.name: importGltfObject
 * @tc.desc: Tests for Import Gltf Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_GLTFImporterTest, importGltfObject, testing::ext::TestSize.Level1)
{
    // Show only warnings and errors because we don't want to see the normal output this test generates.
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE3D_NS::GetPluginLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    auto gltf = gltf2->LoadGLTF(WATER_BOTTLE_GLTF);
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
        ::Test::LogLevelScope(CORE3D_NS::GetPluginLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    auto gltf = gltf2->LoadGLTF(WATER_BOTTLE_GLTF);
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
            auto root = gltf2.ImportGltfScene(gltf.data->GetDefaultSceneIndex(),
                *gltf.data,
                result.data,
                ecs,
                {},
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
}  // namespace

/**
 * @tc.name: defaultOptionsCaching
 * @tc.desc: Tests that default options preserve flags-only import behavior.
 *
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_GLTFImporterTest, defaultOptionsCaching, testing::ext::TestSize.Level1)
{
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE3D_NS::GetPluginLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    auto gltf = gltf2->LoadGLTF(WATER_BOTTLE_GLTF);
    ASSERT_TRUE(gltf.success);

    auto baselineImporter = gltf2->CreateGLTF2Importer(*ecs);
    baselineImporter->ImportGLTF(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL);
    const GLTFImportResult baselineResult = baselineImporter->GetResult();
    ASSERT_TRUE(baselineResult.success);

    GltfImportOptions options;
    const GLTFImportResult optionsResult = ImportWithOptions(*gltf2, *ecs, *gltf.data, options);
    ASSERT_TRUE(optionsResult.success);

    EXPECT_EQ(baselineResult.data.images.size(), optionsResult.data.images.size());
    EXPECT_EQ(baselineResult.data.textures.size(), optionsResult.data.textures.size());
    EXPECT_EQ(baselineResult.data.materials.size(), optionsResult.data.materials.size());
    EXPECT_EQ(baselineResult.data.meshes.size(), optionsResult.data.meshes.size());

    ASSERT_FALSE(baselineResult.data.images.empty());
    ASSERT_FALSE(optionsResult.data.images.empty());
    EXPECT_EQ(static_cast<Entity>(baselineResult.data.images[0]), static_cast<Entity>(optionsResult.data.images[0]));

    ASSERT_FALSE(baselineResult.data.meshes.empty());
    ASSERT_FALSE(optionsResult.data.meshes.empty());
    EXPECT_EQ(static_cast<Entity>(baselineResult.data.meshes[0]), static_cast<Entity>(optionsResult.data.meshes[0]));

    delete gltf2;
}

/**
 * @tc.name: cachedMeshImport
 * @tc.desc: Tests that flags-only mesh import can reuse cached meshes even if the legacy
 * VID override is invalid.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_GLTFImporterTest, cachedMeshImport, testing::ext::TestSize.Level1)
{
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE3D_NS::GetPluginLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    auto gltf = gltf2->LoadGLTF(WATER_BOTTLE_GLTF);
    ASSERT_TRUE(gltf.success);

    auto firstImporter = gltf2->CreateGLTF2Importer(*ecs);
    firstImporter->ImportGLTF(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_MESH);
    const GLTFImportResult firstResult = firstImporter->GetResult();
    ASSERT_TRUE(firstResult.success);
    ASSERT_FALSE(firstResult.data.meshes.empty());

    auto dataStorePod = refcnt_ptr<IRenderDataStorePod>(
        renderContext->GetRenderDataStoreManager().GetRenderDataStore("RenderDataStorePod"));
    ASSERT_TRUE(dataStorePod);

    constexpr string_view invalidVid = "3dvertexinputdeclarations://missing_cached_import_test.shadervid";
    dataStorePod->CreatePod("RenderDataStorePod",
        "VertexInputDeclarations",
        array_view<const uint8_t>(reinterpret_cast<const uint8_t*>(invalidVid.data()), invalidVid.size()));

    auto secondImporter = gltf2->CreateGLTF2Importer(*ecs);
    secondImporter->ImportGLTF(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_MESH);
    const GLTFImportResult secondResult = secondImporter->GetResult();
    EXPECT_TRUE(secondResult.success) << secondResult.error.c_str();
    EXPECT_EQ(firstResult.data.meshes.size(), secondResult.data.meshes.size());
    if ((firstResult.data.meshes.size() == secondResult.data.meshes.size()) && !secondResult.data.meshes.empty()) {
        EXPECT_EQ(static_cast<Entity>(firstResult.data.meshes[0]), static_cast<Entity>(secondResult.data.meshes[0]));
    }

    dataStorePod->DestroyPod("RenderDataStorePod", "VertexInputDeclarations");

    delete gltf2;
}

/**
 * @tc.name: cachedMeshData
 * @tc.desc: Tests that cached CPU-access mesh imports keep vertex input declaration data.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_GLTFImporterTest, cachedMeshData, testing::ext::TestSize.Level1)
{
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE3D_NS::GetPluginLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    auto gltf = gltf2->LoadGLTF(WATER_BOTTLE_GLTF);
    ASSERT_TRUE(gltf.success);

    constexpr GltfResourceImportFlags flags =
        CORE_GLTF_IMPORT_RESOURCE_MESH | CORE_GLTF_IMPORT_RESOURCE_MESH_CPU_ACCESS;
    auto firstImporter = gltf2->CreateGLTF2Importer(*ecs);
    firstImporter->ImportGLTF(*gltf.data, flags);
    const GLTFImportResult firstResult = firstImporter->GetResult();
    ASSERT_TRUE(firstResult.success);
    ASSERT_FALSE(firstResult.data.meshes.empty());

    auto secondImporter = gltf2->CreateGLTF2Importer(*ecs);
    secondImporter->ImportGLTF(*gltf.data, flags);
    const GLTFImportResult secondResult = secondImporter->GetResult();
    ASSERT_TRUE(secondResult.success);
    ASSERT_EQ(firstResult.data.meshes.size(), secondResult.data.meshes.size());
    ASSERT_FALSE(secondResult.data.meshes.empty());
    EXPECT_EQ(static_cast<Entity>(firstResult.data.meshes[0]), static_cast<Entity>(secondResult.data.meshes[0]));

    const GltfMeshData& meshData = secondImporter->GetMeshData();
    ASSERT_EQ(1U, meshData.meshes.size());
    ASSERT_FALSE(meshData.meshes[0].subMeshes.empty());
    EXPECT_GT(meshData.vertexInputDeclaration.bindingDescriptionCount, 0U);
    EXPECT_GT(meshData.vertexInputDeclaration.attributeDescriptionCount, 0U);
    const auto& submesh = meshData.meshes[0].subMeshes[0];
    EXPECT_EQ(sizeof(uint16_t) * submesh.indices, submesh.indexBuffer.size());
    EXPECT_NE(nullptr, submesh.indexBuffer.data());
    EXPECT_FALSE(submesh.attributeBuffers[MeshComponent::Submesh::DM_VB_POS].empty());
    EXPECT_NE(nullptr, submesh.attributeBuffers[MeshComponent::Submesh::DM_VB_POS].data());

    delete gltf2;
}

/**
 * @tc.name: textureOptionsMeshCache
 * @tc.desc: Tests that texture import options do not affect mesh-only resource
 * caching.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_GLTFImporterTest, textureOptionsMeshCache, testing::ext::TestSize.Level1)
{
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE3D_NS::GetPluginLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    auto gltf = gltf2->LoadGLTF(WATER_BOTTLE_GLTF);
    ASSERT_TRUE(gltf.success);

    auto firstImporter = gltf2->CreateGLTF2Importer(*ecs);
    firstImporter->ImportGLTF(*gltf.data, CORE_GLTF_IMPORT_RESOURCE_MESH);
    const GLTFImportResult firstResult = firstImporter->GetResult();
    ASSERT_TRUE(firstResult.success);
    ASSERT_FALSE(firstResult.data.meshes.empty());

    GltfImportOptions options;
    options.flags = CORE_GLTF_IMPORT_RESOURCE_MESH;
    options.maxTextureMipLevels = 1U;
    options.baseColorTextureColorSpace = GltfBaseColorTextureColorSpace::LINEAR;
    const GLTFImportResult secondResult = ImportWithOptions(*gltf2, *ecs, *gltf.data, options);
    ASSERT_TRUE(secondResult.success);
    ASSERT_EQ(firstResult.data.meshes.size(), secondResult.data.meshes.size());
    ASSERT_FALSE(secondResult.data.meshes.empty());
    EXPECT_EQ(static_cast<Entity>(firstResult.data.meshes[0]), static_cast<Entity>(secondResult.data.meshes[0]));

    delete gltf2;
}

/**
 * @tc.name: textureMipLimit
 * @tc.desc: Tests max texture mip levels import option.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_GLTFImporterTest, textureMipLimit, testing::ext::TestSize.Level1)
{
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE3D_NS::GetPluginLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    auto gltf = gltf2->LoadGLTF(WATER_BOTTLE_GLTF);
    ASSERT_TRUE(gltf.success);

    GltfImportOptions options;
    options.maxTextureMipLevels = 1U;
    const GLTFImportResult importResult = ImportWithOptions(*gltf2, *ecs, *gltf.data, options);
    ASSERT_TRUE(importResult.success);
    ASSERT_FALSE(importResult.data.images.empty());

    for (const Entity imageEntity : importResult.data.images) {
        const RenderHandleReference imageHandle = GetImportedImageHandle(*ecs, imageEntity);
        ASSERT_TRUE(imageHandle);
        const GpuImageDesc imageDesc = GetImportedImageDesc(*renderContext, imageHandle);
        ASSERT_NE(BASE_FORMAT_UNDEFINED, imageDesc.format);
        EXPECT_EQ(1U, imageDesc.mipCount);
        const EngineImageCreationFlags generateMipsFlag =
            EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS;
        EXPECT_EQ(0U, imageDesc.engineCreationFlags & generateMipsFlag);
    }

    delete gltf2;
}

/**
 * @tc.name: baseColorColorSpace
 * @tc.desc: Tests base color texture color space import option.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_GLTFImporterTest, baseColorColorSpace, testing::ext::TestSize.Level1)
{
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE3D_NS::GetPluginLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    auto gltf = gltf2->LoadGLTF(WATER_BOTTLE_GLTF);
    ASSERT_TRUE(gltf.success);
    auto& gltfData = const_cast<GLTF2::GltfData&>(gltf.data->GetData());
    ASSERT_FALSE(gltfData.materials.empty());
    gltfData.materials[0]->emissiveTexture = gltfData.materials[0]->metallicRoughness.baseColorTexture;
    const size_t baseColorImage = FindBaseColorImageIndex(gltfData);
    ASSERT_NE(GLTF2::GLTF_INVALID_INDEX, baseColorImage);

    GltfImportOptions options;
    options.maxTextureMipLevels = 1U;
    options.baseColorTextureColorSpace = GltfBaseColorTextureColorSpace::LINEAR;
    GLTFImportResult importResult = ImportWithOptions(*gltf2, *ecs, *gltf.data, options);
    ASSERT_TRUE(importResult.success);
    ASSERT_LT(baseColorImage, importResult.data.images.size());
    RenderHandleReference imageHandle = GetImportedImageHandle(*ecs, importResult.data.images[baseColorImage]);
    ASSERT_TRUE(imageHandle);
    GpuImageDesc imageDesc = GetImportedImageDesc(*renderContext, imageHandle);
    EXPECT_EQ(BASE_FORMAT_R8G8B8A8_UNORM, imageDesc.format);

    options.baseColorTextureColorSpace = GltfBaseColorTextureColorSpace::SRGB;
    importResult = ImportWithOptions(*gltf2, *ecs, *gltf.data, options);
    ASSERT_TRUE(importResult.success);
    ASSERT_LT(baseColorImage, importResult.data.images.size());
    imageHandle = GetImportedImageHandle(*ecs, importResult.data.images[baseColorImage]);
    ASSERT_TRUE(imageHandle);
    imageDesc = GetImportedImageDesc(*renderContext, imageHandle);
    EXPECT_EQ(BASE_FORMAT_R8G8B8A8_SRGB, imageDesc.format);

    delete gltf2;
}

/**
 * @tc.name: asyncImportOptions
 * @tc.desc: Tests async import with GLTF import options.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_GLTFImporterTest, asyncImportOptions, testing::ext::TestSize.Level1)
{
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE3D_NS::GetPluginLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    auto gltf = gltf2->LoadGLTF(WATER_BOTTLE_GLTF);
    ASSERT_TRUE(gltf.success);

    auto& shaderManager = renderContext->GetDevice().GetShaderManager();
    const auto depthVertexInput = shaderManager.GetVertexInputDeclarationView(
        shaderManager.GetVertexInputDeclarationHandle(DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_DEPTH));
    ASSERT_FALSE(depthVertexInput.bindingDescriptions.empty());
    ASSERT_FALSE(depthVertexInput.attributeDescriptions.empty());

    GltfImportOptions options;
    options.flags = CORE_GLTF_IMPORT_RESOURCE_MESH | CORE_GLTF_IMPORT_RESOURCE_MESH_CPU_ACCESS;
    options.vertexInputDeclarationPath = DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_DEPTH;
    ImportListener listener{};
    auto importer = gltf2->CreateGLTF2Importer(*ecs);
    importer->ImportGLTFAsync(*gltf.data, options, &listener);
    while (!importer->Execute(0)) {
    }
    EXPECT_TRUE(listener.IsDone());

    const GLTFImportResult importResult = importer->GetResult();
    ASSERT_TRUE(importResult.success);
    const GltfMeshData& meshData = importer->GetMeshData();
    EXPECT_EQ(1U, meshData.meshes.size());
    EXPECT_EQ(depthVertexInput.bindingDescriptions.size(), meshData.vertexInputDeclaration.bindingDescriptionCount);
    EXPECT_EQ(depthVertexInput.attributeDescriptions.size(), meshData.vertexInputDeclaration.attributeDescriptionCount);

    delete gltf2;
}

/**
 * @tc.name: vertexInputDeclaration
 * @tc.desc: Tests vertex input declaration import option.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuTest_GLTFImporterTest, vertexInputDeclaration, testing::ext::TestSize.Level1)
{
    ::Test::LogLevelScope logLevel =
        ::Test::LogLevelScope(CORE3D_NS::GetPluginLogger(), CORE_NS::ILogger::LogLevel::LOG_WARNING);

    UTest::TestContext* testContext = UTest::GetTestContext();
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;
    auto gltf2 = new Gltf2(*graphicsContext);

    auto gltf = gltf2->LoadGLTF(WATER_BOTTLE_GLTF);
    ASSERT_TRUE(gltf.success);

    auto& shaderManager = renderContext->GetDevice().GetShaderManager();
    const auto depthVertexInput = shaderManager.GetVertexInputDeclarationView(
        shaderManager.GetVertexInputDeclarationHandle(DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_DEPTH));
    ASSERT_FALSE(depthVertexInput.bindingDescriptions.empty());
    ASSERT_FALSE(depthVertexInput.attributeDescriptions.empty());

    GltfImportOptions options;
    options.flags = CORE_GLTF_IMPORT_RESOURCE_MESH | CORE_GLTF_IMPORT_RESOURCE_MESH_CPU_ACCESS;
    options.vertexInputDeclarationPath = DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_DEPTH;

    auto importer = gltf2->CreateGLTF2Importer(*ecs);
    importer->ImportGLTF(*gltf.data, options);
    const GLTFImportResult importResult = importer->GetResult();
    ASSERT_TRUE(importResult.success) << importResult.error.c_str();
    const GltfMeshData& meshData = importer->GetMeshData();
    EXPECT_EQ(1U, meshData.meshes.size());
    EXPECT_EQ(depthVertexInput.bindingDescriptions.size(), meshData.vertexInputDeclaration.bindingDescriptionCount);
    EXPECT_EQ(depthVertexInput.attributeDescriptions.size(), meshData.vertexInputDeclaration.attributeDescriptionCount);
    ASSERT_FALSE(meshData.meshes.empty());
    ASSERT_FALSE(meshData.meshes[0].subMeshes.empty());
    const auto& submesh = meshData.meshes[0].subMeshes[0];
    const auto normalStride = GetBindingStrideForLocation(depthVertexInput, MeshComponent::Submesh::DM_VB_NOR);
    ASSERT_GT(normalStride, 0U);
    EXPECT_EQ(normalStride * submesh.vertices, submesh.attributeBuffers[MeshComponent::Submesh::DM_VB_NOR].size());
    ExpectVidBindings(depthVertexInput, meshData.vertexInputDeclaration);
    ExpectVidAttributes(depthVertexInput, meshData.vertexInputDeclaration);
    ExpectInvalidVidFails(*gltf2, *ecs, *gltf.data, options);

    delete gltf2;
}

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

    GLTFImportResult originalGlb = LoadAndImport(WATER_BOTTLE_GLTF, *gltf2, *ecs);
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
