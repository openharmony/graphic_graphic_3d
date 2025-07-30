/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>

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

#include "TestRunner.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;
using namespace testing::ext;

namespace {
struct TestContext {
    std::shared_ptr<ISceneInit> sceneInit_ = nullptr;
    CORE_NS::IEcs::Ptr ecs_;
};
static TestContext g_context;

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
static constexpr BASE_NS::Uid ENGINE_THREAD{"2070e705-d061-40e4-bfb7-90fad2c280af"};
static constexpr BASE_NS::Uid APP_THREAD{"b2e8cef3-453a-4651-b564-5190f8b5190d"};
static constexpr BASE_NS::Uid IO_QUEUE{"be88e9a0-9cd8-45ab-be48-937953dc258f"};
static constexpr BASE_NS::Uid JS_RELEASE_THREAD{"3784fa96-b25b-4e9c-bbf1-e897d36f73af"};

bool SceneDispose(TestContext &context)
{
    context.ecs_ = nullptr;
    context.sceneInit_ = nullptr;
    return true;
}

bool SceneCreate(TestContext &context)
{
    context.sceneInit_ = CreateTestScene();
    context.sceneInit_->LoadPluginsAndInit();
    if (!context.sceneInit_->GetEngineInstance().engine_) {
        WIDGET_LOGE("fail to get engine");
        return false;
    }
    context.ecs_ = context.sceneInit_->GetEngineInstance().engine_->CreateEcs();
    if (!context.ecs_) {
        WIDGET_LOGE("fail to get ecs");
        return false;
    }
    auto factory = GetInstance<Core::ISystemGraphLoaderFactory>(UID_SYSTEM_GRAPH_LOADER);
    auto systemGraphLoader = factory->Create(context.sceneInit_->GetEngineInstance().engine_->GetFileManager());
    systemGraphLoader->Load("rofs3D://systemGraph.json", *(context.ecs_));
    auto& ecs = *(context.ecs_);
    ecs.Initialize();

    using namespace SCENE_NS;
#if SCENE_META_TEST
    auto fun = [&context]() {
        auto &obr = META_NS::GetObjectRegistry();

        context.params_ = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        if (!context.params_) {
            CORE_LOG_E("default obj null");
        }
        context.scene_ =
            interface_pointer_cast<SCENE_NS::IScene>(obr.Create(SCENE_NS::ClassId::Scene, context.params_));
        
        auto onLoaded = META_NS::MakeCallback<META_NS::IOnChanged>([&context]() {
            bool complete = false;
            auto status = context.scene_->Status()->GetValue();
            if (status == SCENE_NS::IScene::SCENE_STATUS_READY) {
                // still in engine thread
                complete = true;
            } else if (status == SCENE_NS::IScene::SCENE_STATUS_LOADING_FAILED) {
                // make sure we don't have anything in result if error
                complete = true;
            }

            if (complete) {
                if (context.scene_) {
                    auto &obr = META_NS::GetObjectRegistry();
                    // make sure we have renderconfig
                    auto rc = context.scene_->RenderConfiguration()->GetValue();
                    if (!rc) {
                        // Create renderconfig
                        rc = obr.Create<SCENE_NS::IRenderConfiguration>(SCENE_NS::ClassId::RenderConfiguration);
                        context.scene_->RenderConfiguration()->SetValue(rc);
                    }

                    interface_cast<IEcsScene>(context.scene_)
                        ->RenderMode()
                        ->SetValue(IEcsScene::RenderMode::RENDER_ALWAYS);
                    auto duh = context.params_->GetArrayPropertyByName<IntfWeakPtr>("Scenes");
                    if (!duh) {
                        return ;
                    }
                    duh->AddValue(interface_pointer_cast<CORE_NS::IInterface>(context.scene_));
                }
            }
        });
        context.scene_->Asynchronous()->SetValue(false);
        context.scene_->Uri()->SetValue("scene://empty");
        return META_NS::IAny::Ptr{};
    };
    // Should it be possible to cancel? (ie. do we need to store the token for something ..)
    META_NS::GetTaskQueueRegistry()
        .GetTaskQueue(ENGINE_THREAD)
        ->AddWaitableTask(META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>(BASE_NS::move(fun)))
        ->Wait();
#endif
    return true;
}
} // namespace

class RenderDataStoreDefaultMaterialTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SceneCreate(g_context);
    }
    static void TearDownTestSuite()
    {
        SceneDispose(g_context);
    }
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: CreateDestroyTest
 * @tc.desc: test CreateDestroy
 * @tc.type: FUNC
 */
HWTEST_F(RenderDataStoreDefaultMaterialTest, CreateDestroyTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

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
 * @tc.desc: test DefaultMaterial
 * @tc.type: FUNC
 */
HWTEST_F(RenderDataStoreDefaultMaterialTest, DefaultMaterialTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

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
        constexpr uint32_t expectedDefaultMc = MaterialComponent::LightingFlagBits::SHADOW_RECEIVER_BIT |
                                               MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT |
                                               MaterialComponent::LightingFlagBits::PUNCTUAL_LIGHT_RECEIVER_BIT |
                                               MaterialComponent::LightingFlagBits::INDIRECT_LIGHT_RECEIVER_BIT;
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
 * @tc.desc: test UpdateMaterial
 * @tc.type: FUNC
 */
HWTEST_F(RenderDataStoreDefaultMaterialTest, UpdateMaterialTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

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
 * @tc.desc: test GetSetRenderSlots
 * @tc.type: FUNC
 */
HWTEST_F(RenderDataStoreDefaultMaterialTest, GetSetRenderSlotsTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

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
 * @tc.desc: test AddMaterialData
 * @tc.type: FUNC
 */
HWTEST_F(RenderDataStoreDefaultMaterialTest, AddMaterialDataTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

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
        EXPECT_EQ(RenderSceneDataConstants::INVALID_INDEX,
            dataStoreDefaultMaterial->AddFrameMaterialData(dummyEntityIndex, 0u).index);
        const auto batchMaterialIndex = dataStoreDefaultMaterial->AddFrameMaterialData(dummyEntityIndex, 2u);
        EXPECT_NE(materialIndex, batchMaterialIndex.index);
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
 * @tc.desc: test AddInstanceMaterialData
 * @tc.type: FUNC
 */
HWTEST_F(RenderDataStoreDefaultMaterialTest, AddInstanceMaterialDataTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

    auto& dsManager = renderContext->GetRenderDataStoreManager();

    constexpr BASE_NS::string_view dataStoreName = "DataStoreDefaultMaterial0";
    auto dataStore = dsManager.Create(IRenderDataStoreDefaultMaterial::UID, dataStoreName.data());
    ASSERT_TRUE(dataStore);

    auto dataStoreDefaultMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(dataStore.get());

    {
        RenderDataDefaultMaterial::InputMaterialUniforms uniforms;
        RenderDataDefaultMaterial::MaterialData data;
        uint8_t customPropertyData[16];
        uint64_t entityIndex = 5ULL;

        const auto materialIndex = dataStoreDefaultMaterial->UpdateMaterialData(
            entityIndex, uniforms, {}, data, { customPropertyData, countof(customPropertyData) });
        dataStoreDefaultMaterial->AddFrameMaterialData(entityIndex, 2U);
        const auto frameIndices = dataStoreDefaultMaterial->GetMaterialFrameIndices();
        EXPECT_EQ(frameIndices.size(), 2U);
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
 * @tc.desc: test SubmeshJoints
 * @tc.type: FUNC
 */
HWTEST_F(RenderDataStoreDefaultMaterialTest, SubmeshJointsTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

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
 * @tc.desc: test RenderSortLayer
 * @tc.type: FUNC
 */
HWTEST_F(RenderDataStoreDefaultMaterialTest, RenderSortLayerTest, TestSize.Level1)
{
    auto engine = g_context.sceneInit_->GetEngineInstance().engine_;
    auto renderContext = g_context.sceneInit_->GetEngineInstance().renderContext_;
    auto graphicsContext = g_context.sceneInit_->GetEngineInstance().graphicsContext_;
    auto ecs = g_context.ecs_;

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
        matIndices = dataStoreDefaultMaterial->AddFrameMaterialData(entityIndex, 1U);
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
