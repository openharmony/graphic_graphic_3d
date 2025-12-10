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
#include <device/swapchain.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "log_util.h"
#include "test_runner.h"
#endif

using namespace BASE_NS;
using CORE_NS::IEngine;
using namespace RENDER_NS;

namespace {
void TestSwapchainCreation(DeviceBackendType backend)
{
    UTest::EngineResources engine;
    engine.backend = backend;
    engine.createWindow = true;
    UTest::CreateEngineSetup(engine);
    ASSERT_NE(nullptr, engine.device);

    SwapchainCreateInfo swapchainInfo {};
#if defined(__OHOS__)
    swapchainInfo.window.window = reinterpret_cast<uintptr_t>(::Test::g_ohosApp->GetWindowHandle());
    ASSERT_NE(::Test::g_ohosApp->GetWindowHandle(), nullptr);
#else
    swapchainInfo.surfaceHandle = UTest::CreateSurface(engine);
    ASSERT_NE(swapchainInfo.surfaceHandle, 0);
#endif
    swapchainInfo.swapchainFlags = RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_COLOR_BUFFER_BIT |
                                   RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_DEPTH_BUFFER_BIT |
                                   RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_VSYNC_BIT |
                                   RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_SRGB_BIT;

    engine.device->CreateSwapchain(swapchainInfo);

    const RenderHandle defaultSwapchain;
    ASSERT_NE(nullptr, ((Device*)engine.device)->GetSwapchain(defaultSwapchain));
    const Swapchain* swapchain = ((Device*)engine.device)->GetSwapchain(defaultSwapchain);
    uint32_t swapchainFlags = swapchain->GetFlags();
    CORE_LOG_I("Swapchain flags: %d", swapchainFlags);
    engine.device->DestroySwapchain();
    ASSERT_EQ(nullptr, ((Device*)engine.device)->GetSwapchain(defaultSwapchain));

    {
        UTest::DestroySurface(engine, swapchainInfo.surfaceHandle);
        UTest::DestroyEngine(engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: SwapchainCreateTestVulkan
 * @tc.desc: Tests the creation of a swapchain via the device in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_Swapchain, SwapchainCreateTestVulkan, testing::ext::TestSize.Level1)
{
    TestSwapchainCreation(DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: SwapchainCreateTestOpenGL
 * @tc.desc: Tests the creation of a swapchain via the device in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_Swapchain, SwapchainCreateTestOpenGL, testing::ext::TestSize.Level1)
{
    TestSwapchainCreation(UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
