/*
* Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <scene/interface/intf_material.h>

#include "MaterialJS.h"
#include "MaterialPropertyJS.h"

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
using namespace SCENE_NS;

void MaterialPropertyJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> node_props;
    node_props.push_back(
        GetSetProperty<Object, MaterialPropertyJS, &MaterialPropertyJS::GetImage, &MaterialPropertyJS::SetImage>(
            "image"));
    node_props.push_back(
        GetSetProperty<Object, MaterialPropertyJS, &MaterialPropertyJS::GetFactor, &MaterialPropertyJS::SetFactor>(
            "factor"));
    node_props.push_back(
        MakeTROMethod<FunctionContext<>, MaterialPropertyJS, &MaterialPropertyJS::Dispose>("destroy"));

    napi_value func;
    auto status = napi_define_class(env, "MaterialProperty", NAPI_AUTO_LENGTH, BaseObject::ctor<MaterialPropertyJS>(),
        nullptr, node_props.size(), node_props.data(), &func);

    MyInstanceState* mis;
    GetInstanceData(env, (void**)&mis);
    mis->StoreCtor("MaterialProperty", func);
}
MaterialPropertyJS::MaterialPropertyJS(napi_env e, napi_callback_info i) : BaseObject<MaterialPropertyJS>(e, i)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        return;
    }

    NapiApi::Object meJs(fromJs.This());
    NapiApi::Object args = fromJs.Arg<1>();
    if (auto obj = GetNativeObjectParam<META_NS::IObject>(args)) {
        SetNativeObject(obj, false);
        StoreJsObj(obj, meJs);
    }
}
MaterialPropertyJS::~MaterialPropertyJS()
{
    DisposeNative(nullptr);
    if (!GetNativeObject()) {
        return;
    }
}
void* MaterialPropertyJS::GetInstanceImpl(uint32_t id)
{
    if (id == MaterialPropertyJS::ID) {
        return this;
    }
    return nullptr;
}
napi_value MaterialPropertyJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    DisposeNative(nullptr);
    return {};
}
void MaterialPropertyJS::Finalize(napi_env env)
{
    DisposeNative(nullptr);
    BaseObject<MaterialPropertyJS>::Finalize(env);
}
void MaterialPropertyJS::DisposeNative(void*)
{
    if (!disposed_) {
        disposed_ = true;
        SetNativeObject(nullptr, false);
        SetNativeObject(nullptr, true);
    }
}
napi_value MaterialPropertyJS::GetImage(NapiApi::FunctionContext<>& ctx)
{
    auto texture = interface_pointer_cast<SCENE_NS::ITexture>(GetThisNativeObject(ctx));
    if (!texture) {
        return ctx.GetUndefined();
    }

    SCENE_NS::IBitmap::Ptr image = META_NS::GetValue(texture->Image());
    auto obj = interface_pointer_cast<META_NS::IObject>(image);

    if (auto cached = FetchJsObj(obj)) {
        return cached.ToNapiValue();
    }
    napi_value args[] = { ctx.This().ToNapiValue() };
    return CreateFromNativeInstance(ctx.GetEnv(), obj, false, BASE_NS::countof(args), args).ToNapiValue();
}
void MaterialPropertyJS::SetImage(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto texture = interface_pointer_cast<SCENE_NS::ITexture>(GetThisNativeObject(ctx));
    if (!texture) {
        return;
    }

    NapiApi::Object imageJS = ctx.Arg<0>();
    if (auto nat = imageJS.Native<TrueRootObject>()) {
        SCENE_NS::IBitmap::Ptr image = interface_pointer_cast<SCENE_NS::IBitmap>(nat->GetNativeObject());
        META_NS::SetValue(texture->Image(), image);
    }
}
napi_value MaterialPropertyJS::GetFactor(NapiApi::FunctionContext<>& ctx)
{
    auto texture = interface_pointer_cast<SCENE_NS::ITexture>(GetThisNativeObject(ctx));
    if (!texture) {
        return ctx.GetUndefined();
    }

    BASE_NS::Math::Vec4 factor = META_NS::GetValue(texture->Factor());

    NapiApi::Env env(ctx.Env());
    NapiApi::Object res(env);
    res.Set("x", NapiApi::Value<float> { env, factor.x });
    res.Set("y", NapiApi::Value<float> { env, factor.y });
    res.Set("z", NapiApi::Value<float> { env, factor.z });
    res.Set("w", NapiApi::Value<float> { env, factor.w });
    return res.ToNapiValue();
}
void MaterialPropertyJS::SetFactor(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto texture = interface_pointer_cast<SCENE_NS::ITexture>(GetThisNativeObject(ctx));
    if (!texture) {
        return;
    }

    if (NapiApi::Object factorJS = ctx.Arg<0>()) {
        auto x = factorJS.Get<float>("x");
        auto y = factorJS.Get<float>("y");
        auto z = factorJS.Get<float>("z");
        auto w = factorJS.Get<float>("w");
        META_NS::SetValue(texture->Factor(), { x, y, z, w });
    }
}
