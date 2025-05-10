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

#include "JsObjectCache.h"
#include "LightJS.h"
#include "MaterialJS.h"
#include "MeshResourceJS.h"
#include "NodeJS.h"
#include "ParamParsing.h"
#include "Promise.h"
#include "RenderContextJS.h"
#include "nodejstaskqueue.h"
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

#include "nodejstaskqueue.h"

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
        rr(SCENE_NS::IScene::Ptr scene)
        {
            CORE_NS::IEcs::Ptr ecs = scene->GetInternalScene()->GetEcsContext().GetNativeEcs();
            cm = CORE_NS::GetManager<CORE3D_NS::INodeComponentManager>(*ecs);
            nm = CORE_NS::GetManager<CORE3D_NS::INameComponentManager>(*ecs);
            fix();
        }
        void scan()
        {
            const auto count = cm->GetComponentCount();
            // collect nodes and their children.
            tree.reserve(cm->GetComponentCount());
            for (auto i = 0; i < count; i++) {
                auto enti = cm->GetEntity(i);
                // add node to our list. (if not yet added)
                tree.insert({ enti, {} });
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
                /*// force root node name to match legacy by default.
                CORE3D_NS::NameComponent c;
                c.name = "rootNode_";
                nm->Set(i, c);*/
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

void SceneJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;
    // clang-format off
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
        napi_property_descriptor{ "load", nullptr, loadFun, nullptr, nullptr, nullptr,
            (napi_property_attributes)(napi_static|napi_default_method)},
        napi_property_descriptor{ "getDefaultRenderContext", nullptr, getDefaultRenderContextFun,
            nullptr, nullptr, nullptr, (napi_property_attributes)(napi_static|napi_default_method)},
        // properties
        GetSetProperty<uint32_t, SceneJS, &SceneJS::GetRenderMode, &SceneJS::SetRenderMode>("renderMode"),
        GetSetProperty<NapiApi::Object, SceneJS, &SceneJS::GetEnvironment, &SceneJS::SetEnvironment>("environment"),
        GetProperty<NapiApi::Array, SceneJS, &SceneJS::GetAnimations>("animations"),
        // animations
        GetProperty<BASE_NS::string, SceneJS, &SceneJS::GetRoot>("root"),
        // scene methods
        Method<NapiApi::FunctionContext<BASE_NS::string>, SceneJS, &SceneJS::GetNode>("getNodeByPath"),
        Method<NapiApi::FunctionContext<>, SceneJS, &SceneJS::GetResourceFactory>("getResourceFactory"),
        Method<NapiApi::FunctionContext<>, SceneJS, &SceneJS::Dispose>("destroy"),

        // SceneResourceFactory methods
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateCamera>("createCamera"),
        Method<NapiApi::FunctionContext<NapiApi::Object, uint32_t>, SceneJS, &SceneJS::CreateLight>("createLight"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateNode>("createNode"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateTextNode>("createTextNode"),
        Method<NapiApi::FunctionContext<NapiApi::Object, uint32_t>,
            SceneJS, &SceneJS::CreateMaterial>("createMaterial"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateShader>("createShader"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateImage>("createImage"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateEnvironment>("createEnvironment"),
        Method<NapiApi::FunctionContext<>, SceneJS, &SceneJS::CreateScene>("createScene"),

        Method<NapiApi::FunctionContext<BASE_NS::string, NapiApi::Object, NapiApi::Object>, SceneJS, &SceneJS::ImportNode>("importNode"),
        Method<NapiApi::FunctionContext<BASE_NS::string, NapiApi::Object, NapiApi::Object>, SceneJS, &SceneJS::ImportScene>("importScene"),

        Method<NapiApi::FunctionContext<>, SceneJS, &SceneJS::RenderFrame>("renderFrame"),

        Method<FunctionContext<Object, Object>, SceneJS, &SceneJS::CreateMeshResource>("createMesh"),
        Method<FunctionContext<Object, Object>, SceneJS, &SceneJS::CreateGeometry>("createGeometry"),
        Method<FunctionContext<Object, BASE_NS::string>, SceneJS, &SceneJS::CreateComponent>("createComponent"),
        Method<FunctionContext<Object, BASE_NS::string>, SceneJS, &SceneJS::GetComponent>("getComponent"),
        Method<FunctionContext<>, SceneJS, &SceneJS::GetRenderContext>("getRenderContext"),
    };
    // clang-format on

    napi_value func;
    auto status = napi_define_class(env, "Scene", NAPI_AUTO_LENGTH, BaseObject::ctor<SceneJS>(), nullptr,
        sizeof(props) / sizeof(props[0]), props, &func);

    napi_set_named_property(env, exports, "Scene", func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, reinterpret_cast<void**>(&mis));
    mis->StoreCtor("Scene", func);
}

void SceneJS::RegisterEnums(NapiApi::Object exports)
{
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
    const auto env = ctx.Env();
    auto promise = Promise(env);

    // A SceneJS instance keeps a NodeJS task queue acquired, but we're in a static method creating a SceneJS.
    // Acquire the JS queue before invoking the JS task. Release it only after the scene is created (in the JS task).
    const auto jsQ = GetOrCreateNodeTaskQueue(env);
    auto queueRefCount = interface_cast<INodeJSTaskQueue>(jsQ);
    if (queueRefCount) {
        queueRefCount->Acquire();
    } else {
        return promise.Reject("Error creating a JS task queue");
    }

    BASE_NS::string uri = ExtractUri(ctx);
    if (uri.empty()) {
        uri = "scene://empty";
    }
    // make sure slashes are correct.. *eh*
    for (;;) {
        auto t = uri.find_first_of('\\');
        if (t == BASE_NS::string::npos) {
            break;
        }
        uri[t] = '/';
    }

    auto massageScene = [](SCENE_NS::IScene::Ptr scene) -> SCENE_NS::IScene::Ptr {
        if (!scene || !scene->RenderConfiguration()->GetValue()) {
            return {};
        }

        // Make sure there's a valid root node
        scene->GetInternalScene()->GetEcsContext().CreateUnnamedRootNode();

        // LEGACY COMPATIBILITY start
        Fixnames(scene);
        // LEGACY COMPATIBILITY end
        auto& obr = META_NS::GetObjectRegistry();
        AddScene(&obr, scene);
        return scene;
    };

    auto convertToJs = [promise, queueRefCount = BASE_NS::move(queueRefCount)](SCENE_NS::IScene::Ptr scene) mutable {
        if (!scene) {
            promise.Reject("Scene creation failed");
            return;
        }

        const auto env = promise.Env();
        auto jsscene = CreateFromNativeInstance(env, scene, PtrType::STRONG, {});
        const auto sceneJs = jsscene.GetJsWrapper<SceneJS>();
        sceneJs->renderMan_ =
            scene->CreateObject<SCENE_NS::IRenderResourceManager>(SCENE_NS::ClassId::RenderResourceManager).GetResult();

        auto curenv = jsscene.Get<NapiApi::Object>("environment");
        if (curenv.IsUndefinedOrNull()) {
            // setup default env
            NapiApi::Object argsIn(env);
            argsIn.Set("name", "DefaultEnv");

            auto res = sceneJs->CreateEnvironment(jsscene, argsIn);
            res.Set("backgroundType", NapiApi::Value<uint32_t>(env, 1)); // image.. but with null.
            jsscene.Set("environment", res);
        }
        for (auto&& c : scene->GetCameras().GetResult()) {
            c->RenderingPipeline()->SetValue(SCENE_NS::CameraPipeline::FORWARD);
            c->ColorTargetCustomization()->SetValue(
                { SCENE_NS::ColorFormat { BASE_NS::BASE_FORMAT_R16G16B16A16_SFLOAT } });
        }

        const auto result = jsscene.ToNapiValue();

#ifdef __SCENE_ADAPTER__
        // set SceneAdapter
        auto sceneAdapter = std::make_shared<OHOS::Render3D::SceneAdapter>();
        sceneAdapter->SetSceneObj(interface_pointer_cast<META_NS::IObject>(scene));
        sceneJs->scene_ = sceneAdapter;
#endif
        promise.Resolve(result);
        queueRefCount->Release();
    };

    auto sceneManager = CreateSceneManager(uri);
    if (!sceneManager) {
        return promise.Reject("Creating scene manager failed");
    }

    auto engineQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
    /* REMOVED DUE TO SCENE API CHANGE
    if (uri.ends_with(".scene2")) {
        const auto scene = SCENE_NS::LoadScene(sceneManager->GetContext(), uri);
        META_NS::AddFutureTaskOrRunDirectly(engineQ, [=]() {
            return massageScene(scene);
        }).Then(BASE_NS::move(convertToJs), jsQ);
    } else {*/
        sceneManager->CreateScene(uri).Then(BASE_NS::move(massageScene), engineQ).Then(BASE_NS::move(convertToJs), jsQ);
    //}
    return promise;
}

SCENE_NS::ISceneManager::Ptr SceneJS::CreateSceneManager(BASE_NS::string_view uri)
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

BASE_NS::string_view SceneJS::ExtractPathToProject(BASE_NS::string_view uri)
{
    // Assume the scene file is in a folder that is at the root of the project.
    // ExtractPathToProject("schema://path/to/PROJECT/assets/default.scene2") == "schema://path/to/PROJECT"
    const auto secondToLastSlashPos = uri.find_last_of('/', uri.find_last_of('/') - 1);
    return uri.substr(0, secondToLastSlashPos);
}

napi_value SceneJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    DisposeNative(nullptr);
#ifdef __SCENE_ADAPTER__
    if (scene_) {
        scene_->Deinit();
    }
#endif
    return {};
}
void SceneJS::DisposeNative(void*)
{
    if (!disposed_) {
        disposed_ = true;
        LOG_V("SCENE_JS::DisposeNative");

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
                func.Invoke(env, 1, &scene);
            }
        }
        environmentJS_.Reset();

        // dispose all cameras/env/etcs.
        while (!strongDisposables_.empty()) {
            auto it = strongDisposables_.begin();
            auto token = it->first;
            auto env = it->second.GetObject();
            if (env) {
                auto size = strongDisposables_.size();
                NapiApi::Function func = env.Get<NapiApi::Function>("destroy");
                if (func) {
                    func.Invoke(env, 1, &scene);
                }

                if (size == strongDisposables_.size()) {
                    LOG_E("Dispose function didn't dispose.");
                    strongDisposables_.erase(strongDisposables_.begin());
                }
            } else {
                strongDisposables_.erase(strongDisposables_.begin());
            }
        }

        // dispose
        while (!disposables_.empty()) {
            auto env = disposables_.begin()->second.GetObject();
            if (env) {
                NapiApi::Function func = env.Get<NapiApi::Function>("destroy");
                if (func) {
                    func.Invoke(env, 1, &scene);
                }
            } else {
                disposables_.erase(disposables_.begin());
            }
        }

        if (auto nativeScene = interface_pointer_cast<SCENE_NS::IScene>(GetNativeObject())) {
            UnsetNativeObject();

            auto r = nativeScene->RenderConfiguration()->GetValue();
            if (r) {
                r->Environment()->SetValue(nullptr);
                r.reset();
            }
        }
        FlushScenes();

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

        resources_.reset();
        renderContextJS_.Reset();
    }
}
void* SceneJS::GetInstanceImpl(uint32_t id)
{
    if (id == SceneJS::ID)
        return this;
    return nullptr;
}
void SceneJS::Finalize(napi_env env)
{
    DisposeNative(nullptr);
    BaseObject::Finalize(env);
}

napi_value SceneJS::CreateNode(
    const NapiApi::FunctionContext<>& variableCtx, BASE_NS::string_view jsClassName, META_NS::ObjectId classId)
{
    // Take only the first arg of the variable-length context.
    auto ctx = NapiApi::FunctionContext<NapiApi::Object> { variableCtx.GetEnv(), variableCtx.GetInfo(),
        NapiApi::ArgCount::PARTIAL };
    const auto env = ctx.GetEnv();
    auto promise = Promise(env);
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
    auto scene = ctx.This().GetNative<SCENE_NS::IScene>();
    const auto jsQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(JS_THREAD_DEP);
    scene->CreateNode(nodePath, classId).Then(BASE_NS::move(convertToJs), jsQ);
    return promise;
}

void SceneJS::AddScene(META_NS::IObjectRegistry* obr, SCENE_NS::IScene::Ptr scene)
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

SceneJS::SceneJS(napi_env e, napi_callback_info i) : BaseObject(e, i)
{
    LOG_V("SceneJS ++");

    // Acquire the js task queue. (make sure that as long as we have a scene, the nodejstaskqueue is useable)
    if (auto jsQueue = interface_cast<INodeJSTaskQueue>(GetOrCreateNodeTaskQueue(e))) {
        jsQueue->Acquire();
    }

    env_ = e; // store..

    NapiApi::FunctionContext<NapiApi::Object> fromJs(e, i);
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
    LOG_V("SceneJS --");
    DisposeNative(nullptr);
    // flush all null scene objects here too.
    FlushScenes();
    if (!GetNativeObject()) {
        return;
    }
}

napi_value SceneJS::GetNode(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    // verify that path starts from "correct root" and then let the root node handle the rest.
    NapiApi::Object meJs(ctx.This());
    NapiApi::Object root = meJs.Get<NapiApi::Object>("root");
    BASE_NS::string rootName = root.Get<BASE_NS::string>("name");
    NapiApi::Function func = root.Get<NapiApi::Function>("getNodeByPath");
    BASE_NS::string path = ctx.Arg<0>();
    if (path.empty() || (path == BASE_NS::string_view("/")) || (path == rootName)) {
        // empty or '/' or "exact rootnodename". so return root
        return root.ToNapiValue();
    }

    // remove the "root nodes name", if given (make sure it also matches though..)
    auto pos = 0;
    if (path[0] != '/') {
        pos = path.find('/', 0);
        BASE_NS::string_view step = path.substr(0, pos);
        if (!step.empty() && (step != rootName)) {
            // root not matching
            return ctx.GetNull();
        }
    }
    if (pos != BASE_NS::string_view::npos) {
        path = path.substr(pos + 1);
    }

    if (path.empty()) {
        // after removing the root node name
        // nothing left in path, so return root.
        return root.ToNapiValue();
    }

    napi_value newpath = ctx.GetString(path);
    if (newpath) {
        return func.Invoke(root, 1, &newpath);
    }
    return ctx.GetNull();
}
napi_value SceneJS::GetRoot(NapiApi::FunctionContext<>& ctx)
{
    if (auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject())) {
        SCENE_NS::INode::Ptr root = scene->GetRootNode().GetResult();

        NapiApi::StrongRef sceneRef { ctx.This() };
        if (!sceneRef.GetObject().GetNative<SCENE_NS::IScene>()) {
            LOG_F("INVALID SCENE!");
        }

        NapiApi::Object argJS(ctx.GetEnv());
        napi_value args[] = { sceneRef.GetObject().ToNapiValue(), argJS.ToNapiValue() };

        // Store a weak ref, as these are owned by the scene.
        auto js = CreateFromNativeInstance(ctx.GetEnv(), root, PtrType::WEAK, args);
        if (auto node = js.GetJsWrapper<NodeImpl>()) {
            node->Attached(true);
        }
        return js.ToNapiValue();
    }
    return ctx.GetUndefined();
}

napi_value SceneJS::GetEnvironment(NapiApi::FunctionContext<>& ctx)
{
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
    // just return this. as scene is the factory also.
    return ctx.This().ToNapiValue();
}
NapiApi::Object SceneJS::CreateEnvironment(NapiApi::Object scene, NapiApi::Object argsIn)
{
    napi_env env = scene.GetEnv();
    napi_value args[] = { scene.ToNapiValue(), argsIn.ToNapiValue() };
    auto nativeScene = scene.GetNative<SCENE_NS::IScene>();
    auto nativeEnv = nativeScene->CreateObject(SCENE_NS::ClassId::Environment).GetResult();
    return CreateFromNativeInstance(env, nativeEnv, PtrType::STRONG, args);
}

napi_value SceneJS::CreateEnvironment(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    const auto env = ctx.GetEnv();
    napi_value args[] = { ctx.This().ToNapiValue(), ctx.Arg<0>().ToNapiValue() };
    return Promise(env).Resolve(CreateEnvironment(ctx.This(), ctx.Arg<0>()).ToNapiValue());
}

napi_value SceneJS::CreateCamera(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    const auto env = ctx.GetEnv();
    auto promise = Promise(env);

    const auto sceneJs = ctx.This();
    auto scene = sceneJs.GetNative<SCENE_NS::IScene>();

    auto params = NapiApi::Object { ctx.Arg<0>() };
    const auto path = ExtractNodePath(params);

    // TODO: renderPipeline is (at the moment of writing) an undocumented param. Check the API docs and usage.
    // Remove this TODO, if it has been added to the API. Else if it's not used anywhere, remove the implementation.
    auto pipeline = uint32_t(SCENE_NS::CameraPipeline::LIGHT_FORWARD);
    if (const auto renderPipelineJs = params.Get("renderPipeline")) {
        pipeline = NapiApi::Value<uint32_t>(env, renderPipelineJs);
    }

    // Don't create the camera asynchronously. There's a race condition, and we need to deactivate it immediately.
    // Otherwise we get tons of render validation issues.
    const auto camera = scene->CreateNode<SCENE_NS::ICamera>(path, SCENE_NS::ClassId::CameraNode).GetResult();
    camera->SetActive(false);
    camera->RenderingPipeline()->SetValue(SCENE_NS::CameraPipeline(pipeline));
    napi_value args[] = { sceneJs.ToNapiValue(), params.ToNapiValue() };
    // Store a weak ref, as these are owned by the scene.
    const auto result = CreateFromNativeInstance(env, camera, PtrType::WEAK, args);
    if (auto node = result.GetJsWrapper<NodeImpl>()) {
        node->Attached(true);
    }
    return promise.Resolve(result.ToNapiValue());
}

napi_value SceneJS::CreateLight(NapiApi::FunctionContext<NapiApi::Object, uint32_t>& ctx)
{
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
    return CreateNode(ctx, "Node", SCENE_NS::ClassId::Node);
}

napi_value SceneJS::CreateTextNode(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    return CreateNode(ctx, "TextNode", SCENE_NS::ClassId::TextNode);
}

napi_value SceneJS::CreateMaterial(NapiApi::FunctionContext<NapiApi::Object, uint32_t>& ctx)
{
    const auto env = ctx.GetEnv();
    auto promise = Promise(env);
    uint32_t type = ctx.Arg<1>();

    auto convertToJs = [promise, type, sceneRef = NapiApi::StrongRef(ctx.This()),
                           paramRef = NapiApi::StrongRef(ctx.Arg<0>())](SCENE_NS::IMaterial::Ptr material) mutable {
        const auto env = promise.Env();
        if (type == BaseMaterial::SHADER) {
            META_NS::SetValue(material->Type(), SCENE_NS::MaterialType::CUSTOM);
        }
        napi_value args[] = { sceneRef.GetValue(), paramRef.GetValue() };
        const auto result = CreateFromNativeInstance(env, material, PtrType::STRONG, args);
        promise.Resolve(result.ToNapiValue());
    };

    const auto scene = interface_pointer_cast<SCENE_NS::IScene>(GetNativeObject());
    const auto jsQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(JS_THREAD_DEP);
    scene->CreateObject<SCENE_NS::IMaterial>(SCENE_NS::ClassId::Material).Then(BASE_NS::move(convertToJs), jsQ);
    return promise;
}

napi_value SceneJS::CreateScene(NapiApi::FunctionContext<>& ctx)
{
    return Load(ctx);
}

napi_value SceneJS::ImportNode(NapiApi::FunctionContext<BASE_NS::string, NapiApi::Object, NapiApi::Object>& ctx)
{
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
    BASE_NS::string name = ctx.Arg<0>();
    NapiApi::Object nextScene = ctx.Arg<1>();
    NapiApi::Object nparent = ctx.Arg<2>();
    auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject());

    if (!nextScene || !scene) {
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
        return CreateFromNativeInstance(ctx.GetEnv(), result, PtrType::WEAK, args).ToNapiValue();
    }
    return ctx.GetNull();
}

napi_value SceneJS::GetRenderMode(NapiApi::FunctionContext<>& ctx)
{
    auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject());
    if (!scene) {
        return ctx.GetUndefined();
    }
    return ctx.GetNumber(uint32_t(scene->GetRenderMode().GetResult()));
}
void SceneJS::SetRenderMode(NapiApi::FunctionContext<uint32_t>& ctx)
{
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
    if (ctx.ArgCount() > 1) {
        return ctx.GetUndefined();
    }
    auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject());
    if (!scene) {
        return ctx.GetUndefined();
    }

    // todo: fix this
    // NapiApi::Object params = ctx.Arg<0>();
    // auto alwaysRender = params ? params.Get<bool>("alwaysRender") : true;

    if (auto sc = scene->GetInternalScene()) {
        sc->RenderFrame();
    }

    return ctx.GetBoolean(true);
}

napi_value SceneJS::CreateComponent(NapiApi::FunctionContext<NapiApi::Object, BASE_NS::string>& ctx)
{
    auto env = ctx.GetEnv();
    auto promise = Promise(env);
    if (ctx.ArgCount() > 2) {
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
        return promise.Resolve(CreateFromNativeInstance(env, "SceneComponent", comp, PtrType::WEAK, args).ToNapiValue());
    }
    return promise.Reject("Could not create component");
}

napi_value SceneJS::GetComponent(NapiApi::FunctionContext<NapiApi::Object, BASE_NS::string>& ctx)
{
    if (ctx.ArgCount() > 2) {
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
    return renderContextJS_.GetValue();
}

napi_value SceneJS::CreateMeshResource(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
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

    napi_value args[] = { ctx.This().ToNapiValue(), resourceParams.ToNapiValue() };
    const auto meshResource = META_NS::GetObjectRegistry().Create(SCENE_NS::ClassId::MeshResource);
    const auto result = CreateFromNativeInstance(env, meshResource, PtrType::STRONG, args);
    return promise.Resolve(result.ToNapiValue());
}

napi_value SceneJS::CreateGeometry(NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object>& ctx)
{
    const auto env = ctx.GetEnv();
    auto promise = Promise(env);

    const auto sceneJs = ctx.This();
    auto params = NapiApi::Object { ctx.Arg<0>() };
    auto meshRes = NapiApi::Object { ctx.Arg<1>() };
    const auto meshResourceJs = static_cast<MeshResourceJS*>(meshRes.GetRoot()->GetInstanceImpl(MeshResourceJS::ID));
    if (!meshResourceJs) {
        return promise.Reject("Invalid mesh resource given");
    }

    // We don't use futures here, but rather just GetResult everything immediately.
    // Otherwise there's a race condition for a deadlock.
    auto scene = sceneJs.GetNative<SCENE_NS::IScene>();
    const auto path = ExtractNodePath(params);
    auto meshNode = scene->CreateNode(path, SCENE_NS::ClassId::MeshNode).GetResult();
    if (auto access = interface_pointer_cast<SCENE_NS::IMeshAccess>(meshNode)) {
        const auto mesh = meshResourceJs->CreateMesh();
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
    const auto env = ctx.GetEnv();
    auto promise = Promise(env);
    NapiApi::Object resourceParams = ctx.Arg<0>();
    if (!resourceParams) {
        return promise.Reject("Invalid scene resource parameters given");
    }

    auto uri = ExtractUri(resourceParams.Get<NapiApi::Object>("uri"));
    auto scene = interface_pointer_cast<SCENE_NS::IScene>(GetNativeObject());
    auto convertToJs = [promise, uri, sceneRef = NapiApi::StrongRef(ctx.This()),
                           paramRef = NapiApi::StrongRef(resourceParams)](SCENE_NS::IShader::Ptr shader) mutable {
        if (!shader) {
            promise.Reject(BASE_NS::string { "Failed to load shader from URI: " }.append(uri));
            return;
        }
        const auto env = promise.Env();
        napi_value args[] = { sceneRef.GetValue(), paramRef.GetValue() };
        NapiApi::Object parms(env, args[1]);

        napi_value null;
        napi_get_null(env, &null);
        parms.Set("Material", null); // not bound to anything...
        const auto result = CreateFromNativeInstance(env, shader, PtrType::STRONG, args);
        promise.Resolve(result.ToNapiValue());
    };

    auto jsQ = META_NS::GetTaskQueueRegistry().GetTaskQueue(JS_THREAD_DEP);
    renderMan_->LoadShader(uri).Then(BASE_NS::move(convertToJs), jsQ);
    return promise;
}

// To pass LoadResult between tasks with Future::Then.
META_TYPE(BASE_NS::shared_ptr<CORE_NS::IImageLoaderManager::LoadResult>);

napi_value SceneJS::CreateImage(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!renderContextJS_.IsEmpty()) {
        if (auto context = renderContextJS_.GetObject().GetJsWrapper<RenderContextJS>()) {
            NapiApi::Function f(ctx.GetEnv(), renderContextJS_.GetObject().Get("createImage"));
            auto param = ctx.Arg<0>().ToNapiValue();
            return f.Invoke(renderContextJS_.GetObject(), 1, &param);
        }
    }

    return ctx.GetUndefined();
}
napi_value SceneJS::GetAnimations(NapiApi::FunctionContext<>& ctx)
{
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

void SceneJS::DisposeHook(uintptr_t token, NapiApi::Object obj)
{
    disposables_[token] = { obj };
}
void SceneJS::ReleaseDispose(uintptr_t token)
{
    auto it = disposables_.find(token);
    if (it != disposables_.end()) {
        it->second.Reset();
        disposables_.erase(it->first);
    }
}

void SceneJS::StrongDisposeHook(uintptr_t token, NapiApi::Object obj)
{
    strongDisposables_[token] = NapiApi::StrongRef(obj);
}
void SceneJS::ReleaseStrongDispose(uintptr_t token)
{
    auto it = strongDisposables_.find(token);
    if (it != strongDisposables_.end()) {
        it->second.Reset();
        strongDisposables_.erase(it->first);
    }
}
