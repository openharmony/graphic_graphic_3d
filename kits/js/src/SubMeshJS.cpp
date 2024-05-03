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
#include <scene_plugin/api/camera_uid.h>
#include <scene_plugin/interface/intf_camera.h>
#include <scene_plugin/interface/intf_scene.h>

void* SubMeshJS::GetInstanceImpl(uint32_t id)
{
    if (id == SubMeshJS::ID) {
        return this;
    }
    // not a node.
    return nullptr;
}
void SubMeshJS::DisposeNative()
{
    // do nothing for now..
    LOG_F("SubMeshJS::DisposeNative");
    aabbMin_.reset();
    aabbMax_.reset();
    scene_.Reset();
}
void SubMeshJS::Init(napi_env env, napi_value exports)
{
    BASE_NS::vector<napi_property_descriptor> node_props;

    using namespace NapiApi;
    node_props.push_back(GetSetProperty<BASE_NS::string, SubMeshJS, &SubMeshJS::GetName, &SubMeshJS::SetName>("name"));
    node_props.push_back(GetProperty<Object, SubMeshJS, &SubMeshJS::GetAABB>("aabb"));
    node_props.push_back(
        GetSetProperty<Object, SubMeshJS, &SubMeshJS::GetMaterial, &SubMeshJS::SetMaterial>("material"));

    napi_value func;
    auto status = napi_define_class(env, "SubMesh", NAPI_AUTO_LENGTH, BaseObject::ctor<SubMeshJS>(), nullptr,
        node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    napi_get_instance_data(env, (void**)&mis);
    mis->StoreCtor("SubMesh", func);
}

SubMeshJS::SubMeshJS(napi_env e, napi_callback_info i) : BaseObject<SubMeshJS>(e, i)
{
    LOG_F("SubMeshJS ++ ");
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
SubMeshJS::~SubMeshJS()
{
    LOG_F("SubMeshJS -- ");
}

napi_value SubMeshJS::GetAABB(NapiApi::FunctionContext<>& ctx)
{
    napi_value undef;
    napi_get_undefined(ctx, &undef);
    auto node = interface_pointer_cast<SCENE_NS::ISubMesh>(GetThisNativeObject(ctx));
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

napi_value SubMeshJS::GetName(NapiApi::FunctionContext<>& ctx)
{
    BASE_NS::string name;
    if (auto node = interface_pointer_cast<SCENE_NS::ISubMesh>(GetThisNativeObject(ctx))) {
        ExecSyncTask([node, &name]() {
            name = node->Name()->GetValue();
            return META_NS::IAny::Ptr {};
        });
    }
    napi_value value;
    napi_status status = napi_create_string_utf8(ctx, name.c_str(), name.length(), &value);
    return value;
}
void SubMeshJS::SetName(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    if (auto node = interface_pointer_cast<SCENE_NS::ISubMesh>(GetThisNativeObject(ctx))) {
        BASE_NS::string name = ctx.Arg<0>();
        ExecSyncTask([node, name]() {
            node->Name()->SetValue(name);
            return META_NS::IAny::Ptr {};
        });
    }
}

napi_value SubMeshJS::GetMaterial(NapiApi::FunctionContext<>& ctx)
{
    napi_value undef;
    napi_get_undefined(ctx, &undef);
    auto sm = interface_pointer_cast<SCENE_NS::ISubMesh>(GetThisNativeObject(ctx));
    if (!sm) {
        // return undefined..
        return undef;
    }
    META_NS::IObject::Ptr obj;
    ExecSyncTask([sm, &obj]() {
        auto material = sm->Material()->GetValue();
        obj = interface_pointer_cast<META_NS::IObject>(material);
        return META_NS::IAny::Ptr {};
    });

    if (auto cached = FetchJsObj(obj)) {
        // always return the same js object.
        return cached;
    }

    // No jswrapper for this material , so create it.
    NapiApi::Object argJS(ctx);
    napi_value args[] = { scene_.GetValue(), argJS };
    if (!GetNativeMeta<SCENE_NS::IScene>(scene_.GetObject())) {
        CORE_LOG_F("INVALID SCENE!");
    }
    auto argc = BASE_NS::countof(args);
    auto argv = args;
    return CreateFromNativeInstance(ctx, obj, false /*these are owned by the scene*/, argc, argv);
}

void SubMeshJS::SetMaterial(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    napi_value undef;
    napi_get_undefined(ctx, &undef);
    auto sm = interface_pointer_cast<SCENE_NS::ISubMesh>(GetThisNativeObject(ctx));
    if (!sm) {
        // return undefined.. as no actual node.
        return;
    }
    NapiApi::Object obj = ctx.Arg<0>();
    auto new_material = GetNativeMeta<SCENE_NS::IMaterial>(obj);
    if (new_material) {
        ExecSyncTask([sm, &new_material]() {
            auto cur = sm->Material()->GetValue();
            if (cur != new_material) {
                sm->Material()->SetValue(new_material);
            }
            return META_NS::IAny::Ptr {};
        });
    }

    return;
}
