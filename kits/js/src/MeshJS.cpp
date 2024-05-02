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
#include <scene_plugin/api/camera_uid.h>
#include <scene_plugin/interface/intf_camera.h>
#include <scene_plugin/interface/intf_scene.h>

void* MeshJS::GetInstanceImpl(uint32_t id)
{
    if (id == MeshJS::ID) {
        return this;
    }
    return SceneResourceImpl::GetInstanceImpl(id);
}
void MeshJS::DisposeNative()
{
    // do nothing for now..
    LOG_F("MeshJS::DisposeNative");
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
    napi_get_instance_data(env, (void**)&mis);
    mis->StoreCtor("Mesh", func);
}

MeshJS::MeshJS(napi_env e, napi_callback_info i) : BaseObject<MeshJS>(e, i), SceneResourceImpl(SceneResourceImpl::MESH)
{
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        // okay internal create. we will receive the object after.
        return;
    }
    NapiApi::Object scene = fromJs.Arg<0>();
    scene_ = scene;
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        CORE_LOG_F("INVALID SCENE!");
    }
}
MeshJS::~MeshJS()
{
    LOG_F("MeshJS -- ");
}

napi_value MeshJS::GetSubmesh(NapiApi::FunctionContext<>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::IMesh>(GetThisNativeObject(ctx));
    if (!node) {
        // return undefined.. as no actual node.
        napi_value undef;
        napi_get_undefined(ctx, &undef);
        return undef;
    }

    BASE_NS::vector<SCENE_NS::ISubMesh::Ptr> subs;
    ExecSyncTask([node, &subs]() {
        subs = node->SubMeshes()->GetValue();
        return META_NS::IAny::Ptr {};
    });

    napi_value tmp;
    auto status = napi_create_array_with_length(ctx, subs.size(), &tmp);
    size_t i = 0;
    for (const auto& node : subs) {
        NapiApi::Object argJS(ctx);
        napi_value args[] = { scene_.GetValue(), argJS };

        napi_value val = CreateFromNativeInstance(
            ctx, interface_pointer_cast<META_NS::IObject>(node), false, BASE_NS::countof(args), args);
        status = napi_set_element(ctx, tmp, i++, val);
    }

    return tmp;
}

napi_value MeshJS::GetAABB(NapiApi::FunctionContext<>& ctx)
{
    napi_value undef;
    napi_get_undefined(ctx, &undef);
    auto node = interface_pointer_cast<SCENE_NS::IMesh>(GetThisNativeObject(ctx));
    if (!node) {
        // return undefined.. as no actual node.
        return undef;
    }
    BASE_NS::Math::Vec3 aabmin;
    BASE_NS::Math::Vec3 aabmax;
    ExecSyncTask([node, &aabmin, &aabmax]() {
        aabmin = node->AABBMin()->GetValue();
        aabmax = node->AABBMax()->GetValue();
        return META_NS::IAny::Ptr {};
    });
    NapiApi::Object res(ctx);

    NapiApi::Object min(ctx);
    min.Set("x", NapiApi::Value<float> { ctx, aabmin.x });
    min.Set("y", NapiApi::Value<float> { ctx, aabmin.y });
    min.Set("z", NapiApi::Value<float> { ctx, aabmin.z });
    res.Set("aabbMin", min);

    NapiApi::Object max(ctx);
    max.Set("x", NapiApi::Value<float> { ctx, aabmax.x });
    max.Set("y", NapiApi::Value<float> { ctx, aabmax.y });
    max.Set("z", NapiApi::Value<float> { ctx, aabmax.z });
    res.Set("aabbMax", max);
    return res;
}

napi_value MeshJS::GetMaterialOverride(NapiApi::FunctionContext<>& ctx)
{
    napi_value undef;
    napi_get_undefined(ctx, &undef);
    auto sm = interface_pointer_cast<SCENE_NS::IMesh>(GetThisNativeObject(ctx));
    if (!sm) {
        // return undefined.. as submesh bound.
        return undef;
    }
    META_NS::IObject::Ptr obj;
    ExecSyncTask([sm, &obj]() {
        auto material = sm->MaterialOverride()->GetValue();
        obj = interface_pointer_cast<META_NS::IObject>(material);
        return META_NS::IAny::Ptr {};
    });

    if (obj == nullptr) {
        return ctx.GetUndefined();
    }
    if (auto cached = FetchJsObj(obj)) {
        // always return the same js object.
        return cached;
    }
    napi_value args[] = { ctx.This() };
    return CreateFromNativeInstance(ctx, obj, false /*these are owned by the scene*/, BASE_NS::countof(args), args);
}

void MeshJS::SetMaterialOverride(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto sm = interface_pointer_cast<SCENE_NS::IMesh>(GetThisNativeObject(ctx));
    if (!sm) {
        // return undefined.. as no actual node.
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    auto new_material = GetNativeMeta<SCENE_NS::IMaterial>(obj);
    ExecSyncTask([sm, &new_material]() {
        sm->MaterialOverride()->SetValue(new_material);
        return META_NS::IAny::Ptr {};
    });
}
