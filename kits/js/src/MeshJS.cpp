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
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_mesh.h>

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
    GetInstanceData(env, (void**)&mis);
    mis->StoreCtor("Mesh", func);
}

MeshJS::MeshJS(napi_env e, napi_callback_info i) : BaseObject<MeshJS>(e, i), SceneResourceImpl(SceneResourceImpl::MESH)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        // okay internal create. we will receive the object after.
        return;
    }
    scene_ = fromJs.Arg<0>().valueOrDefault();
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        LOG_F("INVALID SCENE!");
    }
    {
        // add the dispose hook to scene. (so that the geometry node is disposed when scene is disposed)
        NapiApi::Object meJs(fromJs.This());
        NapiApi::Object scene = fromJs.Arg<0>();
        if (auto sceneJS = GetJsWrapper<SceneJS>(scene)) {
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
    DisposeNative(GetJsWrapper<SceneJS>(scene_.GetObject()));
    BaseObject::Finalize(env);
}
bool MeshJS::UpdateSubmesh(uint32_t index, SCENE_NS::ISubMesh::Ptr newSubmesh)
{
    if (index < subs_.size()) {
        subs_[index] = newSubmesh;
        if (auto mesh = interface_pointer_cast<SCENE_NS::IMesh>(GetNativeObject())) {
            return mesh->SetSubmeshes(subs_).GetResult();
        }
    }
    return false;
}

napi_value MeshJS::GetSubmesh(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    auto node = interface_pointer_cast<SCENE_NS::IMesh>(GetThisNativeObject(ctx));
    if (!node) {
        // return undefined.. as no actual node.
        return ctx.GetUndefined();
    }

    subs_ = node->GetSubmeshes().GetResult();

    NapiApi::Env env(ctx.Env());
    napi_value tmp;
    auto status = napi_create_array_with_length(env, subs_.size(), &tmp);
    uint32_t i = 0;
    for (const auto& subMesh : subs_) {
        napi_value args[] = { scene_.GetValue(), ctx.This().ToNapiValue(), env.GetNumber(i) };

        auto subobj = interface_pointer_cast<META_NS::IObject>(subMesh);
        auto val = CreateFromNativeInstance(ctx.Env(), subobj, true, BASE_NS::countof(args), args);
        status = napi_set_element(ctx.Env(), tmp, i++, val.ToNapiValue());
    }

    return tmp;
}

napi_value MeshJS::GetAABB(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    auto node = interface_pointer_cast<SCENE_NS::IMesh>(GetThisNativeObject(ctx));
    if (!node) {
        // return undefined.. as no actual node.
        return ctx.GetUndefined();
    }
    BASE_NS::Math::Vec3 aabmin, aabmax;
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
    auto sm = interface_pointer_cast<SCENE_NS::IMesh>(GetThisNativeObject(ctx));
    if (!sm) {
        // return undefined.. as submesh bound.
        return ctx.GetUndefined();
    }
    META_NS::IObject::Ptr obj;
    auto material = sm->MaterialOverride()->GetValue();
    obj = interface_pointer_cast<META_NS::IObject>(material);

    if (obj == nullptr) {
        return ctx.GetUndefined();
    }
    if (auto cached = FetchJsObj(obj)) {
        // always return the same js object.
        return cached.ToNapiValue();
    }
    napi_value args[] = { ctx.This().ToNapiValue() };
    return CreateFromNativeInstance(
        ctx.Env(), obj, false /*these are owned by the scene*/, BASE_NS::countof(args), args)
        .ToNapiValue();
}

void MeshJS::SetMaterialOverride(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    auto sm = interface_pointer_cast<SCENE_NS::IMesh>(GetThisNativeObject(ctx));
    if (!sm) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    auto new_material = GetNativeMeta<SCENE_NS::IMaterial>(obj);
    sm->MaterialOverride()->SetValue(new_material);
}
