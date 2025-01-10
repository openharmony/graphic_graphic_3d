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

#include "Vec2Proxy.h"

Vec2Proxy::Vec2Proxy(napi_env env, META_NS::Property<BASE_NS::Math::Vec2> prop) : ObjectPropertyProxy(prop)
{
    // Construct a "Lume::Vec2" instance
    Create(env, "Vec2");
    // hook to the objects members (set custom get/set)
    Hook("x");
    Hook("y");
    SyncGet();
}
Vec2Proxy::~Vec2Proxy()
{
    Reset();
}
void Vec2Proxy::UpdateLocalValues()
{
    // executed in javascript thread (locks handled outside)
    value = META_NS::Property<BASE_NS::Math::Vec2>(prop_)->GetValue();
}
void Vec2Proxy::UpdateRemoteValues()
{
    // executed in engine thread (locks handled outside)
    META_NS::Property<BASE_NS::Math::Vec2>(prop_)->SetValue(value);
}
void Vec2Proxy::SetValue(const BASE_NS::Math::Vec2& v)
{
    Lock();
    if (value != v) {
        value = v;
        ScheduleUpdate();
    }
    Unlock();
}
void Vec2Proxy::SetMemberValue(NapiApi::FunctionContext<>& cb, BASE_NS::string_view memb)
{
    // should be executed in the javascript thread.
    NapiApi::FunctionContext<float> info(cb.GetEnv(), cb.GetInfo());
    if (info) {
        float val = info.Arg<0>();
        Lock();
        if ((memb == "x") && (val != value.x)) {
            value.x = val;
            ScheduleUpdate();
        } else if ((memb == "y") && (val != value.y)) {
            value.y = val;
            ScheduleUpdate();
        }
        Unlock();
    }
}
napi_value Vec2Proxy::GetMemberValue(const NapiApi::Env cb, BASE_NS::string_view memb)
{
    // should be executed in the javascript thread.
    float res;
    Lock();
    if (memb == "x") {
        res = value.x;
    } else if (memb == "y") {
        res = value.y;
    } else {
        // invalid member?
        Unlock();
        return cb.GetUndefined();
    }
    Unlock();
    return cb.GetNumber(res);
}

void Vec2Proxy::SetValue(NapiApi::Object obj)
{
    auto x = obj.Get<float>("x");
    auto y = obj.Get<float>("y");
    if (x.IsValid() && y.IsValid()) {
        SetValue({ x, y });
    }
}

BASE_NS::Math::Vec2 Vec2Proxy::ToNative(NapiApi::Object vec2, bool& success)
{
    auto x = vec2.Get<float>("x");
    auto y = vec2.Get<float>("y");
    success = x.IsValid() && y.IsValid();
    return BASE_NS::Math::Vec2 { x, y };
}

NapiApi::Object Vec2Proxy::ToNapiObject(BASE_NS::Math::Vec2 vec2, napi_env env)
{
    auto screenCoordJs = NapiApi::Object(env);
    screenCoordJs.Set("x", NapiApi::Value<float>(env, vec2.x));
    screenCoordJs.Set("y", NapiApi::Value<float>(env, vec2.y));
    return screenCoordJs;
}
