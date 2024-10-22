/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "test_runner.h"

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

#include "ability.h"
#include "data_ability_helper.h"
#include "napi_base_context.h"

#include <render/implementation_uids.h>
#include <render/gles/intf_device_gles.h>
#include <render/intf_renderer.h>
#include <render/intf_render_context.h>

#include "3d_widget_adapter_log.h"
#include "ohos/texture_layer.h"

CORE_BEGIN_NAMESPACE()
IPluginRegister& (*GetPluginRegister) () = nullptr;

void (*CreatePluginRegistry) (
    const struct PlatformCreateInfo& platformCreateInfo) = nullptr;

bool (*IsDebugBuild) () = nullptr;

BASE_NS::string_view (*GetVersion) () = nullptr;
CORE_END_NAMESPACE()

META_BEGIN_NAMESPACE()
OHOS::Render3D::HapInfo GetHapInfo()
{
    std::shared_ptr<OHOS::AbilityRuntime::ApplicationContext> context =
        OHOS::AbilityRuntime::ApplicationContext::GetApplicationContext();
    if (!context) {
        WIDGET_LOGE("Failed to get application context.");
        return {};
    }
    auto resourceManager = context->GetResourceManager();
    if (!resourceManager) {
        WIDGET_LOGE("Failed to get resource manager.");
        return {};
    }
    OHOS::Render3D::HapInfo hapInfo;
    hapInfo.bundleName_ = resourceManager->bundleInfo.first;
    hapInfo.moduleName_ = resourceManager->bundleInfo.second;

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
    void *libHandle = nullptr;
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext;
    BASE_NS::shared_ptr<CORE_NS::IEngine> engine;
};

static EngineInstance g_engineInstance;

META_NS::ITaskQueue::Ptr engineThread;
META_NS::ITaskQueue::Ptr ioThread;

static constexpr BASE_NS::Uid ENGINE_THREAD { "2070e705-d061-40e4-bfb7-90fad2c280af" };
static constexpr BASE_NS::Uid APP_THREAD { "b2e8cef3-453a-4651-b564-5190f8b5190d" };
static constexpr BASE_NS::Uid IO_QUEUE { "be88e9a0-9cd8-45ab-be48-937953dc258f" };

OHOS::Render3D::HapInfo hapInfo;

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


bool LoadEngineLib()
{
    if (g_engineInstance.libHandle != nullptr) {
        WIDGET_LOGD("%s, already loaded", __func__);
        return true;
    }

    #define TO_STRING(name) #name
    #define LIB_NAME(name) TO_STRING(name)
    constexpr std::string_view lib { "/system/lib/libAGPDLL.z.so" };
    g_engineInstance.libHandle = dlopen(lib.data(), RTLD_LAZY);

    if (g_engineInstance.libHandle == nullptr) {
        WIDGET_LOGE("%s, open lib fail %s", __func__, dlerror());
        return false;
    }
    #undef TO_STRING
    #undef LIB_NAME

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

bool LoadPlugins(const CORE_NS::PlatformCreateInfo& platformCreateInfo)
{
    WIDGET_LOGD("LoadPlugins Begin lgp");
    if (g_engineInstance.engine) {
        return true;
    }
    if (!LoadEngineLib()) {
        return false;
    }
    WIDGET_LOGD("load engine success!");
    const BASE_NS::Uid DefaultPluginList[] { SCENE_NS::UID_SCENE_PLUGIN };

    CORE_NS::CreatePluginRegistry(platformCreateInfo);
    if (!CORE_NS::GetPluginRegister().LoadPlugins(DefaultPluginList)) {
        WIDGET_LOGE("fail to load scene widget plugin");
        return false;
    }
    WIDGET_LOGD("load plugin success");
    return true;
}

bool InitEngine(CORE_NS::PlatformCreateInfo platformCreateInfo)
{
    if (g_engineInstance.engine) {
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
    return true;
}

bool LoadPluginsAndInit()
{
    WIDGET_LOGD("scene adapter loadPlugins");
    hapInfo = GetHapInfo();

    #define TO_STRING(name) #name
    #define PLATFORM_PATH_NAME(name) TO_STRING(name)
    CORE_NS::PlatformCreateInfo platformCreateInfo {
        PLATFORM_PATH_NAME(PLATFORM_CORE_ROOT_PATH),
        PLATFORM_PATH_NAME(PLATFORM_APP_ROOT_PATH),
        PLATFORM_PATH_NAME(PLATFORM_APP_PLUGIN_PATH),
        hapInfo.hapPath_.c_str(),
        hapInfo.bundleName_.c_str(),
        hapInfo.moduleName_.c_str()
    };
    #undef TO_STRING
    #undef PLATFORM_PATH_NAME
    if (!LoadPlugins(platformCreateInfo)) {
        return false;
    }

    if (!InitEngine(platformCreateInfo)) {
        return false;
    }

    return true;
}

void SetTest()
{
    LoadPluginsAndInit();
    RegisterTestTypes();
}

void ResetTest()
{
    g_engineInstance.engine.reset();
}
META_END_NAMESPACE()

