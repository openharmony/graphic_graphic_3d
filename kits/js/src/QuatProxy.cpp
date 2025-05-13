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

#include "QuatProxy.h"

#include <napi_api.h>
QuatProxy::QuatProxy(napi_env env, META_NS::Property<BASE_NS::Math::Quat> prop) : ObjectPropertyProxy(prop)
{
    Create(env, "Quaternion");
    Hook("x");
    Hook("y");
    Hook("z");
    Hook("w");
}
QuatProxy::~QuatProxy()
{
    Reset();
}

void QuatProxy::SetValue(const BASE_NS::Math::Quat& v)
{
    META_NS::SetValue(GetProperty<BASE_NS::Math::Quat>(), v);
}
void QuatProxy::SetMemberValue(NapiApi::FunctionContext<>& cb, BASE_NS::string_view memb)
{
    NapiApi::FunctionContext<float> info(cb);
    if (info) {
        auto p = GetProperty<BASE_NS::Math::Quat>();
        auto value = META_NS::GetValue(p);
        float val = info.Arg<0>();
        bool changed = false;
        if ((memb == "x") && (val != value.x)) {
            value.x = val;
            changed = true;
        } else if ((memb == "y") && (val != value.y)) {
            value.y = val;
            changed = true;
        } else if ((memb == "z") && (val != value.z)) {
            value.z = val;
            changed = true;
        } else if ((memb == "w") && (val != value.w)) {
            value.w = val;
            changed = true;
        }
        if (changed) {
            META_NS::SetValue(p, value);
        }
    }
}
napi_value QuatProxy::GetMemberValue(const NapiApi::Env info, BASE_NS::string_view memb)
{
    float res;
    auto value = META_NS::GetValue(GetProperty<BASE_NS::Math::Quat>());
    if (memb == "x") {
        res = value.x;
    } else if (memb == "y") {
        res = value.y;
    } else if (memb == "z") {
        res = value.z;
    } else if (memb == "w") {
        res = value.w;
    } else {
        // invalid member?
        return info.GetUndefined();
    }
    return info.GetNumber(res);
}

void QuatProxy::SetValue(NapiApi::Object obj)
{
    auto x = obj.Get<float>("x");
    auto y = obj.Get<float>("y");
    auto z = obj.Get<float>("z");
    auto w = obj.Get<float>("w");
    if (x.IsValid() && y.IsValid() && z.IsValid() && w.IsValid()) {
        SetValue({ x, y, z, w });
    }
}
