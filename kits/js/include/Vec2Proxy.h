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

#ifndef VEC2_PROXY_H
#define VEC2_PROXY_H
#include <meta/interface/property/property.h>

#include <base/math/vector.h>

#include "PropertyProxy.h"
class Vec2Proxy : public ObjectPropertyProxy {
public:
    Vec2Proxy(napi_env env, META_NS::Property<BASE_NS::Math::Vec2> prop);
    ~Vec2Proxy() override;
    void SetValue(NapiApi::Object v) override;

    static BASE_NS::Math::Vec2 ToNative(NapiApi::Object vec2, bool& success);
    static NapiApi::Object ToNapiObject(BASE_NS::Math::Vec2 vec2, napi_env env);

private:
    void SetValue(const BASE_NS::Math::Vec2& v);
    void SetMemberValue(NapiApi::FunctionContext<>& info, BASE_NS::string_view memb) override;
    napi_value GetMemberValue(const NapiApi::Env info, BASE_NS::string_view memb) override;
};
#endif
