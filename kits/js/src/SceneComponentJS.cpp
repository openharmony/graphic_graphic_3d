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

#include "SceneComponentJS.h"

#include <meta/api/make_callback.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/property_events.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_scene.h>
#include <scene/ext/intf_component.h>
#include <scene/ext/util.h>

#include <render/intf_render_context.h>

using namespace SCENE_NS;

void SceneComponentJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> props;

    napi_value func;
    auto status = napi_define_class(env, "SceneComponent", NAPI_AUTO_LENGTH, BaseObject::ctor<SceneComponentJS>(),
        nullptr, props.size(), props.data(), &func);

    NapiApi::MyInstanceState* mis;
    napi_get_instance_data(env, (void**)&mis);
    mis->StoreCtor("SceneComponent", func);
}

napi_value SceneComponentJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    LOG_V("SceneComponentJS::Dispose");
    DisposeNative(nullptr);
    return {};
}
void SceneComponentJS::DisposeNative(void*)
{
    if (!disposed_) {
        disposed_ = true;
        LOG_V("SceneComponentJS::DisposeNative");

        if (!jsProps_.IsEmpty()) {
            NapiApi::Object inp = jsProps_.GetObject();
            for (auto&& v : proxies_) {
                inp.DeleteProperty(v.first);
            }
            proxies_.clear();
        }
        jsProps_.Reset();

        SetNativeObject(nullptr, false);
        SetNativeObject(nullptr, true);

        scene_.Reset();
    }
}

void* SceneComponentJS::GetInstanceImpl(uint32_t id)
{
    if (id == SceneComponentJS::ID) {
        return this;
    }
    return nullptr;
}
void SceneComponentJS::Finalize(napi_env env)
{
    DisposeNative(nullptr);
    BaseObject<SceneComponentJS>::Finalize(env);
}
SceneComponentJS::SceneComponentJS(napi_env e, napi_callback_info i) : BaseObject<SceneComponentJS>(e, i)
{
    LOG_V("SceneComponentJS ++");
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        return;
    }
    scene_ = { NapiApi::Object(fromJs.Arg<0>()) };
    NapiApi::Object node = fromJs.Arg<1>();
    META_NS::IObject::Ptr native = GetNativeObjectParam<META_NS::IObject>(node);

    NapiApi::Object meJs(fromJs.This());

    BASE_NS::string name = native->GetName();
    meJs.Set("name", name);
    AddProperties(meJs, native);

    SetNativeObject(native, false);
}
SceneComponentJS::~SceneComponentJS()
{
    LOG_V("SceneComponentJS --");
    DisposeNative(nullptr);
}

void SceneComponentJS::AddProperties(NapiApi::Object meJs, const META_NS::IObject::Ptr& obj)
{
    auto comp = interface_cast<SCENE_NS::IComponent>(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    if (!comp || !meta) {
        return;
    }
    comp->PopulateAllProperties();

    NapiApi::Object jsProps(meJs.GetEnv());
    BASE_NS::vector<napi_property_descriptor> napi_descs;
    for (auto&& p : meta->GetProperties()) {
        if (auto proxy = PropertyToProxy(scene_.GetObject(), jsProps, p)) {
            auto res = proxies_.insert_or_assign(SCENE_NS::PropertyName(p->GetName()), proxy);
            napi_descs.push_back(CreateProxyDesc(res.first->first.c_str(), BASE_NS::move(proxy)));
        }
    }

    if (!napi_descs.empty()) {
        napi_define_properties(jsProps.GetEnv(), jsProps.ToNapiValue(), napi_descs.size(), napi_descs.data());
    }

    jsProps_ = NapiApi::StrongRef(jsProps);
    meJs.Set("properties", jsProps_.GetValue());
}
