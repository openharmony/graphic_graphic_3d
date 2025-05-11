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

#include <meta/api/util.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_image.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_scene.h>

#include "SceneJS.h"
#include "Vec2Proxy.h"
#include "Vec3Proxy.h"
#include "Vec4Proxy.h"

using IntfPtr = META_NS::SharedPtrIInterface;
using IntfWeakPtr = META_NS::WeakPtrIInterface;

void ShaderJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;
    BASE_NS::vector<napi_property_descriptor> node_props;

    SceneResourceImpl::GetPropertyDescs(node_props);

    napi_value func;
    auto status = napi_define_class(env, "Shader", NAPI_AUTO_LENGTH, BaseObject::ctor<ShaderJS>(), nullptr,
        node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, reinterpret_cast<void**>(&mis));
    mis->StoreCtor("Shader", func);
}

ShaderJS::ShaderJS(napi_env e, napi_callback_info i)
    : BaseObject(e, i), SceneResourceImpl(SceneResourceImpl::SHADER)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    NapiApi::Object meJs(fromJs.This());

    NapiApi::Object scene = fromJs.Arg<0>(); // access to owning scene...
    NapiApi::Object args = fromJs.Arg<1>();  // other args
    scene_ = { scene };
    if (!scene_.GetObject().GetNative<SCENE_NS::IScene>()) {
        LOG_F("INVALID SCENE!");
    }

    if (const auto sceneJS = scene_.GetObject().GetJsWrapper<SceneJS>()) {
        sceneJS->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
    }

    NapiApi::Object material = args.Get<NapiApi::Object>("Material"); // see if we SHOULD be bound to a material.
    if (material) {
        BindToMaterial(meJs, material);
    }

    BASE_NS::string name;
    if (auto prm = args.Get<BASE_NS::string>("name"); prm.IsDefined()) {
        name = prm;
    } else {
        if (const auto named = GetNativeObject()) {
            name = named->GetName();
        }
    }
    meJs.Set("name", name);
}

void ShaderJS::BindToMaterial(NapiApi::Object meJs, NapiApi::Object material)
{
    // unbind existing inputs.
    UnbindInputs();

    // inputs are actually owned (and used) by the material.
    // create the input object
    NapiApi::Object inputs(meJs.GetEnv());

    napi_env e = inputs.GetEnv();
    auto* tro = material.GetRoot();
    auto mat = interface_pointer_cast<SCENE_NS::IMaterial>(tro->GetNativeObject());

    BASE_NS::vector<napi_property_descriptor> inputProps;

    META_NS::IMetadata::Ptr customProperties;
    BASE_NS::vector<SCENE_NS::ITexture::Ptr> Textures = mat->Textures()->GetValue();
    customProperties = mat->GetCustomProperties();

    if (!Textures.empty()) {
        int index = 0;
        for (auto t : Textures) {
            BASE_NS::string name;
            auto nn = interface_cast<META_NS::IObject>(t);
            if (nn) {
                name = nn->GetName();
            } else {
                name = "TextureInfo_" + BASE_NS::to_string(index);
            }
            BASE_NS::shared_ptr<PropertyProxy> proxt;
            // factor
            if (proxt = BASE_NS::shared_ptr { new Vec4Proxy(e, t->Factor()) }) {
                auto n = (name.empty() ? BASE_NS::string_view("") : name + BASE_NS::string_view("_")) +
                         t->Factor()->GetName();

                const auto& res = proxies_.insert_or_assign(n, proxt);
                inputProps.push_back(CreateProxyDesc(res.first->first.c_str(), BASE_NS::move(proxt)));
            }

            if (proxt = BASE_NS::shared_ptr { new ImageProxy(scene_.GetObject(), inputs, t->Image()) }) {
                auto n = (name.empty() ? BASE_NS::string_view("") : name + BASE_NS::string_view("_")) +
                         t->Image()->GetName();
                const auto& res = proxies_.insert_or_assign(n, proxt);
                inputProps.push_back(CreateProxyDesc(res.first->first.c_str(), BASE_NS::move(proxt)));
            }
            index++;
        }
    }
    // default stuff
    {
        auto proxt = BASE_NS::shared_ptr { new TypeProxy<float>(inputs, mat->AlphaCutoff()) };
        if (proxt) {
            auto res = proxies_.insert_or_assign(mat->AlphaCutoff()->GetName(), proxt);
            inputProps.push_back(CreateProxyDesc(res.first->first.c_str(), BASE_NS::move(proxt)));
        }
    }
    if (customProperties) {
        BASE_NS::shared_ptr<CORE_NS::IInterface> intsc;
        if (auto scene = scene_.GetObject().GetNative<SCENE_NS::IScene>()) {
            if (auto ints = scene->GetInternalScene()) {
                intsc = interface_pointer_cast<CORE_NS::IInterface>(ints);
            }
        }
        if (intsc) {
            for (auto t : customProperties->GetProperties()) {
                BASE_NS::shared_ptr<PropertyProxy> proxt = PropertyToProxy(scene_.GetObject(), inputs, t);
                if (proxt) {
                    proxt->SetExtra(intsc);
                    auto res = proxies_.insert_or_assign(t->GetName(), proxt);
                    inputProps.push_back(CreateProxyDesc(res.first->first.c_str(), BASE_NS::move(proxt)));
                }
            }
        }
    }
    if (!inputProps.empty()) {
        napi_define_properties(e, inputs.ToNapiValue(), inputProps.size(), inputProps.data());
    }

    inputs_ = NapiApi::StrongRef(inputs);
    meJs.Set("inputs", inputs_.GetValue());
}

ShaderJS::~ShaderJS()
{
    DisposeNative(nullptr);
}

void* ShaderJS::GetInstanceImpl(uint32_t id)
{
    if (id == ShaderJS::ID)
        return this;
    return SceneResourceImpl::GetInstanceImpl(id);
}
void ShaderJS::UnbindInputs()
{
    /// destroy the input object.
    if (!inputs_.IsEmpty()) {
        NapiApi::Object inp = inputs_.GetObject();
        while (!proxies_.empty()) {
            auto it = proxies_.begin();
            // removes hooks for meta property & jsproperty.
            inp.DeleteProperty(it->first);
            // destroy the proxy.
            proxies_.erase(it);
        }
    }
    inputs_.Reset();
}
void ShaderJS::DisposeNative(void* in)
{
    if (!disposed_) {
        disposed_ = true;
        UnbindInputs();
        UnsetNativeObject();

        if (const auto sceneJS = scene_.GetObject().GetJsWrapper<SceneJS>()) {
            sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
        }
        scene_.Reset();
    }
}
void ShaderJS::Finalize(napi_env env)
{
    DisposeNative(scene_.GetObject().GetJsWrapper<SceneJS>());
    BaseObject::Finalize(env);
}
