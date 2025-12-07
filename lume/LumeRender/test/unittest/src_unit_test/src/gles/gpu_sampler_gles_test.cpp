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

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
#include <gles/gpu_sampler_gles.h>
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND

#include <device/device.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GpuSamplerTestOpenGL
 * @tc.desc: Tests GLES creation of samplers with different border colors.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuSampler, GpuSamplerTestOpenGL, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = UTest::GetOpenGLBackend();
    engine.createWindow = true;
    UTest::CreateEngineSetup(engine);
    Device& device = *static_cast<Device*>(engine.device);
    device.Activate();
    {
        GpuSamplerDesc desc;
        desc.enableCompareOp = true;
        desc.borderColor = CORE_BORDER_COLOR_INT_OPAQUE_WHITE;
        GpuSamplerGLES sampler { device, desc };
        auto realDesc = sampler.GetDesc();
        ASSERT_EQ(desc.enableCompareOp, realDesc.enableCompareOp);
        ASSERT_EQ(desc.borderColor, realDesc.borderColor);
    }
    {
        GpuSamplerDesc desc;
        desc.enableCompareOp = true;
        desc.borderColor = CORE_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        GpuSamplerGLES sampler { device, desc };
        auto realDesc = sampler.GetDesc();
        ASSERT_EQ(desc.enableCompareOp, realDesc.enableCompareOp);
        ASSERT_EQ(desc.borderColor, realDesc.borderColor);
    }
    device.Deactivate();
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
