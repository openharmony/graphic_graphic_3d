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
        TROGetSetProperty<uint32_t, BaseMaterial, &BaseMaterial::GetCullMode, &BaseMaterial::SetCullMode>(
            "cullMode"));
    node_props.emplace_back(
        TROGetSetProperty<NapiApi::Object, BaseMaterial, &BaseMaterial::GetBlend, &BaseMaterial::SetBlend>(
            "blend"));
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
    GetInstanceData(env, (void**)&mis);
    mis->StoreCtor(class_name, func);

    NapiApi::Object exp1(env, exports);
    napi_value eType1, v1;
    napi_create_object(env, &eType1);
#define DECL_ENUM(enu, x)                         \
    napi_create_uint32(env, MaterialType::x, &v1); \
    napi_set_named_property(env, enu, #x, v1);

    DECL_ENUM(eType1, SHADER);
    DECL_ENUM(eType1, METALLIC_ROUGHNESS);
#undef DECL_ENUM
    exp1.Set("MaterialType", eType1);

    NapiApi::Object exp2(env, exports);
    napi_value eType2, v2;
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
void BaseMaterial::DisposeNative(TrueRootObject* tro)
{
    // do nothing for now..
    LOG_V("BaseMaterial::DisposeNative");
    if (auto material = interface_pointer_cast<SCENE_NS::IMaterial>(tro->GetNativeObject())) {
        // reset the native object refs
        tro->SetNativeObject(nullptr, false);
        tro->SetNativeObject(nullptr, true);
    }
    renderSort_.Reset();
    blend_.Reset();
    scene_.Reset();
}
napi_value BaseMaterial::GetMaterialType(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    uint32_t type = -1; // return -1 if the object does not exist anymore
    if (auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetThisNativeObject(ctx))) {
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
    if (auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetThisNativeObject(ctx))) {
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
    if (auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetThisNativeObject(ctx))) {
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
    if (auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetThisNativeObject(ctx))) {
        if (auto shader = META_NS::GetValue(material->MaterialShader())) {
            if (auto scene = GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
                cullMode = static_cast<SCENE_NS::CullModeFlags>(shader->GetCullMode().GetResult());
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

    if (auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetThisNativeObject(ctx))) {
        if (auto shader = META_NS::GetValue(material->MaterialShader())) {
            shader->SetCullMode(cullMode);
        }
    }
}
napi_value BaseMaterial::GetBlend(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetThisNativeObject(ctx));
    if (!material) {
        blend_.Reset();
        return ctx.GetUndefined();
    }

    auto shader = META_NS::GetValue(material->MaterialShader());
    if (!shader) {
        blend_.Reset();
        return ctx.GetUndefined();
    }

    bool enableBlend = shader->IsBlendEnabled().GetResult();

    if (blend_.IsEmpty()) {
        return ctx.GetUndefined();
    }

    return blend_.GetValue();
}
void BaseMaterial::SetBlend(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }

    auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetThisNativeObject(ctx));
    if (!material) {
        // material destroyed, just make sure we have no shader reference anymore.
        blend_.Reset();
        return;
    }

    auto shader = META_NS::GetValue(material->MaterialShader());
    if (!shader) {
        blend_.Reset();
        return;
    }

    NapiApi::Object blendObjectJS = ctx.Arg<0>();
    bool enableBlend = blendObjectJS.Get<bool>("enabled");
    shader->EnableBlend(enableBlend);

    blend_ = NapiApi::StrongRef(blendObjectJS);
}
napi_value BaseMaterial::GetAlphaCutoff(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    float alphaCutoff = 1.0f;
    if (auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetThisNativeObject(ctx))) {
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

    if (auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetThisNativeObject(ctx))) {
        META_NS::SetValue(material->AlphaCutoff(), alphaCutoff);
    }
}
napi_value BaseMaterial::GetRenderSort(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetThisNativeObject(ctx));
    if (!material) {
        renderSort_.Reset();
        return ctx.GetUndefined();
    }

    auto renderSort = META_NS::GetValue(material->RenderSort());

    if (renderSort_.IsEmpty()) {
        return ctx.GetUndefined();
    }

    return renderSort_.GetValue();
}
void BaseMaterial::SetRenderSort(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }

    auto material = interface_pointer_cast<SCENE_NS::IMaterial>(GetThisNativeObject(ctx));
    if (!material) {
        // material destroyed, just make sure we have no shader reference anymore.
        renderSort_.Reset();
        return;
    }

    SCENE_NS::RenderSort renderSort;
    NapiApi::Object renderSortJS = ctx.Arg<0>();
    renderSort.renderSortLayer = renderSortJS.Get<uint32_t>("renderSortLayer");
    renderSort.renderSortLayerOrder = renderSortJS.Get<uint32_t>("renderSortLayerOrder");
    META_NS::SetValue(material->RenderSort(), renderSort);

    renderSort_ = NapiApi::StrongRef(renderSortJS);
}
