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

#include <device/gpu_resource_cache.h>
#include <device/gpu_resource_manager.h>

#include <render/device/intf_gpu_resource_cache.h>

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
void TestGpuResourceCache(const UTest::EngineResources& er)
{
    GpuResourceManager& gpuResourceMgr = static_cast<GpuResourceManager&>(er.device->GetGpuResourceManager());
    GpuResourceCache gpuResourceCache { gpuResourceMgr };

    gpuResourceCache.BeginFrame(0u);
    CacheGpuImageDesc desc;
    desc.width = 16;
    desc.height = 16;
    desc.mipCount = 1;
    desc.layerCount = 1;
    desc.format = BASE_FORMAT_R16G16B16A16_SFLOAT;
    auto imageHandle = gpuResourceCache.ReserveGpuImage(desc);
    auto resultDesc = gpuResourceCache.GetCacheGpuImageDesc(imageHandle);
    ASSERT_EQ(desc.width, resultDesc.width);
    ASSERT_EQ(desc.height, resultDesc.height);
    ASSERT_EQ(desc.mipCount, resultDesc.mipCount);
    ASSERT_EQ(desc.layerCount, resultDesc.layerCount);
    ASSERT_EQ(desc.format, resultDesc.format);

    gpuResourceCache.BeginFrame(1u);
    {
        auto images = gpuResourceCache.GetImageData();
        ASSERT_EQ(1, images.size());
        ASSERT_EQ(imageHandle.GetHandle(), images[0].handle.GetHandle());
    }

    imageHandle = {};
    imageHandle = gpuResourceCache.ReserveGpuImage(desc);
    gpuResourceCache.BeginFrame(2u);
    gpuResourceCache.BeginFrame(3u);
    {
        auto images = gpuResourceCache.GetImageData();
        ASSERT_EQ(0, images.size());
    }
}
} // namespace

/**
 * @tc.name: GpuResourceCacheTest
 * @tc.desc: Tests GpuResourceCache and verifies it caches an image and frees the cache after some frames have passed.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuResourceCache, GpuResourceCacheTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    if (engine.backend == DeviceBackendType::OPENGL) {
        engine.createWindow = true;
    }
    UTest::CreateEngineSetup(engine);
    TestGpuResourceCache(engine);
    UTest::DestroyEngine(engine);
}
