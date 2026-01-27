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

#include "SceneJS.h"

#include "CameraJS.h"
#include "JsObjectCache.h"
#include "LightJS.h"
#include "MaterialJS.h"
#include "MeshResourceJS.h"
#include "NodeJS.h"
#include "ParamParsing.h"
#include "Promise.h"
#include "RenderContextJS.h"
#include "SamplerJS.h"
#include "nodejstaskqueue.h"
#include "EffectJS.h"
#include "RenderConfigurationJS.h"
static constexpr BASE_NS::Uid IO_QUEUE { "be88e9a0-9cd8-45ab-be48-937953dc258f" };

#include <meta/api/make_callback.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/property/property_events.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_mesh_resource.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_render_context.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_text.h>
#include <scene/interface/resource/intf_render_resource_manager.h>

#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>

#ifdef __SCENE_ADAPTER__
#include <parameters.h>
#include "3d_widget_adapter_log.h"
#include "scene_adapter/scene_adapter.h"
#endif

// LEGACY COMPATIBILITY start
#include <geometry_definition/GeometryDefinition.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_ecs_object_access.h>

#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>

#include "lume_trace.h"
#include "nodejstaskqueue.h"

#include "napi/value.h"

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;

void SceneJS::Init(napi_env env, napi_value exports)
{
    LUME_TRACE_FUNC()
    using namespace NapiApi;
    auto loadFun = [](napi_env e, napi_callback_info cb) -> napi_value {
        FunctionContext<> fc(e, cb);
        return SceneJS::Load(fc);
    };

    auto getDefaultRenderContextFun = [](napi_env e, napi_callback_info cb) -> napi_value {
        FunctionContext<> fc(e, cb);
        return RenderContextJS::GetDefaultContext(e);
    };

    napi_property_descriptor props[] = {
        // static methods
        napi_property_descriptor{"load", nullptr, loadFun, nullptr, nullptr, nullptr,
                                 (napi_property_attributes)(napi_static | napi_default_method)},
        napi_property_descriptor{"getDefaultRenderContext", nullptr, getDefaultRenderContextFun, nullptr, nullptr,
                                 nullptr, (napi_property_attributes)(napi_static | napi_default_method)},
        // properties
        GetSetProperty<uint32_t, SceneJS, &SceneJS::GetRenderMode, &SceneJS::SetRenderMode>("renderMode"),
        GetSetProperty<NapiApi::Object, SceneJS, &SceneJS::GetEnvironment, &SceneJS::SetEnvironment>("environment"),
        GetProperty<NapiApi::Array, SceneJS, &SceneJS::GetAnimations>("animations"),
        // animations
        GetProperty<BASE_NS::string, SceneJS, &SceneJS::GetRoot>("root"),
        // scene methods
        Method<NapiApi::FunctionContext<>, SceneJS, &SceneJS::GetNode>("getNodeByPath"),
        Method<NapiApi::FunctionContext<>, SceneJS, &SceneJS::GetResourceFactory>("getResourceFactory"),
        Method<NapiApi::FunctionContext<>, SceneJS, &SceneJS::Dispose>("destroy"),

        // SceneResourceFactory methods
        Method<NapiApi::FunctionContext<>, SceneJS, &SceneJS::CreateCamera>("createCamera"),
        Method<NapiApi::FunctionContext<NapiApi::Object, uint32_t>, SceneJS, &SceneJS::CreateLight>("createLight"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateNode>("createNode"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateTextNode>("createTextNode"),
        Method<NapiApi::FunctionContext<NapiApi::Object, uint32_t>, SceneJS, &SceneJS::CreateMaterial>(
            "createMaterial"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateShader>("createShader"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateImage>("createImage"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateSampler>("createSampler"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateEnvironment>("createEnvironment"),
        Method<NapiApi::FunctionContext<>, SceneJS, &SceneJS::CreateScene>("createScene"),
        Method<FunctionContext<Object>, SceneJS, &SceneJS::CreateEffect>("createEffect"),

        Method<NapiApi::FunctionContext<BASE_NS::string, NapiApi::Object, NapiApi::Object>, SceneJS,
               &SceneJS::ImportNode>("importNode"),
        Method<NapiApi::FunctionContext<BASE_NS::string, NapiApi::Object, NapiApi::Object>, SceneJS,
               &SceneJS::ImportScene>("importScene"),

        Method<FunctionContext<NapiApi::Object, NapiApi::Object, BASE_NS::string>, SceneJS,
               &SceneJS::CloneNode>("cloneNode"),

        Method<NapiApi::FunctionContext<>, SceneJS, &SceneJS::RenderFrame>("renderFrame"),

        Method<FunctionContext<Object, Object>, SceneJS, &SceneJS::CreateMeshResource>("createMesh"),
        Method<FunctionContext<Object, Object>, SceneJS, &SceneJS::CreateGeometry>("createGeometry"),
        Method<FunctionContext<Object, BASE_NS::string>, SceneJS, &SceneJS::CreateComponent>("createComponent"),
        Method<FunctionContext<Object, BASE_NS::string>, SceneJS, &SceneJS::GetComponent>("getComponent"),
        Method<FunctionContext<>, SceneJS, &SceneJS::GetRenderContext>("getRenderContext"),

        GetProperty<Object, SceneJS, &SceneJS::GetRenderConfiguration>("renderConfiguration"),
    };

    napi_value func;
    auto status = napi_define_class(env, "Scene", NAPI_AUTO_LENGTH, BaseObject::ctor<SceneJS>(), nullptr,
        sizeof(props) / sizeof(props[0]), props, &func);

    napi_set_named_property(env, exports, "Scene", func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, reinterpret_cast<void**>(&mis));
    if (mis) {
        mis->StoreCtor("Scene", func);
    }
}

void SceneJS::RegisterEnums(NapiApi::Object exports)
{
    LUME_TRACE_FUNC()
    napi_value v;
    NapiApi::Object en(exports.GetEnv());

    napi_create_uint32(en.GetEnv(), static_cast<uint32_t>(SCENE_NS::RenderMode::IF_DIRTY), &v);
    en.Set("RENDER_WHEN_DIRTY", v);
    napi_create_uint32(en.GetEnv(), static_cast<uint32_t>(SCENE_NS::RenderMode::ALWAYS), &v);
    en.Set("RENDER_CONTINUOUSLY", v);
    napi_create_uint32(en.GetEnv(), static_cast<uint32_t>(SCENE_NS::RenderMode::MANUAL), &v);
    en.Set("RENDER_MANUALLY", v);

    exports.Set("RenderMode", en);
}

napi_value SceneJS::Load(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    auto renderContext = NapiApi::Object(ctx.GetEnv(), RenderContextJS::GetDefaultContext(ctx.GetEnv()));
    NapiApi::Function f(ctx.GetEnv(), renderContext.Get("createScene"));
    napi_value param = {};

    size_t n = 1;
    auto status = napi_get_cb_info(ctx.GetEnv(), ctx.GetInfo(), &n, &param, nullptr, nullptr);
    if (status != napi_ok) {
        n = 0;
    }
    return renderContext.Invoke("createScene", { n, &param });
}

napi_value SceneJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    DisposeNative(nullptr);
    return {};
}

void FreeResources(META_NS::IObject::Ptr me)
{
    if (auto nativeScene = interface_pointer_cast<SCENE_NS::IScene>(me)) {
        auto r = nativeScene->RenderConfiguration()->GetValue();
        if (r) {
            r->Environment()->SetValue(nullptr);
            r.reset();
        }
        if (auto res = interface_pointer_cast<CORE_NS::IResource>(BASE_NS::move(nativeScene))) {
            auto id = res->GetResourceId();
            if (id.IsValid()) {
                auto& objRegistry = META_NS::GetObjectRegistry();
                auto objContext = interface_pointer_cast<META_NS::IMetadata>(objRegistry.GetDefaultObjectContext());
                if (const auto renderContext =
                        SCENE_NS::GetBuildArg<SCENE_NS::IRenderContext::Ptr>(objContext, "RenderContext")) {
                    if (auto resources = renderContext->GetResources()) {
                        resources->RemoveGroup(id.group);
                    }
                }
            }
        }
    }
}

// Release the js task queue.
void ReleaseJsTaskQueue()
{
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

void SceneJS::DisposeNative(void*)
{
    LUME_TRACE_FUNC()
#ifdef __SCENE_ADAPTER__
    if (scene_) {
        scene_->Deinit();
    }
    scene_.reset();
#endif
    if (!disposed_) {
        disposed_ = true;
        DisposeBridge(this);
        if (auto native = GetNativeObject<META_NS::IObject>()) {
            DetachJsObj(native, "_JSW");
        }
        NapiApi::Object scen(env_);
        napi_value tmp;
        napi_create_external(
            env_, static_cast<void*>(this),
            [](napi_env env, void* data, void* finalize_hint) {
                // do nothing.
            },
            nullptr, &tmp);
        scen.Set("SceneJS", tmp);
        napi_value scene = scen.ToNapiValue();
        // dispose active environment
        if (auto env = environmentJS_.GetObject()) {
            NapiApi::Function func = env.Get<NapiApi::Function>("destroy");
            if (func) {
                func.Invoke(env, { scene });
            }
        }
        environmentJS_.Reset();
        if (strongDisposables_.size() || disposables_.size()) {
            resources_->Dispose(env_, strongDisposables_, disposables_, this);
            strongDisposables_.clear();
            disposables_.clear();
        }
        FreeResources(GetNativeObject());
        UnsetNativeObject();
        FlushScenes();
        ReleaseJsTaskQueue();
        resources_.reset();
        renderContextJS_.Reset();
        if (auto bmjs = renderConfiguration_.GetObject()) {
            bmjs.Invoke("destroy", {});
        }
        renderConfiguration_.Reset();
    }
}
void* SceneJS::GetInstanceImpl(uint32_t id)
{
    LUME_TRACE_FUNC()
    if (id == SceneJS::ID) {
        return static_cast<SceneJS*>(this);
    }
    return BaseObject::GetInstanceImpl(id);
}
void SceneJS::Finalize(napi_env env)
{
    LUME_TRACE_FUNC()
    DisposeNative(nullptr);
    BaseObject::Finalize(env);
    FinalizeBridge(this);
}

napi_value SceneJS::CreateNode(
    const NapiApi::FunctionContext<>& variableCtx, BASE_NS::string_view jsClassName, META_NS::ObjectId classId)
{
    LUME_TRACE_FUNC()
    // Take only the first arg of the variable-length context.
    auto ctx = NapiApi::FunctionContext<NapiApi::Object> { variableCtx.GetEnv(), variableCtx.GetInfo(),
        NapiApi::ArgCount::PARTIAL };
    const auto env = ctx.GetEnv();
    auto promise = Promise(env);
    auto scene = ctx.This().GetNative<SCENE_NS::IScene>();
    if (!scene) {
        return promise.Reject("Invalid scene");
    }
    auto sceneNodeParameters = ctx.Arg<0>();

    auto convertToJs = [promise, jsClassName = BASE_NS::string { jsClassName },
                           sceneRef = NapiApi::StrongRef { ctx.This() },
                           paramRef = NapiApi::StrongRef { sceneNodeParameters }](SCENE_NS::INode::Ptr node) mutable {
        const auto env = sceneRef.GetEnv();
        napi_value args[] = { sceneRef.GetValue(), paramRef.GetValue() };
        const auto result = CreateFromNativeInstance(env, jsClassName, node, PtrType::WEAK, args);
        if (auto node = result.GetJsWrapper<NodeImpl>()) {
            node->Attached(true);
        }
        promise.Resolve(result.ToNapiValue());
    };

    const auto nodePath = ExtractNodePath(sceneNodeParameters);
    const auto jsQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(JS_THREAD_DEP);
    scene->CreateNode(nodePath, classId).Then(BASE_NS::move(convertToJs), jsQ);
    return promise;
}

SceneJS::SceneJS(napi_env e, napi_callback_info i) : BaseObject(e, i)
{
    LUME_TRACE_FUNC()
    LOG_V("SceneJS ++");

    // Acquire the js task queue. (make sure that as long as we have a scene, the nodejstaskqueue is useable)
    if (auto jsQueue = interface_cast<INodeJSTaskQueue>(GetOrCreateNodeTaskQueue(e))) {
        jsQueue->Acquire();
    }

    env_ = e; // store..

    NapiApi::FunctionContext<NapiApi::Object> fromJs(e, i);
    AddBridge("SceneJS",fromJs.This());
    if (fromJs) {
        if (auto obj = fromJs.Arg<0>().valueOrDefault()) {
            if (obj.GetJsWrapper<RenderContextJS>()) {
                renderContextJS_ = NapiApi::StrongRef(obj);
            }
        }
    }

    if (renderContextJS_.IsEmpty()) {
        renderContextJS_ = NapiApi::StrongRef(e, RenderContextJS::GetDefaultContext(e));
    }

    if (renderContextJS_.IsEmpty()) {
        LOG_E("No render context");
    }

    if (auto wrapper = renderContextJS_.GetObject().GetJsWrapper<RenderContextJS>()) {
        resources_ = wrapper->GetResources();
    }

    if (!fromJs) {
        // okay internal create. we will receive the object after.
        return;
    }
    // SceneJS should be created by "Scene.Load" only.
    // so this never happens.
}

void SceneJS::FlushScenes()
{
    LUME_TRACE_FUNC()
    ExecSyncTask([]() {
        auto& obr = META_NS::GetObjectRegistry();
        if (auto params = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext())) {
            if (auto duh = params->GetArrayProperty<IntfWeakPtr>("Scenes")) {
                for (auto i = 0; i < duh->GetSize();) {
                    auto w = duh->GetValueAt(i);
                    if (w.lock() == nullptr) {
                        duh->RemoveAt(i);
                    } else {
                        i++;
                    }
                }
            }
        }
        return META_NS::IAny::Ptr {};
    });
}
SceneJS::~SceneJS()
{
    LUME_TRACE_FUNC()
    LOG_V("SceneJS --");
    DisposeNative(nullptr);
    // flush all null scene objects here too.
    FlushScenes();
    DestroyBridge(this);
    if (!GetNativeObject()) {
        return;
    }
}
napi_value SceneJS::GetNode(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    const auto env = ctx.GetEnv();
    const auto info = ctx.GetInfo();
    NapiApi::Object meJs(ctx.This());

    BASE_NS::string path;
    uint32_t exceptNodeType = 0;
    if (auto newCtx = NapiApi::FunctionContext<BASE_NS::string>{ env, info }) {
        path = newCtx.Arg<0>();
    } else if (auto newCtx = NapiApi::FunctionContext<BASE_NS::string, uint32_t>{ env, info }) {
        path = newCtx.Arg<0>();
        exceptNodeType = newCtx.Arg<1>();
    } else {
        LOG_E("Invalid args given to getNodeByPath");
        return ctx.GetNull();
    }
    auto root = root_.GetNapiObject();
    if (!root) {
        root = meJs.Get<NapiApi::Object>("root");
    }
    // remove the "root nodes name", if given (make sure it also matches though..)
    size_t pos = 0;
    if (path[0] != '/') {
        pos = path.find('/', 0);
        BASE_NS::string_view step = path.substr(0, pos);
        if (!step.empty() && (step != rootName_)) {
            // root not matching
            CORE_LOG_E("Root not matching, path is %s", path.c_str());
            return ctx.GetNull();
        }
    }
    // verify that path starts from "correct root" and then let the root node handle the rest.
    if (pos != BASE_NS::string_view::npos) {
        path = path.substr(pos + 1);
    }
    if (path.empty() || (path == BASE_NS::string_view("/"))) {
        // empty or '/'. so return root
        return root_.GetValue();
    }
    NapiApi::Function func = root.Get<NapiApi::Function>("getNodeByPath");

    if (auto newpath = ctx.GetString(path)) {
        napi_value node = func.Invoke(root, { newpath });
        if (exceptNodeType == 0) {  // the nodeType parameter is not provided
            return node;
        } else {
            NapiApi::Object nodeObj(env, node);
            uint32_t nodeType = nodeObj.Get<uint32_t>("nodeType");
            return nodeType == exceptNodeType ? node : ctx.GetNull();
        }
    }
    CORE_LOG_E("Napi path is null, path is %s", path.c_str());
    return ctx.GetNull();
}
napi_value SceneJS::GetRoot(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    if (root_.GetNapiObject()) {
        return root_.GetValue();
    }

    if (auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject())) {
        SCENE_NS::INode::Ptr root = scene->GetRootNode().GetResult();
        rootPath_ = root->GetPath().GetResult();

        NapiApi::StrongRef sceneRef { ctx.This() };
        if (!sceneRef.GetObject().GetNative<SCENE_NS::IScene>()) {
            LOG_F("INVALID SCENE!");
        }

        NapiApi::Object argJS(ctx.GetEnv());
        napi_value args[] = { sceneRef.GetObject().ToNapiValue(), argJS.ToNapiValue() };
        {
            LUME_TRACE("create root")
            // Store a weak ref, as these are owned by the scene.
            root_ = CreateFromNativeInstance(ctx.GetEnv(), root, PtrType::WEAK, args);
            if (auto node = root_.GetJsWrapper<NodeImpl>()) {
                node->Attached(true);
            }
            rootName_ = root_.GetNapiObject().Get<BASE_NS::string>("name");
            return root_.GetValue();
        }
    }
    return ctx.GetUndefined();
}

napi_value SceneJS::GetEnvironment(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    if (auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject())) {
        SCENE_NS::IEnvironment::Ptr environment;
        auto rc = scene->RenderConfiguration()->GetValue();
        if (rc) {
            environment = rc->Environment()->GetValue();
        }
        if (environment) {
            NapiApi::StrongRef sceneRef { ctx.This() };
            if (!sceneRef.GetObject().GetNative<SCENE_NS::IScene>()) {
                LOG_F("INVALID SCENE!");
            }

            NapiApi::Env env(ctx.Env());
            NapiApi::Object argJS(env);
            napi_value args[] = { sceneRef.GetObject().ToNapiValue(), argJS.ToNapiValue() };

            environmentJS_ = NapiApi::StrongRef(CreateFromNativeInstance(env, environment, PtrType::WEAK, args));
            return environmentJS_.GetValue();
        }
    }
    return ctx.GetNull();
}

void SceneJS::SetEnvironment(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    NapiApi::Object envObj = ctx.Arg<0>();
    if (!envObj) {
        return;
    }
    if (auto currentlySet = environmentJS_.GetObject()) {
        if (envObj.StrictEqual(currentlySet)) { // setting the exactly the same environment. do nothing.
            return;
        }
    }
    environmentJS_ = NapiApi::StrongRef(envObj);

    SCENE_NS::IEnvironment::Ptr environment = envObj.GetNative<SCENE_NS::IEnvironment>();

    if (auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject())) {
        auto rc = scene->RenderConfiguration()->GetValue();
        if (rc) {
            rc->Environment()->SetValue(environment);
        }
    }
}

// resource factory

napi_value SceneJS::GetResourceFactory(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    // just return this. as scene is the factory also.
    return ctx.This().ToNapiValue();
}
NapiApi::Object SceneJS::CreateEnvironment(NapiApi::Object scene, NapiApi::Object argsIn)
{
    LUME_TRACE_FUNC()
    napi_env env = scene.GetEnv();
    napi_value args[] = { scene.ToNapiValue(), argsIn.ToNapiValue() };
    if (auto nativeScene = scene.GetNative<SCENE_NS::IScene>()) {
        if (auto nativeEnv = nativeScene->CreateObject(SCENE_NS::ClassId::Environment).GetResult()) {
            return CreateFromNativeInstance(env, nativeEnv, PtrType::STRONG, args);
        }
    }
    return {};
}

napi_value SceneJS::CreateEnvironment(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    const auto env = ctx.GetEnv();
    auto promise = Promise(env);
    auto res = CreateEnvironment(ctx.This(), ctx.Arg<0>());
    if (res) {
        return promise.Resolve(res.ToNapiValue());
    }
    return promise.Reject("Invalid Scene");
}

napi_value SceneJS::CreateCamera(NapiApi::FunctionContext<>& vCtx)
{
    LUME_TRACE_FUNC()
    const auto env = vCtx.GetEnv();
    const auto info = vCtx.GetInfo();
    auto promise = Promise(env);

    auto sceneNodeParams = NapiApi::Object {};
    auto cameraParams = NapiApi::Object {};
    if (auto ctx = NapiApi::FunctionContext<NapiApi::Object> { env, info }) {
        sceneNodeParams = ctx.Arg<0>();
    } else if (auto ctx = NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> { env, info }) {
        sceneNodeParams = ctx.Arg<0>();
        cameraParams = ctx.Arg<1>();
    } else {
        return promise.Reject("Invalid args given to createCamera");
    }

    const auto sceneJs = vCtx.This();
    const auto scene = sceneJs.GetNative<SCENE_NS::IScene>();
    if (!scene) {
        return promise.Reject("Invalid scene");
    }

    // renderPipeline is an undocumented, deprecated param. It is in use in multiple apps, so support it for now.
    // If the documented param renderingPipeline is supplied in CameraParameters, it will take precedence.
    auto pipeline = uint32_t(SCENE_NS::CameraPipeline::LIGHT_FORWARD);
    if (const auto renderPipelineJs = sceneNodeParams.Get("renderPipeline")) {
        pipeline = NapiApi::Value<uint32_t>(env, renderPipelineJs);
    }

    auto deactivateCamera = [](SCENE_NS::ICamera::Ptr camera) {
        if (camera) {
            camera->SetActive(false);
        }
        return camera;
    };
    auto convertToJs = [promise, pipeline, sceneRef = NapiApi::StrongRef { sceneJs },
                        sceneNodeParamRef = NapiApi::StrongRef { sceneNodeParams },
                        cameraParamRef = NapiApi::StrongRef { cameraParams }](
                        SCENE_NS::ICamera::Ptr camera) mutable {
        if (!camera) {
            promise.Reject("Camera creation failed");
            return;
        }
        const auto env = promise.Env();
        camera->ColorTargetCustomization()->SetValue({SCENE_NS::ColorFormat{BASE_NS::BASE_FORMAT_R16G16B16A16_SFLOAT}});
        camera->RenderingPipeline()->SetValue(SCENE_NS::CameraPipeline(pipeline));
        camera->PostProcess()->SetValue(nullptr);
        napi_value args[] = { sceneRef.GetValue(), sceneNodeParamRef.GetValue() };
        // Store a weak ref, as these are owned by the scene.
        auto result = CreateFromNativeInstance(env, camera, PtrType::WEAK, args);
        if (auto node = result.GetJsWrapper<NodeImpl>()) {
            node->Attached(true);
        }
        if (auto cameraJs = result.GetJsWrapper<CameraJS>()) {
            camera->PostProcess()->SetValue(cameraJs->MakeDefaultPostProcess());
        }
        if (auto cameraParams = cameraParamRef.GetObject()) {
            ApplyCameraParameters(cameraParams, result);
        }
        promise.Resolve(result.ToNapiValue());
    };

    const auto path = ExtractNodePath(sceneNodeParams);
    const auto engineQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
    const auto jsQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(JS_THREAD_DEP);
    // Immediately deactivate camera in the same thread where it was created to avoid a race condition.
    scene->CreateNode<SCENE_NS::ICamera>(path, SCENE_NS::ClassId::CameraNode)
        .Then(BASE_NS::move(deactivateCamera), engineQ)
        .Then(BASE_NS::move(convertToJs), jsQ);
    return promise;
}

void SceneJS::ApplyCameraParameters(NapiApi::Object& params, NapiApi::Object& cameraJs)
{
    LUME_TRACE_FUNC()
    if (const auto value = params.Get<bool>("msaa"); value.IsValid()) {
        cameraJs.Set<bool>("msaa", value);
    }
    if (const auto value = params.Get<uint32_t>("renderingPipeline"); value.IsValid()) {
        cameraJs.Set<uint32_t>("renderingPipeline", value);
    }
    if (const auto value = params.Get<uint32_t>("defaultRenderTargetFormat"); value.IsValid()) {
        cameraJs.Set<uint32_t>("defaultRenderTargetFormat", value);
    }
}

napi_value SceneJS::CreateLight(NapiApi::FunctionContext<NapiApi::Object, uint32_t>& ctx)
{
    LUME_TRACE_FUNC()
    uint32_t lightType = ctx.Arg<1>();
    BASE_NS::string ctorName;
    if (lightType == BaseLight::DIRECTIONAL) {
        ctorName = "DirectionalLight";
    } else if (lightType == BaseLight::POINT) {
        ctorName = "PointLight";
    } else if (lightType == BaseLight::SPOT) {
        ctorName = "SpotLight";
    } else {
        return Promise(ctx.GetEnv()).Reject("Unknown light type given");
    }
    return CreateNode(ctx, ctorName, SCENE_NS::ClassId::LightNode);
}

napi_value SceneJS::CreateNode(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    return CreateNode(ctx, "Node", SCENE_NS::ClassId::Node);
}

napi_value SceneJS::CreateTextNode(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    return CreateNode(ctx, "TextNode", SCENE_NS::ClassId::TextNode);
}

napi_value SceneJS::CreateMaterial(NapiApi::FunctionContext<NapiApi::Object, uint32_t>& ctx)
{
    LUME_TRACE_FUNC()
    const auto env = ctx.GetEnv();
    auto promise = Promise(env);
    uint32_t type = ctx.Arg<1>();
    const auto scene = interface_pointer_cast<SCENE_NS::IScene>(GetNativeObject());
    if (!scene) {
        return promise.Reject("Invalid scene");
    }

    auto params = NapiApi::Object { ctx.Arg<0>() };
    auto uri = params.Get<BASE_NS::string>("uri");
    if (uri.IsDefinedAndNotNull()) {
        return promise.Reject("Material creation from uri is not supported");
    }

    auto convertToJs = [promise, type, sceneRef = NapiApi::StrongRef(ctx.This()),
                           paramRef = NapiApi::StrongRef(ctx.Arg<0>())](SCENE_NS::IMaterial::Ptr material) mutable {
        const auto env = promise.Env();
        if (type == BaseMaterial::SHADER) {
            META_NS::SetValue(material->Type(), SCENE_NS::MaterialType::CUSTOM);
        } else if (type == BaseMaterial::UNLIT) {
            META_NS::SetValue(material->Type(), SCENE_NS::MaterialType::UNLIT);
        }
        napi_value args[] = { sceneRef.GetValue(), paramRef.GetValue() };
        const auto result = CreateFromNativeInstance(env, material, PtrType::STRONG, args);
        promise.Resolve(result.ToNapiValue());
    };

    const auto jsQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(JS_THREAD_DEP);
    const auto classId = type == BaseMaterial::MaterialType::OCCLUSION ? SCENE_NS::ClassId::OcclusionMaterial
                                                                       : SCENE_NS::ClassId::Material;
    scene->CreateObject<SCENE_NS::IMaterial>(classId).Then(BASE_NS::move(convertToJs), jsQ);
    return promise;
}

napi_value SceneJS::CreateScene(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    return Load(ctx);
}

napi_value SceneJS::ImportNode(NapiApi::FunctionContext<BASE_NS::string, NapiApi::Object, NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    BASE_NS::string name = ctx.Arg<0>();
    NapiApi::Object nnode = ctx.Arg<1>();
    NapiApi::Object nparent = ctx.Arg<2>();
    auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject());
    if (!nnode || !scene) {
        return ctx.GetNull();
    }

    SCENE_NS::INode::Ptr node = nnode.GetNative<SCENE_NS::INode>();
    SCENE_NS::INode::Ptr parent;
    if (nparent) {
        parent = nparent.GetNative<SCENE_NS::INode>();
    } else {
        parent = scene->GetRootNode().GetResult();
    }

    if (auto import = interface_cast<SCENE_NS::INodeImport>(parent)) {
        auto importedNode = import->ImportChild(node).GetResult();
        if (!name.empty()) {
            if (auto named = interface_cast<META_NS::INamed>(importedNode)) {
                named->Name()->SetValue(name);
            }
        }

        NapiApi::StrongRef sceneRef { ctx.This() };
        NapiApi::Object argJS(ctx.GetEnv());
        napi_value args[] = { sceneRef.GetObject().ToNapiValue(), argJS.ToNapiValue() };
        // Store a weak ref, as these are owned by the scene.
        auto jsNode = CreateFromNativeInstance(ctx.GetEnv(), importedNode, PtrType::WEAK, args);
        if (auto jsNodeWrapper = jsNode.GetJsWrapper<NodeImpl>()) {
            jsNodeWrapper->Attached(true);
        }
        return jsNode.ToNapiValue();
    }
    return ctx.GetNull();
}

napi_value SceneJS::ImportScene(NapiApi::FunctionContext<BASE_NS::string, NapiApi::Object, NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    BASE_NS::string name = ctx.Arg<0>();
    NapiApi::Object nextScene = ctx.Arg<1>();
    NapiApi::Object nparent = ctx.Arg<2>();
    auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject());
    if (!nextScene || !scene) {
        CORE_LOG_E("SceneJS::ImportScene %s nextScene or scene is null: nextScene[%s], scene[%s]",
            name.c_str(), nextScene ? "true" : "false", scene ? "true" : "false");
        return ctx.GetNull();
    }

    SCENE_NS::IScene::Ptr extScene = nextScene.GetNative<SCENE_NS::IScene>();
    SCENE_NS::INode::Ptr parent;
    if (nparent) {
        parent = nparent.GetNative<SCENE_NS::INode>();
    } else {
        parent = scene->GetRootNode().GetResult();
    }

    if (auto import = interface_cast<SCENE_NS::INodeImport>(parent)) {
        auto result = import->ImportChildScene(extScene, name).GetResult();

        NapiApi::StrongRef sceneRef { ctx.This() };
        NapiApi::Object argJS(ctx.GetEnv());
        napi_value args[] = { sceneRef.GetObject().ToNapiValue(), argJS.ToNapiValue() };
        // Store a weak ref, as these are owned by the scene.
        auto res = CreateFromNativeInstance(ctx.GetEnv(), result, PtrType::WEAK, args);
        if (auto node = res.GetJsWrapper<NodeImpl>()) {
            node->Attached(true);
        }
        return res.ToNapiValue();
    }
    CORE_LOG_E("SceneJS::ImportScene %s import is null", name.c_str());
    return ctx.GetNull();
}

napi_value SceneJS::GetRenderMode(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject());
    if (!scene) {
        return ctx.GetUndefined();
    }
    return ctx.GetNumber(uint32_t(scene->GetRenderMode().GetResult()));
}
void SceneJS::SetRenderMode(NapiApi::FunctionContext<uint32_t>& ctx)
{
    LUME_TRACE_FUNC()
    auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject());
    if (!scene) {
        return;
    }
    uint32_t v = ctx.Arg<0>();
    if (v >= static_cast<uint32_t>(SCENE_NS::RenderMode::IF_DIRTY) &&
        v <= static_cast<uint32_t>(SCENE_NS::RenderMode::MANUAL)) {
        scene->SetRenderMode(static_cast<SCENE_NS::RenderMode>(v)).Wait();
    }
}

napi_value SceneJS::RenderFrame(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    if (ctx.ArgCount() > 1) {
        CORE_LOG_E("render frame %d", __LINE__);
        return ctx.GetBoolean(false);
    }

#ifdef __SCENE_ADAPTER__
    auto sceneAdapter = std::static_pointer_cast<OHOS::Render3D::SceneAdapter>(scene_);
    if (sceneAdapter) {
        auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject());
        if (!scene) {
            return ctx.GetBoolean(false);
        }

        // set RenderMode based on Arg<0>.alwaysRender
        NapiApi::FunctionContext<NapiApi::Object> paramsCtx(ctx);
        NapiApi::Object params = paramsCtx.Arg<0>();
        bool alwaysRender = params.IsUndefinedOrNull("alwaysRender") ? true : params.Get<bool>("alwaysRender");
        if (currentAlwaysRender_ != alwaysRender) {
            currentAlwaysRender_ = alwaysRender;
            scene->SetRenderMode(static_cast<SCENE_NS::RenderMode>(alwaysRender));
        }

        sceneAdapter->SetNeedsRepaint(false);
        sceneAdapter->RenderFrame(false);
    } else {
        return ctx.GetBoolean(false);
    }
#else
    auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject());
    if (!scene) {
        return ctx.GetBoolean(false);
    }
    // todo: fix this
    // NapiApi::Object params = ctx.Arg<0>();
    // auto alwaysRender = params ? params.Get<bool>("alwaysRender") : true;
    if (auto sc = scene->GetInternalScene()) {
        sc->RenderFrame();
    }
#endif
    return ctx.GetBoolean(true);
}

napi_value SceneJS::CreateComponent(NapiApi::FunctionContext<NapiApi::Object, BASE_NS::string>& ctx)
{
    LUME_TRACE_FUNC()
    auto env = ctx.GetEnv();
    auto promise = Promise(env);
    if (ctx.ArgCount() > 2) { // 2: arg num
        return promise.Reject("Invalid number of arguments");
    }
    NapiApi::Object nnode = ctx.Arg<0>();
    BASE_NS::string name = ctx.Arg<1>();
    auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject());
    if (!scene || !nnode || name.empty()) {
        return promise.Reject("Invalid parameters given");
    }
    auto node = nnode.GetNative<SCENE_NS::INode>();
    if (auto comp = scene->CreateComponent(node, name).GetResult()) {
        NapiApi::Env env(ctx.GetEnv());
        NapiApi::Object argJS(env);
        NapiApi::WeakRef sceneRef { ctx.This() };
        napi_value args[] = { sceneRef.GetValue(), argJS.ToNapiValue() };
        return promise.Resolve(
            CreateFromNativeInstance(env, "SceneComponent", comp, PtrType::WEAK, args).ToNapiValue());
    }
    return promise.Reject("Could not create component");
}

napi_value SceneJS::GetComponent(NapiApi::FunctionContext<NapiApi::Object, BASE_NS::string>& ctx)
{
    LUME_TRACE_FUNC()
    if (ctx.ArgCount() > 2) { // 2: arg num
        return ctx.GetNull();
    }
    NapiApi::Object nnode = ctx.Arg<0>();
    BASE_NS::string name = ctx.Arg<1>();
    if (!nnode) {
        return ctx.GetNull();
    }
    if (auto attach = nnode.GetNative<META_NS::IAttach>()) {
        if (auto cont = attach->GetAttachmentContainer(false)) {
            if (auto comp = cont->FindByName<SCENE_NS::IComponent>(name)) {
                NapiApi::Env env(ctx.GetEnv());
                NapiApi::Object argJS(env);
                NapiApi::WeakRef sceneRef { ctx.This() };
                napi_value args[] = { sceneRef.GetValue(), argJS.ToNapiValue() };
                return CreateFromNativeInstance(env, "SceneComponent", comp, PtrType::WEAK, args).ToNapiValue();
            }
        }
    }
    return ctx.GetNull();
}

napi_value SceneJS::GetRenderContext(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    return renderContextJS_.GetValue();
}

napi_value SceneJS::GetRenderConfiguration(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC() 

    if (!renderConfiguration_.IsEmpty()) {
        return renderConfiguration_.GetValue();
    }

    BASE_NS::unique_ptr<RenderConfiguration> rc;
    auto scene = interface_pointer_cast<SCENE_NS::IScene>(GetNativeObject());
    if (!scene) {
        return ctx.GetUndefined();
    }

    if (auto ptr = META_NS::GetValue(scene->RenderConfiguration())) {
        rc = BASE_NS::make_unique<RenderConfiguration>();
        rc->SetFrom(ctx.GetEnv(), BASE_NS::move(ptr));
    }

    NapiApi::Env env(ctx.GetEnv());
    NapiApi::Object obj(env);
    renderConfiguration_ = BASE_NS::move(rc->Wrap(obj));
    rc.release(); // renderConfiguration_ owns the ptr now
    return renderConfiguration_.GetValue();
}

napi_value SceneJS::CreateMeshResource(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    if (auto renderContext = renderContextJS_.GetObject()) {
        napi_value params[] = { ctx.Arg<0>().ToNapiValue(), ctx.Arg<1>().ToNapiValue() };
        return renderContext.Invoke("createMesh", params);
    }

    auto promise = Promise(ctx.GetEnv());
    return promise.Reject("No render context to create a mesh.");
}

napi_value SceneJS::CreateGeometry(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    const auto env = ctx.GetEnv();
    auto promise = Promise(env);

    const auto sceneJs = ctx.This();
    auto params = NapiApi::Object { ctx.Arg<0>() };
    auto meshRes = NapiApi::Object { ctx.Arg<1>() };
    auto tro = meshRes.GetRoot();
    if (!tro) {
        return promise.Reject("Invalid mesh resource given");
    }
    const auto meshResourceJs = static_cast<MeshResourceJS*>(tro->GetInstanceImpl(MeshResourceJS::ID));
    if (!meshResourceJs) {
        return promise.Reject("Invalid mesh resource given");
    }

    // We don't use futures here, but rather just GetResult everything immediately.
    // Otherwise there's a race condition for a deadlock.
    auto scene = sceneJs.GetNative<SCENE_NS::IScene>();
    if (!scene) {
        return promise.Reject("Invalid scene");
    }
    const auto path = ExtractNodePath(params);
    auto meshNode = scene->CreateNode(path, SCENE_NS::ClassId::MeshNode).GetResult();
    if (auto access = interface_pointer_cast<SCENE_NS::IMeshAccess>(meshNode)) {
        const auto mesh = meshResourceJs->CreateMesh(scene);
        access->SetMesh(mesh).GetResult();
        napi_value args[] = { sceneJs.ToNapiValue(), params.ToNapiValue() };
        // Store a weak ref, as these are owned by the scene.
        const auto result = CreateFromNativeInstance(env, meshNode, PtrType::WEAK, args);
        if (auto node = result.GetJsWrapper<NodeImpl>()) {
            node->Attached(true);
        }
        return promise.Resolve(result.ToNapiValue());
    }
    return promise.Reject("Geometry node creation failed. Is the given node path unique and valid?");
}

napi_value SceneJS::CreateShader(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    if (auto renderContext = renderContextJS_.GetObject()) {
        return renderContext.Invoke("createShader", ctx.Arg<0>().ToNapiValue());
    }

    auto promise = Promise(ctx.GetEnv());
    return promise.Reject("No render context to create a shader.");
}

// To pass LoadResult between tasks with Future::Then.
META_TYPE(BASE_NS::shared_ptr<CORE_NS::IImageLoaderManager::LoadResult>);

napi_value SceneJS::CreateImage(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    if (auto renderContext = renderContextJS_.GetObject()) {
        return renderContext.Invoke("createImage", ctx.Arg<0>().ToNapiValue());
    }
    auto promise = Promise(ctx.GetEnv());
    return promise.Reject("No render context to create an image.");
}

napi_value SceneJS::CreateSampler(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    return Promise(ctx.GetEnv()).Resolve(SamplerJS::CreateRawJsObject(ctx.GetEnv()));
}

napi_value SceneJS::CreateEffect(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    const auto env = ctx.GetEnv();
    auto promise = Promise(env);
    const auto sceneJs = ctx.This();
    auto scene = sceneJs.GetNative<SCENE_NS::IScene>();
    if (!scene) {
        return promise.Reject("Invalid scene");
    }

    auto params = NapiApi::Object { ctx.Arg<0>() };
    BASE_NS::string id = params.Get<BASE_NS::string>("effectId");
    if (!BASE_NS::IsUidString(id)) {
        return promise.Reject("Invalid effect id");
    }
    if (auto effectClassId = BASE_NS::StringToUid(id); effectClassId != BASE_NS::Uid {}) {
        const auto effect = EffectJS::CreateEffectInstance();
        if (!(effect && effect->InitializeEffect(scene, effectClassId).GetResult())) {
            return promise.Reject("Failed to instantiate Effect");
        }
        NapiApi::Object argJS(env);
        NapiApi::WeakRef sceneRef { ctx.This() };
        napi_value args[] = { sceneRef.GetValue(), argJS.ToNapiValue() };
        // Strong ref as nobody else owns the effect object at this point 
        const auto result = CreateFromNativeInstance(env, "Effect", effect, PtrType::STRONG, args);
        return promise.Resolve(result.ToNapiValue());
    }
    return promise.Reject("Geometry node creation failed. Is the given node path unique and valid?");
}

napi_value SceneJS::GetAnimations(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    auto scene = ctx.This().GetNative<SCENE_NS::IScene>();
    if (!scene) {
        return ctx.GetUndefined();
    }

    BASE_NS::vector<META_NS::IAnimation::Ptr> animRes;
    ExecSyncTask([scene, &animRes]() {
        animRes = scene->GetAnimations().GetResult();
        return META_NS::IAny::Ptr {};
    });

    napi_value tmp;
    auto status = napi_create_array_with_length(ctx.Env(), animRes.size(), &tmp);
    size_t i = 0;
    napi_value args[] = { ctx.This().ToNapiValue(), NapiApi::Object(ctx.Env()).ToNapiValue() };
    for (const auto& node : animRes) {
        auto val = CreateFromNativeInstance(ctx.Env(), node, PtrType::STRONG, args);
        status = napi_set_element(ctx.Env(), tmp, i++, val.ToNapiValue());
    }

    return tmp;
}

napi_value SceneJS::CloneNode(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object, BASE_NS::string>& ctx)
{
    auto scene = ctx.This().GetNative<SCENE_NS::IScene>();
    if (!scene) {
        return ctx.GetUndefined();
    }

    auto arg0 = ctx.Arg<0>();
    auto arg1 = ctx.Arg<1>();
    BASE_NS::string name = ctx.Arg<2>();
    if (arg1.IsUndefinedOrNull()) {
        // okay. Invalid arg error?
        return ctx.GetUndefined();
    }
    NapiApi::Object nobj = arg0;
    NapiApi::Object pnobj = arg1;

    if(auto parentNode = pnobj.GetNative<SCENE_NS::INode>()) {
        if (auto n  = nobj.GetNative<SCENE_NS::INode>()) {
            auto node = n->Clone(name, parentNode).GetResult();
            if (!node) {
                return ctx.GetNull();
            }
            const auto env = ctx.GetEnv();
            auto argJS = NapiApi::Object { env };
            napi_value args[] = { ctx.This().ToNapiValue(), argJS.ToNapiValue() };
            // These are owned by the scene, so store only weak reference.
            auto js = CreateFromNativeInstance(env, node, PtrType::WEAK, args);
            if (js.GetJsWrapper<NodeImpl>()) {
                js.GetJsWrapper<NodeImpl>()->Attached(true);
                return js.ToNapiValue();
            }
        }
    }
    return ctx.GetNull();
}

void SceneJS::DisposeHook(uintptr_t token, NapiApi::Object obj)
{
    LUME_TRACE_FUNC()
    disposables_.push_back(token);
    resources_->DisposeHook(token, obj);
}
void SceneJS::ReleaseDispose(uintptr_t token)
{
    LUME_TRACE_FUNC()
    for (auto ite = disposables_.cbegin();
        ite != disposables_.cend(); ite++ ) {
        if (*ite == token) {
            resources_->ReleaseDispose(token);
            disposables_.erase(ite);
            return;
        }
    }
}

void SceneJS::StrongDisposeHook(uintptr_t token, NapiApi::Object obj)
{
    LUME_TRACE_FUNC()
    strongDisposables_.push_back(token);
    resources_->StrongDisposeHook(token, obj);
}
void SceneJS::ReleaseStrongDispose(uintptr_t token)
{
    LUME_TRACE_FUNC()
    for (auto ite = strongDisposables_.cbegin();
        ite != strongDisposables_.cend(); ite++ ) {
        if (*ite == token) {
            resources_->ReleaseStrongDispose(token);
            strongDisposables_.erase(ite);
            return;
        }
    }
}
