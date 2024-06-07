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
#include "LightJS.h"
#include "MaterialJS.h"
static constexpr BASE_NS::Uid IO_QUEUE { "be88e9a0-9cd8-45ab-be48-937953dc258f" };
#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/property_events.h>
#include <scene_plugin/api/camera.h> //for the classid...
#include <scene_plugin/api/environment_uid.h>
#include <scene_plugin/api/material_uid.h>
#include <scene_plugin/api/render_configuration_uid.h>
#include <scene_plugin/api/scene_uid.h>
#include <scene_plugin/interface/intf_ecs_scene.h>
#include <scene_plugin/interface/intf_material.h>
#include <scene_plugin/interface/intf_render_configuration.h>
#include <scene_plugin/interface/intf_scene.h>

#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#include "scene_adapter/scene_adapter.h"
#endif

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;

static META_NS::ITaskQueue::Ptr releaseThread;
static constexpr BASE_NS::Uid JS_RELEASE_THREAD { "3784fa96-b25b-4e9c-bbf1-e897d36f73af" };

void SceneJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;
    // clang-format off
    auto loadFun = [](napi_env e,napi_callback_info cb) -> napi_value
        {
            FunctionContext<> fc(e,cb);
            return SceneJS::Load(fc);
        };

    napi_property_descriptor props[] = { 
        // static methods
        napi_property_descriptor{ "load", nullptr, loadFun, nullptr, nullptr, nullptr, (napi_property_attributes)(napi_static|napi_default_method)},
        // properties
        GetSetProperty<NapiApi::Object, SceneJS, &SceneJS::GetEnvironment, &SceneJS::SetEnvironment>("environment"),
        GetProperty<NapiApi::Array,SceneJS, &SceneJS::GetAnimations>("animations"),
        // animations
        GetProperty<BASE_NS::string, SceneJS, &SceneJS::GetRoot>("root"),
        // scene methods
        Method<NapiApi::FunctionContext<BASE_NS::string>, SceneJS, &SceneJS::GetNode>("getNodeByPath"),
        Method<NapiApi::FunctionContext<>, SceneJS, &SceneJS::GetResourceFactory>("getResourceFactory"),
        Method<NapiApi::FunctionContext<>, SceneJS, &SceneJS::Dispose>("destroy"),
        
        // SceneResourceFactory methods
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateCamera>("createCamera"),
        Method<NapiApi::FunctionContext<NapiApi::Object,uint32_t>, SceneJS, &SceneJS::CreateLight>("createLight"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateNode>("createNode"),
        Method<NapiApi::FunctionContext<NapiApi::Object,uint32_t>, SceneJS, &SceneJS::CreateMaterial>("createMaterial"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateShader>("createShader"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateImage>("createImage"),
        Method<NapiApi::FunctionContext<NapiApi::Object>, SceneJS, &SceneJS::CreateEnvironment>("createEnvironment")
    };
    // clang-format on

    napi_value func;
    auto status = napi_define_class(env, "Scene", NAPI_AUTO_LENGTH, BaseObject::ctor<SceneJS>(), nullptr,
        sizeof(props) / sizeof(props[0]), props, &func);

    napi_set_named_property(env, exports, "Scene", func);

    NapiApi::MyInstanceState* mis;
    napi_get_instance_data(env, (void**)&mis);
    mis->StoreCtor("Scene", func);
}
class AsyncStateBase {
public:
    virtual ~AsyncStateBase()
    {
        // assert that the promise has been fulfilled.
        CORE_ASSERT(deferred == nullptr);
        // assert that the threadsafe func has been released.
        CORE_ASSERT(termfun == nullptr);
    }

    // inherit from this
    napi_deferred deferred { nullptr };
    napi_threadsafe_function termfun { nullptr };
    napi_value result { nullptr };
    virtual bool finally(napi_env env) = 0;
    template<typename A>
    A Flip(A& a)
    {
        A tmp = a;
        a = nullptr;
        return tmp;
    }
    void CallIt()
    {
        // should be called from engine thread only.
        // use an extra task in engine to trigger this
        // to woraround an issue wherer CallIt is called IN an eventhandler.
        // as there seems to be cases where (uncommon, have no repro. but has happend)
        // napi_release_function waits for threadsafe function completion
        // and the "js function" is waiting for the enghen thread (which is blocked releasing the function)
        if (auto tf = Flip(termfun)) {
            META_NS::GetTaskQueueRegistry()
                .GetTaskQueue(JS_RELEASE_THREAD)
                ->AddTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>(BASE_NS::move([tf]() {
                    napi_call_threadsafe_function(tf, nullptr, napi_threadsafe_function_call_mode::napi_tsfn_blocking);
                    napi_release_threadsafe_function(tf, napi_threadsafe_function_release_mode::napi_tsfn_release);
                    return false;
                })));
        }
    }
    // callable from js thread
    void Fail(napi_env env)
    {
        napi_status status;
        if (auto df = Flip(deferred)) {
            status = napi_reject_deferred(env, df, result);
        }
        if (auto tf = Flip(termfun)) {
            // if called from java thread. then release it here...
            napi_release_threadsafe_function(tf, napi_threadsafe_function_release_mode::napi_tsfn_release);
        }
    }
    void Success(napi_env env)
    {
        napi_status status;
        // return success
        if (auto df = Flip(deferred)) {
            status = napi_resolve_deferred(env, df, result);
        }
        if (auto tf = Flip(termfun)) {
            // if called from java thread. then release it here...
            napi_release_threadsafe_function(tf, napi_threadsafe_function_release_mode::napi_tsfn_release);
        }
    }
};
napi_value MakePromise(napi_env env, AsyncStateBase* data)
{
    napi_value promise;
    napi_status status = napi_create_promise(env, &data->deferred, &promise);
    napi_value name;
    napi_create_string_latin1(env, "a", 1, &name);
    status = napi_create_threadsafe_function(
        env, nullptr, nullptr, name, 1, 1, data /*finalize_data*/,
        [](napi_env env, void* finalize_data, void* finalize_hint) {
            AsyncStateBase* data = (AsyncStateBase*)finalize_data;
            delete data;
        },
        data /*context*/,
        [](napi_env env, napi_value js_callback, void* context, void* inData) {
            // IN JS THREAD. (so careful with the calls to engine)
            napi_status status;
            AsyncStateBase* data = (AsyncStateBase*)context;
            status = napi_get_undefined(env, &data->result);
            if (data->finally(env)) {
                data->Success(env);
            } else {
                data->Fail(env);
            }
        },
        &data->termfun);
    return promise;
}
BASE_NS::string FetchResourceOrUri(napi_env e, napi_value arg)
{
    napi_valuetype type;
    napi_typeof(e, arg, &type);
    if (type == napi_string) {
        return NapiApi::Value<BASE_NS::string>(e, arg);
    }
    if (type == napi_object) {
        NapiApi::Object resource(e, arg);
        uint32_t id = resource.Get<uint32_t>("id");
        uint32_t type = resource.Get<uint32_t>("type");
        NapiApi::Array parms = resource.Get<NapiApi::Array>("params");
        BASE_NS::string uri;
        if ((id == 0) && (type == 30000)) {
            // seems like a correct rawfile.
            uri = parms.Get<BASE_NS::string>(0);
        }
        if (!uri.empty()) {
            // add the schema then
            uri.insert(0, "OhosRawFile://");
        }
        return uri;
    }
    return "";
}

BASE_NS::string FetchResourceOrUri(NapiApi::FunctionContext<>& ctx)
{
    BASE_NS::string uri;
    NapiApi::FunctionContext<NapiApi::Object> resourceContext(ctx);
    NapiApi::FunctionContext<BASE_NS::string> uriContext(ctx);

    if (uriContext) {
        // actually not supported anymore.
        uri = uriContext.Arg<0>();
        // check if there is a protocol
        auto t = uri.find_first_of("://");
        if (t == BASE_NS::string::npos) {
            // no proto . so use default
            uri.insert(0, "OhosRawFile:///");
        }
    } else if (resourceContext) {
        // get it from resource then
        NapiApi::Object resource = resourceContext.Arg<0>();
        uint32_t id = resource.Get<uint32_t>("id");
        uint32_t type = resource.Get<uint32_t>("type");
        NapiApi::Array parms = resource.Get<NapiApi::Array>("params");
        if ((id == 0) && (type == 30000)) { // 30000 : type
            // seems like a correct rawfile.
            uri = parms.Get<BASE_NS::string>(0);
        }
        if (!uri.empty()) {
            // add the schema then
            uri.insert(0, "OhosRawFile://");
        }
    }
    return uri;
}

napi_value SceneJS::Load(NapiApi::FunctionContext<>& ctx)
{
    BASE_NS::string uri = FetchResourceOrUri(ctx);

    if (uri.empty()) {
        // unsupported input..
        return {};
    }
    // make sure slashes are correct.. *eh*
    for (;;) {
        auto t = uri.find_first_of('\\');
        if (t == BASE_NS::string::npos) {
            break;
        }
        uri[t] = '/';
    }

    auto &tr = META_NS::GetTaskQueueRegistry();
    releaseThread = META_NS::GetTaskQueueRegistry().GetTaskQueue(JS_RELEASE_THREAD);
    if (!releaseThread) {
        auto &obr = META_NS::GetObjectRegistry();
        releaseThread = obr.Create<META_NS::ITaskQueue>(META_NS::ClassId::ThreadedTaskQueue);
        tr.RegisterTaskQueue(releaseThread, JS_RELEASE_THREAD);
    }

    struct AsyncState : public AsyncStateBase {
        BASE_NS::string uri;
        SCENE_NS::IScene::Ptr scene;
        META_NS::IEvent::Token onLoadedToken { 0 };
        bool finally(napi_env env) override
        {
            if (scene) {
                auto obj = interface_pointer_cast<META_NS::IObject>(scene);
                result = CreateJsObj(env, "Scene", obj, true, 0, nullptr);
                // link the native object to js object.
                StoreJsObj(obj, { env, result });

                NapiApi::Object me(env, result);
                auto curenv = me.Get<NapiApi::Object>("environment");
                if (!curenv) {
                    // setup default env
                    NapiApi::Object argsIn(env);
                    argsIn.Set("name", "DefaultEnv");

                    auto* tro = (SceneJS*)(me.Native<TrueRootObject>());
                    auto res = tro->CreateEnvironment(me, argsIn);
                    res.Set("backgroundType", NapiApi::Value<uint32_t>(env, 1)); // image.. but with null.
                    me.Set("environment", res);
                }

#ifdef __SCENE_ADAPTER__
                // set SceneAdapter
                auto oo = GetRootObject(env, result);

                auto sceneAdapter = std::make_shared<OHOS::Render3D::SceneAdapter>();
                sceneAdapter->SetSceneObj(oo->GetNativeObject());
                auto sceneJs = static_cast<SceneJS*>(oo);
                sceneJs->scene_ = sceneAdapter;
#endif
                return true;
            }
            scene.reset();
            return false;
        };
    };
    AsyncState* data = new AsyncState();
    data->uri = uri;

    auto fun = [data]() {
        // IN ENGINE THREAD! (so no js calls)
        using namespace SCENE_NS;
        auto& obr = META_NS::GetObjectRegistry();
        auto params = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        auto scene = interface_pointer_cast<SCENE_NS::IScene>(obr.Create(SCENE_NS::ClassId::Scene, params));
        if (!scene) {
            // insta fail
            data->CallIt();
            return false;
        }
        data->scene = scene;

        auto onLoaded = META_NS::MakeCallback<META_NS::IOnChanged>([data]() {
            bool complete = false;
            auto scene = data->scene;
            auto status = scene->Status()->GetValue();
            if (status == SCENE_NS::IScene::SCENE_STATUS_READY) {
                // still in engine thread..
                complete = true;
            } else if (status == SCENE_NS::IScene::SCENE_STATUS_LOADING_FAILED) {
                data->scene.reset(); // make sure we don't have anything in result if error.
                complete = true;
            }

            if (complete) {
                scene->Status()->OnChanged()->RemoveHandler(data->onLoadedToken);
                data->onLoadedToken = 0;
                if (scene) {
                    auto& obr = META_NS::GetObjectRegistry();
                    // make sure we have renderconfig
                    auto rc = scene->RenderConfiguration()->GetValue();
                    if (!rc) {
                        // Create renderconfig
                        rc = obr.Create<SCENE_NS::IRenderConfiguration>(SCENE_NS::ClassId::RenderConfiguration);
                        scene->RenderConfiguration()->SetValue(rc);
                    }
                    /*if (auto env = rc->Environment()->GetValue(); !env) {
                        // create default env then
                        env = scene->CreateNode<SCENE_NS::IEnvironment>("default_env");
                        env->Background()->SetValue(SCENE_NS::IEnvironment::CUBEMAP);
                        rc->Environment()->SetValue(env);
                    }*/

                    interface_cast<IEcsScene>(scene)->RenderMode()->SetValue(IEcsScene::RenderMode::RENDER_ALWAYS);
                    auto params = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
                    auto duh = params->GetArrayPropertyByName<IntfWeakPtr>("Scenes");
                    duh->AddValue(interface_pointer_cast<CORE_NS::IInterface>(scene));
                }
                // call the threadsafe func here. (let the javascript know we are done)
                data->CallIt();
            }
        });
        data->onLoadedToken = scene->Status()->OnChanged()->AddHandler(onLoaded);
        scene->Asynchronous()->SetValue(false);
        scene->Uri()->SetValue(data->uri);
        return false;
    };
    // Should it be possible to cancel? (ie. do we need to store the token for something..)
    META_NS::GetTaskQueueRegistry()
        .GetTaskQueue(ENGINE_THREAD)
        ->AddTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>(BASE_NS::move(fun)));

    return MakePromise(ctx, data);
}

napi_value SceneJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    LOG_F("SCENE_JS::Dispose");
    DisposeNative();
    return {};
}
void SceneJS::DisposeNative()
{
    LOG_F("SCENE_JS::DisposeNative");
    // dispose
    while (!disposables_.empty()) {
        auto env = disposables_.begin()->second.GetObject();
        if (env) {
            NapiApi::Function func = env.Get<NapiApi::Function>("destroy");
            if (func) {
                func.Invoke(env);
            }
        }
    }

    // dispose all cameras/env/etcs.
    while (!strongDisposables_.empty()) {
        auto it = strongDisposables_.begin();
        auto token = it->first;
        auto env = it->second.GetObject();
        if (env) {
            NapiApi::Function func = env.Get<NapiApi::Function>("destroy");
            if (func) {
                func.Invoke(env);
            }
        }
    }
    if (auto env = environmentJS_.GetObject()) {
        NapiApi::Function func = env.Get<NapiApi::Function>("destroy");
        if (func) {
            func.Invoke(env);
        }
    }
    environmentJS_.Reset();
    for (auto b : bitmaps_) {
        b.second.reset();
    }
    if (auto scene = interface_pointer_cast<SCENE_NS::IScene>(GetNativeObject())) {
        // reset the native object refs
        SetNativeObject(nullptr, false);
        SetNativeObject(nullptr, true);

        ExecSyncTask([scn = BASE_NS::move(scene)]() {
            auto r = scn->RenderConfiguration()->GetValue();
            auto e = r->Environment()->GetValue();
            e.reset();
            r.reset();
            scn->RenderConfiguration()->SetValue(nullptr);
            return META_NS::IAny::Ptr {};
        });
    }
}
void* SceneJS::GetInstanceImpl(uint32_t id)
{
    if (id == SceneJS::ID) {
        return this;
    }
    return nullptr;
}
void SceneJS::Finalize(napi_env env)
{
    // hmm.. do i need to do something BEFORE the object gets deleted..
    BaseObject<SceneJS>::Finalize(env);
}

SceneJS::SceneJS(napi_env e, napi_callback_info i) : BaseObject<SceneJS>(e, i)
{
    LOG_F("SceneJS ++");
    NapiApi::FunctionContext<NapiApi::Object> fromJs(e, i);

    if (!fromJs) {
        // okay internal create. we will receive the object after.
        return;
    }

    // Construct native object
    SCENE_NS::IScene::Ptr scene;
    ExecSyncTask([&scene]() {
        using namespace SCENE_NS;
        auto& obr = META_NS::GetObjectRegistry();
        auto params = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        scene = interface_pointer_cast<SCENE_NS::IScene>(obr.Create(SCENE_NS::ClassId::Scene, params));
        if (scene) {
            // only asynch false works.. (otherwise nodes are in random state when scene is ready.. )
            scene->Asynchronous()->SetValue(false);
            interface_cast<IEcsScene>(scene)->RenderMode()->SetValue(IEcsScene::RenderMode::RENDER_ALWAYS);
            auto duh = params->GetArrayPropertyByName<IntfWeakPtr>("Scenes");
            duh->AddValue(interface_pointer_cast<CORE_NS::IInterface>(scene));
        }
        return META_NS::IAny::Ptr {};
    });

    // process constructor args..
    NapiApi::Object meJs(e, fromJs.This());
    SetNativeObject(interface_pointer_cast<META_NS::IObject>(scene), true /* KEEP STRONG REF */);
    StoreJsObj(interface_pointer_cast<META_NS::IObject>(scene), meJs);

    NapiApi::Object args = fromJs.Arg<0>();
    if (args) {
        if (auto name = args.Get("name")) {
            meJs.Set("name", name);
        }
        if (auto uri = args.Get("uri")) {
            meJs.Set("uri", uri);
        }
    }
}

SceneJS::~SceneJS()
{
    LOG_F("SceneJS --");
    if (!GetNativeObject()) {
        return;
    }
    ExecSyncTask([scene = GetNativeObject()]() {
        auto& obr = META_NS::GetObjectRegistry();
        auto params = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        auto duh = params->GetArrayPropertyByName<IntfWeakPtr>("Scenes");
        if (duh) {
            for (auto i = 0; i < duh->GetSize();) {
                auto w = duh->GetValueAt(i);
                if (w.lock() == nullptr) {
                    duh->RemoveAt(i);
                } else {
                    i++;
                }
            }
        }
        return META_NS::IAny::Ptr {};
    });
}

napi_value SceneJS::GetNode(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    // verify that path starts from "correct root" and then let the root node handle the rest.
    NapiApi::Object meJs(ctx, ctx.This());
    NapiApi::Object root = meJs.Get<NapiApi::Object>("root");
    BASE_NS::string rootName = root.Get<BASE_NS::string>("name");
    NapiApi::Function func = root.Get<NapiApi::Function>("getNodeByPath");
    BASE_NS::string path = ctx.Arg<0>();

    // remove the "root nodes name" (make sure it also matches though..)
    auto pos = path.find('/', 0);
    BASE_NS::string_view step = path.substr(0, pos);
    if (step != rootName) {
        // root not matching
        return ctx.GetNull();
    }

    if (pos != BASE_NS::string_view::npos) {
        BASE_NS::string rest(path.substr(pos + 1));
        napi_value newpath = nullptr;
        napi_status status = napi_create_string_utf8(ctx, rest.c_str(), rest.length(), &newpath);
        if (newpath) {
            return func.Invoke(root, 1, &newpath);
        }
        return ctx.GetNull();
    }
    return root;
}
napi_value SceneJS::GetRoot(NapiApi::FunctionContext<>& ctx)
{
    if (auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject())) {
        SCENE_NS::INode::Ptr root;
        auto cb = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([scene, &root]() {
            root = scene->RootNode()->GetValue();
            if (root) {
                // make sure our direct descendants exist.
                root->BuildChildren(SCENE_NS::INode::BuildBehavior::NODE_BUILD_ONLY_DIRECT_CHILDREN);
            }
            return META_NS::IAny::Ptr {};
        });
        META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD)->AddWaitableTask(cb)->Wait();
        auto obj = interface_pointer_cast<META_NS::IObject>(root);

        if (auto cached = FetchJsObj(obj)) {
            // always return the same js object.
            return cached;
        }

        NapiApi::StrongRef sceneRef { ctx, ctx.This() };
        if (!GetNativeMeta<SCENE_NS::IScene>(sceneRef.GetObject())) {
            CORE_LOG_F("INVALID SCENE!");
        }

        NapiApi::Object argJS(ctx);
        napi_value args[] = { sceneRef.GetObject(), argJS };

        return CreateFromNativeInstance(ctx, obj, false /*these are owned by the scene*/, BASE_NS::countof(args), args);
    }
    return ctx.GetUndefined();
}

napi_value SceneJS::GetEnvironment(NapiApi::FunctionContext<>& ctx)
{
    if (auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject())) {
        SCENE_NS::IEnvironment::Ptr environment;
        ExecSyncTask([scene, &environment]() {
            auto rc = scene->RenderConfiguration()->GetValue();
            if (rc) {
                environment = rc->Environment()->GetValue();
            }
            return META_NS::IAny::Ptr {};
        });
        if (environment) {
            auto obj = interface_pointer_cast<META_NS::IObject>(environment);
            if (auto cached = FetchJsObj(obj)) {
                // always return the same js object.
                return cached;
            }

            NapiApi::StrongRef sceneRef { ctx, ctx.This() };
            if (!GetNativeMeta<SCENE_NS::IScene>(sceneRef.GetObject())) {
                CORE_LOG_F("INVALID SCENE!");
            }

            NapiApi::Object argJS(ctx);
            napi_value args[] = { sceneRef.GetObject(), argJS };

            environmentJS_ = { ctx, CreateFromNativeInstance(ctx, obj, false, BASE_NS::countof(args), args) };
            return environmentJS_.GetValue();
        }
    }
    return ctx.GetNull();
}

void SceneJS::SetEnvironment(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    NapiApi::Object env = ctx.Arg<0>();

    if (auto currentlySet = environmentJS_.GetObject()) {
        if ((napi_value)currentlySet == (napi_value)env) {
            // setting the exactly the same environment. do nothing.
            return;
        }
    }
    environmentJS_.Reset();
    if (env) {
        environmentJS_ = { env };
    }
    SCENE_NS::IEnvironment::Ptr environment;
    if (env) {
        environment = GetNativeMeta<SCENE_NS::IEnvironment>(env);
    }
    if (auto scene = interface_cast<SCENE_NS::IScene>(GetNativeObject())) {
        ExecSyncTask([scene, environment]() {
            auto rc = scene->RenderConfiguration()->GetValue();
            if (!rc) {
                // no render config, so create it.
                auto& obr = META_NS::GetObjectRegistry();
                rc = obr.Create<SCENE_NS::IRenderConfiguration>(SCENE_NS::ClassId::RenderConfiguration);
                scene->RenderConfiguration()->SetValue(rc);
            }
            if (rc) {
                rc->Environment()->SetValue(environment);
            }
            return META_NS::IAny::Ptr {};
        });
    }
}

// resource factory

napi_value SceneJS::GetResourceFactory(NapiApi::FunctionContext<>& ctx)
{
    // just return this. as scene is the factory also.
    return ctx.This();
}
NapiApi::Object SceneJS::CreateEnvironment(NapiApi::Object scene, NapiApi::Object argsIn)
{
    napi_env env = scene.GetEnv();
    napi_value args[] = { scene, argsIn };
    auto result = NapiApi::Object(GetJSConstructor(env, "Environment"), BASE_NS::countof(args), args);
    auto ref = NapiApi::StrongRef { env, result };
    return { env, ref.GetValue() };
}
napi_value SceneJS::CreateEnvironment(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    struct AsyncState : public AsyncStateBase {
        NapiApi::StrongRef this_;
        NapiApi::StrongRef args_;
        bool finally(napi_env env) override
        {
            auto* tro = (SceneJS*)(this_.GetObject().Native<TrueRootObject>());
            result = tro->CreateEnvironment(this_.GetObject(), args_.GetObject());
            return true;
        };
    };
    AsyncState* data = new AsyncState();
    data->this_ = NapiApi::StrongRef(ctx, ctx.This());
    data->args_ = NapiApi::StrongRef(ctx, ctx.Arg<0>());
    napi_value result = MakePromise(ctx, data);
    // and just instantly complete it
    data->finally(ctx);
    data->Success(ctx);
    return result;
}

napi_value SceneJS::CreateCamera(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    struct AsyncState : public AsyncStateBase {
        NapiApi::StrongRef this_;
        NapiApi::StrongRef args_;
        bool finally(napi_env env) override
        {
            napi_value args[] = {
                this_.GetValue(), // scene..
                args_.GetValue()  // params.
            };
            result = NapiApi::Object(GetJSConstructor(env, "Camera"), BASE_NS::countof(args), args);
            return true;
        };
    };
    AsyncState* data = new AsyncState();
    data->this_ = NapiApi::StrongRef(ctx, ctx.This());
    data->args_ = NapiApi::StrongRef(ctx, ctx.Arg<0>());
    napi_value result = MakePromise(ctx, data);
    // and just instantly complete it
    data->finally(ctx);
    data->Success(ctx);
    return result;
}

napi_value SceneJS::CreateLight(NapiApi::FunctionContext<NapiApi::Object, uint32_t>& ctx)
{
    struct AsyncState : public AsyncStateBase {
        NapiApi::StrongRef this_;
        NapiApi::StrongRef args_;
        uint32_t lightType_;
        bool finally(napi_env env) override
        {
            napi_value args[] = {
                this_.GetValue(), // scene..
                args_.GetValue()  // params.
            };
            NapiApi::Function func;
            switch (lightType_) {
                case BaseLight::DIRECTIONAL: {
                    func = GetJSConstructor(env, "DirectionalLight");
                    break;
                }
                case BaseLight::POINT: {
                    func = GetJSConstructor(env, "PointLight");
                    break;
                }
                case BaseLight::SPOT: {
                    func = GetJSConstructor(env, "SpotLight");
                    break;
                }
                default:
                    break;
            }
            if (!func) {
                return false;
            }
            result = NapiApi::Object(func, BASE_NS::countof(args), args);
            return true;
        };
    };
    AsyncState* data = new AsyncState();
    data->this_ = NapiApi::StrongRef(ctx, ctx.This());
    data->args_ = NapiApi::StrongRef(ctx, ctx.Arg<0>());
    data->lightType_ = ctx.Arg<1>();
    napi_value result = MakePromise(ctx, data);
    // and just instantly complete it
    data->finally(ctx);
    data->Success(ctx);
    return result;
}

napi_value SceneJS::CreateNode(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    struct AsyncState : public AsyncStateBase {
        NapiApi::StrongRef this_;
        NapiApi::StrongRef args_;
        bool finally(napi_env env) override
        {
            napi_value args[] = {
                this_.GetValue(), // scene..
                args_.GetValue()  // params.
            };
            result = NapiApi::Object(GetJSConstructor(env, "Node"), BASE_NS::countof(args), args);
            return true;
        };
    };
    AsyncState* data = new AsyncState();
    data->this_ = NapiApi::StrongRef(ctx, ctx.This());
    data->args_ = NapiApi::StrongRef(ctx, ctx.Arg<0>());

    napi_value ret = MakePromise(ctx, data);
    // and just instantly complete it
    data->finally(ctx);
    data->Success(ctx);
    return ret;
}

napi_value SceneJS::CreateMaterial(NapiApi::FunctionContext<NapiApi::Object, uint32_t>& ctx)
{
    struct AsyncState : public AsyncStateBase {
        NapiApi::StrongRef this_;
        NapiApi::StrongRef args_;
        uint32_t type_;
        BASE_NS::string name_;
        SCENE_NS::IMaterial::Ptr material_;
        SCENE_NS::IScene::Ptr scene_;
        bool finally(napi_env env) override
        {
            napi_value args[] = {
                this_.GetValue(), // scene..
                args_.GetValue()  // params.
            };

            if (type_ == BaseMaterial::SHADER) {
                MakeNativeObjectParam(env, material_, BASE_NS::countof(args), args);
                NapiApi::Object materialJS(GetJSConstructor(env, "ShaderMaterial"), BASE_NS::countof(args), args);
                result = materialJS;
            } else {
                // fail..
                material_.reset();
            }
            return true;
        };
    };
    AsyncState* data = new AsyncState();
    data->this_ = NapiApi::StrongRef(ctx, ctx.This());
    data->args_ = NapiApi::StrongRef(ctx, ctx.Arg<0>());
    data->type_ = ctx.Arg<1>();
    if (ctx.Arg<0>()) {
        NapiApi::Object parms = ctx.Arg<0>();
        data->name_ = parms.Get<BASE_NS::string>("name");
    }

    napi_value result = MakePromise(ctx, data);
    data->scene_ = interface_pointer_cast<SCENE_NS::IScene>(GetNativeObject());

    // create an engine task and complete it there..
    auto fun = [data]() {
        data->material_ = data->scene_->CreateMaterial(data->name_);
        data->CallIt();
        return false;
    };

    META_NS::GetTaskQueueRegistry()
        .GetTaskQueue(ENGINE_THREAD)
        ->AddTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>(BASE_NS::move(fun)));

    return result;
}

napi_value SceneJS::CreateShader(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    struct AsyncState : public AsyncStateBase {
        NapiApi::StrongRef this_;
        NapiApi::StrongRef args_;
        BASE_NS::string uri_;
        BASE_NS::string name_;
        SCENE_NS::IShader::Ptr shader_;
        bool finally(napi_env env) override
        {
            napi_value args[] = {
                this_.GetValue(), // scene..
                args_.GetValue()  // params.
            };
            NapiApi::Object parms(env, args[1]);

            napi_value null;
            napi_get_null(env, &null);
            parms.Set("Material", null); // not bound to anything...

            MakeNativeObjectParam(env, shader_, BASE_NS::countof(args), args);
            NapiApi::Object shaderJS(GetJSConstructor(env, "Shader"), BASE_NS::countof(args), args);
            result = shaderJS;

            return true;
        };
    };
    AsyncState* data = new AsyncState();
    data->this_ = NapiApi::StrongRef(ctx, ctx.This());
    data->args_ = NapiApi::StrongRef(ctx, ctx.Arg<0>());
    if (ctx.Arg<0>()) {
        NapiApi::Object parms = ctx.Arg<0>();
        data->name_ = parms.Get<BASE_NS::string>("name");
        data->uri_ = FetchResourceOrUri(ctx, parms.Get("uri"));
    }

    napi_value result = MakePromise(ctx, data);

    auto fun = [data]() {
        auto& obr = META_NS::GetObjectRegistry();
        auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        auto renderContext = doc->GetPropertyByName<IntfPtr>("RenderContext")->GetValue();

        auto params =
            interface_pointer_cast<META_NS::IMetadata>(META_NS::GetObjectRegistry().Create(META_NS::ClassId::Object));

        params->AddProperty(META_NS::ConstructProperty<IntfPtr>(
            "RenderContext", renderContext, META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE));

        params->AddProperty(META_NS::ConstructProperty<BASE_NS::string>(
            "uri", data->uri_, META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE));

        data->shader_ = META_NS::GetObjectRegistry().Create<SCENE_NS::IShader>(SCENE_NS::ClassId::Shader, params);
        data->CallIt();
        return false;
    };

    META_NS::GetTaskQueueRegistry()
        .GetTaskQueue(ENGINE_THREAD)
        ->AddTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>(BASE_NS::move(fun)));

    return result;
}

void SceneJS::StoreBitmap(BASE_NS::string_view uri, SCENE_NS::IBitmap::Ptr bitmap)
{
    CORE_NS::UniqueLock lock(mutex_);
    if (bitmap) {
        bitmaps_[uri] = bitmap;
    } else {
        // setting null. releases.
        bitmaps_.erase(uri);
    }
}
SCENE_NS::IBitmap::Ptr SceneJS::FetchBitmap(BASE_NS::string_view uri)
{
    CORE_NS::UniqueLock lock(mutex_);
    auto it = bitmaps_.find(uri);
    if (it != bitmaps_.end()) {
        return it->second;
    }
    return {};
}

napi_value SceneJS::CreateImage(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    using namespace RENDER_NS;

    struct AsyncState : public AsyncStateBase {
        NapiApi::StrongRef this_;
        NapiApi::StrongRef args_;
        BASE_NS::shared_ptr<IRenderContext> renderContext_;
        BASE_NS::string name_;
        BASE_NS::string uri_;
        SCENE_NS::IBitmap::Ptr bitmap_;
        RenderHandleReference imageHandle_;
        SceneJS* owner_;
        CORE_NS::IImageLoaderManager::LoadResult imageLoadResult_;
        bool cached_ { false };
        bool finally(napi_env env) override
        {
            if (!bitmap_) {
                // return the error string..
                napi_create_string_utf8(env, imageLoadResult_.error, NAPI_AUTO_LENGTH, &result);
                return false;
            }
            if (!cached_) {
                // create the jsobject if we don't have one.
                napi_value args[] = {
                    this_.GetValue(), // scene..
                    args_.GetValue()  // params.
                };
                MakeNativeObjectParam(env, bitmap_, BASE_NS::countof(args), args);
                owner_->StoreBitmap(uri_, BASE_NS::move(bitmap_));
                NapiApi::Object imageJS(GetJSConstructor(env, "Image"), BASE_NS::countof(args), args);
                result = imageJS;
            }
            return true;
        };
    };
    AsyncState* data = new AsyncState();
    data->owner_ = this;
    data->this_ = NapiApi::StrongRef(ctx, ctx.This());
    data->args_ = NapiApi::StrongRef(ctx, ctx.Arg<0>());

    NapiApi::Object args = ctx.Arg<0>();
    if (args) {
        if (auto n = args.Get<BASE_NS::string>("name")) {
            data->name_ = n;
        }
        data->uri_ = FetchResourceOrUri(ctx, args.Get("uri"));
    }

    napi_value result = MakePromise(ctx, data);
    if (auto bitmap = FetchBitmap(data->uri_)) {
        // no aliasing.. so the returned bitmaps name is.. the old one.
        // *fix*
        // oh we have it already, no need to do anything in engine side.
        data->cached_ = true;
        data->bitmap_ = bitmap;
        auto obj = interface_pointer_cast<META_NS::IObject>(data->bitmap_);
        data->result = FetchJsObj(obj);
        data->Success(ctx);
        return result;
    }

    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    data->renderContext_ =
        interface_pointer_cast<IRenderContext>(doc->GetPropertyByName<IntfPtr>("RenderContext")->GetValue());

    // create an IO task (to load the cpu data)
    auto fun = [data]() -> META_NS::IAny::Ptr {
        uint32_t imageLoaderFlags = CORE_NS::IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS;
        auto& imageLoaderMgr = data->renderContext_->GetEngine().GetImageLoaderManager();
        data->imageLoadResult_ = imageLoaderMgr.LoadImage(data->uri_, imageLoaderFlags);
        return {};
    };

    // create final engine task (to create gpu resource)
    auto fun2 = [data](const META_NS::IAny::Ptr&) -> META_NS::IAny::Ptr {
        if (!data->imageLoadResult_.success) {
            CORE_LOG_E("Could not load image asset: %s", data->imageLoadResult_.error);
            data->bitmap_ = nullptr;
        } else {
            auto& gpuResourceMgr = data->renderContext_->GetDevice().GetGpuResourceManager();
            RenderHandleReference imageHandle {};
            GpuImageDesc gpuDesc = gpuResourceMgr.CreateGpuImageDesc(data->imageLoadResult_.image->GetImageDesc());
            gpuDesc.usageFlags = CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
            if (gpuDesc.engineCreationFlags & EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS) {
                gpuDesc.usageFlags |= CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
            }
            gpuDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            data->imageHandle_ = gpuResourceMgr.Create(data->uri_, gpuDesc, std::move(data->imageLoadResult_.image));
            data->bitmap_ = META_NS::GetObjectRegistry().Create<SCENE_NS::IBitmap>(SCENE_NS::ClassId::Bitmap);
            data->bitmap_->Uri()->SetValue(data->uri_);
            data->bitmap_->SetRenderHandle(data->imageHandle_, { gpuDesc.width, gpuDesc.height });
        }
        data->CallIt();
        return {};
    };

    // execute first step in io thread..
    // second in engine thread.
    // and the final in js thread (with threadsafefunction)
    auto ioqueue = META_NS::GetTaskQueueRegistry().GetTaskQueue(IO_QUEUE);
    auto enginequeue = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
    ioqueue->AddWaitableTask(META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>(BASE_NS::move(fun)))
        ->Then(META_NS::MakeCallback<META_NS::IFutureContinuation>(BASE_NS::move(fun2)), enginequeue);

    return result;
}
napi_value SceneJS::GetAnimations(NapiApi::FunctionContext<>& ctx)
{
    auto scene = interface_pointer_cast<SCENE_NS::IScene>(GetThisNativeObject(ctx));
    if (!scene) {
        return ctx.GetUndefined();
    }

    BASE_NS::vector<META_NS::IAnimation::Ptr> animRes;
    ExecSyncTask([scene, &animRes]() {
        animRes = scene->GetAnimations();
        return META_NS::IAny::Ptr {};
    });

    napi_value tmp;
    auto status = napi_create_array_with_length(ctx, animRes.size(), &tmp);
    size_t i = 0;
    napi_value args[] = { ctx.This() };
    for (const auto& node : animRes) {
        napi_value val;
        val = CreateFromNativeInstance(
            ctx, interface_pointer_cast<META_NS::IObject>(node), false, BASE_NS::countof(args), args);
        status = napi_set_element(ctx, tmp, i++, val);

        // disposables_[
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
    strongDisposables_[token] = { obj };
}
void SceneJS::ReleaseStrongDispose(uintptr_t token)
{
    auto it = strongDisposables_.find(token);
    if (it != strongDisposables_.end()) {
        it->second.Reset();
        strongDisposables_.erase(it->first); 
    }
}
