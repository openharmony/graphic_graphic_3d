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

#include <device/device.h>
#include <device/gpu_resource_cache.h>
#include <device/gpu_resource_manager.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

/**
 * @tc.name: GpuResourceCacheBasicTest
 * @tc.desc: Tests that IGpuResourceCache caches GPUImages correctly.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuResourceCache, GpuResourceCacheBasicTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources er;
    if (er.backend == DeviceBackendType::OPENGL) {
        er.createWindow = true;
    }
    // er.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(er);
    ASSERT_TRUE(er.device != nullptr);

    unique_ptr<GpuResourceManager> gpuResourceMgr =
        make_unique<GpuResourceManager>(*(Device*)er.device, GpuResourceManager::CreateInfo {});
    ASSERT_TRUE(gpuResourceMgr != nullptr);

    IGpuResourceCache& gpuResourceCache = gpuResourceMgr->GetGpuResourceCache();

    // Get same image.
    {
        CacheGpuImageDesc desc;
        desc.format = BASE_FORMAT_R8G8B8A8_SRGB;
        desc.width = 2u;
        desc.height = 2u;
        const RenderHandleReference handle0 = gpuResourceCache.ReserveGpuImage(desc);
        const RenderHandleReference handle1 = gpuResourceCache.ReserveGpuImage(desc);

        ASSERT_TRUE((handle0.GetHandle() != handle1.GetHandle()));
        ASSERT_TRUE(gpuResourceMgr->IsGpuImage(handle0));
        ASSERT_TRUE(gpuResourceMgr->IsGpuImage(handle1));
    }

    // Get other image.
    {
        CacheGpuImageDesc desc;
        desc.format = BASE_FORMAT_R8G8B8A8_SRGB;
        desc.width = 2u;
        desc.height = 2u;
        vector<CacheGpuImageDesc> descs;
        descs.emplace_back(desc);
        descs.emplace_back(desc);
        descs.emplace_back(desc);

        const vector<RenderHandleReference> handles0 =
            gpuResourceCache.ReserveGpuImages({ descs.data(), descs.size() });

        const vector<RenderHandleReference> handles1 =
            gpuResourceCache.ReserveGpuImages({ descs.data(), descs.size() });

        for (size_t idx = 0; idx < handles0.size(); ++idx) {
            for (size_t jdx = 0; jdx < handles0.size(); ++jdx) {
                if (idx != jdx) {
                    ASSERT_TRUE(handles0[idx].GetHandle() != handles0[jdx].GetHandle());
                }
            }
        }
        for (size_t idx = 0; idx < handles1.size(); ++idx) {
            for (size_t jdx = 0; jdx < handles1.size(); ++jdx) {
                if (idx != jdx) {
                    ASSERT_TRUE(handles1[idx].GetHandle() != handles1[jdx].GetHandle());
                }
            }
        }
        ASSERT_TRUE(handles0.size() == handles1.size());
        for (size_t idx = 0; idx < handles0.size(); ++idx) {
            for (size_t jdx = 0; jdx < handles1.size(); ++jdx) {
                ASSERT_TRUE(handles0[idx].GetHandle() != handles1[jdx].GetHandle());
            }
        }
        for (size_t idx = 0; idx < handles0.size(); ++idx) {
            ASSERT_TRUE(gpuResourceMgr->IsGpuImage(handles0[idx]));
            ASSERT_TRUE(gpuResourceMgr->IsGpuImage(handles1[idx]));
        }
    }
}
