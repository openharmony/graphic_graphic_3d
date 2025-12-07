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

#ifndef CORE3D_TEST_GFX_COMMON_H
#define CORE3D_TEST_GFX_COMMON_H

#include <3d/gltf/gltf.h>
#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <base/containers/byte_array.h>
#include <base/containers/unique_ptr.h>
#include <base/math/vector.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/engine_info.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/plugin/intf_plugin_register.h>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/gles/intf_device_gles.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>
#include <render/vulkan/intf_device_vk.h>

#if (RENDER_HAS_VULKAN_BACKEND)
#include <vulkan/vulkan.h>

#include <render/vulkan/intf_device_vk.h>
#endif

#if (RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND)
#include <render/gles/intf_device_gles.h>
#endif

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

CORE3D_BEGIN_NAMESPACE();
namespace UTest {

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

constexpr const bool WITH_SWAPCHAIN = false;

struct GltfImportInfo {
    enum AnimationTarget { AnimateMainScene, AnimateImportedScene };

    const char* filename;
    AnimationTarget target;
    GltfResourceImportFlags resourceImportFlags;
    GltfSceneImportFlags sceneImportFlags;
};

class TestResources {
public:
    TestResources(uint32_t windowWidth, uint32_t windowHeight) : windowWidth_(windowWidth), windowHeight_(windowHeight)
    {}
    TestResources(uint32_t windowWidth, uint32_t windowHeight, DeviceBackendType backend)
        : windowWidth_(windowWidth), windowHeight_(windowHeight), testBackend_(backend)
    {}

    void LiftTestUp(int viewportSizeX, int viewportSizeY);
    void TickTest(int frameCountToTick);
    void TickTestAutoRng(int frameCountToTick);
    void ShutdownTest();

    void AppendResources(const GLTFResourceData& resources);

    IEcs& GetEcs();
    IGraphicsContext& GetGraphicsContext();
    IEngine& GetEngine();
    IRenderContext& GetRenderContext();
    uint32_t GetWindowWidth();
    uint32_t GetWindowHeight();
    RenderHandleReference GetImage();
    ByteArray* GetByteArray();
    vector<EntityReference>& GetImportedResources();
    DeviceBackendType GetDeviceBackendType();

private:
    void CreateTestWindow(int w, int h);
    void DestroyTestWindow();

    void TakeImageData();
    void SetBackendExtra(DeviceCreateInfo& deviceCreateInfo);
    void SetBackendSurface(SwapchainCreateInfo& swapchainCreateInfo, IDevice& device);

    IEngine::Ptr engine_;
    IEcs::Ptr ecs_;
    IRenderContext::Ptr renderContext_;
    IGraphicsContext::Ptr graphicsContext_;
    RenderHandleReference renderNodeGraph_;
    vector<EntityReference> importedResources_;

    uint32_t windowWidth_;
    uint32_t windowHeight_;

    unique_ptr<ByteArray> byteArray_;
    refcnt_ptr<IRenderDataStoreDefaultGpuResourceDataCopy> dataStoreDataCopy_;
    RenderHandleReference imageHandle_;
    RenderHandleReference bufferHandle_;

    DeviceBackendType testBackend_ = DeviceBackendType::VULKAN;
#if defined(RENDER_HAS_GL_BACKEND) && RENDER_HAS_GL_BACKEND
    BackendExtraGL glExtra_;
#endif
    BackendExtraVk vkExtra_;

    uint64_t surface_;
};

Entity CreateSolidColorMaterial(IEcs& ecs, Math::Vec4 color);

} // namespace UTest
CORE3D_END_NAMESPACE()

#endif // CORE3D_TEST_GFX_COMMON_H
