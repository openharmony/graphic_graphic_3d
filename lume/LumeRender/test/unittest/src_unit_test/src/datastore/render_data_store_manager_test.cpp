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

#include <datastore/render_data_store_default_gpu_resource_data_copy.h>
#include <datastore/render_data_store_default_staging.h>
#include <datastore/render_data_store_manager.h>

#include <render/datastore/intf_render_data_store_default_acceleration_structure_staging.h>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using CORE_NS::IEngine;
using namespace RENDER_NS;

namespace {
void CreatePods(UTest::EngineResources& er)
{
    // post processing configuration
    {
        refcnt_ptr<IRenderDataStore> dataStore =
            er.context->GetRenderDataStoreManager().Create(IRenderDataStorePod::UID, "PostProcessDataStore");
        IRenderDataStorePod* dataStorePod = static_cast<IRenderDataStorePod*>(dataStore.get());
        PostProcessConfiguration ppConf;

        ppConf.enableFlags = PostProcessConfiguration::ENABLE_BLOOM_BIT | PostProcessConfiguration::ENABLE_BLUR_BIT |
                             PostProcessConfiguration::ENABLE_FXAA_BIT | PostProcessConfiguration::ENABLE_TAA_BIT;

        ppConf.bloomConfiguration.bloomQualityType = BloomConfiguration::BloomQualityType::QUALITY_TYPE_LOW;
        ppConf.bloomConfiguration.useCompute = true;

        ppConf.blurConfiguration.blurQualityType = BlurConfiguration::QUALITY_TYPE_HIGH;
        ppConf.blurConfiguration.blurType = BlurConfiguration::TYPE_NORMAL;
        ppConf.blurConfiguration.filterSize = 10.f;

        ppConf.fxaaConfiguration.quality = FxaaConfiguration::Quality::HIGH;
        ppConf.fxaaConfiguration.sharpness = FxaaConfiguration::Sharpness::SHARP;

        ppConf.taaConfiguration.quality = TaaConfiguration::Quality::HIGH;
        ppConf.taaConfiguration.sharpness = TaaConfiguration::Sharpness::SHARP;

        const array_view<const uint8_t> dataView = { reinterpret_cast<const uint8_t*>(&ppConf), sizeof(ppConf) };
        dataStorePod->CreatePod("PostProcessConfiguration", "PostProcessConfiguration", dataView);

        // Create again to trigger set with updated values
        ppConf.bloomConfiguration.useCompute = false;
        const array_view<const uint8_t> dataView2 = { reinterpret_cast<const uint8_t*>(&ppConf), sizeof(ppConf) };
        dataStorePod->CreatePod("PostProcessConfiguration", "PostProcessConfiguration", dataView2);
    }
    // empty pod
    {
        refcnt_ptr<IRenderDataStore> dataStore =
            er.context->GetRenderDataStoreManager().Create(IRenderDataStorePod::UID, "PostProcessDataStore");
        IRenderDataStorePod* dataStorePod = static_cast<IRenderDataStorePod*>(dataStore.get());
        const array_view<const uint8_t> dataView;
        dataStorePod->CreatePod("EmptyPod", "EmptyPod", dataView);
    }
}

template<class RenderDataStoreType>
RenderDataStoreTypeInfo FillRenderDataStoreTypeInfo()
{
    return {
        { RenderDataStoreTypeInfo::UID },
        RenderDataStoreType::UID,
        RenderDataStoreType::TYPE_NAME,
        RenderDataStoreType::Create,
    };
}

void Validate(const UTest::EngineResources& er)
{
    {
        RenderDataStoreManager& manager = static_cast<RenderDataStoreManager&>(er.context->GetRenderDataStoreManager());

        manager.RemoveRenderDataStoreFactory(FillRenderDataStoreTypeInfo<RenderDataStoreDefaultGpuResourceDataCopy>());
        manager.AddRenderDataStoreFactory(FillRenderDataStoreTypeInfo<RenderDataStoreDefaultGpuResourceDataCopy>());
        {
            refcnt_ptr<IRenderDataStore> dataStore = manager.GetRenderDataStore("NonExistingDataStore");
            ASSERT_FALSE(dataStore);
        }
        {
            IRenderDataStore* dataStore = manager.GetRenderTimeRenderDataStore("NonExistingDataStore");
            ASSERT_FALSE(dataStore);
        }
    }
    {
        refcnt_ptr<IRenderDataStore> dataStore =
            er.context->GetRenderDataStoreManager().Create(IRenderDataStoreDefaultGpuResourceDataCopy::UID, "Name1");
        ASSERT_TRUE(dataStore);
        dataStore->Clear();
        ASSERT_EQ("RenderDataStoreDefaultGpuResourceDataCopy", dataStore->GetTypeName());
        ASSERT_EQ("Name1", dataStore->GetName());
        ASSERT_EQ(IRenderDataStoreDefaultGpuResourceDataCopy::UID, dataStore->GetUid());
        ASSERT_EQ(0, dataStore->GetFlags());
        RenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy copyOp;
        copyOp.gpuHandle = {};
        static_cast<RenderDataStoreDefaultGpuResourceDataCopy*>(dataStore.get())->AddCopyOperation(copyOp);

        dataStore = er.context->GetRenderDataStoreManager().Create(IRenderDataStoreDefaultStaging::UID, "Name2");
        ASSERT_TRUE(dataStore);
        dataStore->Clear();
        ASSERT_EQ("RenderDataStoreDefaultStaging", dataStore->GetTypeName());
        ASSERT_EQ("Name2", dataStore->GetName());
        ASSERT_EQ(IRenderDataStoreDefaultStaging::UID, dataStore->GetUid());
        ASSERT_EQ(0, dataStore->GetFlags());
        RenderDataStoreDefaultStaging* dsStaging = static_cast<RenderDataStoreDefaultStaging*>(dataStore.get());
        {
            GpuImageDesc desc;
            desc.width = 2u;
            desc.height = 2u;
            desc.depth = 1u;
            desc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
            desc.format = BASE_FORMAT_R8G8B8A8_UINT;
            desc.imageType = CORE_IMAGE_TYPE_2D;
            desc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
            desc.layerCount = 1u;
            desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            desc.mipCount = 1u;
            desc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            auto handle = er.device->GetGpuResourceManager().Create(desc);
            dsStaging->ClearColorImage(handle, ClearColorValue {});
            dsStaging->PreRender();
            auto byteSize = dsStaging->GetImageClearByteSize();
            ASSERT_EQ(16u, byteSize);
        }
        {
            GpuBufferDesc desc;
            desc.byteSize = 1u;
            desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            desc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_SRC_BIT;
            auto handle = er.device->GetGpuResourceManager().Create(desc);
            uint8_t data = 0u;
            BufferCopy bufferCopy;
            bufferCopy.dstOffset = 0u;
            bufferCopy.srcOffset = 0u;
            bufferCopy.size = 1u;
            dsStaging->CopyDataToBufferOnCpu({ &data, 1 }, handle, bufferCopy);
        }

        dataStore = er.context->GetRenderDataStoreManager().Create(
            IRenderDataStoreDefaultAccelerationStructureStaging::UID, "Name3");
        ASSERT_TRUE(dataStore);
        ASSERT_EQ("RenderDataStoreDefaultAccelerationStructureStaging", dataStore->GetTypeName());
        ASSERT_EQ("Name3", dataStore->GetName());
        ASSERT_EQ(IRenderDataStoreDefaultAccelerationStructureStaging::UID, dataStore->GetUid());
    }
    // render data store pod
    {
        refcnt_ptr<IRenderDataStore> dataStore =
            er.context->GetRenderDataStoreManager().GetRenderDataStore("PostProcessDataStore");
        IRenderDataStorePod* dataStorePod = static_cast<IRenderDataStorePod*>(dataStore.get());
        ASSERT_NE(nullptr, dataStorePod);
        ASSERT_EQ("PostProcessDataStore", dataStorePod->GetName());
        ASSERT_EQ(0, dataStorePod->GetFlags());

        dataStorePod->Clear();
        dataStore = er.context->GetRenderDataStoreManager().Create(IRenderDataStorePod::UID, "PostProcessDataStore");
        ASSERT_TRUE(dataStore);

        dataStore = er.context->GetRenderDataStoreManager().Create(
            IRenderDataStoreDefaultGpuResourceDataCopy::UID, "PostProcessDataStore");
        ASSERT_FALSE(dataStore);

        auto names = dataStorePod->GetPodNames("PostProcessConfiguration");
        ASSERT_EQ(1, names.size());
        ASSERT_EQ("PostProcessConfiguration", names[0]);

        names = dataStorePod->GetPodNames("NonExistingPod");
        ASSERT_EQ(0, names.size());

        dataStorePod->DestroyPod("PostProcessConfiguration", "PostProcessConfiguration");
        names = dataStorePod->GetPodNames("PostProcessConfiguration");
        ASSERT_EQ(0, names.size());

        // Destruction is deffered
    }
    ASSERT_TRUE(er.context->GetRenderDataStoreManager().GetRenderDataStore("PostProcessDataStore"));
#if NDEBUG
    ASSERT_FALSE(er.context->GetRenderDataStoreManager().Create({}, ""));
#endif // NDEBUG
}
} // namespace

/**
 * @tc.name: RenderDataStoreManagerTest
 * @tc.desc: Tests different Render Data Store Managers and verifies that they work correctly.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderDataStoreManager, RenderDataStoreManagerTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);
    CreatePods(engine);
    Validate(engine);
    UTest::DestroyEngine(engine);
}
