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
#include "MaterialJS.h"

#include <meta/api/make_callback.h>
#include <meta/api/util.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/construct_property.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_shader.h>

#include "JsObjectCache.h"
#include "MaterialPropertyJS.h"
#include "SceneJS.h"
#include "ShaderJS.h"
#include "lume_trace.h"
#include "MaterialPropertyJS.h"
using IntfPtr = META_NS::SharedPtrIInterface;
using IntfWeakPtr = META_NS::WeakPtrIInterface;

BaseMaterial::BaseMaterial(MaterialType lt) : SceneResourceImpl(SceneResourceImpl::MATERIAL), materialType_(lt) {}
BaseMaterial::~BaseMaterial() {}
void BaseMaterial::Init(const char* class_name, napi_env env, napi_value exports, napi_callback ctor,
    BASE_NS::vector<napi_property_descriptor>& node_props)
{
    LUME_TRACE_FUNC()
    SceneResourceImpl::GetPropertyDescs(node_props);
    node_props.emplace_back(TROGetProperty<uint32_t, BaseMaterial, &BaseMaterial::GetMaterialType>("materialType"));
    node_props.emplace_back(TROGetSetProperty<std::optional<bool>, BaseMaterial, &BaseMaterial::GetShadowReceiver,
        &BaseMaterial::SetShadowReceiver>("shadowReceiver"));
    node_props.emplace_back(TROGetSetProperty<std::optional<uint32_t>, BaseMaterial, &BaseMaterial::GetCullMode,
        &BaseMaterial::SetCullMode>("cullMode"));
    node_props.emplace_back(TROGetSetProperty<std::optional<uint32_t>, BaseMaterial, &BaseMaterial::GetPolygonMode,
        &BaseMaterial::SetPolygonMode>("polygonMode"));
    node_props.emplace_back(
        TROGetSetProperty<NapiApi::Object, BaseMaterial, &BaseMaterial::GetBlend, &BaseMaterial::SetBlend>("blend"));
    node_props.emplace_back(TROGetSetProperty<std::optional<float>, BaseMaterial, &BaseMaterial::GetAlphaCutoff,
        &BaseMaterial::SetAlphaCutoff>("alphaCutoff"));
    node_props.emplace_back(
        TROGetSetProperty<NapiApi::Object, BaseMaterial, &BaseMaterial::GetRenderSort, &BaseMaterial::SetRenderSort>(
            "renderSort"));
    napi_value func;
    auto status = napi_define_class(
        env, class_name, NAPI_AUTO_LENGTH, ctor, nullptr, node_props.size(), node_props.data(), &func);
    if (status != napi_ok) {
        LOG_E("export class failed in %s", __func__);
    }
    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    if (mis) {
        mis->StoreCtor(class_name, func);
    }
    InitEnumType(env, exports);
}

void BaseMaterial::InitEnumType(napi_env env, napi_value exports)
{
    NapiApi::Object exp1(env, exports);
    napi_value eType1 = nullptr;
    napi_value v1 = nullptr;
    napi_create_object(env, &eType1);
#define DECL_ENUM(enu, x)                          \
    napi_create_uint32(env, MaterialType::x, &v1); \
    napi_set_named_property(env, enu, #x, v1)
    DECL_ENUM(eType1, SHADER);
    DECL_ENUM(eType1, METALLIC_ROUGHNESS);
    DECL_ENUM(eType1, UNLIT);
    DECL_ENUM(eType1, OCCLUSION);
#undef DECL_ENUM
    exp1.Set("MaterialType", eType1);
    NapiApi::Object exp2(env, exports);
    napi_value eType2 = nullptr;
    napi_value v2 = nullptr;
    napi_create_object(env, &eType2);
#define DECL_ENUM(enu, x)                      \
    napi_create_uint32(env, CullMode::x, &v2); \
    napi_set_named_property(env, enu, #x, v2)
    DECL_ENUM(eType2, NONE);
    DECL_ENUM(eType2, FRONT);
    DECL_ENUM(eType2, BACK);
#undef DECL_ENUM
    exp2.Set("CullMode", eType2);

    NapiApi::Object exp3(env, exports);
    napi_value eType3 = nullptr;
    napi_value v3 = nullptr;
    napi_create_object(env, &eType3);
#define DECL_ENUM(enu, x)                         \
    napi_create_uint32(env, PolygonMode::x, &v3); \
    napi_set_named_property(env, enu, #x, v3)

    DECL_ENUM(eType3, FILL);
    DECL_ENUM(eType3, LINE);
    DECL_ENUM(eType3, POINT);
#undef DECL_ENUM
    exp3.Set("PolygonMode", eType3);
}

void* BaseMaterial::GetInstanceImpl(uint32_t id)
{
    LUME_TRACE_FUNC()
    if (id == BaseMaterial::ID)
        return (BaseMaterial*)this;
    return SceneResourceImpl::GetInstanceImpl(id);
}
void BaseMaterial::DisposeNative(BaseObject* tro)
{
    LUME_TRACE_FUNC()
    // do nothing for now..
    LOG_V("BaseMaterial::DisposeNative");
    shaderChanged_.Unsubscribe();
    blend_ = {};
    tro->UnsetNativeObject();
    scene_.Reset();
    defaultCullMode_.reset();
}
napi_value BaseMaterial::GetMaterialType(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    uint32_t type = -1; // return -1 if the object does not exist anymore
    if (auto material = ctx.This().GetNative<SCENE_NS::IMaterial>()) {
        if (META_NS::GetValue(material->Type()) == SCENE_NS::MaterialType::METALLIC_ROUGHNESS) {
            type = BaseMaterial::METALLIC_ROUGHNESS;
        } else if (META_NS::GetValue(material->Type()) == SCENE_NS::MaterialType::UNLIT) {
            type = BaseMaterial::UNLIT;
        } else {
            type = BaseMaterial::SHADER;
        }
    }
    return ctx.GetNumber(type);
}

SCENE_NS::IShader::Ptr BaseMaterial::GetMaterialShader(const NapiApi::Object& me) const
{
    if (validateSceneRef()) {
        auto material = me.GetNative<SCENE_NS::IMaterial>();
        if (material) {
            return META_NS::GetValue(material->MaterialShader());
        }
    }
    return {};
}

napi_value BaseMaterial::GetShadowReceiver(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    bool shadowReceiver = false;
    if (auto material = ctx.This().GetNative<SCENE_NS::IMaterial>()) {
        uint32_t lightingFlags = uint32_t(META_NS::GetValue(material->LightingFlags()));
        shadowReceiver = lightingFlags & uint32_t(SCENE_NS::LightingFlags::SHADOW_RECEIVER_BIT);
    }
    return ctx.GetBoolean(shadowReceiver);
}
void BaseMaterial::SetShadowReceiver(NapiApi::FunctionContext<std::optional<bool>>& ctx)
{
    LUME_TRACE_FUNC()
    if (!validateSceneRef()) {
        return;
    }
    auto material = ctx.This().GetNative<SCENE_NS::IMaterial>();
    if (material) {
        std::optional<bool> value = ctx.Arg<0>();
        bool shadowReceiver = value.value_or(true); // If undefined default to true
        uint32_t lightingFlags = uint32_t(META_NS::GetValue(material->LightingFlags()));
        if (shadowReceiver) {
            lightingFlags |= uint32_t(SCENE_NS::LightingFlags::SHADOW_RECEIVER_BIT);
        } else {
            lightingFlags &= ~uint32_t(SCENE_NS::LightingFlags::SHADOW_RECEIVER_BIT);
        }
        META_NS::SetValue(material->LightingFlags(), static_cast<SCENE_NS::LightingFlags>(lightingFlags));
    }
}

void BaseMaterial::ShaderChanged(SCENE_NS::IShader::Ptr& shader)
{
    defaultCullMode_.reset();
    StoreDefaultCullMode(shader);
}

void BaseMaterial::StoreDefaultCullMode(const SCENE_NS::IShader::Ptr& shader)
{
    // Store when first called, not after
    if (!defaultCullMode_.has_value() && shader) {
        defaultCullMode_ = META_NS::GetValue(shader->CullMode());
    }
}

napi_value BaseMaterial::GetCullMode(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    auto shader = GetMaterialShader(ctx.This());
    if (shader) {
        auto cullMode = META_NS::GetValue(shader->CullMode());
        return ctx.GetNumber(static_cast<uint32_t>(cullMode));
    }
    return ctx.GetUndefined();
}

void BaseMaterial::SetCullMode(NapiApi::FunctionContext<std::optional<uint32_t>>& ctx)
{
    LUME_TRACE_FUNC()
    auto shader = GetMaterialShader(ctx.This());
    if (shader) {
        // Default value of cull mode depends on the shader, so store the default value when the user
        // changes it to be used later if the value is set to undefined
        StoreDefaultCullMode(shader);
        SetPropertyValue<uint32_t, 0, SCENE_NS::CullModeFlags>(ctx, shader->CullMode(), defaultCullMode_);
    }
}

napi_value BaseMaterial::GetPolygonMode(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    auto shader = GetMaterialShader(ctx.This());
    if (shader) {
        auto polyMode = SCENE_NS::PolygonMode::FILL;
        polyMode = META_NS::GetValue(shader->PolygonMode());
        return ctx.GetNumber(static_cast<uint32_t>(polyMode));
    }
    return ctx.GetUndefined();
}

void BaseMaterial::SetPolygonMode(NapiApi::FunctionContext<std::optional<uint32_t>>& ctx)
{
    LUME_TRACE_FUNC()
    auto shader = GetMaterialShader(ctx.This());
    if (shader) {
        SetPropertyValue<uint32_t, 0, SCENE_NS::PolygonMode>(ctx, shader->PolygonMode());
    }
}
napi_value BaseMaterial::GetBlend(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    if (auto proxy = blend_.GetObject()) {
        return proxy.ToNapiValue();
    }
    // Create an Object and add one property "blend" to it which maps to shader->Blend() (Property<bool>)
    if (const auto material = ctx.This().GetNative<SCENE_NS::IMaterial>()) {
        if (const auto shader = META_NS::GetValue(material->MaterialShader())) {
            NapiApi::Object blendProxy(ctx.Env());
            blend_.objectProxy = NapiApi::StrongRef(blendProxy);
            blend_.enabledProxy = PropertyToProxy(scene_.GetNapiObject(), blendProxy, shader->Blend());
            // Currently only one property in Blend class, but setting through a vector to make it easier
            // to add new props later
            if (blend_.enabledProxy) {
                BASE_NS::vector<napi_property_descriptor> napi_descs;
                napi_descs.push_back(CreateProxyDesc("enabled", blend_.enabledProxy));
                // Add new properties defined in the future here
                napi_define_properties(
                    blendProxy.GetEnv(), blendProxy.ToNapiValue(), napi_descs.size(), napi_descs.data());
            }
        }
        // If shader changes we need to re-inítialize the blend proxy
        shaderChanged_.Subscribe(material->MaterialShader(), [this]() { blend_ = {}; });
    }
    return blend_.GetObject().ToNapiValue();
}
void BaseMaterial::SetBlend(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    auto shader = GetMaterialShader(ctx.This());
    if (shader) {
        NapiApi::Object blendObjectJS = ctx.Arg<0>();
        auto value = blendObjectJS.Get<bool>("enabled");
        auto prop = shader->Blend();
        if (value.IsDefinedAndNotNull()) {
            META_NS::SetValue(prop, value);
        } else {
            prop->ResetValue();
        }
    }
}
napi_value BaseMaterial::GetAlphaCutoff(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    float alphaCutoff = 1.0f;
    if (auto material = ctx.This().GetNative<SCENE_NS::IMaterial>()) {
        alphaCutoff = META_NS::GetValue(material->AlphaCutoff());
    }
    return ctx.GetNumber(alphaCutoff);
}
void BaseMaterial::SetAlphaCutoff(NapiApi::FunctionContext<std::optional<float>>& ctx)
{
    LUME_TRACE_FUNC()
    if (!validateSceneRef()) {
        return;
    }
    auto material = ctx.This().GetNative<SCENE_NS::IMaterial>();
    if (material) {
        SetPropertyValue<float, 0, float>(ctx, material->AlphaCutoff());
    }
}
napi_value BaseMaterial::GetRenderSort(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto material = ctx.This().GetNative<SCENE_NS::IMaterial>();
    if (!material) {
        return ctx.GetUndefined();
    }

    auto renderSort = META_NS::GetValue(material->RenderSort());

    NapiApi::Object renderSortJS(ctx.Env());
    renderSortJS.Set("renderSortLayer", ctx.GetNumber(renderSort.renderSortLayer));
    renderSortJS.Set("renderSortLayerOrder", ctx.GetNumber(renderSort.renderSortLayerOrder));
    return renderSortJS.ToNapiValue();
}
void BaseMaterial::SetRenderSort(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    if (!validateSceneRef()) {
        return;
    }
    auto material = ctx.This().GetNative<SCENE_NS::IMaterial>();
    if (!material) {
        // material destroyed, just make sure we have no shader reference anymore.
        return;
    }
    SCENE_NS::RenderSort renderSort;
    bool isset = false;

    if (NapiApi::Object renderSortJS = ctx.Arg<0>(); renderSortJS.IsDefined()) {
        auto layer = renderSortJS.Get<uint32_t>("renderSortLayer");
        auto order = renderSortJS.Get<uint32_t>("renderSortLayerOrder");
        if (layer.IsDefinedAndNotNull()) {
            renderSort.renderSortLayer = layer;
            isset = true;
        }
        if (order.IsDefinedAndNotNull()) {
            renderSort.renderSortLayerOrder = order;
            isset = true;
        }
    }
    if (isset) {
        META_NS::SetValue(material->RenderSort(), renderSort);
    } else {
        material->RenderSort()->ResetValue();
    }
}

//------

void MaterialJS::Init(napi_env env, napi_value exports)
{
    LUME_TRACE_FUNC()
#define MRADDPROP(index, name)                                                                    \
    NapiApi::GetSetProperty<NapiApi::Object, MaterialJS, &MaterialJS::GetMaterialProperty<index>, \
        &MaterialJS::SetMaterialProperty<index>>(name)

    BASE_NS::vector<napi_property_descriptor> props = {
        NapiApi::GetSetProperty<NapiApi::Object, MaterialJS, &MaterialJS::GetColorShader, &MaterialJS::SetColorShader>(
            "colorShader"),
        MRADDPROP(0, "baseColor"), MRADDPROP(1, "normal"), MRADDPROP(2, "material"), MRADDPROP(3, "emissive"),
        MRADDPROP(4, "ambientOcclusion"), MRADDPROP(5, "clearCoat"), MRADDPROP(6, "clearCoatRoughness"),
        MRADDPROP(7, "clearCoatNormal"), MRADDPROP(8, "sheen"), MRADDPROP(10, "specular")
    };
#undef MRADDPROP

    BaseMaterial::Init("Material", env, exports, BaseObject::ctor<MaterialJS>(), props);
}

MaterialJS::MaterialJS(napi_env e, napi_callback_info i)
    : BaseObject(e, i), BaseMaterial(BaseMaterial::MaterialType::METALLIC_ROUGHNESS)
{
    LUME_TRACE_FUNC()
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    NapiApi::Object meJs(fromJs.This());
    AddBridge("MaterialJS", meJs);

    NapiApi::Object scene = fromJs.Arg<0>(); // access to owning scene... (do i need it here?)
    NapiApi::Object args = fromJs.Arg<1>();  // other args

    scene_ = scene;
    if (!scene.GetNative<SCENE_NS::IScene>()) {
        LOG_F("INVALID SCENE!");
    }

    if (const auto sceneJS = scene.GetJsWrapper<SceneJS>()) {
        sceneJS->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
    }

    const auto material = GetNativeObject<SCENE_NS::IMaterial>();
    if (material) {
        if (META_NS::GetValue(material->Type()) == SCENE_NS::MaterialType::CUSTOM) {
            materialType_ = MaterialType::SHADER;
        } else if (META_NS::GetValue(material->Type()) == SCENE_NS::MaterialType::UNLIT) {
            materialType_ = MaterialType::UNLIT;
        } else {
            materialType_ = MaterialType::METALLIC_ROUGHNESS;
        }
    }

    BASE_NS::string name;
    if (auto prm = args.Get<BASE_NS::string>("name"); prm.IsDefined()) {
        name = prm;
    } else {
        if (auto named = interface_cast<META_NS::IObject>(material)) {
            name = named->GetName();
        }
    }
    meJs.Set("name", name);
}

MaterialJS::~MaterialJS()
{
    LUME_TRACE_FUNC()
    DestroyBridge(this);
}

NapiApi::Object MaterialJS::GetSceneObject()
{
    LUME_TRACE_FUNC()
    return scene_.GetNapiObject();
}

void* MaterialJS::GetInstanceImpl(uint32_t id)
{
    LUME_TRACE_FUNC()
    if (id == MaterialJS::ID) {
        return static_cast<MaterialJS*>(this);
    }
    if (auto ret = BaseMaterial::GetInstanceImpl(id)) {
        return ret;
    }
    return BaseObject::GetInstanceImpl(id);
}
void MaterialJS::DisposeNative(void* scn)
{
    LUME_TRACE_FUNC()
    if (disposed_) {
        return;
    }
    disposed_ = true;
    DisposeBridge(this);
    if (auto sh = shader_.GetObject().GetJsWrapper<ShaderJS>()) {
        sh->DetachFromMaterial(shader_.GetObject());
    }
    shader_.Reset();

    if (auto material = GetNativeObject<META_NS::IObject>()) {
        DetachJsObj(material, "_JSW");
    }
    if (auto* sceneJS = static_cast<SceneJS*>(scn)) {
        sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
    }

    UnsetNativeObject();
    shaderBind_.reset();

    BaseMaterial::DisposeNative(this);
}
void MaterialJS::Finalize(napi_env env)
{
    LUME_TRACE_FUNC()
    DisposeNative(scene_.GetJsWrapper<SceneJS>());
    BaseObject::Finalize(env);
    FinalizeBridge(this);
}
bool MaterialJS::EarlyExitSet(NapiApi::Object& shaderJS, SCENE_NS::IMaterial::Ptr& material)
{
    LUME_TRACE_FUNC()
    if (!material) {
        // material destroyed, just make sure we have no shader reference anymore.
        shaderBind_.reset();
        if (auto sh = shader_.GetObject().GetJsWrapper<ShaderJS>()) {
            sh->DetachFromMaterial(shader_.GetObject());
        }
        shader_.Reset();
        return true;
    }
    // check if the instance shader uses the same raw shader as input.
    if (auto curb = shader_.GetObject()) {
        if (shaderJS && shaderJS.StrictEqual(curb)) {
            // shader given as parameter is already bound.
            return true;
        }
    }
    return false;
}
// If owning scene has been destroyed. The material etc should also be gone,
// but there is a possible issue where the native object is still alive.
// Most likely could happen if scene.dispose is not called, but all references
// to the scene have been released, and garbage collection may or may not have
// been done yet. (or is partially done). If the scene is garbage collected then
// all the resources should be disposed.
void MaterialJS::SetColorShader(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    if (disposed_) {
        return;
    }
    NapiApi::Object shaderJS = ctx.Arg<0>();
    auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetNativeObject());
    if (!validateSceneRef() || EarlyExitSet(shaderJS, material)) {
        return;
    }
    if (auto sh = shader_.GetObject().GetJsWrapper<ShaderJS>()) {
        sh->DetachFromMaterial(shader_.GetObject());
    }
    auto shader = shaderJS.GetNative<SCENE_NS::IShader>();
    if (!shader && shaderJS) {
        // handle setting a non-raw shader.. (ie. from a material instance, current inputs will be lost though)..
        if (const auto native = shaderJS.GetNative<META_NS::IMetadata>()) {
            if (auto pp = native->GetProperty<IntfPtr>("shader", META_NS::MetadataQuery::EXISTING)) {
                shader = interface_pointer_cast<SCENE_NS::IShader>(META_NS::GetValue(pp));
            }
        }
    }
    materialType_ = shader ? MaterialType::SHADER : MaterialType::METALLIC_ROUGHNESS;
    META_NS::SetValue(
        material->Type(), shader ? SCENE_NS::MaterialType::CUSTOM : SCENE_NS::MaterialType::METALLIC_ROUGHNESS);
    // bind it to material (in native)
    material->MaterialShader()->SetValue(shader);
    ShaderChanged(shader);
    if (!shader) {
        shaderBind_.reset();
        shader_.Reset();
        return;
    }
    // construct a "bound" shader object from the "non bound" one.
    NapiApi::Env env(ctx.Env());
    NapiApi::Object parms(env);
    napi_value args[] = {
        scene_.GetValue(),  // bind the new instance of the shader to this javascript scene object
        parms.ToNapiValue() // other constructor parameters
    };
    parms.Set("name", shaderJS.Get("name"));
    parms.Set("uri", shaderJS.Get("uri"));
    parms.Set("Material", ctx.This()); // js material object that we are bound to.
    shaderBind_ = META_NS::GetObjectRegistry().Create(META_NS::ClassId::Object);
    interface_cast<META_NS::IMetadata>(shaderBind_)
        ->AddProperty(META_NS::ConstructProperty<IntfPtr>(
            "shader", nullptr, META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE));
    interface_cast<META_NS::IMetadata>(shaderBind_)
        ->GetProperty<IntfPtr>("shader")
        ->SetValue(interface_pointer_cast<CORE_NS::IInterface>(shader));
    auto result = CreateFromNativeInstance(env, "Shader", shaderBind_, PtrType::WEAK, args);
    shader_ = NapiApi::StrongRef(result);
}
napi_value MaterialJS::GetColorShader(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    if (disposed_ || !validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetNativeObject());
    if (!material) {
        // material destroyed, just make sure we have no shader reference anymore.
        shaderBind_.reset();
        if (auto sh = shader_.GetObject().GetJsWrapper<ShaderJS>()) {
            sh->DetachFromMaterial(shader_.GetObject());
        }
        shader_.Reset();
        return ctx.GetUndefined();
    }

    if (!shader_.GetObject()) {
        // check native side..
        SCENE_NS::IShader::Ptr shader = material->MaterialShader()->GetValue();
        if (!shader) {
            // no shader in native also.
            return ctx.GetUndefined();
        }

        // construct a "bound" shader object from the "non bound" one.
        NapiApi::Env env(ctx.Env());
        NapiApi::Object parms(env);
        napi_value args[] = {
            scene_.GetValue(),  // bind the new instance of the shader to this javascript scene object
            parms.ToNapiValue() // other constructor parameters
        };

        if (!scene_.GetObject<SCENE_NS::IScene>()) {
            LOG_F("INVALID SCENE!");
        }
        parms.Set("Material", ctx.This()); // js material object that we are bound to.

        shaderBind_ = META_NS::GetObjectRegistry().Create(META_NS::ClassId::Object);
        interface_cast<META_NS::IMetadata>(shaderBind_)
            ->AddProperty(META_NS::ConstructProperty<IntfPtr>(
                "shader", nullptr, META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE));
        interface_cast<META_NS::IMetadata>(shaderBind_)
            ->GetProperty<IntfPtr>("shader")
            ->SetValue(interface_pointer_cast<CORE_NS::IInterface>(shader));

        auto result = CreateFromNativeInstance(env, "Shader", shaderBind_, PtrType::WEAK, args);
        shader_ = NapiApi::StrongRef(result);
    }
    return shader_.GetObject().ToNapiValue();
}

template<size_t Index>
napi_value MaterialJS::GetMaterialProperty(NapiApi::FunctionContext<>& ctx)
{
    LUME_TRACE_FUNC()
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    auto material = ctx.This().GetNative<SCENE_NS::IMaterial>();
    if (!material || !material->Textures()) {
        return ctx.GetUndefined();
    }
    SCENE_NS::ITexture::Ptr texture = material->Textures()->GetValueAt(Index);
    if (!texture) {
        return ctx.GetUndefined();
    }
    napi_value args[] = { ctx.This().ToNapiValue() };
    return CreateFromNativeInstance(ctx.GetEnv(), texture, PtrType::STRONG, args).ToNapiValue();
}
template<size_t Index>
void MaterialJS::SetMaterialProperty(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    LUME_TRACE_FUNC()
    if (!validateSceneRef()) {
        return;
    }
    auto material = ctx.This().GetNative<SCENE_NS::IMaterial>();
    if (!material || !material->Textures()) {
        return;
    }
    SCENE_NS::ITexture::Ptr texture = material->Textures()->GetValueAt(Index);
    if (!texture) {
        return;
    }

    if (NapiApi::Object obj = ctx.Arg<0>()) {
        MaterialPropertyJS::SetImage(texture, obj.Get<NapiApi::Object>("image"));
        MaterialPropertyJS::SetFactor(texture, obj.Get<NapiApi::Object>("factor"));
        MaterialPropertyJS::SetSampler(texture, obj.Get<NapiApi::Object>("sampler"));
    }
}
