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
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/construct_property.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_material.h>

#include "SceneJS.h"
#include "MaterialPropertyJS.h"
using IntfPtr = META_NS::SharedPtrIInterface;
using IntfWeakPtr = META_NS::WeakPtrIInterface;

BaseMaterial::BaseMaterial(MaterialType lt) : SceneResourceImpl(SceneResourceImpl::MATERIAL), materialType_(lt) {}
BaseMaterial::~BaseMaterial() {}
void BaseMaterial::Init(const char* class_name, napi_env env, napi_value exports, napi_callback ctor,
    BASE_NS::vector<napi_property_descriptor>& node_props)
{
    SceneResourceImpl::GetPropertyDescs(node_props);

    using namespace NapiApi;
    node_props.emplace_back(TROGetProperty<uint32_t, BaseMaterial, &BaseMaterial::GetMaterialType>("materialType"));
    node_props.emplace_back(
        TROGetSetProperty<bool, BaseMaterial, &BaseMaterial::GetShadowReceiver, &BaseMaterial::SetShadowReceiver>(
            "shadowReceiver"));
    node_props.emplace_back(
        TROGetSetProperty<uint32_t, BaseMaterial, &BaseMaterial::GetCullMode, &BaseMaterial::SetCullMode>("cullMode"));
    node_props.emplace_back(
        TROGetSetProperty<NapiApi::Object, BaseMaterial, &BaseMaterial::GetBlend, &BaseMaterial::SetBlend>("blend"));
    node_props.emplace_back(
        TROGetSetProperty<float, BaseMaterial, &BaseMaterial::GetAlphaCutoff, &BaseMaterial::SetAlphaCutoff>(
            "alphaCutoff"));
    node_props.emplace_back(
        TROGetSetProperty<NapiApi::Object, BaseMaterial, &BaseMaterial::GetRenderSort, &BaseMaterial::SetRenderSort>(
            "renderSort"));

    napi_value func;
    auto status = napi_define_class(
        env, class_name, NAPI_AUTO_LENGTH, ctor, nullptr, node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    if (mis) {
        mis->StoreCtor(class_name, func);
    }

    NapiApi::Object exp1(env, exports);
    napi_value eType1 = nullptr;
    napi_value v1 = nullptr;
    napi_create_object(env, &eType1);
#define DECL_ENUM(enu, x)                         \
    napi_create_uint32(env, MaterialType::x, &v1); \
    napi_set_named_property(env, enu, #x, v1);

    DECL_ENUM(eType1, SHADER);
    DECL_ENUM(eType1, METALLIC_ROUGHNESS);
#undef DECL_ENUM
    exp1.Set("MaterialType", eType1);

    NapiApi::Object exp2(env, exports);
    napi_value eType2 = nullptr;
    napi_value v2 = nullptr;
    napi_create_object(env, &eType2);
#define DECL_ENUM(enu, x)                         \
    napi_create_uint32(env, CullMode::x, &v2); \
    napi_set_named_property(env, enu, #x, v2);

    DECL_ENUM(eType2, NONE);
    DECL_ENUM(eType2, FRONT);
    DECL_ENUM(eType2, BACK);
#undef DECL_ENUM
    exp2.Set("CullMode", eType2);
}

void* BaseMaterial::GetInstanceImpl(uint32_t id)
{
    if (id == BaseMaterial::ID)
        return (BaseMaterial*)this;
    return SceneResourceImpl::GetInstanceImpl(id);
}
void BaseMaterial::DisposeNative(BaseObject* tro)
{
    // do nothing for now..
    LOG_V("BaseMaterial::DisposeNative");
    tro->UnsetNativeObject();
    scene_.Reset();
}
napi_value BaseMaterial::GetMaterialType(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    uint32_t type = -1; // return -1 if the object does not exist anymore
    if (auto material = ctx.This().GetNative<SCENE_NS::IMaterial>()) {
        type = META_NS::GetValue(material->Type()) == SCENE_NS::MaterialType::METALLIC_ROUGHNESS
                   ? BaseMaterial::METALLIC_ROUGHNESS
                   : BaseMaterial::SHADER;
    }
    return ctx.GetNumber(type);
}
napi_value BaseMaterial::GetShadowReceiver(NapiApi::FunctionContext<>& ctx)
{
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
void BaseMaterial::SetShadowReceiver(NapiApi::FunctionContext<bool>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    bool shadowReceiver = ctx.Arg<0>();
    if (auto material = ctx.This().GetNative<SCENE_NS::IMaterial>()) {
        uint32_t lightingFlags = uint32_t(META_NS::GetValue(material->LightingFlags()));
        if (shadowReceiver) {
            lightingFlags |= uint32_t(SCENE_NS::LightingFlags::SHADOW_RECEIVER_BIT);
        } else {
            lightingFlags &= ~uint32_t(SCENE_NS::LightingFlags::SHADOW_RECEIVER_BIT);
        }
        META_NS::SetValue(material->LightingFlags(), static_cast<SCENE_NS::LightingFlags>(lightingFlags));
    }
}
napi_value BaseMaterial::GetCullMode(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    auto cullMode = SCENE_NS::CullModeFlags::NONE;
    if (auto material = ctx.This().GetNative<SCENE_NS::IMaterial>()) {
        if (auto shader = META_NS::GetValue(material->MaterialShader())) {
            if (auto scene = scene_.GetObject().GetNative<SCENE_NS::IScene>()) {
                cullMode = static_cast<SCENE_NS::CullModeFlags>(shader->CullMode()->GetValue());
            }
        }
    }
    return ctx.GetNumber(static_cast<uint32_t>(cullMode));
}
void BaseMaterial::SetCullMode(NapiApi::FunctionContext<uint32_t>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    auto cullMode = static_cast<SCENE_NS::CullModeFlags>((uint32_t)ctx.Arg<0>());

    if (auto material = ctx.This().GetNative<SCENE_NS::IMaterial>()) {
        if (auto shader = META_NS::GetValue(material->MaterialShader())) {
            shader->CullMode()->SetValue(cullMode);
            // need to forcefully refresh the material, otherwise renderer will ignore the change
            auto val = META_NS::GetValue(material->MaterialShader());
            META_NS::SetValue(material->MaterialShader(), val);
        }
    }
}
napi_value BaseMaterial::GetBlend(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto material = ctx.This().GetNative<SCENE_NS::IMaterial>();
    if (!material) {
        return ctx.GetUndefined();
    }

    auto shader = META_NS::GetValue(material->MaterialShader());
    if (!shader) {
        return ctx.GetUndefined();
    }

    bool enableBlend = shader->Blend()->GetValue();

    NapiApi::Object blendObjectJS(ctx.Env());
    blendObjectJS.Set("enabled", ctx.GetBoolean(enableBlend));
    return blendObjectJS.ToNapiValue();
}
void BaseMaterial::SetBlend(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }

    auto material = ctx.This().GetNative<SCENE_NS::IMaterial>();
    if (!material) {
        // material destroyed, just make sure we have no shader reference anymore.
        return;
    }

    auto shader = META_NS::GetValue(material->MaterialShader());
    if (!shader) {
        return;
    }

    NapiApi::Object blendObjectJS = ctx.Arg<0>();
    bool enableBlend = blendObjectJS.Get<bool>("enabled");
    shader->Blend()->SetValue(enableBlend);

    // need to forcefully refresh the material, otherwise renderer will ignore the change
    auto val = META_NS::GetValue(material->MaterialShader());
    META_NS::SetValue(material->MaterialShader(), val);
}
napi_value BaseMaterial::GetAlphaCutoff(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    float alphaCutoff = 1.0f;
    if (auto material = ctx.This().GetNative<SCENE_NS::IMaterial>()) {
        alphaCutoff = META_NS::GetValue(material->AlphaCutoff());
    }
    return ctx.GetNumber(alphaCutoff);
}
void BaseMaterial::SetAlphaCutoff(NapiApi::FunctionContext<float>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    float alphaCutoff = ctx.Arg<0>();

    if (auto material = ctx.This().GetNative<SCENE_NS::IMaterial>()) {
        META_NS::SetValue(material->AlphaCutoff(), alphaCutoff);
    }
}
napi_value BaseMaterial::GetRenderSort(NapiApi::FunctionContext<>& ctx)
{
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
    if (!validateSceneRef()) {
        return;
    }

    auto material = ctx.This().GetNative<SCENE_NS::IMaterial>();
    if (!material) {
        // material destroyed, just make sure we have no shader reference anymore.
        return;
    }

    SCENE_NS::RenderSort renderSort;
    NapiApi::Object renderSortJS = ctx.Arg<0>();
    renderSort.renderSortLayer = renderSortJS.Get<uint32_t>("renderSortLayer");
    renderSort.renderSortLayerOrder = renderSortJS.Get<uint32_t>("renderSortLayerOrder");
    META_NS::SetValue(material->RenderSort(), renderSort);
}

//------

void MaterialJS::Init(napi_env env, napi_value exports)
{
#define MRADDPROP(index, name)                           \
    NapiApi::GetSetProperty<NapiApi::Object, MaterialJS, \
            &MaterialJS::GetMaterialProperty<index>, \
        &MaterialJS::SetMaterialProperty<index>>(name)

    BASE_NS::vector<napi_property_descriptor> props = {
        NapiApi::GetSetProperty<NapiApi::Object, MaterialJS, &MaterialJS::GetColorShader,
            &MaterialJS::SetColorShader>("colorShader"),
        MRADDPROP(0, "baseColor"),
        MRADDPROP(1, "normal"),
        MRADDPROP(2, "material"),
        MRADDPROP(3, "emissive"),
        MRADDPROP(4, "ambientOcclusion"),
        MRADDPROP(5, "clearCoat"),
        MRADDPROP(6, "clearCoatRoughness"),
        MRADDPROP(7, "clearCoatNormal"),
        MRADDPROP(8, "sheen"),
        MRADDPROP(10, "specular")
    };
    #undef MRADDPROP

    BaseMaterial::Init("Material", env, exports, BaseObject::ctor<MaterialJS>(), props);
}

MaterialJS::MaterialJS(napi_env e, napi_callback_info i)
    : BaseObject(e, i), BaseMaterial(BaseMaterial::MaterialType::METALLIC_ROUGHNESS)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    NapiApi::Object meJs(fromJs.This());

    NapiApi::Object scene = fromJs.Arg<0>(); // access to owning scene... (do i need it here?)
    NapiApi::Object args = fromJs.Arg<1>();  // other args

    scene_ = scene;
    if (!scene_.GetObject().GetNative<SCENE_NS::IScene>()) {
        LOG_F("INVALID SCENE!");
    }

    if (const auto sceneJS = scene.GetJsWrapper<SceneJS>()) {
        sceneJS->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
    }

    const auto material = GetNativeObject<SCENE_NS::IMaterial>();
    if (material) {
        materialType_ = META_NS::GetValue(material->Type()) == SCENE_NS::MaterialType::CUSTOM
                            ? MaterialType::SHADER
                            : MaterialType::METALLIC_ROUGHNESS;
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

MaterialJS::~MaterialJS() {}
void* MaterialJS::GetInstanceImpl(uint32_t id)
{
    if (id == MaterialJS::ID)
        return this;
    return BaseMaterial::GetInstanceImpl(id);
}
void MaterialJS::DisposeNative(void* scn)
{
    if (disposed_) {
        return;
    }
    disposed_ = true;
    if (auto* sceneJS = static_cast<SceneJS*>(scn)) {
        sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
    }

    UnsetNativeObject();
    shaderBind_.reset();
    shader_.Reset();

    BaseMaterial::DisposeNative(this);
}
void MaterialJS::Finalize(napi_env env)
{
    DisposeNative(scene_.GetObject().GetJsWrapper<SceneJS>());
    BaseObject::Finalize(env);
}
void MaterialJS::SetColorShader(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        // owning scene has been destroyed.
        // the material etc should also be gone.
        // but there is a possible issue where the native object is still alive.

        // most likely could happen if scene.dispose is not called
        // but all references to the scene have been released,
        // and garbage collection may or may not have been done yet. (or is partially done)
        // if the scene is garbage collected then all the resources should be disposed.
        return;
    }

    NapiApi::Object shaderJS = ctx.Arg<0>();
    auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetNativeObject());
    if (!material) {
        // material destroyed, just make sure we have no shader reference anymore.
        shader_.Reset();
        return;
    }
    auto shader = shaderJS.GetNative<SCENE_NS::IShader>();
    materialType_ = shader ? MaterialType::SHADER : MaterialType::METALLIC_ROUGHNESS;
    META_NS::SetValue(material->Type(),
                      shader ? SCENE_NS::MaterialType::CUSTOM : SCENE_NS::MaterialType::METALLIC_ROUGHNESS);

    // bind it to material (in native)
    material->MaterialShader()->SetValue(shader);

    if (!shader) {
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
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetNativeObject());
    if (!material) {
        // material destroyed, just make sure we have no shader reference anymore.
        shader_.Reset();
        return ctx.GetNull();
    }

    if (shader_.IsEmpty()) {
        // no shader set yet..
        // see if we have one on the native side.
        // and create the "bound shader" object from it.

        // check native side..
        SCENE_NS::IShader::Ptr shader = material->MaterialShader()->GetValue();
        if (!shader) {
            // no shader in native also.
            return ctx.GetNull();
        }

        // construct a "bound" shader object from the "non bound" one.
        NapiApi::Env env(ctx.Env());
        NapiApi::Object parms(env);
        napi_value args[] = {
            scene_.GetValue(),  // bind the new instance of the shader to this javascript scene object
            parms.ToNapiValue() // other constructor parameters
        };

        if (!scene_.GetObject().GetNative<SCENE_NS::IScene>()) {
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
    return shader_.GetValue();
}

template<size_t Index>
napi_value MaterialJS::GetMaterialProperty(NapiApi::FunctionContext<>& ctx)
{
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
