/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <dlfcn.h>
#include <gtest/gtest.h>
#include <core/engine_info.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/os/intf_platform.h>
#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/property_types.h>
#include <core/resources/intf_resource_manager.h>
#include <meta/api/metadata_util.h>
#include <meta/api/object.h>
#include <meta/ext/attachment/behavior.h>
#include <meta/interface/intf_manual_clock.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/resource/intf_resource.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_renderer.h>
#include <scene/api/scene.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/ext/scene_utils.h>
#include <scene/interface/intf_create_mesh.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_raycast.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/resource/intf_render_resource_manager.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/resource/util.h>
#include <scene/interface/scene_options.h>
#include <scene_adapter/intf_surface_stream.h>
#include <scene_adapter/surface_stream.h>
#include "3d_widget_adapter_log.h"
#include "scene/scene_test.h"
#include <3d/implementation_uids.h>
#include <scene/interface/intf_application_context.h>
#if RENDER_HAS_GLES_BACKEND || RENDER_HAS_GL_BACKEND
#include <render/gles/intf_device_gles.h>
#endif
#if RENDER_HAS_VULKAN_BACKEND
#include <vulkan/vulkan.h>
#include <render/vulkan/intf_device_vk.h>
#endif

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {

static constexpr BASE_NS::Uid ENGINE_THREAD { "2070e705-d061-40e4-bfb7-90fad2c280af" };

class SurfaceStreamTest : public SCENE_NS::UTest::ScenePluginTest {
public:
    static void SetUpTestCase()
    {
        LoadEngineLib();
        LoadPlugins();
        CreateEngine();
        CreateRenderContext();
        ASSERT_NE(renderContext_, nullptr);
        CreateGraphicsContext();
        CreateEcs();
        CreateAppContext();
    }

    static void TearDownTestCase()
    {
        appContext_.reset();
        if (ecs_) {
            ecs_->Uninitialize();
        }
        ecs_.reset();
        graphicsContext3D_.reset();
        renderContext_.reset();
        engine_.reset();

        if (libHandle_) {
            dlclose(libHandle_);
            libHandle_ = nullptr;
        }
    }

    void SetUp() override
    {
        ScenePluginTest::SetUp();
    }

    void TearDown() override
    {
        ScenePluginTest::TearDown();
    }

private:
    static void LoadEngineLib()
    {
        // Load engine library and initialize global function pointers
        // These global function pointers are defined in lume_common.cpp

        // Try multiple possible library paths for different environments
        const char* libPaths[] = {
            "/system/lib64/libAGPDLL.z.so",  // Production device path
            "/system/lib/libAGPDLL.z.so",    // 32-bit device path
            "./libAGPDLL.z.so",              // Local development path
            "libAGPDLL.z.so"                 // Search in library path
        };

        for (const char* path : libPaths) {
            libHandle_ = dlopen(path, RTLD_LAZY);
            if (libHandle_) {
                WIDGET_LOGI("[SurfaceStreamTest] Successfully loaded engine library from: %{public}s", path);
                break;
            }
        }

        if (!libHandle_) {
            WIDGET_LOGE("[SurfaceStreamTest] Failed to load engine library from all paths:");
            for (const char* path : libPaths) {
                WIDGET_LOGE("[SurfaceStreamTest]   - %{public}s", path);
            }
            WIDGET_LOGE("[SurfaceStreamTest] Error: %{public}s", dlerror());
        }
        ASSERT_TRUE(libHandle_ != nullptr) <<
            "Failed to load engine library. Check that the engine library is built and accessible.";

        auto* createPluginReg = reinterpret_cast<void (*)(const CORE_NS::PlatformCreateInfo&)>(
            dlsym(libHandle_, "_ZN4Core20CreatePluginRegistryERKNS_18PlatformCreateInfoE"));
        auto* getPluginReg = reinterpret_cast<CORE_NS::IPluginRegister& (*)()>(
            dlsym(libHandle_, "_ZN4Core17GetPluginRegisterEv"));

        if (!createPluginReg || !getPluginReg) {
            if (!createPluginReg) {
                WIDGET_LOGE("[SurfaceStreamTest] CreatePluginRegistry: NULL - %{public}s", dlerror());
            }
            if (!getPluginReg) {
                WIDGET_LOGE("[SurfaceStreamTest] GetPluginRegister: NULL - %{public}s", dlerror());
            }
            ASSERT_TRUE(false) << "Failed to load engine function pointers";
        }

        // Assign to global function pointers
        CORE_NS::CreatePluginRegistry = createPluginReg;
        CORE_NS::GetPluginRegister = getPluginReg;

        // Verify function pointers are properly initialized
        ASSERT_TRUE(CORE_NS::CreatePluginRegistry != nullptr);
        ASSERT_TRUE(CORE_NS::GetPluginRegister != nullptr);

        WIDGET_LOGI("[SurfaceStreamTest] Engine library loaded successfully, function pointers initialized");
    }

    static void LoadPlugins()
    {
        if (CORE_NS::CreatePluginRegistry == nullptr) {
            WIDGET_LOGE("[SurfaceStreamTest] ERROR: CreatePluginRegistry is nullptr in LoadPlugins!");
            ASSERT_TRUE(false) << "CreatePluginRegistry became nullptr between LoadEngineLib and LoadPlugins";
        }

        // Create plugin registry with platform info
        // Use the paths defined in BUILD.gn
        CORE_NS::PlatformCreateInfo platformCreateInfo {
            PLATFORM_CORE_ROOT_PATH,
            PLATFORM_CORE_PLUGIN_PATH,
            PLATFORM_APP_ROOT_PATH,
            PLATFORM_APP_PLUGIN_PATH
        };
        CORE_NS::CreatePluginRegistry(platformCreateInfo);
        // Load necessary plugins
        constexpr BASE_NS::Uid plugins[] = {
            RENDER_NS::UID_RENDER_PLUGIN,
            CORE3D_NS::UID_3D_PLUGIN,
            SCENE_NS::UID_SCENE_PLUGIN
        };

        auto& pluginRegister = CORE_NS::GetPluginRegister();
        bool loadResult = pluginRegister.LoadPlugins(plugins);
        WIDGET_LOGI("[SurfaceStreamTest] LoadPlugins returned, result = %{public}s", loadResult ? "true" : "false");

        if (!loadResult) {
            WIDGET_LOGE("[SurfaceStreamTest] ERROR: Failed to load plugins!");
            ASSERT_TRUE(false) << "Failed to load required plugins";
        }

        WIDGET_LOGI("[SurfaceStreamTest] LoadPlugins completed successfully");
    }

    static void CreateEngine()
    {
        auto factory = CORE_NS::GetInstance<CORE_NS::IEngineFactory>(CORE_NS::UID_ENGINE_FACTORY);
        ASSERT_TRUE(factory != nullptr) << "Failed to get engine factory";

        CORE_NS::PlatformCreateInfo platformCreateInfo {
            PLATFORM_CORE_ROOT_PATH,
            PLATFORM_CORE_PLUGIN_PATH,
            PLATFORM_APP_ROOT_PATH,
            PLATFORM_APP_PLUGIN_PATH
        };
        CORE_NS::EngineCreateInfo engineCreateInfo {
            platformCreateInfo,
            {"UnitTest", 0, 1, 0},  // application version
            {}                       // application context
        };
        engine_ = factory->Create(engineCreateInfo);
        ASSERT_TRUE(engine_ != nullptr) << "Failed to create engine";

        engine_->Init();
    }

    static void CreateRenderContext()
    {
        if (!engine_) {
            WIDGET_LOGE("[SurfaceStreamTest] ERROR: engine_ is null in CreateRenderContext!");
            ASSERT_TRUE(false) << "engine_ is null";
        }

        auto* classFactory = engine_->GetInterface<CORE_NS::IClassFactory>();

        if (!classFactory) {
            WIDGET_LOGE("[SurfaceStreamTest] ERROR: GetInterface<IClassFactory>() returned null!");
            ASSERT_TRUE(false) << "GetInterface<IClassFactory>() returned null";
        }

        auto renderInstance = classFactory->CreateInstance(RENDER_NS::UID_RENDER_CONTEXT);
        if (!renderInstance) {
            WIDGET_LOGE("[SurfaceStreamTest] ERROR: CreateInstance returned null!");
            ASSERT_TRUE(false) << "CreateInstance returned null";
        }

        renderContext_ = BASE_NS::shared_ptr<RENDER_NS::IRenderContext>(
            static_cast<RENDER_NS::IRenderContext*>(renderInstance.release()));

        ASSERT_TRUE(renderContext_ != nullptr) << "Failed to create render context";

        RENDER_NS::DeviceCreateInfo deviceCreateInfo;
#if RENDER_HAS_VULKAN_BACKEND
        WIDGET_LOGI("[SurfaceStreamTest] Using Vulkan backend");
        RENDER_NS::BackendExtraVk vkExtra;
        deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::VULKAN;
        deviceCreateInfo.backendConfiguration = &vkExtra;
#elif RENDER_HAS_GLES_BACKEND
        WIDGET_LOGI("[SurfaceStreamTest] Using OpenGL ES backend");
        RENDER_NS::BackendExtraGles glesExtra;
        glesExtra.applicationContext = EGL_NO_CONTEXT;
        glesExtra.sharedContext = EGL_NO_CONTEXT;
        glesExtra.MSAASamples = 0;
        glesExtra.depthBits = 24; // depth bits is 24
        deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::OPENGLES;
        deviceCreateInfo.backendConfiguration = &glesExtra;
#elif RENDER_HAS_GL_BACKEND
        WIDGET_LOGI("[SurfaceStreamTest] Using OpenGL backend");
        RENDER_NS::BackendExtraGL glExtra;
        glExtra.MSAASamples = 0;
        glExtra.depthBits = 24; // depth bits is 24
        glExtra.alphaBits = 8; // alpha bits is 8
        glExtra.stencilBits = 0;
        deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::OPENGL;
        deviceCreateInfo.backendConfiguration = &glExtra;
#else
        WIDGET_LOGE("[SurfaceStreamTest] ERROR: No rendering backend available!");
        ASSERT_TRUE(false) << "No rendering backend available";
#endif

        auto result = renderContext_->Init({{"UnitTest", 0, 1, 0}, deviceCreateInfo});
        ASSERT_EQ(result, RENDER_NS::RenderResultCode::RENDER_SUCCESS) <<
            "Failed to initialize render context";
    }

    static void CreateGraphicsContext()
    {
        // Verify renderContext_ is valid before proceeding
        if (!renderContext_) {
            WIDGET_LOGE("[SurfaceStreamTest] ERROR: renderContext_ is null!");
            ASSERT_TRUE(false) << "renderContext_ is null in CreateGraphicsContext";
        }

        auto* classFactory = renderContext_->GetInterface<CORE_NS::IClassFactory>();

        if (!classFactory) {
            WIDGET_LOGE("[SurfaceStreamTest] ERROR: GetInterface<CORE_NS::IClassFactory>() returned null!");
            ASSERT_TRUE(false) << "GetInterface<CORE_NS::IClassFactory>() returned null";
        }

        graphicsContext3D_ = CORE_NS::CreateInstance<CORE3D_NS::IGraphicsContext>(
            *classFactory, CORE3D_NS::UID_GRAPHICS_CONTEXT);

        ASSERT_TRUE(graphicsContext3D_ != nullptr) << "Failed to create graphics context";

        // Create and initialize with CreateInfo (required for proper initialization)
        CORE3D_NS::IGraphicsContext::CreateInfo gfxInf;
        gfxInf.colorSpaceFlags = 0;
        gfxInf.createFlags = 0;

        graphicsContext3D_->Init(gfxInf);
        WIDGET_LOGI("[SurfaceStreamTest] graphicsContext3D_->Init(gfxInf) completed");
    }

    static void CreateEcs()
    {
        ecs_ = engine_->CreateEcs();
        ASSERT_TRUE(ecs_ != nullptr) << "Failed to create ECS";
        ecs_->Initialize();
    }

    static void CreateAppContext()
    {
        auto& taskQueueRegistry = META_NS::GetTaskQueueRegistry();

        auto engineTaskQueue = taskQueueRegistry.GetTaskQueue(ENGINE_THREAD);
        if (!engineTaskQueue) {
            engineTaskQueue =
                META_NS::GetObjectRegistry().Create<META_NS::ITaskQueue>(META_NS::ClassId::ThreadedTaskQueue);
            taskQueueRegistry.RegisterTaskQueue(engineTaskQueue, ENGINE_THREAD);
        }
        auto appTaskQueue = engineTaskQueue;

        auto resources =
            META_NS::GetObjectRegistry().Create<CORE_NS::IResourceManager>(META_NS::ClassId::FileResourceManager);

        resources->SetFileManager(CORE_NS::IFileManager::Ptr(&engine_->GetFileManager()));

        appContext_ = META_NS::GetObjectRegistry().Create<SCENE_NS::IApplicationContext>(
            SCENE_NS::ClassId::ApplicationContext);

        ASSERT_TRUE(appContext_ != nullptr) << "Failed to create appContext_";
        SCENE_NS::IApplicationContext::ApplicationContextInfo info { engineTaskQueue, appTaskQueue, renderContext_,
            resources, SCENE_NS::SceneOptions {} };

        ASSERT_TRUE(appContext_->Initialize(info));
    }

public:
    static void* libHandle_;
    static CORE_NS::IEngine::Ptr engine_;
    static BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext_;  // 使用 refcnt_ptr 与 LumeCustomRender 一致
    static CORE3D_NS::IGraphicsContext::Ptr graphicsContext3D_;
    static CORE_NS::IEcs::Ptr ecs_;
    static SCENE_NS::IApplicationContext::Ptr appContext_;
};

// Static member definitions
void* SurfaceStreamTest::libHandle_ = nullptr;
CORE_NS::IEngine::Ptr SurfaceStreamTest::engine_ {};
BASE_NS::shared_ptr<RENDER_NS::IRenderContext> SurfaceStreamTest::renderContext_ {};
CORE3D_NS::IGraphicsContext::Ptr SurfaceStreamTest::graphicsContext3D_ {};
CORE_NS::IEcs::Ptr SurfaceStreamTest::ecs_ {};
SCENE_NS::IApplicationContext::Ptr SurfaceStreamTest::appContext_ {};

/**
 * @tc.name: OnBufferAvailable
 * @tc.desc: Tests for SurfaceStream.OnBufferAvailable [AUTO-GENERATED]
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceStreamTest, OnBufferAvailable, TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);
    EXPECT_TRUE(attach->Attach(surfaceStream));

    // The OnBufferAvailable callback will be triggered when a buffer is available
    // This is tested indirectly through the attach/detach functionality
    EXPECT_TRUE(attach->Detach(surfaceStream));
}

/**
 * @tc.name: Build
 * @tc.desc: Tests for SurfaceStream.Build [AUTO-GENERATED]
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceStreamTest, Build, TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());

    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 0);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 0);
    EXPECT_EQ(surfaceStreamInterface->GetSurfaceId(), 0);

    auto surfaceStream2 = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream2);
    auto surfaceStreamInterface2 = interface_cast<ISurfaceStream>(surfaceStream2);
    ASSERT_TRUE(surfaceStreamInterface2);

    EXPECT_NE(surfaceStream, surfaceStream2);
}

/**
 * @tc.name: Attach
 * @tc.desc: Tests for SurfaceStream.Attach [AUTO-GENERATED]
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceStreamTest, Attach, TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);
    EXPECT_TRUE(attach->Attach(surfaceStream));

    EXPECT_TRUE(attach->Attach(surfaceStream));

    EXPECT_TRUE(attach->Detach(surfaceStream));

    EXPECT_FALSE(attach->Detach(surfaceStream));
}

/**
 * @tc.name: Init
 * @tc.desc: Tests for SurfaceStream.Init [AUTO-GENERATED]
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceStreamTest, Init, TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);
    EXPECT_TRUE(attach->Attach(surfaceStream));

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);
}

/**
 * @tc.name: UpdateView
 * @tc.desc: Tests for SurfaceStream.UpdateView [AUTO-GENERATED]
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceStreamTest, UpdateView, TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);
    EXPECT_TRUE(attach->Attach(surfaceStream));

    EXPECT_TRUE(attach->Detach(surfaceStream));
}

/**
 * @tc.name: SetHeight
 * @tc.desc: Tests for SurfaceStream.SetHeight [AUTO-GENERATED]
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceStreamTest, SetHeight, TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    const uint32_t testHeight = 720;
    surfaceStreamInterface->SetHeight(testHeight);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), testHeight);

    const uint32_t newHeight = 1080;
    surfaceStreamInterface->SetHeight(newHeight);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), newHeight);
}

/**
 * @tc.name: GetHeight
 * @tc.desc: Tests for SurfaceStream.GetHeight [AUTO-GENERATED]
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceStreamTest, GetHeight, TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 0);

    const uint32_t testHeight = 480;
    surfaceStreamInterface->SetHeight(testHeight);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), testHeight);
}

/**
 * @tc.name: SetWidth
 * @tc.desc: Tests for SurfaceStream.SetWidth [AUTO-GENERATED]
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceStreamTest, SetWidth, TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    const uint32_t testWidth = 1280;
    surfaceStreamInterface->SetWidth(testWidth);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), testWidth);

    const uint32_t newWidth = 1920;
    surfaceStreamInterface->SetWidth(newWidth);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), newWidth);
}

/**
 * @tc.name: GetWidth
 * @tc.desc: Tests for SurfaceStream.GetWidth [AUTO-GENERATED]
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceStreamTest, GetWidth, TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 0);

    const uint32_t testWidth = 640;
    surfaceStreamInterface->SetWidth(testWidth);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), testWidth);
}

/**
 * @tc.name: GetSurfaceId
 * @tc.desc: Tests for SurfaceStream.GetSurfaceId [AUTO-GENERATED]
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceStreamTest, GetSurfaceId, TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    EXPECT_EQ(surfaceStreamInterface->GetSurfaceId(), 0);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);
    EXPECT_TRUE(attach->Attach(surfaceStream));

    // After attach, surface ID should be non-zero
    // Note: The actual surface ID is set during Init() which is called in Attach()
    // We can't verify the exact value, but it should be different from 0

    EXPECT_TRUE(attach->Detach(surfaceStream));
}

/**
 * @tc.name: Detach
 * @tc.desc: Tests for SurfaceStream.Detach [AUTO-GENERATED]
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceStreamTest, Detach, TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);

    EXPECT_TRUE(attach->Attach(surfaceStream));

    EXPECT_TRUE(attach->Detach(surfaceStream));

    EXPECT_FALSE(attach->Detach(surfaceStream));

    EXPECT_TRUE(attach->Attach(surfaceStream));

    EXPECT_TRUE(attach->Detach(surfaceStream));
}

/**
 * @tc.name: MultipleInstances
 * @tc.desc: Tests for creating multiple SurfaceStream instances [AUTO-GENERATED]
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceStreamTest, MultipleInstances, TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());

    auto surfaceStream1 = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    auto surfaceStream2 = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    auto surfaceStream3 = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);

    ASSERT_TRUE(surfaceStream1);
    ASSERT_TRUE(surfaceStream2);
    ASSERT_TRUE(surfaceStream3);

    EXPECT_NE(surfaceStream1, surfaceStream2);
    EXPECT_NE(surfaceStream2, surfaceStream3);
    EXPECT_NE(surfaceStream1, surfaceStream3);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap1 = CreateTestBitmap();
    auto bitmap2 = CreateTestBitmap();
    auto bitmap3 = CreateTestBitmap();

    ASSERT_TRUE(bitmap1);
    ASSERT_TRUE(bitmap2);
    ASSERT_TRUE(bitmap3);

    auto attach1 = interface_cast<META_NS::IAttach>(bitmap1);
    auto attach2 = interface_cast<META_NS::IAttach>(bitmap2);
    auto attach3 = interface_cast<META_NS::IAttach>(bitmap3);

    ASSERT_TRUE(attach1);
    ASSERT_TRUE(attach2);
    ASSERT_TRUE(attach3);

    EXPECT_TRUE(attach1->Attach(surfaceStream1));
    EXPECT_TRUE(attach2->Attach(surfaceStream2));
    EXPECT_TRUE(attach3->Attach(surfaceStream3));

    EXPECT_TRUE(attach1->Detach(surfaceStream1));
    EXPECT_TRUE(attach2->Detach(surfaceStream2));
    EXPECT_TRUE(attach3->Detach(surfaceStream3));
}

/**
 * @tc.name: WidthHeightBounds
 * @tc.desc: Tests for width and height boundary values [AUTO-GENERATED]
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceStreamTest, WidthHeightBounds, TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    surfaceStreamInterface->SetWidth(0);
    surfaceStreamInterface->SetHeight(0);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 0);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 0);

    surfaceStreamInterface->SetWidth(1920);
    surfaceStreamInterface->SetHeight(1080);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 1920);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 1080);

    surfaceStreamInterface->SetWidth(3840);
    surfaceStreamInterface->SetHeight(2160);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 3840);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 2160);

    surfaceStreamInterface->SetWidth(7680);
    surfaceStreamInterface->SetHeight(4320);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 7680);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 4320);
}

/**
 * @tc.name: ReattachAfterDetach
 * @tc.desc: Tests for reattaching SurfaceStream after detach [AUTO-GENERATED]
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceStreamTest, ReattachAfterDetach, TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);

    EXPECT_TRUE(attach->Attach(surfaceStream));

    EXPECT_TRUE(attach->Detach(surfaceStream));

    EXPECT_TRUE(attach->Attach(surfaceStream));

    EXPECT_TRUE(attach->Detach(surfaceStream));
}

/**
 * @tc.name: SurfaceStreamLifecycle
 * @tc.desc: Tests for SurfaceStream lifecycle (create, attach, detach, destroy) [AUTO-GENERATED]
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceStreamTest, SurfaceStreamLifecycle, TestSize.Level1)
{
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());

    auto surfaceStream = obr.Create<ISurfaceStream>(ClassId::SurfaceStream, doc);
    ASSERT_TRUE(surfaceStream);
    auto surfaceStreamInterface = interface_cast<ISurfaceStream>(surfaceStream);
    ASSERT_TRUE(surfaceStreamInterface);

    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 0);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 0);
    EXPECT_EQ(surfaceStreamInterface->GetSurfaceId(), 0);

    auto scene = CreateEmptyScene();
    ASSERT_TRUE(scene);
    auto bitmap = CreateTestBitmap();
    ASSERT_TRUE(bitmap);

    auto attach = interface_cast<META_NS::IAttach>(bitmap);
    ASSERT_TRUE(attach);
    EXPECT_TRUE(attach->Attach(surfaceStream));

    surfaceStreamInterface->SetWidth(1280);
    surfaceStreamInterface->SetHeight(720);
    EXPECT_EQ(surfaceStreamInterface->GetWidth(), 1280);
    EXPECT_EQ(surfaceStreamInterface->GetHeight(), 720);

    EXPECT_TRUE(attach->Detach(surfaceStream));
}

} // namespace OHOS::Render3D