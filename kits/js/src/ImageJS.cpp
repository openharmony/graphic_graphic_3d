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

#include <scene_plugin/interface/intf_scene.h>
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
    napi_get_instance_data(env, (void**)&mis);
    mis->StoreCtor("Image", func);
}

void ImageJS::DisposeNative()
{
    if (!disposed_) {
        disposed_ = true;
        LOG_F("ImageJS::DisposeNative");
        NapiApi::Object obj = scene_.GetObject();
        auto* tro = obj.Native<TrueRootObject>();
        SceneJS* sceneJS;
        if (tro) {
            sceneJS = ((SceneJS*)tro->GetInstanceImpl(SceneJS::ID));
            sceneJS->ReleaseDispose((uintptr_t)&scene_);
        }
        // reset the native object refs
        if (auto bitmap = interface_pointer_cast<IBitmap>(GetNativeObject())) {
            SetNativeObject(nullptr, false);
            SetNativeObject(nullptr, true);
            if (obj) {
                BASE_NS::string uri = FetchResourceOrUri(obj.GetEnv(), uriRef_.GetObject());
                ExecSyncTask([uri, sceneJS]() -> META_NS::IAny::Ptr {
                    sceneJS->StoreBitmap(uri, nullptr);
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
    if (id == ImageJS::ID) {
        return this;
    }
    return SceneResourceImpl::GetInstanceImpl(id);
}
void ImageJS::Finalize(napi_env env)
{
    DisposeNative();
    BaseObject<ImageJS>::Finalize(env);
}

ImageJS::ImageJS(napi_env e, napi_callback_info i)
    : BaseObject<ImageJS>(e, i), SceneResourceImpl(SceneResourceType::IMAGE)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    NapiApi::Object meJs(e, fromJs.This());
    NapiApi::Object scene = fromJs.Arg<0>(); // access to owning scene...
    scene_ = { scene };
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        CORE_LOG_F("INVALID SCENE!");
    }

    auto* tro = scene.Native<TrueRootObject>();
    auto* sceneJS = ((SceneJS*)tro->GetInstanceImpl(SceneJS::ID));
    NapiApi::Object args = fromJs.Arg<1>();

    SCENE_NS::IBitmap::Ptr bitmap;

    // check if we got the NativeObject as parameter.
    bitmap = GetNativeObjectParam<SCENE_NS::IBitmap>(args);

    if (auto prm = args.Get("uri")) {
        uriRef_ = { e, prm };
        BASE_NS::string uri = FetchResourceOrUri(e, prm);
        if (!uri.empty()) {
            SetUri(uriRef_);
        }
        if (!bitmap) {
            // if we did not receive it already...
            bitmap = sceneJS->FetchBitmap(uri);
        }
    }
    sceneJS->DisposeHook((uintptr_t)&scene_, meJs);
    auto obj = interface_pointer_cast<META_NS::IObject>(bitmap);
    SetNativeObject(obj, false);
    StoreJsObj(obj, meJs);

    BASE_NS::string name;
    if (auto prm = args.Get<BASE_NS::string>("name")) {
        name = prm;
    } else {
        if (auto named = interface_cast<META_NS::INamed>(obj)) {
            name = named->Name()->GetValue();
        }
    }
    meJs.Set("name", name);
}

ImageJS::~ImageJS()
{
    DisposeNative();
}

napi_value ImageJS::GetWidth(NapiApi::FunctionContext<>& ctx)
{
    uint32_t width = 0;
    if (auto env = interface_cast<IBitmap>(GetNativeObject())) {
        ExecSyncTask([env, &width]() {
            width = env->Size()->GetValue().x;
            return META_NS::IAny::Ptr {};
        });
    }
    napi_value value;
    napi_create_uint32(ctx, width, &value);
    return value;
}

napi_value ImageJS::GetHeight(NapiApi::FunctionContext<>& ctx)
{
    uint32_t height = 0;
    if (auto env = interface_cast<IBitmap>(GetNativeObject())) {
        ExecSyncTask([env, &height]() {
            height = env->Size()->GetValue().y;
            return META_NS::IAny::Ptr {};
        });
    }
    napi_value value;
    napi_create_uint32(ctx, height, &value);
    return value;
}
