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

#ifndef OHOS_RENDER_3D_COLOR_PROXY_H
#define OHOS_RENDER_3D_COLOR_PROXY_H
#include <meta/interface/property/property.h>
#include <scene_plugin/interface/compatibility.h>

#include "PropertyProxy.h"
class ColorProxy : public PropertyProxy {
    SCENE_NS::Color value;
    void UpdateLocalValues() override;
    void UpdateRemoteValues() override;

public:
    void SetValue(const SCENE_NS::Color& v);
    ColorProxy(napi_env env, META_NS::Property<SCENE_NS::Color> prop);
    ~ColorProxy() override;
    bool SetValue(NapiApi::Object obj) override;
    void SetValue(NapiApi::FunctionContext<>& info, BASE_NS::string_view memb) override;
    napi_value GetValue(NapiApi::FunctionContext<>& info, BASE_NS::string_view memb) override;
};

#endif // OHOS_RENDER_3D_COLOR_PROXY_H