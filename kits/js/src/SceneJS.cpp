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
#include "NodeJS.h"
#include "PromiseBase.h"
static constexpr BASE_NS::Uid IO_QUEUE { "be88e9a0-9cd8-45ab-be48-937953dc258f" };

#include <meta/api/make_callback.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/property/property_events.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_render_configuration.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/intf_node_import.h>

#include <core/image/intf_image_loader_manager.h>
#include <core/intf_engine.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#include "scene_adapter/scene_adapter.h"
#endif

// LEGACY COMPATIBILITY start
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_ecs_object_access.h>

#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
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
                tree.insert({ enti });
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

void SceneJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;
    // clang-format off
    auto loadFun = [](napi_env e, napi_callback_info cb) -> napi_value {
        FunctionContext<> fc(e, cb);
        return SceneJS::Load(fc);
    };

    napi_property_descriptor props[] = {
        // static methods
        napi_property_descriptor{ "load", nullptr, loadFun, nullptr, nullptr, nullptr,
            (napi_property_attributes)(napi_static|napi_default_method)},
        // properties
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

        Method<NapiApi::FunctionContext<BASE_NS::string, NapiApi::Object, NapiApi::Object>, SceneJS,
	    &SceneJS::ImportNode>("importNode"),
        Method<NapiApi::FunctionContext<BASE_NS::string, NapiApi::Object, NapiApi::Object>, SceneJS,
	    &SceneJS::ImportScene>("importScene")
    };
    // clang-format on

    napi_value func;
    auto status = napi_define_class(env, "Scene", NAPI_AUTO_LENGTH, BaseObject::ctor<SceneJS>(), nullptr,
        sizeof(props) / sizeof(props[0]), props, &func);

    napi_set_named_property(env, exports, "Scene", func);

    NapiApi::MyInstanceState* mis;
    GetInstanceData(env, reinterpret_cast<void**>(&mis));
    mis->StoreCtor("Scene", func);
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
        uint32_t resourceType = resource.Get<uint32_t>("type");
        NapiApi::Array parms = resource.Get<NapiApi::Array>("params");
        BASE_NS::string uri;
        if ((id == 0) && (resourceType == 30000)) { // 30000: param
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
BASE_NS::string FetchResourceOrUri(const NapiApi::Object obj)
{
    return FetchResourceOrUri(obj.GetEnv(), obj.ToNapiValue());
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
        auto t = uri.find("://");
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
        // try to identify a $rawfile. (from resource object)
        if ((id == 0) && (type == 30000)) { // 30000: param
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

    struct Promise : public PromiseBase {
        using PromiseBase::PromiseBase;
        BASE_NS::string uri;
        SCENE_NS::IScene::Ptr scene;
        bool SetResult() override
        {
            if (scene) {
                auto obj = interface_pointer_cast<META_NS::IObject>(scene);
                auto jsscene = CreateJsObj(env_, "Scene", obj, true, 0, nullptr);
                // link the native object to js object.
                StoreJsObj(obj, jsscene);
                result_ = jsscene.ToNapiValue();

                auto curenv = jsscene.Get<NapiApi::Object>("environment");
                if (curenv.IsUndefinedOrNull()) {
                    // setup default env
                    NapiApi::Object argsIn(env_);
                    argsIn.Set("name", "DefaultEnv");

                    auto* tro = static_cast<SceneJS*>(jsscene.Native<TrueRootObject>());
                    auto res = tro->CreateEnvironment(jsscene, argsIn);
                    res.Set("backgroundType", NapiApi::Value<uint32_t>(env_, 1)); // image.. but with null.
                    jsscene.Set("environment", res);
                }
                for (auto&& c : scene->GetCameras().GetResult()) {
                    c->RenderingPipeline()->SetValue(SCENE_NS::CameraPipeline::FORWARD);
                    c->ColorTargetCustomization()->SetValue(
                        { SCENE_NS::ColorFormat{BASE_NS::BASE_FORMAT_R16G16B16A16_SFLOAT}});
                }

#ifdef __SCENE_ADAPTER__
                // set SceneAdapter
                auto oo = GetRootObject(NapiApi::Object(env_, result_));
                if (oo == nullptr) {
                    CORE_LOG_E("GetRootObject return nullptr in SceneJS Load.");
                    return false;
                }

                auto sceneAdapter = std::make_shared<OHOS::Render3D::SceneAdapter>();
                sceneAdapter->SetSceneObj(oo->GetNativeObject());
                auto sceneJs = static_cast<SceneJS*>(oo);
                sceneJs->scene_ = sceneAdapter;
#endif
                return true;
            }
            return false;
        };
    };
    Promise* promise = new Promise(ctx.Env());
    promise->uri = uri;

    auto jsPromise = promise->ToNapiValue();

    auto params = interface_pointer_cast<META_NS::IMetadata>(META_NS::GetObjectRegistry().GetDefaultObjectContext());
    auto m = META_NS::GetObjectRegistry().Create<SCENE_NS::ISceneManager>(SCENE_NS::ClassId::SceneManager, params);
    if (m) {
        m->CreateScene(uri).Then(
            [promise](SCENE_NS::IScene::Ptr scene) {
                if (scene) {
                    promise->scene = scene;
                    auto& obr = META_NS::GetObjectRegistry();
                    // make sure we have renderconfig
                    auto rc = scene->RenderConfiguration()->GetValue();
                    if (!rc) {
                        LOG_F("No render config");
                    }
                    // LEGACY COMPATIBILITY start
                    Fixnames(scene);
                    // LEGACY COMPATIBILITY end
                    AddScene(&obr, scene);
                }
                promise->SettleLater();
            },
            nullptr);
    } else {
        promise->Reject();
    }
    return jsPromise;
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
                NapiApi::Function func = env.Get<NapiApi::Function>("destroy");
                if (func) {
                    func.Invoke(env, 1, &scene);
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

        for (auto b : bitmaps_) {
            b.second.reset();
        }
        if (auto nativeScene = interface_pointer_cast<SCENE_NS::IScene>(GetNativeObject())) {
            // reset the native object refs
            SetNativeObject(nullptr, false);
            SetNativeObject(nullptr, true);

            auto r = nativeScene->RenderConfiguration()->GetValue();
            if (r) {
                r->Environment()->SetValue(nullptr);
                r.reset();
            }
            FlushScenes();
        }
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
    // hmm.. do i need to do something BEFORE the object gets deleted..
    BaseObject<SceneJS>::Finalize(env);
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

SceneJS::SceneJS(napi_env e, napi_callback_info i) : BaseObject<SceneJS>(e, i)
{
    LOG_V("SceneJS ++");
    env_ = e; // store..
    NapiApi::FunctionContext<NapiApi::Object> fromJs(e, i);
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
    if (path.empty() || (path == BASE_NS::string_view("/")) || (path==rootName)) {
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
        auto obj = interface_pointer_cast<META_NS::IObject>(root);

        if (auto cached = FetchJsObj(obj)) {
            // always return the same js object.
            return cached.ToNapiValue();
        }

        NapiApi::StrongRef sceneRef { ctx.This() };
        if (!GetNativeMeta<SCENE_NS::IScene>(sceneRef.GetObject())) {
            LOG_F("INVALID SCENE!");
        }

        NapiApi::Object argJS(ctx.GetEnv());
        napi_value args[] = { sceneRef.GetObject().ToNapiValue(), argJS.ToNapiValue() };

        return CreateFromNativeInstance(
            ctx.GetEnv(), obj, false /*these are owned by the scene*/, BASE_NS::countof(args), args)
            .ToNapiValue();
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
            auto obj = interface_pointer_cast<META_NS::IObject>(environment);
            if (auto cached = FetchJsObj(obj)) {
                // always return the same js object.
                return cached.ToNapiValue();
            }

            NapiApi::StrongRef sceneRef { ctx.This() };
            if (!GetNativeMeta<SCENE_NS::IScene>(sceneRef.GetObject())) {
                LOG_F("INVALID SCENE!");
            }

            NapiApi::Env env(ctx.Env());
            NapiApi::Object argJS(env);
            napi_value args[] = { sceneRef.GetObject().ToNapiValue(), argJS.ToNapiValue() };

            environmentJS_ =
                NapiApi::StrongRef(CreateFromNativeInstance(env, obj, false, BASE_NS::countof(args), args));
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

    SCENE_NS::IEnvironment::Ptr environment = GetNativeMeta<SCENE_NS::IEnvironment>(envObj);

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
    return NapiApi::Object(GetJSConstructor(env, "Environment"), BASE_NS::countof(args), args);
}
napi_value SceneJS::CreateEnvironment(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    struct Promise : public PromiseBase {
        using PromiseBase::PromiseBase;
        NapiApi::StrongRef this_;
        NapiApi::StrongRef args_;
        bool SetResult() override
        {
            auto* tro = static_cast<SceneJS*>(this_.GetObject().Native<TrueRootObject>());
            result_ = tro->CreateEnvironment(this_.GetObject(), args_.GetObject()).ToNapiValue();
            return true;
        };
    };
    Promise* promise = new Promise(ctx.Env());
    auto jsPromise = promise->ToNapiValue();
    promise->this_ = NapiApi::StrongRef(ctx.This());
    promise->args_ = NapiApi::StrongRef(ctx.Arg<0>());

    promise->SettleNow();
    return jsPromise;
}

napi_value SceneJS::CreateCamera(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    struct Promise : public PromiseBase {
        using PromiseBase::PromiseBase;
        NapiApi::StrongRef this_;
        NapiApi::StrongRef args_;
        bool SetResult() override
        {
            napi_value args[] = {
                this_.GetValue(), // scene..
                args_.GetValue()  // params.
            };
            result_ = NapiApi::Object(GetJSConstructor(env_, "Camera"), BASE_NS::countof(args), args).ToNapiValue();
            return true;
        };
    };
    Promise* promise = new Promise(ctx.Env());
    auto jsPromise = promise->ToNapiValue();
    promise->this_ = NapiApi::StrongRef(ctx.This());
    promise->args_ = NapiApi::StrongRef(ctx.Arg<0>());

    promise->SettleNow();
    return jsPromise;
}

napi_value SceneJS::CreateLight(NapiApi::FunctionContext<NapiApi::Object, uint32_t>& ctx)
{
    struct Promise : public PromiseBase {
        using PromiseBase::PromiseBase;
        NapiApi::StrongRef this_;
        NapiApi::StrongRef args_;
        uint32_t lightType_;
        bool SetResult() override
        {
            napi_value args[] = {
                this_.GetValue(), // scene..
                args_.GetValue()  // params.
            };
            NapiApi::Function func;
            switch (lightType_) {
                case BaseLight::DIRECTIONAL: {
                    func = GetJSConstructor(env_, "DirectionalLight");
                    break;
                }
                case BaseLight::POINT: {
                    func = GetJSConstructor(env_, "PointLight");
                    break;
                }
                case BaseLight::SPOT: {
                    func = GetJSConstructor(env_, "SpotLight");
                    break;
                }
                default:
                    break;
            }
            if (!func) {
                return false;
            }
            result_ = NapiApi::Object(func, BASE_NS::countof(args), args).ToNapiValue();
            return true;
        };
    };
    Promise* promise = new Promise(ctx.Env());
    auto jsPromise = promise->ToNapiValue();
    promise->this_ = NapiApi::StrongRef(ctx.This());
    promise->args_ = NapiApi::StrongRef(ctx.Arg<0>());
    promise->lightType_ = ctx.Arg<1>();

    promise->SettleNow();
    return jsPromise;
}

napi_value SceneJS::CreateNode(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    struct Promise : public PromiseBase {
        using PromiseBase::PromiseBase;
        NapiApi::StrongRef this_;
        NapiApi::StrongRef args_;
        bool SetResult() override
        {
            napi_value args[] = {
                this_.GetValue(), // scene..
                args_.GetValue()  // params.
            };
            result_ = NapiApi::Object(GetJSConstructor(env_, "Node"), BASE_NS::countof(args), args).ToNapiValue();
            return true;
        };
    };
    Promise* promise = new Promise(ctx.Env());
    auto jsPromise = promise->ToNapiValue();
    promise->this_ = NapiApi::StrongRef(ctx.This());
    promise->args_ = NapiApi::StrongRef(ctx.Arg<0>());

    promise->SettleNow();
    return jsPromise;
}

napi_value SceneJS::CreateTextNode(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    struct Promise : public PromiseBase {
        using PromiseBase::PromiseBase;
        NapiApi::StrongRef this_;
        NapiApi::StrongRef args_;
        bool SetResult() override
        {
            napi_value args[] = {
                this_.GetValue(), // scene..
                args_.GetValue()  // params.
            };
            result_ = NapiApi::Object(GetJSConstructor(env_, "TextNode"), BASE_NS::countof(args), args).ToNapiValue();
            return true;
        };
    };
    Promise* promise = new Promise(ctx.Env());
    auto jsPromise = promise->ToNapiValue();
    promise->this_ = NapiApi::StrongRef(ctx.This());
    promise->args_ = NapiApi::StrongRef(ctx.Arg<0>());

    promise->SettleNow();
    return jsPromise;
}

napi_value SceneJS::CreateMaterial(NapiApi::FunctionContext<NapiApi::Object, uint32_t>& ctx)
{
    struct Promise : public PromiseBase {
        using PromiseBase::PromiseBase;
        NapiApi::StrongRef this_;
        NapiApi::StrongRef args_;
        uint32_t type_;
        BASE_NS::string name_;
        SCENE_NS::IMaterial::Ptr material_;
        SCENE_NS::IScene::Ptr scene_;
        bool SetResult() override
        {
            napi_value args[] = {
                this_.GetValue(), // scene..
                args_.GetValue()  // params.
            };

            if (type_ == BaseMaterial::SHADER) {
                MakeNativeObjectParam(env_, material_, BASE_NS::countof(args), args);
                NapiApi::Object materialJS(GetJSConstructor(env_, "ShaderMaterial"), BASE_NS::countof(args), args);
                result_ = materialJS.ToNapiValue();
            } else {
                // fail..
                material_.reset();
            }
            return true;
        };
    };
    Promise* promise = new Promise(ctx.Env());
    auto jsPromise = promise->ToNapiValue();
    promise->this_ = NapiApi::StrongRef(ctx.This());
    promise->args_ = NapiApi::StrongRef(ctx.Arg<0>());
    promise->type_ = ctx.Arg<1>();
    NapiApi::Object parms = ctx.Arg<0>();
    if (parms) {
        promise->name_ = parms.Get<BASE_NS::string>("name");
    }

    promise->scene_ = interface_pointer_cast<SCENE_NS::IScene>(GetNativeObject());

    // create an engine task and complete it there..
    auto fun = [promise]() {
        promise->material_ =
            promise->scene_->CreateObject<SCENE_NS::IMaterial>(SCENE_NS::ClassId::Material).GetResult();
        promise->SettleLater();
        return false;
    };

    META_NS::GetTaskQueueRegistry()
        .GetTaskQueue(ENGINE_THREAD)
        ->AddTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>(BASE_NS::move(fun)));

    return jsPromise;
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
    SCENE_NS::INode::Ptr node = GetNativeMeta<SCENE_NS::INode>(nnode);
    SCENE_NS::INode::Ptr parent;
    if (nparent) {
        parent = GetNativeMeta<SCENE_NS::INode>(nparent);
    } else {
        parent = scene->GetRootNode().GetResult();
    }

    if (auto import = interface_cast<SCENE_NS::INodeImport>(parent)) {
        auto result = import->ImportChild(node).GetResult();
        if (!name.empty()) {
            result->SetName(name).Wait();
        }
        auto obj = interface_pointer_cast<META_NS::IObject>(result);
        if (auto cached = FetchJsObj(obj)) {
            // always return the same js object.
            return cached.ToNapiValue();
        }

        NapiApi::StrongRef sceneRef { ctx.This() };
        NapiApi::Object argJS(ctx.GetEnv());
        napi_value args[] = { sceneRef.GetObject().ToNapiValue(), argJS.ToNapiValue() };

        return CreateFromNativeInstance(
            ctx.GetEnv(), obj, false /*these are owned by the scene*/, BASE_NS::countof(args), args)
            .ToNapiValue();
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
    SCENE_NS::IScene::Ptr extScene = GetNativeMeta<SCENE_NS::IScene>(nextScene);
    SCENE_NS::INode::Ptr parent;
    if (nparent) {
        parent = GetNativeMeta<SCENE_NS::INode>(nparent);
    } else {
        parent = scene->GetRootNode().GetResult();
    }

    if (auto import = interface_cast<SCENE_NS::INodeImport>(parent)) {
        auto result = import->ImportChildScene(extScene, name).GetResult();
        auto obj = interface_pointer_cast<META_NS::IObject>(result);
        if (auto cached = FetchJsObj(obj)) {
            // always return the same js object.
            return cached.ToNapiValue();
        }

        NapiApi::StrongRef sceneRef { ctx.This() };
        NapiApi::Object argJS(ctx.GetEnv());
        napi_value args[] = { sceneRef.GetObject().ToNapiValue(), argJS.ToNapiValue() };

        return CreateFromNativeInstance(
            ctx.GetEnv(), obj, false /*these are owned by the scene*/, BASE_NS::countof(args), args)
            .ToNapiValue();
    }
    return ctx.GetNull();
}

napi_value SceneJS::CreateShader(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    struct Promise : public PromiseBase {
        using PromiseBase::PromiseBase;
        NapiApi::StrongRef this_;
        NapiApi::StrongRef args_;
        BASE_NS::string uri_;
        BASE_NS::string name_;
        SCENE_NS::IShader::Ptr shader_;
        SCENE_NS::IScene::Ptr scene_;
        bool SetResult() override
        {
            napi_value args[] = {
                this_.GetValue(), // scene..
                args_.GetValue()  // params.
            };
            NapiApi::Object parms(env_, args[1]);

            napi_value null;
            napi_get_null(env_, &null);
            parms.Set("Material", null); // not bound to anything...

            MakeNativeObjectParam(env_, shader_, BASE_NS::countof(args), args);
            NapiApi::Object shaderJS(GetJSConstructor(env_, "Shader"), BASE_NS::countof(args), args);
            result_ = shaderJS.ToNapiValue();

            return true;
        };
    };
    Promise* promise = new Promise(ctx.Env());
    auto jsPromise = promise->ToNapiValue();
    promise->this_ = NapiApi::StrongRef(ctx.This());
    promise->args_ = NapiApi::StrongRef(ctx.Arg<0>());
    promise->scene_ = interface_pointer_cast<SCENE_NS::IScene>(GetNativeObject());

    NapiApi::Object parms = ctx.Arg<0>();
    if (parms) {
        promise->name_ = parms.Get<BASE_NS::string>("name");
        promise->uri_ = FetchResourceOrUri(ctx.Env(), parms.Get("uri"));
    }

    auto fun = [promise]() {
        promise->shader_ = META_NS::GetObjectRegistry().Create<SCENE_NS::IShader>(SCENE_NS::ClassId::Shader);
        if (!promise->shader_->LoadShader(promise->scene_, promise->uri_).GetResult()) {
            LOG_W("Failed to load shader: %s", promise->uri_.c_str());
        }
        promise->SettleLater();
        return false;
    };

    META_NS::GetTaskQueueRegistry()
        .GetTaskQueue(ENGINE_THREAD)
        ->AddTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>(BASE_NS::move(fun)));

    return jsPromise;
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

    struct Promise : public PromiseBase {
        using PromiseBase::PromiseBase;
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
        SCENE_NS::IScene::Ptr scene_;
        bool SetResult() override
        {
            if (!bitmap_) {
                // return the error string..
                NapiApi::Env e(env_);
                result_ = e.GetString(imageLoadResult_.error);
                return false;
            }
            if (cached_) {
                auto obj = interface_pointer_cast<META_NS::IObject>(bitmap_);
                result_ = FetchJsObj(obj).ToNapiValue();
            } else {
                // create the jsobject if we don't have one.
                napi_value args[] = {
                    this_.GetValue(), // scene..
                    args_.GetValue()  // params.
                };
                MakeNativeObjectParam(env_, bitmap_, BASE_NS::countof(args), args);
                owner_->StoreBitmap(uri_, BASE_NS::move(bitmap_));
                NapiApi::Object imageJS(GetJSConstructor(env_, "Image"), BASE_NS::countof(args), args);
                result_ = imageJS.ToNapiValue();
            }
            return true;
        };
    };
    Promise* promise = new Promise(ctx.Env());
    auto jsPromise = promise->ToNapiValue();
    promise->owner_ = this;
    promise->this_ = NapiApi::StrongRef(ctx.This());
    promise->args_ = NapiApi::StrongRef(ctx.Arg<0>());
    promise->scene_ = interface_pointer_cast<SCENE_NS::IScene>(GetNativeObject());

    NapiApi::Object args = ctx.Arg<0>();
    if (args) {
        if (auto n = args.Get<BASE_NS::string>("name"); n.IsDefined()) {
            promise->name_ = n;
        }
        promise->uri_ = FetchResourceOrUri(ctx.Env(), args.Get("uri"));
    }

    if (auto bitmap = FetchBitmap(promise->uri_)) {
        // no aliasing.. so the returned bitmaps name is.. the old one.
        // *fix*
        // oh we have it already, no need to do anything in engine side.
        promise->cached_ = true;
        promise->bitmap_ = bitmap;
        auto jsPromise = promise->ToNapiValue();
        promise->SettleNow();
        return jsPromise;
    }

    auto& obr = META_NS::GetObjectRegistry();
    auto doc = interface_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
    promise->renderContext_ =
        interface_pointer_cast<IRenderContext>(doc->GetProperty<IntfPtr>("RenderContext")->GetValue());

    // create an IO task (to load the cpu data)
    auto fun = [promise]() -> META_NS::IAny::Ptr {
        uint32_t imageLoaderFlags = CORE_NS::IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS;
        auto& imageLoaderMgr = promise->renderContext_->GetEngine().GetImageLoaderManager();
        promise->imageLoadResult_ = imageLoaderMgr.LoadImage(promise->uri_, imageLoaderFlags);
        return {};
    };

    // create final engine task (to create gpu resource)
    auto fun2 = [promise](const META_NS::IAny::Ptr&) -> META_NS::IAny::Ptr {
        if (!promise->imageLoadResult_.success) {
            LOG_E("Could not load image asset: %s", promise->imageLoadResult_.error);
            promise->bitmap_ = nullptr;
        } else {
            auto& gpuResourceMgr = promise->renderContext_->GetDevice().GetGpuResourceManager();
            RenderHandleReference imageHandle {};
            GpuImageDesc gpuDesc = gpuResourceMgr.CreateGpuImageDesc(promise->imageLoadResult_.image->GetImageDesc());
            gpuDesc.usageFlags = CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
            if (gpuDesc.engineCreationFlags & EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS) {
                gpuDesc.usageFlags |= CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
            }
            gpuDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            promise->imageHandle_ =
                gpuResourceMgr.Create(promise->uri_, gpuDesc, std::move(promise->imageLoadResult_.image));
            promise->bitmap_ = META_NS::GetObjectRegistry().Create<SCENE_NS::IBitmap>(SCENE_NS::ClassId::Bitmap);
            if (auto m = interface_cast<META_NS::IMetadata>(promise->bitmap_)) {
                auto uri = META_NS::ConstructProperty<BASE_NS::string>("Uri", promise->uri_);
                m->AddProperty(uri);
            }
            if (auto i = interface_cast<SCENE_NS::IRenderResource>(promise->bitmap_)) {
                i->SetRenderHandle(promise->scene_->GetInternalScene(), promise->imageHandle_);
            }
        }
        promise->SettleLater();
        return {};
    };

    // execute first step in io thread..
    // second in engine thread.
    // and the final in js thread (with threadsafefunction)
    auto ioqueue = META_NS::GetTaskQueueRegistry().GetTaskQueue(IO_QUEUE);
    auto enginequeue = META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD);
    ioqueue->AddWaitableTask(META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>(BASE_NS::move(fun)))
        ->Then(META_NS::MakeCallback<META_NS::IFutureContinuation>(BASE_NS::move(fun2)), enginequeue);

    return jsPromise;
}
napi_value SceneJS::GetAnimations(NapiApi::FunctionContext<>& ctx)
{
    auto scene = interface_pointer_cast<SCENE_NS::IScene>(GetThisNativeObject(ctx));
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
        auto val = CreateFromNativeInstance(
            ctx.Env(), interface_pointer_cast<META_NS::IObject>(node), true, BASE_NS::countof(args), args);
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

#ifdef __OHOS_PLATFORM__
// This will circumvent the broken napi_set_instance_data and napi_get_instance_data implementations
// on ohos platform
static void* g_instanceData = nullptr;

napi_status SetInstanceData(napi_env env, void* data, napi_finalize finalizeCb, void* finalizeHint)
{
    g_instanceData = data;
    return napi_ok;
}

napi_status GetInstanceData(napi_env env, void** data)
{
    if (data) {
        *data = g_instanceData;
    }

    return napi_ok;
}

#else // __OHOS_PLATFORM__

napi_status SetInstanceData(napi_env env, void* data, napi_finalize finalizeCb, void* finalizeHint)
{
    return napi_set_instance_data(env, data, finalizeCb, finalizeHint);
}

napi_status GetInstanceData(napi_env env, void** data)
{
    return napi_get_instance_data(env, data);
}

#endif // __OHOS_PLATFORM__

