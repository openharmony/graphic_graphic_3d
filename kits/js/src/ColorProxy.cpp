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
#include "ColorProxy.h"
ColorProxy::ColorProxy(napi_env env, META_NS::Property<BASE_NS::Color> prop) : ObjectPropertyProxy(prop)
{
    Create(env, "Color");
    Hook("r");
    Hook("g");
    Hook("b");
    Hook("a");
    SyncGet();
}
ColorProxy::~ColorProxy()
{
    Reset();
}
void ColorProxy::UpdateLocalValues()
{
    // update local values. (runs in engine thread)
    value = META_NS::Property<BASE_NS::Color>(prop_)->GetValue();
}
void ColorProxy::UpdateRemoteValues()
{
    // update remote values. (runs in engine thread)
    META_NS::Property<BASE_NS::Color>(prop_)->SetValue(value);
}
void ColorProxy::SetValue(const BASE_NS::Color& v)
{
    Lock();
    if (value != v) {
        value = v;
        ScheduleUpdate();
    }
    Unlock();
}

void ColorProxy::SetMemberValue(NapiApi::FunctionContext<>& cb, BASE_NS::string_view memb)
{
    NapiApi::FunctionContext<float> info(cb);
    if (info) {
        float val = info.Arg<0>();
        Lock();
        if ((memb == "r") && (val != value.x)) {
            value.x = val;
            ScheduleUpdate();
        } else if ((memb == "g") && (val != value.y)) {
            value.y = val;
            ScheduleUpdate();
        } else if ((memb == "b") && (val != value.z)) {
            value.z = val;
            ScheduleUpdate();
        } else if ((memb == "a") && (val != value.w)) {
            value.w = val;
            ScheduleUpdate();
        }
        Unlock();
    }
}
napi_value ColorProxy::GetMemberValue(const NapiApi::Env info, BASE_NS::string_view memb)
{
    float res;
    Lock();
    if (memb == "r") {
        res = value.x;
    } else if (memb == "g") {
        res = value.y;
    } else if (memb == "b") {
        res = value.z;
    } else if (memb == "a") {
        res = value.w;
    } else {
        // invalid member?
        Unlock();
        return info.GetUndefined();
    }
    Unlock();
    return info.GetNumber(res);
}
void ColorProxy::SetValue(NapiApi::Object obj)
{
    auto r = obj.Get<float>("r");
    auto g = obj.Get<float>("g");
    auto b = obj.Get<float>("b");
    auto a = obj.Get<float>("a");
    if (r.IsValid() && g.IsValid() && b.IsValid() && a.IsValid()) {
        SetValue({ r, g, b, a });
    }
}

BASE_NS::Color ColorProxy::ToNative(NapiApi::Object colorJs, bool& success)
{
    auto r = colorJs.Get<float>("r");
    auto g = colorJs.Get<float>("g");
    auto b = colorJs.Get<float>("b");
    auto a = colorJs.Get<float>("a");
    success = r.IsValid() && g.IsValid() && b.IsValid() && a.IsValid();
    return BASE_NS::Color { r, g, b, a };
}

NapiApi::Object ColorProxy::ToNapiObject(BASE_NS::Color color, napi_env env)
{
    auto colorJs = NapiApi::Object(env);
    colorJs.Set("r", NapiApi::Value<float>(env, color.x));
    colorJs.Set("g", NapiApi::Value<float>(env, color.y));
    colorJs.Set("b", NapiApi::Value<float>(env, color.z));
    colorJs.Set("a", NapiApi::Value<float>(env, color.w));
    return colorJs;
}
