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
#include "LightJS.h"

#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <scene/interface/intf_light.h>
#include <scene/interface/intf_scene.h>

#include "SceneJS.h"

using namespace NapiApi;
BaseLight::BaseLight(LightType lt) : NodeImpl(NodeImpl::NodeType::LIGHT), lightType_(lt) {}
void BaseLight::RegisterEnums(NapiApi::Object exports)
{
    napi_value v;
    NapiApi::Object LightType(exports.GetEnv());

#define DECL_ENUM(enu, x)                                              \
    {                                                                  \
        napi_create_uint32(enu.GetEnv(), BaseLight::LightType::x, &v); \
        enu.Set(#x, v);                                                \
    }
    DECL_ENUM(LightType, DIRECTIONAL);
    DECL_ENUM(LightType, POINT);
    DECL_ENUM(LightType, SPOT);
#undef DECL_ENUM
    exports.Set("LightType", LightType);
}

void BaseLight::Create(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        // no arguments. so internal create.
        // expecting caller to finish initialization
        return;
    }

    // java script call.. with arguments
    scene_ = fromJs.Arg<0>().valueOrDefault();
    auto scn = GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject());
    if (scn == nullptr) {
        // hmm..
        LOG_F("Invalid scene for LightJS!");
        return;
    }
    // collect parameters
    NapiApi::Value<BASE_NS::string> name;
    NapiApi::Value<BASE_NS::string> path;
    NapiApi::Object args = fromJs.Arg<1>();
    if (auto prm = args.Get("name")) {
        name = NapiApi::Value<BASE_NS::string>(e, prm);
    }
    if (auto prm = args.Get("path")) {
        path = NapiApi::Value<BASE_NS::string>(e, prm);
    }

    BASE_NS::string nodePath;

    if (path.IsDefined()) {
        // create using path
        nodePath = path.valueOrDefault("");
    } else if (name.IsDefined()) {
        // use the name as path (creates under root)
        nodePath = name.valueOrDefault("");
    }

    // Create actual light object.
    SCENE_NS::ILight::Ptr node = scn->CreateNode<SCENE_NS::ILight>(nodePath, SCENE_NS::ClassId::LightNode).GetResult();

    TrueRootObject* instance = GetThisRootObject(fromJs);

    instance->SetNativeObject(interface_pointer_cast<META_NS::IObject>(node), false);
    node.reset();

    NapiApi::Object meJs(fromJs.This());
    StoreJsObj(instance->GetNativeObject(), meJs);

    if (name.IsDefined()) {
        // set the name of the object. if we were given one
        meJs.Set("name", name);
    }

    {
        // add the dispose hook to scene. (so that the geometry node is disposed when scene is disposed)
        NapiApi::Object scene = fromJs.Arg<0>();
        auto* tro = scene.Native<TrueRootObject>();
        if (tro) {
            auto* sceneJS = static_cast<SceneJS*>(tro->GetInstanceImpl(SceneJS::ID));
            if (sceneJS) {
                sceneJS->StrongDisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
            }
        }
    }
}
BaseLight::~BaseLight()
{
    colorProxy_.reset();
}
void BaseLight::Init(const char* class_name, napi_env env, napi_value exports,
    BASE_NS::vector<napi_property_descriptor>& np, napi_callback ctor)
{
    NodeImpl::GetPropertyDescs(np);

    np.push_back(TROGetProperty<float, BaseLight, &BaseLight::GetlightType>("lightType"));
    np.push_back(TROGetSetProperty<Object, BaseLight, &BaseLight::GetColor, &BaseLight::SetColor>("color"));
    np.push_back(TROGetSetProperty<float, BaseLight, &BaseLight::GetIntensity, &BaseLight::SetIntensity>("intensity"));
    np.push_back(TROGetSetProperty<bool, BaseLight, &BaseLight::GetShadowEnabled, &BaseLight::SetShadowEnabled>(
        "shadowEnabled"));
    np.push_back(TROGetSetProperty<bool, BaseLight, &BaseLight::GetEnabled, &BaseLight::SetEnabled>("enabled"));

    napi_value func;
    auto status = napi_define_class(env, class_name, NAPI_AUTO_LENGTH, ctor, nullptr, np.size(), np.data(), &func);

    NapiApi::MyInstanceState* mis;
    GetInstanceData(env, (void**)&mis);
    mis->StoreCtor(class_name, func);
}
void BaseLight::Finalize(napi_env env, TrueRootObject* tro)
{
    tro->Finalize(env);
}
void* BaseLight::GetInstanceImpl(uint32_t id)
{
    if (id == BaseLight::ID)
        return this;
    return NodeImpl::GetInstanceImpl(id);
}

void BaseLight::DisposeNative(TrueRootObject* tro)
{
    LOG_V("BaseLight::DisposeNative");

    NapiApi::Object obj = scene_.GetObject();
    auto* stro = obj.Native<TrueRootObject>();
    if (stro) {
        SceneJS* sceneJS = static_cast<SceneJS*>(stro->GetInstanceImpl(SceneJS::ID));
        if (sceneJS) {
            sceneJS->ReleaseStrongDispose(reinterpret_cast<uintptr_t>(&scene_));
        }
    }

    colorProxy_.reset();
    if (auto light = interface_pointer_cast<SCENE_NS::ILight>(tro->GetNativeObject())) {
        // reset the native object refs
        tro->SetNativeObject(nullptr, false);
        tro->SetNativeObject(nullptr, true);
        if (auto node = interface_pointer_cast<SCENE_NS::INode>(light)) {
            if (auto scene = node->GetScene()) {
                scene->ReleaseNode(node);
            }
        }
    }
    scene_.Reset();
}
napi_value BaseLight::GetlightType(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    uint32_t type = -1; // return -1 if the object does not exist anymore
    if (auto node = interface_cast<SCENE_NS::ILight>(GetThisNativeObject(ctx))) {
        type = lightType_;
    }
    return ctx.GetNumber(type);
}

napi_value BaseLight::GetEnabled(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    bool enable = false;
    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    if (node) {
        enable = node->Enabled()->GetValue();
    }
    return ctx.GetBoolean(enable);
}
void BaseLight::SetEnabled(NapiApi::FunctionContext<bool>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    bool enabled = ctx.Arg<0>();
    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    if (node) {
        node->Enabled()->SetValue(enabled);
    }
}

napi_value BaseLight::GetColor(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    auto node = interface_pointer_cast<SCENE_NS::ILight>(GetThisNativeObject(ctx));
    if (!node) {
        return ctx.GetUndefined();
    }
    if (colorProxy_ == nullptr) {
        colorProxy_ = BASE_NS::make_unique<ColorProxy>(ctx.GetEnv(), node->Color());
    }
    return colorProxy_->Value();
}
void BaseLight::SetColor(NapiApi::FunctionContext<Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    auto node = interface_pointer_cast<SCENE_NS::ILight>(GetThisNativeObject(ctx));
    if (!node) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (colorProxy_ == nullptr) {
        colorProxy_ = BASE_NS::make_unique<ColorProxy>(ctx.GetEnv(), node->Color());
    }
    colorProxy_->SetValue(obj);
}

napi_value BaseLight::GetShadowEnabled(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    bool enable = false;
    auto node = interface_pointer_cast<SCENE_NS::ILight>(GetThisNativeObject(ctx));
    if (node) {
        enable = node->ShadowEnabled()->GetValue();
    }
    return ctx.GetBoolean(enable);
}
void BaseLight::SetShadowEnabled(NapiApi::FunctionContext<bool>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    bool enabled = ctx.Arg<0>();
    auto node = interface_pointer_cast<SCENE_NS::ILight>(GetThisNativeObject(ctx));
    if (node) {
        node->ShadowEnabled()->SetValue(enabled);
    }
}

napi_value BaseLight::GetIntensity(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    float intensity = 0.0f;
    auto node = interface_pointer_cast<SCENE_NS::ILight>(GetThisNativeObject(ctx));
    if (node) {
        intensity = node->Intensity()->GetValue();
    }
    return ctx.GetNumber(intensity);
}
void BaseLight::SetIntensity(NapiApi::FunctionContext<float>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    float intensity = ctx.Arg<0>();
    auto node = interface_pointer_cast<SCENE_NS::ILight>(GetThisNativeObject(ctx));
    if (node) {
        node->Intensity()->SetValue(intensity);
    }
}

SpotLightJS::SpotLightJS(napi_env e, napi_callback_info i)
    : BaseObject<SpotLightJS>(e, i), BaseLight(BaseLight::LightType::SPOT)
{
    Create(e, i);
    if (auto light = interface_pointer_cast<SCENE_NS::ILight>(GetNativeObject())) {
        light->Type()->SetValue(SCENE_NS::LightType::SPOT);
    }
}
void SpotLightJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> node_props;

    BaseLight::Init("SpotLight", env, exports, node_props, BaseObject::ctor<SpotLightJS>());
}

void* SpotLightJS::GetInstanceImpl(uint32_t id)
{
    if (id == SpotLightJS::ID)
        return this;
    return BaseLight::GetInstanceImpl(id);
}
void SpotLightJS::DisposeNative(void*)
{
    BaseLight::DisposeNative(this);
}
void SpotLightJS::Finalize(napi_env env)
{
    BaseObject<SpotLightJS>::Finalize(env);
}

PointLightJS::PointLightJS(napi_env e, napi_callback_info i)
    : BaseObject<PointLightJS>(e, i), BaseLight(BaseLight::LightType::POINT)
{
    Create(e, i);
    if (auto light = interface_pointer_cast<SCENE_NS::ILight>(GetNativeObject())) {
        light->Type()->SetValue(SCENE_NS::LightType::SPOT);
    }
}
void* PointLightJS::GetInstanceImpl(uint32_t id)
{
    if (id == PointLightJS::ID)
        return this;
    return BaseLight::GetInstanceImpl(id);
}
void PointLightJS::DisposeNative(void*)
{
    BaseLight::DisposeNative(this);
}
void PointLightJS::Finalize(napi_env env)
{
    BaseObject<PointLightJS>::Finalize(env);
}
void PointLightJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> node_props;
    BaseLight::Init("PointLight", env, exports, node_props, BaseObject::ctor<PointLightJS>());
}

DirectionalLightJS::DirectionalLightJS(napi_env e, napi_callback_info i)
    : BaseObject<DirectionalLightJS>(e, i), BaseLight(BaseLight::LightType::DIRECTIONAL)
{
    Create(e, i);
    if (auto light = interface_pointer_cast<SCENE_NS::ILight>(GetNativeObject())) {
        light->Type()->SetValue(SCENE_NS::LightType::DIRECTIONAL);
    }
}
void* DirectionalLightJS::GetInstanceImpl(uint32_t id)
{
    if (id == DirectionalLightJS::ID)
        return this;
    return BaseLight::GetInstanceImpl(id);
}
void DirectionalLightJS::DisposeNative(void*)
{
    BaseLight::DisposeNative(this);
}
void DirectionalLightJS::Finalize(napi_env env)
{
    BaseObject<DirectionalLightJS>::Finalize(env);
}

void DirectionalLightJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> node_props;
    node_props.push_back(
        GetSetProperty<float, DirectionalLightJS, &DirectionalLightJS::GetNear, &DirectionalLightJS::SetNear>(
            "nearPlane"));
    BaseLight::Init("DirectionalLight", env, exports, node_props, BaseObject::ctor<DirectionalLightJS>());
}

napi_value DirectionalLightJS::GetNear(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    float near = 0.0;
    if (auto light = interface_cast<SCENE_NS::ILight>(GetNativeObject())) {
        near = light->NearPlane()->GetValue();
    }
    return ctx.GetNumber(near);
}

void DirectionalLightJS::SetNear(NapiApi::FunctionContext<float>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    float near = ctx.Arg<0>();
    if (auto light = interface_cast<SCENE_NS::ILight>(GetNativeObject())) {
        light->NearPlane()->SetValue(near);
    }
}
