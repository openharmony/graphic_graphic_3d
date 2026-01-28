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

#include "ets_test.h"

#include <dlfcn.h>
#include <memory>

#include <3d/implementation_uids.h>
#include <core/os/platform_create_info.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/engine_info.h>
#include <core/io/intf_file_manager.h>
#include <core/os/intf_platform.h>
#include <core/plugin/intf_class_factory.h>
#include <meta/interface/intf_class_registry.h>
#include <meta/interface/intf_object_context.h>
#include <render/device/intf_device.h>
#include <render/implementation_uids.h>
#include <render/intf_renderer.h>
#include <render/util/intf_render_frame_util.h>
#include <render/util/intf_render_util.h>

#if (RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND)
#include <render/gles/intf_device_gles.h>
#endif

#if (RENDER_HAS_VULKAN_BACKEND)
#include <vulkan/vulkan.h>
#include <render/vulkan/intf_device_vk.h>
#endif

#include "3d_widget_adapter_log.h"
#include "Utils.h"

// using namespace BASE_NS;
// using namespace CORE_NS;
// using namespace RENDER_NS;
// using namespace SCENE_NS;
using IntfPtr = META_NS::SharedPtrIInterface;
using intfWeakPtr = META_NS::WeakPtrIInterface;

CORE_BEGIN_NAMESPACE()
/** Get plugin register */
IPluginRegister &(*GetPluginRegister)() { nullptr };
/** Setup the plugin registry */
void (*CreatePluginRegistry)(const struct PlatformCreateInfo& platformCreateInfo) { nullptr };
/** Get where engine is build in debug mode */
bool (*IsDebugBuild)() { nullptr };
/** Get version */
BASE_NS::string_view (*GetVersion)() { nullptr };
CORE_END_NAMESPACE()

template<typename T>
bool LoadFunc(T &fn, const char *fName, void *handle)
{
    fn = reinterpret_cast<T>(dlsym(handle, fName));
    return fn != nullptr;
}

namespace OHOS::Render3D {
void EtsTest::SetUp()
{
    LoadEngineLib();
#define TO_STRING(name) #name
#define PLATFORM_PATH_NAME(name) TO_STRING(name)
    CORE_NS::PlatformCreateInfo platformCreateInfo{
        PLATFORM_PATH_NAME(PLATFORM_CORE_ROOT_PATH),
        PLATFORM_PATH_NAME(PLATOFRM_CORE_PLUGIN_PATH),
        PLATFORM_PATH_NAME(PLATFORM_APP_ROOT_PATH),
        PLATFORM_PATH_NAME(PLATFORM_APP_PLUGIN_PATH),
    };
#undef TO_STRING
#undef PLATFORM_PATH_NAME
    LoadPlugins(platformCreateInfo);
    CreateEngine(platformCreateInfo);
    CreateRenderContext();
    CreateGraphicsContext();
    CreateApplicationContext();
}

void EtsTest::LoadEngineLib()
{
    libHandle_ = dlopen(LIB_ENGINE_CORE, RTLD_LAZY);
    ASSERT_TRUE(LoadFunc(CORE_NS::CreatePluginRegistry, "_ZN4Core20CreatePluginRegistryERKNS_18PlatformCreateInfoE", libHandle_));
    ASSERT_TRUE(LoadFunc(CORE_NS::GetPluginRegister, "_ZN4Core17GetPluginRegisterEv", libHandle_));
    ASSERT_TRUE(LoadFunc(CORE_NS::IsDebugBuild, "_ZN4Core12IsDebugBuildEv", libHandle_));
    ASSERT_TRUE(LoadFunc(CORE_NS::GetVersion, "_ZN4Core13GetVersionRevEv", libHandle_));
}

void EtsTest::LoadPlugins(const CORE_NS::PlatformCreateInfo &platformCreateInfo)
{
    CORE_NS::CreatePluginRegistry(platformCreateInfo);
    constexpr BASE_NS::Uid plugins[] = { SCENE_NS::UID_SCENE_PLUGIN };
    ASSERT_TRUE(CORE_NS::GetPluginRegister().LoadPlugins(plugins));
}

void EtsTest::CreateEngine(const CORE_NS::PlatformCreateInfo &platformCreateInfo)
{
    auto factory = CORE_NS::GetInstance<CORE_NS::IEngineFactory>(CORE_NS::UID_ENGINE_FACTORY);
    engine_ = factory->Create({platformCreateInfo, {"EtsTest", 0, 1, 0}, {}});
    auto &fileManager = engine_->GetFileManager();
    const auto &platform = engine_->GetPlatform();
    platform.RegisterDefaultPaths(fileManager);
    engine_->Init();
}

void EtsTest::CreateRenderContext()
{
    renderContext_.reset(static_cast<RENDER_NS::IRenderContext *>(
        engine_->GetInterface<CORE_NS::IClassFactory>()->CreateInstance(RENDER_NS::UID_RENDER_CONTEXT).release()));

    RENDER_NS::DeviceCreateInfo deviceCreateInfo;
#if RENDER_HAS_VULKAN_BACKEND
    RENDER_NS::BackendExtraVk vkExtra;
    deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::VULKAN;
    deviceCreateInfo.backendConfiguration = &vkExtra;
#elif RENDER_HAS_GLES_BACKEND
    RENDER_NS::BackendExtraGles glesExtra;
    glesExtra.applicationContext = EGL_NO_CONTEXT;
    glesExtra.sharedContext = EGL_NO_CONTEXT;
    glesExtra.MSAASamples = 0;
    glesExtra.depthBits = 24; // 24: bits of depth
    deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::OPENGLES;
    deviceCreateInfo.backendConfiguration = &glesExtra;
#elif RENDER_HAS_GL_BACKEND
    RENDER_NS::BackendExtraGL glExtra;
    glExtra.MSAASamples = 0;
    glExtra.depthBits = 24; // 24: bits of depth
    glExtra.alphaBits = 8; // 8: bits of alpha
    glExtra.stencilBits = 0;
    deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::OPENGL;
    deviceCreateInfo.backendConfiguration = &glExtra;
#endif
    auto rrc = renderContext_->Init({{"EtsTest", 0, 1, 0}, deviceCreateInfo});
    ASSERT_EQ(rrc, RENDER_NS::RenderResultCode::RENDER_SUCCESS);
}

void EtsTest::CreateGraphicsContext()
{
    graphicsContext3D_ = CORE_NS::CreateInstance<CORE3D_NS::IGraphicsContext>(
        *renderContext_->GetInterface<CORE_NS::IClassFactory>(), CORE3D_NS::UID_GRAPHICS_CONTEXT);
    graphicsContext3D_->Init();
}

void EtsTest::CreateApplicationContext()
{
    auto &tr = META_NS::GetTaskQueueRegistry();
    auto &obr = META_NS::GetObjectRegistry();
    auto engineTaskQueue = obr.Create<META_NS::ITaskQueue>(META_NS::ClassId::ThreadedTaskQueue);
    tr.RegisterTaskQueue(engineTaskQueue, ENGINE_THREAD);
    auto appTaskQueue = engineTaskQueue;
    auto resources = obr.Create<CORE_NS::IResourceManager>(META_NS::ClassId::FileResourceManager);
    auto &fileManager = engine_->GetFileManager();
    resources->SetFileManager(CORE_NS::IFileManager::Ptr(&fileManager));

    applicationContext_ = obr.Create<SCENE_NS::IApplicationContext>(SCENE_NS::ClassId::ApplicationContext);
    ASSERT_TRUE(applicationContext_);
    SCENE_NS::IApplicationContext::ApplicationContextInfo info{
        engineTaskQueue, appTaskQueue, renderContext_, resources, SCENE_NS::SceneOptions{}};
    ASSERT_TRUE(applicationContext_->Initialize(info));

    auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    ASSERT_TRUE(doc);
    auto flags = META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE;
    doc->AddProperty(META_NS::ConstructProperty<IntfPtr>("EngineQueue", engineTaskQueue, flags));
    doc->AddProperty(META_NS::ConstructProperty<IntfPtr>("AppQueue", appTaskQueue, flags));
    doc->AddProperty(META_NS::ConstructProperty<SCENE_NS::IRenderContext::Ptr>(
        "RenderContext", applicationContext_->GetRenderContext(), flags));
}

void EtsTest::TearDown()
{
    auto &tr = META_NS::GetTaskQueueRegistry();
    tr.UnregisterAllTaskQueues();

    auto aq = applicationContext_->GetRenderContext()->GetApplicationQueue();
    auto rq = applicationContext_->GetRenderContext()->GetRenderQueue();
    auto rc = applicationContext_->GetRenderContext()->GetRenderer();
    applicationContext_->GetRenderContext()->GetResources()->RemoveAllResources();
    applicationContext_.reset();
    // flush the queue
    META_NS::AddFutureTaskOrRunDirectly(rq, [] {}).Wait();

    auto &obr = META_NS::GetObjectRegistry();
    auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    ASSERT_TRUE(doc);
    doc->RemoveProperty(doc->GetProperty<IntfPtr>("EngineQueue"));
    doc->RemoveProperty(doc->GetProperty<IntfPtr>("AppQueue"));
    doc->RemoveProperty(doc->GetProperty<IntfPtr>("RenderContext"));
    rq.reset();
    aq.reset();
    rc.reset();

    applicationContext_.reset();
    graphicsContext3D_.reset();
    renderContext_.reset();
    engine_.reset();

    if (libHandle_) {
        dlclose(libHandle_);
        libHandle_ = nullptr;
    }
}

}  // namespace OHOS::Render3D


