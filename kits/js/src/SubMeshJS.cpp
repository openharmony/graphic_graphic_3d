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

#include "JsObjectCache.h"
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
    if (id == SubMeshJS::ID) {
        return static_cast<SubMeshJS*>(this);
    }
    return BaseObject::GetInstanceImpl(id);
}
void SubMeshJS::DisposeNative(void* scn)
{
    if (disposed_) {
        return;
    }
    disposed_ = true;
    DisposeBridge(this);
    if (auto submesh = GetNativeObject<META_NS::IObject>()) {
        DetachJsObj(submesh,"_JSW");
    }

    LOG_V("SubMeshJS::DisposeNative");
    if (auto* sceneJS = static_cast<SceneJS*>(scn)) {
        sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
    }

    aabbMin_.reset();
    aabbMax_.reset();
    parentMesh_.Reset();
    scene_.Reset();
    UnsetNativeObject();
}

void SubMeshJS::Finalize(napi_env env)
{
    DisposeNative(scene_.GetJsWrapper<SceneJS>());
    BaseObject::Finalize(env);
    FinalizeBridge(this);
}

napi_value SubMeshJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    // Dispose of the native object. (makes the js object invalid)
    if (TrueRootObject* instance = ctx.This().GetRoot()) {
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
            ptr = scene_.GetJsWrapper<SceneJS>();
        }
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
    if (status != napi_ok) {
        LOG_E("export class failed in %s", __func__);
    }

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    if (mis) {
        mis->StoreCtor("SubMesh", func);
    }
}

SubMeshJS::SubMeshJS(napi_env e, napi_callback_info i) : BaseObject(e, i)
{
    LOG_V("SubMeshJS ++ ");
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object, uint32_t> fromJs(e, i);
    if (!fromJs) {
        // okay internal create. we will receive the object after.
        return;
    }
    NapiApi::Object scene = fromJs.Arg<0>().valueOrDefault();

    scene_ = scene;
    parentMesh_ = fromJs.Arg<1>().valueOrDefault();
    indexInParent_ = fromJs.Arg<2>().valueOrDefault(); // 2: arg num
    if (!scene.GetNative<SCENE_NS::IScene>()) {
        LOG_F("INVALID SCENE!");
    }
    // add the dispose hook to scene. (so that the geometry node is disposed when scene is disposed)
    NapiApi::Object meJs(fromJs.This());
    AddBridge("SubMeshJS",meJs);
    if (const auto sceneJS = scene.GetJsWrapper<SceneJS>()) {
        sceneJS->DisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
    }
}
SubMeshJS::~SubMeshJS()
{
    LOG_V("SubMeshJS -- %p", this);
    DestroyBridge(this);
}

napi_value SubMeshJS::GetAABB(NapiApi::FunctionContext<>& ctx)
{
    auto node = ctx.This().GetNative<SCENE_NS::ISubMesh>();
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

napi_value SubMeshJS::GetName(NapiApi::FunctionContext<>& ctx)
{
    BASE_NS::string name;
    if (auto node = ctx.This().GetNative<META_NS::INamed>()) {
        name = node->Name()->GetValue();
    }
    return ctx.GetString(name);
}
void SubMeshJS::SetName(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    if (auto node = ctx.This().GetNative<META_NS::INamed>()) {
        BASE_NS::string name = ctx.Arg<0>();
        node->Name()->SetValue(name);
    }
}

napi_value SubMeshJS::GetMaterial(NapiApi::FunctionContext<>& ctx)
{
    auto sm = ctx.This().GetNative<SCENE_NS::ISubMesh>();
    if (!sm) {
        return ctx.GetUndefined();
    }
    META_NS::IObject::Ptr obj;
    auto material = sm->Material()->GetValue();

    NapiApi::Env env(ctx.Env());
    NapiApi::Object argJS(env);
    napi_value args[] = { scene_.GetValue(), argJS.ToNapiValue() };
    if (!scene_.GetObject<SCENE_NS::IScene>()) {
        LOG_F("INVALID SCENE!");
    }
    return CreateFromNativeInstance(env, material, PtrType::WEAK, args).ToNapiValue();
}

void SubMeshJS::SetMaterial(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    auto sm = ctx.This().GetNative<SCENE_NS::ISubMesh>();
    if (!sm) {
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    auto new_material = obj.GetNative<SCENE_NS::IMaterial>();
    if (new_material) {
        auto cur = sm->Material()->GetValue();
        if (cur != new_material) {
            sm->Material()->SetValue(new_material);
        }
    }
}
