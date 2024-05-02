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
ColorProxy::ColorProxy(napi_env env, META_NS::Property<SCENE_NS::Color> prop) : PropertyProxy(prop)
{
    Create(env, "Color");
    Hook("r");
    Hook("g");
    Hook("b");
    Hook("a");
    SyncGet();
}
ColorProxy::~ColorProxy() {}
void ColorProxy::UpdateLocalValues()
{
    // update local values. (runs in engine thread)
    value = META_NS::Property<SCENE_NS::Color>(prop_)->GetValue();
}
void ColorProxy::UpdateRemoteValues()
{
    META_NS::Property<SCENE_NS::Color>(prop_)->SetValue(value);
}
void ColorProxy::SetValue(const SCENE_NS::Color& v)
{
    duh.Lock();
    if (value != v) {
        value = v;
        ScheduleUpdate();
    }
    duh.Unlock();
}
void ColorProxy::SetValue(NapiApi::FunctionContext<>& cb, BASE_NS::string_view memb)
{
    NapiApi::FunctionContext<float> info(cb);
    float val = info.Arg<0>();
    duh.Lock();
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
    duh.Unlock();
}
napi_value ColorProxy::GetValue(NapiApi::FunctionContext<>& info, BASE_NS::string_view memb)
{
    float res;
    duh.Lock();
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
        duh.Unlock();
        return {};
    }
    duh.Unlock();
    napi_value value;
    napi_status status = napi_create_double(info, res, &value);
    return value;
}
bool ColorProxy::SetValue(NapiApi::Object obj)
{
    auto r = obj.Get<float>("r");
    auto g = obj.Get<float>("g");
    auto b = obj.Get<float>("b");
    auto a = obj.Get<float>("a");
    if (r.IsValid() && g.IsValid() && b.IsValid() && a.IsValid()) {
        SCENE_NS::Color c(r, g, b, a);
        SetValue(c);
        return true;
    }
    return false;
}