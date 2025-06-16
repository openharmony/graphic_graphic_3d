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
#include <base/containers/shared_ptr.h>

#include <core/intf_engine.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/engine_info.h>
#include <core/image/intf_image_loader_manager.h>
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
#include <meta/base/interface_macros.h>
#include <meta/api/make_callback.h>
#include <meta/ext/object.h>

#include <png/implementation_uids.h>
#include <scene/base/namespace.h>
#include <scene/interface/intf_scene.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/uri_component.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_material.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_camera.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/interface/intf_application_context.h>

#include <scene/ext/intf_ecs_object_access.h>
#include <text_3d/implementation_uids.h>
#include "ability.h"

#include "data_ability_helper.h"

#include "napi_base_context.h"

#include <render/implementation_uids.h>
#include <render/gles/intf_device_gles.h>
#include <render/intf_renderer.h>
#include <render/intf_render_context.h>
#include <render/util/intf_render_util.h>
#include <render/vulkan/intf_device_vk.h>

#include "3d_widget_adapter_log.h"
#include "widget_trace.h"
#include "ohos/texture_layer.h"
#include "lume_render_config.h"

#include <cam_preview/namespace.h>
#include <cam_preview/implementation_uids.h>

// surface buffer
#include <native_buffer.h>
#include <surface_buffer.h>
#include <securec.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>

namespace OHOS::Render3D {
#define RETURN_IF_NULL(ptr)                                  \
    do {                                                     \
        if (!(ptr)) {                                        \
            WIDGET_LOGE("%s is null in %s", #ptr, __func__); \
            return;                                          \
        }                                                    \
    } while (0)

#define RETURN_FALSE_IF_NULL(ptr)                            \
    do {                                                     \
        if (!(ptr)) {                                        \
            WIDGET_LOGE("%s is null in %s", #ptr, __func__); \
            return false;                                    \
        }                                                    \
    } while (0)

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
    SCENE_NS::IApplicationContext::Ptr applicationContext_;
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
static constexpr uint32_t FLOAT_TO_BYTE = sizeof(float) / sizeof(uint8_t);
static constexpr uint32_t TRANSFORM_MATRIX_SIZE = 16;
static const char* const RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
static const char* const ENV_PROPERTIES_BUFFER_NAME = "AR_CAMERA_TRANSFORM_MATRIX";
static constexpr uint64_t BUFFER_FENCE_HANDLE_BASE { 0xAAAAAAAAaaaaaaaa };
static constexpr uint32_t TIME_OUT_MILLISECONDS = 5000;

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
{
}

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
    if (engineInstance_.libHandle_) {
        WIDGET_LOGI("%s, already loaded", __func__);
        return true;
    }
    if (!LoadEngineLib()) {
        return false;
    }
    WIDGET_LOGD("load engine suceess!");

    BASE_NS::vector<BASE_NS::Uid> DefaultPluginVector = {
        CAM_PREVIEW_NS::UID_CAMERA_PREVIEW_PLUGIN,
        SCENE_NS::UID_SCENE_PLUGIN,
#if defined(USE_LIB_PNG_JPEG_DYNAMIC_PLUGIN) && USE_LIB_PNG_JPEG_DYNAMIC_PLUGIN
        JPGPlugin::UID_JPG_PLUGIN,
        PNGPlugin::UID_PNG_PLUGIN,
#endif
    };
    const BASE_NS::array_view<BASE_NS::Uid> DefaultPluginList(DefaultPluginVector.data(), DefaultPluginVector.size());
    CORE_NS::CreatePluginRegistry(platformCreateInfo);
    if (!CORE_NS::GetPluginRegister().LoadPlugins(DefaultPluginList)) {
        WIDGET_LOGE("fail to load scene widget plugin");
        return false;
    }
    WIDGET_LOGI("load plugin success");
    return true;
}

bool SceneAdapter::InitEngine(CORE_NS::PlatformCreateInfo platformCreateInfo)
{
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

    bool inited = false;
    auto initCheck = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([&inited]() {
        inited = (engineInstance_.engine_ != nullptr);
        return META_NS::IAny::Ptr {};
    });
    engineThread->AddWaitableTask(initCheck)->Wait();
    if (inited) {
        WIDGET_LOGI("engine already inited");
        return true;
    }

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

        RENDER_NS::RenderCreateInfo renderCreateInfo{};
        RENDER_NS::BackendExtraGLES glExtra{};
        Render::DeviceCreateInfo deviceCreateInfo{};

        std::string backendProp = LumeRenderConfig::GetInstance().renderBackend_ == "force_vulkan" ? "vulkan" : "gles";
        if (backendProp == "vulkan") {
            WIDGET_LOGI("backend vulkan");
        } else {
            WIDGET_LOGI("backend gles");
            glExtra.depthBits = 24; // 24 : bits size
            glExtra.sharedContext = EGL_NO_CONTEXT;
            deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::OPENGLES;
            deviceCreateInfo.backendConfiguration = &glExtra;
            renderCreateInfo.applicationInfo = {};
            renderCreateInfo.deviceCreateInfo = deviceCreateInfo;
        }

        engineInstance_.renderContext_.reset(
            static_cast<RENDER_NS::IRenderContext *>(
                engineInstance_.engine_->GetInterface<CORE_NS::IClassFactory>()
                    ->CreateInstance(RENDER_NS::UID_RENDER_CONTEXT)
                    .release()));

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

        doc->AddProperty(META_NS::ConstructProperty<IntfPtr>("EngineQueue", nullptr, flags));
        doc->AddProperty(META_NS::ConstructProperty<IntfPtr>("AppQueue", nullptr, flags));
        doc->AddProperty(META_NS::ConstructArrayProperty<IntfWeakPtr>("Scenes", {}, flags));

        // Use engine file manager for our resource manager
        auto resources =
            META_NS::GetObjectRegistry().Create<CORE_NS::IResourceManager>(
                    META_NS::ClassId::FileResourceManager);
        resources->SetFileManager(CORE_NS::IFileManager::Ptr(&fileManager));

        engineInstance_.applicationContext_ =
            META_NS::GetObjectRegistry().Create<SCENE_NS::IApplicationContext>(
                    SCENE_NS::ClassId::ApplicationContext);
        if (engineInstance_.applicationContext_) {
            SCENE_NS::IApplicationContext::ApplicationContextInfo info{
                engineThread, appThread, engineInstance_.renderContext_,
                    resources, SCENE_NS::SceneOptions{}};
            engineInstance_.applicationContext_->Initialize(info);
        }

        doc->GetProperty<META_NS::SharedPtrIInterface>("EngineQueue")->SetValue(engineThread);
        doc->GetProperty<META_NS::SharedPtrIInterface>("AppQueue")->SetValue(appThread);
        doc->AddProperty(META_NS::ConstructProperty<SCENE_NS::IRenderContext::Ptr>(
                    "RenderContext", engineInstance_.applicationContext_->GetRenderContext(), flags));

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
    WIDGET_LOGI("SceneAdapter::CreateTextureLayer: %u", key_);
    auto cb = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([this]() {
        textureLayer_ = std::make_shared<TextureLayer>(key_);
        key_++;
        return META_NS::IAny::Ptr {};
    });
    if (engineThread) {
        engineThread->AddWaitableTask(cb)->Wait();
    } else {
        WIDGET_LOGE("ENGINE_THREAD not ready in CreateTextureLayer");
    }
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
        if (!textureLayer_) {
            WIDGET_LOGE("textureLayer is null in OnWindowChange");
            return META_NS::IAny::Ptr {};
        }
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
                {}, // instance is not needed for eglCreateSurface or vkCreateSurfaceOHOS
            }
        };
        swapchainHandle_ = device.CreateSwapchainHandle(swapchainCreateInfo, swapchainHandle_, {});

        auto &obr = META_NS::GetObjectRegistry();
        auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        bitmap_ = obr.Create<SCENE_NS::IRenderTarget>(SCENE_NS::ClassId::Bitmap, doc);
        if (auto i = interface_cast<SCENE_NS::IRenderResource>(bitmap_)) {
            i->SetRenderHandle(swapchainHandle_);
        }

        if (auto scene = interface_pointer_cast<SCENE_NS::IScene>(sceneWidgetObj_)) {
            auto cams = scene->GetCameras().GetResult();
            if (!cams.empty()) {
                for (auto c : cams) {
                    AttachSwapchain(interface_pointer_cast<META_NS::IObject>(c));
                }
            }
        }
        return META_NS::IAny::Ptr {};
    });

    if (engineThread) {
        engineThread->AddWaitableTask(cb)->Wait();
        onWindowChanged_ = true;
    } else {
        WIDGET_LOGE("ENGINE_THREAD not ready in OnWindowChange");
    }
}

void SceneAdapter::CreateRenderFunction()
{
    propSyncSync_ = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([this]() {
        PropSync();
        return META_NS::IAny::Ptr {};
    });
    // Task used for oneshot renders
    singleFrameAsync_ = META_NS::MakeCallback<META_NS::ITaskQueueTask>([this]() {
        RenderFunction();
        return 0;
    });
    // Task used for oneshot synchronous renders
    singleFrameSync_ = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([this]() {
        RenderFunction();
        return META_NS::IAny::Ptr {};
    });
}

void SceneAdapter::DeinitRenderThread()
{
    RETURN_IF_NULL(engineThread);
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
        engineInstance_.applicationContext_.reset();
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

void SceneAdapter::PropSync()
{
    auto scene = interface_pointer_cast<SCENE_NS::IScene>(sceneWidgetObj_);
    if (!scene) {
        return;
    }
    auto internal = scene->GetInternalScene();
    if (!internal) {
        return;
    }
    internal->SyncProperties();
}

void SceneAdapter::RenderFunction()
{
    WIDGET_SCOPED_TRACE("SceneAdapter::RenderFunction");
    auto rc = engineInstance_.renderContext_;
    RETURN_IF_NULL(rc);
    RETURN_IF_NULL(sceneWidgetObj_);
    auto scene = interface_pointer_cast<SCENE_NS::IScene>(sceneWidgetObj_);
    RETURN_IF_NULL(scene);
    BASE_NS::vector<SCENE_NS::ICamera::Ptr> disabledCameras;
    auto cams = scene->GetCameras().GetResult();
    for (auto c : cams) {
        if (!bitmap_ && c->IsActive()) {
            c->SetActive(false);
            disabledCameras.push_back(c);
        }

        AttachSwapchain(interface_pointer_cast<META_NS::IObject>(c));
    }

    scene->GetInternalScene()->Update(false);
    bool receiveBuffer = receiveBuffer_;
    receiveBuffer_ = false;

    auto& renderFrameUtil = rc->GetRenderUtil().GetRenderFrameUtil();
    auto backendType = rc->GetDevice().GetBackendType();
    if (receiveBuffer) {
        if (sfBufferInfo_.acquireFence_) {
            int32_t ret = sfBufferInfo_.acquireFence_->Wait(TIME_OUT_MILLISECONDS);
            if (sfBufferInfo_.acquireFence_->Get() >= 0 && ret < 0) {
                WIDGET_LOGE("wait fence error: %d", ret);
            }
            sfBufferInfo_.acquireFence_ = nullptr;
        }
        if (backendType == RENDER_NS::DeviceBackendType::VULKAN) {
            RENDER_NS::IRenderFrameUtil::SignalData signalData;
            signalData.signaled = false;
            signalData.signalResourceType = RENDER_NS::IRenderFrameUtil::SignalResourceType::GPU_SEMAPHORE;
            signalData.handle = bufferFenceHandle_;
            renderFrameUtil.AddGpuSignal(signalData);
        } else if (backendType == RENDER_NS::DeviceBackendType::OPENGLES) {
            RENDER_NS::IRenderFrameUtil::SignalData signalData;
            signalData.signaled = false;
            signalData.signalResourceType = RENDER_NS::IRenderFrameUtil::SignalResourceType::GPU_FENCE;
            signalData.handle = bufferFenceHandle_;
            renderFrameUtil.AddGpuSignal(signalData);
        }
    }
    rc->GetRenderer().RenderDeferredFrame();

    for (auto& camera : disabledCameras) {
        camera->SetActive(true);
    }

    if (receiveBuffer) {
        auto addedSD = renderFrameUtil.GetFrameGpuSignalData();
        for (const auto& ref : addedSD) {
            if (ref.handle.GetHandle().id == bufferFenceHandle_.GetHandle().id) {
                int32_t fenceFileDesc = CreateFenceFD(ref, rc->GetDevice());
                sfBufferInfo_.fn_(fenceFileDesc);
                break;
            }
        }
    }
}

int32_t SceneAdapter::CreateFenceFD(
    const RENDER_NS::IRenderFrameUtil::SignalData &signalData, RENDER_NS::IDevice &device)
{
    int32_t fenceFileDesc = -1;
    auto backendType = device.GetBackendType();
    if (backendType == RENDER_NS::DeviceBackendType::VULKAN) {
        // NOTE: vulkan not implemented yet
        WIDGET_LOGD("vulkan fence");
    } else if (backendType == RENDER_NS::DeviceBackendType::OPENGLES) {
        const auto disp = static_cast<const RENDER_NS::DevicePlatformDataGLES &>(device.GetPlatformData()).display;
        EGLSyncKHR sync = reinterpret_cast<EGLSyncKHR>(static_cast<uintptr_t>(signalData.gpuSignalResourceHandle));
        if (sync == EGL_NO_SYNC_KHR) {
            WIDGET_LOGE("invalid EGLSync from signal data");
            return fenceFileDesc;
        }
        fenceFileDesc = eglDupNativeFenceFDANDROID(disp, sync);
        if (fenceFileDesc == -1) {
            WIDGET_LOGE("eglDupNativeFence fail");
        }
    } else {
        WIDGET_LOGE("no supported backend");
    }
    return fenceFileDesc;
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

    if (!singleFrameAsync_ || !singleFrameSync_ || !propSyncSync_) {
        CreateRenderFunction();
    }
    if (!onWindowChanged_) {
        WIDGET_LOGW("window has not changed");
        return;
    }
    if (propSyncSync_) {
        WIDGET_SCOPED_TRACE("SceneAdapter::propSyncSync_");
        engineThread->AddWaitableTask(propSyncSync_)->Wait();
    }

    if (!needsSyncPaint && singleFrameAsync_) {
        renderTask = engineThread->AddTask(singleFrameAsync_);
    } else if (singleFrameSync_) {
        engineThread->AddWaitableTask(singleFrameSync_)->Wait();
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
    RETURN_IF_NULL(engineThread);
    WIDGET_LOGI("SceneAdapter::Deinit");
    auto func = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([this]() {
        if (swapchainHandle_) {
            auto& device = engineInstance_.renderContext_->GetDevice();
            device.DestroySwapchain(swapchainHandle_);
        }
        swapchainHandle_ = {};
        if (bitmap_) {
            bitmap_.reset();
            auto scene = interface_pointer_cast<SCENE_NS::IScene>(sceneWidgetObj_);
            if (!scene) {
                return META_NS::IAny::Ptr {};
            }
        }
        return META_NS::IAny::Ptr {};
    });
    engineThread->AddWaitableTask(func)->Wait();

    sceneWidgetObj_.reset();
    singleFrameAsync_.reset();
    singleFrameSync_.reset();
    propSyncSync_.reset();
    needsRepaint_ = false;
    onWindowChanged_ = false;
}

void SceneAdapter::AttachSwapchain(META_NS::IObject::Ptr cameraObj)
{
    auto camera = interface_pointer_cast<SCENE_NS::ICamera>(cameraObj);
    if (!camera) {
        WIDGET_LOGE("cast cameraObj failed in AttachSwapchain.");
        return;
    }
    if (!bitmap_ || !camera->IsActive()) {
        camera->SetRenderTarget({});
        return;
    }
    camera->SetRenderTarget(bitmap_);
}

namespace {
RENDER_NS::RenderHandleReference CreateOESTextureHandle(CORE_NS::IEcs::Ptr ecs,
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext, const SurfaceBufferInfo &surfaceBufferInfo)
{
    if (!ecs || !renderContext) {
        WIDGET_LOGE("null ecs or renderContext");
        return {};
    }
    auto surfaceBuffer = surfaceBufferInfo.sfBuffer_;
    if (!surfaceBuffer) {
        WIDGET_LOGE("surface buffer null");
        return {};
    }
    auto nativeBuffer = surfaceBuffer->SurfaceBufferToNativeBuffer();
    if (!nativeBuffer) {
        WIDGET_LOGE("surface buffer to native buffer fail");
        return {};
    }

    std::shared_ptr<RENDER_NS::BackendSpecificImageDesc> data = nullptr;
    auto backendType = renderContext->GetDevice().GetBackendType();
    if (backendType == RENDER_NS::DeviceBackendType::VULKAN) {
        data = std::make_shared<RENDER_NS::ImageDescVk>();
        auto vkData = std::static_pointer_cast<RENDER_NS::ImageDescVk>(data);
        vkData->platformHwBuffer = reinterpret_cast<uintptr_t>(nativeBuffer);
    } else if (backendType == RENDER_NS::DeviceBackendType::OPENGLES) {
        data = std::make_shared<RENDER_NS::ImageDescGLES>();
        auto glesData = std::static_pointer_cast<RENDER_NS::ImageDescGLES>(data);
        glesData->type = GL_TEXTURE_EXTERNAL_OES;
        glesData->platformHwBuffer = reinterpret_cast<uintptr_t>(nativeBuffer);
    }

    if (!data) {
        WIDGET_LOGE("create ImageDesc based on backend fail!");
        return {};
    }

    auto &gpuResourceManager = renderContext->GetDevice().GetGpuResourceManager();
    auto handle = gpuResourceManager.CreateView("AR_CAMERA_PREVIEW", {}, *data);

    return handle;
}
}  // namespace

void SceneAdapter::InitEnvironmentResource(const uint32_t bufferSize)
{
    WIDGET_LOGI("Init resource for camera view");
    auto scene = interface_pointer_cast<SCENE_NS::IScene>(sceneWidgetObj_);
    RETURN_IF_NULL(scene);
    auto internalScene = scene->GetInternalScene();
    RETURN_IF_NULL(internalScene);
    auto ecs = internalScene->GetEcsContext().GetNativeEcs();
    RETURN_IF_NULL(ecs);
    auto renderContext = engineInstance_.renderContext_;
    RETURN_IF_NULL(renderContext);

    bufferFenceHandle_ = RENDER_NS::RenderHandleReference{{static_cast<uint64_t>(key_) + BUFFER_FENCE_HANDLE_BASE}, {}};

    auto envManager = CORE_NS::GetManager<CORE3D_NS::IEnvironmentComponentManager>(*ecs);
    RETURN_IF_NULL(envManager);
    const auto &shaderMgr = renderContext->GetDevice().GetShaderManager();
    auto &entityManager = (*ecs).GetEntityManager();
    auto renderHandleManager = CORE_NS::GetManager<CORE3D_NS::IRenderHandleComponentManager>(*ecs);
    RETURN_IF_NULL(renderHandleManager);
    auto uriManager = CORE_NS::GetManager<CORE3D_NS::IUriComponentManager>(*ecs);
    RETURN_IF_NULL(uriManager);

    CORE_NS::EntityReference shaderEF = entityManager.CreateReferenceCounted();
    renderHandleManager->Create(shaderEF);
    BASE_NS::string_view shaderUri = "campreviewshaders://shader/camera_stream.shader";
    renderContext->GetDevice().GetShaderManager().LoadShaderFile(shaderUri.data());
    if (auto renderEntityHandle = renderHandleManager->Write(shaderEF)) {
        renderEntityHandle->reference = shaderMgr.GetShaderHandle(shaderUri);
    }
    uriManager->Create(shaderEF);
    if (auto uriHandle = uriManager->Write(shaderEF)) {
        uriHandle->uri = shaderUri;
    }

    RENDER_NS::GpuBufferDesc envBufferDesc{
        RENDER_NS::CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT | RENDER_NS::CORE_BUFFER_USAGE_TRANSFER_DST_BIT,
        RENDER_NS::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RENDER_NS::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        RENDER_NS::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,
        bufferSize};
    BASE_NS::array_view<const uint8_t> transData(
        reinterpret_cast<const uint8_t *>(sfBufferInfo_.transformMatrix_.data()), bufferSize);
    camTransBufferHandle_ =
        renderContext->GetDevice().GetGpuResourceManager().Create(ENV_PROPERTIES_BUFFER_NAME, envBufferDesc, transData);
    CORE_NS::EntityReference transBufferEF = entityManager.CreateReferenceCounted();
    renderHandleManager->Create(transBufferEF);
    if (auto renderEntityHandle = renderHandleManager->Write(transBufferEF)) {
        renderEntityHandle->reference = camTransBufferHandle_;
    }

    CORE_NS::EntityReference transBufferEF2 = entityManager.CreateReferenceCounted();
    renderHandleManager->Create(transBufferEF2);
    if (auto renderEntityHandle = renderHandleManager->Write(transBufferEF2)) {
        renderEntityHandle->reference = camTransBufferHandle_;
    }

    CORE_NS::EntityReference transBufferEF3 = entityManager.CreateReferenceCounted();
    renderHandleManager->Create(transBufferEF3);
    if (auto renderEntityHandle = renderHandleManager->Write(transBufferEF3)) {
        renderEntityHandle->reference = camTransBufferHandle_;
    }

    camImageEF_ = entityManager.CreateReferenceCounted();
    renderHandleManager->Create(camImageEF_);

    if (auto rc = scene->RenderConfiguration()->GetValue()) {
        if (auto env = rc->Environment()->GetValue()) {
            CORE_NS::Entity environmentEntity = interface_pointer_cast<SCENE_NS::IEcsObjectAccess>(env)->
                GetEcsObject()->GetEntity();
            if (auto envDataHandle = envManager->Write(environmentEntity)) {
                CORE3D_NS::EnvironmentComponent &envComponent = *envDataHandle;
                envComponent.background = CORE3D_NS::EnvironmentComponent::Background::IMAGE;
                envComponent.shader = shaderEF;
                envComponent.customResources.push_back({camImageEF_});
                envComponent.customResources.push_back({transBufferEF});
                envComponent.customResources.push_back({transBufferEF2});
                envComponent.customResources.push_back({transBufferEF3});
            }
        }
    }
    if (!renderDataStoreDefaultStaging_) {
        renderDataStoreDefaultStaging_ = BASE_NS::refcnt_ptr<RENDER_NS::IRenderDataStoreDefaultStaging>(
            static_cast<RENDER_NS::IRenderDataStoreDefaultStaging *>(
            renderContext->GetRenderDataStoreManager().GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING).get()));
        RETURN_IF_NULL(renderDataStoreDefaultStaging_);
    }
}

void SceneAdapter::UpdateSurfaceBuffer()
{
    auto scene = interface_pointer_cast<SCENE_NS::IScene>(sceneWidgetObj_);
    RETURN_IF_NULL(scene);
    auto internalScene = scene->GetInternalScene();
    RETURN_IF_NULL(internalScene);
    auto ecs = internalScene->GetEcsContext().GetNativeEcs();
    RETURN_IF_NULL(ecs);
    auto renderContext = engineInstance_.renderContext_;
    RETURN_IF_NULL(renderContext);

    auto viewHandle = CreateOESTextureHandle(ecs, renderContext, sfBufferInfo_);
    RETURN_IF_NULL(viewHandle);

    const uint32_t bufferSize = sfBufferInfo_.transformMatrix_.size() * FLOAT_TO_BYTE;
    if (bufferSize < TRANSFORM_MATRIX_SIZE * FLOAT_TO_BYTE) {
        WIDGET_LOGE("transform matrix size:%d not 4x4", bufferSize / FLOAT_TO_BYTE);
        const float *arr = BASE_NS::Math::IDENTITY_4X4.data;
        sfBufferInfo_.transformMatrix_.assign(arr, arr + TRANSFORM_MATRIX_SIZE);
    } else if (bufferSize > TRANSFORM_MATRIX_SIZE * FLOAT_TO_BYTE) {
        WIDGET_LOGE("transform matrix size:%d not 4x4", bufferSize / FLOAT_TO_BYTE);
    }

    if (!initCamRNG_) {
        InitEnvironmentResource(bufferSize);
        initCamRNG_ = true;
    }

    if (auto renderHandleManager = CORE_NS::GetManager<CORE3D_NS::IRenderHandleComponentManager>(*ecs)) {
        if (auto camImageRenderHandle = renderHandleManager->Write(camImageEF_)) {
            camImageRenderHandle->reference = viewHandle;
        }
    }

    if (camTransBufferHandle_ && renderDataStoreDefaultStaging_) {
        BASE_NS::array_view<const uint8_t> transData(
            reinterpret_cast<const uint8_t *>(sfBufferInfo_.transformMatrix_.data()), bufferSize);
        const RENDER_NS::BufferCopy bufferCopy{0, 0, bufferSize};
        renderDataStoreDefaultStaging_->CopyDataToBufferOnCpu(transData, camTransBufferHandle_, bufferCopy);
    }
}

void SceneAdapter::AcquireImage(const SurfaceBufferInfo &bufferInfo)
{
    RETURN_IF_NULL(engineThread);
    engineThread->AddTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>([this, bufferInfo]() {
        sfBufferInfo_ = bufferInfo;
        receiveBuffer_ = true;
        UpdateSurfaceBuffer();
        return false;
    }));
}
}  // namespace OHOS::Render3D
