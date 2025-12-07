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
#include "MeshJS.h"

#include "JsObjectCache.h"
#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_scene.h>

#include "SceneJS.h"
void* MeshJS::GetInstanceImpl(uint32_t id)
{
    if (id == MeshJS::ID) {
        return static_cast<MeshJS*>(this);
    }
    if (auto ret = SceneResourceImpl::GetInstanceImpl(id)) {
        return ret;
    }
    return BaseObject::GetInstanceImpl(id);
}
void MeshJS::DisposeNative(void* scn)
{
    if (disposed_) {
        return;
    }
    disposed_ = true;
    DisposeBridge(this);
    if (auto mesh = GetNativeObject<META_NS::IObject>()) {
        DetachJsObj(mesh,"_JSWMesh");
    }
    LOG_V("MeshJS::DisposeNative");
    if (auto* sceneJS = static_cast<SceneJS*>(scn)) {
        sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
    }

    scene_.Reset();
}
void MeshJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> node_props;
    SceneResourceImpl::GetPropertyDescs(node_props);

    using namespace NapiApi;
    node_props.push_back(GetProperty<Object, MeshJS, &MeshJS::GetSubmesh>("subMeshes"));
    node_props.push_back(GetProperty<Object, MeshJS, &MeshJS::GetAABB>("aabb"));
    node_props.push_back(
        GetSetProperty<Object, MeshJS, &MeshJS::GetMaterialOverride, &MeshJS::SetMaterialOverride>("materialOverride"));

    napi_value func;
    auto status = napi_define_class(env, "Mesh", NAPI_AUTO_LENGTH, BaseObject::ctor<MeshJS>(), nullptr,
        node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    if (mis) {
        mis->StoreCtor("Mesh", func);
    }
}

MeshJS::MeshJS(napi_env e, napi_callback_info i) : BaseObject(e, i), SceneResourceImpl(SceneResourceImpl::MESH)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        // okay internal create. we will receive the object after.
        return;
    }
    NapiApi::Object scene = fromJs.Arg<0>().valueOrDefault();
    scene_ = scene;
    if (!scene_.GetObject<SCENE_NS::IScene>()) {
        LOG_F("INVALID SCENE!");
    }
    // add the dispose hook to scene. (so that the geometry node is disposed when scene is disposed)
    NapiApi::Object meJs(fromJs.This());
    AddBridge("MeshJS",meJs);
    if (const auto sceneJS = scene.GetJsWrapper<SceneJS>()) {
        sceneJS->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
    }
}
MeshJS::~MeshJS()
{
    LOG_V("MeshJS -- ");
    DestroyBridge(this);
}
void MeshJS::Finalize(napi_env env)
{
    DisposeNative(scene_.GetJsWrapper<SceneJS>());
    BaseObject::Finalize(env);
    FinalizeBridge(this);
}

napi_value MeshJS::GetSubmesh(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    auto node = ctx.This().GetNative<SCENE_NS::IMesh>();
    if (!node) {
        // return undefined.. as no actual node.
        return ctx.GetUndefined();
    }

    auto subs = node->SubMeshes()->GetValue();

    NapiApi::Env env(ctx.Env());
    napi_value tmp;
    auto status = napi_create_array_with_length(env, subs.size(), &tmp);
    if (status != napi_ok) {
        LOG_E("create array failed in %s", __func__);
    }
    uint32_t i = 0;
    for (const auto& subMesh : subs) {
        napi_value args[] = { scene_.GetValue(), ctx.This().ToNapiValue(), env.GetNumber(i) };
        auto val = CreateFromNativeInstance(ctx.Env(), subMesh, PtrType::WEAK, args);
        status = napi_set_element(ctx.Env(), tmp, i++, val.ToNapiValue());
    }

    return tmp;
}

napi_value MeshJS::GetAABB(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    auto node = ctx.This().GetNative<SCENE_NS::IMesh>();
    if (!node) {
        // return undefined.. as no actual node.
        return ctx.GetUndefined();
    }
    BASE_NS::Math::Vec3 aabmin;
    BASE_NS::Math::Vec3 aabmax;
    aabmin = node->AABBMin()->GetValue();
    aabmax = node->AABBMax()->GetValue();
    NapiApi::Env env(ctx.Env());
    NapiApi::Object res(env);

    NapiApi::Object min(env);
    min.Set("x", NapiApi::Value<float> { env, aabmin.x });
    min.Set("y", NapiApi::Value<float> { env, aabmin.y });
    min.Set("z", NapiApi::Value<float> { env, aabmin.z });
    res.Set("aabbMin", min);

    NapiApi::Object max(env);
    max.Set("x", NapiApi::Value<float> { env, aabmax.x });
    max.Set("y", NapiApi::Value<float> { env, aabmax.y });
    max.Set("z", NapiApi::Value<float> { env, aabmax.z });
    res.Set("aabbMax", max);
    return res.ToNapiValue();
}

napi_value MeshJS::GetMaterialOverride(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    return ctx.GetUndefined();
}

void MeshJS::SetMaterialOverride(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    auto sm = ctx.This().GetNative<SCENE_NS::IMesh>();
    if (!sm) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    auto newMaterial = obj.GetNative<SCENE_NS::IMaterial>();
    for (auto&& sm : sm->SubMeshes()->GetValue()) {
        if (auto *smmeta = interface_cast<META_NS::IMetadata>(sm)) {
            if (newMaterial) {
                if (!smmeta->GetProperty("OriginalMaterial", META_NS::MetadataQuery::EXISTING)) {
                    // if we do not have "OriginalMaterial" (material set before first override)
                    // and store the current material as "OriginalMaterial".
                    auto curMat = META_NS::GetValue(sm->Material());
                    auto prop = META_NS::ConstructProperty<SCENE_NS::IMaterial::Ptr>("OriginalMaterial",
                        nullptr,
                        META_NS::ObjectFlagBits::INTERNAL | META_NS::ObjectFlagBits::NATIVE);
                    smmeta->AddProperty(prop);
                    META_NS::SetValue(prop, curMat);
                }
                // change the material.
                META_NS::SetValue(sm->Material(), newMaterial);
            } else {
                // do we have the original material.
                if (auto om = smmeta->GetProperty("OriginalMaterial", META_NS::MetadataQuery::EXISTING)) {
                    // okay, set it back then.
                    auto origMat = META_NS::GetValue<SCENE_NS::IMaterial::Ptr>(om);
                    META_NS::SetValue<SCENE_NS::IMaterial::Ptr>(sm->Material(), origMat);
                    // remove the temp storage property.
                    smmeta->RemoveProperty(om);
                }
            }
        }
    }
}
