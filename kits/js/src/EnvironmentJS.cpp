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
#include "EnvironmentJS.h"

#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/property_events.h>
#include <scene_plugin/api/camera.h> // for the classid..
#include <scene_plugin/api/node_uid.h>
#include <scene_plugin/interface/intf_ecs_scene.h>
#include <scene_plugin/interface/intf_node.h>
#include <scene_plugin/interface/intf_scene.h>

#include <render/intf_render_context.h>

#include "SceneJS.h"
using namespace SCENE_NS;

void EnvironmentJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> node_props;
    SceneResourceImpl::GetPropertyDescs(node_props);
    // clang-format off

    node_props.emplace_back(GetSetProperty<uint32_t, EnvironmentJS, &EnvironmentJS::GetBackgroundType,
        &EnvironmentJS::SetBackgroundType>("backgroundType"));
    node_props.emplace_back(GetSetProperty<Object, EnvironmentJS, &EnvironmentJS::GetEnvironmentImage,
        &EnvironmentJS::SetEnvironmentImage>("environmentImage"));
    node_props.emplace_back(GetSetProperty<Object, EnvironmentJS, &EnvironmentJS::GetRadianceImage,
        &EnvironmentJS::SetRadianceImage>("radianceImage"));
    node_props.emplace_back(GetSetProperty<NapiApi::Array, EnvironmentJS, &EnvironmentJS::GetIrradianceCoefficients,
        &EnvironmentJS::SetIrradianceCoefficients>("irradianceCoefficients"));
    node_props.emplace_back(GetSetProperty<Object, EnvironmentJS, &EnvironmentJS::GetIndirectDiffuseFactor,
        &EnvironmentJS::SetIndirectDiffuseFactor>("indirectDiffuseFactor"));
    node_props.emplace_back(GetSetProperty<Object, EnvironmentJS, &EnvironmentJS::GetIndirectSpecularFactor,
        &EnvironmentJS::SetIndirectSpecularFactor>("indirectSpecularFactor"));
    node_props.emplace_back(GetSetProperty<Object, EnvironmentJS, &EnvironmentJS::GetEnvironmentMapFactor,
        &EnvironmentJS::SetEnvironmentMapFactor>("environmentMapFactor"));

    // clang-format on

    napi_value func;
    auto status = napi_define_class(env, "Environment", NAPI_AUTO_LENGTH, BaseObject::ctor<EnvironmentJS>(), nullptr,
        node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    napi_get_instance_data(env, (void**)&mis);
    mis->StoreCtor("Environment", func);

    NapiApi::Object exp(env, exports);

    napi_value eType; 
    napi_value v;
    napi_create_object(env, &eType);
#define DECL_ENUM(enu, x)                                      \
    napi_create_uint32(env, EnvironmentBackgroundType::x, &v); \
    napi_set_named_property(env, enu, #x, v)

    DECL_ENUM(eType, BACKGROUND_NONE);
    DECL_ENUM(eType, BACKGROUND_IMAGE);
    DECL_ENUM(eType, BACKGROUND_CUBEMAP);
    DECL_ENUM(eType, BACKGROUND_EQUIRECTANGULAR);
#undef DECL_ENUM
    exp.Set("EnvironmentBackgroundType", eType);
}

napi_value EnvironmentJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    LOG_F("EnvironmentJS::Dispose");
    DisposeNative();
    return {};
}
void EnvironmentJS::DisposeNative()
{
    if (!disposed_) {
        CORE_LOG_F("EnvironmentJS::DisposeNative");
        disposed_ = true;
        NapiApi::Object obj = scene_.GetObject();
        auto* tro = obj.Native<TrueRootObject>();
        if (tro) {
            SceneJS* sceneJS = ((SceneJS*)tro->GetInstanceImpl(SceneJS::ID));
            if (sceneJS) {
                sceneJS->ReleaseStrongDispose((uintptr_t)&scene_);
            }
        }

        diffuseFactor_.reset();
        specularFactor_.reset();
        environmentFactor_.reset();
        if (auto env = interface_pointer_cast<IEnvironment>(GetNativeObject())) {
            // reset the native object refs
            SetNativeObject(nullptr, false);
            SetNativeObject(nullptr, true);
            diffuseFactor_.reset();
            specularFactor_.reset();
            environmentFactor_.reset();

            NapiApi::Object sceneJS = scene_.GetObject();
            if (sceneJS) {
                napi_value null;
                napi_get_null(sceneJS.GetEnv(), &null);
                sceneJS.Set("environment", null);

                scene_.Reset();
                auto* tro = sceneJS.Native<TrueRootObject>();
                IScene::Ptr scene = interface_pointer_cast<IScene>(tro->GetNativeObject());
                ExecSyncTask([s = BASE_NS::move(scene), e = BASE_NS::move(env)]() {
                    auto en = interface_pointer_cast<SCENE_NS::INode>(e);
                    s->ReleaseNode(en);
                    en.reset();
                    return META_NS::IAny::Ptr {};
                });
            }
        }
    }
    scene_.Reset();
}
void* EnvironmentJS::GetInstanceImpl(uint32_t id)
{
    if (id == EnvironmentJS::ID) {
        return this;
    }
    return SceneResourceImpl::GetInstanceImpl(id);
}
void EnvironmentJS::Finalize(napi_env env)
{
    // hmm.. do i need to do something BEFORE the object gets deleted..
    DisposeNative();
    BaseObject<EnvironmentJS>::Finalize(env);
}

EnvironmentJS::EnvironmentJS(napi_env e, napi_callback_info i)
    : BaseObject<EnvironmentJS>(e, i), SceneResourceImpl(SceneResourceImpl::ENVIRONMENT)
{
    LOG_F("EnvironmentJS ++");
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        // no arguments. so internal create.
        // expecting caller to finish
        return;
    }

    scene_ = { fromJs, fromJs.Arg<0>() };
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        CORE_LOG_F("INVALID SCENE!");
    }

    NapiApi::Object meJs(e, fromJs.This());
    auto* tro = scene_.GetObject().Native<TrueRootObject>();
    auto* sceneJS = ((SceneJS*)tro->GetInstanceImpl(SceneJS::ID));
    sceneJS->StrongDisposeHook((uintptr_t)&scene_, meJs);

    IScene::Ptr scene = interface_pointer_cast<IScene>(tro->GetNativeObject());

    NapiApi::Value<BASE_NS::string> name;
    NapiApi::Object args = fromJs.Arg<1>();
    if (auto prm = args.Get("name")) {
        name = NapiApi::Value<BASE_NS::string>(e, prm);
    }

    BASE_NS::string nameS = name;
    if (nameS.empty()) {
        // create "unique" name
        nameS = BASE_NS::to_string((uint64_t)this);
    }
    IEnvironment::Ptr env = GetNativeMeta<IEnvironment>(meJs);
    // Construct native object (if needed)

    if (!env) {
        ExecSyncTask([&env, scene, nameS]() {
            BASE_NS::string_view n = nameS; /*nodepath actually*/
            env = scene->CreateNode<SCENE_NS::IEnvironment>(nameS);
            return META_NS::IAny::Ptr {};
        });
    }

    // process constructor args
    // weak ref, due to being owned by the scene.
    SetNativeObject(interface_pointer_cast<META_NS::IObject>(env), false);
    StoreJsObj(interface_pointer_cast<META_NS::IObject>(env), meJs);
    env.reset();

    if (name) {
        // set the name of the object. if we were given one
        meJs.Set("name", name);
    }
}

EnvironmentJS::~EnvironmentJS()
{
    LOG_F("EnvironmentJS --");
    DisposeNative();
    if (!GetNativeObject()) {
        return;
    }
}

napi_value EnvironmentJS::GetBackgroundType(NapiApi::FunctionContext<>& ctx)
{
    uint32_t typeI = 0;
    if (auto env = interface_cast<IEnvironment>(GetNativeObject())) {
        ExecSyncTask([env, &typeI]() {
            typeI = env->Background()->GetValue();
            return META_NS::IAny::Ptr {};
        });
    }
    return NapiApi::Value(ctx, (uint32_t)typeI);
}

void EnvironmentJS::SetBackgroundType(NapiApi::FunctionContext<uint32_t>& ctx)
{
    if (auto env = interface_cast<IEnvironment>(GetNativeObject())) {
        uint32_t typeI = ctx.Arg<0>();
        auto typeE = static_cast<EnvironmentBackgroundType>(typeI);
        IEnvironment::BackgroundType type;
        switch (typeE) {
            case EnvironmentBackgroundType::BACKGROUND_NONE:
                type = IEnvironment::BackgroundType::NONE;
                break;
            case EnvironmentBackgroundType::BACKGROUND_IMAGE:
                type = IEnvironment::BackgroundType::IMAGE;
                break;
            case EnvironmentBackgroundType::BACKGROUND_CUBEMAP:
                type = IEnvironment::BackgroundType::CUBEMAP;
                break;
            case EnvironmentBackgroundType::BACKGROUND_EQUIRECTANGULAR:
                type = IEnvironment::BackgroundType::EQUIRECTANGULAR;
                break;
            default:
                type = IEnvironment::BackgroundType::NONE;
                break;
        }
        ExecSyncTask([env, &type]() {
            env->Background()->SetValue(type);
            return META_NS::IAny::Ptr {};
        });
    }
}
napi_value EnvironmentJS::GetEnvironmentImage(NapiApi::FunctionContext<>& ctx)
{
    if (auto environment = interface_cast<SCENE_NS::IEnvironment>(GetNativeObject())) {
        SCENE_NS::IBitmap::Ptr image;
        ExecSyncTask([environment, &image]() {
            image = environment->EnvironmentImage()->GetValue();
            return META_NS::IAny::Ptr {};
        });
        auto obj = interface_pointer_cast<META_NS::IObject>(image);

        if (auto cached = FetchJsObj(obj)) {
            return cached;
        }

        napi_value args[] = { scene_.GetValue(), NapiApi::Object(ctx) };
        return CreateFromNativeInstance(ctx, obj, false, BASE_NS::countof(args), args);
    }
    return ctx.GetNull();
}

void EnvironmentJS::SetEnvironmentImage(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    NapiApi::Object imageJS = ctx.Arg<0>();
    SCENE_NS::IBitmap::Ptr image;
    if (auto nat = imageJS.Native<TrueRootObject>()) {
        image = interface_pointer_cast<SCENE_NS::IBitmap>(nat->GetNativeObject());
    }
    if (auto environment = interface_cast<SCENE_NS::IEnvironment>(GetNativeObject())) {
        ExecSyncTask([environment, image]() {
            environment->EnvironmentImage()->SetValue(image);
            return META_NS::IAny::Ptr {};
        });
    }
}

napi_value EnvironmentJS::GetRadianceImage(NapiApi::FunctionContext<>& ctx)
{
    if (auto environment = interface_cast<SCENE_NS::IEnvironment>(GetNativeObject())) {
        SCENE_NS::IBitmap::Ptr image;
        ExecSyncTask([environment, &image]() {
            image = environment->RadianceImage()->GetValue();
            return META_NS::IAny::Ptr {};
        });
        auto obj = interface_pointer_cast<META_NS::IObject>(image);

        if (auto cached = FetchJsObj(obj)) {
            return cached;
        }

        napi_value args[] = { scene_.GetValue(), NapiApi::Object(ctx) };
        return CreateFromNativeInstance(ctx, obj, false, BASE_NS::countof(args), args);
    }
    return ctx.GetNull();
}

void EnvironmentJS::SetRadianceImage(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    NapiApi::Object imageJS = ctx.Arg<0>();
    SCENE_NS::IBitmap::Ptr image;
    if (auto nat = imageJS.Native<TrueRootObject>()) {
        image = interface_pointer_cast<SCENE_NS::IBitmap>(nat->GetNativeObject());
    }
    if (auto environment = interface_cast<SCENE_NS::IEnvironment>(GetNativeObject())) {
        ExecSyncTask([environment, image]() {
            environment->RadianceImage()->SetValue(image);
            return META_NS::IAny::Ptr {};
        });
    }
}
napi_value EnvironmentJS::GetIrradianceCoefficients(NapiApi::FunctionContext<>& ctx)
{
    BASE_NS::vector<BASE_NS::Math::Vec3> coeffs;
    if (auto environment = interface_cast<SCENE_NS::IEnvironment>(GetNativeObject())) {
        ExecSyncTask([environment, &coeffs]() {
            coeffs = environment->IrradianceCoefficients()->GetValue();
            return META_NS::IAny::Ptr {};
        });
    }
    NapiApi::Array res(ctx, 9); // array size 9
    size_t index = 0;
    for (auto& v : coeffs) {
        NapiApi::Object vec(ctx);
        vec.Set("x", NapiApi::Value<float>(ctx, v.x));
        vec.Set("y", NapiApi::Value<float>(ctx, v.y));
        vec.Set("z", NapiApi::Value<float>(ctx, v.z));
        res.Set(index++, vec);
    }
    return res;
}
void EnvironmentJS::SetIrradianceCoefficients(NapiApi::FunctionContext<NapiApi::Array>& ctx)
{
    NapiApi::Array coeffJS = ctx.Arg<0>();
    if (coeffJS.Count() != 9) { // array size 9
        // not enough elements in array
        return;
    }
    BASE_NS::vector<BASE_NS::Math::Vec3> coeffs;
    for (auto i = 0; i < coeffJS.Count(); i++) {
        NapiApi::Object obj = coeffJS.Get<NapiApi::Object>(i);
        if (!obj) {
            // not an object in array
            return;
        }
        auto x = obj.Get<float>("x");
        auto y = obj.Get<float>("y");
        auto z = obj.Get<float>("z");
        if (!x || !y || !z) {
            // invalid kind of object.
            return;
        }
        coeffs.emplace_back((float)x, (float)y, (float)z);
    }

    if (auto environment = interface_cast<SCENE_NS::IEnvironment>(GetNativeObject())) {
        ExecSyncTask([environment, &coeffs]() {
            environment->IrradianceCoefficients()->SetValue(coeffs);
            return META_NS::IAny::Ptr {};
        });
    }
}

napi_value EnvironmentJS::GetIndirectDiffuseFactor(NapiApi::FunctionContext<>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::IEnvironment>(GetThisNativeObject(ctx));
    if (!node) {
        return ctx.GetUndefined();
    }
    if (diffuseFactor_ == nullptr) {
        diffuseFactor_ = BASE_NS::make_unique<Vec4Proxy>(ctx, node->IndirectDiffuseFactor());
    }
    return *diffuseFactor_;
}

void EnvironmentJS::SetIndirectDiffuseFactor(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::IEnvironment>(GetThisNativeObject(ctx));
    if (!node) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (diffuseFactor_ == nullptr) {
        diffuseFactor_ = BASE_NS::make_unique<Vec4Proxy>(ctx, node->IndirectDiffuseFactor());
    }
    diffuseFactor_->SetValue(obj);
}

napi_value EnvironmentJS::GetIndirectSpecularFactor(NapiApi::FunctionContext<>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::IEnvironment>(GetThisNativeObject(ctx));
    if (!node) {
        return ctx.GetUndefined();
    }
    if (specularFactor_ == nullptr) {
        specularFactor_ = BASE_NS::make_unique<Vec4Proxy>(ctx, node->IndirectSpecularFactor());
    }
    return *specularFactor_;
}

void EnvironmentJS::SetIndirectSpecularFactor(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::IEnvironment>(GetThisNativeObject(ctx));
    if (!node) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (specularFactor_ == nullptr) {
        specularFactor_ = BASE_NS::make_unique<Vec4Proxy>(ctx, node->IndirectSpecularFactor());
    }
    specularFactor_->SetValue(obj);
}

napi_value EnvironmentJS::GetEnvironmentMapFactor(NapiApi::FunctionContext<>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::IEnvironment>(GetThisNativeObject(ctx));
    if (!node) {
        return ctx.GetUndefined();
    }
    if (environmentFactor_ == nullptr) {
        environmentFactor_ = BASE_NS::make_unique<Vec4Proxy>(ctx, node->EnvMapFactor());
    }
    return *environmentFactor_;
}

void EnvironmentJS::SetEnvironmentMapFactor(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::IEnvironment>(GetThisNativeObject(ctx));
    if (!node) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (environmentFactor_ == nullptr) {
        environmentFactor_ = BASE_NS::make_unique<Vec4Proxy>(ctx, node->EnvMapFactor());
    }
    environmentFactor_->SetValue(obj);
}
