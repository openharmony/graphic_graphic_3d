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

#ifndef VEC3_PROXY_H
#define VEC3_PROXY_H
#include <meta/interface/property/property.h>

#include <base/math/vector.h>

#include "PropertyProxy.h"
class Vec3Proxy : public ObjectPropertyProxy {
public:
    Vec3Proxy(napi_env env, META_NS::Property<BASE_NS::Math::Vec3> prop);
    ~Vec3Proxy() override;
    void SetValue(NapiApi::Object obj) override;

    static BASE_NS::Math::Vec3 ToNative(NapiApi::Object vec3, bool& success);
    static NapiApi::Object ToNapiObject(BASE_NS::Math::Vec3 vec3, napi_env env);
private:
    void SetValue(const BASE_NS::Math::Vec3& v);
    void SetMemberValue(NapiApi::FunctionContext<>& info, BASE_NS::string_view memb) override;
    napi_value GetMemberValue(const NapiApi::Env info, BASE_NS::string_view memb) override;
};
#endif
