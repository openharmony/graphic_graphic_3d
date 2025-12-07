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

#include <device/gpu_resource_handle_util.h>
#include <resource_handle_impl.h>

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
 * @tc.name: RenderHandleReferenceTest
 * @tc.desc: Tests that RenderHandle is reference counted correctly.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderHandleReference, RenderHandleReferenceTest, testing::ext::TestSize.Level1)
{
    // Create valid reference and destroy the ref
    {
        const RenderHandle handle = RenderHandleUtil::CreateHandle(RenderHandleType::RENDER_NODE_GRAPH, 1, 2);
        IRenderReferenceCounter::Ptr counter = IRenderReferenceCounter::Ptr(new RenderReferenceCounter());

        ASSERT_TRUE(RenderHandleUtil::IsValid(handle));
        ASSERT_TRUE(RenderHandleUtil::GetIndexPart(handle) == 1);
        ASSERT_TRUE(RenderHandleUtil::GetGenerationIndexPart(handle) == 2);
        ASSERT_TRUE(counter);
        ASSERT_TRUE(((RenderReferenceCounter*)counter.get())->GetRefCount() == 1);

        RenderHandleReference rhr = RenderHandleReference(handle, counter);
        ASSERT_TRUE(rhr);
        ASSERT_TRUE(((RenderReferenceCounter*)counter.get())->GetRefCount() == 2);

        // Destroy the initial ref
        counter = {};
    }

    // Create valid reference and increase ref count
    {
        const RenderHandle handle = RenderHandleUtil::CreateHandle(RenderHandleType::RENDER_NODE_GRAPH, 4, 0);
        IRenderReferenceCounter::Ptr counter = IRenderReferenceCounter::Ptr(new RenderReferenceCounter());

        ASSERT_TRUE(RenderHandleUtil::IsValid(handle));
        ASSERT_TRUE(RenderHandleUtil::GetIndexPart(handle) == 4);
        ASSERT_TRUE(RenderHandleUtil::GetGenerationIndexPart(handle) == 0);
        ASSERT_TRUE(counter);
        ASSERT_TRUE(((RenderReferenceCounter*)counter.get())->GetRefCount() == 1);

        RenderHandleReference rhr = RenderHandleReference(handle, counter);
        ASSERT_TRUE(rhr);

        RenderHandleReference rhr1 = RenderHandleReference(handle, counter);
        ASSERT_TRUE(rhr1);

        ASSERT_TRUE(((RenderReferenceCounter*)counter.get())->GetRefCount() == 3);

        RenderHandleReference rhr2 = RenderHandleReference(handle, counter);
        ASSERT_TRUE(rhr2);
        rhr2 = {};

        ASSERT_TRUE(((RenderReferenceCounter*)counter.get())->GetRefCount() == 3);

        // Destroy the initial ref
        counter = {};
    }
}

/**
 * @tc.name: GpuResourceHandleUtilTest
 * @tc.desc: Tests for RenderHandleUtil::* funcions to create correct RenderHandles.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuResourceHandleUtil, GpuResourceHandleUtilTest, testing::ext::TestSize.Level1)
{
    const uint64_t invalidIndex = RenderHandleUtil::RES_HANDLE_ID_MASK >> RenderHandleUtil::RES_HANDLE_ID_SHIFT;
    RenderHandleUtil::CreateGpuResourceHandle(RenderHandleType::GPU_IMAGE, 0u, invalidIndex, 0u);
    RenderHandleUtil::CreateGpuResourceHandle(RenderHandleType::GPU_IMAGE, 0u, invalidIndex, 0u, 0u);
    RenderHandleUtil::CreateHandle(RenderHandleType::GPU_IMAGE, invalidIndex, 0u);
    RenderHandleUtil::CreateEngineResourceHandle(RenderHandleType::GPU_IMAGE, invalidIndex, 0u);
    ASSERT_NE(RenderHandle {}, RenderHandleUtil::CreateGpuResourceHandle(RenderHandleType::GPU_IMAGE, 0u, 0u, 0u));
    ASSERT_NE(RenderHandle {}, RenderHandleUtil::CreateGpuResourceHandle(RenderHandleType::GPU_IMAGE, 0u, 0u, 0u, 0u));
    ASSERT_NE(RenderHandle {}, RenderHandleUtil::CreateHandle(RenderHandleType::GPU_IMAGE, 0u, 0u));
    ASSERT_NE(EngineResourceHandle {}.id,
        RenderHandleUtil::CreateEngineResourceHandle(RenderHandleType::GPU_IMAGE, 0u, 0u).id);
}
