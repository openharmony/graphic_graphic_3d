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

#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_scene.h>

#include "SceneJS.h"
void* MeshJS::GetInstanceImpl(uint32_t id)
{
    if (id == MeshJS::ID)
        return this;
    return SceneResourceImpl::GetInstanceImpl(id);
}
void MeshJS::DisposeNative(void* scn)
{
    if (disposed_) {
        return;
    }
    disposed_ = true;
    LOG_V("MeshJS::DisposeNative");
    if (auto* sceneJS = static_cast<SceneJS*>(scn)) {
        sceneJS->ReleaseStrongDispose(reinterpret_cast<uintptr_t>(&scene_));
    }

    subs_.clear();
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
    mis->StoreCtor("Mesh", func);
}

MeshJS::MeshJS(napi_env e, napi_callback_info i) : BaseObject(e, i), SceneResourceImpl(SceneResourceImpl::MESH)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        // okay internal create. we will receive the object after.
        return;
    }
    scene_ = fromJs.Arg<0>().valueOrDefault();
    if (!scene_.GetObject().GetNative<SCENE_NS::IScene>()) {
        LOG_F("INVALID SCENE!");
    }
    {
        // add the dispose hook to scene. (so that the geometry node is disposed when scene is disposed)
        NapiApi::Object meJs(fromJs.This());
        NapiApi::Object scene = fromJs.Arg<0>();
        if (const auto sceneJS = scene.GetJsWrapper<SceneJS>()) {
            sceneJS->StrongDisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
        }
    }
}
MeshJS::~MeshJS()
{
    LOG_V("MeshJS -- ");
}
void MeshJS::Finalize(napi_env env)
{
    DisposeNative(scene_.GetObject().GetJsWrapper<SceneJS>());
    BaseObject::Finalize(env);
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

    subs_ = node->SubMeshes()->GetValue();

    NapiApi::Env env(ctx.Env());
    napi_value tmp;
    auto status = napi_create_array_with_length(env, subs_.size(), &tmp);
    uint32_t i = 0;
    for (const auto& subMesh : subs_) {
        napi_value args[] = { scene_.GetValue(), ctx.This().ToNapiValue(), env.GetNumber(i) };
        auto val = CreateFromNativeInstance(ctx.Env(), subMesh, PtrType::STRONG, args);
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
    /* auto sm = ctx.This().GetNative<SCENE_NS::IMesh>();
    if (!sm) {
        // return undefined.. as submesh bound.
        return ctx.GetUndefined();
    }
    if (const auto material = sm->MaterialOverride()->GetValue()) {
        napi_value args[] = { ctx.This().ToNapiValue() };
        // These are owned by the scene, so store only weak reference.
        return CreateFromNativeInstance(ctx.Env(), material, PtrType::WEAK, args).ToNapiValue();
    }*/
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
    auto new_material = obj.GetNative<SCENE_NS::IMaterial>();
    SCENE_NS::SetMaterialForAllSubMeshes(sm, new_material);
}
