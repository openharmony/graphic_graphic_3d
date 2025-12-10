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

#include "JsObjectCache.h"
#include "MaterialJS.h"
#include "SceneJS.h"
#include "Vec2Proxy.h"
#include "Vec3Proxy.h"
#include "Vec4Proxy.h"
#include "lume_trace.h"

using IntfPtr = META_NS::SharedPtrIInterface;
using IntfWeakPtr = META_NS::WeakPtrIInterface;

void ShaderJS::Init(napi_env env, napi_value exports)
{
    LUME_TRACE_FUNC()
    BASE_NS::vector<napi_property_descriptor> node_props;
    SceneResourceImpl::GetPropertyDescs(node_props);

#define NAPI_API_JS_NAME Shader
    DeclareMethod("SetInputs", SetInputs, NapiApi::Object);
    DeclareGet(NapiApi::Object, "inputs", GetInputs);

    DeclareClass();
#undef NAPI_API_JS_NAME
}

ShaderJS::ShaderJS(napi_env e, napi_callback_info i)
    : BaseObject(e, i), SceneResourceImpl(SceneResourceImpl::SHADER)
{
    LUME_TRACE_FUNC()
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    NapiApi::Object meJs(fromJs.This());
    AddBridge("ShaderJS",meJs);
    NapiApi::Object scene = fromJs.Arg<0>(); // access to owning scene...
    NapiApi::Object args = fromJs.Arg<1>();  // other args
    scene_ = { scene };

    if (const auto sceneJS = scene.GetJsWrapper<SceneJS>()) {
        sceneJS->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
    } else if (const auto renderContextJs = scene.GetJsWrapper<RenderContextJS>()) {
        renderContextJs->GetResources()->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
    } else {
        LOG_F("Trying to construct ShaderJS instance without a valid scene or rendercontext");
    }

    NapiApi::Object material = args.Get<NapiApi::Object>("Material"); // see if we SHOULD be bound to a material.
    if (material) {
        BindToMaterial(meJs, material);
    }
    if (args.Get("uri")) {
        SetUri(args);
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

NapiApi::Object GetRenderContext(NapiApi::Object scene) {
    if (scene) {
        NapiApi::Function func = scene.Get<NapiApi::Function>("getRenderContext");
        if (func) {
            return NapiApi::Object(scene.GetEnv(), func.Invoke(scene));
        }
    }
    return NapiApi::Object{};
}

SCENE_NS::IMaterial::Ptr ShaderJS::PrepareBind(NapiApi::Object& inputs, NapiApi::Object& material,
    NapiApi::Object& meJs)
{
    LUME_TRACE_FUNC()
    // unbind existing inputs.
    UnbindInputs(meJs);
    material_ = NapiApi::StrongRef(material);

    auto* tro = material.GetRoot();
    if (!tro) {
        LOG_E("null material root object");
        return {};
    }
    auto materialJs = material.GetJsWrapper<MaterialJS>();
    if (!materialJs) {
        LOG_E("null material js wrapper.");
        return {};
    }
    if (!GetRenderContext(materialJs->GetSceneObject())) {
        LOG_E("no render (/dispose) context");
        return {};
    }
    auto mat = interface_pointer_cast<SCENE_NS::IMaterial>(tro->GetNativeObject());
    if (!mat) {
        LOG_E("null material engine object");
    }

    return mat;
}

void ShaderJS::SetDefaults(NapiApi::Object &inputs, NapiApi::Object &material, SCENE_NS::IMaterial::Ptr &mat,
    BASE_NS::vector<napi_property_descriptor> &inputProps, META_NS::IMetadata::Ptr customProperties)
{
    LUME_TRACE_FUNC()
    auto proxt = BASE_NS::shared_ptr { new TypeProxy<float>(inputs, mat->AlphaCutoff()) };
    if (proxt) {
        auto res = proxies_.insert_or_assign(mat->AlphaCutoff()->GetName(), proxt);
        inputProps.push_back(CreateProxyDesc(res.first->first.c_str(), BASE_NS::move(proxt)));
    }
    if (customProperties) {
        BASE_NS::shared_ptr<CORE_NS::IInterface> intsc;
        auto materialJs = material.GetJsWrapper<MaterialJS>(); // this was initially checked (!= null)
        if (!materialJs) {
            return;
        }
        if (auto scene = materialJs->GetSceneObject().GetNative<SCENE_NS::IScene>()) {
            if (auto ints = scene->GetInternalScene()) {
                intsc = interface_pointer_cast<CORE_NS::IInterface>(ints);
            }
        }
        if (intsc) {
            for (auto t : customProperties->GetProperties()) {
                BASE_NS::shared_ptr<PropertyProxy> proxt = PropertyToProxy(materialJs->GetSceneObject(), inputs, t);
                if (proxt) {
                    proxt->SetExtra(intsc);
                    auto res = proxies_.insert_or_assign(t->GetName(), proxt);
                    inputProps.push_back(CreateProxyDesc(res.first->first.c_str(), BASE_NS::move(proxt)));
                }
            }
        }
    }
    inputs_ = NapiApi::StrongRef(inputs);
}

void ShaderJS::DefineProperties(
    NapiApi::Object& inputs, napi_env& e, BASE_NS::vector<napi_property_descriptor>& inputProps)
{
    LUME_TRACE_FUNC()
    if (!inputProps.empty()) {
        napi_define_properties(e, inputs.ToNapiValue(), inputProps.size(), inputProps.data());
    }
}

constexpr BASE_NS::string_view UNDS("_");
void ShaderJS::BindToMaterial(NapiApi::Object meJs, NapiApi::Object material)
{
    LUME_TRACE_FUNC()
    UnbindInputs(meJs);
    material_ = NapiApi::StrongRef(material);

    auto* tro = material.GetRoot();
    if (!tro) {
        LOG_E("null material root object");
        return;
    }
    auto materialJs = material.GetJsWrapper<MaterialJS>();
    if (!materialJs) {
        LOG_E("null material js wrapper.");
        return;
    }
    if (!GetRenderContext(materialJs->GetSceneObject())) {
        LOG_E("no render (/dispose) context");
        return;
    }
}

ShaderJS::~ShaderJS()
{
    LUME_TRACE_FUNC()
    DisposeNative(nullptr);
    DestroyBridge(this);
}

void* ShaderJS::GetInstanceImpl(uint32_t id)
{
    LUME_TRACE_FUNC()
    if (id == ShaderJS::ID) {
        return static_cast<ShaderJS*>(this);
    }
    if (auto ret = SceneResourceImpl::GetInstanceImpl(id)) {
        return ret;
    }
    return BaseObject::GetInstanceImpl(id);
}
void ShaderJS::UnbindInputs(NapiApi::Object meJs)
{
    LUME_TRACE_FUNC()
    /// destroy the input object.
    if (!inputs_.IsEmpty()) {
        NapiApi::Object inp = inputs_.GetObject();
        while (!proxies_.empty()) {
            auto it = proxies_.begin();
            // removes hooks for meta property & jsproperty.
            auto ret = inp.DeleteProperty(it->first);
            LOG_V("#### Release %s %zu %d", it->first.c_str(), proxies_.size(), ret);
            // destroy the proxy.
            proxies_.erase(it);
        }
        if (meJs) {
            meJs.DeleteProperty("inputs");
        }
        inputs_.Reset();
    }
    isInputsReflValid_ = false;
}
void ShaderJS::DisposeNative(void* in)
{
    LUME_TRACE_FUNC()
    if (!disposed_) {
        disposed_ = true;
        DisposeBridge(this);
        UnbindInputs(FetchJsObj(GetNativeObject()));
        if (auto native = GetNativeObject<META_NS::IObject>()) {
            DetachJsObj(native, "_JSW");
        }
        UnsetNativeObject();
        // see if we were bound to material
        if (auto obj = material_.GetObject()) {
            // if so, reset the material default shader.
            NapiApi::Env e(obj.GetEnv());
            obj.Set("colorShader", e.GetUndefined());
        }
        material_.Reset();
        if (auto* sceneJS = static_cast<SceneJS*>(in)) {
            sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
        } else {
            if (const auto sceneJS = scene_.GetJsWrapper<SceneJS>()) {
                sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
            }
            if (const auto renderContextJs = scene_.GetJsWrapper<RenderContextJS>()) {
                renderContextJs->GetResources()->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
            }
        }

        scene_.Reset();
    }
}
void ShaderJS::Finalize(napi_env env)
{
    LUME_TRACE_FUNC()
    DisposeNative(nullptr);
    BaseObject::Finalize(env);
    FinalizeBridge(this);
}

void ShaderJS::DetachFromMaterial(NapiApi::Object meJs)
{
    LUME_TRACE_FUNC()
    // convert this back to a "kind of non bound" shader object.
    if (const auto native = GetNativeObject<META_NS::IMetadata>()) {
        SetNativeObject(GetNativeObject(), PtrType::STRONG);
    }
    material_.Reset();
    // delete the inputs, as we are no longer bound to a material.
    UnbindInputs(meJs);
}

template<class type>
ShaderJS::InputValueType ShaderJS::ParseInputs(NapiApi::Object& obj, BASE_NS::string_view key)
{
    auto v = obj.Get<type>(key);
    if (v.IsUndefinedOrNull()) {
        return InputValueType {};
    }
    if constexpr (BASE_NS::is_same_v<type, NapiApi::Object>) {
        // parse object
        NapiApi::Object jsValue = v.valueOrDefault();
        auto x = jsValue.Get<float>("x");
        auto y = jsValue.Get<float>("y");
        auto z = jsValue.Get<float>("z");
        auto w = jsValue.Get<float>("w");
        if (x.IsValid() || y.IsValid() || z.IsValid() || w.IsValid()) {
            if (w.IsValid()) {
                return BASE_NS::Math::Vec4 { x, y, z, w };
            } else if (z.IsValid()) {
                return BASE_NS::Math::Vec3 { x, y, z };
            } else {
                return BASE_NS::Math::Vec2 { x, y };
            }
        } else {
            return jsValue.GetNative<SCENE_NS::IImage>();
        }
    } else {
        auto value = static_cast<type>(v);
        return value;
    }
}

void ShaderJS::SetInput(BASE_NS::string_view key, InputValueType& value)
{
    SCENE_NS::IMaterial::Ptr mat;
    if (auto material = material_.GetObject()) {
        auto* tro = material.GetRoot();
        if (!tro) {
            LOG_E("null material root object");
            return;
        }
        auto materialJs = material.GetJsWrapper<MaterialJS>();
        if (!materialJs) {
            LOG_E("null material js wrapper.");
            return;
        }
        if (!GetRenderContext(materialJs->GetSceneObject())) {
            LOG_E("no render (/dispose) context");
            return;
        }
        mat = interface_pointer_cast<SCENE_NS::IMaterial>(tro->GetNativeObject());
    }
    if (!mat) {
        LOG_E("null material pointer");
        return;
    }
    META_NS::IMetadata::Ptr customProperties = mat->GetCustomProperties();
    for (auto& prop : customProperties->GetProperties()) {
        if (key == prop->GetName()) {
            std::visit([&prop](auto&& arg) {
                using Type = std::decay_t<decltype(arg)>;
                if constexpr (!BASE_NS::is_same_v<Type, std::monostate>) {
                    META_NS::Property<Type>(prop)->SetValue(arg);
                }
            }, value);
            return;
        }
    }
    BASE_NS::vector<SCENE_NS::ITexture::Ptr> textures = mat->Textures()->GetValue();
    for (size_t index = 0; index < textures.size(); ++index) {
        auto& texture = textures[index];
        BASE_NS::string prefix;
        auto nn = interface_cast<META_NS::IObject>(texture);
        if (nn) {
            prefix = nn->GetName();
        } else {
            prefix = "TextureInfo_" + BASE_NS::to_string(index);
        }
        if (!key.starts_with(prefix)) {
            continue;
        }
        if (auto ptr = std::get_if<BASE_NS::Math::Vec4>(&value);
            ptr && key == prefix + UNDS + texture->Factor()->GetName()) {
            texture->Factor()->SetValue(*ptr);
            return;
        }
        if (auto ptr = std::get_if<SCENE_NS::IImage::Ptr>(&value);
            ptr && key == prefix + UNDS + texture->Image()->GetName()) {
            texture->Image()->SetValue(*ptr);
            return;
        }
    }
    if (auto ptr = std::get_if<float>(&value); ptr && key == mat->AlphaCutoff()->GetName()) {
        mat->AlphaCutoff()->SetValue(*ptr);
        return;
    }

}

napi_value ShaderJS::SetInputs(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    NapiApi::Object obj = ctx.Arg<0>();
    napi_value jsPropertyNames;
    napi_get_property_names(obj.GetEnv(), obj.ToNapiValue(), &jsPropertyNames);
    NapiApi::Array propertyNames(obj.GetEnv(), jsPropertyNames);
    size_t propertyLength = propertyNames.Count();

    NapiApi::Object meJs(ctx.This());

    for (size_t i = 0; i < propertyLength; ++i) {
        auto key = NapiApi::Value<BASE_NS::string> { obj.GetEnv(), propertyNames.Get_value(i) }.valueOrDefault();
        if (!obj.Has(key)) {
            continue;
        }

        auto parsedResult = ParseInputs<float>(obj, key);
        if (std::holds_alternative<std::monostate>(parsedResult)) {
            parsedResult = ParseInputs<NapiApi::Object>(obj, key);
        }
        if (std::holds_alternative<std::monostate>(parsedResult)) {
            continue;
        }
        SetInput(key, parsedResult);
    }

    return ctx.GetUndefined();
}

napi_value ShaderJS::GetInputs(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC();
    if (isInputsReflValid_) {
        return inputs_.GetValue();
    }

    if (!material_.GetObject()) {
        return ctx.GetNull();
    }

    InitInputs(ctx.This(), material_.GetObject());
    isInputsReflValid_ = true;
    return inputs_.GetValue();
}

void ShaderJS::InitInputs(NapiApi::Object meJs, NapiApi::Object material)
{
    NapiApi::Object inputs(meJs.GetEnv());
    napi_env e = inputs.GetEnv();
    if (auto mat = PrepareBind(inputs, material, meJs)) {
	    auto materialJs = material.GetJsWrapper<MaterialJS>();
        BASE_NS::vector<napi_property_descriptor> inputProps;
        META_NS::IMetadata::Ptr customProperties = mat->GetCustomProperties();
        BASE_NS::vector<SCENE_NS::ITexture::Ptr> Textures = mat->Textures()->GetValue();
        NapiApi::Object sceneObject;
        if (materialJs) {
            sceneObject = materialJs->GetSceneObject();
        }
        if (!Textures.empty() && sceneObject) {
            static constexpr BASE_NS::string_view default_names[] = { "BASE_COLOR", "NORMAL", "MATERIAL", "EMISSIVE",
             "AO", "CLEARCOAT", "CLEARCOAT_ROUGHNESS", "CLEARCOAT_NORMAL", "SHEEN", "TRANSMISSION", "SPECULAR" };
            proxies_.reserve(Textures.size() * 4);
            inputProps.reserve(Textures.size() * 4);
            for (size_t index = 0; index < Textures.size(); index++) {
                auto& t = Textures[index];
                auto nn = interface_cast<META_NS::IObject>(t);
                BASE_NS::string name = nn ?
                    nn->GetName() : BASE_NS::string("TextureInfo_").append(BASE_NS::to_string(index));
                const auto factorProp = t->Factor();
                const auto imageProp = t->Image();
                const auto imageBase = UNDS + imageProp->GetName();
                const auto factorBase = UNDS + factorProp->GetName();
                const BASE_NS::string factorName = name.empty() ? "" : name + factorBase;
                auto factorProxy = BASE_NS::shared_ptr { new Vec4Proxy(e, factorProp) };
                if (factorProxy) {
                    const auto& res = proxies_.insert_or_assign(factorName, factorProxy);
                    inputProps.push_back(CreateProxyDesc(res.first->first.c_str(), factorProxy));
                }
                auto imageProxy = BASE_NS::shared_ptr { new ImageProxy(sceneObject, inputs, imageProp) };
                const BASE_NS::string imageName = name.empty() ? "" : name + imageBase;
                if (imageProxy) {
                    const auto& res = proxies_.insert_or_assign(imageName, imageProxy);
                    inputProps.push_back(CreateProxyDesc(res.first->first.c_str(), imageProxy));
                }
                if (name != default_names[index]) {
                    if (factorProxy) {
                        auto n = default_names[index] + factorBase;
                        const auto& res = proxies_.insert_or_assign(n, factorProxy);
                        inputProps.push_back(CreateProxyDesc(res.first->first.c_str(), BASE_NS::move(factorProxy)));
                    }
                    if (imageProxy) {
                        auto n = default_names[index] + imageBase;
                        const auto& res = proxies_.insert_or_assign(n, imageProxy);
                        inputProps.push_back(CreateProxyDesc(res.first->first.c_str(), BASE_NS::move(imageProxy)));
                    }
                }
            }
        }
        SetDefaults(inputs, material, mat, inputProps, customProperties);
        DefineProperties(inputs, e, inputProps);
    }
}