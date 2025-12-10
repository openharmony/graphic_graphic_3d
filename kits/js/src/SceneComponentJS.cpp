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

#include <algorithm>

#include <meta/api/make_callback.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/property_events.h>
#include <scene/ext/intf_component.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_scene.h>

#include <render/intf_render_context.h>

#include "JsObjectCache.h"
#include "SceneJS.h"

#include <cctype>

using namespace SCENE_NS;

void SceneComponentJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> props;

    napi_value func;
    auto status = napi_define_class(env, "SceneComponent", NAPI_AUTO_LENGTH, BaseObject::ctor<SceneComponentJS>(),
        nullptr, props.size(), props.data(), &func);
    if (status != napi_ok) {
        LOG_E("export class failed in %s", __func__);
    }

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    if (mis) {
        mis->StoreCtor("SceneComponent", func);
    }
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

        if (auto native = GetNativeObject<META_NS::IObject>()) {
            DetachJsObj(native, "_JSW");
        }

        if (!jsProps_.IsEmpty()) {
            NapiApi::Object inp = jsProps_.GetObject();
            for (auto&& v : proxies_) {
                inp.DeleteProperty(v.first);
            }
            proxies_.clear();
        }
        jsProps_.Reset();

        UnsetNativeObject();

        scene_.Reset();
    }
}

void* SceneComponentJS::GetInstanceImpl(uint32_t id)
{
    if (id == SceneComponentJS::ID) {
        return static_cast<SceneComponentJS*>(this);
    }
    return BaseObject::GetInstanceImpl(id);
}
void SceneComponentJS::Finalize(napi_env env)
{
    DisposeNative(scene_.GetJsWrapper<SceneJS>());
    BaseObject::Finalize(env);
}
SceneComponentJS::SceneComponentJS(napi_env e, napi_callback_info i) : BaseObject(e, i)
{
    LOG_V("SceneComponentJS ++");
    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        return;
    }
    scene_ = { NapiApi::Object(fromJs.Arg<0>()) };
    NapiApi::Object node = fromJs.Arg<1>();
    const auto native = GetNativeObject();
    if (!native) {
        return;
    }

    NapiApi::Object meJs(fromJs.This());

    BASE_NS::string name = native->GetName();
    meJs.Set("name", name);
    AddProperties(meJs, native);

    SetNativeObject(native, PtrType::WEAK);
}
SceneComponentJS::~SceneComponentJS()
{
    LOG_V("SceneComponentJS --");
    DisposeNative(nullptr);
}

using GenericComponentMapping = BASE_NS::unordered_map<BASE_NS::string_view, BASE_NS::vector<BASE_NS::string_view>>;

const GenericComponentMapping& GetComponentMapping()
{
    static const GenericComponentMapping map = []() {
        GenericComponentMapping m;
        m["RenderConfigurationComponent"] = { "shadowType", "shadowQuality", "shadowSmoothness", "renderingFlags" };
        return m;
    }();
    return map;
}

bool ShouldExposeToJS(BASE_NS::string_view componentName, BASE_NS::string_view propertyName)
{
    // Weather system uses mixed casing, we cannot exclude properties based on first char
    if (componentName == "WeatherEffectComponent") {
        return true;
    }

    if (!propertyName.empty() && std::isupper(propertyName[0])) {
        // Some specific component wrappers in LumeScene expose ECS properties as LumeScene
        // level properties (whose name starts with an upper case letter). Do not expose those
        // to JS.
        return false;
    }

    const auto& map = GetComponentMapping();
    auto it = map.find(componentName);
    if (it == map.end()) {
        return true; // No mapping defined, expose all
    }
    const auto& mapping = it->second;
    return std::find(mapping.begin(), mapping.end(), propertyName) != mapping.end();
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
    BASE_NS::string componentName;
    if (auto o = interface_cast<META_NS::IObject>(comp)) {
        componentName = o->GetName();
    }
    for (auto&& p : meta->GetProperties()) {
        if (p) {
            const auto name = BASE_NS::string(SCENE_NS::PropertyName(p->GetName()));
            if (ShouldExposeToJS(componentName, name)) {
                if (auto proxy = PropertyToProxy(scene_.GetNapiObject(), jsProps, p)) {
                    auto res = proxies_.insert_or_assign(name, proxy);
                    napi_descs.push_back(CreateProxyDesc(res.first->first.c_str(), BASE_NS::move(proxy)));
                }
            }
        }
    }

    if (!napi_descs.empty()) {
        napi_define_properties(jsProps.GetEnv(), jsProps.ToNapiValue(), napi_descs.size(), napi_descs.data());
    }

    jsProps_ = NapiApi::StrongRef(jsProps);
    meJs.Set("property", jsProps_.GetValue());
}
