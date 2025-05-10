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

#ifndef QUAT_PROXY_H
#define QUAT_PROXY_H
#include <meta/interface/property/property.h>

#include <base/math/quaternion.h>

#include "PropertyProxy.h"
class QuatProxy : public ObjectPropertyProxy {
public:
    QuatProxy(napi_env env, META_NS::Property<BASE_NS::Math::Quat> prop);
    ~QuatProxy() override;
    void SetValue(NapiApi::Object obj) override;

protected:
    void SetValue(const BASE_NS::Math::Quat& v);
    void SetMemberValue(NapiApi::FunctionContext<>& info, BASE_NS::string_view memb) override;
    napi_value GetMemberValue(const NapiApi::Env info, BASE_NS::string_view memb) override;
};
#endif
