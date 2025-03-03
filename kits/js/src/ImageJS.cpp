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
#include "ImageJS.h"

#include <scene/interface/intf_scene.h>

#include "SceneJS.h"

using namespace SCENE_NS;

void ImageJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;
    // clang-format off
    BASE_NS::vector<napi_property_descriptor> props;
    SceneResourceImpl::GetPropertyDescs(props);
    props.emplace_back(GetProperty<uint32_t, ImageJS, &ImageJS::GetWidth>("width"));
    props.emplace_back(GetProperty<uint32_t, ImageJS, &ImageJS::GetHeight>("height"));

    // clang-format on

    napi_value func;
    auto status = napi_define_class(
        env, "Image", NAPI_AUTO_LENGTH, BaseObject::ctor<ImageJS>(), nullptr, props.size(), props.data(), &func);

    NapiApi::MyInstanceState* mis;
    GetInstanceData(env, (void**)&mis);
    mis->StoreCtor("Image", func);
}

void ImageJS::DisposeNative(void* sc)
{
    if (!disposed_) {
        disposed_ = true;
        LOG_V("ImageJS::DisposeNative");
        NapiApi::Object obj = scene_.GetObject();
        SceneJS* sceneJS = nullptr;
        if (sc) {
            sceneJS = static_cast<SceneJS*>(sc);
        } else {
            auto* tro = obj.Native<TrueRootObject>();
            if (tro) {
                sceneJS = static_cast<SceneJS*>(tro->GetInstanceImpl(SceneJS::ID));
            }
        }
        if (sc) {
            sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
        }
        // reset the native object refs
        if (auto bitmap = interface_pointer_cast<IBitmap>(GetNativeObject())) {
            SetNativeObject(nullptr, false);
            SetNativeObject(nullptr, true);
            if (obj) {
                BASE_NS::string uri = FetchResourceOrUri(uri_.GetEnv(), uri_.GetObject().ToNapiValue());
                ExecSyncTask([uri, sceneJS]() -> META_NS::IAny::Ptr {
                    if (sceneJS) {
                        sceneJS->StoreBitmap(uri, nullptr);
                    }
                    return {};
                });
            }
        } else {
            SetNativeObject(nullptr, false);
        }
        scene_.Reset();
    }
}
void* ImageJS::GetInstanceImpl(uint32_t id)
{
    if (id == ImageJS::ID)
        return this;
    return SceneResourceImpl::GetInstanceImpl(id);
}
void ImageJS::Finalize(napi_env env)
{
    DisposeNative(nullptr);
    BaseObject<ImageJS>::Finalize(env);
}

ImageJS::ImageJS(napi_env e, napi_callback_info i)
    : BaseObject<ImageJS>(e, i), SceneResourceImpl(SceneResourceType::IMAGE)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    NapiApi::Object meJs(fromJs.This());
    NapiApi::Object scene = fromJs.Arg<0>(); // access to owning scene...
    scene_ = { scene };
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        LOG_F("INVALID SCENE!");
    }

    SceneJS* sceneJS = nullptr;
    auto* tro = scene.Native<TrueRootObject>();
    if (tro) {
        sceneJS = static_cast<SceneJS*>(tro->GetInstanceImpl(SceneJS::ID));
    }
    NapiApi::Object args = fromJs.Arg<1>();

    SCENE_NS::IBitmap::Ptr bitmap;

    // check if we got the NativeObject as parameter.
    bitmap = GetNativeObjectParam<SCENE_NS::IBitmap>(args);

    if (auto prm = args.Get("uri")) {
        NapiApi::Object resType = args.Get<NapiApi::Object>("uri");
        BASE_NS::string uriType = args.Get<BASE_NS::string>("uri");

        BASE_NS::string uri = FetchResourceOrUri(args.GetEnv(), prm);
        if (!uri.empty()) {
            if (!resType) {
                // raw string then.. make it  "resource" / "rawfile" if possible.
                if (uri.find("OhosRawFile://") == 0) {
                    // we can only convert "OhosRawFile://" type uris back to "resource" objects.
                    NapiApi::Env env(e);
                    napi_value global;
                    napi_get_global(env, &global);
                    NapiApi::Object g(env, global);
                    napi_value func = g.Get("$rawfile");
                    NapiApi::Function f(env, func);
                    if (f) {
                        BASE_NS::string noschema(uri.substr(14)); // 14: length
                        napi_value arg = env.GetString(noschema);
                        napi_value res = f.Invoke(g, 1, &arg);
                        SetUri(NapiApi::StrongRef(env, res));
                    }
                }
            } else {
                SetUri(NapiApi::StrongRef(resType));
            }
        }
        if (!bitmap) {
            // if we did not receive it already...
            if (sceneJS) {
                bitmap = sceneJS->FetchBitmap(uri);
            }
        }
    }
    if (sceneJS) {
        sceneJS->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
    }
    auto obj = interface_pointer_cast<META_NS::IObject>(bitmap);
    SetNativeObject(obj, false);
    StoreJsObj(obj, meJs);

    BASE_NS::string name;
    if (auto prm = args.Get<BASE_NS::string>("name"); prm.IsDefined()) {
        name = prm;
    } else {
        if (auto named = interface_cast<META_NS::IObject>(obj)) {
            name = named->GetName();
        }
    }
    meJs.Set("name", name);
}

ImageJS::~ImageJS()
{
    DisposeNative(nullptr);
}

napi_value ImageJS::GetWidth(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    uint32_t width = 0;
    if (auto env = interface_cast<IBitmap>(GetNativeObject())) {
        width = env->Size()->GetValue().x;
    }
    return ctx.GetNumber(width);
}

napi_value ImageJS::GetHeight(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    uint32_t height = 0;
    if (auto env = interface_cast<IBitmap>(GetNativeObject())) {
        height = env->Size()->GetValue().y;
    }
    return ctx.GetNumber(height);
}
