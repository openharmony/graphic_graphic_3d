/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "ObjectProxy.h"

#include <meta/interface/property/property.h>

NapiApi::Object ObjectProxy::GetObject() const
{
    return obj_.GetObject();
}

ObjectProxy::operator NapiApi::Object() const
{
    return obj_.GetObject();
}

void ObjectProxy::Init(napi_env env, const BASE_NS::vector<PropertyMapping>& mappings, NapiApi::Object scene)
{
    auto obj = NapiApi::Object { env };
    obj_ = NapiApi::StrongRef { obj };

    mappings_.reserve(mappings.size());
    for (auto&& m : mappings) {
        if (const auto proxy = PropertyToProxy(scene, obj, m.nativeProp)) {
            // mappings_ must never reallocate, as we pass a pointer to its contents.
            mappings_.emplace_back(Mapping { m.jsName, proxy, m.scaleToNative, m.scaleToJs });
            obj.AddProperty(ObjectProxy::CreateProxyDesc(m.jsName.data(), &mappings_.back()));
        } else {
            LOG_E("Adding a property proxy for '%s' failed", m.jsName.data());
            assert(false);
        }
    }
}

void ObjectProxy::ReplaceObject(NapiApi::Object& replacement)
{
    if (auto obj = obj_.GetObject()) {
        if (obj.StrictEqual(replacement)) {
            return;
        }
    }
    UnbindObject();
    obj_ = NapiApi::StrongRef { replacement };
    BindObject();
}

void ObjectProxy::Reset()
{
    UnbindObject();
    obj_.Reset();
    mappings_.clear();
}

void ObjectProxy::UnbindObject()
{
    if (auto obj = obj_.GetObject()) {
        for (auto&& m : mappings_) {
            // Get value through getter to automatically apply scaling.
            const auto value = obj.Get(m.jsName);
            constexpr auto n = nullptr;
            // Convert the proxied property to a raw JS value (with no native getters or setters).
            // Can't use NapiApi::Object::Set, as it would go through the proxy setter.
            obj.AddProperty({ m.jsName.data(), n, n, n, n, value, napi_default_jsproperty, n });
        }
    }
}

void ObjectProxy::BindObject()
{
    if (auto obj = obj_.GetObject()) {
        for (auto&& m : mappings_) {
            const auto jsValue = obj.Get(m.jsName);
            obj.AddProperty(CreateProxyDesc(m.jsName.data(), &m));
            // Set the value originally held by the object through the setter to automatically apply scaling.
            obj.Set(m.jsName, jsValue);
        }
    }
}

napi_property_descriptor ObjectProxy::CreateProxyDesc(const char* name, Mapping* mapping)
{
    constexpr auto n = nullptr;
    napi_property_descriptor desc { name, n, n, n, n, n, napi_default_jsproperty, static_cast<void*>(mapping) };
    desc.getter = PropProxyGet;
    desc.setter = PropProxySet;
    return desc;
}

napi_value ObjectProxy::PropProxyGet(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<> ctx(e, i);
    auto mapping = static_cast<Mapping*>(ctx.GetData());
    const auto original = mapping->proxy->Value();
    return mapping->scaleToJs ? mapping->scaleToJs(ctx.GetEnv(), original) : original;
};

napi_value ObjectProxy::PropProxySet(napi_env e, napi_callback_info i)
{
    NapiApi::FunctionContext<> ctx(e, i);
    auto mapping = static_cast<Mapping*>(ctx.GetData());
    if (mapping->scaleToNative) {
        if (const auto jsValue = ctx.Value(0)) {
            ctx.SetValue(0, mapping->scaleToNative(ctx.GetEnv(), jsValue));
        }
    }
    mapping->proxy->SetValue(ctx);
    return ctx.GetUndefined();
};