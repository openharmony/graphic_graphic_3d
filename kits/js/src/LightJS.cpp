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
#include <scene_plugin/api/light_uid.h>
#include <scene_plugin/interface/intf_light.h>
#include <scene_plugin/interface/intf_scene.h>
using namespace NapiApi;
BaseLight::BaseLight(LightType lt) : NodeImpl(NodeImpl::NodeType::LIGHT), lightType_(lt) {}
void BaseLight::RegisterEnums(NapiApi::Object exports)
{
    napi_value v;
    NapiApi::Object LightType(exports.GetEnv());

#define DECL_ENUM(enu, x)                                     \
    {                                                         \
        napi_create_uint32(enu.GetEnv(), BaseLight::LightType::x, (&v)); \
        enu.Set((#x), (v));                                 \
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
    NapiApi::Object scene = fromJs.Arg<0>();
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        CORE_LOG_F("INVALID SCENE!");
    }
    scene_ = { scene };
    auto scn = GetNativeMeta<SCENE_NS::IScene>(scene);

    if (scn == nullptr) {
        // hmm..
        CORE_LOG_F("Invalid scene for LightJS!");
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

    if (path) {
        // create using path
        nodePath = path.valueOrDefault("");
    } else if (name) {
        // use the name as path (creates under root)
        nodePath = name.valueOrDefault("");
    } else {
        // no name or path defined should this just fail?
    }

    // Create actual camera object.
    SCENE_NS::ILight::Ptr node;
    ExecSyncTask([scn, nodePath, &node]() {
        node = scn->CreateNode<SCENE_NS::ILight>(nodePath, true);
        return META_NS::IAny::Ptr {};
    });

    TrueRootObject* instance = GetThisRootObject(fromJs);

    instance->SetNativeObject(interface_pointer_cast<META_NS::IObject>(node), false);
    node.reset();

    NapiApi::Object meJs(e, fromJs.This());
    StoreJsObj(instance->GetNativeObject(), meJs);

    if (name) {
        // set the name of the object. if we were given one
        meJs.Set("name", name);
    }
}
BaseLight::~BaseLight() {}
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
    napi_get_instance_data(env, (void**)&mis);
    mis->StoreCtor(class_name, func);
}
void BaseLight::Finalize(napi_env env, TrueRootObject* tro)
{
    tro->Finalize(env);
}

void* BaseLight::GetInstanceImpl(uint32_t id)
{
    if (id == BaseLight::ID) {
        return this;
    }
    return NodeImpl::GetInstanceImpl(id);
}
void BaseLight::DisposeNative(TrueRootObject* tro)
{
    // do nothing for now..
    LOG_F("BaseLight::DisposeNative");
    colorProxy_.reset();
    if (auto light = interface_pointer_cast<SCENE_NS::ILight>(tro->GetNativeObject())) {
        // reset the native object refs
        tro->SetNativeObject(nullptr, false);
        tro->SetNativeObject(nullptr, true);

        ExecSyncTask([light = BASE_NS::move(light)]() mutable {
            auto node = interface_pointer_cast<SCENE_NS::INode>(light);
            if (node == nullptr) {
                return META_NS::IAny::Ptr {};
            }
            auto scene = node->GetScene();
            if (scene == nullptr) {
                return META_NS::IAny::Ptr {};
            }
            scene->ReleaseNode(node);
            return META_NS::IAny::Ptr {};
        });
    }
    scene_.Reset();
}
napi_value BaseLight::GetlightType(NapiApi::FunctionContext<>& ctx)
{
    uint32_t type = -1; // return -1 if the object does not exist anymore
    if (auto node = interface_cast<SCENE_NS::ILight>(GetThisNativeObject(ctx))) {
        type = lightType_;
    }
    napi_value value;
    napi_status status = napi_create_uint32(ctx, type, &value);
    return value;
}

napi_value BaseLight::GetEnabled(NapiApi::FunctionContext<>& ctx)
{
    bool enable = true;
    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    if (node) {
        ExecSyncTask([node, &enable]() {
            enable = node->Visible()->GetValue();
            return META_NS::IAny::Ptr {};
        });
    }
    napi_value value;
    napi_status status = napi_get_boolean(ctx, enable, &value);
    return value;
}

void BaseLight::SetEnabled(NapiApi::FunctionContext<bool>& ctx)
{
    bool enabled = ctx.Arg<0>();
    auto node = interface_pointer_cast<SCENE_NS::INode>(GetThisNativeObject(ctx));
    if (node) {
        ExecSyncTask([node, enabled]() {
            node->Visible()->SetValue(enabled);
            return META_NS::IAny::Ptr {};
        });
    }
}

napi_value BaseLight::GetColor(NapiApi::FunctionContext<>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::ILight>(GetThisNativeObject(ctx));
    if (!node) {
        return {};
    }
    if (colorProxy_ == nullptr) {
        colorProxy_ = BASE_NS::make_unique<ColorProxy>(ctx, node->Color());
    }
    return colorProxy_->Value();
}
void BaseLight::SetColor(NapiApi::FunctionContext<Object>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::ILight>(GetThisNativeObject(ctx));
    if (!node) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    if (colorProxy_ == nullptr) {
        colorProxy_ = BASE_NS::make_unique<ColorProxy>(ctx, node->Color());
    }
    colorProxy_->SetValue(obj);
}

napi_value BaseLight::GetShadowEnabled(NapiApi::FunctionContext<>& ctx)
{
    bool enable = true;
    auto node = interface_pointer_cast<SCENE_NS::ILight>(GetThisNativeObject(ctx));
    if (node) {
        ExecSyncTask([node, &enable]() {
            enable = node->ShadowEnabled()->GetValue();
            return META_NS::IAny::Ptr {};
        });
    }
    napi_value value;
    napi_status status = napi_get_boolean(ctx, enable, &value);
    return value;
}
void BaseLight::SetShadowEnabled(NapiApi::FunctionContext<bool>& ctx)
{
    bool enabled = ctx.Arg<0>();
    auto node = interface_pointer_cast<SCENE_NS::ILight>(GetThisNativeObject(ctx));
    if (node) {
        ExecSyncTask([node, enabled]() {
            node->ShadowEnabled()->SetValue(enabled);
            return META_NS::IAny::Ptr {};
        });
    }
}

napi_value BaseLight::GetIntensity(NapiApi::FunctionContext<>& ctx)
{
    float intensity = 0.0f;
    auto node = interface_pointer_cast<SCENE_NS::ILight>(GetThisNativeObject(ctx));
    if (node) {
        ExecSyncTask([node, &intensity]() {
            intensity = node->Intensity()->GetValue();
            return META_NS::IAny::Ptr {};
        });
    }
    napi_value value;
    napi_status status = napi_create_double(ctx, intensity, &value);
    return value;
}
void BaseLight::SetIntensity(NapiApi::FunctionContext<float>& ctx)
{
    float intensity = ctx.Arg<0>();
    auto node = interface_pointer_cast<SCENE_NS::ILight>(GetThisNativeObject(ctx));
    if (node) {
        ExecSyncTask([node, intensity]() {
            node->Intensity()->SetValue(intensity);
            return META_NS::IAny::Ptr {};
        });
    }
}

SpotLightJS::SpotLightJS(napi_env e, napi_callback_info i)
    : BaseObject<SpotLightJS>(e, i), BaseLight(BaseLight::LightType::SPOT)
{
    Create(e, i);
    if (auto light = interface_pointer_cast<SCENE_NS::ILight>(GetNativeObject())) {
        light->Type()->SetValue(SCENE_NS::ILight::SCENE_LIGHT_SPOT);
    }
}
void SpotLightJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> node_props;

    BaseLight::Init("SpotLight", env, exports, node_props, BaseObject::ctor<SpotLightJS>());
}

void* SpotLightJS::GetInstanceImpl(uint32_t id)
{
    if (id == SpotLightJS::ID) {
        return this;
    }
    return BaseLight::GetInstanceImpl(id);
}
void SpotLightJS::DisposeNative()
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
        light->Type()->SetValue(SCENE_NS::ILight::SCENE_LIGHT_POINT);
    }
}
void* PointLightJS::GetInstanceImpl(uint32_t id)
{
    if (id == PointLightJS::ID) {
        return this;
    }
    return BaseLight::GetInstanceImpl(id);
}
void PointLightJS::DisposeNative()
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
        light->Type()->SetValue(SCENE_NS::ILight::SCENE_LIGHT_DIRECTIONAL);
    }
}
void* DirectionalLightJS::GetInstanceImpl(uint32_t id)
{
    if (id == DirectionalLightJS::ID) {
        return this;
    }
    return BaseLight::GetInstanceImpl(id);
}
void DirectionalLightJS::DisposeNative()
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
    float fov = 0.0;
    if (auto camera = interface_cast<SCENE_NS::ILight>(GetNativeObject())) {
        ExecSyncTask([camera, &fov]() {
            fov = 0.0;
            if (camera) {
                fov = camera->NearPlane()->GetValue();
            }
            return META_NS::IAny::Ptr {};
        });
    }

    napi_value value;
    napi_status status = napi_create_double(ctx, fov, &value);
    return value;
}

void DirectionalLightJS::SetNear(NapiApi::FunctionContext<float>& ctx)
{
    float fov = ctx.Arg<0>();
    if (auto camera = interface_cast<SCENE_NS::ILight>(GetNativeObject())) {
        ExecSyncTask([camera, fov]() {
            camera->NearPlane()->SetValue(fov);
            return META_NS::IAny::Ptr {};
        });
    }
}