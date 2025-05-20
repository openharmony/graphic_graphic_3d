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
    isColorType_ = true;
    Create(env, "Color");
    Hook("r");
    Hook("g");
    Hook("b");
    Hook("a");
}
ColorProxy::ColorProxy(napi_env env, META_NS::Property<BASE_NS::Math::Vec4> prop) : ObjectPropertyProxy(prop)
{
    isColorType_ = false;
    Create(env, "Color");
    Hook("r");
    Hook("g");
    Hook("b");
    Hook("a");
}
ColorProxy::~ColorProxy()
{
    Reset();
}
void ColorProxy::SetValue(const BASE_NS::Color& v)
{
    if (isColorType_) {
        META_NS::SetValue(GetProperty<BASE_NS::Color>(), v);
    } else {
        META_NS::SetValue(GetProperty<BASE_NS::Math::Vec4>(), BASE_NS::Math::Vec4(v.r, v.g, v.b, v.a));
    }
}
void ColorProxy::SetValue(const BASE_NS::Math::Vec4& v)
{
    if (isColorType_) {
        META_NS::SetValue(GetProperty<BASE_NS::Color>(), BASE_NS::Color(v.x, v.y, v.z, v.w));
    } else {
        META_NS::SetValue(GetProperty<BASE_NS::Math::Vec4>(), v);
    }
}

template<class T>
auto SetColorMemberValue(META_NS::Property<T>& p, BASE_NS::string_view memb, float val)
{
    auto value = META_NS::GetValue(p);

    bool changed = false;
    if ((memb == "r") && (val != value.x)) {
        value.x = val;
        changed = true;
    } else if ((memb == "g") && (val != value.y)) {
        value.y = val;
        changed = true;
    } else if ((memb == "b") && (val != value.z)) {
        value.z = val;
        changed = true;
    } else if ((memb == "a") && (val != value.w)) {
        value.w = val;
        changed = true;
    }
    if (changed) {
        META_NS::SetValue(p, value);
    }
}

template<class T>
bool GetColorMemberValue(const NapiApi::Env& info, META_NS::Property<T>& p, BASE_NS::string_view memb, float& value)
{
    auto val = META_NS::GetValue(p);
    if (memb == "r") {
        value = val.x;
        return true;
    }
    if (memb == "g") {
        value = val.y;
        return true;
    }
    if (memb == "b") {
        value = val.z;
        return true;
    }
    if (memb == "a") {
        value = val.w;
        return true;
    }
    return false;
}

void ColorProxy::SetMemberValue(NapiApi::FunctionContext<>& cb, BASE_NS::string_view memb)
{
    NapiApi::FunctionContext<float> info(cb);
    if (info) {
        float val = info.Arg<0>();
        if (isColorType_) {
            auto p = GetProperty<BASE_NS::Color>();
            SetColorMemberValue<BASE_NS::Color>(p, memb, val);
        } else {
            auto p = GetProperty<BASE_NS::Math::Vec4>();
            SetColorMemberValue<BASE_NS::Math::Vec4>(p, memb, val);
        }
    }
}
napi_value ColorProxy::GetMemberValue(const NapiApi::Env info, BASE_NS::string_view memb)
{
    float res;
    bool success = false;
    if (isColorType_) {
        auto p = GetProperty<BASE_NS::Color>();
        success = GetColorMemberValue<BASE_NS::Color>(info, p, memb, res);
    } else {
        auto p = GetProperty<BASE_NS::Math::Vec4>();
        success = GetColorMemberValue<BASE_NS::Math::Vec4>(info, p, memb, res);
    }
    return success ? info.GetNumber(res) : info.GetUndefined();
}
void ColorProxy::SetValue(NapiApi::Object obj)
{
    auto r = obj.Get<float>("r");
    auto g = obj.Get<float>("g");
    auto b = obj.Get<float>("b");
    auto a = obj.Get<float>("a");
    if (r.IsValid() && g.IsValid() && b.IsValid() && a.IsValid()) {
        SetValue(BASE_NS::Color(r, g, b, a));
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