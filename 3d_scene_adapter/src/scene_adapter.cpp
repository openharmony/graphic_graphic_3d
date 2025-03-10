/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "scene_adapter/scene_adapter.h"

#include <dlfcn.h>
#include <memory>
#include <string_view>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include <base/containers/array_view.h>

#include <core/intf_engine.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/engine_info.h>
#include <core/implementation_uids.h>
#include <core/io/intf_file_manager.h>
#include <core/namespace.h>
#include <core/os/intf_platform.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/intf_property_handle.h>

#include <jpg/implementation_uids.h>

#include <meta/interface/intf_meta_object_lib.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/base/shared_ptr.h>
#include <meta/base/interface_macros.h>
#include <meta/api/make_callback.h>
#include <meta/ext/object.h>

#include <png/implementation_uids.h>
#include <scene/base/namespace.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_material.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_camera.h>
#include <scene/ext/intf_internal_scene.h>

#include "ability.h"

#include "data_ability_helper.h"

#include "napi_base_context.h"

#include <render/implementation_uids.h>
#include <render/gles/intf_device_gles.h>
#include <render/intf_renderer.h>
#include <render/intf_render_context.h>

#include "3d_widget_adapter_log.h"
#include "widget_trace.h"
#include "ohos/texture_layer.h"
#include "lume_render_config.h"

namespace OHOS::Render3D {

HapInfo GetHapInfo()
{
    WIDGET_SCOPED_TRACE("SceneAdapter::GetHapInfo");
    std::shared_ptr<AbilityRuntime::ApplicationContext> context =
        AbilityRuntime::ApplicationContext::GetApplicationContext();
    if (!context) {
        WIDGET_LOGE("Failed to get application context.");
        return {};
    }
    auto resourceManager = context->GetResourceManager();
    if (!resourceManager) {
        WIDGET_LOGE("Failed to get resource manager.");
        return {};
    }
    HapInfo hapInfo;
    hapInfo.bundleName_ = resourceManager->bundleInfo.first;
    hapInfo.moduleName_ = resourceManager->bundleInfo.second;
    hapInfo.resourceManager_ = context->CreateModuleResourceManager(hapInfo.bundleName_, hapInfo.moduleName_);
    // hapPath
    std::string hapPath = context->GetBundleCodeDir();
    hapInfo.hapPath_ = hapPath + "/" + hapInfo.moduleName_ + ".hap";
    WIDGET_LOGD("bundle %s, module %s, hapPath %s",
        hapInfo.bundleName_.c_str(),
        hapInfo.moduleName_.c_str(),
        hapInfo.hapPath_.c_str());

    return hapInfo;
}

using IntfPtr = META_NS::SharedPtrIInterface;
using IntfWeakPtr = META_NS::WeakPtrIInterface;

struct EngineInstance {
    void *libHandle_ = nullptr;
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext_;
    BASE_NS::shared_ptr<CORE_NS::IEngine> engine_;
};

static EngineInstance engineInstance_;
static std::mutex mute;
static HapInfo hapInfo_;
META_NS::ITaskQueue::Ptr engineThread;
META_NS::ITaskQueue::Ptr ioThread;
META_NS::ITaskQueue::Ptr releaseThread;
META_NS::ITaskQueue::Token renderTask {};

void LockCompositor()
{
    mute.lock();
}

void UnlockCompositor()
{
    mute.unlock();
}

static constexpr BASE_NS::Uid ENGINE_THREAD { "2070e705-d061-40e4-bfb7-90fad2c280af" };
static constexpr BASE_NS::Uid APP_THREAD { "b2e8cef3-453a-4651-b564-5190f8b5190d" };
static constexpr BASE_NS::Uid IO_QUEUE { "be88e9a0-9cd8-45ab-be48-937953dc258f" };
static constexpr BASE_NS::Uid JS_RELEASE_THREAD { "3784fa96-b25b-4e9c-bbf1-e897d36f73af" };

template<typename T>
bool LoadFunc(T &fn, const char *fName, void* handle)
{
    fn = reinterpret_cast<T>(dlsym(handle, fName));
    if (fn == nullptr) {
        WIDGET_LOGE("%s open %s", __func__, dlerror());
        return false;
    }
    return true;
}

SceneAdapter::~SceneAdapter()
{}

SceneAdapter::SceneAdapter()
{
    WIDGET_LOGD("scene adapter Impl create");
}

bool SceneAdapter::LoadEngineLib()
{
    if (engineInstance_.libHandle_ != nullptr) {
        WIDGET_LOGD("%s, already loaded", __func__);
        return true;
    }

    #define TO_STRING(name) #name
    #define LIB_NAME(name) TO_STRING(name)
    constexpr std::string_view lib { LIB_NAME(LIB_ENGINE_CORE)".so" };
    engineInstance_.libHandle_ = dlopen(lib.data(), RTLD_LAZY);

    if (engineInstance_.libHandle_ == nullptr) {
        WIDGET_LOGE("%s, open lib fail %s", __func__, dlerror());
    }
    #undef TO_STRING
    #undef LIB_NAME

    #define LOAD_FUNC(fn, name) LoadFunc<decltype(fn)>(fn, name, engineInstance_.libHandle_)
    if (!(LOAD_FUNC(CORE_NS::CreatePluginRegistry,
        "_ZN4Core20CreatePluginRegistryERKNS_18PlatformCreateInfoE")
        && LOAD_FUNC(CORE_NS::GetPluginRegister, "_ZN4Core17GetPluginRegisterEv")
        && LOAD_FUNC(CORE_NS::IsDebugBuild, "_ZN4Core12IsDebugBuildEv")
        && LOAD_FUNC(CORE_NS::GetVersion, "_ZN4Core13GetVersionRevEv"))) {
        return false;
    }
    #undef LOAD_FUNC

    return true;
}

bool SceneAdapter::LoadPlugins(const CORE_NS::PlatformCreateInfo& platformCreateInfo)
{
    if (engineInstance_.engine_) {
        return true;
    }
    if (!LoadEngineLib()) {
        return false;
    }
    WIDGET_LOGI("load engine suceess!");
    CORE_NS::CreatePluginRegistry(platformCreateInfo);

    CORE_NS::GetLogger()->SetLogLevel(CORE_NS::ILogger::LogLevel::LOG_VERBOSE); // All logs for now.

    const BASE_NS::Uid pluginList[] {
#if defined(USE_LIB_PNG_JPEG_DYNAMIC_PLUGIN) && USE_LIB_PNG_JPEG_DYNAMIC_PLUGIN
        JPGPlugin::UID_JPG_PLUGIN,
        PNGPlugin::UID_PNG_PLUGIN,
#endif
        SCENE_NS::UID_SCENE_PLUGIN
    };
    if (!CORE_NS::GetPluginRegister().LoadPlugins(pluginList)) {
        WIDGET_LOGE("fail to load plugins");
        return false;
    }
    WIDGET_LOGI("load plugin success");
    return true;
}

bool SceneAdapter::InitEngine(CORE_NS::PlatformCreateInfo platformCreateInfo)
{
    if (engineInstance_.engine_) {
        return true;
    }
    auto& tr = META_NS::GetTaskQueueRegistry();
    auto& obr = META_NS::GetObjectRegistry();

    engineThread = tr.GetTaskQueue(ENGINE_THREAD);
    if (!engineThread) {
        engineThread = obr.Create<META_NS::ITaskQueue>(META_NS::ClassId::ThreadedTaskQueue);
        tr.RegisterTaskQueue(engineThread, ENGINE_THREAD);
    }
    ioThread = tr.GetTaskQueue(IO_QUEUE);
    if (!ioThread) {
        ioThread = obr.Create<META_NS::ITaskQueue>(META_NS::ClassId::ThreadedTaskQueue);
        tr.RegisterTaskQueue(ioThread, IO_QUEUE);
    }
    releaseThread = tr.GetTaskQueue(JS_RELEASE_THREAD);
    if (!releaseThread) {
        auto &obr = META_NS::GetObjectRegistry();
        releaseThread = obr.Create<META_NS::ITaskQueue>(META_NS::ClassId::ThreadedTaskQueue);
        tr.RegisterTaskQueue(releaseThread, JS_RELEASE_THREAD);
    }

// auto engineInit = META_NS::MakeCallback<META_NS::ITaskQueueTask>([platformCreateInfo]() {
    auto engineInit = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([platformCreateInfo]() {
        auto &obr = META_NS::GetObjectRegistry();
        // Initialize lumeengine/render etc
        CORE_NS::EngineCreateInfo engineCreateInfo{platformCreateInfo, {}, {}};
        if (auto factory = CORE_NS::GetInstance<CORE_NS::IEngineFactory>(CORE_NS::UID_ENGINE_FACTORY)) {
            engineInstance_.engine_.reset(factory->Create(engineCreateInfo).get());
        } else {
            WIDGET_LOGE("could not get engine factory");
            return META_NS::IAny::Ptr {};
        }
        if (!engineInstance_.engine_) {
            WIDGET_LOGE("get engine fail");
            return META_NS::IAny::Ptr {};
        }
        auto &fileManager = engineInstance_.engine_->GetFileManager();
        const auto &platform = engineInstance_.engine_->GetPlatform();
        platform.RegisterDefaultPaths(fileManager);
        engineInstance_.engine_->Init();

        engineInstance_.renderContext_.reset(CORE_NS::CreateInstance<RENDER_NS::IRenderContext>(
            *engineInstance_.engine_->GetInterface<Core::IClassFactory>(), RENDER_NS::UID_RENDER_CONTEXT).get());

        RENDER_NS::RenderCreateInfo renderCreateInfo{};
        RENDER_NS::BackendExtraGLES glExtra{};
        Render::DeviceCreateInfo deviceCreateInfo{};

        std::string backendProp = LumeRenderConfig::GetInstance().renderBackend_ == "force_vulkan" ? "vulkan" : "gles";
        if (backendProp == "vulkan") {
        } else {
            glExtra.depthBits = 24; // 24 : bits size
            glExtra.sharedContext = EGL_NO_CONTEXT;
            deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::OPENGLES;
            deviceCreateInfo.backendConfiguration = &glExtra;
            renderCreateInfo.applicationInfo = {};
            renderCreateInfo.deviceCreateInfo = deviceCreateInfo;
        }

        auto rrc = engineInstance_.renderContext_->Init(renderCreateInfo);
        if (rrc != RENDER_NS::RenderResultCode::RENDER_SUCCESS) {
            WIDGET_LOGE("Failed to create render context");
            return META_NS::IAny::Ptr {};
        }

        // Save the stuff to the default object context.
        auto engineThread = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
        // thats the javascript thread...
        auto appThread = engineThread;
        auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        if (doc == nullptr) {
            WIDGET_LOGE("nullptr from interface_cast");
            return META_NS::IAny::Ptr {};
        }
        auto flags = META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE;

        doc->AddProperty(META_NS::ConstructProperty<IntfPtr>("RenderContext", nullptr, flags));
        doc->AddProperty(META_NS::ConstructProperty<IntfPtr>("EngineQueue", nullptr, flags));
        doc->AddProperty(META_NS::ConstructProperty<IntfPtr>("AppQueue", nullptr, flags));
        doc->AddProperty(META_NS::ConstructArrayProperty<IntfWeakPtr>("Scenes", {}, flags));

        doc->GetProperty<META_NS::SharedPtrIInterface>("EngineQueue")->SetValue(engineThread);
        doc->GetProperty<META_NS::SharedPtrIInterface>("AppQueue")->SetValue(appThread);
        doc->GetProperty<META_NS::SharedPtrIInterface>("RenderContext")->SetValue(engineInstance_.renderContext_);

        WIDGET_LOGD("register shader paths");
        static constexpr const RENDER_NS::IShaderManager::ShaderFilePathDesc desc { "shaders://" };
        engineInstance_.engine_->GetFileManager().RegisterPath("shaders", "OhosRawFile://shaders", false);
        engineInstance_.renderContext_->GetDevice().GetShaderManager().LoadShaderFiles(desc);

        engineInstance_.engine_->GetFileManager().RegisterPath("appshaders", "OhosRawFile://shaders", false);
        engineInstance_.engine_->GetFileManager().RegisterPath("apppipelinelayouts",
            "OhosRawFile:///pipelinelayouts/", true);
        engineInstance_.engine_->GetFileManager().RegisterPath("fonts", "OhosRawFile:///fonts", true);

        static constexpr const RENDER_NS::IShaderManager::ShaderFilePathDesc desc1 { "appshaders://" };
        engineInstance_.renderContext_->GetDevice().GetShaderManager().LoadShaderFiles(desc1);

        // "render" one frame. this is to initialize all the default resources etc.
        // if we never render a single frame, we will get "false positive" leaks of gpu resources.
        engineInstance_.renderContext_->GetRenderer().RenderFrame({});
        WIDGET_LOGI("init engine success");
        return META_NS::IAny::Ptr {};
    });
    engineThread->AddWaitableTask(engineInit)->Wait();
    return true;
}

void SceneAdapter::SetSceneObj(META_NS::IObject::Ptr pt)
{
    WIDGET_LOGD("SceneAdapterImpl::SetSceneObj");
    sceneWidgetObj_ = pt;
}

std::shared_ptr<TextureLayer> SceneAdapter::CreateTextureLayer()
{
    auto cb = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([this]() {
        textureLayer_ = std::make_shared<TextureLayer>(key_);
        key_++;
        return META_NS::IAny::Ptr {};
    });
    META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD)->AddWaitableTask(cb)->Wait();
    return textureLayer_;
}

bool SceneAdapter::LoadPluginsAndInit()
{
    LockCompositor(); // an APP_FREEZE here, so add lock just in case, but suspect others' error
    WIDGET_LOGI("scene adapter loadPlugins");

    if (hapInfo_.hapPath_ == "") {
        hapInfo_ = GetHapInfo();
    }

    #define TO_STRING(name) #name
    #define PLATFORM_PATH_NAME(name) TO_STRING(name)
    CORE_NS::PlatformCreateInfo platformCreateInfo {
        PLATFORM_PATH_NAME(PLATFORM_CORE_ROOT_PATH),
        PLATFORM_PATH_NAME(PLATFORM_CORE_PLUGIN_PATH),
        PLATFORM_PATH_NAME(PLATFORM_APP_ROOT_PATH),
        PLATFORM_PATH_NAME(PLATFORM_APP_PLUGIN_PATH),
        hapInfo_.hapPath_.c_str(),
        hapInfo_.bundleName_.c_str(),
        hapInfo_.moduleName_.c_str(),
        hapInfo_.resourceManager_
    };
    #undef TO_STRING
    #undef PLATFORM_PATH_NAME
    if (!LoadPlugins(platformCreateInfo)) {
        UnlockCompositor();
        return false;
    }

    if (!InitEngine(platformCreateInfo)) {
        UnlockCompositor();
        return false;
    }

    CreateRenderFunction();
    UnlockCompositor();
    return true;
}

void SceneAdapter::OnWindowChange(const WindowChangeInfo &windowChangeInfo)
{
    WIDGET_LOGI("SceneAdapter::OnWindowchange");
    auto cb = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([this, &windowChangeInfo]() {
        textureLayer_->OnWindowChange(windowChangeInfo);
        const auto& textureInfo = textureLayer_->GetTextureInfo();
        auto& device = engineInstance_.renderContext_->GetDevice();
        RENDER_NS::SwapchainCreateInfo swapchainCreateInfo {
            0U,
            RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_COLOR_BUFFER_BIT |
            RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_DEPTH_BUFFER_BIT |
            RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_SRGB_BIT,
            RENDER_NS::ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            {
                reinterpret_cast<uintptr_t>(textureInfo.nativeWindow_),
                reinterpret_cast<uintptr_t>(static_cast<const RENDER_NS::DevicePlatformDataGLES&>(
                    device.GetPlatformData()).display)
            }
        };
        swapchainHandle_ = device.CreateSwapchainHandle(swapchainCreateInfo, swapchainHandle_, {});
        return META_NS::IAny::Ptr {};
    });

    META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD)->AddWaitableTask(cb)->Wait();
}

void SceneAdapter::CreateRenderFunction()
{
    // Task used for oneshot renders
    singleFrameAsync = META_NS::MakeCallback<META_NS::ITaskQueueTask>([this]() {
        RenderFunction();
        return 0;
    });
    // Task used for oneshot synchronous renders
    singleFrameSync = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([this]() {
        RenderFunction();
        return META_NS::IAny::Ptr {};
    });
}

void SceneAdapter::DeinitRenderThread()
{
    if (renderTask) {
        engineThread->CancelTask(renderTask);
        renderTask = nullptr;
    }
    auto engine_deinit = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([]() {
        // destroy all swapchains
        auto &obr = META_NS::GetObjectRegistry();
        auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        if (doc == nullptr) {
            WIDGET_LOGE("nullptr from interface_cast");
            return META_NS::IAny::Ptr {};
        }
        // hmm.. this is somewhat uncool
        {
            auto p1 = doc->GetProperty<IntfPtr>("EngineQueue");
            doc->RemoveProperty(p1);
            auto p2 = doc->GetProperty<IntfPtr>("AppQueue");
            doc->RemoveProperty(p2);
            auto p3 = doc->GetProperty<IntfPtr>("RenderContext");
            doc->RemoveProperty(p3);
        }

        doc->GetArrayProperty<IntfWeakPtr>("Scenes")->Reset();
        engineInstance_.renderContext_.reset();
        engineInstance_.engine_.reset();

        return META_NS::IAny::Ptr{};
    });
    engineThread->AddWaitableTask(engine_deinit)->Wait();
    auto &tr = META_NS::GetTaskQueueRegistry();
    tr.UnregisterTaskQueue(ENGINE_THREAD);
    engineThread.reset();
    tr.UnregisterTaskQueue(IO_QUEUE);
    ioThread.reset();
    tr.UnregisterTaskQueue(JS_RELEASE_THREAD);
    releaseThread.reset();
}

void SceneAdapter::ShutdownPluginRegistry()
{
    if (engineInstance_.libHandle_ == nullptr) {
        return;
    }
    dlclose(engineInstance_.libHandle_);
    engineInstance_.libHandle_ = nullptr;

    CORE_NS::GetPluginRegister = nullptr;
    CORE_NS::CreatePluginRegistry = nullptr;
    CORE_NS::IsDebugBuild = nullptr;
    CORE_NS::GetVersion = nullptr;
}

void SceneAdapter::RenderFunction()
{
    WIDGET_SCOPED_TRACE("SceneAdapter::RenderFunction");
    auto &obr = META_NS::GetObjectRegistry();
    auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> rc;
    if (auto prp = doc->GetProperty<IntfPtr>("RenderContext")) {
        rc = interface_pointer_cast<RENDER_NS::IRenderContext>(prp->GetValue());
    }
    LockCompositor();
    if (!sceneWidgetObj_) {
        UnlockCompositor();
        return;
    }
    auto scene = interface_pointer_cast<SCENE_NS::IScene>(sceneWidgetObj_);
    if (!bitmap_) {
        auto cams = scene->GetCameras().GetResult();
        if (!cams.empty()) {
            for (auto c : cams) {
                AttachSwapchain(interface_pointer_cast<META_NS::IObject>(c), swapchainHandle_);
            }
        }
    }
    scene->GetInternalScene()->Update();
    rc->GetRenderer().RenderDeferredFrame();
    UnlockCompositor();
}

void SceneAdapter::RenderFrame(bool needsSyncPaint)
{
    if (!engineThread) {
        WIDGET_LOGE("no engineThread for Render");
        return;
    }
    if (renderTask) {
        engineThread->CancelTask(renderTask);
        renderTask = nullptr;
    }

    if (!needsSyncPaint && singleFrameAsync) {
        renderTask = engineThread->AddTask(singleFrameAsync);
    } else if (singleFrameSync) {
        engineThread->AddWaitableTask(singleFrameSync)->Wait();
    } else {
        WIDGET_LOGE("No render function available.");
    }
}

bool SceneAdapter::NeedsRepaint()
{
    return needsRepaint_;
}

void SceneAdapter::SetNeedsRepaint(bool needsRepaint)
{
    needsRepaint_ = needsRepaint;
}

void SceneAdapter::Deinit()
{
    if (!engineThread) {
        return;
    }
    WIDGET_LOGI("SceneAdapter::Deinit");
    auto func = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([this]() {
        if (bitmap_) {
            auto scene = interface_pointer_cast<SCENE_NS::IScene>(sceneWidgetObj_);
            if (!scene) {
                return META_NS::IAny::Ptr {};
            }
            if (auto i = interface_cast<SCENE_NS::IRenderResource>(bitmap_)) {
                i->SetRenderHandle(scene->GetInternalScene(), swapchainHandle_);
            }
        }
        if (swapchainHandle_) {
            auto& device = engineInstance_.renderContext_->GetDevice();
            device.DestroySwapchain(swapchainHandle_);
        }
        swapchainHandle_ = {};
        return META_NS::IAny::Ptr {};
    });
    engineThread->AddWaitableTask(func)->Wait();

    sceneWidgetObj_.reset();
    singleFrameAsync.reset();
    singleFrameSync.reset();
    needsRepaint_ = false;
}

void SceneAdapter::AttachSwapchain(META_NS::IObject::Ptr cameraObj, RENDER_NS::RenderHandleReference swapchain)
{
    WIDGET_LOGI("attach swapchain");
    auto node = interface_cast<SCENE_NS::INode>(cameraObj);
    if (node == nullptr) {
        WIDGET_LOGE("cast cameraObj failed in AttachSwapchain.");
        return;
    }
    auto scene = node->GetScene();
    auto camera = interface_pointer_cast<SCENE_NS::ICamera>(cameraObj);
    if (camera->IsActive()) {
        bitmap_ = META_NS::GetObjectRegistry().Create<SCENE_NS::IRenderTarget>(SCENE_NS::ClassId::Bitmap);

        const auto &info = textureLayer_->GetTextureInfo();
        if (auto i = interface_cast<SCENE_NS::IRenderResource>(bitmap_)) {
            i->SetRenderHandle(scene->GetInternalScene(), swapchainHandle_);
        }
        camera->SetRenderTarget(bitmap_);
    }
}

} // namespace OHOS::Render3D
