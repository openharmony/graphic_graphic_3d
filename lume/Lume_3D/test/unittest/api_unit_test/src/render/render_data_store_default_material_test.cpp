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

#include <algorithm>

#include <3d/ecs/components/material_component.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_plugin.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/intf_renderer.h>

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

/**
 * @tc.name: CreateDestroyTest
 * @tc.desc: Tests for Create Destroy Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreDefaultMaterial, CreateDestroyTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    constexpr BASE_NS::string_view dataStoreName = "DataStoreDefaultMaterial0";
    auto dataStoreDefaultMaterial = dsManager.Create(IRenderDataStoreDefaultMaterial::UID, dataStoreName.data());
    ASSERT_TRUE(dataStoreDefaultMaterial);
    ASSERT_EQ("RenderDataStoreDefaultMaterial", dataStoreDefaultMaterial->GetTypeName());
    ASSERT_EQ(dataStoreName, dataStoreDefaultMaterial->GetName());
    ASSERT_EQ(IRenderDataStoreDefaultMaterial::UID, dataStoreDefaultMaterial->GetUid());
    ASSERT_EQ(0u, dataStoreDefaultMaterial->GetFlags());

    EXPECT_TRUE(dsManager.GetRenderDataStore(dataStoreName));
    // Destruction is deferred
    dataStoreDefaultMaterial.reset();
    // Render with no render node graph just to trigger destruction
    renderContext->GetRenderer().RenderFrame({});
    EXPECT_FALSE(dsManager.GetRenderDataStore(dataStoreName));
}

/**
 * @tc.name: DefaultMaterialTest
 * @tc.desc: Tests for Default Material Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreDefaultMaterial, DefaultMaterialTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    constexpr BASE_NS::string_view dataStoreName = "DataStoreDefaultMaterial0";
    auto dataStoreDefaultMaterial = dsManager.Create(IRenderDataStoreDefaultMaterial::UID, dataStoreName.data());
    ASSERT_TRUE(dataStoreDefaultMaterial);

    const uint32_t invalidId = ~0U; // built-in invalid
    auto ds = static_cast<IRenderDataStoreDefaultMaterial*>(dataStoreDefaultMaterial.get());
    const uint32_t materialIndex = ds->GetMaterialIndex(invalidId);
    EXPECT_EQ(0U, materialIndex);

    const auto& matUniforms = ds->GetMaterialUniforms();
    EXPECT_EQ(1U, matUniforms.size());
    if (!matUniforms.empty()) {
        const MaterialComponent mc;
        const auto& mat = matUniforms[0U];

        for (uint32_t idx = 0; idx < MaterialComponent::TextureIndex::TEXTURE_COUNT; ++idx) {
            EXPECT_EQ(mat.factors.factors[idx], mc.textures[idx].factor);
        }
        // alpha cut-off
        EXPECT_EQ(mat.factors.factors[RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT].x, mc.alphaCutoff);

        // needs to match
        const uint32_t mcLightingFlags = mc.materialLightingFlags;
        constexpr uint32_t expectedDefaultMc =
            MaterialComponent::LightingFlagBits::SHADOW_RECEIVER_BIT |
            MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT |
            MaterialComponent::LightingFlagBits::PUNCTUAL_LIGHT_RECEIVER_BIT |
            MaterialComponent::LightingFlagBits::INDIRECT_LIGHT_RECEIVER_BIT |
            MaterialComponent::LightingFlagBits::INDIRECT_IRRADIANCE_LIGHT_RECEIVER_BIT;
        EXPECT_EQ(mcLightingFlags, expectedDefaultMc);
        // needs to match
        const RenderDataDefaultMaterial::MaterialData mdDefault;
        const uint32_t dsLightingFlags = mdDefault.renderMaterialFlags;
        constexpr uint32_t expectedDefaultDs = RENDER_MATERIAL_SHADOW_RECEIVER_BIT | RENDER_MATERIAL_SHADOW_CASTER_BIT |
                                               RENDER_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT |
                                               RENDER_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT;
        EXPECT_EQ(dsLightingFlags, expectedDefaultDs);
    }

    EXPECT_TRUE(dsManager.GetRenderDataStore(dataStoreName));
    // Destruction is deferred
    dataStoreDefaultMaterial.reset();
    // Render with no render node graph just to trigger destruction
    renderContext->GetRenderer().RenderFrame({});
    EXPECT_FALSE(dsManager.GetRenderDataStore(dataStoreName));
}

/**
 * @tc.name: UpdateMaterialTest
 * @tc.desc: Tests for Update Material Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreDefaultMaterial, UpdateMaterialTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    constexpr BASE_NS::string_view dataStoreName = "DataStoreDefaultMaterial0";
    auto dataStoreDefaultMaterial = dsManager.Create(IRenderDataStoreDefaultMaterial::UID, dataStoreName.data());
    ASSERT_TRUE(dataStoreDefaultMaterial);

    auto ds = static_cast<IRenderDataStoreDefaultMaterial*>(dataStoreDefaultMaterial.get());
    const uint64_t matId = 4;
    ds->UpdateMaterialData(matId, {}, {}, {}, {});
    {
        const auto& matUniforms = ds->GetMaterialUniforms();
        EXPECT_EQ(2U, matUniforms.size());
    }

    RenderDataDefaultMaterial::InputMaterialUniforms imu;
    imu.alphaCutoff = 0.25f;
    imu.texCoordSetBits = 1U;
    imu.textureData[2U].factor = { 0.25f, 0.25f, 0.25f, 0.25f };
    const uint32_t matIdx = ds->UpdateMaterialData(matId, imu, {}, {}, {});
    {
        const auto& matUniforms = ds->GetMaterialUniforms();
        EXPECT_EQ(2U, matUniforms.size());

        for (uint32_t idx = 0; idx < RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT; ++idx) {
            EXPECT_EQ(imu.textureData[idx].factor, matUniforms[matIdx].factors.factors[idx]);
        }
    }

    EXPECT_TRUE(dsManager.GetRenderDataStore(dataStoreName));
    // Destruction is deferred
    dataStoreDefaultMaterial.reset();
    // Render with no render node graph just to trigger destruction
    renderContext->GetRenderer().RenderFrame({});
    EXPECT_FALSE(dsManager.GetRenderDataStore(dataStoreName));
}

/**
 * @tc.name: GetSetRenderSlotsTest
 * @tc.desc: Tests for Get Set Render Slots Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreDefaultMaterial, GetSetRenderSlotsTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    constexpr BASE_NS::string_view dataStoreName = "DataStoreDefaultMaterial0";
    auto dataStore = dsManager.Create(IRenderDataStoreDefaultMaterial::UID, dataStoreName.data());
    ASSERT_TRUE(dataStore);

    auto dataStoreDefaultMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(dataStore.get());

    {
        uint32_t slots[] = { 0u, 3u, 12u };
        dataStoreDefaultMaterial->SetRenderSlots(
            RenderDataDefaultMaterial::MaterialSlotType::SLOT_TYPE_OPAQUE, { slots, countof(slots) });
        auto slotMask =
            dataStoreDefaultMaterial->GetRenderSlotMask(RenderDataDefaultMaterial::MaterialSlotType::SLOT_TYPE_OPAQUE);
        for (uint32_t slot : slots) {
            EXPECT_TRUE((1ULL << uint64_t(slot)) & slotMask);
        }
    }
    {
        uint32_t slots[] = { 1u, 15u, 2u, 4u };
        dataStoreDefaultMaterial->SetRenderSlots(
            RenderDataDefaultMaterial::MaterialSlotType::SLOT_TYPE_DEPTH, { slots, countof(slots) });
        auto slotMask =
            dataStoreDefaultMaterial->GetRenderSlotMask(RenderDataDefaultMaterial::MaterialSlotType::SLOT_TYPE_DEPTH);
        for (uint32_t slot : slots) {
            EXPECT_TRUE((1ULL << uint64_t(slot)) & slotMask);
        }
    }
    {
        uint32_t slots[] = { 11u, 10u };
        dataStoreDefaultMaterial->SetRenderSlots(
            RenderDataDefaultMaterial::MaterialSlotType::SLOT_TYPE_TRANSLUCENT, { slots, countof(slots) });
        auto slotMask = dataStoreDefaultMaterial->GetRenderSlotMask(
            RenderDataDefaultMaterial::MaterialSlotType::SLOT_TYPE_TRANSLUCENT);
        for (uint32_t slot : slots) {
            EXPECT_TRUE((1ULL << uint64_t(slot)) & slotMask);
        }
    }
    // Destruction is deferred
    dataStore.reset();
    // Render with no render node graph just to trigger destruction
    renderContext->GetRenderer().RenderFrame({});
    EXPECT_FALSE(dsManager.GetRenderDataStore(dataStoreName));
}

/**
 * @tc.name: AddMaterialDataTest
 * @tc.desc: Tests for Add Material Data Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreDefaultMaterial, AddMaterialDataTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    constexpr BASE_NS::string_view dataStoreName = "DataStoreDefaultMaterial0";
    auto dataStore = dsManager.Create(IRenderDataStoreDefaultMaterial::UID, dataStoreName.data());
    ASSERT_TRUE(dataStore);

    auto dataStoreDefaultMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(dataStore.get());

    {
        RenderDataDefaultMaterial::InputMaterialUniforms uniforms;
        RenderDataDefaultMaterial::MaterialHandlesWithHandleReference handles;
        RenderDataDefaultMaterial::MaterialData data;
        uint8_t customPropertyData[16];
        const uint64_t entityIndex = 5ULL;
        const uint64_t dummyEntityIndex = 6ULL;

        auto materialIndex = dataStoreDefaultMaterial->UpdateMaterialData(
            entityIndex, uniforms, handles, data, { customPropertyData, countof(customPropertyData) });
        materialIndex = dataStoreDefaultMaterial->UpdateMaterialData(
            entityIndex, uniforms, handles, data, { customPropertyData, countof(customPropertyData) }, {});
        const auto matIdx = dataStoreDefaultMaterial->GetMaterialIndex(entityIndex);
        EXPECT_EQ(materialIndex, matIdx);
        EXPECT_EQ(materialIndex, dataStoreDefaultMaterial->UpdateMaterialData(entityIndex, uniforms, handles, data));
        EXPECT_EQ(materialIndex, dataStoreDefaultMaterial->GetMaterialIndex(entityIndex));
        EXPECT_EQ(RenderSceneDataConstants::INVALID_INDEX, dataStoreDefaultMaterial->GetMaterialIndex(100U));
        EXPECT_EQ(
            0, dataStoreDefaultMaterial->GetMaterialCustomPropertyData(RenderSceneDataConstants::INVALID_INDEX).size());
        RenderHandleReference rhr[3];
        auto resIndex = dataStoreDefaultMaterial->UpdateMaterialData(entityIndex, uniforms, handles, data,
            { customPropertyData, countof(customPropertyData) }, { rhr, countof(rhr) });
        EXPECT_NE(RenderSceneDataConstants::INVALID_INDEX, resIndex);

        const auto frameIndices = dataStoreDefaultMaterial->GetMaterialFrameIndices();
        EXPECT_NE(frameIndices.size(), 2U);
    }

    // Destruction is deferred
    dataStore.reset();
    // Render with no render node graph just to trigger destruction
    renderContext->GetRenderer().RenderFrame({});
    EXPECT_FALSE(dsManager.GetRenderDataStore(dataStoreName));
}

/**
 * @tc.name: AddInstanceMaterialDataTest
 * @tc.desc: Tests for Add Instance Material Data Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreDefaultMaterial, AddInstanceMaterialDataTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    constexpr BASE_NS::string_view dataStoreName = "DataStoreDefaultMaterial0";
    auto dataStore = dsManager.Create(IRenderDataStoreDefaultMaterial::UID, dataStoreName.data());
    ASSERT_TRUE(dataStore);

    auto dataStoreDefaultMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(dataStore.get());

    {
        RenderDataDefaultMaterial::InputMaterialUniforms uniforms;
        RenderDataDefaultMaterial::MaterialData data;
        uint8_t customPropertyData[16];
        const uint64_t entityId = 5ULL;
        const uint32_t meshEntityId = 6ULL;

        const auto materialIndex = dataStoreDefaultMaterial->UpdateMaterialData(
            entityId, uniforms, {}, data, { customPropertyData, countof(customPropertyData) });

        MeshDataWithHandleReference meshData;
        meshData.meshId = meshEntityId;
        meshData.submeshes.resize(1U);
        meshData.submeshes[0].materialId = entityId;
        dataStoreDefaultMaterial->UpdateMeshData(6ULL, meshData);

        RenderMeshData rmd;
        rmd.id = 0;
        rmd.meshId = meshEntityId;
        dataStoreDefaultMaterial->AddFrameRenderMeshData(rmd);

        dataStoreDefaultMaterial->SubmitFrameMeshData();

        const auto frameIndices = dataStoreDefaultMaterial->GetMaterialFrameIndices();
        EXPECT_EQ(frameIndices.size(), 1U);
        EXPECT_EQ(16u, dataStoreDefaultMaterial->GetMaterialCustomPropertyData(materialIndex).size());
    }

    // Destruction is deferred
    dataStore.reset();
    // Render with no render node graph just to trigger destruction
    renderContext->GetRenderer().RenderFrame({});
    EXPECT_FALSE(dsManager.GetRenderDataStore(dataStoreName));
}

/**
 * @tc.name: SubmeshJointsTest
 * @tc.desc: Tests for Submesh Joints Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreDefaultMaterial, SubmeshJointsTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    constexpr BASE_NS::string_view dataStoreName = "DataStoreDefaultMaterial0";
    auto dataStore = dsManager.Create(IRenderDataStoreDefaultMaterial::UID, dataStoreName.data());
    ASSERT_TRUE(dataStore);

    auto dataStoreDefaultMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(dataStore.get());

    {
        RenderSubmeshWithHandleReference submesh;
        submesh.indices.skinJointIndex = 0u;
        submesh.indices.materialFrameOffset = 64U;
        submesh.indices.materialIndex = RenderSceneDataConstants::INVALID_INDEX;
        dataStoreDefaultMaterial->AddSubmesh(submesh);
    }
    {
        RenderSubmeshWithHandleReference submesh;
        submesh.indices.skinJointIndex = 0u;
        submesh.indices.materialFrameOffset = 64U;
        submesh.indices.meshIndex = 1500u;
        submesh.indices.materialIndex = 1500u;
        dataStoreDefaultMaterial->AddSubmesh(submesh, {});
    }
    {
        RenderSubmeshWithHandleReference submesh;
        submesh.submeshFlags = RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT;
        submesh.indices.skinJointIndex = 1500u;
        submesh.indices.materialFrameOffset = 64U;
        submesh.indices.meshIndex = 1500u;
        submesh.indices.materialIndex = 1500u;
        dataStoreDefaultMaterial->AddSubmesh(submesh, {});
    }
    {
        dataStoreDefaultMaterial->Clear();

        Math::Mat4X4 matrices[16];
        Math::Mat4X4 prevMatrices[16];
        for (uint32_t i = 0; i < countof(matrices); ++i) {
            matrices[i] = Math::Mat4X4 { 1.0f };
        }
        for (uint32_t i = 0; i < countof(prevMatrices); ++i) {
            prevMatrices[i] = Math::Mat4X4 { 2.0f };
        }

        const uint64_t meshId = 6;
        MeshDataWithHandleReference meshData;
        meshData.meshId = meshId;
        meshData.submeshes.push_back({});
        dataStoreDefaultMaterial->UpdateMeshData(meshId, meshData);

        RenderMeshData rmd;
        rmd.id = 5;
        rmd.meshId = meshId;
        RenderMeshSkinData rmsd;
        rmsd.skinJointMatrices = { matrices, countof(matrices) };
        rmsd.prevSkinJointMatrices = { prevMatrices, countof(prevMatrices) };
        dataStoreDefaultMaterial->AddFrameRenderMeshData(rmd, rmsd);
        dataStoreDefaultMaterial->SubmitFrameMeshData();

        const auto submeshes = dataStoreDefaultMaterial->GetSubmeshes();
        ASSERT_EQ(submeshes.size(), size_t(1));
        if (!submeshes.empty()) {
            auto result = dataStoreDefaultMaterial->GetSubmeshJointMatrixData(submeshes[0].indices.skinJointIndex);
            ASSERT_EQ(countof(matrices) * 2, result.size());
            for (uint32_t matIdx = 0; matIdx < result.size(); ++matIdx) {
                Math::Mat4X4 expectedMatrix { 1.0f };
                if (matIdx >= countof(matrices)) {
                    expectedMatrix = Math::Mat4X4 { 2.0f };
                }
                for (uint32_t i = 0; i < 4u; ++i) {
                    for (uint32_t j = 0; j < 4u; ++j) {
                        EXPECT_EQ(expectedMatrix[i][j], result[matIdx][i][j]);
                    }
                }
            }
        }
    }
    // If number of prev matrices is different than number of matrices, prev matrices are taken as current matrices
    {
        dataStoreDefaultMaterial->Clear();

        Math::Mat4X4 matrices[16];
        Math::Mat4X4 prevMatrices[17];
        for (uint32_t i = 0; i < countof(matrices); ++i) {
            matrices[i] = Math::Mat4X4 { 1.0f };
        }
        for (uint32_t i = 0; i < countof(prevMatrices); ++i) {
            prevMatrices[i] = Math::Mat4X4 { 2.0f };
        }

        const uint64_t meshId = 6;
        MeshDataWithHandleReference meshData;
        meshData.meshId = meshId;
        meshData.submeshes.push_back({});
        dataStoreDefaultMaterial->UpdateMeshData(meshId, meshData);

        RenderMeshData rmd;
        rmd.id = 5;
        rmd.meshId = meshId;
        RenderMeshSkinData rmsd;
        rmsd.skinJointMatrices = { matrices, countof(matrices) };
        rmsd.prevSkinJointMatrices = { prevMatrices, countof(prevMatrices) };
        dataStoreDefaultMaterial->AddFrameRenderMeshData(rmd, rmsd);

        dataStoreDefaultMaterial->SubmitFrameMeshData();

        const auto submeshes = dataStoreDefaultMaterial->GetSubmeshes();
        ASSERT_EQ(submeshes.size(), size_t(1));
        if (!submeshes.empty()) {
            auto result = dataStoreDefaultMaterial->GetSubmeshJointMatrixData(submeshes[0].indices.skinJointIndex);
            ASSERT_EQ(countof(matrices) * 2, result.size());
            for (uint32_t matIdx = 0; matIdx < result.size(); ++matIdx) {
                Math::Mat4X4 expectedMatrix { 1.0f };
                for (uint32_t i = 0; i < 4u; ++i) {
                    for (uint32_t j = 0; j < 4u; ++j) {
                        EXPECT_EQ(expectedMatrix[i][j], result[matIdx][i][j]);
                    }
                }
            }
        }
    }
    dataStoreDefaultMaterial->Clear();
    EXPECT_EQ(0, dataStoreDefaultMaterial->GetSubmeshJointMatrixData(RenderSceneDataConstants::INVALID_INDEX).size());

    // Destruction is deferred
    dataStore.reset();
    // Render with no render node graph just to trigger destruction
    renderContext->GetRenderer().RenderFrame({});
    EXPECT_FALSE(dsManager.GetRenderDataStore(dataStoreName));
}

/**
 * @tc.name: RenderSortLayerTest
 * @tc.desc: Tests for Render Sort Layer Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreDefaultMaterial, RenderSortLayerTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    constexpr BASE_NS::string_view dataStoreName = "DataStoreDefaultMaterial0";
    auto dataStore = dsManager.Create(IRenderDataStoreDefaultMaterial::UID, dataStoreName.data());
    ASSERT_TRUE(dataStore);

    auto dataStoreDefaultMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(dataStore.get());

    // Non-default values
    constexpr uint64_t renderSortLayer = 2;
    constexpr uint64_t renderSortLayerOrder = 53;
    constexpr uint64_t meshRenderSortLayer = 48;
    constexpr uint64_t meshRenderSortLayerOrder = 45;
    constexpr uint64_t entityIndex = 14;
    RenderFrameMaterialIndices matIndices;
    {
        RenderDataDefaultMaterial::InputMaterialUniforms uniforms;
        RenderDataDefaultMaterial::MaterialData data;
        data.renderSortLayer = renderSortLayer;
        data.renderSortLayerOrder = renderSortLayerOrder;

        const auto materialIndex = dataStoreDefaultMaterial->UpdateMaterialData(entityIndex, uniforms, {}, data);
        matIndices.index = materialIndex;
    }
    {
        RenderSubmeshWithHandleReference submesh;
        submesh.indices.materialIndex = matIndices.index;
        submesh.indices.materialFrameOffset = matIndices.frameOffset;
        submesh.layers.meshRenderSortLayer = meshRenderSortLayer;
        submesh.layers.meshRenderSortLayerOrder = meshRenderSortLayerOrder;
        dataStoreDefaultMaterial->AddSubmesh(submesh);

        const auto& submeshes = dataStoreDefaultMaterial->GetSubmeshes();

        EXPECT_EQ(submeshes.size(), size_t(1));
        if (submeshes.size() >= 1) {
            EXPECT_EQ(submeshes[0U].layers.materialRenderSortLayer, renderSortLayer);
            EXPECT_EQ(submeshes[0U].layers.materialRenderSortLayerOrder, renderSortLayerOrder);
            EXPECT_EQ(submeshes[0U].layers.meshRenderSortLayer, meshRenderSortLayer);
            EXPECT_EQ(submeshes[0U].layers.meshRenderSortLayerOrder, meshRenderSortLayerOrder);
        }
    }
    {
        RenderSubmeshWithHandleReference submesh;
        submesh.indices.materialIndex = matIndices.index;
        submesh.indices.materialFrameOffset = matIndices.frameOffset;
        submesh.layers.meshRenderSortLayer = meshRenderSortLayer;
        submesh.layers.meshRenderSortLayerOrder = meshRenderSortLayerOrder;
        // non-default values
        submesh.layers.materialRenderSortLayer = 8U;
        submesh.layers.materialRenderSortLayerOrder = 9U;
        dataStoreDefaultMaterial->AddSubmesh(submesh);

        const auto& submeshes = dataStoreDefaultMaterial->GetSubmeshes();

        EXPECT_EQ(submeshes.size(), size_t(2));
        if (submeshes.size() >= 2) {
            EXPECT_EQ(submeshes[1U].layers.materialRenderSortLayer, 8U);
            EXPECT_EQ(submeshes[1U].layers.materialRenderSortLayerOrder, 9U);
            EXPECT_EQ(submeshes[1U].layers.meshRenderSortLayer, meshRenderSortLayer);
            EXPECT_EQ(submeshes[1U].layers.meshRenderSortLayerOrder, meshRenderSortLayerOrder);
        }
    }

    // Destruction is deferred
    dataStore.reset();
    // Render with no render node graph just to trigger destruction
    renderContext->GetRenderer().RenderFrame({});
    EXPECT_FALSE(dsManager.GetRenderDataStore(dataStoreName));
}

/**
 * @tc.name: RenderSceneBoundingTest
 * @tc.desc: Tests for Render Scene Bounding Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_RenderDataStoreDefaultMaterial, RenderSceneBoundingTest, testing::ext::TestSize.Level1)
{
    UTest::TestContext* testContext = UTest::GetTestContext();
    auto engine = testContext->engine;
    auto renderContext = testContext->renderContext;
    auto graphicsContext = testContext->graphicsContext;
    auto ecs = testContext->ecs;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    constexpr BASE_NS::string_view dataStoreName = "DataStoreDefaultMaterial0";
    auto dataStore = dsManager.Create(IRenderDataStoreDefaultMaterial::UID, dataStoreName.data());
    ASSERT_TRUE(dataStore);

    auto dataStoreDefaultMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(dataStore.get());

    {
        const auto frameObjectInfo = dataStoreDefaultMaterial->GetRenderFrameObjectInfo();
        const RenderBoundingSphere rbs = frameObjectInfo.shadowCasterBoundingSphere;
        EXPECT_EQ(0.0f, rbs.center.x);
        EXPECT_EQ(0.0f, rbs.center.y);
        EXPECT_EQ(0.0f, rbs.center.z);
        EXPECT_EQ(0.0f, rbs.radius);
    }

    constexpr uint64_t entityId = 14;
    RenderFrameMaterialIndices matIndices;
    {
        RenderDataDefaultMaterial::InputMaterialUniforms uniforms;
        RenderDataDefaultMaterial::MaterialData data;

        const auto materialIndex = dataStoreDefaultMaterial->UpdateMaterialData(entityId, uniforms, {}, data);
        matIndices.index = materialIndex;
    }
    {
        MeshDataWithHandleReference mesh;
        mesh.meshId = 1ULL;
        mesh.submeshes.resize(1U);
        mesh.submeshes[0].materialId = entityId;
        mesh.submeshes[0].aabbMin = { -0.25f, -0.25f, -0.25f };
        mesh.submeshes[0].aabbMax = { 0.75f, 0.75f, 0.75f };
        dataStoreDefaultMaterial->UpdateMeshData(1ULL, mesh);
    }
    {
        RenderMeshData rmd;
        rmd.id = 9ULL;
        rmd.meshId = 1ULL;
        rmd.world = Math::IDENTITY_4X4;
        dataStoreDefaultMaterial->AddFrameRenderMeshData(rmd);
    }

    dataStoreDefaultMaterial->SubmitFrameMeshData();

    {
        const auto frameObjectInfo = dataStoreDefaultMaterial->GetRenderFrameObjectInfo();
        const RenderBoundingSphere rbs = frameObjectInfo.shadowCasterBoundingSphere;
        EXPECT_EQ(0.25f, rbs.center.x);
        EXPECT_EQ(0.25f, rbs.center.y);
        EXPECT_EQ(0.25f, rbs.center.z);
        EXPECT_GT(0.9f, rbs.radius);
        EXPECT_LT(0.85f, rbs.radius);
    }

    // Destruction is deferred
    dataStore.reset();
    // Render with no render node graph just to trigger destruction
    renderContext->GetRenderer().RenderFrame({});
    EXPECT_FALSE(dsManager.GetRenderDataStore(dataStoreName));
}
