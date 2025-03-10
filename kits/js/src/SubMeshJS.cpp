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

#include "SubMeshJS.h"

#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_mesh.h>

#include "MeshJS.h"
#include "SceneJS.h"
void* SubMeshJS::GetInstanceImpl(uint32_t id)
{
    if (id == SubMeshJS::ID)
        return this;
    // no id will match
    return nullptr;
}
void SubMeshJS::DisposeNative(void*)
{
    // do nothing for now..
    LOG_V("SubMeshJS::DisposeNative");
    NapiApi::Object obj = scene_.GetObject();
    auto* tro = obj.Native<TrueRootObject>();
    if (tro) {
        SceneJS* sceneJS = static_cast<SceneJS*>(tro->GetInstanceImpl(SceneJS::ID));
        if (sceneJS) {
            sceneJS->ReleaseStrongDispose(reinterpret_cast<uintptr_t>(&scene_));
        }
    }
    aabbMin_.reset();
    aabbMax_.reset();
    scene_.Reset();
}

napi_value SubMeshJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    // Dispose of the native object. (makes the js object invalid)
    if (TrueRootObject* instance = GetThisRootObject(ctx)) {
        // see if we have "scenejs" as ext (prefer one given as argument)
        napi_status stat;
        SceneJS* ptr { nullptr };
        NapiApi::FunctionContext<NapiApi::Object> args(ctx);
        if (args) {
            if (NapiApi::Object obj = args.Arg<0>()) {
                if (napi_value ext = obj.Get("SceneJS")) {
                    stat = napi_get_value_external(ctx.GetEnv(), ext, (void**)&ptr);
                }
            }
        }
        if (!ptr) {
            NapiApi::Object obj = scene_.GetObject();
            if (obj) {
                auto* tro = obj.Native<TrueRootObject>();
                if (tro) {
                    ptr = static_cast<SceneJS*>(tro->GetInstanceImpl(SceneJS::ID));
                }
            }
        }
        SetNativeObject(nullptr, true);
        instance->DisposeNative(ptr);
    }
    return ctx.GetUndefined();
}
void SubMeshJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> node_props;

    using namespace NapiApi;
    node_props.push_back(GetSetProperty<BASE_NS::string, SubMeshJS, &SubMeshJS::GetName, &SubMeshJS::SetName>("name"));
    node_props.push_back(GetProperty<Object, SubMeshJS, &SubMeshJS::GetAABB>("aabb"));
    node_props.push_back(
        GetSetProperty<Object, SubMeshJS, &SubMeshJS::GetMaterial, &SubMeshJS::SetMaterial>("material"));

    node_props.push_back(MakeTROMethod<FunctionContext<>, SubMeshJS, &SubMeshJS::Dispose>("destroy"));

    napi_value func;
    auto status = napi_define_class(env, "SubMesh", NAPI_AUTO_LENGTH, BaseObject::ctor<SubMeshJS>(), nullptr,
        node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    GetInstanceData(env, (void**)&mis);
    mis->StoreCtor("SubMesh", func);
}

SubMeshJS::SubMeshJS(napi_env e, napi_callback_info i) : BaseObject<SubMeshJS>(e, i)
{
    LOG_V("SubMeshJS ++ ");
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object, uint32_t> fromJs(e, i);
    if (!fromJs) {
        // okay internal create. we will receive the object after.
        return;
    }
    scene_ = fromJs.Arg<0>().valueOrDefault();
    parentMesh_ = fromJs.Arg<1>().valueOrDefault();
    indexInParent_ = fromJs.Arg<2>().valueOrDefault(); // 2: idx
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        LOG_F("INVALID SCENE!");
    }
    {
        // add the dispose hook to scene. (so that the geometry node is disposed when scene is disposed)
        NapiApi::Object meJs(fromJs.This());
        NapiApi::Object scene = fromJs.Arg<0>();
        auto* tro = scene.Native<TrueRootObject>();
        if (tro) {
            auto* sceneJS = static_cast<SceneJS*>(tro->GetInstanceImpl(SceneJS::ID));
            if (sceneJS) {
                sceneJS->StrongDisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
            }
        }
    }
}
SubMeshJS::~SubMeshJS()
{
    LOG_V("SubMeshJS -- %p", this);
}

napi_value SubMeshJS::GetAABB(NapiApi::FunctionContext<>& ctx)
{
    auto node = interface_pointer_cast<SCENE_NS::ISubMesh>(GetThisNativeObject(ctx));
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

napi_value SubMeshJS::GetName(NapiApi::FunctionContext<>& ctx)
{
    BASE_NS::string name;
    if (auto node = interface_pointer_cast<META_NS::INamed>(GetThisNativeObject(ctx))) {
        name = node->Name()->GetValue();
    }
    return ctx.GetString(name);
}
void SubMeshJS::SetName(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    if (auto node = interface_pointer_cast<META_NS::INamed>(GetThisNativeObject(ctx))) {
        BASE_NS::string name = ctx.Arg<0>();
        node->Name()->SetValue(name);
    }
}

napi_value SubMeshJS::GetMaterial(NapiApi::FunctionContext<>& ctx)
{
    auto sm = interface_pointer_cast<SCENE_NS::ISubMesh>(GetThisNativeObject(ctx));
    if (!sm) {
        // return undefined..
        return ctx.GetUndefined();
    }
    META_NS::IObject::Ptr obj;
    auto material = sm->Material()->GetValue();
    obj = interface_pointer_cast<META_NS::IObject>(material);
    if (auto cached = FetchJsObj(obj)) {
        // always return the same js object.
        return cached.ToNapiValue();
    }

    // No jswrapper for this material , so create it.
    NapiApi::Env env(ctx.Env());
    NapiApi::Object argJS(env);
    napi_value args[] = { scene_.GetValue(), argJS.ToNapiValue() };
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        LOG_F("INVALID SCENE!");
    }
    auto argc = BASE_NS::countof(args);
    auto argv = args;
    return CreateFromNativeInstance(env, obj, true, argc, argv).ToNapiValue();
}

void SubMeshJS::SetMaterial(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto sm = interface_pointer_cast<SCENE_NS::ISubMesh>(GetThisNativeObject(ctx));
    if (!sm) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    auto new_material = GetNativeMeta<SCENE_NS::IMaterial>(obj);
    if (new_material) {
        auto cur = sm->Material()->GetValue();
        if (cur != new_material) {
            sm->Material()->SetValue(new_material);
        }
    }
    UpdateParentMesh();
}

void SubMeshJS::UpdateParentMesh()
{
    auto success = false;
    if (auto self = interface_pointer_cast<SCENE_NS::ISubMesh>(GetNativeObject())) {
        if (auto tro = parentMesh_.GetObject().Native<TrueRootObject>()) {
            if (auto mesh = static_cast<MeshJS*>(tro->GetInstanceImpl(MeshJS::ID))) {
                success = mesh->UpdateSubmesh(indexInParent_, self);
            }
        }
    }
    if (!success) {
        LOG_E("Unable to update submesh change to scene");
    }
}
