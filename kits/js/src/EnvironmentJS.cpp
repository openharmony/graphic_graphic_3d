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
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_scene.h>

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
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    if (mis) {
        mis->StoreCtor("Environment", func);
    }

    NapiApi::Object exp(env, exports);

    napi_value eType = nullptr;
    napi_value v = nullptr;
    napi_create_object(env, &eType);
#define DECL_ENUM(enu, x)                                      \
    napi_create_uint32(env, EnvironmentBackgroundType::x, &v); \
    napi_set_named_property(env, enu, #x, v);

    DECL_ENUM(eType, BACKGROUND_NONE);
    DECL_ENUM(eType, BACKGROUND_IMAGE);
    DECL_ENUM(eType, BACKGROUND_CUBEMAP);
    DECL_ENUM(eType, BACKGROUND_EQUIRECTANGULAR);
#undef DECL_ENUM
    exp.Set("EnvironmentBackgroundType", eType);
}

void EnvironmentJS::DisposeNative(void* scene)
{
    if (scene == nullptr) {
        if (!disposed_) {
            LOG_F("EnvironmentJS::DisposeNative but argument NULL");
        }
        return;
    }
    if (!disposed_) {
        LOG_V("EnvironmentJS::DisposeNative");
        disposed_ = true;

        SceneJS* sceneJS { static_cast<SceneJS*>(scene) };
        if (sceneJS) {
            sceneJS->ReleaseStrongDispose(reinterpret_cast<uintptr_t>(&scene_));
        }
        diffuseFactor_.reset();
        specularFactor_.reset();
        environmentFactor_.reset();
        if (auto env = interface_pointer_cast<IEnvironment>(GetNativeObject())) {
            UnsetNativeObject();

            // if we still have javascript scene reference, detach from it.
            // (if not, then scene has died and we are detaching already)
            NapiApi::Object sceneJs = scene_.GetObject();
            if (sceneJs) {
                napi_value null = nullptr;
                napi_get_null(sceneJs.GetEnv(), &null);
                sceneJs.Set("environment", null);
            }
            if (sceneJS) {
                if (auto s = interface_pointer_cast<IScene>(sceneJS->GetNativeObject())) {
                    env->EnvironmentImage()->SetValue(nullptr);
                    env->RadianceImage()->SetValue(nullptr);
                    env.reset();
                    s.reset();
                }
            }
        }
    }
    scene_.Reset();
}
void* EnvironmentJS::GetInstanceImpl(uint32_t id)
{
    if (id == EnvironmentJS::ID)
        return this;
    return SceneResourceImpl::GetInstanceImpl(id);
}
void EnvironmentJS::Finalize(napi_env env)
{
    // hmm.. do i need to do something BEFORE the object gets deleted..
    DisposeNative(scene_.GetObject().GetJsWrapper<SceneJS>());
    BaseObject::Finalize(env);
}

EnvironmentJS::EnvironmentJS(napi_env e, napi_callback_info i)
    : BaseObject(e, i), SceneResourceImpl(SceneResourceImpl::ENVIRONMENT)
{
    LOG_V("EnvironmentJS ++");
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        // no arguments. so internal create.
        // expecting caller to finish
        return;
    }

    scene_ = fromJs.Arg<0>().valueOrDefault();
    if (!scene_.GetObject().GetNative<SCENE_NS::IScene>()) {
        LOG_F("INVALID SCENE!");
    }

    NapiApi::Object meJs(fromJs.This());
    if (const auto sceneJS = scene_.GetObject().GetJsWrapper<SceneJS>()) {
        sceneJS->StrongDisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
    }

    if (!meJs.GetNative<IEnvironment>()) {
        LOG_E("Cannot finish creating an environment: Native environment object missing");
        assert(false);
        return;
    }

    NapiApi::Value<BASE_NS::string> name;
    NapiApi::Object args = fromJs.Arg<1>();
    if (auto prm = args.Get("name")) {
        name = NapiApi::Value<BASE_NS::string>(e, prm);
    }
    if (name.IsDefined()) {
        // set the name of the object. if we were given one
        meJs.Set("name", name);
    }
}

EnvironmentJS::~EnvironmentJS()
{
    LOG_V("EnvironmentJS --");
    DisposeNative(nullptr);
    if (!GetNativeObject()) {
        return;
    }
}

napi_value EnvironmentJS::GetBackgroundType(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    uint32_t typeI = 0;
    if (auto env = interface_cast<IEnvironment>(GetNativeObject())) {
        typeI = uint32_t(env->Background()->GetValue());
    }
    return ctx.GetNumber(static_cast<uint32_t>(typeI));
}

void EnvironmentJS::SetBackgroundType(NapiApi::FunctionContext<uint32_t>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }

    if (auto env = interface_cast<IEnvironment>(GetNativeObject())) {
        uint32_t typeI = ctx.Arg<0>();
        auto typeE = static_cast<EnvironmentBackgroundType>(typeI);
        EnvBackgroundType type;
        switch (typeE) {
            case EnvironmentBackgroundType::BACKGROUND_NONE:
                type = EnvBackgroundType::NONE;
                break;
            case EnvironmentBackgroundType::BACKGROUND_IMAGE:
                type = EnvBackgroundType::IMAGE;
                break;
            case EnvironmentBackgroundType::BACKGROUND_CUBEMAP:
                type = EnvBackgroundType::CUBEMAP;
                break;
            case EnvironmentBackgroundType::BACKGROUND_EQUIRECTANGULAR:
                type = EnvBackgroundType::EQUIRECTANGULAR;
                break;
            default:
                type = EnvBackgroundType::NONE;
                break;
        }
        env->Background()->SetValue(type);
    }
}
napi_value EnvironmentJS::GetEnvironmentImage(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    if (auto environment = interface_cast<SCENE_NS::IEnvironment>(GetNativeObject())) {
        SCENE_NS::IBitmap::Ptr image = environment->EnvironmentImage()->GetValue();
        napi_value args[] = { scene_.GetValue(), NapiApi::Object(ctx.GetEnv()).ToNapiValue() };
        return CreateFromNativeInstance(ctx.Env(), image, PtrType::WEAK, args).ToNapiValue();
    }
    return ctx.GetNull();
}

void EnvironmentJS::SetEnvironmentImage(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    NapiApi::Object imageJS = ctx.Arg<0>();
    SCENE_NS::IBitmap::Ptr image;
    if (auto nat = imageJS.GetRoot()) {
        image = interface_pointer_cast<SCENE_NS::IBitmap>(nat->GetNativeObject());
    }
    if (auto environment = interface_cast<SCENE_NS::IEnvironment>(GetNativeObject())) {
        environment->EnvironmentImage()->SetValue(image);
    }
}

napi_value EnvironmentJS::GetRadianceImage(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    if (auto environment = interface_cast<SCENE_NS::IEnvironment>(GetNativeObject())) {
        SCENE_NS::IBitmap::Ptr image = environment->RadianceImage()->GetValue();
        napi_value args[] = { scene_.GetValue(), NapiApi::Object(ctx.GetEnv()).ToNapiValue() };
        return CreateFromNativeInstance(ctx.GetEnv(), image, PtrType::WEAK, args).ToNapiValue();
    }
    return ctx.GetNull();
}

void EnvironmentJS::SetRadianceImage(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }

    NapiApi::Object imageJS = ctx.Arg<0>();
    SCENE_NS::IBitmap::Ptr image;
    if (auto nat = imageJS.GetRoot()) {
        image = interface_pointer_cast<SCENE_NS::IBitmap>(nat->GetNativeObject());
    }
    if (auto environment = interface_cast<SCENE_NS::IEnvironment>(GetNativeObject())) {
        environment->RadianceImage()->SetValue(image);
    }
}
napi_value EnvironmentJS::GetIrradianceCoefficients(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    BASE_NS::vector<BASE_NS::Math::Vec3> coeffs;
    if (auto environment = interface_cast<SCENE_NS::IEnvironment>(GetNativeObject())) {
        coeffs = environment->IrradianceCoefficients()->GetValue();
    }
    NapiApi::Env env(ctx.Env());
    NapiApi::Array res(env, 9); // 9: parm
    size_t index = 0;
    for (auto& v : coeffs) {
        NapiApi::Object vec(env);
        vec.Set("x", NapiApi::Value<float>(env, v.x));
        vec.Set("y", NapiApi::Value<float>(env, v.y));
        vec.Set("z", NapiApi::Value<float>(env, v.z));
        res.Set(index++, vec);
    }
    return res;
}
void EnvironmentJS::SetIrradianceCoefficients(NapiApi::FunctionContext<NapiApi::Array>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }

    NapiApi::Array coeffJS = ctx.Arg<0>();
    if (coeffJS.Count() != 9) { // 9: size
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
        environment->IrradianceCoefficients()->SetValue(coeffs);
    }
}

napi_value EnvironmentJS::GetIndirectDiffuseFactor(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto node = ctx.This().GetNative<SCENE_NS::IEnvironment>();
    if (!node) {
        return ctx.GetUndefined();
    }
    if (diffuseFactor_ == nullptr) {
        diffuseFactor_ = BASE_NS::make_unique<Vec4Proxy>(ctx.Env(), node->IndirectDiffuseFactor());
    }
    return diffuseFactor_->Value();
}

void EnvironmentJS::SetIndirectDiffuseFactor(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    auto node = ctx.This().GetNative<SCENE_NS::IEnvironment>();
    if (!node) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (diffuseFactor_ == nullptr) {
        diffuseFactor_ = BASE_NS::make_unique<Vec4Proxy>(ctx.Env(), node->IndirectDiffuseFactor());
    }
    diffuseFactor_->SetValue(obj);
}

napi_value EnvironmentJS::GetIndirectSpecularFactor(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    auto node = ctx.This().GetNative<SCENE_NS::IEnvironment>();
    if (!node) {
        return ctx.GetUndefined();
    }
    if (specularFactor_ == nullptr) {
        specularFactor_ = BASE_NS::make_unique<Vec4Proxy>(ctx.Env(), node->IndirectSpecularFactor());
    }
    return specularFactor_->Value();
}

void EnvironmentJS::SetIndirectSpecularFactor(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    auto node = ctx.This().GetNative<SCENE_NS::IEnvironment>();
    if (!node) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (specularFactor_ == nullptr) {
        specularFactor_ = BASE_NS::make_unique<Vec4Proxy>(ctx.Env(), node->IndirectSpecularFactor());
    }
    specularFactor_->SetValue(obj);
}

napi_value EnvironmentJS::GetEnvironmentMapFactor(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    auto node = ctx.This().GetNative<SCENE_NS::IEnvironment>();
    if (!node) {
        return ctx.GetUndefined();
    }
    if (environmentFactor_ == nullptr) {
        environmentFactor_ = BASE_NS::make_unique<Vec4Proxy>(ctx.Env(), node->EnvMapFactor());
    }
    return environmentFactor_->Value();
}

void EnvironmentJS::SetEnvironmentMapFactor(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    auto node = ctx.This().GetNative<SCENE_NS::IEnvironment>();
    if (!node) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (environmentFactor_ == nullptr) {
        environmentFactor_ = BASE_NS::make_unique<Vec4Proxy>(ctx.Env(), node->EnvMapFactor());
    }
    environmentFactor_->SetValue(obj);
}
