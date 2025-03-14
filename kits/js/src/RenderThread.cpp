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

#include <base/containers/unordered_map.h>
#include <meta/api/make_callback.h>
#include <meta/base/shared_ptr.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/property/construct_array_property.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_camera.h>
#include <scene/ext/intf_internal_scene.h>

#include <core/engine_info.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_filesystem_api.h>
#include <core/os/intf_platform.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>

#include "napi_api.h"

void CreateDevice(BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext);
uintptr_t OsGetCurrentThreadId();
NapiApi::Function GetJSConstructor(napi_env env, const BASE_NS::string_view jsName);
napi_value lumeInit(napi_env env, napi_callback_info info);

using IntfPtr = META_NS::SharedPtrIInterface;
using IntfWeakPtr = META_NS::WeakPtrIInterface;

static constexpr BASE_NS::Uid APP_THREAD_DEP { "b2e8cef3-453a-4651-b564-5190f8b5190d" };
static constexpr BASE_NS::Uid IO_QUEUE { "be88e9a0-9cd8-45ab-be48-937953dc258f" };
static constexpr BASE_NS::Uid ENGINE_THREAD_DEP { "2070e705-d061-40e4-bfb7-90fad2c280af" };
static constexpr BASE_NS::Uid JS_RELEASE_THREAD { "3784fa96-b25b-4e9c-bbf1-e897d36f73af" };

BASE_NS::shared_ptr<CORE_NS::IEngine> engine;
BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext_;
META_NS::ITaskQueue::Ptr engineThread;
META_NS::ITaskQueue::Ptr ioThread;
META_NS::ITaskQueue::Ptr releaseThread;
META_NS::ITaskQueue::Token renderTask {};
META_NS::ITaskQueueTask::Ptr singleFrameAsync;
META_NS::ITaskQueueTask::Ptr continuousFrameAsync;
META_NS::ITaskQueueWaitableTask::Ptr singleFrameSync;
extern void UpdateSwapState(const SCENE_NS::IScene::Ptr&, SCENE_NS::ICamera::Ptr);
extern void PresentSwap(SCENE_NS::ICamera::Ptr);
extern void DestroySwap(SCENE_NS::ICamera::Ptr);
extern void DestroySwaps();

extern void FrameDone();

extern void LockCompositor();
extern void UnlockCompositor();

namespace Internal {
BASE_NS::string GetCWD();
};
META_NS::IAny::Ptr InitFunction(CORE_NS::PlatformCreateInfo platformCreateInfo)
{
    using namespace RENDER_NS;
#if 1
    CORE_LOG_I("Engine thread: %p\n", (void*)OsGetCurrentThreadId());
#endif

    auto& obr = META_NS::GetObjectRegistry();
    // Initialize lumeengine/render etc
    CORE_NS::EngineCreateInfo engineCreateInfo { platformCreateInfo, {}, {} };
    if (auto factory = CORE_NS::GetInstance<CORE_NS::IEngineFactory>(CORE_NS::UID_ENGINE_FACTORY)) {
        engine.reset(factory->Create(engineCreateInfo).get());
    }
    if (!engine) {
        return META_NS::IAny::Ptr {};
    }
#if WIN32
    auto fsa = CORE_NS::GetInstance<CORE_NS::IFileSystemApi>(CORE_NS::UID_FILESYSTEM_API_FACTORY);
    // create a fs ...
    auto& fm = engine->GetFileManager();
    auto curdir = Internal::GetCWD();
    auto fs = fsa->CreateStdFileSystem("file:///" + curdir + "/assets/");
    // and make its name "raw" (to kinda match ohos side)
    fm.RegisterFilesystem("OhosRawFile", BASE_NS::move(fs));
#elif OHOS
    auto& fm = engine->GetFileManager();
    auto& plat = engine->GetPlatform();
    // could register cache etc here.. but not now..
    plat.RegisterRawFilesystem(fm, "OhosRawFile");
#endif
    fm.RegisterPath("appshaders", "OhosRawFile:///shaders/", true);
    fm.RegisterPath("apppipelinelayouts", "OhosRawFile:///pipelinelayouts/", true);
    fm.RegisterPath("fonts", "OhosRawFile:///fonts", true);
    engine->Init();

    renderContext_.reset(static_cast<RENDER_NS::IRenderContext*>(
        engine->GetInterface<CORE_NS::IClassFactory>()->CreateInstance(RENDER_NS::UID_RENDER_CONTEXT).release()));
    CreateDevice(renderContext_);

    // Save the stuff to the default object context.
    auto engineThread = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD_DEP);
    /*force sceneplugin to use engine thread as app thread. simplifies synchronization.*/
    auto appThread = engineThread;
    auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());

    auto flags = META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE;
    doc->AddProperty(META_NS::ConstructProperty<IntfPtr>("RenderContext", nullptr, flags));
    doc->AddProperty(META_NS::ConstructProperty<IntfPtr>("EngineQueue", nullptr, flags));
    doc->AddProperty(META_NS::ConstructProperty<IntfPtr>("AppQueue", nullptr, flags));
    doc->AddProperty(META_NS::ConstructArrayProperty<IntfWeakPtr>("Scenes", {}, flags));

    // interesting.. refcount of the objects (the intfptrs) increases by TWO?
    doc->GetProperty<IntfPtr>("EngineQueue")->SetValue(engineThread);
    doc->GetProperty<IntfPtr>("AppQueue")->SetValue(appThread);
    doc->GetProperty<IntfPtr>("RenderContext")->SetValue(renderContext_);

    // "render" one frame. this is to initialize all the default resources etc.
    // if we never render a single frame, we will get "false positive" leaks of gpu resources.
    renderContext_->GetRenderer().RenderFrame({});
    return META_NS::IAny::Ptr {};
}

void RenderFrame()
{
    auto tid = OsGetCurrentThreadId();
    // main ticker that renders all active scenes.. (single engine way)
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> rc;
    if (auto prp = doc->GetProperty<IntfPtr>("RenderContext")) {
        rc = interface_pointer_cast<RENDER_NS::IRenderContext>(prp->GetValue());
    }

    BASE_NS::unordered_map<uintptr_t, BASE_NS::vector<SCENE_NS::ICamera::Ptr>> results;
    META_NS::ArrayProperty<IntfWeakPtr> scenes = doc->GetProperty("Scenes");
    if (scenes->GetSize() == 0) {
        // No scenes..
        FrameDone();
        return;
    }
    LockCompositor();
    for (auto i = 0; i < scenes->GetSize(); ++i) {
        auto t = scenes->GetValueAt(i);
        if (auto scene = interface_pointer_cast<SCENE_NS::IScene>(t)) {
            auto internal = scene->GetInternalScene();
            if (internal->GetRenderMode() != SCENE_NS::RenderMode::MANUAL || internal->HasPendingRender()) {
                auto cams = scene->GetCameras().GetResult();
                for (auto it = cams.begin(); it != cams.end();) {
                    UpdateSwapState(scene, *it); // deactivates cameras if needed.
                    if (!(*it)->IsActive()) {
                        it = cams.erase(it);
                    } else {
                        ++it;
                    }
                }
                if (!cams.empty()) {
                    results[(uintptr_t)scene.get()] = cams;
                }
            }
            scene->GetInternalScene()->Update();
        } else {
            scenes->RemoveAt(i);
        }
    }

    rc->GetRenderer().RenderDeferredFrame();

    // flush all deferred scenes..
    if (!results.empty()) {
        for (const auto& t : results) {
            auto scene = (SCENE_NS::IScene*)t.first;
            auto updatedCams = t.second;
            auto cams = scene->GetCameras().GetResult();
            for (const auto& e : updatedCams) {
                for (const auto& c : cams) {
                    if (e == c) {
                        // "present" the texture for the camera.
                        // (may be nop, depends on if swapchain target or texture targets)
                        PresentSwap(c);
                    }
                }
            }
        }
    }
    UnlockCompositor();
    FrameDone();
}

void InitRenderThread(CORE_NS::PlatformCreateInfo& platformCreateInfo)
{
    CORE_NS::GetLogger()->SetLogLevel(CORE_NS::ILogger::LogLevel::LOG_INFO);

    auto& tr = META_NS::GetTaskQueueRegistry();
    engineThread = tr.GetTaskQueue(ENGINE_THREAD_DEP);
    // create engine thread
    if (!engineThread) {
        engineThread = META_NS::GetObjectRegistry().Create<META_NS::ITaskQueue>(META_NS::ClassId::ThreadedTaskQueue);
        tr.RegisterTaskQueue(engineThread, ENGINE_THREAD_DEP);
    }
    // create io thread
    ioThread = tr.GetTaskQueue(IO_QUEUE);
    if (!ioThread) {
        ioThread = META_NS::GetObjectRegistry().Create<META_NS::ITaskQueue>(META_NS::ClassId::ThreadedTaskQueue);
        tr.RegisterTaskQueue(ioThread, IO_QUEUE);
    }
    // create a "js release thread" (workaround for the possible napi_release_threadsafe_function)
    releaseThread = META_NS::GetTaskQueueRegistry().GetTaskQueue(JS_RELEASE_THREAD);
    if (!releaseThread) {
        auto& obr = META_NS::GetObjectRegistry();
        releaseThread = obr.Create<META_NS::ITaskQueue>(META_NS::ClassId::ThreadedTaskQueue);
        tr.RegisterTaskQueue(releaseThread, JS_RELEASE_THREAD);
    }
    // Task used for oneshot renders
    singleFrameAsync = META_NS::MakeCallback<META_NS::ITaskQueueTask>([]() {
        RenderFrame();
        return 0;
    });
    // Task used for oneshot synchronous renders
    singleFrameSync = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([]() {
        RenderFrame();
        return META_NS::IAny::Ptr {};
    });
    // Task used for continuous rendering..
    continuousFrameAsync = META_NS::MakeCallback<META_NS::ITaskQueueTask>([]() {
        RenderFrame();
        return 1;
    });

    auto engine_init = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>(InitFunction, platformCreateInfo);
    engineThread->AddWaitableTask(engine_init)->Wait();
}
void DeinitRenderThread()
{
    if (renderTask) {
        engineThread->CancelTask(renderTask);
        renderTask = nullptr;
    }
    auto engine_init = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([]() {
        // destroy all swapchains..
        DestroySwaps();
        auto& obr = META_NS::GetObjectRegistry();
        auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());

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

        renderContext_.reset();
        engine.reset();

        return META_NS::IAny::Ptr {};
    });
    engineThread->AddWaitableTask(engine_init)->Wait();

    singleFrameAsync.reset();
    continuousFrameAsync.reset();
    auto& tr = META_NS::GetTaskQueueRegistry();
    tr.UnregisterTaskQueue(ENGINE_THREAD_DEP);
    engineThread.reset();
    tr.UnregisterTaskQueue(IO_QUEUE);
    ioThread.reset();
    tr.UnregisterTaskQueue(JS_RELEASE_THREAD);
    releaseThread.reset();
}

napi_value renderFrame(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext<bool> fc(env, info);
    bool async = fc.Arg<0>();

    if (renderTask) {
        engineThread->CancelTask(renderTask);
        renderTask = nullptr;
    }
    if (async) {
        renderTask = engineThread->AddTask(singleFrameAsync);
    } else {
        engineThread->AddWaitableTask(singleFrameSync)->Wait();
    }

    return nullptr;
}

napi_value stopRendering(napi_env env, napi_callback_info info)
{
    engineThread->CancelTask(renderTask);
    return nullptr;
}

typedef void (*FrameDoneCBType)(void*);
extern void (*SetFrameCB)(FrameDoneCBType cb, void*);

napi_value startRendering(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext<double> fc(env, info);
    if (renderTask) {
        engineThread->CancelTask(renderTask);
        renderTask = nullptr;
    }
    if (fc) {
        double intr = fc.Arg<0>();
        if (intr == 0.0f) {
            return nullptr;
        }
        auto ts = META_NS::TimeSpan::Seconds(1) / intr;
        renderTask = engineThread->AddTask(continuousFrameAsync, ts);
    } else {
        // yeah..
        if (!SetFrameCB) {
            renderTask = engineThread->AddTask(continuousFrameAsync);
        } else {
            // two options here..
            // 1. hook to the "vsync" event. (directly with out JS interference.)
            auto CB = [](void* ctx) {
                if (renderTask) {
                    engineThread->CancelTask(renderTask);
                    renderTask = nullptr;
                }
                renderTask = engineThread->AddTask(singleFrameAsync);
            };
            SetFrameCB(CB, reinterpret_cast<void*>(0xDEED));
            if (!renderTask) {
                renderTask = engineThread->AddTask(singleFrameAsync);
            }
            // 2. hook to the "vsync" event. (in JS thread.)
        }
    }
    return nullptr;
}
