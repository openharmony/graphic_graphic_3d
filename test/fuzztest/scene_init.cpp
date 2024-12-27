/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "scene_init.h"
#include "scene_adapter/scene_adapter.h"

#include "3d_widget_adapter_log.h"

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
#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>

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

#include <scene_plugin/namespace.h>
#include <scene_plugin/interface/intf_scene.h>
#include <scene_plugin/interface/intf_ecs_scene.h>
#include <scene_plugin/interface/intf_mesh.h>
#include <scene_plugin/interface/intf_material.h>
#include <scene_plugin/api/scene_uid.h>

#include "napi_base_context.h"

#include <render/implementation_uids.h>
#include <render/gles/intf_device_gles.h>
#include <render/intf_renderer.h>
#include <render/intf_render_context.h>

#include "3d_widget_adapter_log.h"
#include "widget_trace.h"
#include "ohos/texture_layer.h"

CORE_BEGIN_NAMESPACE()
/** Get plugin register */
IPluginRegister &(*GetPluginRegister)() = nullptr;

/** Setup the plugin register */
void (*CreatePluginRegistry)(const struct PlatformCreateInfo &platformCreateInfo) = nullptr;

/** Get whether engine is build in debug mode */
bool (*IsDebugBuild)() = nullptr;

/** Get version */
BASE_NS::string_view (*GetVersion)() = nullptr;
CORE_END_NAMESPACE()

namespace OHOS::FUZZ {

// class SceneInit : public ISceneInit {
class SceneInit : public ISceneInit {
public:
    SceneInit();
    
    bool LoadPluginsAndInit() override;
    
    std::shared_ptr<Render3D::TextureLayer> CreateTextureLayer() override;

    void OnWindowChange(const Render3D::WindowChangeInfo &windowChangeInfo) override;

    void RenderFrame(bool needsSyncPaint = false) override;

    EngineInstance& GetEngineInstance() override;

    void Deinit() override;

    bool NeedsRepaint() override
    {
        return false;
    }

    static void ShutdownPluginRegistry();
    static void DeinitRenderThread();

    ~SceneInit();

private:
    static bool LoadEngineLib();
    bool LoadPlugins(const CORE_NS::PlatformCreateInfo &platformCreateInfo);
    bool InitEngine(CORE_NS::PlatformCreateInfo platformCreateInfo);
    void AttachSwapchain(META_NS::IObject::Ptr camera, RENDER_NS::RenderHandleReference swapchain);
    void RenderFunction();
    void CreateRenderFunction();
    std::shared_ptr<ISceneAdapter> scene_;
};

Render3D::HapInfo GetHapInfo()
{
    return {};
}

using IntfPtr = META_NS::SharedPtrIInterface;
using IntfWeakPtr = META_NS::WeakPtrIInterface;

static EngineInstance engineInstance_;
static std::mutex mute;
static Render3D::HapInfo hapInfo_;
META_NS::ITaskQueue::Ptr engineThread;
META_NS::ITaskQueue::Ptr ioThread;
META_NS::ITaskQueue::Ptr releaseThread;
META_NS::ITaskQueue::Token renderTask{};

EngineInstance& SceneInit::GetEngineInstance() {
    return engineInstance_;
}

void LockCompositor()
{
    mute.lock();
}

void UnlockCompositor()
{
    mute.unlock();
}

static constexpr BASE_NS::Uid ENGINE_THREAD{"2070e705-d061-40e4-bfb7-90fad2c280af"};
static constexpr BASE_NS::Uid APP_THREAD{"b2e8cef3-453a-4651-b564-5190f8b5190d"};
static constexpr BASE_NS::Uid IO_QUEUE{"be88e9a0-9cd8-45ab-be48-937953dc258f"};
static constexpr BASE_NS::Uid JS_RELEASE_THREAD{"3784fa96-b25b-4e9c-bbf1-e897d36f73af"};

template <typename T>
bool LoadFunc(T &fn, const char *fName, void *handle)
{
    fn = reinterpret_cast<T>(dlsym(handle, fName));
    if (fn == nullptr) {
        WIDGET_LOGE("%s open %s", __func__, dlerror());
        return false;
    }
    return true;
}

SceneInit::SceneInit()
{
    WIDGET_LOGD("scene adapter Impl create");
}

bool SceneInit::LoadEngineLib()
{
    if (engineInstance_.libHandle_ != nullptr) {
        WIDGET_LOGD("%s, already loaded", __func__);
        return true;
    }

#define TO_STRING(name) #name
#define LIB_NAME(name) TO_STRING(name)
    constexpr std::string_view lib{LIB_NAME(LIB_ENGINE_CORE) ".so"};
    engineInstance_.libHandle_ = dlopen(lib.data(), RTLD_LAZY);

    if (engineInstance_.libHandle_ == nullptr) {
        WIDGET_LOGE("%s, open lib fail %s", __func__, dlerror());
    }
#undef TO_STRING
#undef LIB_NAME

#define LOAD_FUNC(fn, name) LoadFunc<decltype(fn)>(fn, name, engineInstance_.libHandle_)
    if (!(LOAD_FUNC(CORE_NS::CreatePluginRegistry, "_ZN4Core20CreatePluginRegistryERKNS_18PlatformCreateInfoE") &&
            LOAD_FUNC(CORE_NS::GetPluginRegister, "_ZN4Core17GetPluginRegisterEv") &&
            LOAD_FUNC(CORE_NS::IsDebugBuild, "_ZN4Core12IsDebugBuildEv") &&
            LOAD_FUNC(CORE_NS::GetVersion, "_ZN4Core13GetVersionRevEv"))) {
        return false;
    }
#undef LOAD_FUNC

    return true;
}

bool SceneInit::LoadPlugins(const CORE_NS::PlatformCreateInfo &platformCreateInfo)
{
    if (engineInstance_.libsLoaded_) {
        return true;
    }
    if (!LoadEngineLib()) {
        return false;
    }
    engineInstance_.libsLoaded_ = true;
    WIDGET_LOGD("load engine success!");

    return true;
}

bool SceneInit::InitEngine(CORE_NS::PlatformCreateInfo platformCreateInfo)
{
#ifdef SCENE_META_FUZZ
    auto &tr = META_NS::GetTaskQueueRegistry();
    auto &obr = META_NS::GetObjectRegistry();

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

    auto engineInit = META_NS::MakeCallback<META_NS::ITaskQueueTask>([platformCreateInfo]() {
#endif
    // Initialize lumeengine/render etc
#ifdef SCENE_META_FUZZ
        const BASE_NS::Uid DefaultPluginList[] { SCENE_NS::UID_SCENE_PLUGIN };
#else
        const BASE_NS::Uid DefaultPluginList[] { RENDER_NS::UID_RENDER_PLUGIN, CORE3D_NS::UID_3D_PLUGIN };
#endif
        CORE_NS::CreatePluginRegistry(platformCreateInfo);
        if (!CORE_NS::GetPluginRegister().LoadPlugins(DefaultPluginList)) {
            WIDGET_LOGE("fail to load scene widget plugin");
            return false;
        }
        WIDGET_LOGI("load plugin success");
        CORE_NS::EngineCreateInfo engineCreateInfo{platformCreateInfo, {}, {}};
        if (auto factory = CORE_NS::GetInstance<CORE_NS::IEngineFactory>(CORE_NS::UID_ENGINE_FACTORY)) {
            engineInstance_.engine_.reset(factory->Create(engineCreateInfo).get());
        }

        if (!engineInstance_.engine_) {
            WIDGET_LOGE("get engine fail");
            return false;
        }
        engineInstance_.engine_->Init();

        engineInstance_.renderContext_.reset(
            CORE_NS::CreateInstance<RENDER_NS::IRenderContext>(*engineInstance_.engine_, RENDER_NS::UID_RENDER_CONTEXT)
                .get());
        if (!engineInstance_.renderContext_) {
            WIDGET_LOGE("get render context fail");
            return false;
        }

        RENDER_NS::RenderCreateInfo renderCreateInfo;
        RENDER_NS::BackendExtraGLES glExtra;
        Render::DeviceCreateInfo deviceCreateInfo;
        // this context needs to be "not busy" (not selected current in any thread) for creation to succeed.
        // vulkan currently has no bloom
        std::string backendProp =
            "gles";  // Render3D::RenderConfig::GetInstance().renderBackend_ == "force_vulkan" ? "vulkan" : "gles";
        if (backendProp == "vulkan") {
        } else {
            glExtra.depthBits = 24; // 24 :bits size
            glExtra.sharedContext = EGL_NO_CONTEXT;
            deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::OPENGLES;
            deviceCreateInfo.backendConfiguration = &glExtra;
            renderCreateInfo.applicationInfo = {};
            renderCreateInfo.deviceCreateInfo = deviceCreateInfo;
        }

        auto rrc = engineInstance_.renderContext_->Init(renderCreateInfo);
        if (rrc != RENDER_NS::RenderResultCode::RENDER_SUCCESS) {
            WIDGET_LOGE("Failed to create render context");
            return false;
        }

        engineInstance_.graphicsContext_.reset(CORE_NS::CreateInstance<CORE3D_NS::IGraphicsContext>(
            *engineInstance_.renderContext_->GetInterface<CORE_NS::IClassFactory>(), CORE3D_NS::UID_GRAPHICS_CONTEXT)
                .get());
        if (!engineInstance_.graphicsContext_) {
            WIDGET_LOGE("create 3D context fail");
            return false;
        }
        engineInstance_.graphicsContext_->Init();

        WIDGET_LOGD("regester shader path");
        static constexpr const RENDER_NS::IShaderManager::ShaderFilePathDesc desc{"shaders://"};
        engineInstance_.engine_->GetFileManager().RegisterPath("shaders", "OhosRawFile://shaders", false);
        engineInstance_.renderContext_->GetDevice().GetShaderManager().LoadShaderFiles(desc);

        engineInstance_.engine_->GetFileManager().RegisterPath("appshaders", "OhosRawFile://shaders", false);
        static constexpr const RENDER_NS::IShaderManager::ShaderFilePathDesc desc1{"appshaders://"};
        engineInstance_.renderContext_->GetDevice().GetShaderManager().LoadShaderFiles(desc1);

        WIDGET_LOGI("init engine success");

#ifdef SCENE_META_FUZZ
        auto &obr = META_NS::GetObjectRegistry();
        // scene/meta
        // Save the stuff to the default object context.
        auto engineThread = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
        // // this is the javascript thread...
        auto appThread = engineThread;
        auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        if (doc == nullptr) {
            WIDGET_LOGE("nullptr from interface_cast");
            return false;
        }

        auto flags = META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE;

        doc->AddProperty(META_NS::ConstructProperty<IntfPtr>("RenderContext", nullptr, flags));
        doc->AddProperty(META_NS::ConstructProperty<IntfPtr>("EngineQueue", nullptr, flags));
        doc->AddProperty(META_NS::ConstructProperty<IntfPtr>("AppQueue", nullptr, flags));
        doc->AddProperty(META_NS::ConstructArrayProperty<IntfWeakPtr>("Scenes", {}, flags));

        doc->GetPropertyByName<META_NS::SharedPtrIInterface>("EngineQueue")->SetValue(engineThread);
        doc->GetPropertyByName<META_NS::SharedPtrIInterface>("AppQueue")->SetValue(appThread);
        doc->GetPropertyByName<META_NS::SharedPtrIInterface>("RenderContext")->SetValue(engineInstance_.renderContext_);

        return false;
    });

    engineThread->AddTask(engineInit);
#endif
    return true;
}

void SceneInit::Deinit()
{
    engineInstance_.graphicsContext_.reset();

    engineInstance_.renderContext_->GetRenderer().RenderFrame({});

    engineInstance_.renderContext_->GetDevice().WaitForIdle();

    engineInstance_.renderContext_.reset();

#if SCENE_META_FUZZ
    const BASE_NS::Uid DefaultPluginList[]{SCENE_NS::UID_SCENE_PLUGIN};
#else
    const BASE_NS::Uid DefaultPluginList[]{RENDER_NS::UID_RENDER_PLUGIN, CORE3D_NS::UID_3D_PLUGIN};
#endif
    CORE_NS::GetPluginRegister().UnloadPlugins(DefaultPluginList);
}

SceneInit::~SceneInit()
{
    Deinit();
}

void SceneInit::DeinitRenderThread()
{
#if SCENE_META_FUZZ
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
            return META_NS::IAny::Ptr{};
        }
        // this is somewhat uncool
        {
            auto p3 = doc->GetPropertyByName<IntfPtr>("RenderContext");
            doc->RemoveProperty(p3);
            auto p2 = doc->GetPropertyByName<IntfPtr>("AppQueue");
            doc->RemoveProperty(p2);
            auto p1 = doc->GetPropertyByName<IntfPtr>("EngineQueue");
            doc->RemoveProperty(p1);
        }

        doc->GetArrayPropertyByName<IntfWeakPtr>("Scenes")->Reset();

        return META_NS::IAny::Ptr{};
    });
    auto dummy = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([]() { return META_NS::IAny::Ptr{}; });
    auto &tr = META_NS::GetTaskQueueRegistry();
    if (releaseThread) {
        releaseThread->AddWaitableTask(dummy)->Wait();
        tr.UnregisterTaskQueue(JS_RELEASE_THREAD);
        releaseThread.reset();
    }

    if (ioThread) {
        ioThread->AddWaitableTask(dummy)->Wait();
        tr.UnregisterTaskQueue(IO_QUEUE);
        ioThread.reset();
    }

    if (engineThread) {
        engineThread->AddWaitableTask(engine_deinit)->Wait();

        tr.UnregisterTaskQueue(ENGINE_THREAD);
        engineThread.reset();
    }
#endif
    CORE_LOG_E("unload E");
#if SCENE_META_FUZZ
    const BASE_NS::Uid DefaultPluginList[]{SCENE_NS::UID_SCENE_PLUGIN};
#else
    const BASE_NS::Uid DefaultPluginList[]{RENDER_NS::UID_RENDER_PLUGIN, CORE3D_NS::UID_3D_PLUGIN};
#endif
    CORE_NS::GetPluginRegister().UnloadPlugins(DefaultPluginList);
    CORE_LOG_E("unload X");
    engineInstance_.renderContext_.reset();
    engineInstance_.engine_.reset();
}

std::shared_ptr<Render3D::TextureLayer> SceneInit::CreateTextureLayer()
{
    return {};
}

bool SceneInit::LoadPluginsAndInit()
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
        PLATFORM_PATH_NAME(PLATFORM_APP_ROOT_PATH),
        PLATFORM_PATH_NAME(PLATFORM_APP_PLUGIN_PATH),
        hapInfo_.hapPath_.c_str(),
        hapInfo_.bundleName_.c_str(),
        hapInfo_.moduleName_.c_str()
    };
#undef TO_STRING
#undef PLATFORM_PATH_NAME
    if (!LoadPlugins(platformCreateInfo)) {
        WIDGET_LOGE("LoadPlugins failed");
        UnlockCompositor();
        return false;
    }

    if (!InitEngine(platformCreateInfo)) {
        WIDGET_LOGE("InitEngine failed");
        UnlockCompositor();
        return false;
    }

    CreateRenderFunction();
    UnlockCompositor();
    return true;
}

void SceneInit::OnWindowChange(const Render3D::WindowChangeInfo &windowChangeInfo)
{}

void SceneInit::CreateRenderFunction()
{}

void SceneInit::ShutdownPluginRegistry()
{
    if (engineInstance_.libHandle_ == nullptr) {
        return ;
    }

    CORE_NS::GetPluginRegister = nullptr;
    CORE_NS::CreatePluginRegistry = nullptr;
    CORE_NS::IsDebugBuild = nullptr;
    CORE_NS::GetVersion = nullptr;
}

void SceneInit::RenderFunction()
{}

void SceneInit::RenderFrame(bool needsSyncPaint)
{}

std::shared_ptr<ISceneInit> CreateFuzzScene()
{
    return std::make_shared<SceneInit>();
}
} // namespace OHOS::FUZZ