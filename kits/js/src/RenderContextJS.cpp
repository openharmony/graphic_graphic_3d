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

#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/property/construct_property.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_render_context.h>
#include <scene/interface/intf_shader.h>
#if defined(__OHOS_PLATFORM__)
#include <scene_adapter/intf_surface_stream.h>
#include <scene_adapter/surface_stream.h>
#endif
#include <render/device/intf_gpu_resource_manager.h>

#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <base/util/uid.h>
#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>

#include "DisposeContainer.h"
#include "JsObjectCache.h"
#include "ParamParsing.h"
#include "Promise.h"
#include "SamplerJS.h"
#include "SceneJS.h"
#include "geometry_definition/GeometryDefinition.h"
#include "nodejstaskqueue.h"

#ifdef __SCENE_ADAPTER__
#include <parameters.h>

#include "3d_widget_adapter_log.h"
#include "scene_adapter/scene_adapter.h"
#endif
#include "lume_trace.h"
#include "product_stubs.h"

static constexpr BASE_NS::Uid IO_QUEUE{"be88e9a0-9cd8-45ab-be48-937953dc258f"};
static constexpr size_t MIN_SCENE_LOAD_ARG_COUNT{2};

META_TYPE(BASE_NS::shared_ptr<CORE_NS::IImageLoaderManager::LoadResult>);

struct GlobalResources {
    NapiApi::WeakRef defaultContext;
};

static BASE_NS::weak_ptr<GlobalResources> globalResources;

namespace {
// fix names to match "ye olde" implementation
// the bug that unnamed nodes stops hierarchy creation also still exists, works around that issue too.
void Fixnames(SCENE_NS::IScene::Ptr scene)
{
    struct rr {
        uint32_t id_ = 1;
        // not actual tree, but map of entities, and their children.
        BASE_NS::unordered_map<CORE_NS::Entity, BASE_NS::vector<CORE_NS::Entity>> tree;
        BASE_NS::vector<CORE_NS::Entity> roots;
        CORE3D_NS::INodeComponentManager* cm;
        CORE3D_NS::INameComponentManager* nm;
        explicit rr(SCENE_NS::IScene::Ptr scene)
        {
            auto internalScene = scene ? scene->GetInternalScene() : nullptr;
            CORE_NS::IEcs::Ptr ecs = internalScene ? internalScene->GetEcsContext().GetNativeEcs() : nullptr;
            cm = ecs ? CORE_NS::GetManager<CORE3D_NS::INodeComponentManager>(*ecs) : nullptr;
            nm = ecs ? CORE_NS::GetManager<CORE3D_NS::INameComponentManager>(*ecs) : nullptr;
            if (cm && nm) {
                fix();
            }
        }
        void scan()
        {
            const auto count = cm->GetComponentCount();
            // collect nodes and their children.
            tree.reserve(cm->GetComponentCount());
            for (auto i = 0; i < count; i++) {
                auto enti = cm->GetEntity(i);
                // add node to our list. (if not yet added)
                tree.insert({enti, {}});
                auto parent = cm->Get(i).parent;
                if (CORE_NS::EntityUtil::IsValid(parent)) {
                    tree[parent].push_back(enti);
                } else {
                    // no parent, so it's a "root"
                    roots.push_back(enti);
                }
            }
        }
        void recurse(CORE_NS::Entity id)
        {
            CORE3D_NS::NameComponent c = nm->Get(id);
            if (c.name.empty()) {
                // create a name for unnamed node.
                c.name = "Unnamed Node ";
                c.name += BASE_NS::to_string(id_++);
                nm->Set(id, c);
            }
            for (auto c : tree[id]) {
                recurse(c);
            }
        }
        void fix()
        {
            scan();
            for (auto i : roots) {
                id_ = 1;
                // force root node name to match legacy by default.
                for (auto c : tree[i]) {
                    recurse(c);
                }
            }
        }
    } r(scene);
}
// LEGACY COMPATIBILITY end

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;

BASE_NS::string_view ExtractPathToProject(BASE_NS::string_view uri)
{
    // Assume the scene file is in a folder that is at the root of the project.
    // ExtractPathToProject("schema://path/to/PROJECT/assets/default.scene2") == "schema://path/to/PROJECT"
    const auto secondToLastSlashPos = uri.find_last_of('/', uri.find_last_of('/') - 1);
    return uri.substr(0, secondToLastSlashPos);
}

// Based on the file extension, supply the scene manager a file resource manager to handle loading.
SCENE_NS::ISceneManager::Ptr CreateSceneManager(BASE_NS::string_view uri)
{
    auto& objRegistry = META_NS::GetObjectRegistry();
    auto objContext = interface_pointer_cast<META_NS::IMetadata>(objRegistry.GetDefaultObjectContext());

    if (uri.ends_with(".scene2")) {
        const auto renderContext = SCENE_NS::GetBuildArg<SCENE_NS::IRenderContext::Ptr>(objContext, "RenderContext");
        if (!renderContext || !renderContext->GetRenderer()) {
            LOG_E("Unable to configure file resource manager for loading scene files: render context missing");
            return {};
        }
        auto& fileManager = renderContext->GetRenderer()->GetEngine().GetFileManager();
        fileManager.RegisterPath("project", ExtractPathToProject(uri), true);
    }

    return objRegistry.Create<SCENE_NS::ISceneManager>(SCENE_NS::ClassId::SceneManager, objContext);
}

void AddScene(META_NS::IObjectRegistry* obr, SCENE_NS::IScene::Ptr scene)
{
    if (!obr) {
        return;
    }
    auto params = interface_pointer_cast<META_NS::IMetadata>(obr->GetDefaultObjectContext());
    if (!params) {
        return;
    }
    auto duh = params->GetArrayProperty<IntfWeakPtr>("Scenes");
    if (!duh) {
        return;
    }
    duh->AddValue(interface_pointer_cast<CORE_NS::IInterface>(scene));
}
BASE_NS::string ExtractResourceUri(NapiApi::Object& params)
{
    auto uri = ExtractUri(params.Get<NapiApi::Object>("uri"));
    if (uri.empty()) {
        uri = ExtractUri(params.Get<BASE_NS::string>("uri"));
    }
    return uri;
}

BASE_NS::string NormalizeUri(BASE_NS::string uri)
{
    if (uri.empty()) {
        uri = "scene://empty";
    }
    for (auto t = uri.find_first_of('\\'); t != BASE_NS::string::npos; t = uri.find_first_of('\\')) {
        uri[t] = '/';
    }
    return uri;
}

SCENE_NS::IRenderContext::Ptr GetRenderContextFromDefault()
{
    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    if (!doc) {
        return nullptr;
    }
    auto prop = doc->GetProperty<SCENE_NS::IRenderContext::Ptr>("RenderContext");
    if (!prop) {
        return nullptr;
    }
    return prop->GetValue();
}

SCENE_NS::IScene::Ptr PrepareScene(SCENE_NS::IScene::Ptr scene)
{
    if (!scene) {
        return {};
    }
    auto renderConfig = scene->RenderConfiguration();
    if (!renderConfig || !renderConfig->GetValue()) {
        return {};
    }
    auto internalScene = scene->GetInternalScene();
    if (!internalScene) {
        return {};
    }
    internalScene->GetEcsContext().CreateUnnamedRootNode();
    Fixnames(scene);
    auto& obr = META_NS::GetObjectRegistry();
    AddScene(&obr, scene);
    return scene;
}
}  // anonymous namespace

RenderResources::RenderResources(napi_env env) : env_(env)
{
    LUME_TRACE_FUNC()
    // Acquire the js task queue. (make sure that as long as we have a scene, the nodejstaskqueue is useable)
    if (auto jsQueue = interface_cast<INodeJSTaskQueue>(GetOrCreateNodeTaskQueue(env))) {
        jsQueue->Acquire();
    }
}

RenderResources::~RenderResources()
{
    LUME_TRACE_FUNC()
    disposeContainer_.Dispose(env_, BASE_NS::vector<uintptr_t>{}, BASE_NS::vector<uintptr_t>{});

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
    LUME_TRACE_FUNC()
    disposeContainer_.DisposeHook(token, obj);
}

void RenderResources::ReleaseDispose(uintptr_t token)
{
    LUME_TRACE_FUNC()
    disposeContainer_.ReleaseDispose(token);
}

void RenderResources::StrongDisposeHook(uintptr_t token, NapiApi::Object obj)
{
    LUME_TRACE_FUNC()
    disposeContainer_.StrongDisposeHook(token, obj);
}

void RenderResources::ReleaseStrongDispose(uintptr_t token)
{
    LUME_TRACE_FUNC()
    disposeContainer_.ReleaseStrongDispose(token);
}

void RenderResources::Dispose(
    napi_env env, BASE_NS::array_view<const uintptr_t> strongs, BASE_NS::array_view<const uintptr_t> weaks, SceneJS* sc)
{
    LUME_TRACE_FUNC()
    disposeContainer_.Dispose(env, strongs, weaks, sc);
}

void RenderResources::StoreBitmap(BASE_NS::string_view uri, SCENE_NS::IBitmap::Ptr bitmap)
{
    LUME_TRACE_FUNC()
    // we need to extend the life-cycle of an existing item over the container lock
    SCENE_NS::IBitmap::Ptr old;
    CORE_NS::UniqueLock lock(mutex_);
    if (bitmap) {
        auto& existing = bitmaps_[uri];
        old = BASE_NS::move(existing);
        existing = BASE_NS::move(bitmap);
    } else {
        if (auto ite = bitmaps_.find(uri); ite != bitmaps_.cend()) {
            // setting null. releases.
            old = BASE_NS::move(ite->second);
            bitmaps_.erase(ite);
        }
    }
}

SCENE_NS::IBitmap::Ptr RenderResources::FetchBitmap(BASE_NS::string_view uri)
{
    LUME_TRACE_FUNC()
    CORE_NS::UniqueLock lock(mutex_);
    auto it = bitmaps_.find(uri);
    if (it != bitmaps_.end()) {
        return it->second;
    }
    return {};
}

void RenderContextJS::Init(napi_env env, napi_value exports)
{
    LUME_TRACE_FUNC()
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
        Method<NapiApi::FunctionContext<NapiApi::Object>, RenderContextJS, &RenderContextJS::CreateImageStream>(
            "createImageStream"),
        Method<NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>,
            RenderContextJS,
            &RenderContextJS::CreateMeshResource>("createMesh"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, RenderContextJS, &RenderContextJS::CreateSampler>(
            "createSampler"),
        Method<NapiApi::FunctionContext<>, RenderContextJS, &RenderContextJS::CreateScene>("createScene"),
        Method<NapiApi::FunctionContext<BASE_NS::string, BASE_NS::string>,
            RenderContextJS,
            &RenderContextJS::RegisterResourcePath>("registerResourcePath"),
    };

    napi_value func;
    auto status = napi_define_class(env,
        "RenderContext",
        NAPI_AUTO_LENGTH,
        BaseObject::ctor<RenderContextJS>(),
        nullptr,
        sizeof(props) / sizeof(props[0]),
        props,
        &func);
    if (status != napi_ok) {
        LOG_E("Failed to define class RenderContext");
    }

    napi_set_named_property(env, exports, "RenderContext", func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, reinterpret_cast<void**>(&mis));
    if (mis) {
        mis->StoreCtor("RenderContext", func);
    }
}

void RenderContextJS::RegisterEnums(NapiApi::Object exports)
{}

RenderContextJS::RenderContextJS(napi_env env, napi_callback_info info)
    : BaseObject(env, info), env_(env), globalResources_(globalResources.lock())
{
    LUME_TRACE_FUNC()
    LOG_V("RenderContextJS ++");
    // Acquire the js task queue. (make sure that as long as we have a scene, the nodejstaskqueue is useable)
    if (auto jsQueue = interface_cast<INodeJSTaskQueue>(GetOrCreateNodeTaskQueue(env))) {
        jsQueue->Acquire();
    }

    InitRenderManager();
    if (!renderManager_) {
        LOG_E("Init render manager failed");
    }
}

RenderContextJS::~RenderContextJS()
{
    LUME_TRACE_FUNC()
    LOG_V("RenderContextJS --");
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

void* RenderContextJS::GetInstanceImpl(uint32_t id)
{
    if (id == RenderContextJS::ID) {
        return static_cast<RenderContextJS*>(this);
    }

    return BaseObject::GetInstanceImpl(id);
}

BASE_NS::shared_ptr<RenderResources> RenderContextJS::GetResources() const
{
    LUME_TRACE_FUNC()
    if (!resources_) {
        resources_.reset(new RenderResources(env_));
    }
    return resources_;
}

napi_value RenderContextJS::GetDefaultContext(napi_env env)
{
    LUME_TRACE_FUNC()
#ifdef __SCENE_ADAPTER__
    if (!OHOS::Render3D::SceneAdapter::IsEngineInitSuccessful()) {
        return NapiApi::Env(env).GetNull();
    }
#endif
    auto resources = globalResources.lock();
    if (!resources) {
        resources.reset(new GlobalResources);
        globalResources = resources;
    }

    if (auto value = resources->defaultContext.GetValue()) {
        return value;
    }

    NapiApi::Object context = NapiApi::Object(env, "RenderContext", {});
    resources->defaultContext = context;

    return context.ToNapiValue();
}

napi_value RenderContextJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    return {};
}

void RenderContextJS::DisposeNative()
{}

void RenderContextJS::Finalize(napi_env env)
{}

napi_value RenderContextJS::GetResourceFactory(NapiApi::FunctionContext<>& ctx)
{
    return ctx.This().ToNapiValue();
}

napi_value RenderContextJS::LoadPlugin(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    LUME_TRACE_FUNC()
    Promise promise(ctx.GetEnv());
    auto c = ctx.Arg<0>().valueOrDefault();
    if (!BASE_NS::IsUidString(c)) {
        LOG_E("\"%s\" is not a Uid string", c.c_str());
        NapiApi::Value<bool> result(ctx.GetEnv(), false);
        promise.Resolve(result.ToNapiValue());
        return promise;
    }

    BASE_NS::Uid uid(*(char(*)[37])c.data());

    // Acquire the JS queue to prevent teardown while the engine task is in flight.
    // Released in convertToJs after the promise is settled.
    const auto jsQ = GetOrCreateNodeTaskQueue(ctx.GetEnv());
    auto jsQRef = interface_cast<INodeJSTaskQueue>(jsQ);
    if (jsQRef) {
        jsQRef->Acquire();
    }

    auto convertToJs = [promise, jsQRef](bool success) mutable {
        NapiApi::Value<bool> result(promise.Env(), success);
        promise.Resolve(result.ToNapiValue());
        if (jsQRef) {
            jsQRef->Release();
        }
    };
    const auto engineQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
    META_NS::AddFutureTaskOrRunDirectly(engineQ, [uid]() {
        return Core::GetPluginRegister().LoadPlugins({uid});
    }).Then(BASE_NS::move(convertToJs), jsQ);

    return promise;
}

META_NS::ITaskQueue::Ptr AcquireJsQueue(napi_env env)
{
    auto jsQ = GetOrCreateNodeTaskQueue(env);
    if (auto ref = interface_cast<INodeJSTaskQueue>(jsQ)) {
        ref->Acquire();
    }
    return jsQ;
}

void ConvertImageToJs(Promise& promise, const BASE_NS::string& uri, SCENE_NS::IBitmap::Ptr bitmap,
    NapiApi::StrongRef& contextRef, NapiApi::StrongRef& paramRef)
{
    if (!bitmap) {
        promise.Reject(BASE_NS::string{"Failed to load image from URI "}.append(uri));
        return;
    }
    const auto env = promise.Env();
    napi_value args[] = {contextRef.GetValue(), paramRef.GetValue()};
    const auto result = CreateFromNativeInstance(env, bitmap, PtrType::WEAK, args);
    promise.Resolve(result.ToNapiValue());
}

void ConvertShaderToJs(Promise& promise, const BASE_NS::string& uri, SCENE_NS::IShader::Ptr shader,
    NapiApi::StrongRef& sceneRef, NapiApi::StrongRef& paramRef)
{
    if (!shader) {
        promise.Reject(BASE_NS::string{"Failed to load shader from URI "}.append(uri));
        return;
    } else {
        CORE_LOG_I("success to load shader %s", uri.c_str());
    }
    const auto env = promise.Env();
    napi_value args[] = {sceneRef.GetValue(), paramRef.GetValue()};
    NapiApi::Object parms(env, args[1]);

    napi_value null;
    napi_get_null(env, &null);
    parms.Set("Material", null);
    const auto result = CreateFromNativeInstance(env, shader, PtrType::STRONG, args);
    promise.Resolve(result.ToNapiValue());
}

void ConvertSceneToJs(Promise& promise, const BASE_NS::string& uri, SCENE_NS::IScene::Ptr scene)
{
    if (!scene) {
        promise.Reject("Scene creation failed");
        return;
    }

    const auto env = promise.Env();
    auto jsscene = CreateFromNativeInstance(env, scene, PtrType::STRONG, {});
    const auto sceneJs = jsscene.GetJsWrapper<SceneJS>();
    if (!sceneJs) {
        promise.Reject("SceneJS creation failed");
        return;
    }

    auto curenv = jsscene.Get<NapiApi::Object>("environment");
    if (curenv.IsUndefinedOrNull()) {
        NapiApi::Object argsIn(env);
        argsIn.Set("name", "DefaultEnv");
        auto res = sceneJs->CreateEnvironment(jsscene, argsIn);
        res.Set("backgroundType", NapiApi::Value<uint32_t>(env, 1));
        jsscene.Set("environment", res);
    }
    for (auto&& c : scene->GetCameras().GetResult()) {
        c->RenderingPipeline()->SetValue(SCENE_NS::CameraPipeline::FORWARD);
        c->ColorTargetCustomization()->SetValue({SCENE_NS::ColorFormat{BASE_NS::BASE_FORMAT_R16G16B16A16_SFLOAT}});
    }

    const auto result = jsscene.ToNapiValue();
#ifdef __SCENE_ADAPTER__
    auto sceneAdapter = std::make_shared<OHOS::Render3D::SceneAdapter>();
    sceneAdapter->SetSceneObj(interface_pointer_cast<META_NS::IObject>(scene));
    sceneJs->scene_ = sceneAdapter;
    RenderContextJSPrivate::InitSystems(sceneAdapter, uri);
#endif
    promise.Resolve(result);
}

SCENE_NS::IImage::Ptr FetchExistingImage(
    const BASE_NS::shared_ptr<RenderResources>& resources, BASE_NS::string_view uri)
{
    return resources ? resources->FetchBitmap(uri) : nullptr;
}

SCENE_NS::IBitmap::Ptr CreateGpuImageResource(BASE_NS::string_view uri, SCENE_NS::IRenderContext::Ptr renderContext,
    BASE_NS::shared_ptr<RenderResources> resources,
    BASE_NS::shared_ptr<CORE_NS::IImageLoaderManager::LoadResult> loadResult)
{
    using namespace RENDER_NS;
    if (const auto bitmap = FetchExistingImage(resources, uri)) {
        return bitmap;
    }
    if (!loadResult || !loadResult->success) {
        LOG_E("%s",
            BASE_NS::string{"Failed to load image: "}.append(loadResult ? loadResult->error : "null result").c_str());
        return {};
    }
    if (!loadResult->image) {
        LOG_E("Image load succeeded but image data is null");
        return {};
    }

    auto& gpuResourceMgr = renderContext->GetRenderer()->GetDevice().GetGpuResourceManager();
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
        m->AddProperty(META_NS::ConstructProperty<BASE_NS::string>("Uri", BASE_NS::string(uri)));
    }
    if (auto i = interface_cast<SCENE_NS::IRenderResource>(bitmap)) {
        i->SetRenderHandle(imageHandle);
    }
    if (bitmap && resources) {
        resources->StoreBitmap(uri, bitmap);
    }
    return bitmap;
}

napi_value RenderContextJS::CreateShader(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    const auto env = ctx.GetEnv();
    auto promise = Promise(env);

    if (!renderManager_) {
        InitRenderManager();
    }
    if (!renderManager_) {
        return promise.Reject("Invalid render context");
    }

    NapiApi::Object resourceParams = ctx.Arg<0>();
    if (!resourceParams) {
        return promise.Reject("Invalid scene resource shader parameters given");
    }
    auto uri = ExtractResourceUri(resourceParams);
    if (uri.empty()) {
        return promise.Reject("Invalid scene resource shader parameters given");
    }

    auto jsQ = AcquireJsQueue(env);
    auto jsQRef = interface_cast<INodeJSTaskQueue>(jsQ);
    auto convertToJs = [promise,
                           uri,
                           sceneRef = NapiApi::StrongRef(ctx.This()),
                           paramRef = NapiApi::StrongRef(resourceParams),
                           jsQRef](SCENE_NS::IShader::Ptr shader) mutable {
        ConvertShaderToJs(promise, uri, shader, sceneRef, paramRef);
        if (jsQRef) {
            jsQRef->Release();
        }
    };

    renderManager_->LoadShader(uri).Then(BASE_NS::move(convertToJs), jsQ);
    return promise;
}

napi_value RenderContextJS::CreateImage(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    using namespace RENDER_NS;
    const auto env = ctx.GetEnv();
    auto promise = Promise(env);
    NapiApi::Object resourceParams = ctx.Arg<0>();
    auto uri = ExtractResourceUri(resourceParams);
    if (uri.empty()) {
        return promise.Reject("Invalid scene resource Image parameters given");
    }

    const auto resources = GetResources();
    if (const auto bitmap = FetchExistingImage(resources, uri)) {
        const auto result = FetchJsObj(bitmap);
        if (result.IsDefined() && !result.IsNull()) {
            return promise.Resolve(result.ToNapiValue());
        }
    }
    auto renderContext = GetRenderContextFromDefault();
    if (!renderContext) {
        return promise.Reject("CreateImage: renderer not available");
    }
    using LoadResult = CORE_NS::IImageLoaderManager::LoadResult;
    auto loadImage = [uri, renderContext]() {
        auto& mgr = renderContext->GetRenderer()->GetEngine().GetImageLoaderManager();
        return BASE_NS::shared_ptr<LoadResult>{
            new LoadResult{mgr.LoadImage(uri, CORE_NS::IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS)}};
    };
    auto createGpuResource = [uri, renderContext, resources](
                                 BASE_NS::shared_ptr<LoadResult> loadResult) -> SCENE_NS::IBitmap::Ptr {
        return CreateGpuImageResource(uri, renderContext, resources, BASE_NS::move(loadResult));
    };
    const auto jsQ = AcquireJsQueue(env);
    auto jsQRef = interface_cast<INodeJSTaskQueue>(jsQ);
    auto convertToJs = [promise,
                           uri,
                           contextRef = NapiApi::StrongRef(ctx.This()),
                           paramRef = NapiApi::StrongRef(resourceParams),
                           jsQRef](SCENE_NS::IBitmap::Ptr bitmap) mutable {
        ConvertImageToJs(promise, uri, bitmap, contextRef, paramRef);
        if (jsQRef) {
            jsQRef->Release();
        }
    };
    const auto ioQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(IO_QUEUE);
    const auto engineQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
    META_NS::AddFutureTaskOrRunDirectly(ioQ, BASE_NS::move(loadImage))
        .Then(BASE_NS::move(createGpuResource), engineQ)
        .Then(BASE_NS::move(convertToJs), jsQ);
    return promise;
}

napi_value RenderContextJS::CreateImageStream(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
#if !defined(__OHOS_PLATFORM__)
    LOG_E("CreateImageStream is not supported on this platform");
    return ctx.GetUndefined();
#else
    LUME_TRACE_FUNC()
    using namespace RENDER_NS;
    const auto env = ctx.GetEnv();
    auto promise = Promise(env);

    NapiApi::Object resourceParams = ctx.Arg<0>();
    BASE_NS::string uri = "Internal://SurfaceStream_";
    auto& obr = META_NS::GetObjectRegistry();
    obr.RegisterObjectType<OHOS::Render3D::SurfaceStream>();
    const auto resources = GetResources();

    auto createGpuResource = [uri, resources]() mutable -> SCENE_NS::IBitmap::Ptr {
        auto& obr = META_NS::GetObjectRegistry();
        auto doc = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        auto bitmap = obr.Create<SCENE_NS::IBitmap>(SCENE_NS::ClassId::Bitmap, doc);
        uri += BASE_NS::to_string(reinterpret_cast<uintptr_t>(bitmap.get()));
        if (auto metadata = interface_cast<META_NS::IMetadata>(bitmap)) {
            metadata->AddProperty(META_NS::ConstructProperty<BASE_NS::string>("Uri", uri));
        }

        auto surfaceStream = obr.Create<OHOS::Render3D::ISurfaceStream>(OHOS::Render3D::ClassId::SurfaceStream, doc);
        if (auto attach = interface_cast<Meta::IAttach>(bitmap); attach && surfaceStream) {
            attach->Attach(surfaceStream);
        }

        if (bitmap && resources) {
            resources->StoreBitmap(uri, bitmap);
        }
        return bitmap;
    };

    auto convertToJs =
        [promise, uri, contextRef = NapiApi::StrongRef(ctx.This()), paramRef = NapiApi::StrongRef(resourceParams)](
            SCENE_NS::IBitmap::Ptr bitmap) mutable { ConvertImageToJs(promise, uri, bitmap, contextRef, paramRef); };

    const auto engineQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
    const auto jsQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(JS_THREAD_DEP);
    META_NS::AddFutureTaskOrRunDirectly(engineQ, BASE_NS::move(createGpuResource))
        .Then(BASE_NS::move(convertToJs), jsQ);

    return promise;
#endif  // defined(__OHOS_PLATFORM__)
}

napi_value RenderContextJS::CreateMeshResource(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    auto env = ctx.GetEnv();
    auto promise = Promise(env);
    auto geometry = GeometryDefinition::GeometryDefinition::FromJs(ctx.Arg<1>());
    if (!geometry) {
        return promise.Reject("Invalid geometry definition given");
    }

    // Piggyback the native geometry definition inside the resource param. Need to ditch smart pointers for the ride.
    napi_value geometryNapiValue;
    napi_create_external(ctx.Env(), geometry.release(), nullptr, nullptr, &geometryNapiValue);
    NapiApi::Object resourceParams = ctx.Arg<0>();
    resourceParams.Set("GeometryDefinition", geometryNapiValue);

    napi_value args[] = {ctx.This().ToNapiValue(), resourceParams.ToNapiValue()};
    // dummy object for now
    const auto meshResource = META_NS::GetObjectRegistry().Create(META_NS::ClassId::Object);
    const auto result = CreateFromNativeInstance(env, "MeshResource", meshResource, PtrType::STRONG, args);
    return promise.Resolve(result.ToNapiValue());
}

napi_value RenderContextJS::CreateSampler(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    return Promise(ctx.GetEnv()).Resolve(SamplerJS::CreateRawJsObject(ctx.GetEnv()));
}

static const char* ParseSceneLoadParams(NapiApi::FunctionContext<>& ctx, napi_env env, SceneLoadParams& param)
{
    if (ctx.ArgCount() < MIN_SCENE_LOAD_ARG_COUNT) {
        return nullptr;
    }

    {
        napi_valuetype type1 = napi_undefined;
        napi_typeof(env, ctx.Value(1), &type1);
        if (type1 != napi_object) {
            return "The second argument must be an object";
        }
    }

    NapiApi::Object options(env, ctx.Value(1));

    if (options.IsUndefinedOrNull("offset")) {
        return nullptr;
    }

    {
        napi_valuetype offsetType = napi_undefined;
        auto offsetNapi = options.Get("offset");
        napi_typeof(env, offsetNapi, &offsetType);
        if (offsetType != napi_number) {
            return "offset must be a number";
        }
    }

    {
        double val = 0.0;
        napi_get_value_double(env, options.Get("offset"), &val);

        if (val < 0.0) {
            return "offset must not be negative";
        }

        if (val != static_cast<double>(static_cast<int64_t>(val))) {
            return "offset must be an integer, got a float";
        }

        if (val > static_cast<double>(std::numeric_limits<int64_t>::max())) {
            return "offset exceeds int64_t range";
        }

        param.offset = static_cast<int64_t>(val);
    }

    return nullptr;
}

napi_value RenderContextJS::CreateScene(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    const auto env = ctx.Env();
    auto promise = Promise(env);

    SceneLoadParams sceneLoadParams;
    if (const char* err = ParseSceneLoadParams(ctx, env, sceneLoadParams)) {
        return promise.Reject(err);
    }

    const auto jsQ = AcquireJsQueue(env);
    auto queueRefCount = interface_cast<INodeJSTaskQueue>(jsQ);
    if (!queueRefCount) {
        return promise.Reject("Error creating a JS task queue");
    }

    BASE_NS::string uri = NormalizeUri(ExtractUri(ctx));

    auto convertToJs = [promise, uri, queueRefCount = BASE_NS::move(queueRefCount)](
                           SCENE_NS::IScene::Ptr scene) mutable {
        ConvertSceneToJs(promise, uri, scene);
        queueRefCount->Release();
    };

    auto sceneManager = CreateSceneManager(uri);
    if (!sceneManager) {
        return promise.Reject("Creating scene manager failed");
    }
    auto params = interface_pointer_cast<META_NS::IMetadata>(META_NS::GetObjectRegistry().GetDefaultObjectContext());
    if (params) {
        RenderContextJSPrivate::PopulateCustomParams(params);
    }

    auto engineQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
    sceneManager->CreateScene(uri, sceneLoadParams.offset).Then(PrepareScene, engineQ).Then(BASE_NS::move(convertToJs), jsQ);
    return promise;
}

napi_value RenderContextJS::RegisterResourcePath(NapiApi::FunctionContext<BASE_NS::string, BASE_NS::string>& ctx)
{
    LUME_TRACE_FUNC()
    using namespace RENDER_NS;

    // 1.read current path of shader and protocol name
    auto registerProtocol = ctx.Arg<0>().valueOrDefault();
    auto resourcePath = ctx.Arg<1>().valueOrDefault();
    CORE_LOG_I(
        "register resource path is: [%s], register protocol is : [%s]", resourcePath.c_str(), registerProtocol.c_str());

    // 2.Check Empty for path & protocol
    if (resourcePath.empty() || registerProtocol.empty()) {
        LOG_E("Invalid register path or protocol of assets given");
        return ctx.GetBoolean(false);
    }

    auto renderCtx = GetRenderContextFromDefault();
    if (!renderCtx) {
        LOG_E("RegisterResourcePath: render context not available");
        return ctx.GetBoolean(false);
    }
    auto& fileManager = renderCtx->GetRenderer()->GetEngine().GetFileManager();

    // 3.Check if the proxy protocol exists already.
    if (!(fileManager.CheckExistence(registerProtocol))) {
        LOG_E("Register protocol exists already");
        return ctx.GetBoolean(false);
    }
    // 4. Check if the protocol is already declared. | Register now!
    if (!(fileManager.RegisterPath(registerProtocol.c_str(), resourcePath.c_str(), false))) {
        LOG_E("Register protocol declared already");
        return ctx.GetBoolean(false);
    }

    return ctx.GetBoolean(true);
}

void RenderContextJS::InitRenderManager()
{
    auto& r = META_NS::GetObjectRegistry();
    auto obj = META_NS::GetObjectRegistry().Create<META_NS::IMetadata>(META_NS::ClassId::Object);
    if (obj) {
        auto doc = interface_cast<META_NS::IMetadata>(r.GetDefaultObjectContext());
        if (!doc) {
            LOG_E("RenderContextJS::InitRenderManager() default object context is null");
            return;
        }
        if (auto ctx = doc->GetProperty<SCENE_NS::IRenderContext::Ptr>("RenderContext"); ctx && ctx->GetProperty()) {
            auto renderContext = ctx->GetValue();
            obj->AddProperty(META_NS::ConstructProperty<SCENE_NS::IRenderContext::Ptr>("RenderContext", renderContext));

            renderManager_ = interface_pointer_cast<SCENE_NS::IRenderResourceManager>(
                r.Create(SCENE_NS::ClassId::RenderResourceManager, obj));
        } else {
            if (!ctx) {
                LOG_E("RenderContextJS::InitRenderManager() ctx is invalid");
            } else {
                LOG_E("RenderContextJS::InitRenderManager() ctx->GetProperty() is invalid");
            }
        }
    }
}
