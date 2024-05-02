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
#include "ShaderJS.h"

#include <scene_plugin/api/material_uid.h>
#include <scene_plugin/interface/intf_ecs_object.h>
#include <scene_plugin/interface/intf_mesh.h>
#include <scene_plugin/interface/intf_scene.h>

#include "MaterialJS.h"
#include "SceneJS.h"
#include "Vec2Proxy.h"
#include "Vec3Proxy.h"
#include "Vec4Proxy.h"

using IntfPtr = META_NS::SharedPtrIInterface;
using IntfWeakPtr = META_NS::WeakPtrIInterface;

template<typename type>
class TypeProxy : public Proxy {
    META_NS::IProperty::Ptr prop_;
    BASE_NS::string name_;

public:
    explicit TypeProxy(META_NS::IProperty::Ptr prop) : prop_(prop), name_(prop_->GetName()) {}
    ~TypeProxy()
    {
        prop_.reset();
    }

    napi_value Get(NapiApi::FunctionContext<> ctx)
    {
        META_NS::Property<type> p(prop_);
        return NapiApi::Value<type>(ctx, p->GetValue());
    }
    napi_value Set(NapiApi::FunctionContext<type> ctx)
    {
        type val = ctx.template Arg<0>();
        META_NS::Property<type> p(prop_);
        p->SetValue(val);
        return ctx.GetUndefined();
    }
    void insertProp(BASE_NS::vector<napi_property_descriptor>& props)
    {
        props.push_back(napi_property_descriptor { name_.c_str(), nullptr, nullptr,
            [](napi_env e, napi_callback_info i) -> napi_value {
                NapiApi::FunctionContext<> info(e, i);
                auto me_ = (TypeProxy<type>*)info.GetData();
                return me_->Get(info);
            },
            [](napi_env e, napi_callback_info i) -> napi_value {
                NapiApi::FunctionContext<type> info(e, i);
                auto me_ = (TypeProxy<type>*)info.GetData();
                return me_->Set(info);
            },
            nullptr, napi_default_jsproperty, (void*)this });
    }
};

class BitmapProxy : public Proxy {
    NapiApi::StrongRef scene_;
    META_NS::IProperty::Ptr prop_;
    BASE_NS::string name_;

public:
    BitmapProxy(NapiApi::Object scene, META_NS::IProperty::Ptr prop, BASE_NS::string_view prefix = "") : prop_(prop)
    {
        scene_ = scene;
        if (!prefix.empty()) {
            name_ = prefix;
            name_ += "_";
        }
        name_ += prop->GetName();
    }
    ~BitmapProxy()
    {
        prop_.reset();
    }

    napi_value Get(NapiApi::FunctionContext<> ctx)
    {
        META_NS::Property<SCENE_NS::IBitmap::Ptr> p(prop_);
        // return NapiApi::Value<type>(ctx, p->GetValue());
        auto obj = p->GetValue();
        if (auto cached = FetchJsObj(obj)) {
            return cached;
        }

        if (obj) {
            // okay.. there is a native bitmap set.. but no js wrapper yet.

            // create the jsobject if we don't have one.
            NapiApi::Object parms(ctx);
            napi_value args[] = {
                scene_.GetValue(), // scene..
                parms              // params.
            };
            MakeNativeObjectParam(ctx, obj, BASE_NS::countof(args), args);

            auto size = obj->Size()->GetValue();
            auto uri = obj->Uri()->GetValue();
            auto name = interface_cast<META_NS::INamed>(obj)->Name()->GetValue();
            parms.Set("uri", uri);
            NapiApi::Object imageJS(GetJSConstructor(ctx, "Image"), BASE_NS::countof(args), args);
            return imageJS;
        }
        return ctx.GetNull();
    }
    napi_value Set(NapiApi::FunctionContext<NapiApi::Object> ctx)
    {
        NapiApi::Object val = ctx.Arg<0>();
        auto bitmap = GetNativeMeta<SCENE_NS::IBitmap>(val);
        if (bitmap) {
            META_NS::Property<SCENE_NS::IBitmap::Ptr> p(prop_);
            p->SetValue(bitmap);
        }
        return ctx.GetUndefined();
    }
    void insertProp(BASE_NS::vector<napi_property_descriptor>& props)
    {
        props.push_back(napi_property_descriptor { name_.c_str(), nullptr, nullptr,
            [](napi_env e, napi_callback_info i) -> napi_value {
                NapiApi::FunctionContext<> info(e, i);
                auto me_ = (BitmapProxy*)info.GetData();
                return me_->Get(info);
            },
            [](napi_env e, napi_callback_info i) -> napi_value {
                NapiApi::FunctionContext<NapiApi::Object> info(e, i);
                auto me_ = (BitmapProxy*)info.GetData();
                return me_->Set(info);
            },
            nullptr, napi_default_jsproperty, (void*)this });
    }
};

template<typename proxType>
class PropProxy : public Proxy {
    BASE_NS::shared_ptr<proxType> proxy_;
    BASE_NS::string name_;

public:
    PropProxy(napi_env e, META_NS::IProperty::Ptr prop, BASE_NS::string_view prefix = "")
    {
        proxy_.reset(new proxType(e, prop));
        if (!prefix.empty()) {
            name_ = prefix;
            name_ += "_";
        }
        name_ += prop->GetName();
    }
    ~PropProxy()
    {
        proxy_.reset();
    }
    napi_value Get(NapiApi::FunctionContext<> ctx)
    {
        return proxy_->Value();
    }

    napi_value Set(NapiApi::FunctionContext<NapiApi::Object> ctx)
    {
        NapiApi::Object val = ctx.Arg<0>();
        proxy_->SetValue(val);
        return ctx.GetUndefined();
    }

    void insertProp(BASE_NS::vector<napi_property_descriptor>& props)
    {
        props.push_back(napi_property_descriptor { name_.c_str(), nullptr, nullptr,
            [](napi_env e, napi_callback_info i) -> napi_value {
                NapiApi::FunctionContext<> info(e, i);
                auto me_ = (PropProxy<proxType>*)info.GetData();
                return me_->Get(info);
            },
            [](napi_env e, napi_callback_info i) -> napi_value {
                NapiApi::FunctionContext<NapiApi::Object> info(e, i);
                auto me_ = (PropProxy<proxType>*)info.GetData();
                return me_->Set(info);
            },
            nullptr, napi_default_jsproperty, (void*)this });
    }
};

void ShaderJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;
    BASE_NS::vector<napi_property_descriptor> node_props;

    SceneResourceImpl::GetPropertyDescs(node_props);

    napi_value func;
    auto status = napi_define_class(env, "Shader", NAPI_AUTO_LENGTH, BaseObject::ctor<ShaderJS>(), nullptr,
        node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    napi_get_instance_data(env, (void**)&mis);
    mis->StoreCtor("Shader", func);
}

ShaderJS::ShaderJS(napi_env e, napi_callback_info i)
    : BaseObject<ShaderJS>(e, i), SceneResourceImpl(SceneResourceImpl::SHADER)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    NapiApi::Object meJs(e, fromJs.This());

    NapiApi::Object scene = fromJs.Arg<0>(); // access to owning scene...
    NapiApi::Object args = fromJs.Arg<1>();  // other args
    scene_ = { scene };
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        CORE_LOG_F("INVALID SCENE!");
    }

    auto* tro = scene.Native<TrueRootObject>();
    auto* sceneJS = ((SceneJS*)tro->GetInstanceImpl(SceneJS::ID));
    sceneJS->DisposeHook((uintptr_t)&scene_, meJs);

    // check if we got the NativeObject as parameter. (meta object created when bound to material..)
    auto metaobj = GetNativeObjectParam<META_NS::IMetadata>(args);

    StoreJsObj(interface_pointer_cast<META_NS::IObject>(metaobj), meJs);

    // check if it's a SCENE_NS::IShader (can only be set on instances created createShader, these are the "template"
    // shaders.)
    auto shader = interface_pointer_cast<SCENE_NS::IShader>(metaobj);
    if (shader) {
        // we should not be bound to a material then.. this is a place holder object.
        SetNativeObject(interface_pointer_cast<META_NS::IObject>(shader), true);
    } else {
        // should be bound to a material..
        // so the shader should be stored as a parameter..
        SetNativeObject(interface_pointer_cast<META_NS::IObject>(metaobj), true);
        shader = interface_pointer_cast<SCENE_NS::IShader>(metaobj->GetPropertyByName<IntfPtr>("shader")->GetValue());
    }

    NapiApi::Object material = args.Get<NapiApi::Object>("Material"); // see if we SHOULD be bound to a material.
    if (material) {
        BindToMaterial(meJs, material);
    }

    BASE_NS::string name;
    if (auto prm = args.Get<BASE_NS::string>("name")) {
        name = prm;
    } else {
        if (auto named = interface_cast<META_NS::INamed>(metaobj)) {
            name = named->Name()->GetValue();
        }
    }
    meJs.Set("name", name);
}

void ShaderJS::BindToMaterial(NapiApi::Object meJs, NapiApi::Object material)
{
    auto metaobj = GetNativeMeta<META_NS::IMetadata>(meJs);
    auto shader = interface_pointer_cast<SCENE_NS::IShader>(
        metaobj->GetPropertyByName<IntfPtr>("shader")->GetValue());

    napi_env e = meJs.GetEnv();
    // inputs are actually owned (and used) by the material.
    // create the input object
    NapiApi::Object inputs(e);

    auto* tro = material.Native<TrueRootObject>();
    auto mat = interface_pointer_cast<SCENE_NS::IMaterial>(tro->GetNativeObject());

    BASE_NS::vector<napi_property_descriptor> inputProps;

    META_NS::IMetadata::Ptr customProperties;
    BASE_NS::vector<SCENE_NS::ITextureInfo::Ptr> Textures;

    ExecSyncTask([mat, &customProperties, &Textures]() {
        Textures = mat->Inputs()->GetValue();
        customProperties = interface_pointer_cast<META_NS::IMetadata>(mat->CustomProperties()->GetValue());
        return META_NS::IAny::Ptr {};
    });
    if (!Textures.empty()) {
        int index = 0;
        for (auto t : Textures) {
            BASE_NS::string name;
            auto nn = interface_cast<META_NS::INamed>(t);
            if (nn) {
                name = nn->Name()->GetValue();
            } else {
                name = "TextureInfo_" + BASE_NS::to_string(index);
            }
            BASE_NS::shared_ptr<Proxy> proxt;
            // factor
            proxt = BASE_NS::shared_ptr { new PropProxy<Vec4Proxy>(e, t->Factor(), name) };
            if (proxt) {
                proxies_.push_back(proxt);
                proxt->insertProp(inputProps);
            }
            proxt = BASE_NS::shared_ptr { new BitmapProxy(scene_.GetObject(), t->Image(), name) };
            if (proxt) {
                proxies_.push_back(proxt);
                proxt->insertProp(inputProps);
            }

            index++;
        }
    }
    if (customProperties) {
        for (auto t : customProperties->GetAllProperties()) {
            auto name = t->GetName();
            auto type = t->GetTypeId();
            auto tst = type.ToString();
            BASE_NS::shared_ptr<Proxy> proxt;
            if (type == META_NS::UidFromType<float>()) {
                proxt = BASE_NS::shared_ptr { new TypeProxy<float>(t) };
            }
            if (type == META_NS::UidFromType<int32_t>()) {
                proxt = BASE_NS::shared_ptr { new TypeProxy<int32_t>(t) };
            }
            if (type == META_NS::UidFromType<uint32_t>()) {
                proxt = BASE_NS::shared_ptr { new TypeProxy<uint32_t>(t) };
            }
            if (type == META_NS::UidFromType<BASE_NS::Math::Vec2>()) {
                proxt = BASE_NS::shared_ptr { new PropProxy<Vec2Proxy>(e, t) };
            }
            if (type == META_NS::UidFromType<BASE_NS::Math::Vec3>()) {
                proxt = BASE_NS::shared_ptr { new PropProxy<Vec3Proxy>(e, t) };
            }
            if (type == META_NS::UidFromType<BASE_NS::Math::Vec4>()) {
                proxt = BASE_NS::shared_ptr { new PropProxy<Vec4Proxy>(e, t) };
            }
            if (proxt) {
                proxies_.push_back(proxt);
                proxt->insertProp(inputProps);
            }
        }
    }
    if (!inputProps.empty()) {
        napi_define_properties(e, inputs, inputProps.size(), inputProps.data());
    }

    inputs_ = { e, inputs };
    meJs.Set("inputs", inputs_.GetValue());
}

ShaderJS::~ShaderJS()
{
    DisposeNative();
}

void* ShaderJS::GetInstanceImpl(uint32_t id)
{
    if (id == ShaderJS::ID) {
        return this;
    }
    return SceneResourceImpl::GetInstanceImpl(id);
}

void ShaderJS::DisposeNative()
{
    if (!disposed_) {
        disposed_ = true;
        SetNativeObject(nullptr, false);
        SetNativeObject(nullptr, true);
        NapiApi::Object obj = scene_.GetObject();
        if (obj) {
            auto* tro = obj.Native<TrueRootObject>();

            if (tro) {
                SceneJS* sceneJS = ((SceneJS*)tro->GetInstanceImpl(SceneJS::ID));
                sceneJS->ReleaseDispose((uintptr_t)&scene_);
            }
        }
        proxies_.clear();
        inputs_.Reset();
        scene_.Reset();
    }
}
void ShaderJS::Finalize(napi_env env)
{
    DisposeNative();
    BaseObject<ShaderJS>::Finalize(env);
}
