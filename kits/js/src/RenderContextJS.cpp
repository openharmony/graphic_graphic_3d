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

#include "RenderContextJS.h"

#include <base/util/uid.h>

#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_context.h>

#include <scene/interface/intf_shader.h>
#include <scene/interface/intf_render_context.h>
#include <render/device/intf_gpu_resource_manager.h>


#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>

#include <render/device/intf_gpu_resource_manager.h>

#include "JsObjectCache.h"
#include "ParamParsing.h"
#include "Promise.h"
#include "nodejstaskqueue.h"

static constexpr BASE_NS::Uid IO_QUEUE { "be88e9a0-9cd8-45ab-be48-937953dc258f" };

META_TYPE(BASE_NS::shared_ptr<CORE_NS::IImageLoaderManager::LoadResult>);

struct GlobalResources {
    NapiApi::WeakRef defaultContext;
};

static BASE_NS::weak_ptr<GlobalResources> globalResources;

RenderResources::RenderResources(napi_env env) : env_(env)
{
    // Acquire the js task queue. (make sure that as long as we have a scene, the nodejstaskqueue is useable)
    if (auto jsQueue = interface_cast<INodeJSTaskQueue>(GetOrCreateNodeTaskQueue(env))) {
        jsQueue->Acquire();
    }
}

RenderResources::~RenderResources()
{
    disposeContainer_.DisposeAll(env_);

    for (auto b : bitmaps_) {
        b.second.reset();
    }

    // Release the js task queue.
    auto tq = META_NS::GetTaskQueueRegistry().GetTaskQueue(JS_THREAD_DEP);
    if (auto p = interface_cast<INodeJSTaskQueue>(tq)) {
        p->Release();
        // check if we can safely de-init here also.
        if (p->IsReleased()) {
            // destroy and unregister the queue.
            DeinitNodeTaskQueue();
        }
    }
}

void RenderResources::DisposeHook(uintptr_t token, NapiApi::Object obj)
{
    disposeContainer_.DisposeHook(token, obj);
}

void RenderResources::ReleaseDispose(uintptr_t token)
{
    disposeContainer_.ReleaseDispose(token);
}

void RenderResources::StrongDisposeHook(uintptr_t token, NapiApi::Object obj)
{
    disposeContainer_.StrongDisposeHook(token, obj);
}

void RenderResources::ReleaseStrongDispose(uintptr_t token)
{
    disposeContainer_.ReleaseStrongDispose(token);
}

void RenderResources::StoreBitmap(BASE_NS::string_view uri, SCENE_NS::IBitmap::Ptr bitmap)
{
    CORE_NS::UniqueLock lock(mutex_);
    if (bitmap) {
        bitmaps_[uri] = bitmap;
    } else {
        // setting null. releases.
        bitmaps_.erase(uri);
    }
}

SCENE_NS::IBitmap::Ptr RenderResources::FetchBitmap(BASE_NS::string_view uri)
{
    CORE_NS::UniqueLock lock(mutex_);
    auto it = bitmaps_.find(uri);
    if (it != bitmaps_.end()) {
        return it->second;
    }
    return {};
}

void RenderContextJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;
    napi_property_descriptor props[] = {
        // static methods
        Method<NapiApi::FunctionContext<>, RenderContextJS, &RenderContextJS::GetResourceFactory>(
            "getRenderResourceFactory"),
        Method<NapiApi::FunctionContext<>, RenderContextJS, &RenderContextJS::Dispose>("destroy"),

        // SceneResourceFactory methods
        Method<NapiApi::FunctionContext<BASE_NS::string>, RenderContextJS, &RenderContextJS::LoadPlugin>("loadPlugin"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, RenderContextJS, &RenderContextJS::CreateShader>(
            "createShader"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, RenderContextJS, &RenderContextJS::CreateImage>(
            "createImage"),
        Method<NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>, RenderContextJS,
               &RenderContextJS::CreateMeshResource>("createMesh"),
    };

    napi_value func;
    auto status = napi_define_class(env, "RenderContext", NAPI_AUTO_LENGTH, BaseObject::ctor<RenderContextJS>(),
        nullptr, sizeof(props) / sizeof(props[0]), props, &func);

    napi_set_named_property(env, exports, "RenderContext", func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, reinterpret_cast<void**>(&mis));
    if (mis) {
        mis->StoreCtor("RenderContext", func);
    }
}

void RenderContextJS::RegisterEnums(NapiApi::Object exports) {}

RenderContextJS::RenderContextJS(napi_env env, napi_callback_info info)
    : BaseObject(env, info), env_(env), globalResources_(globalResources.lock())
{
    LOG_V("RenderContextJS ++");
}

RenderContextJS::~RenderContextJS()
{
    LOG_V("RenderContextJS --");
}

void* RenderContextJS::GetInstanceImpl(uint32_t id)
{
    if (id == RenderContextJS::ID) {
        return this;
    }

    return nullptr;
}

BASE_NS::shared_ptr<RenderResources> RenderContextJS::GetResources() const
{
    if (auto resources = resources_.lock()) {
        return resources;
    }

    BASE_NS::shared_ptr<RenderResources> resources(new RenderResources(env_));
    resources_ = resources;

    return resources;
}

napi_value RenderContextJS::GetDefaultContext(napi_env env)
{
    auto resources = globalResources.lock();
    if (!resources) {
        resources.reset(new GlobalResources);
        globalResources = resources;
    }

    NapiApi::Object context = NapiApi::Object(env, "RenderContext", {});
    resources->defaultContext = context;

    return context.ToNapiValue();
}

napi_value RenderContextJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    return {};
}

void RenderContextJS::DisposeNative(void*) {}

void RenderContextJS::Finalize(napi_env env) {}

napi_value RenderContextJS::GetResourceFactory(NapiApi::FunctionContext<>& ctx)
{
    return ctx.This().ToNapiValue();
}

napi_value RenderContextJS::LoadPlugin(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    auto c = ctx.Arg<0>().valueOrDefault();
    if (!BASE_NS::IsUidString(c)) {
        LOG_E("\"%s\" is not a Uid string", c.c_str());

        Promise promise(ctx.GetEnv());
        NapiApi::Value<bool> result(ctx.GetEnv(), false);
        promise.Resolve(result.ToNapiValue());

        return promise.ToNapiValue();
    }

    LOG_E("Loading plugin: %s", c.c_str());

    BASE_NS::Uid uid(*(char(*)[37])c.data());

    const auto engineQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
    META_NS::AddFutureTaskOrRunDirectly(engineQ, [uid]() {
        Core::GetPluginRegister().LoadPlugins({uid});
    });

    Promise promise(ctx.GetEnv());
    NapiApi::Value<bool> result(ctx.GetEnv(), true);
    promise.Resolve(result.ToNapiValue());

    return promise.ToNapiValue();
}

napi_value RenderContextJS::CreateShader(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    return ctx.GetUndefined();
}

napi_value RenderContextJS::CreateImage(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    // Create an image in four steps:
    // 1. Parse args in JS thread (this function body)
    // 2. Load image data in IO thread
    // 3. Create GPU resource in engine thread
    // 4. Settle promise by converting to JS object in JS release thread
    using namespace RENDER_NS;
    const auto env = ctx.GetEnv();
    auto promise = Promise(env);

    NapiApi::Object resourceParams = ctx.Arg<0>();
    auto uri = ExtractUri(resourceParams.Get<NapiApi::Object>("uri"));
    if (uri.empty()) {
        auto u = resourceParams.Get<BASE_NS::string>("uri");
        uri = ExtractUri(u);
    }

    if (uri.empty()) {
        return promise.Reject("Invalid scene resource Image parameters given");
    }

    if (auto resources = resources_.lock()) {
        if (const auto bitmap = resources->FetchBitmap(uri)) {
            // no aliasing.. so the returned bitmaps name is.. the old one.
            const auto result = FetchJsObj(bitmap).ToNapiValue();
            return promise.Resolve(result);
        }
    }

    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    auto renderContext = doc->GetProperty<SCENE_NS::IRenderContext::Ptr>("RenderContext")->GetValue()->GetRenderer();

    using LoadResult = CORE_NS::IImageLoaderManager::LoadResult;
    auto loadImage = [uri, renderContext]() {
        uint32_t imageLoaderFlags = CORE_NS::IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS;
        auto& imageLoaderMgr = renderContext->GetEngine().GetImageLoaderManager();
        // LoadResult contains a unique pointer, so can't copy. Move it to the heap and pass a pointer instead.
        return BASE_NS::shared_ptr<LoadResult> { new LoadResult { imageLoaderMgr.LoadImage(uri, imageLoaderFlags) } };
    };

    auto createGpuResource = [uri, renderContext](
                                 BASE_NS::shared_ptr<LoadResult> loadResult) -> SCENE_NS::IBitmap::Ptr {
        if (!loadResult->success) {
            LOG_E("%s", BASE_NS::string { "Failed to load image: " }.append(loadResult->error).c_str());
            return {};
        }

        auto& gpuResourceMgr = renderContext->GetDevice().GetGpuResourceManager();
        GpuImageDesc gpuDesc = gpuResourceMgr.CreateGpuImageDesc(loadResult->image->GetImageDesc());
        gpuDesc.usageFlags = CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (gpuDesc.engineCreationFlags & EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS) {
            gpuDesc.usageFlags |= CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        gpuDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        const auto imageHandle = gpuResourceMgr.Create(uri, gpuDesc, std::move(loadResult->image));

        auto& obr = META_NS::GetObjectRegistry();
        auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        auto bitmap = META_NS::GetObjectRegistry().Create<SCENE_NS::IBitmap>(SCENE_NS::ClassId::Bitmap, doc);
        if (auto m = interface_cast<META_NS::IMetadata>(bitmap)) {
            m->AddProperty(META_NS::ConstructProperty<BASE_NS::string>("Uri", uri));
        }
        if (auto i = interface_cast<SCENE_NS::IRenderResource>(bitmap)) {
            i->SetRenderHandle(imageHandle);
        }
        return bitmap;
    };

    auto convertToJs = [promise, uri, contextRef = NapiApi::StrongRef(ctx.This()), resources = GetResources(),
                           paramRef = NapiApi::StrongRef(resourceParams)](SCENE_NS::IBitmap::Ptr bitmap) mutable {
        if (!bitmap) {
            promise.Reject(BASE_NS::string { "Failed to load image from URI " }.append(uri));
            return;
        }
        const auto env = promise.Env();
        napi_value args[] = { contextRef.GetValue(), paramRef.GetValue() };
        const auto result = CreateFromNativeInstance(env, bitmap, PtrType::WEAK, args);

        auto renderContextJs = contextRef.GetObject().GetJsWrapper<RenderContextJS>();
        resources->StoreBitmap(uri, BASE_NS::move(bitmap));

        promise.Resolve(result.ToNapiValue());
    };

    const auto ioQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(IO_QUEUE);
    const auto engineQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
    const auto jsQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(JS_THREAD_DEP);
    META_NS::AddFutureTaskOrRunDirectly(ioQ, BASE_NS::move(loadImage))
        .Then(BASE_NS::move(createGpuResource), engineQ)
        .Then(BASE_NS::move(convertToJs), jsQ);

    return promise;
}

napi_value RenderContextJS::CreateMeshResource(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    return ctx.GetUndefined();
}
