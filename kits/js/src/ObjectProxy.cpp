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
#include "ObjectProxy.h"

#include <meta/interface/intf_metadata.h>
#include <meta/interface/property/construct_array_property.h>
#include <meta/interface/property/property.h>

#include <base/math/vector.h>

#include "Vec2Proxy.h"

ObjectProxy::ObjectProxy(META_NS::IProperty::ConstPtr& property, napi_env env)
    : PropertyProxy(META_NS::IProperty::Ptr(property, const_cast<META_NS::IProperty*>(property.get()))), env_(env)
{
    napi_value value;
    napi_create_object(env, &value);
    obj = NapiApi::StrongRef { NapiApi::Object(env, value) };

    META_NS::ConstProperty<META_NS::IObject::Ptr> prop(GetProperty());

    if (prop) {
        auto meta = interface_cast<META_NS::IMetadata>(prop->GetValue());
        if (meta) {
            for (auto& p : meta->GetAllProperties()) {
                AddProperty(p, p->GetName());
            }
        }
    }
}

void ObjectProxy::AddProperty(BASE_NS::unique_ptr<PropertyProxy> property, BASE_NS::string_view member)
{
    duh.Lock();

    properties_.insert_or_assign(member, BASE_NS::move(property));

    Hook(BASE_NS::string(member));

    ScheduleUpdate();

    duh.Unlock();
}

void ObjectProxy::AddProperty(const META_NS::IProperty::Ptr& property, BASE_NS::string_view member)
{
    if (property->IsCompatible(META_NS::UidFromType<BASE_NS::Math::Vec2>())) {
        AddProperty(BASE_NS::make_unique<Vec2Proxy>(env_, property), member);
    }
}

void ObjectProxy::SetValue(NapiApi::FunctionContext<>& info, BASE_NS::string_view memb)
{
    duh.Lock();
    auto i = properties_.find(memb);
    if (i == properties_.end()) {
        return;
    }

    NapiApi::FunctionContext<NapiApi::Object> ctx(info.GetEnv(), info.GetInfo());

    if (i->second->GetProperty()->IsCompatible(META_NS::UidFromType<BASE_NS::Math::Vec2>())) {
        NapiApi::Object obj = ctx.Arg<0>();

        BASE_NS::Math::Vec2 vec(obj.Get<float>("x"), obj.Get<float>("y"));
        static_cast<Vec2Proxy&>(*i->second).SetValue(vec);

        ScheduleUpdate();
    }

    duh.Unlock();
}

napi_value ObjectProxy::GetValue(NapiApi::FunctionContext<>& info, BASE_NS::string_view memb)
{
    duh.Lock();
    auto i = properties_.find(memb);
    if (i == properties_.end()) {
        duh.Unlock();
        return {};
    }

    napi_value result = i->second->Value();

    duh.Unlock();

    return result;
}

void ObjectProxy::UpdateLocalValues() {}

void ObjectProxy::UpdateRemoteValues() {}
