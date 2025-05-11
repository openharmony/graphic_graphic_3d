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

#include "MorpherJS.h"

#include <meta/api/make_callback.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/property_events.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_scene.h>
#include <scene/ext/util.h>

#include <render/intf_render_context.h>

#include "SceneJS.h"

using namespace SCENE_NS;

void MorpherJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> props;

    napi_value func;
    auto status = napi_define_class(env, "Morpher", NAPI_AUTO_LENGTH, BaseObject::ctor<MorpherJS>(),
        nullptr, props.size(), props.data(), &func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, reinterpret_cast<void**>(&mis));
    mis->StoreCtor("Morpher", func);
}

napi_value MorpherJS::Dispose(NapiApi::FunctionContext<>& ctx)
{
    LOG_V("MorpherJS::Dispose");
    DisposeNative(nullptr);
    return {};
}
void MorpherJS::DisposeNative(void* scene)
{
    if (!disposed_) {
        disposed_ = true;
        LOG_V("MorpherJS::DisposeNative");

        /// destroy the target object.
        if (!targets_.IsEmpty()) {
            NapiApi::Object target = targets_.GetObject();
            while (!proxies_.empty()) {
                auto it = proxies_.begin();
                // removes hooks for meta property & jsproperty.
                target.DeleteProperty(it->first);
                // destroy the proxy.
                proxies_.erase(it);
            }
        }
        targets_.Reset();
        UnsetNativeObject();

        if (auto* sceneJS = static_cast<SceneJS*>(scene)) {
            sceneJS->ReleaseDispose(reinterpret_cast<uintptr_t>(&scene_));
        }
        scene_.Reset();
    }
}

void* MorpherJS::GetInstanceImpl(uint32_t id)
{
    if (id == MorpherJS::ID) {
        return this;
    }
    return nullptr;
}
void MorpherJS::Finalize(napi_env env)
{
    DisposeNative(scene_.GetObject().GetJsWrapper<SceneJS>());
    BaseObject::Finalize(env);
}
MorpherJS::MorpherJS(napi_env e, napi_callback_info i) : BaseObject(e, i)
{
    LOG_V("MorpherJS ++");
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        return;
    }
    scene_ = { NapiApi::Object(fromJs.Arg<0>()) };
    NapiApi::Object node = fromJs.Arg<1>();
    const auto native = GetNativeObject();

    NapiApi::Object meJs(fromJs.This());

    BASE_NS::string name = native->GetName();
    meJs.Set("name", name);
    AddProperties(meJs, native);

    SetNativeObject(native, PtrType::WEAK);
}
MorpherJS::~MorpherJS()
{
    LOG_V("MorpherJS --");
    DisposeNative(nullptr);
}

void MorpherJS::AddProperties(NapiApi::Object meJs, const META_NS::IObject::Ptr& obj)
{
    auto morph = interface_cast<SCENE_NS::IMorpher>(obj);
    if (!morph) {
        return;
    }

    NapiApi::Object targets(meJs.GetEnv());
    napi_env e = targets.GetEnv();

    BASE_NS::vector<napi_property_descriptor> targetProps;

    auto morphNames = morph->MorphNames();
    META_NS::ArrayProperty<float> morphWeights = morph->PropertyMorphWeights();

    for (META_NS::IArrayAny::IndexType i = 0; i < morphNames->GetSize(); i++) {
        auto name = morphNames->GetValueAt(i);

        if (name.empty()) {
            name = "key_" + BASE_NS::to_string(i);
        }
        auto proxt = PropertyToArrayProxy(scene_.GetObject(), targets, morphWeights, i);
        if (proxt) {
            auto res = proxies_.insert_or_assign(name, proxt);
            targetProps.push_back(CreateArrayProxyDesc(res.first->first.c_str(), BASE_NS::move(proxt)));
        }
    }

    if (!targetProps.empty()) {
        napi_define_properties(targets.GetEnv(), targets.ToNapiValue(), targetProps.size(), targetProps.data());
    }

    targets_ = NapiApi::StrongRef(targets);
    meJs.Set("targets", targets_.GetValue());
}
