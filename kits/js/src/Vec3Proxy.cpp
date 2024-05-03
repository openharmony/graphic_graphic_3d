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

#include "Vec3Proxy.h"
Vec3Proxy::Vec3Proxy(napi_env env, META_NS::Property<BASE_NS::Math::Vec3> prop) : PropertyProxy(prop)
{
    // Construct a "Lume::Vec3" instance
    Create(env, "Vec3");
    // hook to the objects members (set custom get/set)
    Hook("x");
    Hook("y");
    Hook("z");
    SyncGet();
}
Vec3Proxy::~Vec3Proxy() {}
void Vec3Proxy::UpdateLocalValues()
{
    // executed in javascript thread (locks handled outside)
    value = META_NS::Property<BASE_NS::Math::Vec3>(prop_)->GetValue();
}
void Vec3Proxy::UpdateRemoteValues()
{
    // executed in engine thread (locks handled otside)
    META_NS::Property<BASE_NS::Math::Vec3>(prop_)->SetValue(value);
}
void Vec3Proxy::SetValue(const BASE_NS::Math::Vec3& v)
{
    // currently executed in engine thread, hence the locks.
    duh.Lock();
    if (value != v) {
        value = v;
        ScheduleUpdate();
    }
    duh.Unlock();
}
void Vec3Proxy::SetValue(NapiApi::FunctionContext<>& cb, BASE_NS::string_view memb)
{
    // should be executed in the javascript thread.
    NapiApi::FunctionContext<float> info(cb.GetEnv(), cb.GetInfo());
    float val = info.Arg<0>();
    duh.Lock();
    if ((memb == "x") && (val != value.x)) {
        value.x = val;
        ScheduleUpdate();
    } else if ((memb == "y") && (val != value.y)) {
        value.y = val;
        ScheduleUpdate();
    } else if ((memb == "z") && (val != value.z)) {
        value.z = val;
        ScheduleUpdate();
    }
    duh.Unlock();
}
napi_value Vec3Proxy::GetValue(NapiApi::FunctionContext<>& cb, BASE_NS::string_view memb)
{
    // should be executed in the javascript thread.
    float res;
    duh.Lock();
    if (memb == "x") {
        res = value.x;
    } else if (memb == "y") {
        res = value.y;
    } else if (memb == "z") {
        res = value.z;
    } else {
        // invalid member?
        duh.Unlock();
        return {};
    }
    duh.Unlock();
    napi_value value;
    napi_status status = napi_create_double(cb.GetEnv(), res, &value);
    return value;
}

bool Vec3Proxy::SetValue(NapiApi::Object obj)
{
    auto x = obj.Get<float>("x");
    auto y = obj.Get<float>("y");
    auto z = obj.Get<float>("z");
    if (x.IsValid() && y.IsValid() && z.IsValid()) {
        BASE_NS::Math::Vec3 v(x, y, z);
        SetValue(v);
        return true;
    }
    return false;
}